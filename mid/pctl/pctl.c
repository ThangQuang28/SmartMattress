/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : pctl.c
 * @description     : Automatic pressure control for the 13 air chambers (RD 05.10.26).
 *                    Chambers above target +/- band exhaust together; inflating goes one chamber at a
 *                    time, P01 to P13 (phase 1: each reaches its target before the next starts; phase 2:
 *                    holding). Sensors followed while valves are open.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "pctl.h"
#include "valve.h"
#include "flash.h"

/* Define ----------------------------------------------------------------------------------------*/

#define PCTL_NVS_KEY_TARGET         "p_tgt"     /* New key: targets saved before this build are dropped. */
#define PCTL_NVS_KEY_BOOT           "p_boot"
#define PCTL_NVS_KEY_TIMEOUT        "p_tmo"
#define PCTL_NVS_KEY_TOL            "p_tol"
#define PCTL_NVS_KEY_POSTURE        "p_post"
#define PCTL_NO_CHAMBER             (-1)
#define PCTL_MSG_LEN                64U

typedef enum
{
    PCTL_ACTION_IDLE = 0,
    PCTL_ACTION_INFLATE,
    PCTL_ACTION_EXHAUST,
} pctl_action_t;

/* Private variables -----------------------------------------------------------------------------*/

/* RD levels 1..8 in kPa. */
static const float g_level_kpa[PCTL_LEVEL_NUM] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 17.0f};

/* Valve output of each chamber P01..P13 (output index = V number - 1, see pctl.h).
   User table (inflate-exhaust): P01 9-27, P02 10-28, P03 11-32, P04 12-31, P05 13-30, P06 14-29,
   P07 15-20, P08 16-19, P09 2-18, P10 1-17, P11 4-22, P12 5-23, P13 6-24. */
static const uint8_t g_inflate_out[PCTL_CHAMBER_NUM] = {8, 9, 10, 11, 12, 13, 14, 15, 1, 0, 3, 4, 5};
static const uint8_t g_exhaust_out[PCTL_CHAMBER_NUM] = {26, 27, 31, 30, 29, 28, 19, 18, 17, 16, 21,
                                                        22, 23};

static SemaphoreHandle_t g_lock = NULL;
static float g_target[PCTL_CHAMBER_NUM];
static float g_boot_kpa = PCTL_TARGET_MAX_KPA;   /* RD Pmd power-on level, set from the web app. */
static float g_tol_kpa = PCTL_START_KPA;     /* Band around each target, set from the web app. */
static uint32_t g_timeout_s = PCTL_ACTION_TIMEOUT_MS / 1000U;   /* Per chamber, 0 = no limit. */
static bool g_auto = false;
static bool g_boot_pending = false;     /* RD Pmd start waits for good sensor readings. */
static uint8_t g_boot_good = 0U;
static TickType_t g_boot_since = 0;
static bool g_fill = false;             /* Power-on fill to PCTL_BOOT_FILL_KPA running. */
static bool g_fill_done = false;        /* Fill done: waiting for the person to lie down. */
static bool g_on_bed = false;           /* Person confirmed: reopening the web app no longer refills until power-off. */
static float g_before[PCTL_CHAMBER_NUM];    /* Pressures when the fill was done (nobody lying). */
static uint32_t g_before_ok = 0U;
static uint8_t g_scan = 0U;             /* 0 none, 1 scanning after "Toi da nam", 2 done. */
static uint8_t g_scan_n = 0U;
static TickType_t g_scan_last = 0;
static float g_p1_ring[PCTL_P1_SECONDS][PCTL_CHAMBER_NUM];   /* Last 30 s, one sample a second. */
static uint32_t g_p1_ok[PCTL_P1_SECONDS];
static uint8_t g_p1_i = 0U;
static TickType_t g_p1_last = 0;
static bool g_p1_ready = false;     /* P1 taken at "San sang nam": the scan keeps it. */
static float g_after_sum[PCTL_CHAMBER_NUM];     /* Scans added up: P2 = mean (person lying). */
static uint8_t g_after_cnt[PCTL_CHAMBER_NUM];
static int8_t g_active = PCTL_NO_CHAMBER;
static pctl_action_t g_action = PCTL_ACTION_IDLE;
static pctl_action_t g_act[PCTL_CHAMBER_NUM];         /* What each chamber is doing now. */
static TickType_t g_act_start[PCTL_CHAMBER_NUM];
static TickType_t g_act_move[PCTL_CHAMBER_NUM];     /* Last time the pressure moved the right way. */
static float g_act_p[PCTL_CHAMBER_NUM];             /* Pressure at that time. */
static float g_rate[PCTL_CHAMBER_NUM][2];           /* kPa per ms (inflate, exhaust), 0 = not known. */
static float g_inf_lead[PCTL_CHAMBER_NUM];         /* Inflate: stop the pump this far below target. */
static bool g_exh_ended[PCTL_CHAMBER_NUM];         /* The last action was an exhaust. */
static TickType_t g_idle_since[PCTL_CHAMBER_NUM];   /* When the chamber's last inflate/exhaust ended. */
static uint32_t g_keep = 0U;                       /* Chambers whose valves stayed open after the last step. */
static uint32_t g_short_us[PCTL_CHAMBER_NUM];      /* Last timed opening of each chamber, 0 = none. */
static float g_short_p[PCTL_CHAMBER_NUM];          /* Pressure before it. */
static uint32_t g_pulse_us[PCTL_CHAMBER_NUM][2];    /* Shortest timed opening that works (inflate, exhaust). */
static uint32_t g_fault_mask = 0U;
static uint32_t g_enable_mask = (1UL << PCTL_CHAMBER_NUM) - 1UL;    /* Chambers auto mode adjusts. */
static TickType_t g_fault_since = 0;
static uint8_t g_phase = 1U;        /* 1: first pass P01..P13, 2: holding passes (see pctl_step). */
static uint8_t g_seq = 0U;          /* Chamber the pass is at. */
static uint8_t g_seq_cnt = 0U;      /* Chambers passed since the phase 1 start (2 rounds = 26). */
static float g_seq_last[PCTL_CHAMBER_NUM] = {NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
static char g_msg[PCTL_MSG_LEN] = "";

/* Recent scans of each chamber (see PCTL_FILT_NUM) and what an idle chamber did since it settled. */
static float g_hist[PCTL_CHAMBER_NUM][PCTL_FILT_NUM];
static uint8_t g_hist_n[PCTL_CHAMBER_NUM];
static uint8_t g_hist_i[PCTL_CHAMBER_NUM];
static float g_noise[PCTL_CHAMBER_NUM];             /* Spread with valves closed, kPa. */
static float g_hold_p[PCTL_CHAMBER_NUM];            /* Settled pressure, NAN = not yet. */
static bool g_hold_pump[PCTL_CHAMBER_NUM];          /* Pump ran for another chamber since then. */
static uint32_t g_leak_mask = 0U;
static uint32_t g_noisy_mask = 0U;

/* Saved sets and the RD step 6 posture check. */
static float g_pre_kpa[PCTL_PRESET_NUM][PCTL_CHAMBER_NUM];
static uint32_t g_pre_en[PCTL_PRESET_NUM];
static bool g_pre_ok[PCTL_PRESET_NUM];
static int8_t g_cur_set = PCTL_SET_NONE;
static bool g_pcn_pending = false;   /* Weight entered: pause once every chamber reaches Pcn. */
static bool g_ready = false;         /* Pcn reached and holding paused: person may lie down. */
static bool g_eval = false;
static TickType_t g_cycle_start = 0;
static TickType_t g_eval_start = 0;
static int8_t g_cur_shift = 0;
static bool g_posture_on = true;
static uint32_t g_match_period_s = PCTL_MATCH_PERIOD_MS / 1000U;
static uint32_t g_match_window_s = PCTL_MATCH_WINDOW_MS / 1000U;
static uint32_t g_eval_shift[PCTL_CAND_MAX];        /* Shifts still matching this window. */
static float g_eval_dev[PCTL_CAND_MAX][PCTL_SHIFT_NUM];   /* Deviation added up per shift. */
static float g_exp_kpa[PCTL_CAND_MAX][PCTL_CHAMBER_NUM];  /* Experience mode: chosen named sets. */
static uint8_t g_exp_slot[PCTL_CAND_MAX];           /* Their slots on the web (for the JSON). */
static uint8_t g_exp_n = 0U;                        /* 0 = experience mode off. */
static int8_t g_exp_cur = 0;
static TickType_t g_exp_start = 0;             /* Experience started (time shown on the web). */
static bool g_eval_out = false;                     /* Running back/side set off the whole window. */
static bool g_eval_err = false;                     /* Last check: back and side both matched. */
static float g_eval_dist[PCTL_CAND_MAX];            /* Last check, -1 = no match / not tried. */

typedef struct
{
    uint32_t on;
    uint32_t period_s;
    uint32_t window_s;
} pctl_posture_cfg_t;

static const char *TAG = "PCTL";

/* Private function prototypes -------------------------------------------------------------------*/

static void pctl_stop_locked(void);
static void pctl_restart_seq_locked(void);
static void pctl_start_locked(uint8_t k, pctl_action_t action, float p, TickType_t now);
static void pctl_targets_changed_locked(void);
static void pctl_clamp_targets_locked(void);
static uint32_t pctl_fit_shifts(uint8_t i);
static uint32_t pctl_period_s(void);
static uint32_t pctl_window_s(void);
static void pctl_apply_cand_locked(uint8_t i, int8_t shift);
static bool pctl_posture_step_locked(const float *p_kpa, uint32_t ok_mask);
static void pctl_apply_set_locked(uint8_t slot, int8_t shift);
static uint32_t pctl_valves(uint32_t chambers);
static zerr_t pctl_track_locked(const float *p_kpa, uint32_t running, uint32_t *p_keep,
                                uint32_t *p_moved);
static float pctl_filtered(uint8_t k, float *p_spread);
static float pctl_band(uint8_t k);
static float pctl_hold_off(uint8_t k);
static float pctl_need_move(uint8_t k);
static void pctl_watch_idle_locked(uint8_t k, float p, float spread);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t pctl_open(void)
{
    size_t len = 0U;

    if (NULL != g_lock)
    {
        return ZERR_OK;
    }

    g_lock = xSemaphoreCreateMutex();
    if (NULL == g_lock)
    {
        return ZERR_MEM_OUT;
    }

    /* Targets saved once the user chose a level, a weight or own targets. None saved: RD "Pmd". */
    bool saved = (ZERR_OK == flash_read(PCTL_NVS_KEY_TARGET, g_target, sizeof(g_target), &len)) &&
                 (sizeof(g_target) == len);
    for (uint8_t i = 0U; saved && (i < PCTL_CHAMBER_NUM); i++)
    {
        saved = (g_target[i] >= 0.0f) && (g_target[i] <= PCTL_TARGET_MAX_KPA);
    }

    if ((ZERR_OK != flash_read(PCTL_NVS_KEY_BOOT, &g_boot_kpa, sizeof(g_boot_kpa), &len)) ||
        (sizeof(g_boot_kpa) != len) || !(g_boot_kpa >= 0.0f) || (g_boot_kpa > PCTL_TARGET_MAX_KPA))
    {
        g_boot_kpa = PCTL_TARGET_MAX_KPA;
    }
    if ((ZERR_OK != flash_read(PCTL_NVS_KEY_TIMEOUT, &g_timeout_s, sizeof(g_timeout_s), &len)) ||
        (sizeof(g_timeout_s) != len) || (g_timeout_s > PCTL_TIMEOUT_MAX_S))
    {
        g_timeout_s = PCTL_ACTION_TIMEOUT_MS / 1000U;
    }
    if ((ZERR_OK != flash_read(PCTL_NVS_KEY_TOL, &g_tol_kpa, sizeof(g_tol_kpa), &len)) ||
        (sizeof(g_tol_kpa) != len) || !(g_tol_kpa >= PCTL_TOL_MIN_KPA) || (g_tol_kpa > PCTL_TOL_MAX_KPA))
    {
        g_tol_kpa = PCTL_START_KPA;
    }
    pctl_posture_cfg_t cfg;
    if ((ZERR_OK == flash_read(PCTL_NVS_KEY_POSTURE, &cfg, sizeof(cfg), &len)) && (sizeof(cfg) == len) &&
        (cfg.period_s >= PCTL_PERIOD_MIN_S) && (cfg.period_s <= PCTL_PERIOD_MAX_S) &&
        (cfg.window_s >= PCTL_WINDOW_MIN_S) && (cfg.window_s <= PCTL_WINDOW_MAX_S))
    {
        g_posture_on = (0U != cfg.on);
        g_match_period_s = cfg.period_s;
        g_match_window_s = cfg.window_s;
    }

    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        g_hold_p[i] = NAN;
    }
    for (uint8_t i = 0U; i < PCTL_CAND_MAX; i++)
    {
        g_eval_dist[i] = -1.0f;
    }

    g_auto = false;
#if PCTL_AUTO_AT_BOOT
    /* Power-on (user rule 27.10): every chamber to PCTL_BOOT_FILL_KPA (at most Pmax), then ask the
       person to lie down. Saved targets are not used; a new level, weight or table target takes
       over when set. */
    saved = false;
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        g_target[i] = fminf(PCTL_BOOT_FILL_KPA, g_boot_kpa);
    }
    g_enable_mask = (1UL << PCTL_CHAMBER_NUM) - 1UL;
    g_boot_pending = true;
    g_fill = true;
    g_boot_since = xTaskGetTickCount();
    (void)snprintf(g_msg, sizeof(g_msg), "power-on: waiting for sensors, then all to %.1f kPa",
                   (double)g_target[0]);
#else
    if (!saved)
    {
        for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
        {
            g_target[i] = g_level_kpa[PCTL_LEVEL_NUM - 1U];
        }
    }
#endif
    ESP_LOGI(TAG, "auto %s (power-on: %s, %.1f kPa), valves: user table P01 V9/V27 .. P13 V6/V24, "
             "pump V%u", g_boot_pending ? "starts after sensors are ready" : "off",
             saved ? "saved targets" : "all to", (double)g_target[0],
             (unsigned int)(PCTL_PUMP_OUTPUT + 1U));

    return ZERR_OK;
}

void pctl_step(const float *p_kpa, uint32_t ok_mask)
{
    if (NULL == g_lock)
    {
        return;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);

    /* Filtered readings: single AGR12 reads of some chambers jump by several tenths (P07). */
    float f[PCTL_CHAMBER_NUM];
    float spread[PCTL_CHAMBER_NUM];

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (0U != (ok_mask & (1UL << k)))
        {
            g_hist[k][g_hist_i[k]] = p_kpa[k];
            g_hist_i[k] = (uint8_t)((g_hist_i[k] + 1U) % PCTL_FILT_NUM);
            g_hist_n[k] = (g_hist_n[k] < PCTL_FILT_NUM) ? (uint8_t)(g_hist_n[k] + 1U) : g_hist_n[k];
        }
        f[k] = (0U != g_hist_n[k]) ? pctl_filtered(k, &spread[k]) : p_kpa[k];
        spread[k] = (0U != g_hist_n[k]) ? spread[k] : 0.0f;
    }

    if (g_boot_pending)
    {
        uint32_t all = (1UL << PCTL_CHAMBER_NUM) - 1UL;

        g_boot_good = ((ok_mask & all) == all) ? (uint8_t)(g_boot_good + 1U) : 0U;
        /* A sensor that never reads must not keep every other chamber from its power-on level. */
        bool waited = (0U != (ok_mask & all)) &&
                      ((xTaskGetTickCount() - g_boot_since) > pdMS_TO_TICKS(PCTL_BOOT_WAIT_MS));
        if ((g_boot_good >= PCTL_BOOT_GOOD_SCANS) || waited)
        {
            g_boot_pending = false;
            g_auto = true;
            g_fault_mask = 0U;
            g_cur_set = PCTL_SET_NONE;
            pctl_restart_seq_locked();
            g_cycle_start = xTaskGetTickCount();   /* First posture check one period after Pmax. */
            (void)snprintf(g_msg, sizeof(g_msg), "power-on: holding targets (P01 %.1f kPa)",
                           (double)g_target[0]);
            ESP_LOGI(TAG, "%s (sensors ok 0x%04lX)", g_msg, (unsigned long)(ok_mask & all));
        }
    }

    /* User rule (30.10): keep the last PCTL_P1_SECONDS one-second readings; P1 is their mean at the
       moment the person says they are ready to lie down (pctl_user_ready). */
    if ((xTaskGetTickCount() - g_p1_last) >= pdMS_TO_TICKS(1000U))
    {
        g_p1_last = xTaskGetTickCount();
        memcpy(g_p1_ring[g_p1_i], p_kpa, sizeof(g_p1_ring[0]));
        g_p1_ok[g_p1_i] = ok_mask;
        g_p1_i = (uint8_t)((g_p1_i + 1U) % PCTL_P1_SECONDS);
    }

    /* User rule (27.10): after "Toi da nam", PCTL_SCAN_NUM scans PCTL_SCAN_PERIOD_MS apart. P1 =
       pressures at fill time, P2 = mean of the scans; the web finds A, B on P2 - P1. */
    if ((1U == g_scan) && ((xTaskGetTickCount() - g_scan_last) >= pdMS_TO_TICKS(PCTL_SCAN_PERIOD_MS)))
    {
        g_scan_last = xTaskGetTickCount();
        for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
        {
            if (0U != (ok_mask & (1UL << k)))
            {
                g_after_sum[k] += p_kpa[k];
                g_after_cnt[k]++;
            }
        }
        g_scan_n++;
        if (g_scan_n >= PCTL_SCAN_NUM)
        {
            g_scan = 2U;
            (void)snprintf(g_msg, sizeof(g_msg), "scan done: pressure difference ready");
            ESP_LOGI(TAG, "%s", g_msg);
        }
    }

    if (!g_auto)
    {
        /* Valves by hand (e.g. "Nap tat ca" on the web): the 20 kPa limit still shuts everything. */
        for (uint8_t i = 0U; (i < PCTL_CHAMBER_NUM) && (0U != valve_get_mask()); i++)
        {
            if ((0U != (ok_mask & (1UL << i))) && (p_kpa[i] > PCTL_OVERPRESSURE_KPA))
            {
                (void)valve_all_off();
                (void)snprintf(g_msg, sizeof(g_msg), "P%02u over %.0f kPa: all valves off",
                               (unsigned int)(i + 1U), (double)PCTL_OVERPRESSURE_KPA);
                ESP_LOGE(TAG, "%s", g_msg);
            }
        }
        xSemaphoreGive(g_lock);
        return;
    }

    /* RD step 6: during the 1-minute check nothing is adjusted. */
    if (pctl_posture_step_locked(p_kpa, ok_mask))
    {
        xSemaphoreGive(g_lock);
        return;
    }

    /* Safety first: any readable chamber above the limit stops everything. */
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        if ((0U != (ok_mask & (1UL << i))) && (p_kpa[i] > PCTL_OVERPRESSURE_KPA))
        {
            pctl_stop_locked();
            g_auto = false;
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u over %.0f kPa: all off, auto off",
                           (unsigned int)(i + 1U), (double)PCTL_OVERPRESSURE_KPA);
            ESP_LOGE(TAG, "%s", g_msg);
            xSemaphoreGive(g_lock);
            return;
        }
    }

    /* The chamber being adjusted keeps going until it is within its band of the target. It is let
       go when its sensor fails, it stops moving or it runs too long. */
    TickType_t now = xTaskGetTickCount();
    int8_t inflating = PCTL_NO_CHAMBER;
    uint32_t wait = 0U;     /* Chambers settling after an opening: valves closed this step. */

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        inflating = (PCTL_ACTION_INFLATE == g_act[k]) ? (int8_t)k : inflating;
    }

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (PCTL_ACTION_IDLE == g_act[k])
        {
            continue;
        }

        bool done = true;
        bool inflate = (PCTL_ACTION_INFLATE == g_act[k]);
        float p = f[k];
        float band = pctl_band(k);

        if (0U == (ok_mask & (1UL << k)))
        {
            g_fault_mask |= (1UL << k);
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u sensor error: skipped", (unsigned int)(k + 1U));
        }
        else if (inflate && (0U != (g_keep & (1UL << k))))
        {
            done = false;       /* User rule (30.10): pump still running, the follow stops it at target. */
        }
        else if (!inflate && (0U == (g_keep & (1UL << k))) && (now != g_act_start[k]))
        {
            /* User rule (30.10): an exhaust command ends when its valve closes; the chamber then rests
               PCTL_SEQ_SETTLE_MS for the pressure to recover and is judged again against +/-0,3 kPa. */
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u exhaust done (%.1f kPa): check again in %u s",
                           (unsigned int)(k + 1U), (double)p, (unsigned int)(PCTL_SEQ_SETTLE_MS / 1000U));
        }
        else if ((g_hist_n[k] < PCTL_SETTLE_SCANS) && (fabsf(p - g_target[k]) <= PCTL_FAR_KPA))
        {
            done = false;       /* Near the target: judge only after a few scans with valves closed. */
            wait |= (1UL << k);
        }
        else if (fabsf(p - g_target[k]) <= (inflate ? fmaxf(band, PCTL_INF_ACCEPT_KPA) : band))
        {
            g_hold_p[k] = NAN;
            if (inflate)
            {
                /* User rule (30.10): learn where to stop the pump so the settled reading lands on
                   the target (inflate carries on only a little after the pump stops). */
                float lead = g_inf_lead[k] + (PCTL_INF_LEAD_GAIN * (p - g_target[k]));

                g_inf_lead[k] = (lead > PCTL_INF_LEAD_MAX_KPA) ? PCTL_INF_LEAD_MAX_KPA
                                : ((lead < -PCTL_INF_LEAD_MAX_KPA) ? -PCTL_INF_LEAD_MAX_KPA : lead);
            }
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u reached %.2f kPa (+/-%.2f) in %lu ms",
                           (unsigned int)(k + 1U), (double)p, (double)band,
                           (unsigned long)((now - g_act_start[k]) * portTICK_PERIOD_MS));
        }
        else if (inflate ? (p > g_target[k]) : (p < g_target[k]))
        {
            /* Went past the band: correct it the other way. An exhaust that went too far ends here and
               the chamber is inflated back only PCTL_SEQ_SETTLE_MS later (user 30.10: let the
               pressure settle after an exhaust before inflating). */
            if (!inflate)
            {
                (void)snprintf(g_msg, sizeof(g_msg), "P%02u past target (%.1f): inflate after %u s",
                               (unsigned int)(k + 1U), (double)p, (unsigned int)(PCTL_SEQ_SETTLE_MS / 1000U));
            }
            else
            {
                done = false;
                g_act[k] = PCTL_ACTION_EXHAUST;
                inflating = PCTL_NO_CHAMBER;
                g_short_us[k] = 0U;
                g_act_move[k] = now;
                g_act_p[k] = p;
                (void)snprintf(g_msg, sizeof(g_msg), "P%02u past target (%.1f): exhaust -> %.1f kPa",
                               (unsigned int)(k + 1U), (double)p, (double)g_target[k]);
                ESP_LOGI(TAG, "%s", g_msg);
            }
        }
        else if (inflate ? (p >= (g_act_p[k] + pctl_need_move(k))) : (p <= (g_act_p[k] - pctl_need_move(k))))
        {
            g_act_p[k] = p;         /* Still moving toward the target: keep going. */
            g_act_move[k] = now;
            done = false;
        }
        else if ((now - g_act_move[k]) > pdMS_TO_TICKS(PCTL_NO_MOVE_MS))
        {
            /* User rule (26.10): a chamber is never left before it reaches its target (the pump
               stays on it), only reported. Sensor error, 20 kPa and the web time limit still stop it. */
            g_act_move[k] = now;
            done = false;
            ESP_LOGW(TAG, "P%02u no change in %u s while %s (valve/leak?): still %s",
                     (unsigned int)(k + 1U), (unsigned int)(PCTL_NO_MOVE_MS / 1000U),
                     inflate ? "inflating" : "exhausting", inflate ? "inflating" : "exhausting");
        }
        else
        {
            done = false;
        }

        if (!done && (0U != g_timeout_s) && ((now - g_act_start[k]) > pdMS_TO_TICKS(g_timeout_s * 1000U)))
        {
            done = true;
            g_fault_mask |= (1UL << k);
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u not reached in %u s: skipped",
                           (unsigned int)(k + 1U), (unsigned int)g_timeout_s);
        }

        if (done)
        {
            if (0U != (g_fault_mask & (1UL << k)))
            {
                g_fault_since = now;
            }
            ESP_LOGI(TAG, "%s", g_msg);
            if ((2U == g_phase) && (PCTL_ACTION_INFLATE == g_act[k]))
            {
                g_seq = (uint8_t)((k + 1U) % PCTL_CHAMBER_NUM);     /* Pump turn goes on in order. */
            }
            inflating = ((int8_t)k == inflating) ? PCTL_NO_CHAMBER : inflating;
            g_exh_ended[k] = (PCTL_ACTION_EXHAUST == g_act[k]);
            g_act[k] = PCTL_ACTION_IDLE;
            g_idle_since[k] = now;
        }
    }

    if ((0U != g_fault_mask) && ((now - g_fault_since) > pdMS_TO_TICKS(PCTL_FAULT_RETRY_MS)))
    {
        g_fault_mask = 0U;
        ESP_LOGI(TAG, "retrying skipped chambers");
    }

    /* Idle chambers: noise and leak watch. */
    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if ((PCTL_ACTION_IDLE == g_act[k]) && (0U != (ok_mask & (1UL << k))))
        {
            pctl_watch_idle_locked(k, f[k], spread[k]);
        }
    }

    /* User rule (26.10): exhaust together, inflate in order. Every chamber above its band exhausts at
       once; one chamber at a time inflates (shared pump), P01 first. Phase 1: a pass P01..P13, each
       chamber below its band inflated to the target before the next one starts. Phase 2 (until auto
       mode is stopped): all 13 chambers checked every scan; chambers below their band take the pump
       in turn, P01..P13 and round again, with hysteresis. Nothing is skipped in phase 2 except for a
       sensor error, so every target is kept. */
    bool seq = (1U == g_phase);
    uint32_t ready = 0U;    /* Idle, adjustable chambers with readings good enough to decide on. */

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if ((PCTL_ACTION_IDLE != g_act[k]) || (0U == (g_enable_mask & (1UL << k))) ||
            (0U != (g_fault_mask & (1UL << k))) || (0U == (ok_mask & (1UL << k))) ||
            ((g_hist_n[k] < PCTL_SETTLE_SCANS) && (fabsf(f[k] - g_target[k]) <= PCTL_FAR_KPA)) ||
            (g_exh_ended[k] && ((now - g_idle_since[k]) < pdMS_TO_TICKS(PCTL_SEQ_SETTLE_MS))))
        {
            continue;       /* Not settled yet, or resting after its exhaust (no inflate, no exhaust). */
        }
        ready |= (1UL << k);
        /* User rule (30.10): also in the first pass the +/-0,3 kPa band holds (0,2 off: left alone,
           0,3 off: adjusted; readings step 0,1 kPa). */
        if (f[k] > (g_target[k] + (seq ? fmaxf(pctl_band(k), PCTL_INF_ACCEPT_KPA) : pctl_hold_off(k))))
        {
            pctl_start_locked(k, PCTL_ACTION_EXHAUST, f[k], now);
        }
    }

    for (uint8_t n = 0U; (PCTL_NO_CHAMBER == inflating) && (n < PCTL_CHAMBER_NUM); n++)
    {
        uint8_t k = (uint8_t)((g_seq + (seq ? 0U : n)) % PCTL_CHAMBER_NUM);
        bool skip = (0U == (g_enable_mask & (1UL << k))) || (0U != (g_fault_mask & (1UL << k))) ||
                    (0U == (ok_mask & (1UL << k)));

        if (!skip && (0U == (ready & (1UL << k))) && (PCTL_ACTION_IDLE == g_act[k]))
        {
            if (seq)
            {
                break;      /* Near the target: wait for settled readings of this chamber. */
            }
            continue;
        }
        /* User rule (30.10): the pass stays on a chamber until it is within PCTL_INF_ACCEPT_KPA of
           its target; after an exhaust it first waits PCTL_SEQ_SETTLE_MS (exhaust carries on, an
           over-exhausted chamber is inflated back), after an inflate it does not wait. */
        if (seq && !skip && ((PCTL_ACTION_IDLE != g_act[k]) ||
                             (g_exh_ended[k] && ((now - g_idle_since[k]) < pdMS_TO_TICKS(PCTL_SEQ_SETTLE_MS)))))
        {
            break;
        }
        if (seq && !skip && (f[k] > (g_target[k] + PCTL_INF_ACCEPT_KPA)))
        {
            pctl_start_locked(k, PCTL_ACTION_EXHAUST, f[k], now);
            break;
        }
        if ((0U != (ready & (1UL << k))) && g_exh_ended[k] &&
            ((now - g_idle_since[k]) < pdMS_TO_TICKS(PCTL_SEQ_SETTLE_MS)))
        {
            if (seq)
            {
                break;      /* User rule (30.10): no inflate within 3 s of this chamber's exhaust. */
            }
            continue;
        }
        if ((0U != (ready & (1UL << k))) &&
            (f[k] < (g_target[k] - (seq ? PCTL_INF_ACCEPT_KPA : pctl_hold_off(k)))))
        {
            pctl_start_locked(k, PCTL_ACTION_INFLATE, f[k], now);
            inflating = (int8_t)k;
            break;
        }
        if (!seq)
        {
            continue;
        }
        g_seq = (uint8_t)((g_seq + 1U) % PCTL_CHAMBER_NUM);
        g_seq_cnt++;
        if (g_seq_cnt >= (PCTL_SEQ_ROUNDS * PCTL_CHAMBER_NUM))
        {
            g_phase = 2U;
            g_fault_mask = 0U;      /* Phase 2 holds every chamber, also those skipped in phase 1. */
            (void)snprintf(g_msg, sizeof(g_msg), "phase 1 done (%u rounds): holding all chambers",
                           (unsigned int)PCTL_SEQ_ROUNDS);
            ESP_LOGI(TAG, "%s", g_msg);
            break;
        }
    }

    /* Drive the valves: open every running chamber's valve (inflate: with the pump) and follow their
       sensors, closing each one just before it reaches its target. */
    uint32_t running = 0U;

    g_active = inflating;
    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        running |= ((PCTL_ACTION_IDLE != g_act[k]) && (0U == (wait & (1UL << k)))) ? (1UL << k) : 0U;
        g_active = ((PCTL_NO_CHAMBER == g_active) && (PCTL_ACTION_IDLE != g_act[k])) ? (int8_t)k : g_active;
    }
    g_action = (PCTL_NO_CHAMBER != g_active) ? g_act[g_active] : PCTL_ACTION_IDLE;

    uint32_t keep = 0U;
    uint32_t moved = 0U;
    zerr_t ret = (0U != running) ? pctl_track_locked(f, running, &keep, &moved) : ZERR_OK;
    uint32_t mask = pctl_valves(keep);

    g_keep = keep;

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (0U != ((moved | keep) & (1UL << k)))
        {
            g_hist_n[k] = 0U;       /* Readings from before the opening no longer count. */
            g_hist_i[k] = 0U;
        }
        if ((PCTL_NO_CHAMBER != inflating) && (0U != (moved & (1UL << inflating))) &&
            ((int8_t)k != inflating))
        {
            g_hold_pump[k] = true;  /* Pump pressure was on the manifold: watch for a leaky inlet. */
        }
    }

    if ((ZERR_OK == ret) && (mask != valve_get_mask()))
    {
        ret = valve_set_mask(mask);
    }
    if (ZERR_OK != ret)
    {
        pctl_stop_locked();
        g_auto = false;
        (void)snprintf(g_msg, sizeof(g_msg), "valve driver error: auto off");
        ESP_LOGE(TAG, "%s", g_msg);
    }

    /* RD: once the weight-based set Pcn is reached, pause holding so the person can lie down and
       the body analysis sees a clean pressure picture. */
    /* Only once the first pass is over (2 rounds, each chamber checked): before, any moment with no
       valve open (e.g. the 3 s wait after exhausting) counted as "reached" (user 30.10 bug). */
    if (g_auto && g_pcn_pending && (2U == g_phase) && (PCTL_NO_CHAMBER == g_active))
    {
        uint32_t all = (1UL << PCTL_CHAMBER_NUM) - 1UL;

        if ((ok_mask & g_enable_mask & all) == (g_enable_mask & all))
        {
            pctl_stop_locked();
            g_auto = false;
            g_pcn_pending = false;
            g_ready = true;
            (void)snprintf(g_msg, sizeof(g_msg), "Pcn reached: holding paused, lie down now");
            ESP_LOGI(TAG, "%s", g_msg);
        }
    }

    /* Power-on fill: once the first pass is over and every readable chamber is at its target, ask
       the person to lie down. Holding goes on until they confirm (pctl_user_on_bed). */
    if (g_auto && g_fill && (2U == g_phase) && (PCTL_NO_CHAMBER == g_active))
    {
        bool full = (0U != ok_mask);

        for (uint8_t k = 0U; full && (k < PCTL_CHAMBER_NUM); k++)
        {
            full = (0U == (ok_mask & (1UL << k))) ||
                   (f[k] >= (g_target[k] - pctl_band(k) - PCTL_HYST_KPA));
        }
        if (full)
        {
            /* Holding pauses (valves shut) so lying down is not let out again. User rule (27.10):
               P1, the "before" set of the pressure difference, is the target. */
            pctl_stop_locked();
            g_auto = false;
            g_fill = false;
            g_fill_done = true;
            memcpy(g_before, g_target, sizeof(g_before));
            g_before_ok = (1UL << PCTL_CHAMBER_NUM) - 1UL;
            (void)snprintf(g_msg, sizeof(g_msg), "filled to %.1f kPa: lie down, then confirm",
                           (double)g_target[0]);
            ESP_LOGI(TAG, "%s", g_msg);
        }
    }

    xSemaphoreGive(g_lock);
}

void pctl_go_pmax(void)
{
    if (NULL == g_lock)
    {
        return;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    if (!g_boot_pending && !g_fill && !g_fill_done && !g_on_bed)
    {
        pctl_stop_locked();
        for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
        {
            g_target[i] = fminf(PCTL_BOOT_FILL_KPA, g_boot_kpa);
        }
        g_fill = true;
        g_scan = 0U;
        g_before_ok = 0U;
        g_enable_mask = (1UL << PCTL_CHAMBER_NUM) - 1UL;
        g_fault_mask = 0U;
        g_pcn_pending = false;
        g_ready = false;
        g_eval = false;
        g_cur_set = PCTL_SET_NONE;
        pctl_restart_seq_locked();
        g_cycle_start = xTaskGetTickCount();
        g_auto = true;
        (void)snprintf(g_msg, sizeof(g_msg), "web app back: all to %.1f kPa", (double)g_target[0]);
        ESP_LOGI(TAG, "%s", g_msg);
    }
    xSemaphoreGive(g_lock);
}

zerr_t pctl_user_on_bed(void)
{
    if (NULL == g_lock)
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    pctl_stop_locked();
    g_auto = false;
    g_boot_pending = false;
    g_fill = false;
    g_fill_done = false;
    g_on_bed = true;
    g_eval = false;
    g_pcn_pending = false;
    g_ready = false;
    /* User rule (30.10): P1 = the 30 s mean taken when the person said they were ready to lie down
       (pctl_user_ready); without it (old flow) the targets held until now. */
    if (!g_p1_ready)
    {
        memcpy(g_before, g_target, sizeof(g_before));
        g_before_ok = (1UL << PCTL_CHAMBER_NUM) - 1UL;
    }
    g_p1_ready = false;
    g_scan = 1U;
    g_scan_n = 0U;
    memset(g_after_sum, 0, sizeof(g_after_sum));
    memset(g_after_cnt, 0, sizeof(g_after_cnt));
    g_scan_last = xTaskGetTickCount();
    (void)snprintf(g_msg, sizeof(g_msg), "person on the mattress: pump off, scanning %u s",
                   (unsigned int)(PCTL_SCAN_NUM * PCTL_SCAN_PERIOD_MS / 1000U));
    ESP_LOGI(TAG, "%s", g_msg);
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

void pctl_cancel_flow(void)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_scan = 0U;
    g_p1_ready = false;
    g_pcn_pending = false;
    g_ready = false;
    g_fill = false;
    g_fill_done = false;
    (void)snprintf(g_msg, sizeof(g_msg), "config creation cancelled");
    ESP_LOGI(TAG, "%s", g_msg);
    xSemaphoreGive(g_lock);
}

zerr_t pctl_user_ready(void)
{
    if (NULL == g_lock)
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    /* User rule (30.10): "ready to lie down" stops holding at once; P1 = mean of the last 30 s. */
    pctl_stop_locked();
    g_auto = false;
    g_eval = false;
    g_pcn_pending = false;
    g_ready = false;
    g_fill = false;
    g_fill_done = false;
    g_exp_n = 0U;
    g_before_ok = 0U;
    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        float sum = 0.0f;
        uint8_t n = 0U;

        for (uint8_t i = 0U; i < PCTL_P1_SECONDS; i++)
        {
            if (0U != (g_p1_ok[i] & (1UL << k)))
            {
                sum += g_p1_ring[i][k];
                n++;
            }
        }
        g_before[k] = (0U != n) ? (sum / (float)n) : g_target[k];
        g_before_ok |= (1UL << k);
    }
    g_p1_ready = true;
    (void)snprintf(g_msg, sizeof(g_msg), "ready to lie down: holding off, P1 = last %u s mean",
                   (unsigned int)PCTL_P1_SECONDS);
    ESP_LOGI(TAG, "%s", g_msg);
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

zerr_t pctl_exp_start(const float p_kpa[][PCTL_CHAMBER_NUM], const uint8_t *p_slot, uint8_t n)
{
    if ((n < 2U) || (n > PCTL_CAND_MAX) || (NULL == g_lock))
    {
        return ZERR_INVALID_PARAM;
    }
    xSemaphoreTake(g_lock, portMAX_DELAY);
    pctl_stop_locked();
    memcpy(g_exp_kpa, p_kpa, (size_t)n * sizeof(g_exp_kpa[0]));
    memcpy(g_exp_slot, p_slot, n);
    g_exp_n = n;
    g_boot_pending = false;
    g_fill = false;
    g_fill_done = false;
    g_pcn_pending = false;
    g_ready = false;
    g_scan = 0U;
    g_eval = false;
    g_eval_err = false;
    pctl_apply_cand_locked(0U, 0);
    g_cycle_start = xTaskGetTickCount();
    g_exp_start = g_cycle_start;
    g_auto = true;
    (void)snprintf(g_msg, sizeof(g_msg), "experience: %u sets, starting with set 1", (unsigned int)n);
    ESP_LOGI(TAG, "%s", g_msg);
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

void pctl_exp_stop(void)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_exp_n = 0U;
    g_eval = false;
    g_cycle_start = xTaskGetTickCount();
    (void)snprintf(g_msg, sizeof(g_msg), "experience stopped, holding the current set");
    xSemaphoreGive(g_lock);
}

bool pctl_wait_on_bed(void)
{
    return g_fill_done || (1U == g_scan);
}

zerr_t pctl_set_auto(bool on)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_boot_pending = false;
    g_fill = false;
    g_fill_done = false;
    pctl_stop_locked();
    g_auto = on;
    g_fault_mask = 0U;
    g_leak_mask = 0U;
    pctl_restart_seq_locked();
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        g_hold_p[i] = NAN;
    }
    g_ready = false;
    if (!on)
    {
        g_pcn_pending = false;
        g_exp_n = 0U;
    }
    g_eval = false;
    g_cycle_start = xTaskGetTickCount();
    (void)snprintf(g_msg, sizeof(g_msg), "%s", on ? "auto on" : "auto off");
    ESP_LOGI(TAG, "%s", g_msg);
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

zerr_t pctl_set_timeout(uint32_t seconds)
{
    zerr_t ret;

    if (seconds > PCTL_TIMEOUT_MAX_S)
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_timeout_s = seconds;
    ret = flash_write(PCTL_NVS_KEY_TIMEOUT, &g_timeout_s, sizeof(g_timeout_s));
    ESP_LOGI(TAG, "chamber time limit %lu s%s (saved %s)", (unsigned long)seconds,
             (0U == seconds) ? " (none)" : "", (ZERR_OK == ret) ? "ok" : "FAIL");
    xSemaphoreGive(g_lock);

    return ret;
}

zerr_t pctl_set_posture(bool on, uint32_t period_s, uint32_t window_s)
{
    pctl_posture_cfg_t cfg = {on ? 1U : 0U, period_s, window_s};
    zerr_t ret;

    if ((period_s < PCTL_PERIOD_MIN_S) || (period_s > PCTL_PERIOD_MAX_S) ||
        (window_s < PCTL_WINDOW_MIN_S) || (window_s > PCTL_WINDOW_MAX_S))
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_posture_on = on;
    g_match_period_s = period_s;
    g_match_window_s = window_s;
    g_eval = false;                         /* A running check ends; holding goes on. */
    g_cycle_start = xTaskGetTickCount();
    ret = flash_write(PCTL_NVS_KEY_POSTURE, &cfg, sizeof(cfg));
    ESP_LOGI(TAG, "posture check %s, hold %lu s, check %lu s (saved %s)", on ? "on" : "off",
             (unsigned long)period_s, (unsigned long)window_s, (ZERR_OK == ret) ? "ok" : "FAIL");
    xSemaphoreGive(g_lock);

    return ret;
}

zerr_t pctl_set_tol(float kpa)
{
    zerr_t ret;

    if (!(kpa >= PCTL_TOL_MIN_KPA) || (kpa > PCTL_TOL_MAX_KPA))
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_tol_kpa = kpa;
    ret = flash_write(PCTL_NVS_KEY_TOL, &g_tol_kpa, sizeof(g_tol_kpa));
    ESP_LOGI(TAG, "target band +/-%.2f kPa (saved %s)", (double)kpa, (ZERR_OK == ret) ? "ok" : "FAIL");
    xSemaphoreGive(g_lock);

    return ret;
}

zerr_t pctl_set_boot_kpa(float kpa)
{
    zerr_t ret;

    if (!(kpa >= 0.0f) || (kpa > PCTL_TARGET_MAX_KPA))
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_boot_kpa = kpa;
    ret = flash_write(PCTL_NVS_KEY_BOOT, &g_boot_kpa, sizeof(g_boot_kpa));
    pctl_targets_changed_locked();      /* Targets above the new Pmax come down to it. */
    ESP_LOGI(TAG, "power-on level %.1f kPa (saved %s)", (double)kpa, (ZERR_OK == ret) ? "ok" : "FAIL");
    xSemaphoreGive(g_lock);

    return ret;
}

bool pctl_is_auto(void)
{
    return g_auto;
}

bool pctl_is_busy(void)
{
    return (g_auto && (PCTL_NO_CHAMBER != g_active)) || (1U == g_scan);   /* Scan: 1 s steps exact. */
}

zerr_t pctl_set_level(uint8_t level)
{
    if ((level < 1U) || (level > PCTL_LEVEL_NUM))
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        g_target[i] = g_level_kpa[level - 1U];
    }
    pctl_targets_changed_locked();
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

zerr_t pctl_set_weight(float kg)
{
    if ((kg <= 0.0f) || (kg > 200.0f))
    {
        return ZERR_INVALID_PARAM;
    }

    zerr_t ret = pctl_set_level((kg <= 60.0f) ? 6U : ((kg <= 80.0f) ? 7U : 8U));

    /* RD: go to Pcn on every chamber, then pause holding (see pctl_step). */
    if (ZERR_OK == ret)
    {
        (void)pctl_set_enable((1UL << PCTL_CHAMBER_NUM) - 1UL);
        pctl_set_current(2);    /* Pcn = Ptd, the free-position set. */
        (void)pctl_set_auto(true);
        xSemaphoreTake(g_lock, portMAX_DELAY);
        g_pcn_pending = true;
        (void)snprintf(g_msg, sizeof(g_msg), "going to Pcn %.0f kPa, then pause", (double)g_target[0]);
        xSemaphoreGive(g_lock);
    }

    return ret;
}

zerr_t pctl_set_target(uint8_t index, float kpa)
{
    if ((index >= PCTL_CHAMBER_NUM) || (kpa < 0.0f) || (kpa > PCTL_TARGET_MAX_KPA))
    {
        return ZERR_INVALID_PARAM;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_target[index] = kpa;
    pctl_targets_changed_locked();
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

void pctl_get_targets(float *p_kpa)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
    memcpy(p_kpa, g_target, sizeof(g_target));
    xSemaphoreGive(g_lock);
}

zerr_t pctl_set_enable(uint32_t mask)
{
    mask &= (1UL << PCTL_CHAMBER_NUM) - 1UL;

    xSemaphoreTake(g_lock, portMAX_DELAY);
    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if ((PCTL_ACTION_IDLE != g_act[k]) && (0U == (mask & (1UL << k))))
        {
            pctl_stop_locked();     /* A chamber being adjusted was just switched off. */
            break;
        }
    }
    g_enable_mask = mask;
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

zerr_t pctl_set_targets(const float *p_kpa)
{
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        if (!(p_kpa[i] >= 0.0f) || (p_kpa[i] > PCTL_TARGET_MAX_KPA))
        {
            return ZERR_INVALID_PARAM;
        }
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    memcpy(g_target, p_kpa, sizeof(g_target));
    pctl_targets_changed_locked();
    g_pcn_pending = false;      /* Own targets: keep holding them, no Pcn pause (weight flow). */
    xSemaphoreGive(g_lock);

    return ZERR_OK;
}

void pctl_set_preset(uint8_t slot, const float *p_kpa, uint32_t enable, bool ok)
{
    if ((slot >= PCTL_PRESET_NUM) || (NULL == g_lock))
    {
        return;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    if (ok && (NULL != p_kpa))
    {
        memcpy(g_pre_kpa[slot], p_kpa, sizeof(g_pre_kpa[slot]));
    }
    g_pre_en[slot] = enable & ((1UL << PCTL_CHAMBER_NUM) - 1UL);
    g_pre_ok[slot] = ok && (0U != g_pre_en[slot]);
    xSemaphoreGive(g_lock);
}

void pctl_set_current(int8_t slot)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_cur_set = ((slot >= 0) && (slot < (int8_t)PCTL_PRESET_NUM)) ? slot : PCTL_SET_NONE;
    g_cur_shift = 0;
    g_cycle_start = xTaskGetTickCount();
    xSemaphoreGive(g_lock);
}

size_t pctl_oled_text(char *p_buf, size_t size)
{
    uint32_t inf = 0U;
    uint32_t exh = 0U;
    size_t len;

    xSemaphoreTake(g_lock, portMAX_DELAY);
    if (g_fill_done)
    {
        len = (size_t)snprintf(p_buf, size, "Dem da bom du\n%.0f kPa.\nMoi ban nam len\ndem roi bam\n"
                                            "\"Toi da nam\"\ntren web.", (double)g_target[0]);
        xSemaphoreGive(g_lock);
        return (len < size) ? len : (size - 1U);
    }
    if (0U != g_scan)
    {
        len = (2U == g_scan)
                  ? (size_t)snprintf(p_buf, size, "Da quet xong.\nXem diem A, B\nva bieu do\nchenh ap\ntren web.")
                  : (size_t)snprintf(p_buf, size, "Dang quet ap\nchenh lech...\n%u / %u s\nXin nam yen.",
                                     (unsigned int)g_scan_n, (unsigned int)PCTL_SCAN_NUM);
        xSemaphoreGive(g_lock);
        return (len < size) ? len : (size - 1U);
    }
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        inf += (PCTL_ACTION_INFLATE == g_act[i]) ? 1U : 0U;
        exh += (PCTL_ACTION_EXHAUST == g_act[i]) ? 1U : 0U;
    }
    len = (size_t)snprintf(p_buf, size, "Giu ap: %s\nPha %u P%02u N%lu X%lu\nBo qua: %lu khoang\n"
                                        "Ap bat nguon:\n %.1f kPa\nMuc tieu P01:\n %.1f kPa",
                           g_boot_pending ? "CHO" : (g_fill ? "BOM" : (g_auto ? "BAT" : "TAT")),
                           (unsigned int)g_phase,
                           (unsigned int)(g_seq + 1U), (unsigned long)inf,
                           (unsigned long)exh, (unsigned long)__builtin_popcount(g_fault_mask),
                           (double)fminf(PCTL_BOOT_FILL_KPA, g_boot_kpa), (double)g_target[0]);
    xSemaphoreGive(g_lock);

    return (len < size) ? len : (size - 1U);
}

size_t pctl_json(char *p_buf, size_t size)
{
    static const char *const action_name[] = {"idle", "inflate", "exhaust"};
    size_t len;

    xSemaphoreTake(g_lock, portMAX_DELAY);
    TickType_t now = xTaskGetTickCount();
    uint32_t next_s = 0U;
    if (g_eval)
    {
        uint32_t gone = (uint32_t)((now - g_eval_start) * portTICK_PERIOD_MS);
        next_s = (gone < (pctl_window_s() * 1000U)) ? (pctl_window_s() - (gone / 1000U)) : 0U;
    }
    else if (g_auto)
    {
        uint32_t gone = (uint32_t)((now - g_cycle_start) * portTICK_PERIOD_MS);
        next_s = (gone < (pctl_period_s() * 1000U)) ? (pctl_period_s() - (gone / 1000U)) : 0U;
    }
    len = (size_t)snprintf(p_buf, size, "{\"boot\":%.1f,\"tmo\":%lu,\"set\":%d,\"eval\":%s,\"next_s\":%lu,\"pcn\":%s,\"ready\":%s,",
                           (double)g_boot_kpa, (unsigned long)g_timeout_s, (int)g_cur_set, g_eval ? "true" : "false", (unsigned long)next_s,
                           g_pcn_pending ? "true" : "false", g_ready ? "true" : "false");
    len += (size_t)snprintf(&p_buf[len], size - len, "\"fill\":%s,\"fill_done\":%s,\"fill_kpa\":%.1f,",
                            (g_fill || g_boot_pending) ? "true" : "false", g_fill_done ? "true" : "false",
                            (double)fminf(PCTL_BOOT_FILL_KPA, g_boot_kpa));
    len += (size_t)snprintf(&p_buf[len], size - len, "\"scan\":%u,\"scan_n\":%u,\"scan_num\":%u,\"p1\":[",
                            (unsigned int)g_scan, (unsigned int)g_scan_n, (unsigned int)PCTL_SCAN_NUM);
    for (uint8_t i = 0U; ((2U == g_scan) || g_p1_ready) && (i < PCTL_CHAMBER_NUM) && (len < size); i++)
    {
        len += (0U != (g_before_ok & (1UL << i)))
                   ? (size_t)snprintf(&p_buf[len], size - len, "%s%.1f", (0U == i) ? "" : ",", (double)g_before[i])
                   : (size_t)snprintf(&p_buf[len], size - len, "%snull", (0U == i) ? "" : ",");
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "],\"p2\":[");
    }
    for (uint8_t i = 0U; (2U == g_scan) && (i < PCTL_CHAMBER_NUM) && (len < size); i++)
    {
        len += (0U != g_after_cnt[i])
                   ? (size_t)snprintf(&p_buf[len], size - len, "%s%.1f", (0U == i) ? "" : ",",
                                      (double)(g_after_sum[i] / (float)g_after_cnt[i]))
                   : (size_t)snprintf(&p_buf[len], size - len, "%snull", (0U == i) ? "" : ",");
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "],");
    }
    uint32_t inf = 0U;
    uint32_t exh = 0U;
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        inf |= (PCTL_ACTION_INFLATE == g_act[i]) ? (1UL << i) : 0U;
        exh |= (PCTL_ACTION_EXHAUST == g_act[i]) ? (1UL << i) : 0U;
    }
    len += (size_t)snprintf(&p_buf[len], size - len, "\"phase\":%u,\"seq\":%u,\"inf\":%lu,\"exh\":%lu,\"leak\":%lu,\"noisy\":%lu,",
                            (unsigned int)g_phase, (unsigned int)(g_seq + 1U), (unsigned long)inf, (unsigned long)exh, (unsigned long)g_leak_mask,
                            (unsigned long)g_noisy_mask);
    len += (size_t)snprintf(&p_buf[len], size - len, "\"perr\":%s,\"excur\":%d,\"exsec\":%lu,\"exp\":[", g_eval_err ? "true" : "false",
                            (int)g_exp_cur, (unsigned long)((0U != g_exp_n) ? ((now - g_exp_start) * portTICK_PERIOD_MS / 1000U) : 0U));
    for (uint8_t i = 0U; (i < g_exp_n) && (len < size); i++)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "%s%u", (0U == i) ? "" : ",", (unsigned int)g_exp_slot[i]);
    }
    len += (len < size) ? (size_t)snprintf(&p_buf[len], size - len, "],") : 0U;
    len += (size_t)snprintf(&p_buf[len], size - len, "\"pon\":%s,\"per\":%lu,\"win\":%lu,\"dist\":[%.2f,%.2f,%.2f],",
                            g_posture_on ? "true" : "false", (unsigned long)g_match_period_s,
                            (unsigned long)g_match_window_s, (double)g_eval_dist[0], (double)g_eval_dist[1],
                            (double)g_eval_dist[2]);
    len += (size_t)snprintf(&p_buf[len], size - len, "\"on\":%s,\"tol\":%.2f,\"active\":%d,\"action\":\"%s\","
                                        "\"fault\":%lu,\"en\":%lu,\"msg\":\"%s\",\"target\":[",
                           g_auto ? "true" : "false", (double)g_tol_kpa, (int)g_active,
                           action_name[g_action], (unsigned long)g_fault_mask,
                           (unsigned long)g_enable_mask, g_msg);
    for (uint8_t i = 0U; (i < PCTL_CHAMBER_NUM) && (len < size); i++)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "%s%.1f", (0U == i) ? "" : ",",
                                (double)g_target[i]);
    }
    xSemaphoreGive(g_lock);

    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "]}");
    }

    return (len < size) ? len : (size - 1U);
}

/* Private functions -----------------------------------------------------------------------------*/

/**
 * @brief  Close every valve, stop the pump, forget the active chamber. Called with g_lock held.
 */
static void pctl_stop_locked(void)
{
    (void)valve_all_off();
    g_keep = 0U;
    g_active = PCTL_NO_CHAMBER;
    g_action = PCTL_ACTION_IDLE;
    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        g_act[k] = PCTL_ACTION_IDLE;
    }
}

/**
 * @brief  Start again at phase 1 from P01 (new targets or auto mode switched on). g_lock held.
 */
static void pctl_restart_seq_locked(void)
{
    /* User rule (30.10): the pass starts at the lowest-numbered chamber whose target changed since
       the last start (none changed: P01) and goes round P13 -> P01, PCTL_SEQ_ROUNDS rounds. */
    uint8_t start = PCTL_CHAMBER_NUM;

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if ((PCTL_CHAMBER_NUM == start) && !(fabsf(g_target[k] - g_seq_last[k]) <= PCTL_HYST_KPA))
        {
            start = k;      /* NAN (never started) counts as changed. */
        }
        g_seq_last[k] = g_target[k];
    }
    g_phase = 1U;
    g_seq = (PCTL_CHAMBER_NUM == start) ? 0U : start;
    g_seq_cnt = 0U;
}

/**
 * @brief  Start inflating or exhausting chamber k from filtered pressure p. g_lock held.
 */
static void pctl_start_locked(uint8_t k, pctl_action_t action, float p, TickType_t now)
{
    g_act[k] = action;
    g_short_us[k] = 0U;
    g_act_start[k] = now;
    g_act_move[k] = now;
    g_act_p[k] = p;
    g_hold_p[k] = NAN;
    (void)snprintf(g_msg, sizeof(g_msg), "phase %u: P%02u %s %.2f -> %.1f kPa", (unsigned int)g_phase,
                   (unsigned int)(k + 1U), (PCTL_ACTION_INFLATE == action) ? "inflate" : "exhaust",
                   (double)p, (double)g_target[k]);
    ESP_LOGI(TAG, "%s", g_msg);
}

/**
 * @brief  User rule: no target above Pmax (the power-on level set on the web). g_lock held.
 */
static void pctl_clamp_targets_locked(void)
{
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        if (g_target[i] > g_boot_kpa)
        {
            g_target[i] = g_boot_kpa;
        }
    }
}

/**
 * @brief  New targets: limit them to Pmax, save them, stop the current step and retry chambers
 *         that were skipped.
 */
static void pctl_targets_changed_locked(void)
{
    pctl_clamp_targets_locked();
    if (PCTL_NO_CHAMBER != g_active)
    {
        pctl_stop_locked();
    }
    g_fault_mask = 0U;
    g_leak_mask = 0U;
    for (uint8_t i = 0U; i < PCTL_CHAMBER_NUM; i++)
    {
        g_hold_p[i] = NAN;
    }
    g_ready = false;
    g_fill = false;
    g_fill_done = false;
    g_exp_n = 0U;
    pctl_restart_seq_locked();
    (void)flash_write(PCTL_NVS_KEY_TARGET, g_target, sizeof(g_target));
    (void)snprintf(g_msg, sizeof(g_msg), "targets set");
}

/* Hold time between checks and check time: experience mode uses its own (user 28.10: 2 min / 30 s). */
static uint32_t pctl_period_s(void)
{
    return (0U != g_exp_n) ? PCTL_EXP_PERIOD_S : g_match_period_s;
}

static uint32_t pctl_window_s(void)
{
    return (0U != g_exp_n) ? PCTL_EXP_WINDOW_S : g_match_window_s;
}

/* Posture candidates. Normal: saved sets 0 back, 1 side (matched), 2 free Ptd (default). Experience
   mode (user 28.10, "Trai nghiem"): the chosen named sets, all matched, the first one the default. */
static uint8_t pctl_cand_num(void)
{
    return (0U != g_exp_n) ? g_exp_n : (uint8_t)PCTL_PRESET_NUM;
}

static const float *pctl_cand_kpa(uint8_t i)
{
    return (0U != g_exp_n) ? g_exp_kpa[i] : g_pre_kpa[i];
}

static uint32_t pctl_cand_en(uint8_t i)
{
    return (0U != g_exp_n) ? ((1UL << PCTL_CHAMBER_NUM) - 1UL) : g_pre_en[i];
}

static bool pctl_cand_ok(uint8_t i)
{
    return (0U != g_exp_n) ? (i < g_exp_n) : g_pre_ok[i];
}

static bool pctl_cand_matched(uint8_t i)
{
    return (0U != g_exp_n) ? true : (i < 2U);
}

static uint8_t pctl_cand_default(void)
{
    return (0U != g_exp_n) ? 0U : 2U;
}

static int8_t pctl_cand_cur(void)
{
    return (0U != g_exp_n) ? g_exp_cur : g_cur_set;
}

/**
 * @brief  Shifts (bit = shift + PCTL_SHIFT_OFS) at which every ticked chamber of candidate `i` lands
 *         on P01..P13.
 */
static uint32_t pctl_fit_shifts(uint8_t i)
{
    uint32_t en = pctl_cand_en(i);
    uint32_t mask = 0U;

    for (int8_t shift = -(int8_t)PCTL_SHIFT_OFS; shift <= (int8_t)PCTL_SHIFT_OFS; shift++)
    {
        bool fit = (0U != en);

        for (int8_t k = 0; fit && (k < (int8_t)PCTL_CHAMBER_NUM); k++)
        {
            int8_t at = (int8_t)(k + shift);

            fit = (0U == (en & (1UL << k))) || ((at >= 0) && (at < (int8_t)PCTL_CHAMBER_NUM));
        }
        mask |= fit ? (1UL << (shift + (int8_t)PCTL_SHIFT_OFS)) : 0U;
    }

    return mask;
}

/**
 * @brief  Run candidate `i` at `shift`: a saved set (pctl_apply_set_locked) or, in experience mode,
 *         the chosen named set as it is.
 */
static void pctl_apply_cand_locked(uint8_t i, int8_t shift)
{
    if (0U == g_exp_n)
    {
        pctl_apply_set_locked(i, shift);
        return;
    }
    memcpy(g_target, g_exp_kpa[i], sizeof(g_target));
    pctl_clamp_targets_locked();
    g_enable_mask = (1UL << PCTL_CHAMBER_NUM) - 1UL;
    g_fault_mask = 0U;
    g_exp_cur = (int8_t)i;
    g_cur_shift = 0;
    pctl_restart_seq_locked();
    (void)snprintf(g_msg, sizeof(g_msg), "experience: switched to set %u", (unsigned int)(i + 1U));
}

/**
 * @brief  RD step 6 (user 28.10, RD algorithm). Every "period" holding stops for "window"; every scan
 *         in it is checked:
 *         - each matched candidate: kept at a shift only while every ticked chamber, moved by that
 *           shift (anywhere on P01..P13), reads within +/-PCTL_POSTURE_TOL_KPA of the set's value;
 *         - running matched candidate: "out" while more than half of its chambers are off their
 *           targets by more than PCTL_POSTURE_TOL_KPA.
 *         Window over: one candidate matched the whole window -> that set (smallest deviation shift);
 *         several matched -> error, set kept; else the running one "out" the whole window -> the
 *         default set (Ptd / first chosen set); else the set is kept. Returns true while the window
 *         runs (no valve may move).
 */
static bool pctl_posture_step_locked(const float *p_kpa, uint32_t ok_mask)
{
    TickType_t now = xTaskGetTickCount();
    uint8_t num = pctl_cand_num();
    int8_t cur = pctl_cand_cur();
    bool any = false;

    for (uint8_t s = 0U; (g_posture_on || (0U != g_exp_n)) && !g_pcn_pending && !g_fill && !g_fill_done &&
                         (s < num); s++)
    {
        any = any || pctl_cand_ok(s);
    }

    if (!g_eval)
    {
        if (!any || ((now - g_cycle_start) < pdMS_TO_TICKS(pctl_period_s() * 1000U)))
        {
            return false;
        }
        pctl_stop_locked();
        g_eval = true;
        g_eval_start = now;
        g_eval_out = (cur >= 0) && pctl_cand_matched((uint8_t)cur) && pctl_cand_ok((uint8_t)cur);
        for (uint8_t s = 0U; s < PCTL_CAND_MAX; s++)
        {
            g_eval_shift[s] = ((s < num) && pctl_cand_ok(s) && pctl_cand_matched(s)) ? pctl_fit_shifts(s) : 0U;
        }
        memset(g_eval_dev, 0, sizeof(g_eval_dev));
        (void)snprintf(g_msg, sizeof(g_msg), "posture check: holding paused %lu s",
                       (unsigned long)pctl_window_s());
        ESP_LOGI(TAG, "%s", g_msg);
    }

    for (uint8_t s = 0U; s < num; s++)
    {
        const float *p_set = pctl_cand_kpa(s);
        uint32_t en = pctl_cand_en(s);

        for (uint8_t b = 0U; (0U != g_eval_shift[s]) && (b < PCTL_SHIFT_NUM); b++)
        {
            int8_t shift = (int8_t)b - (int8_t)PCTL_SHIFT_OFS;
            float dev = 0.0f;
            bool hit = (0U != (g_eval_shift[s] & (1UL << b)));

            for (int8_t k = 0; hit && (k < (int8_t)PCTL_CHAMBER_NUM); k++)
            {
                int8_t at = (int8_t)(k + shift);

                if (0U == (en & (1UL << k)))
                {
                    continue;
                }
                hit = (0U != (ok_mask & (1UL << at))) && (fabsf(p_kpa[at] - p_set[k]) <= PCTL_POSTURE_TOL_KPA);
                dev += hit ? fabsf(p_kpa[at] - p_set[k]) : 0.0f;
            }
            g_eval_shift[s] &= hit ? 0xFFFFFFFFUL : ~(1UL << b);
            g_eval_dev[s][b] += dev;
        }
    }
    if (g_eval_out)
    {
        uint32_t en = pctl_cand_en((uint8_t)cur);
        uint8_t n = 0U;
        uint8_t out = 0U;

        for (int8_t k = 0; k < (int8_t)PCTL_CHAMBER_NUM; k++)
        {
            int8_t at = (int8_t)(k + g_cur_shift);

            if ((0U == (en & (1UL << k))) || (at < 0) || (at >= (int8_t)PCTL_CHAMBER_NUM))
            {
                continue;
            }
            n++;
            out += ((0U == (ok_mask & (1UL << at))) ||
                    (fabsf(p_kpa[at] - g_target[at]) > PCTL_POSTURE_TOL_KPA)) ? 1U : 0U;
        }
        g_eval_out = ((2U * out) > n);
    }

    if ((now - g_eval_start) < pdMS_TO_TICKS(pctl_window_s() * 1000U))
    {
        return true;
    }

    /* Window over. */
    int8_t best[PCTL_CAND_MAX];
    uint8_t hits = 0U;
    int8_t pick = PCTL_SET_NONE;

    for (uint8_t s = 0U; s < PCTL_CAND_MAX; s++)
    {
        best[s] = 0;
        g_eval_dist[s] = -1.0f;
        for (uint8_t b = 0U; (s < num) && (b < PCTL_SHIFT_NUM); b++)
        {
            int8_t shift = (int8_t)b - (int8_t)PCTL_SHIFT_OFS;

            if ((0U != (g_eval_shift[s] & (1UL << b))) &&
                ((g_eval_dist[s] < 0.0f) || (g_eval_dev[s][b] < g_eval_dist[s]) ||
                 (((int8_t)s == cur) && (shift == g_cur_shift) && (g_eval_dev[s][b] <= g_eval_dist[s]))))
            {
                g_eval_dist[s] = g_eval_dev[s][b];
                best[s] = shift;
            }
        }
        if (g_eval_dist[s] >= 0.0f)
        {
            hits++;
            pick = (int8_t)s;
        }
    }
    g_eval = false;
    g_cycle_start = now;
    g_eval_err = (hits > 1U);

    if (g_eval_err)
    {
        (void)snprintf(g_msg, sizeof(g_msg), "posture check ERROR: %u sets match at once", (unsigned int)hits);
    }
    else if (1U == hits)
    {
        if ((pick == cur) && (best[pick] == g_cur_shift))
        {
            (void)snprintf(g_msg, sizeof(g_msg), "posture check: set %d still matches", (int)pick + 1);
        }
        else
        {
            pctl_apply_cand_locked((uint8_t)pick, best[pick]);
        }
    }
    else if (g_eval_out && pctl_cand_ok(pctl_cand_default()))
    {
        pctl_apply_cand_locked(pctl_cand_default(), 0);
    }
    else
    {
        (void)snprintf(g_msg, sizeof(g_msg), "posture check: no set matches, set kept");
    }
    ESP_LOGI(TAG, "%s (matches %u, out %d)", g_msg, (unsigned int)hits, (int)g_eval_out);

    return false;
}

/**
 * @brief  Run saved set `slot`, its pattern moved by `shift` chambers. Chambers outside the pattern
 *         take the free set (Ptd) when saved, else keep their target. Not written to NVS.
 */
static void pctl_apply_set_locked(uint8_t slot, int8_t shift)
{
    static const char *const name[PCTL_PRESET_NUM] = {"back", "side", "free"};

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (g_pre_ok[2])
        {
            g_target[k] = g_pre_kpa[2][k];
        }
    }
    for (int8_t k = 0; k < (int8_t)PCTL_CHAMBER_NUM; k++)
    {
        int8_t at = (int8_t)(k + shift);

        if ((0U != (g_pre_en[slot] & (1UL << k))) && (at >= 0) && (at < (int8_t)PCTL_CHAMBER_NUM))
        {
            g_target[at] = g_pre_kpa[slot][k];
        }
    }
    pctl_clamp_targets_locked();
    g_enable_mask = (1UL << PCTL_CHAMBER_NUM) - 1UL;
    g_fault_mask = 0U;
    g_cur_set = (int8_t)slot;
    g_cur_shift = shift;
    pctl_restart_seq_locked();
    (void)snprintf(g_msg, sizeof(g_msg), "posture check: switched to %s set (shift %d)", name[slot],
                   (int)shift);
}

/**
 * @brief  Valve outputs (and the pump for inflating chambers) of the chambers in `chambers`.
 */
static uint32_t pctl_valves(uint32_t chambers)
{
    uint32_t mask = 0U;

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (0U == (chambers & (1UL << k)))
        {
            continue;
        }
        mask |= (PCTL_ACTION_INFLATE == g_act[k])
                    ? ((1UL << g_inflate_out[k]) | (1UL << PCTL_PUMP_OUTPUT))
                    : ((PCTL_ACTION_EXHAUST == g_act[k]) ? (1UL << g_exhaust_out[k]) : 0U);
    }

    return mask;
}

/**
 * @brief  Open the valves of the `running` chambers together and follow their sensors (all read at
 *         once, about 100 ms a round) for up to PCTL_TRACK_MS. Each valve closes once the pressure
 *         its chamber still has to go is what comes in while closing (rate x PCTL_LAG_MS). Chambers
 *         already closer than that get a short timed opening instead. *p_keep: chambers whose
 *         target is still more than PCTL_KEEP_OPEN_MS away, their valves stay open through the next
 *         full scan. g_lock held.
 */
static zerr_t pctl_track_locked(const float *p_kpa, uint32_t running, uint32_t *p_keep,
                                uint32_t *p_moved)
{
    float raw0[PCTL_CHAMBER_NUM];
    float raw[PCTL_CHAMBER_NUM];
    float last[PCTL_CHAMBER_NUM];
    float left[PCTL_CHAMBER_NUM];
    uint32_t short_us[PCTL_CHAMBER_NUM];
    uint32_t ok0 = 0U;
    uint32_t ok = 0U;
    uint32_t have_last = 0U;
    uint32_t open = 0U;
    uint32_t timed = 0U;
    zerr_t ret;

    *p_keep = 0U;
    *p_moved = 0U;
    /* Reference readings before the valves open, mean of two: later readings count from here. */
    (void)pressure_array_read_all(raw0, &ok0);
    (void)pressure_array_read_all(raw, &ok);
    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (0U != (ok0 & ok & (1UL << k)))
        {
            raw0[k] = 0.5f * (raw0[k] + raw[k]);
        }
        else if (0U != (ok & (1UL << k)))
        {
            raw0[k] = raw[k];
        }
    }
    ok0 |= ok;

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        if (0U == (running & ok0 & (1UL << k)))
        {
            continue;       /* No reading: closed, the next full scan decides. */
        }

        bool inflate = (PCTL_ACTION_INFLATE == g_act[k]);
        float rate = g_rate[k][inflate ? 0U : 1U];
        uint8_t a = inflate ? 0U : 1U;
        uint32_t min_us = (0U != g_pulse_us[k][a]) ? g_pulse_us[k][a] : PCTL_SHORT_MIN_US;

        /* A timed opening that did not cover half the gap was too short for this valve (P13, 26.10:
           exhausting near the target took a minute): twice as long now, and once even the longest
           one falls short the learned rate is wrong, so it is forgotten and the valve is opened and
           followed instead. */
        if (0U != g_short_us[k])
        {
            float moved = inflate ? (p_kpa[k] - g_short_p[k]) : (g_short_p[k] - p_kpa[k]);
            float need = 0.5f * fabsf(g_target[k] - g_short_p[k]);

            need = (need > PCTL_SHORT_SEEN_KPA) ? need : PCTL_SHORT_SEEN_KPA;
            if ((moved < need) && (g_short_us[k] >= PCTL_SHORT_MAX_US))
            {
                g_rate[k][a] = 0.0f;
                rate = 0.0f;
                min_us = PCTL_SHORT_MIN_US;
            }
            else if (moved < need)
            {
                min_us = (2U * g_short_us[k] < PCTL_SHORT_MAX_US) ? (2U * g_short_us[k]) : PCTL_SHORT_MAX_US;
            }
            else
            {
                min_us = g_short_us[k];     /* Worked: keep it (noise must not shrink it again). */
            }
            g_pulse_us[k][a] = min_us;
            if (moved >= PCTL_SHORT_SEEN_KPA)
            {
                g_act_move[k] = xTaskGetTickCount();    /* It moved: not a stuck valve. */
            }
            g_short_us[k] = 0U;
        }

        left[k] = inflate ? (g_target[k] - p_kpa[k]) : (p_kpa[k] - g_target[k]);
        if (!inflate && (left[k] <= (rate * (float)PCTL_LAG_MS)))
        {
            uint32_t us = (uint32_t)((PCTL_SHORT_GAIN * left[k] * 1000.0f) / rate);

            short_us[k] = (us < min_us) ? min_us : ((us > PCTL_SHORT_MAX_US) ? PCTL_SHORT_MAX_US : us);
            timed |= (1UL << k);
        }
        else
        {
            open |= (1UL << k);
        }
    }

    *p_moved = open | timed;

    /* Timed openings: all at once, each closed at its own time. */
    if (0U != timed)
    {
        uint32_t gone_us = 0U;

        ret = valve_set_mask(pctl_valves(timed));
        while ((ZERR_OK == ret) && (0U != timed))
        {
            uint8_t next = 0U;
            uint32_t best = UINT32_MAX;

            for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
            {
                if ((0U != (timed & (1UL << k))) && (short_us[k] < best))
                {
                    best = short_us[k];
                    next = k;
                }
            }
            esp_rom_delay_us(best - gone_us);
            gone_us = best;
            timed &= ~(1UL << next);
            g_short_us[next] = best;
            g_short_p[next] = p_kpa[next];
            ret = valve_set_mask(pctl_valves(timed));
        }
        if (ZERR_OK != ret)
        {
            return ret;
        }
    }

    /* Followed openings. */
    if (0U == open)
    {
        return ZERR_OK;
    }
    ret = valve_set_mask(pctl_valves(open));
    if (ZERR_OK != ret)
    {
        return ret;
    }

    TickType_t t0 = xTaskGetTickCount();

    while ((0U != open) && ((xTaskGetTickCount() - t0) < pdMS_TO_TICKS(PCTL_TRACK_MS)))
    {
        uint32_t was = open;

        (void)pressure_array_read_all(raw, &ok);
        for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
        {
            if ((0U != (ok & (1UL << k))) && (raw[k] > PCTL_OVERPRESSURE_KPA))
            {
                *p_keep = 0U;
                return valve_all_off();     /* The next step switches auto mode off. */
            }
        }
        uint32_t ms = (uint32_t)((xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);

        for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
        {
            if (0U == (open & (1UL << k)))
            {
                continue;
            }
            if (0U == (ok & (1UL << k)))
            {
                open &= ~(1UL << k);
                continue;
            }

            bool inflate = (PCTL_ACTION_INFLATE == g_act[k]);
            uint8_t a = inflate ? 0U : 1U;
            /* Mean of the last two reads (noisy sensors), rate over the whole opening: single reads
               only resolve 0.1 kPa. */
            float now_raw = (0U != (have_last & (1UL << k))) ? (0.5f * (raw[k] + last[k])) : raw[k];
            float moved = inflate ? (now_raw - raw0[k]) : (raw0[k] - now_raw);

            last[k] = raw[k];
            have_last |= (1UL << k);

            /* Learned only over a long enough opening: on a noisy sensor (P13, 26.10) one jump over
               100 ms gave a rate ten times too high, so the valve then only got pulses too short to
               move the chamber. */
            if ((moved >= PCTL_MOVE_KPA) && (ms >= PCTL_RATE_MIN_MS))
            {
                g_rate[k][a] = moved / (float)ms;
            }
            left[k] = inflate ? (g_target[k] - p_kpa[k] - moved) : (p_kpa[k] - moved - g_target[k]);
            if (inflate ? ((raw0[k] + moved) >= (g_target[k] - g_inf_lead[k]))
                        : (left[k] <= (g_rate[k][a] * (float)(PCTL_LAG_MS + PCTL_AVG_LAG_MS))))
            {
                open &= ~(1UL << k);
            }
        }
        if (open != was)
        {
            ret = valve_set_mask(pctl_valves(open));
            if (ZERR_OK != ret)
            {
                return ret;
            }
        }
    }

    for (uint8_t k = 0U; k < PCTL_CHAMBER_NUM; k++)
    {
        float rate = g_rate[k][(PCTL_ACTION_INFLATE == g_act[k]) ? 0U : 1U];

        if ((0U != (open & (1UL << k))) && ((PCTL_ACTION_INFLATE == g_act[k]) || (rate <= 0.0f) ||
                                             (left[k] > (rate * (float)PCTL_KEEP_OPEN_MS))))
        {
            *p_keep |= (1UL << k);
        }
    }

    return ZERR_OK;
}

/**
 * @brief  Holding (phase 2): how far off target chamber k may drift before it is adjusted again.
 *         User rule (30.10): 0,3 kPa off (a 0,2 kPa drift is left alone), both ways; wider only when
 *         the chamber's own band is wider (noisy sensor, posture set). It is then brought back to
 *         the target itself (pctl_band).
 */
static float pctl_hold_off(uint8_t k)
{
    float off = pctl_band(k) + PCTL_HYST_KPA;

    return (off > PCTL_HOLD_RESTART_KPA) ? off : PCTL_HOLD_RESTART_KPA;
}

/**
 * @brief  Mean of chamber k's recent scans without the highest and lowest (from 3 scans on).
 *         *p_spread: highest minus lowest.
 */
static float pctl_filtered(uint8_t k, float *p_spread)
{
    float lo = g_hist[k][0];
    float hi = g_hist[k][0];
    float sum = 0.0f;
    uint8_t n = g_hist_n[k];

    for (uint8_t i = 0U; i < n; i++)
    {
        float v = g_hist[k][i];

        sum += v;
        lo = (v < lo) ? v : lo;
        hi = (v > hi) ? v : hi;
    }
    *p_spread = hi - lo;

    return (n >= 3U) ? ((sum - lo - hi) / (float)(n - 2U)) : (sum / (float)n);   /* 3 scans: the middle one. */
}

/**
 * @brief  Band chamber k is held in. User rule (26.10): no tolerance, stop at the target itself, so
 *         the band is half a sensor step (filtered reading equals the target to 0.1 kPa), wider only
 *         for a sensor whose readings spread too much to ever settle there, but within the web band.
 */
static float pctl_band(uint8_t k)
{
    float band = PCTL_NOISE_BAND_GAIN * g_noise[k];

    band = (band > PCTL_NOISE_BAND_MAX_KPA) ? PCTL_NOISE_BAND_MAX_KPA : band;
    /* Never wider than the web tolerance ("sai so"), less the phase 2 hysteresis, so a held
       chamber is always back inside it (26.10: P13 settled at 10.4 kPa, target 10 +/- 0.3). */
    /* RD (user 28.10): while a posture set runs, its chambers are held within +/-0,5 kPa. */
    float tol = ((g_posture_on && (g_cur_set >= 0)) || (0U != g_exp_n)) ? PCTL_POSTURE_TOL_KPA : g_tol_kpa;
    band = (band < (tol - PCTL_HYST_KPA)) ? band : (tol - PCTL_HYST_KPA);

    return (band > PCTL_EXACT_KPA) ? band : PCTL_EXACT_KPA;
}

/**
 * @brief  How far chamber k must move toward its target within PCTL_NO_MOVE_MS: PCTL_MOVE_KPA, less
 *         for a small gap (else a chamber 0.2 kPa off is skipped as stuck), at least one sensor step.
 */
static float pctl_need_move(uint8_t k)
{
    float need = 0.5f * fabsf(g_target[k] - g_act_p[k]);

    need = (need < PCTL_MOVE_KPA) ? need : PCTL_MOVE_KPA;

    return (need > PCTL_SHORT_SEEN_KPA) ? need : PCTL_SHORT_SEEN_KPA;
}

/**
 * @brief  Idle chamber k (valves closed), filtered pressure p: learn its sensor noise, and report a
 *         chamber that moves away from where it settled. g_lock held.
 */
static void pctl_watch_idle_locked(uint8_t k, float p, float spread)
{
    if (g_hist_n[k] < PCTL_FILT_NUM)
    {
        return;
    }

    /* Noise: follows a larger spread at once, a smaller one slowly (a real change also spreads). */
    g_noise[k] = (spread > g_noise[k]) ? spread : (g_noise[k] + (0.05f * (spread - g_noise[k])));
    if ((0U == (g_noisy_mask & (1UL << k))) && (pctl_band(k) > (2.0f * PCTL_EXACT_KPA)))
    {
        g_noisy_mask |= (1UL << k);
        ESP_LOGW(TAG, "P%02u sensor noisy: %.1f kPa spread with valves closed, band +/-%.2f kPa",
                 (unsigned int)(k + 1U), (double)g_noise[k], (double)pctl_band(k));
    }

    if (isnan(g_hold_p[k]))
    {
        g_hold_p[k] = p;
        g_hold_pump[k] = false;
        return;
    }

    float d = p - g_hold_p[k];

    if ((0U == (g_leak_mask & (1UL << k))) && (fabsf(d) > PCTL_LEAK_KPA))
    {
        g_leak_mask |= (1UL << k);
        if (d > 0.0f)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u +%.1f kPa, valves closed%s", (unsigned int)(k + 1U),
                           (double)d, g_hold_pump[k] ? ": inflate valve leak?" : " (load?)");
        }
        else
        {
            (void)snprintf(g_msg, sizeof(g_msg), "P%02u %.1f kPa, valves closed: leak?",
                           (unsigned int)(k + 1U), (double)d);
        }
        ESP_LOGW(TAG, "%s (inflate V%u, exhaust V%u)", g_msg, (unsigned int)(g_inflate_out[k] + 1U),
                 (unsigned int)(g_exhaust_out[k] + 1U));
    }
}

/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : pctl.h
 * @description     : Automatic pressure control for the 13 air chambers (RD 05.10.26): bring every chamber
 *                    to target +/- tolerance one at a time (P01..P13), exhausting together, inflating in order.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef PCTL_H
#define PCTL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "zerr.h"
#include "pressure_array.h"

/* Definitions -----------------------------------------------------------------------------------*/

#define PCTL_CHAMBER_NUM            PRESSURE_SENSOR_NUM     /* Chamber k is read by sensor Pk. */
#define PCTL_LEVEL_NUM              8U                      /* RD pressure levels 1..8. */
#define PCTL_TOLERANCE_KPA          0.5f                    /* RD: +/- 0.5 kPa allowed. */
#define PCTL_START_KPA              0.3f                    /* Default band: adjust a chamber this far off target. */
#define PCTL_TOL_MIN_KPA            0.1f                    /* Band the web may set ("sai so"), +/- kPa. */
#define PCTL_TOL_MAX_KPA            3.0f
#define PCTL_TARGET_MAX_KPA         17.0f                   /* Highest RD level. */
#define PCTL_OVERPRESSURE_KPA       20.0f                   /* Any chamber above this: all off, auto off. */
#define PCTL_ACTION_TIMEOUT_MS      60000U                  /* Default per-chamber time limit (web can change it). */
#define PCTL_TIMEOUT_MAX_S          3600U                   /* Highest time limit the web may set. */
#define PCTL_NO_MOVE_MS             10000U                  /* Warn about a chamber whose pressure did not move */
#define PCTL_MOVE_KPA               0.2f                    /* this much toward the target in that time. */
/* User rule (26.10): chambers above their band exhaust together; inflating goes one chamber at a
   time, P01 to P13 (shared pump). Phase 1: a pass P01..P13, each chamber below its band inflated to
   its target before the next one starts. Phase 2: all chambers checked every scan, those below
   their band take the pump in turn (P01..P13, round again); no chamber is skipped for being slow.
   An overshoot is corrected the other way. While valves are open the sensors are read
   about every 100 ms and each valve closes when the pressure still to go equals what comes in
   during PCTL_LAG_MS (learned rate). */
#define PCTL_TRACK_MS               1500U                   /* Longest time one step follows one chamber. */
#define PCTL_LAG_MS                 100U                    /* One AGR12 read + valve closing. */
#define PCTL_AVG_LAG_MS             50U                     /* Mean of two reads while following. */
#define PCTL_RATE_MIN_MS            500U                    /* Shortest opening a rate is learned from. */
#define PCTL_KEEP_OPEN_MS           1500U                   /* Target further than this: valve stays open. */
#define PCTL_SHORT_GAIN             0.7f                    /* Timed opening covers this share of the gap. */
#define PCTL_SHORT_MIN_US           15000U
#define PCTL_SHORT_MAX_US           300000U
#define PCTL_SHORT_SEEN_KPA         0.1f                    /* Less: the opening was too short, double it. */
#define PCTL_STEP_PERIOD_MS         250U                    /* Scan period while a chamber is adjusted. */
#define PCTL_SEQ_ROUNDS             2U                      /* Phase 1: rounds from the first changed chamber. */
#define PCTL_SEQ_SETTLE_MS          3000U                   /* Phase 1: wait after an exhaust, then check. */
#define PCTL_FAULT_RETRY_MS         30000U                  /* Skipped chambers are retried after 30 s. */

/* Noise and leak handling (26.10, P07 noisy, P13 rising after reaching target). Decisions use the
   mean of the last PCTL_FILT_NUM scans without the highest and lowest. The history restarts after
   every valve opening; near the target a chamber is judged only after PCTL_SETTLE_SCANS fresh scans.
   An idle chamber starts again only further than band + PCTL_HYST_KPA off target. A chamber whose
   readings spread more than twice the band with its valves closed gets a wider band (half that
   spread, at most PCTL_NOISE_BAND_MAX_KPA). An idle chamber that moves more than PCTL_LEAK_KPA from
   where it settled is reported (rise while the pump runs: inflate valve leak; fall: leak). */
#define PCTL_FILT_NUM               3U                      /* User 30.10: middle of the last 3 scans (was 7). */
#define PCTL_SETTLE_SCANS           3U
#define PCTL_FAR_KPA                1.0f                    /* Further off: act on fresh reads at once. */
#define PCTL_HYST_KPA               0.05f                   /* Phase 2 restarts a chamber 0.1 kPa off. */
#define PCTL_HOLD_RESTART_KPA       0.25f                   /* User 30.10: holding acts at 0.3 kPa off (0.1 kPa steps). */
/* Fast inflate (user 30.10): inflate runs continuously (no timed pulses) until the reading reaches
   target - learned lead, its valve and the pump kept open from step to step; a chamber within PCTL_INF_ACCEPT_KPA of
   its target counts as reached. The lead moves by PCTL_INF_LEAD_GAIN x the settled error. */
#define PCTL_INF_ACCEPT_KPA         0.25f
#define PCTL_INF_LEAD_GAIN          0.5f
#define PCTL_INF_LEAD_MAX_KPA       0.5f
#define PCTL_EXACT_KPA              0.05f                   /* Band: half the 0.1 kPa sensor step. */
#define PCTL_NOISE_BAND_GAIN        0.25f                   /* Noisy sensor: band = this x spread. */
#define PCTL_NOISE_BAND_MAX_KPA     1.0f
#define PCTL_LEAK_KPA               0.5f

/* RD step 1 (Pmd): at power-on inflate all 13 chambers to the highest level (17 kPa) and hold it.
   Starts after PCTL_BOOT_GOOD_SCANS scans in a row read all 13 sensors, or after PCTL_BOOT_WAIT_MS
   with the sensors that do read (chambers whose sensor fails are left alone). 0 = start off. */
#define PCTL_AUTO_AT_BOOT           1
#define PCTL_BOOT_GOOD_SCANS        3U
#define PCTL_BOOT_WAIT_MS           10000U
/* User rule (27.10): power-on fills every chamber to PCTL_BOOT_FILL_KPA (at most Pmax), one at a
   time P01..P13, and holds it. Once all are there the web page and OLED ask the person to lie down;
   their "Toi da nam" (pctl_user_on_bed) stops the pump and holding. */
#define PCTL_BOOT_FILL_KPA          16.0f
/* Fill done: holding pauses (valves shut) and the pressures then are the "before" set. After "Toi
   da nam": PCTL_SCAN_NUM scans PCTL_SCAN_PERIOD_MS apart. P1 = target, P2 = mean of the scans, both
   sent to the web rounded to 0.1 kPa; the web finds A, B on P2 - P1. */
#define PCTL_SCAN_NUM               30U
#define PCTL_SCAN_PERIOD_MS         1000U
#define PCTL_P1_SECONDS             30U                     /* P1 = mean of the last 30 s (user 30.10). */

/* RD step 6 (posture check): while auto mode holds, every "period" stop holding for "window" and
   check the saved sets 0 back, 1 side, 2 free (Ptd) as in pctl_posture_step_locked (RD algorithm,
   user 28.10). Period and window are set from the web (defaults RD: hold 5 min, check 1 min). */
#define PCTL_PRESET_NUM             3U
#define PCTL_MATCH_PERIOD_MS        300000U                 /* Default hold time between checks. */
#define PCTL_MATCH_WINDOW_MS        60000U                  /* Default check (pause) time. */
#define PCTL_PERIOD_MIN_S           30U
#define PCTL_PERIOD_MAX_S           3600U
#define PCTL_WINDOW_MIN_S           10U
#define PCTL_WINDOW_MAX_S           600U
#define PCTL_SET_NONE               (-1)
/* RD posture algorithm (user 28.10): a back/side set matches only if every one of its chambers, at
   one shift anywhere on P01..P13, stays within this of the set for the whole check window; the
   same band holds a running posture set. Shifts -12..+12 kept as bits shift + PCTL_SHIFT_OFS. */
#define PCTL_POSTURE_TOL_KPA        0.5f
#define PCTL_SHIFT_OFS              12U
#define PCTL_SHIFT_NUM              25U
#define PCTL_CAND_MAX               8U                      /* Experience mode: up to 8 named sets. */
#define PCTL_EXP_PERIOD_S           120U                    /* Experience: hold 2 min between checks, */
#define PCTL_EXP_WINDOW_S           30U                     /* check (holding paused) 30 s. */

/* Valve outputs on the 74HC595 chain (index 0 = Q0 of the first IC = V1 on the web page).
   Inflate/exhaust valve of each chamber follows the user's table in pctl.c (g_inflate_out,
   g_exhaust_out). V3 pump relay. */
#define PCTL_PUMP_OUTPUT            2U

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief  Load targets (default: all chambers at the highest level, RD "Pmd"). Auto mode starts off.
 */
zerr_t pctl_open(void);

/**
 * @brief  Run one control step with the latest readings (kPa after zero offsets).
 */
void pctl_step(const float *p_kpa, uint32_t ok_mask);

/**
 * @brief  Same as power-on: every chamber to the power-on fill level, hold it and ask the person
 *         to lie down. Used when the web app comes back after being closed. No-op while power-on
 *         still waits, during the fill, or after the person confirmed lying down.
 */
void pctl_go_pmax(void);

/**
 * @brief  Person confirmed lying on the mattress: close every valve, stop the pump, auto mode off.
 */
zerr_t pctl_user_on_bed(void);

/**
 * @brief  True once the power-on fill is done and the person has not confirmed lying down yet.
 */
bool pctl_wait_on_bed(void);

/**
 * @brief  Person is ready to lie down: holding stops at once and P1 becomes the mean of the last
 *         PCTL_P1_SECONDS seconds of readings (used by the next pctl_user_on_bed scan).
 */
zerr_t pctl_user_ready(void);

/**
 * @brief  Cancel creating a pressure set: scan, fill and weight waits end (holding is switched on by the caller).
 */
void pctl_cancel_flow(void);

/**
 * @brief  Experience mode (user 28.10): switch among n (2..8) named sets during sleep with the RD
 *         posture check, the first one the default. Starts holding set 1 at once.
 */
zerr_t pctl_exp_start(const float p_kpa[][PCTL_CHAMBER_NUM], const uint8_t *p_slot, uint8_t n);

/**
 * @brief  End experience mode; the set running now stays held.
 */
void pctl_exp_stop(void);

/**
 * @brief  Switch automatic mode on or off. Off closes every valve and stops the pump.
 */
zerr_t pctl_set_auto(bool on);

bool pctl_is_auto(void);

/**
 * @brief  True while a chamber is being inflated or exhausted (the sensor task then scans faster).
 */
bool pctl_is_busy(void);

/**
 * @brief  Set every chamber to RD level 1..8 (2, 4, 6, 8, 10, 12, 14, 17 kPa).
 */
zerr_t pctl_set_level(uint8_t level);

/**
 * @brief  Set and save (NVS) the pressure every chamber goes to at power-on (RD "Pmd"), 0..17 kPa.
 *         Used from the next power-on.
 */
zerr_t pctl_set_boot_kpa(float kpa);

/**
 * @brief  Set and save (NVS) the band around each target, +/- kPa (PCTL_TOL_MIN_KPA..MAX).
 *         A chamber is adjusted once it is further than this from its target.
 */
zerr_t pctl_set_tol(float kpa);

/**
 * @brief  Set and save (NVS) how long one chamber may inflate/exhaust before it is skipped,
 *         0..PCTL_TIMEOUT_MAX_S seconds. 0 = no limit.
 */
zerr_t pctl_set_timeout(uint32_t seconds);

/**
 * @brief  RD weight rule: <= 60 kg level 6, <= 80 kg level 7, otherwise level 8.
 */
zerr_t pctl_set_weight(float kg);

/**
 * @brief  Set one chamber (0..12) target in kPa (0..17).
 */
zerr_t pctl_set_target(uint8_t index, float kpa);

/**
 * @brief  Set all 13 targets at once (kPa, 0..17). Auto mode then holds them until switched off.
 */
zerr_t pctl_set_targets(const float *p_kpa);

/**
 * @brief  Copy the 13 targets now held (kPa).
 */
void pctl_get_targets(float *p_kpa);

/**
 * @brief  Choose which chambers auto mode adjusts (bit k = chamber k+1). Others are left alone.
 */
zerr_t pctl_set_enable(uint32_t mask);

/**
 * @brief  Posture check on/off, hold time between checks and check time (seconds), saved in NVS.
 */
zerr_t pctl_set_posture(bool on, uint32_t period_s, uint32_t window_s);

/**
 * @brief  Give auto mode a saved set (slot 0 back, 1 side, 2 free). ok = false forgets it.
 */
void pctl_set_preset(uint8_t slot, const float *p_kpa, uint32_t enable, bool ok);

/**
 * @brief  Tell auto mode which saved set the current targets come from (PCTL_SET_NONE = none).
 */
void pctl_set_current(int8_t slot);

/**
 * @brief  Short multi-line control status for the OLED (16 chars per line). Returns the length.
 */
size_t pctl_oled_text(char *p_buf, size_t size);

/**
 * @brief  Write the control state as a JSON object (no newline). Returns the length.
 */
size_t pctl_json(char *p_buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* PCTL_H */

/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : devctl.c
 * @description     : Device state and JSON commands shared by the web server and the USB serial link.
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
#include "esp_timer.h"
#include "driver/uart.h"
#include "cJSON.h"

#include "devctl.h"
#include "webui.h"
#include "pctl.h"
#include "flash.h"
#if DEVCTL_VALVE_ENABLE
#include "valve.h"
#else
#define VALVE_OUTPUT_NUM            32U
#endif

/* Define ----------------------------------------------------------------------------------------*/

#define DEVCTL_NVS_KEY_ZERO         "p_zero"
#define DEVCTL_NVS_KEY_PERIOD       "scan_ms"
#define DEVCTL_PRESET_NUM           3U          /* RD saved sets: 0 = back (Pu-tc), 1 = side (Pi-tc), 2 = free (Ptd). */
#define DEVCTL_PERIOD_DEFAULT_MS    1000U
#define DEVCTL_PERIOD_MIN_MS        200U
#define DEVCTL_PERIOD_MAX_MS        10000U
#define DEVCTL_REOPEN_MS            30000U      /* No web client this long, then one: app reopened. */

#define DEVCTL_UART                 UART_NUM_0
#define DEVCTL_UART_RX_BUF          1024
#define DEVCTL_LINE_MAX             256U
#define DEVCTL_SERIAL_PREFIX_CMD    "@cmd "
#define DEVCTL_SERIAL_PREFIX_JSON   "@J "
#define DEVCTL_SERIAL_STACK         6144U

/* Private variables -----------------------------------------------------------------------------*/

static SemaphoreHandle_t g_lock = NULL;
static float g_raw[PRESSURE_SENSOR_NUM];
static float g_zero[PRESSURE_SENSOR_NUM];
static uint32_t g_ok_mask = 0U;
static uint32_t g_seq = 0U;
static uint32_t g_period_ms = DEVCTL_PERIOD_DEFAULT_MS;
static volatile TickType_t g_last_client = 0;
#if !DEVCTL_VALVE_ENABLE
static const uint32_t g_valve_mask = 0U;
#endif

static const char *TAG = "DEVCTL";

/* Private function prototypes -------------------------------------------------------------------*/

static size_t devctl_append_floats(char *p_buf, size_t size, const float *p_val, uint32_t mask,
                                   bool use_mask);
static void devctl_reply(char *p_reply, size_t size, bool ok, const char *p_msg);
static size_t devctl_presets_json(char *p_buf, size_t size);
static void devctl_serial_task(void *p_para);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t devctl_open(void)
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

    memset(g_zero, 0, sizeof(g_zero));
    if ((ZERR_OK != flash_read(DEVCTL_NVS_KEY_ZERO, g_zero, sizeof(g_zero), &len)) ||
        (sizeof(g_zero) != len))
    {
        memset(g_zero, 0, sizeof(g_zero));
    }

    uint32_t period = 0U;
    if ((ZERR_OK == flash_read(DEVCTL_NVS_KEY_PERIOD, &period, sizeof(period), &len)) &&
        (sizeof(period) == len) && (period >= DEVCTL_PERIOD_MIN_MS) &&
        (period <= DEVCTL_PERIOD_MAX_MS))
    {
        g_period_ms = period;
    }

#if DEVCTL_VALVE_ENABLE
    zerr_t ret = valve_open();
    if (ZERR_OK != ret)
    {
        ESP_LOGE(TAG, "valve open failed: %d", (int)ret);
    }
#else
    ESP_LOGW(TAG, "valve outputs locked (DEVCTL_VALVE_ENABLE = 0)");
#endif

    (void)pctl_open();
    {
        char tmp[8];
        (void)devctl_presets_json(tmp, sizeof(tmp));    /* Loads the saved sets from NVS. */
    }

    return ZERR_OK;
}

uint32_t devctl_period_ms(void)
{
    return g_period_ms;
}

void devctl_touch(void)
{
    TickType_t now = xTaskGetTickCount();

    /* User rule: reopening the web app brings the mattress back to Pmax, as at power-on.
       Runs before the request's own command, so e.g. "auto off" sent right away still wins. */
    if (DEVCTL_VALVE_ENABLE && (0 != g_last_client) &&
        ((now - g_last_client) > pdMS_TO_TICKS(DEVCTL_REOPEN_MS)))
    {
        pctl_go_pmax();
    }
    g_last_client = now;
}

void devctl_tick(void)
{
#if DEVCTL_VALVE_ENABLE && (DEVCTL_VALVE_IDLE_OFF_MS > 0)
    if ((!pctl_is_auto()) && (0U != valve_get_mask()) &&
        ((xTaskGetTickCount() - g_last_client) > pdMS_TO_TICKS(DEVCTL_VALVE_IDLE_OFF_MS)))
    {
        ESP_LOGW(TAG, "no web client for %u s, all valves off",
                 (unsigned int)(DEVCTL_VALVE_IDLE_OFF_MS / 1000U));
        (void)valve_all_off();
    }
#endif
}

void devctl_update(const float *p_raw, uint32_t ok_mask, float *p_out)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
    for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
    {
        g_raw[i] = p_raw[i];
        p_out[i] = p_raw[i] - g_zero[i];
    }
    g_ok_mask = ok_mask;
    g_seq++;
    xSemaphoreGive(g_lock);
}

size_t devctl_state_json(char *p_buf, size_t size)
{
    float val[PRESSURE_SENSOR_NUM];
    size_t len;

    xSemaphoreTake(g_lock, portMAX_DELAY);
    for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
    {
        val[i] = g_raw[i] - g_zero[i];
    }

    len = (size_t)snprintf(p_buf, size, "{\"t\":%llu,\"seq\":%lu,\"period\":%lu,\"ok\":%lu,\"p\":",
                           (unsigned long long)(esp_timer_get_time() / 1000),
                           (unsigned long)g_seq, (unsigned long)g_period_ms,
                           (unsigned long)g_ok_mask);
    if (len < size)
    {
        len += devctl_append_floats(&p_buf[len], size - len, val, g_ok_mask, true);
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, ",\"raw\":");
    }
    if (len < size)
    {
        len += devctl_append_floats(&p_buf[len], size - len, g_raw, g_ok_mask, true);
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, ",\"zero\":");
    }
    if (len < size)
    {
        len += devctl_append_floats(&p_buf[len], size - len, g_zero, 0U, false);
    }
    xSemaphoreGive(g_lock);

    if (len < size)
    {
#if DEVCTL_VALVE_ENABLE
        uint32_t mask = valve_get_mask();
#else
        uint32_t mask = g_valve_mask;
#endif
        uint8_t pins[3] = {0U, 0U, 0U};
#if DEVCTL_VALVE_ENABLE
        valve_get_pins(pins);
#endif
        len += (size_t)snprintf(&p_buf[len], size - len,
                                ",\"valve\":{\"enabled\":%s,\"num\":%u,\"mask\":%lu,"
                                "\"pins\":[%u,%u,%u]},\"wifi\":",
                                DEVCTL_VALVE_ENABLE ? "true" : "false",
                                (unsigned int)VALVE_OUTPUT_NUM, (unsigned long)mask,
                                pins[0], pins[1], pins[2]);
    }
    if (len < size)
    {
        len += webui_wifi_json(&p_buf[len], size - len);
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, ",\"auto\":");
    }
    if (len < size)
    {
        len += pctl_json(&p_buf[len], size - len);
    }
    if (len < size)
    {
        len += devctl_presets_json(&p_buf[len], size - len);
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "}");
    }

    return (len < size) ? len : (size - 1U);
}

/* One saved pressure set: 13 targets and which chambers it regulates. */
typedef struct
{
    float kpa[PRESSURE_SENSOR_NUM];
    uint32_t enable;
} devctl_preset_t;

static const char *const g_preset_key[DEVCTL_PRESET_NUM] = {"preset_u", "preset_i", "preset_t"};
static devctl_preset_t g_preset[DEVCTL_PRESET_NUM];     /* Cache of NVS, loaded once. */
static bool g_preset_ok[DEVCTL_PRESET_NUM];
static bool g_preset_loaded = false;

/**
 * @brief  Append ,"presets":[{..}|null,{..}|null] with the sets saved in NVS.
 */
static size_t devctl_presets_json(char *p_buf, size_t size)
{
    size_t len = (size_t)snprintf(p_buf, size, ",\"presets\":[");

    if (!g_preset_loaded)
    {
        for (uint8_t n = 0U; n < DEVCTL_PRESET_NUM; n++)
        {
            size_t got = 0U;
            g_preset_ok[n] = (ZERR_OK == flash_read(g_preset_key[n], &g_preset[n],
                                                    sizeof(g_preset[n]), &got)) &&
                             (sizeof(g_preset[n]) == got);
        }
        g_preset_loaded = true;
        for (uint8_t n = 0U; n < DEVCTL_PRESET_NUM; n++)
        {
            pctl_set_preset(n, g_preset[n].kpa, g_preset[n].enable, g_preset_ok[n]);
        }
    }

    for (uint8_t n = 0U; (n < DEVCTL_PRESET_NUM) && (len < size); n++)
    {
        const devctl_preset_t set = g_preset[n];
        const char *p_sep = (0U == n) ? "" : ",";

        if (!g_preset_ok[n])
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "%snull", p_sep);
            continue;
        }
        len += (size_t)snprintf(&p_buf[len], size - len, "%s{\"en\":%lu,\"kpa\":[", p_sep,
                                (unsigned long)set.enable);
        for (uint8_t i = 0U; (i < PRESSURE_SENSOR_NUM) && (len < size); i++)
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "%s%.1f", (0U == i) ? "" : ",",
                                    (double)set.kpa[i]);
        }
        if (len < size)
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "]}");
        }
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "]");
    }

    return (len < size) ? len : (size - 1U);
}

/* User rule (28.10): named pressure sets saved after P3 is reached, DEVCTL_CFG_NUM slots in NVS. */
typedef struct
{
    char name[DEVCTL_CFG_NAME_LEN];
    float kpa[PRESSURE_SENSOR_NUM];
} devctl_cfg_t;

static devctl_cfg_t g_cfg[DEVCTL_CFG_NUM];      /* Cache of NVS, name[0] = 0: slot free. */
static bool g_cfg_loaded = false;

static void devctl_cfg_key(uint8_t slot, char *p_key, size_t size)
{
    (void)snprintf(p_key, size, "cfg%u", (unsigned int)slot);
}

static void devctl_cfg_load(void)
{
    char key[8];

    if (g_cfg_loaded)
    {
        return;
    }
    for (uint8_t n = 0U; n < DEVCTL_CFG_NUM; n++)
    {
        size_t got = 0U;

        devctl_cfg_key(n, key, sizeof(key));
        if ((ZERR_OK != flash_read(key, &g_cfg[n], sizeof(g_cfg[n]), &got)) || (sizeof(g_cfg[n]) != got))
        {
            memset(&g_cfg[n], 0, sizeof(g_cfg[n]));
        }
        g_cfg[n].name[DEVCTL_CFG_NAME_LEN - 1U] = '\0';
    }
    g_cfg_loaded = true;
}

size_t devctl_cfgs_json(char *p_buf, size_t size)
{
    size_t len;

    xSemaphoreTake(g_lock, portMAX_DELAY);
    devctl_cfg_load();
    len = (size_t)snprintf(p_buf, size, "{\"cfgs\":[");
    bool first = true;
    for (uint8_t n = 0U; (n < DEVCTL_CFG_NUM) && (len < size); n++)
    {
        if ('\0' == g_cfg[n].name[0])
        {
            continue;
        }
        len += (size_t)snprintf(&p_buf[len], size - len, "%s{\"slot\":%u,\"name\":\"%s\",\"kpa\":[",
                                first ? "" : ",", (unsigned int)n, g_cfg[n].name);
        first = false;
        for (uint8_t i = 0U; (i < PRESSURE_SENSOR_NUM) && (len < size); i++)
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "%s%.1f", (0U == i) ? "" : ",",
                                    (double)g_cfg[n].kpa[i]);
        }
        if (len < size)
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "]}");
        }
    }
    xSemaphoreGive(g_lock);
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "],\"max\":%u}", (unsigned int)DEVCTL_CFG_NUM);
    }

    return (len < size) ? len : (size - 1U);
}

/**
 * @brief  Save named set: same name overwrites, else the first free slot. Name kept JSON-safe.
 */
static zerr_t devctl_cfg_save(const char *p_name, const float *p_kpa)
{
    devctl_cfg_t cfg;
    int slot = -1;
    char key[8];
    size_t i = 0U;

    memset(&cfg, 0, sizeof(cfg));
    for (; (i < (DEVCTL_CFG_NAME_LEN - 1U)) && ('\0' != p_name[i]); i++)
    {
        unsigned char c = (unsigned char)p_name[i];
        cfg.name[i] = ((c < 0x20U) || ('"' == c) || ('\\' == c)) ? ' ' : (char)c;
    }
    /* Name cut short: do not cut a UTF-8 character in half. */
    while ((i > 0U) && (0x80U == ((unsigned char)p_name[i] & 0xC0U)))
    {
        i--;
    }
    cfg.name[i] = '\0';
    if ('\0' == cfg.name[0])
    {
        return ZERR_INVALID_PARAM;
    }
    memcpy(cfg.kpa, p_kpa, sizeof(cfg.kpa));

    devctl_cfg_load();
    for (uint8_t n = 0U; (n < DEVCTL_CFG_NUM) && (slot < 0); n++)
    {
        slot = (0 == strcmp(g_cfg[n].name, cfg.name)) ? (int)n : slot;
    }
    for (uint8_t n = 0U; (n < DEVCTL_CFG_NUM) && (slot < 0); n++)
    {
        slot = ('\0' == g_cfg[n].name[0]) ? (int)n : slot;
    }
    if (slot < 0)
    {
        return ZERR_MEM_OUT;
    }
    devctl_cfg_key((uint8_t)slot, key, sizeof(key));
    zerr_t ret = flash_write(key, &cfg, sizeof(cfg));
    if (ZERR_OK == ret)
    {
        g_cfg[slot] = cfg;
    }

    return ret;
}

zerr_t devctl_command(const char *p_json, char *p_reply, size_t reply_size)
{
    zerr_t ret = ZERR_INVALID_PARAM;
    const char *p_msg = "unknown command";
    cJSON *p_root = cJSON_Parse(p_json);
    const cJSON *p_cmd = cJSON_GetObjectItemCaseSensitive(p_root, "cmd");

    if (!cJSON_IsString(p_cmd))
    {
        cJSON_Delete(p_root);
        devctl_reply(p_reply, reply_size, false, "expected {\"cmd\":\"...\"}");
        return ZERR_INVALID_PARAM;
    }

    const char *p_name = p_cmd->valuestring;

    devctl_touch();

    if ((0 == strcmp(p_name, "state")) || (0 == strcmp(p_name, "ping")))
    {
        ret = ZERR_OK;
        p_msg = "state";
    }
    else if ((0 == strcmp(p_name, "tare")) || (0 == strcmp(p_name, "tare_clear")))
    {
        bool clear = (0 == strcmp(p_name, "tare_clear"));

        xSemaphoreTake(g_lock, portMAX_DELAY);
        for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
        {
            if (clear)
            {
                g_zero[i] = 0.0f;
            }
            else if (0U != (g_ok_mask & (1UL << i)))
            {
                g_zero[i] = g_raw[i];   /* Sensors in error keep their old offset. */
            }
        }
        ret = flash_write(DEVCTL_NVS_KEY_ZERO, g_zero, sizeof(g_zero));
        xSemaphoreGive(g_lock);
        p_msg = (ZERR_OK != ret) ? "could not save offsets" : (clear ? "zero cleared" : "zeroed");
        ESP_LOGI(TAG, "%s", p_msg);
    }
    else if (0 == strcmp(p_name, "period"))
    {
        const cJSON *p_ms = cJSON_GetObjectItemCaseSensitive(p_root, "ms");

        if (cJSON_IsNumber(p_ms) && (p_ms->valuedouble >= DEVCTL_PERIOD_MIN_MS) &&
            (p_ms->valuedouble <= DEVCTL_PERIOD_MAX_MS))
        {
            uint32_t period = (uint32_t)p_ms->valuedouble;

            g_period_ms = period;
            ret = flash_write(DEVCTL_NVS_KEY_PERIOD, &period, sizeof(period));
            p_msg = "period set";
        }
        else
        {
            p_msg = "ms must be 200..10000";
        }
    }
    else if (0 == strcmp(p_name, "wifi"))
    {
        const cJSON *p_ssid = cJSON_GetObjectItemCaseSensitive(p_root, "ssid");
        const cJSON *p_pass = cJSON_GetObjectItemCaseSensitive(p_root, "pass");

        if (cJSON_IsString(p_ssid) && (strlen(p_ssid->valuestring) <= 32U) &&
            ((NULL == p_pass) || (cJSON_IsString(p_pass) && (strlen(p_pass->valuestring) <= 64U))))
        {
            ret = webui_set_sta(p_ssid->valuestring, (NULL != p_pass) ? p_pass->valuestring : "");
            p_msg = (ZERR_OK == ret) ? "connecting" : "wifi error";
        }
        else
        {
            p_msg = "ssid (<=32) and pass (<=64) required";
        }
    }
    else if ((0 == strcmp(p_name, "valve")) || (0 == strcmp(p_name, "valve_mask")) ||
             (0 == strcmp(p_name, "valve_off")))
    {
#if DEVCTL_VALVE_ENABLE
        if (pctl_is_auto() && (0 != strcmp(p_name, "valve_off")))
        {
            ret = ZERR_EXIST;
            p_msg = "auto mode on: switch it off to use valves by hand";
        }
        else if (0 == strcmp(p_name, "valve_off"))
        {
            (void)pctl_set_auto(false); /* Prevent the next scan from reopening a valve. */
            ret = valve_all_off();
        }
        else if (0 == strcmp(p_name, "valve_mask"))
        {
            const cJSON *p_mask = cJSON_GetObjectItemCaseSensitive(p_root, "mask");
            ret = (cJSON_IsNumber(p_mask) && p_mask->valuedouble >= 0.0 &&
                   p_mask->valuedouble <= 4294967295.0 &&
                   floor(p_mask->valuedouble) == p_mask->valuedouble)
                      ? valve_set_mask((uint32_t)p_mask->valuedouble)
                                         : ZERR_INVALID_PARAM;
        }
        else
        {
            const cJSON *p_idx = cJSON_GetObjectItemCaseSensitive(p_root, "index");
            const cJSON *p_on = cJSON_GetObjectItemCaseSensitive(p_root, "on");
            ret = (cJSON_IsNumber(p_idx) && p_idx->valuedouble >= 0.0 &&
                   p_idx->valuedouble < VALVE_OUTPUT_NUM &&
                   p_idx->valuedouble == p_idx->valueint && cJSON_IsBool(p_on))
                      ? valve_set((uint8_t)p_idx->valueint, cJSON_IsTrue(p_on))
                      : ZERR_INVALID_PARAM;
        }
        if (ZERR_EXIST != ret)
        {
            p_msg = (ZERR_OK == ret) ? "valve set" : "valve error";
        }
#else
        ret = ZERR_NOT_OPEN;
        p_msg = "valve outputs locked in firmware (DEVCTL_VALVE_ENABLE = 0)";
#endif
    }

    else if (0 == strcmp(p_name, "valve_pins"))
    {
#if DEVCTL_VALVE_ENABLE
        const cJSON *p_ds = cJSON_GetObjectItemCaseSensitive(p_root, "ds");
        const cJSON *p_sh = cJSON_GetObjectItemCaseSensitive(p_root, "shcp");
        const cJSON *p_st = cJSON_GetObjectItemCaseSensitive(p_root, "stcp");

        if (pctl_is_auto())
        {
            (void)pctl_set_auto(false);
        }
        ret = (cJSON_IsNumber(p_ds) && cJSON_IsNumber(p_sh) && cJSON_IsNumber(p_st))
                  ? valve_set_pins((uint8_t)p_ds->valueint, (uint8_t)p_sh->valueint,
                                   (uint8_t)p_st->valueint)
                  : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "valve pins set" : "invalid valve pins";
#else
        ret = ZERR_NOT_OPEN;
        p_msg = "valve outputs locked in firmware (DEVCTL_VALVE_ENABLE = 0)";
#endif
    }
    else if (0 == strcmp(p_name, "auto"))
    {
        const cJSON *p_on = cJSON_GetObjectItemCaseSensitive(p_root, "on");

        if (cJSON_IsBool(p_on) && DEVCTL_VALVE_ENABLE)
        {
            ret = pctl_set_auto(cJSON_IsTrue(p_on));
            p_msg = cJSON_IsTrue(p_on) ? "auto on" : "auto off";
        }
        else
        {
            p_msg = "on must be true/false (valves must be enabled)";
        }
    }
    else if (0 == strcmp(p_name, "cfg_save"))
    {
        const cJSON *p_nm = cJSON_GetObjectItemCaseSensitive(p_root, "name");
        const cJSON *p_arr = cJSON_GetObjectItemCaseSensitive(p_root, "kpa");
        float kpa[PRESSURE_SENSOR_NUM];

        ret = (cJSON_IsString(p_nm) && cJSON_IsArray(p_arr) && (PRESSURE_SENSOR_NUM == cJSON_GetArraySize(p_arr)))
                  ? ZERR_OK : ZERR_INVALID_PARAM;
        for (uint8_t i = 0U; (ZERR_OK == ret) && (i < PRESSURE_SENSOR_NUM); i++)
        {
            const cJSON *p_v = cJSON_GetArrayItem(p_arr, i);
            ret = (cJSON_IsNumber(p_v) && (p_v->valuedouble >= 0.0) && (p_v->valuedouble <= 17.0))
                      ? ZERR_OK : ZERR_INVALID_PARAM;
            kpa[i] = (ZERR_OK == ret) ? (float)p_v->valuedouble : 0.0f;
        }
        if (ZERR_OK == ret)
        {
            xSemaphoreTake(g_lock, portMAX_DELAY);
            ret = devctl_cfg_save(p_nm->valuestring, kpa);
            xSemaphoreGive(g_lock);
        }
        p_msg = (ZERR_OK == ret) ? "config saved" : ((ZERR_MEM_OUT == ret) ? "config list full" : "name and 13 kpa 0..17 needed");
    }
    else if (0 == strcmp(p_name, "cfg_del"))
    {
        const cJSON *p_slot = cJSON_GetObjectItemCaseSensitive(p_root, "slot");
        char key[8];

        ret = (cJSON_IsNumber(p_slot) && (p_slot->valueint >= 0) && (p_slot->valueint < (int)DEVCTL_CFG_NUM))
                  ? ZERR_OK : ZERR_INVALID_PARAM;
        if (ZERR_OK == ret)
        {
            xSemaphoreTake(g_lock, portMAX_DELAY);
            devctl_cfg_load();
            memset(&g_cfg[p_slot->valueint], 0, sizeof(g_cfg[0]));
            devctl_cfg_key((uint8_t)p_slot->valueint, key, sizeof(key));
            ret = flash_write(key, &g_cfg[p_slot->valueint], sizeof(g_cfg[0]));
            xSemaphoreGive(g_lock);
        }
        p_msg = (ZERR_OK == ret) ? "config deleted" : "slot 0..7 needed";
    }
    else if (0 == strcmp(p_name, "exp_start"))
    {
        /* User rule (28.10): "Trai nghiem" with 2..8 named sets, in the order chosen. */
        const cJSON *p_arr = cJSON_GetObjectItemCaseSensitive(p_root, "slots");
        float kpa[PCTL_CAND_MAX][PRESSURE_SENSOR_NUM];
        uint8_t slot[PCTL_CAND_MAX];
        int n = cJSON_IsArray(p_arr) ? cJSON_GetArraySize(p_arr) : 0;

        ret = ((n >= 2) && (n <= (int)PCTL_CAND_MAX)) ? ZERR_OK : ZERR_INVALID_PARAM;
        xSemaphoreTake(g_lock, portMAX_DELAY);
        devctl_cfg_load();
        for (int i = 0; (ZERR_OK == ret) && (i < n); i++)
        {
            const cJSON *p_v = cJSON_GetArrayItem(p_arr, i);
            int s = cJSON_IsNumber(p_v) ? p_v->valueint : -1;

            ret = ((s >= 0) && (s < (int)DEVCTL_CFG_NUM) && ('\0' != g_cfg[s].name[0])) ? ZERR_OK : ZERR_INVALID_PARAM;
            if (ZERR_OK == ret)
            {
                memcpy(kpa[i], g_cfg[s].kpa, sizeof(kpa[i]));
                slot[i] = (uint8_t)s;
            }
        }
        xSemaphoreGive(g_lock);
        ret = (ZERR_OK == ret) ? pctl_exp_start(kpa, slot, (uint8_t)n) : ret;
        p_msg = (ZERR_OK == ret) ? "experience started" : "2..8 saved config slots needed";
    }
    else if (0 == strcmp(p_name, "exp_stop"))
    {
        pctl_exp_stop();
        ret = ZERR_OK;
        p_msg = "experience stopped";
    }
    else if (0 == strcmp(p_name, "cancel_flow"))
    {
        /* User rule (30.10): "Huy tao cau hinh" - stop the scan / waits, hold the current targets. */
        pctl_cancel_flow();
        ret = pctl_set_auto(true);
        p_msg = "config creation cancelled: holding current targets";
    }
    else if (0 == strcmp(p_name, "ready_lie"))
    {
        ret = pctl_user_ready();
        p_msg = "ready to lie down: holding off, P1 taken";
    }
    else if (0 == strcmp(p_name, "on_bed"))
    {
        ret = pctl_user_on_bed();
        p_msg = "on the mattress: pump off";
    }
    else if (0 == strcmp(p_name, "level"))
    {
        const cJSON *p_lv = cJSON_GetObjectItemCaseSensitive(p_root, "level");

        ret = (cJSON_IsNumber(p_lv) && p_lv->valuedouble >= 1.0 &&
               p_lv->valuedouble <= PCTL_LEVEL_NUM && p_lv->valuedouble == p_lv->valueint)
                  ? pctl_set_level((uint8_t)p_lv->valueint) : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "targets set" : "level must be 1..8";
    }
    else if (0 == strcmp(p_name, "boot_kpa"))
    {
        const cJSON *p_kpa = cJSON_GetObjectItemCaseSensitive(p_root, "kpa");

        ret = cJSON_IsNumber(p_kpa) ? pctl_set_boot_kpa((float)p_kpa->valuedouble) : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "power-on level saved" : "kpa must be 0..17";
    }
    else if (0 == strcmp(p_name, "timeout"))
    {
        const cJSON *p_s = cJSON_GetObjectItemCaseSensitive(p_root, "s");

        ret = (cJSON_IsNumber(p_s) && (p_s->valuedouble >= 0.0) && (p_s->valuedouble == p_s->valueint))
                  ? pctl_set_timeout((uint32_t)p_s->valueint) : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "time limit saved" : "s must be 0..3600";
    }
    else if (0 == strcmp(p_name, "posture"))
    {
        const cJSON *p_on = cJSON_GetObjectItemCaseSensitive(p_root, "on");
        const cJSON *p_per = cJSON_GetObjectItemCaseSensitive(p_root, "period_s");
        const cJSON *p_win = cJSON_GetObjectItemCaseSensitive(p_root, "window_s");

        ret = (cJSON_IsBool(p_on) && cJSON_IsNumber(p_per) && cJSON_IsNumber(p_win) &&
               (p_per->valuedouble >= 0.0) && (p_win->valuedouble >= 0.0))
                  ? pctl_set_posture(cJSON_IsTrue(p_on), (uint32_t)p_per->valuedouble,
                                     (uint32_t)p_win->valuedouble)
                  : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "posture check saved" : "period 30..3600 s, window 10..600 s";
    }
    else if (0 == strcmp(p_name, "tol"))
    {
        const cJSON *p_kpa = cJSON_GetObjectItemCaseSensitive(p_root, "kpa");

        ret = cJSON_IsNumber(p_kpa) ? pctl_set_tol((float)p_kpa->valuedouble) : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "band saved" : "kpa must be 0.1..3";
    }
    else if (0 == strcmp(p_name, "weight"))
    {
        const cJSON *p_kg = cJSON_GetObjectItemCaseSensitive(p_root, "kg");

        ret = cJSON_IsNumber(p_kg) ? pctl_set_weight((float)p_kg->valuedouble) : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "weight set" : "kg must be 1..200";
        if (ZERR_OK == ret)
        {
            /* User rule (28.10): the weight set is Ptd, the free set: saved as set 2 (free) and as the
               named set "Ptd". "Tao cau hinh moi" on the web starts from it. */
            devctl_preset_t set;

            pctl_get_targets(set.kpa);
            set.enable = (1UL << PRESSURE_SENSOR_NUM) - 1UL;
            xSemaphoreTake(g_lock, portMAX_DELAY);
            if ((ZERR_OK == flash_write(g_preset_key[2], &set, sizeof(set))))
            {
                g_preset[2] = set;
                g_preset_ok[2] = true;
            }
            (void)devctl_cfg_save("Ptd", set.kpa);
            xSemaphoreGive(g_lock);
            pctl_set_preset(2U, set.kpa, set.enable, true);
        }
    }
    else if (0 == strcmp(p_name, "preset_save"))
    {
        const cJSON *p_slot = cJSON_GetObjectItemCaseSensitive(p_root, "slot");
        const cJSON *p_arr = cJSON_GetObjectItemCaseSensitive(p_root, "kpa");
        const cJSON *p_en = cJSON_GetObjectItemCaseSensitive(p_root, "enable");
        devctl_preset_t set;

        ret = (cJSON_IsNumber(p_slot) && (p_slot->valueint >= 0) &&
               (p_slot->valueint < (int)DEVCTL_PRESET_NUM) && cJSON_IsArray(p_arr) &&
               (PRESSURE_SENSOR_NUM == cJSON_GetArraySize(p_arr)) && cJSON_IsNumber(p_en))
                  ? ZERR_OK
                  : ZERR_INVALID_PARAM;
        for (uint8_t i = 0U; (ZERR_OK == ret) && (i < PRESSURE_SENSOR_NUM); i++)
        {
            const cJSON *p_v = cJSON_GetArrayItem(p_arr, i);
            if (cJSON_IsNumber(p_v) && (p_v->valuedouble >= 0.0) && (p_v->valuedouble <= 17.0))
            {
                set.kpa[i] = (float)p_v->valuedouble;
            }
            else
            {
                ret = ZERR_INVALID_PARAM;
            }
        }
        if (ZERR_OK == ret)
        {
            set.enable = (uint32_t)p_en->valuedouble & ((1UL << PRESSURE_SENSOR_NUM) - 1UL);
            ret = flash_write(g_preset_key[p_slot->valueint], &set, sizeof(set));
        }
        if (ZERR_OK == ret)
        {
            g_preset[p_slot->valueint] = set;
            g_preset_ok[p_slot->valueint] = true;
            pctl_set_preset((uint8_t)p_slot->valueint, set.kpa, set.enable, true);
        }
        p_msg = (ZERR_OK == ret) ? "preset saved" : "slot 0/1/2, 13 values 0..17 kPa, enable";
    }
    else if (0 == strcmp(p_name, "targets"))
    {
        const cJSON *p_arr = cJSON_GetObjectItemCaseSensitive(p_root, "kpa");
        float kpa[PRESSURE_SENSOR_NUM];

        ret = (cJSON_IsArray(p_arr) && (PRESSURE_SENSOR_NUM == cJSON_GetArraySize(p_arr)))
                  ? ZERR_OK
                  : ZERR_INVALID_PARAM;
        for (uint8_t i = 0U; (ZERR_OK == ret) && (i < PRESSURE_SENSOR_NUM); i++)
        {
            const cJSON *p_v = cJSON_GetArrayItem(p_arr, i);
            if (cJSON_IsNumber(p_v))
            {
                kpa[i] = (float)p_v->valuedouble;
            }
            else
            {
                ret = ZERR_INVALID_PARAM;
            }
        }
        if (ZERR_OK == ret)
        {
            ret = pctl_set_targets(kpa);
        }

        /* Optional "set": which saved set these targets come from (0 back, 1 side, 2 free). */
        const cJSON *p_set = cJSON_GetObjectItemCaseSensitive(p_root, "set");
        if (ZERR_OK == ret)
        {
            pctl_set_current(cJSON_IsNumber(p_set) ? (int8_t)p_set->valueint : PCTL_SET_NONE);
        }

        /* Optional "enable": bit k = chamber k+1 is adjusted; default all 13. */
        const cJSON *p_en = cJSON_GetObjectItemCaseSensitive(p_root, "enable");
        if ((ZERR_OK == ret) && cJSON_IsNumber(p_en))
        {
            ret = pctl_set_enable((uint32_t)p_en->valuedouble);
        }
        p_msg = (ZERR_OK == ret) ? "targets set" : "13 values 0..17 kPa required";
    }
    else if (0 == strcmp(p_name, "target"))
    {
        const cJSON *p_idx = cJSON_GetObjectItemCaseSensitive(p_root, "index");
        const cJSON *p_val = cJSON_GetObjectItemCaseSensitive(p_root, "kpa");

        ret = (cJSON_IsNumber(p_idx) && p_idx->valuedouble >= 0.0 &&
               p_idx->valuedouble < PRESSURE_SENSOR_NUM &&
               p_idx->valuedouble == p_idx->valueint && cJSON_IsNumber(p_val))
                  ? pctl_set_target((uint8_t)p_idx->valueint, (float)p_val->valuedouble)
                  : ZERR_INVALID_PARAM;
        p_msg = (ZERR_OK == ret) ? "targets set" : "index 0..12, kpa 0..17";
    }

    cJSON_Delete(p_root);
    devctl_reply(p_reply, reply_size, (ZERR_OK == ret), p_msg);

    return ret;
}

zerr_t devctl_serial_open(void)
{
    if (!uart_is_driver_installed(DEVCTL_UART))
    {
        if (ESP_OK != uart_driver_install(DEVCTL_UART, DEVCTL_UART_RX_BUF, 0, 0, NULL, 0))
        {
            return ZERR_NOT_OPEN;
        }
    }

    if (pdPASS != xTaskCreate(devctl_serial_task, "devctl serial", DEVCTL_SERIAL_STACK, NULL,
                              tskIDLE_PRIORITY + 1U, NULL))
    {
        return ZERR_MEM_OUT;
    }

    return ZERR_OK;
}

void devctl_serial_push_state(void)
{
    char buf[DEVCTL_JSON_MAX];

    (void)devctl_state_json(buf, sizeof(buf));
    printf(DEVCTL_SERIAL_PREFIX_JSON "%s\n", buf);
}

/* Private functions -----------------------------------------------------------------------------*/

/**
 * @brief  Append [v0,v1,...] with two decimals; values not in mask become null when use_mask.
 */
static size_t devctl_append_floats(char *p_buf, size_t size, const float *p_val, uint32_t mask,
                                   bool use_mask)
{
    size_t len = (size_t)snprintf(p_buf, size, "[");

    for (uint8_t i = 0U; (i < PRESSURE_SENSOR_NUM) && (len < size); i++)
    {
        const char *p_sep = (0U == i) ? "" : ",";

        if ((use_mask && (0U == (mask & (1UL << i)))) || !isfinite(p_val[i]))
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "%snull", p_sep);
        }
        else
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "%s%.2f", p_sep, p_val[i]);
        }
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len, "]");
    }

    return (len < size) ? len : (size - 1U);
}

static void devctl_reply(char *p_reply, size_t size, bool ok, const char *p_msg)
{
    if ((NULL != p_reply) && (0U != size))
    {
        (void)snprintf(p_reply, size, "{\"ok\":%s,\"msg\":\"%s\"}", ok ? "true" : "false", p_msg);
    }
}

/**
 * @brief  Read "@cmd {json}" lines from UART0, run them, answer "@J {reply}" then "@J {state}".
 */
static void devctl_serial_task(void *p_para)
{
    static char s_line[DEVCTL_LINE_MAX];
    char reply[160];
    size_t len = 0U;
    uint8_t ch;

    (void)p_para;

    while (true)
    {
        if (1 != uart_read_bytes(DEVCTL_UART, &ch, 1U, portMAX_DELAY))
        {
            continue;
        }

        if (('\n' != ch) && ('\r' != ch))
        {
            if (len < (sizeof(s_line) - 1U))
            {
                s_line[len++] = (char)ch;
            }
            continue;
        }

        s_line[len] = '\0';
        len = 0U;

        const size_t prefix = strlen(DEVCTL_SERIAL_PREFIX_CMD);
        if (0 != strncmp(s_line, DEVCTL_SERIAL_PREFIX_CMD, prefix))
        {
            continue;
        }

        (void)devctl_command(&s_line[prefix], reply, sizeof(reply));
        /* Serial keepalives have no reply: they must not consume a pending UI command. */
        cJSON *request = cJSON_Parse(&s_line[prefix]);
        const cJSON *command = cJSON_GetObjectItemCaseSensitive(request, "cmd");
        if (!cJSON_IsString(command) || (0 != strcmp(command->valuestring, "ping")))
        {
            printf(DEVCTL_SERIAL_PREFIX_JSON "%s\n", reply);
        }
        cJSON_Delete(request);
        devctl_serial_push_state();
    }
}

/* END OF FILE ************************************************************************************/

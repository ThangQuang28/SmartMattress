/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Mar-2026
 * @file            : zapp.c
 * @dissaption      : zapp module.
 * auther           : duonghd
 ***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Include ---------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/** Middleware */
#include "flash.h"
#include "pressure_array.h"
#include "oled_lvgl.h"
#include "devctl.h"
#include "webui.h"
#include "pctl.h"

/* Define ----------------------------------------------------------------------------------------*/

#define ZAPP_OLED_TEXT_SIZE         160U
#define ZAPP_OLED_ROWS              7U      /* 13 values in two columns, 8 px font on 64 px. */
#define ZAPP_OLED_RETRY_SCANS       5U      /* Try to open a missing OLED every 5 scans. */
#define ZAPP_OLED_PAGE_NUM          3U      /* Pressures, control status, web address, in turn. */

/* Private data types ----------------------------------------------------------------------------*/

/* How long each OLED page stays up. */
static const uint32_t g_oled_page_ms[ZAPP_OLED_PAGE_NUM] = {8000U, 4000U, 4000U};

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

/**
 * @brief      Format the 13 readings as two columns of "N VV.VV" for the OLED.
 *             16 chars per row fill the 128 px width (8 px font): the left column holds P1..P7
 *             with a 1-digit index (7 chars), the right column P8..P13 (8 chars).
 */
static void zapp_format_oled(char *p_buf, size_t size, const float *p_kpa, uint32_t ok_mask)
{
    char cell[2][12];
    size_t len = 0U;

    p_buf[0] = '\0';

    for (uint8_t row = 0U; row < ZAPP_OLED_ROWS; row++)
    {
        for (uint8_t col = 0U; col < 2U; col++)
        {
            uint8_t i = (uint8_t)(row + (col * ZAPP_OLED_ROWS));

            if (i < PRESSURE_SENSOR_NUM)
            {
                if (0U != (ok_mask & (1UL << i)))
                {
                    snprintf(cell[col], sizeof(cell[col]), (0U == col) ? "%u%6.2f" : "%2u%6.2f",
                             (unsigned int)(i + 1U), p_kpa[i]);
                }
                else
                {
                    snprintf(cell[col], sizeof(cell[col]), (0U == col) ? "%u   ERR" : "%2u   ERR",
                             (unsigned int)(i + 1U));
                }
            }
            else
            {
                snprintf(cell[col], sizeof(cell[col]), "OK%2u/%u",
                         (unsigned int)__builtin_popcount(ok_mask),
                         (unsigned int)PRESSURE_SENSOR_NUM);
            }
        }

        if (len < size)
        {
            len += (size_t)snprintf(&p_buf[len], size - len, "%-7.7s %-8.8s%s", cell[0], cell[1],
                                    ((row + 1U) < ZAPP_OLED_ROWS) ? "\n" : "");
        }
    }
}

/**
 * @brief      Open zapp.
 * @param[in]  type: type to be open.
 * @retval     None.
 */
static void zapp_task(void *p_para)
{
    static const char *TAG = "ZAPP";
    float raw[PRESSURE_SENSOR_NUM];
    float kpa[PRESSURE_SENSOR_NUM];
    char line[PRESSURE_SENSOR_NUM * 7U + 1U];
    uint32_t ok_mask = 0U;
    char text[ZAPP_OLED_TEXT_SIZE];
    bool oled_ok;
    bool sensors_ready = false;
    uint32_t scan_count = 0U;
    TickType_t last_wake;
    uint8_t oled_page = 0U;
    TickType_t oled_page_since = 0;

    (void)p_para;

    /* NVS is already initialised in app_main(); this only marks the flash module as open. */
    (void)flash_open();
    (void)devctl_open();

    /* Web app on the device WiFi, and the same JSON over USB serial ("@cmd {...}" / "@J {...}"). */
    if (ZERR_OK != webui_open())
    {
        ESP_LOGE(TAG, "WiFi / web server not started");
    }
    if (ZERR_OK != devctl_serial_open())
    {
        ESP_LOGE(TAG, "serial command link not started");
    }

    oled_ok = (ZERR_OK == oled_lvgl_open());
    if (!oled_ok)
    {
        ESP_LOGW(TAG, "OLED not available, continuing without display");
    }

    last_wake = xTaskGetTickCount();

    while (true)
    {
        /* Keep state reporting and the manual-output watchdog alive even without sensors. */
        if (!sensors_ready)
        {
            sensors_ready = (ZERR_OK == pressure_array_open());
        }
        ok_mask = 0U;
        for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
        {
            raw[i] = NAN;
        }
        zerr_t ret = sensors_ready ? pressure_array_read_all(raw, &ok_mask) : ZERR_NOT_OPEN;
        if (0U == ok_mask)
        {
            sensors_ready = false; /* Retry initialization after a bus disconnection. */
        }
        size_t len = 0U;

        devctl_update(raw, ok_mask, kpa);
        pctl_step(kpa, ok_mask);

        /* One log line per scan: P01..P13 in kPa after zero offsets, "ERR" when not read. */
        for (uint8_t i = 0U; (i < PRESSURE_SENSOR_NUM) && (len < sizeof(line)); i++)
        {
            len += (0U != (ok_mask & (1UL << i)))
                       ? (size_t)snprintf(&line[len], sizeof(line) - len, " %6.2f", kpa[i])
                       : (size_t)snprintf(&line[len], sizeof(line) - len, "    ERR");
        }
        ESP_LOGI(TAG, "kPa%s", line);
        devctl_serial_push_state();
        devctl_tick();

        scan_count++;
        if ((!oled_ok) && (0U == (scan_count % ZAPP_OLED_RETRY_SCANS)))
        {
            oled_ok = (ZERR_OK == oled_lvgl_open());
            if (oled_ok)
            {
                ESP_LOGI(TAG, "OLED connected");
            }
        }

        if (oled_ok)
        {
            /* Time since the page came up, not the clock: a slow scan still shows every page. */
            if ((xTaskGetTickCount() - oled_page_since) >= pdMS_TO_TICKS(g_oled_page_ms[oled_page]))
            {
                oled_page = (uint8_t)((oled_page + 1U) % ZAPP_OLED_PAGE_NUM);
                oled_page_since = xTaskGetTickCount();
            }
            if (pctl_wait_on_bed())
            {
                oled_page = 1U;     /* Keep the "lie down, then confirm" note on screen. */
            }

            if (1U == oled_page)
            {
                (void)pctl_oled_text(text, sizeof(text));
            }
            else if (2U == oled_page)
            {
                (void)webui_net_text(text, sizeof(text));
            }
            else
            {
                zapp_format_oled(text, sizeof(text), kpa, ok_mask);
            }
            (void)oled_lvgl_set_text_small(text);
        }

        if (ZERR_OK != ret)
        {
            ESP_LOGW(TAG, "scan incomplete, ok mask = 0x%04lX", (unsigned long)ok_mask);
        }

        /* Period can be changed from the web page; restart the grid when it does. */
        TickType_t period = pdMS_TO_TICKS(devctl_period_ms());
        if (pctl_is_busy() && (period > pdMS_TO_TICKS(PCTL_STEP_PERIOD_MS)))
        {
            period = pdMS_TO_TICKS(PCTL_STEP_PERIOD_MS);   /* React fast while a valve is open. */
        }
        if ((xTaskGetTickCount() - last_wake) >= period)
        {
            last_wake = xTaskGetTickCount();
        }
        vTaskDelayUntil(&last_wake, period);
    }

    vTaskDelete(NULL);
}

/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief      Open task.
 * @param[in]  type: type to be configured.
 * @retval     None.
 */
void zapp_open(void)
{
    xTaskCreate(
        zapp_task,             /* Function that implements the task. */
        "zapp task",           /* Text name for the task. */
        ZAPP_STACK_DEEP_START, /* Stack deep in words, NOT bytes. */
        NULL,                  /* Parameter passed into the task. */
        ZAPP_PRIO_START,       /* Priority at which the task is created. */
        NULL                   /* Used to pass out the created task's handle.*/
    );
}

/* Private function prototypes -------------------------------------------------------------------*/

/* END OF FILE ************************************************************************************/

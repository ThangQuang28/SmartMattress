/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : pressure_array.c
 * @description     : 13 AGR12 pressure sensors behind two TCA9548A multiplexers.
 *                    Every AGR12 answers at 0x50, so exactly one mux channel may be open at a time.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/
#include "zcfg.h"
#include "pressure_array.h"
#include "tca9548.h"
#include "agr12.h"
#include "iic_global_bsp.h"
#include "driver/i2c.h"
#include <stdio.h>

/* Definitions -----------------------------------------------------------------------------------*/
#define PRESSURE_MUX_NUM            2U
#define PRESSURE_NO_CHANNEL         0xFFU
#define PRESSURE_LOCK_TIMEOUT_MS    1000U

/* Private data types ----------------------------------------------------------------------------*/
typedef struct
{
    uint8_t mux;        /* Index into gh_pressure.mux. */
    uint8_t channel;    /* 0..7 */
} pressure_map_t;

typedef struct
{
    bool is_open;
    SemaphoreHandle_t lock;
    tca9548_t mux[PRESSURE_MUX_NUM];
    agr12_t sensor;             /* Same address for all sensors, so one context is enough. */
    uint8_t active_mux;         /* Mux whose channel is currently open, or PRESSURE_NO_CHANNEL. */
    uint8_t active_channel;
} pressure_array_t;

/* Private variables -----------------------------------------------------------------------------*/

/* Sensor index -> (mux, channel). Sensors 1-8 on TCA one, 9-13 on TCA two.
   Change this table to match the board wiring.
   Boot scan 22-Oct-2026: TCA two has 0x50 only on ch4-ch7, so P09-P12 = ch4-ch7.
   No sensor answered on TCA two ch0-ch3; P13 stays on ch0 until a 13th sensor is wired.
   30-Oct-2026 (user): sensors 4 and 7 read too noisy, so the air lines were swapped: chamber 4 now
   goes to sensor 12 and chamber 12 to sensor 4, chamber 7 to sensor 13 and chamber 13 to sensor 7.
   The table keeps chamber order (P01..P13, valves unchanged); only those four sensors trade places.
   Was: P04 {0,3}, P07 {0,6}, P12 {1,7}, P13 {1,3}. */
static const pressure_map_t g_pressure_map[PRESSURE_SENSOR_NUM] =
{
    {0, 0}, {0, 1}, {0, 2},
    {1, 7},     /* P04: sensor 12 (air line swapped). */
    {0, 4}, {0, 5},
    {1, 3},     /* P07: sensor 13 (air line swapped). */
    {0, 7},
    {1, 4}, {1, 5}, {1, 6},
    {0, 3},     /* P12: sensor 4 (air line swapped). */
    {0, 6},     /* P13: sensor 7 (air line swapped). */
};

static pressure_array_t gh_pressure = {0};

static const char *TAG = "PRESSURE";

/* Private function prototypes -------------------------------------------------------------------*/
static zerr_t pressure_select(uint8_t index);
static void pressure_recover(void);
static void pressure_scan_list(const char *p_label);
static void pressure_scan_channels(void);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t pressure_array_open(void)
{
    zerr_t ret;

    if (gh_pressure.is_open)
    {
        return ZERR_OK;
    }

    gh_pressure.lock = xSemaphoreCreateMutex();
    if (NULL == gh_pressure.lock)
    {
        return ZERR_MEM_OUT;
    }

    ret = tca9548_open(&gh_pressure.mux[0], TCA9548_I2C_ADDR_ONE, TCA_ONE);
    if (ZERR_OK == ret)
    {
        ret = tca9548_open(&gh_pressure.mux[1], TCA9548_I2C_ADDR_TWO, TCA_TWO);
    }
    if (ZERR_OK == ret)
    {
        ret = agr12_open(&gh_pressure.sensor);
    }
    if (ZERR_OK != ret)
    {
        ESP_LOGE(TAG, "open failed: %d", (int)ret);
        vSemaphoreDelete(gh_pressure.lock);
        gh_pressure.lock = NULL;
        return ret;
    }

    gh_pressure.active_mux = PRESSURE_NO_CHANNEL;
    gh_pressure.active_channel = PRESSURE_NO_CHANNEL;
    gh_pressure.is_open = true;

    ESP_LOGI(TAG, "open: %u sensors on TCA 0x%02X and 0x%02X",
             (unsigned int)PRESSURE_SENSOR_NUM, TCA9548_I2C_ADDR_ONE, TCA9548_I2C_ADDR_TWO);

    pressure_scan_channels();

    return ZERR_OK;
}

zerr_t pressure_array_read_all(float *p_kpa, uint32_t *p_ok_mask)
{
    uint32_t triggered = 0U;
    uint32_t ok_mask = 0U;
    agr12_data_t data;

    if ((NULL == p_kpa) || (!gh_pressure.is_open))
    {
        return ZERR_INVALID_PARAM;
    }

    if (pdTRUE != xSemaphoreTake(gh_pressure.lock, pdMS_TO_TICKS(PRESSURE_LOCK_TIMEOUT_MS)))
    {
        return ZERR_TIME_OUT;
    }

    /* 1. Start every sensor. A measurement keeps running after its channel is closed. */
    for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
    {
        p_kpa[i] = NAN;

        if ((ZERR_OK == pressure_select(i)) && (ZERR_OK == agr12_trigger(&gh_pressure.sensor)))
        {
            triggered |= (1UL << i);
        }
        else
        {
            pressure_recover();
        }
    }

    /* 2. One wait covers all of them: the last trigger was the most recent. */
    vTaskDelay(pdMS_TO_TICKS(AGR12_MEASURE_DELAY_MS));

    /* 3. Collect results. */
    for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
    {
        if (0U == (triggered & (1UL << i)))
        {
            continue;
        }

        if ((ZERR_OK == pressure_select(i)) && (ZERR_OK == agr12_fetch(&gh_pressure.sensor, &data)))
        {
            p_kpa[i] = data.pressure_kpa;
            ok_mask |= (1UL << i);
        }
        else
        {
            pressure_recover();
        }
    }

    /* 4. One more try for sensors that failed: a single bad transfer (noise when the pump or a
          valve switches) should not show as ERR or stop the automatic mode for that chamber. */
    uint32_t retry = ((1UL << PRESSURE_SENSOR_NUM) - 1U) & ~ok_mask;
    if (0U != retry)
    {
        uint32_t again = 0U;

        for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
        {
            if ((0U != (retry & (1UL << i))) && (ZERR_OK == pressure_select(i)) &&
                (ZERR_OK == agr12_trigger(&gh_pressure.sensor)))
            {
                again |= (1UL << i);
            }
            else if (0U != (retry & (1UL << i)))
            {
                pressure_recover();
            }
        }
        if (0U != again)
        {
            vTaskDelay(pdMS_TO_TICKS(AGR12_MEASURE_DELAY_MS));
        }
        for (uint8_t i = 0U; i < PRESSURE_SENSOR_NUM; i++)
        {
            if (0U == (again & (1UL << i)))
            {
                continue;
            }
            if ((ZERR_OK == pressure_select(i)) &&
                (ZERR_OK == agr12_fetch(&gh_pressure.sensor, &data)))
            {
                p_kpa[i] = data.pressure_kpa;
                ok_mask |= (1UL << i);
            }
            else
            {
                pressure_recover();
            }
        }
    }

    /* Leave the bus with no sensor attached, so other devices (OLED) never see 0x50 traffic. */
    (void)pressure_select(PRESSURE_SENSOR_NUM);

    xSemaphoreGive(gh_pressure.lock);

    if (NULL != p_ok_mask)
    {
        *p_ok_mask = ok_mask;
    }

    return (ok_mask == ((1UL << PRESSURE_SENSOR_NUM) - 1U)) ? ZERR_OK : ZERR_ERROR;
}

zerr_t pressure_array_read(uint8_t index, float *p_kpa)
{
    zerr_t ret;
    agr12_data_t data;

    if ((NULL == p_kpa) || (index >= PRESSURE_SENSOR_NUM) || (!gh_pressure.is_open))
    {
        return ZERR_INVALID_PARAM;
    }

    if (pdTRUE != xSemaphoreTake(gh_pressure.lock, pdMS_TO_TICKS(PRESSURE_LOCK_TIMEOUT_MS)))
    {
        return ZERR_TIME_OUT;
    }

    ret = pressure_select(index);
    if (ZERR_OK == ret)
    {
        ret = agr12_read(&gh_pressure.sensor, &data);
    }
    if (ZERR_OK == ret)
    {
        *p_kpa = data.pressure_kpa;
    }
    else
    {
        pressure_recover();
    }

    (void)pressure_select(PRESSURE_SENSOR_NUM);

    xSemaphoreGive(gh_pressure.lock);

    return ret;
}

/* Private functions -----------------------------------------------------------------------------*/

/**
 * @brief Open the channel of one sensor and make sure the other mux has every channel closed.
 *        index == PRESSURE_SENSOR_NUM closes everything. Skips I2C writes that change nothing.
 */
static zerr_t pressure_select(uint8_t index)
{
    zerr_t ret = ZERR_OK;
    uint8_t mux = PRESSURE_NO_CHANNEL;
    uint8_t channel = PRESSURE_NO_CHANNEL;

    if (index < PRESSURE_SENSOR_NUM)
    {
        mux = g_pressure_map[index].mux;
        channel = g_pressure_map[index].channel;
    }

    if ((mux == gh_pressure.active_mux) && (channel == gh_pressure.active_channel))
    {
        return ZERR_OK;
    }

    /* Close the previous mux first when moving to the other one (or to none). */
    if ((PRESSURE_NO_CHANNEL != gh_pressure.active_mux) && (mux != gh_pressure.active_mux))
    {
        ret = tca9548_disable_all(&gh_pressure.mux[gh_pressure.active_mux]);
        if (ZERR_OK != ret)
        {
            return ret;
        }
        gh_pressure.active_mux = PRESSURE_NO_CHANNEL;
        gh_pressure.active_channel = PRESSURE_NO_CHANNEL;
    }

    if (PRESSURE_NO_CHANNEL != mux)
    {
        ret = tca9548_select(&gh_pressure.mux[mux], channel);
        if (ZERR_OK != ret)
        {
            return ret;
        }
    }

    gh_pressure.active_mux = mux;
    gh_pressure.active_channel = channel;

    return ZERR_OK;
}

/**
 * @brief After an I2C error: hardware-reset both muxes (all channels closed) and forget the cache.
 */
static void pressure_recover(void)
{
    for (uint8_t m = 0U; m < PRESSURE_MUX_NUM; m++)
    {
        (void)tca958_reset(&gh_pressure.mux[m]);
    }

    gh_pressure.active_mux = PRESSURE_NO_CHANNEL;
    gh_pressure.active_channel = PRESSURE_NO_CHANNEL;
}

/**
 * @brief Probe 0x08..0x77 on the bus as it is now and log every address that ACKs.
 */
static void pressure_scan_list(const char *p_label)
{
    char line[160];
    int len = snprintf(line, sizeof(line), "%s:", p_label);

    for (uint8_t addr = 0x08U; addr <= 0x77U; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (uint8_t)((addr << 1) | I2C_MASTER_WRITE), true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(global_i2c_bsp_get_port(), cmd, pdMS_TO_TICKS(20));
        i2c_cmd_link_delete(cmd);

        if ((ESP_OK == err) && (len < (int)sizeof(line) - 6))
        {
            len += snprintf(&line[len], sizeof(line) - (size_t)len, " 0x%02X", addr);
        }
    }

    ESP_LOGI(TAG, "%s", line);
}

/**
 * @brief Boot diagnostic: list the devices behind every channel of both muxes.
 */
static void pressure_scan_channels(void)
{
    char label[40];

    for (uint8_t m = 0U; m < PRESSURE_MUX_NUM; m++)
    {
        (void)tca9548_disable_all(&gh_pressure.mux[m]);
    }
    pressure_scan_list("scan bus (all closed)");

    for (uint8_t m = 0U; m < PRESSURE_MUX_NUM; m++)
    {
        (void)tca9548_disable_all(&gh_pressure.mux[(m + 1U) % PRESSURE_MUX_NUM]);

        for (uint8_t ch = 0U; ch < TCA9548_CHANNEL_NUM; ch++)
        {
            if (ZERR_OK != tca9548_select(&gh_pressure.mux[m], ch))
            {
                ESP_LOGW(TAG, "scan TCA 0x%02X ch%u: select failed",
                         gh_pressure.mux[m].addr, (unsigned int)ch);
                continue;
            }
            (void)snprintf(label, sizeof(label), "scan TCA 0x%02X ch%u",
                           gh_pressure.mux[m].addr, (unsigned int)ch);
            pressure_scan_list(label);
        }
    }

    for (uint8_t m = 0U; m < PRESSURE_MUX_NUM; m++)
    {
        (void)tca9548_disable_all(&gh_pressure.mux[m]);
    }
    gh_pressure.active_mux = PRESSURE_NO_CHANNEL;
    gh_pressure.active_channel = PRESSURE_NO_CHANNEL;
}

/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : aht20.c                                                                           
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 character
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/* Middleware */
#include "aht20.h"

/* BSP */
#include "iic_global_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

#define BSP_AHT20_CMD_INIT    0xBE
#define BSP_AHT20_CMD_MEASURE 0xAC
#define BSP_AHT20_CMD_RESET   0xBA

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "AHT20";

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void aht20_open(void)
{
    uint8_t params[2] = {0x08, 0x00};

    global_aht20_bsp_write_cmd(BSP_AHT20_CMD_INIT, params, 2);

    vTaskDelay(pdMS_TO_TICKS(50));
}

void aht20_measure(void)
{
    uint8_t params[2] = {0x33, 0x00};

    global_aht20_bsp_write_cmd(BSP_AHT20_CMD_MEASURE, params, 2);

    vTaskDelay(pdMS_TO_TICKS(120));
}

aht20_data_t aht20_read(void)
{
    uint8_t data[6];
    aht20_data_t s_data = {0};

    if (global_aht20_bsp_read_data(data, 6) != ESP_OK)
    {
        ESP_LOGE(TAG, "AHT20 read error");
        return s_data;
    }

    if (data[0] & 0x80)
    {
        ESP_LOGW(TAG, "AHT20 busy");
        return s_data;
    }

    uint32_t raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        ((data[3] & 0xF0) >> 4);

    uint32_t raw_temp =
        ((uint32_t)(data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];

    ESP_LOGI(TAG,"AHT20 RAW: %02X %02X %02X %02X %02X %02X",
         data[0], data[1], data[2], data[3], data[4], data[5]);

    s_data.hum = ((float)raw_humidity * 100.0f) / 1048576.0f;
    s_data.temp = ((float)raw_temp * 200.0f) / 1048576.0f - 50.0f;

    if (s_data.hum > 100) s_data.hum = 100;

    return s_data;
}

void aht20_reset(void)
{
    global_aht20_bsp_write_cmd(BSP_AHT20_CMD_RESET, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(20));
}

/* END OF FILE ************************************************************************************/
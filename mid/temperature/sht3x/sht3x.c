/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : sht3x.c                                                                           
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
#include "sht3x.h"

/* BSP. */
#include "iic_global_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

#define SHT3X_CMD_MEASURE_H   0x2400
#define SHT3X_CMD_SOFT_RESET  0x30A2

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "SHT3X";

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void sht3x_open(void)
{
    global_sht3x_bsp_write(SHT3X_CMD_SOFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));
}

void sht3x_measure(void)
{
    global_sht3x_bsp_write(SHT3X_CMD_MEASURE_H);
    vTaskDelay(pdMS_TO_TICKS(20));
}

sht3x_data_t sht3x_read(void)
{
    uint8_t data[6];
    sht3x_data_t result = {0};

    if (global_sht3x_bsp_read(data, 6) != ESP_OK)
    {
        ESP_LOGE(TAG, "Read failed");
        return result;
    }

    uint16_t raw_temp = (data[0] << 8) | data[1];
    uint16_t raw_hum  = (data[3] << 8) | data[4];

    result.temp = -45 + 175 * ((float)raw_temp / 65535.0f);
    result.hum  = 100 * ((float)raw_hum / 65535.0f);
    
    return result;
}

void sht3x_reset(void)
{
    global_sht3x_bsp_write(SHT3X_CMD_SOFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));
}

/* END OF FILE ************************************************************************************/
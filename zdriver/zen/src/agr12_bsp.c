
/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : agr12_bsp.c
 * @description     : AGR12 I2C pressure sensor hardware implementation.
***************************************************************************************************************/

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"
#include "agr12_bsp.h"
#include "iic_global_bsp.h"

#include "driver/i2c.h"
#include "esp_err.h"

/* Private Functions -----------------------------------------------------------------------------*/

zerr_t agr12_bsp_open(void)
{
    zerr_t ret = ZERR_OK;
    ret = global_i2c_bsp_open();
    return ret;    
}
/**
 * @brief Read AGR12 measurement data and verify CRC.
 */
zerr_t agr12_bsp_read(uint8_t address, uint32_t timeout_ms, agr12_bsp_data_t *p_data)
{
    esp_err_t esp_ret;
    uint8_t cmd[] = { AGR12_BSP_CMD_MEASURE, AGR12_BSP_CMD_PRESSURE };
    uint8_t data[AGR12_BSP_DATA_SIZE] = {0};

    if (NULL == p_data)
    {
        return ZERR_INVALID_PARAM;
    }

    esp_ret = i2c_master_write_to_device(
        BSP_I2C_PORT,
        address,
        cmd,
        sizeof(cmd),
        pdMS_TO_TICKS(timeout_ms));

    if (ESP_OK != esp_ret)
    {
        return ZERR_ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(AGR12_BSP_MEASURE_DELAY_MS));

    esp_ret = i2c_master_read_from_device(
        BSP_I2C_PORT,
        address,
        data,
        sizeof(data),
        pdMS_TO_TICKS(timeout_ms));

    if (ESP_OK != esp_ret)
    {
        return ZERR_ERROR;
    }

    uint8_t crc_calculated = (uint8_t)(data[0] ^ data[1]);


    if (data[2] != crc_calculated)
    {
        return ZERR_INVALID_PARAM;
    }

    p_data->pressure_raw = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
    p_data->crc = data[2];

    return ZERR_OK;
}

/* END OF FILE ************************************************************************************/

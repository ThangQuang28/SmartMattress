
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
 * @brief Send the measure command (0xAC 0x12). Data is ready AGR12_BSP_MEASURE_DELAY_MS later.
 */
zerr_t agr12_bsp_trigger(uint8_t address, uint32_t timeout_ms)
{
    uint8_t cmd[] = { AGR12_BSP_CMD_MEASURE, AGR12_BSP_CMD_PRESSURE };

    esp_err_t esp_ret = i2c_master_write_to_device(
        BSP_I2C_PORT,
        address,
        cmd,
        sizeof(cmd),
        pdMS_TO_TICKS(timeout_ms));

    return (ESP_OK == esp_ret) ? ZERR_OK : ZERR_TX;
}

/**
 * @brief Read 3 bytes (DATA0, DATA1, CRC) of a finished measurement and verify CRC.
 */
zerr_t agr12_bsp_fetch(uint8_t address, uint32_t timeout_ms, agr12_bsp_data_t *p_data)
{
    uint8_t data[AGR12_BSP_DATA_SIZE] = {0};

    if (NULL == p_data)
    {
        return ZERR_INVALID_PARAM;
    }

    esp_err_t esp_ret = i2c_master_read_from_device(
        BSP_I2C_PORT,
        address,
        data,
        sizeof(data),
        pdMS_TO_TICKS(timeout_ms));
    if (ESP_OK != esp_ret)
    {
        return ZERR_RX;
    }

    /* CRC = DATA0 XOR DATA1 (datasheet table 7). */
    if (data[2] != (uint8_t)(data[0] ^ data[1]))
    {
        return ZERR_ERROR;
    }

    p_data->pressure_raw = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
    p_data->crc = data[2];

    return ZERR_OK;
}

/**
 * @brief Trigger, wait and read one measurement (blocking).
 */
zerr_t agr12_bsp_read(uint8_t address, uint32_t timeout_ms, agr12_bsp_data_t *p_data)
{
    if (NULL == p_data)
    {
        return ZERR_INVALID_PARAM;
    }

    zerr_t ret = agr12_bsp_trigger(address, timeout_ms);
    if (ZERR_OK != ret)
    {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(AGR12_BSP_MEASURE_DELAY_MS));

    return agr12_bsp_fetch(address, timeout_ms, p_data);
}

/* END OF FILE ************************************************************************************/

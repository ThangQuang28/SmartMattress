/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : tca9548_bsp.c
 * @description     : TCA9548A I2C multiplexer implementation.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"

/* BSP. */
#include "iic_global_bsp.h"
#include "tca9548_bsp.h"

#include "driver/gpio.h"
#include "esp_err.h"

/* Definitions -----------------------------------------------------------------------------------*/

#define TCA9548_BSP_TIMEOUT_MS       100U
#define TCA9548_BSP_SCAN_TIMEOUT_MS   50U
#define TCA9548_BSP_RESET_DELAY_MS    10U

#define TCA9548_BSP_ONE_RESET_PIN  GPIO_NUM_17
#define TCA9548_BSP_TWO_RESET_PIN  GPIO_NUM_16

/* Private Functions -----------------------------------------------------------------------------*/

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t tca9548_bsp_open(void)
{
    zerr_t ret = ZERR_OK;
    esp_err_t esp_ret = ESP_OK;
    /* I2C configuration */
    ret = global_i2c_bsp_open();

    if(ZERR_OK == ret)
    {
        /* Latch RESET high before the pins become outputs, so configuring them never resets a mux. */
        gpio_set_level(TCA9548_BSP_ONE_RESET_PIN, 1);
        gpio_set_level(TCA9548_BSP_TWO_RESET_PIN, 1);

        /* Reset Pin config */
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << TCA9548_BSP_ONE_RESET_PIN) | (1ULL << TCA9548_BSP_TWO_RESET_PIN),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };

        esp_ret = gpio_config(&config);
        if(esp_ret != ESP_OK)
        {
            
            ret = ZERR_NOT_OPEN;
        }
    }
    if (ZERR_OK == ret)
    {
        gpio_set_level(TCA9548_BSP_ONE_RESET_PIN, 1);
        gpio_set_level(TCA9548_BSP_TWO_RESET_PIN, 1);
    }
    return ret;
}
zerr_t tca9548_bsp_write(uint8_t addr, uint8_t channel_mask)
{
    esp_err_t ret;

    ret = i2c_master_write_to_device(
        BSP_I2C_PORT,
        addr,
        &channel_mask,
        sizeof(channel_mask),
        pdMS_TO_TICKS(TCA9548_BSP_TIMEOUT_MS));

    if (ESP_OK != ret)
    {
        return ZERR_ERROR;
    }

    return ZERR_OK;
}
zerr_t tca9548_bsp_read(uint8_t addr, uint8_t *p_channel_mask)
{
    esp_err_t ret;

    if (NULL == p_channel_mask)
    {
        return ZERR_INVALID_PARAM;
    }

    ret = i2c_master_read_from_device(
        BSP_I2C_PORT,
        addr,
        p_channel_mask,
        sizeof(*p_channel_mask),
        pdMS_TO_TICKS(TCA9548_BSP_TIMEOUT_MS));

    if (ESP_OK != ret)
    {
        return ZERR_ERROR;
    }

    return ZERR_OK;
}
zerr_t tca9548_bsp_scan_addr_channel(void)
{
    esp_err_t esp_ret;
    i2c_cmd_handle_t p_cmd;
    uint8_t found = 0U;

    for (uint8_t addr = 0x08U; addr <= 0x77U; addr++)
    {
        p_cmd = i2c_cmd_link_create();

        if (NULL == p_cmd)
        {
            return ZERR_ERROR;
        }

        esp_ret = i2c_master_start(p_cmd);

        if (ESP_OK == esp_ret)
        {
            esp_ret = i2c_master_write_byte(
                p_cmd,
                (uint8_t)((addr << 1U) | I2C_MASTER_WRITE),
                true);
        }

        if (ESP_OK == esp_ret)
        {
            esp_ret = i2c_master_stop(p_cmd);
        }

        if (ESP_OK == esp_ret)
        {
            esp_ret = i2c_master_cmd_begin(
                BSP_I2C_PORT,
                p_cmd,
                pdMS_TO_TICKS(TCA9548_BSP_SCAN_TIMEOUT_MS));
        }

        i2c_cmd_link_delete(p_cmd);

        if (ESP_OK == esp_ret)
        {
            found++;
        }
    }

    return ZERR_OK;
}
zerr_t tca9548_bsp_reset(tca9548_num_t num)
{
    zerr_t ret = ZERR_OK;
    gpio_num_t reset_pin;

    switch (num)
    {
        case TCA_ONE:
            reset_pin = TCA9548_BSP_ONE_RESET_PIN;
            break;

        case TCA_TWO:
            reset_pin = TCA9548_BSP_TWO_RESET_PIN;
            break;

        default:
            return ZERR_INVALID_PARAM;
    }

    if (ESP_OK != gpio_set_level(reset_pin, 0))
    {
        return ZERR_ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(10U));

    if (ESP_OK != gpio_set_level(reset_pin, 1))
    {
        return ZERR_ERROR;
    }

    vTaskDelay(pdMS_TO_TICKS(10U));

    return ret;
}

/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Oct-2026
 * @file            : iic_global_bsp.c
 * @description     : I2C bus BSP implementation.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"
#include "iic_global_bsp.h"

/* Defines ---------------------------------------------------------------------------------------*/

/* Private variables -----------------------------------------------------------------------------*/

static bool g_i2c_initialized = false;

/* Private function prototypes -------------------------------------------------------------------*/

static zerr_t global_i2c_bsp_scan(void);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t global_i2c_bsp_open(void)
{
    if (g_i2c_initialized)
    {
        return ZERR_OK;
    }

    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA_PIN,
        .scl_io_num = BSP_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_FREQ,
    };

    esp_err_t err = i2c_param_config(BSP_I2C_PORT, &config);
    if (err != ESP_OK)
    {
        ZTRACE_DBG(TR_IIC_BSP, ("I2C config failed: %s", esp_err_to_name(err)));
        return ZERR_NOT_OPEN;
    }

    err = i2c_driver_install(BSP_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE))
    {
        ZTRACE_DBG(TR_IIC_BSP, ("I2C driver install failed: %s", esp_err_to_name(err)));
        return ZERR_NOT_OPEN;
    }

    g_i2c_initialized = true;

    /* Optional: scan the bus for debugging. */
    (void)global_i2c_bsp_scan();

    return ZERR_OK;
}

i2c_port_t global_i2c_bsp_get_port(void)
{
    return BSP_I2C_PORT;
}

/* Private functions -----------------------------------------------------------------------------*/

static zerr_t global_i2c_bsp_scan(void)
{
    uint8_t device_count = 0;

    ZTRACE_DBG(TR_IIC_BSP, ("I2C scan start"));

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        if (cmd == NULL)
        {
            return ZERR_ERROR;
        }

        esp_err_t err = i2c_master_start(cmd);

        if (err == ESP_OK)
        {
            err = i2c_master_write_byte(
                cmd,
                (uint8_t)((addr << 1) | I2C_MASTER_WRITE),
                true);
        }

        if (err == ESP_OK)
        {
            err = i2c_master_stop(cmd);
        }

        if (err == ESP_OK)
        {
            err = i2c_master_cmd_begin(
                BSP_I2C_PORT,
                cmd,
                pdMS_TO_TICKS(50));
        }

        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK)
        {
            ZTRACE_DBG(TR_IIC_BSP, ("I2C device found: 0x%02X", addr));
            device_count++;
        }
    }

    ZTRACE_DBG(TR_IIC_BSP, ("I2C scan done: %u device(s)", device_count));

    return (device_count > 0U) ? ZERR_OK : ZERR_EXIST;
}

/* END OF FILE ************************************************************************************/
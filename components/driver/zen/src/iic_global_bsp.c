/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : iic_global_bsp.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/* BSP. */
#include "iic_global_bsp.h"

/** ESP driver */
#include "driver/i2c.h"

/* Define ----------------------------------------------------------------------------------------*/
#define BSP_I2C_PORT           I2C_NUM_0
#define BSP_I2C_SCL_PIN        22
#define BSP_I2C_SDA_PIN        21
#define BSP_I2C_FREQ           100000

/* OLED SSD1306 */
#define BSP_OLED_ADDR          0x3C
#define BSP_OLED_WIDTH         128
#define BSP_OLED_HEIGHT        64

/* AHT20 */
#define BSP_AHT20_ADDR        0x38

/* BMP280 */
#define BSP_BMP280_ADDR        0x77 

/* SHT3x */
#define BSP_SHT3X_ADDR        0x44

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static esp_err_t global_bsp_i2c_check_addr(uint8_t addr);

/* Public function prototypes --------------------------------------------------------------------*/

void global_i2c_bsp_open(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA_PIN,
        .scl_io_num = BSP_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_FREQ
    };

    i2c_param_config(BSP_I2C_PORT, &conf);
    i2c_driver_install(BSP_I2C_PORT, conf.mode, 0, 0, 0);

    ESP_LOGI("", "\n[I2C] Scanning...\n");

    uint8_t found = 0;

    ESP_LOGI("", "[I2C] Devices: ");

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        if (ESP_OK == global_bsp_i2c_check_addr(addr))
        {
            ESP_LOGI("","0x%02X ", addr);
            found++;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (found == 0)
    {
        ESP_LOGI("", "[I2C] No device found!");
    }
    else
    {
        ESP_LOGI("", "[I2C] Total devices: %d", found);
    }

    ESP_LOGI("", "[I2C] Scan done\n");
}

/* OLED */
esp_err_t global_oled_bsp_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    return i2c_master_write_to_device(BSP_I2C_PORT, BSP_OLED_ADDR,
                                      data, 2, pdMS_TO_TICKS(100));
}

esp_err_t global_oled_bsp_write_data(uint8_t *data, size_t len)
{
    uint8_t buffer[len + 1];
    buffer[0] = 0x40;
    memcpy(&buffer[1], data, len);

    return i2c_master_write_to_device(BSP_I2C_PORT, BSP_OLED_ADDR,
                                      buffer, len + 1, pdMS_TO_TICKS(100));
}

/* AHT20 */
esp_err_t global_aht20_bsp_write_cmd(uint8_t cmd, uint8_t *params, size_t len)
{
    uint8_t buffer[len + 1];
    buffer[0] = cmd;

    if (params != NULL && len > 0)
    {
        memcpy(&buffer[1], params, len);
    }

    return i2c_master_write_to_device(
        BSP_I2C_PORT,
        BSP_AHT20_ADDR,
        buffer,
        len + 1,
        pdMS_TO_TICKS(100)
    );
}
esp_err_t global_aht20_bsp_read_data(uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(
        BSP_I2C_PORT,
        BSP_AHT20_ADDR,
        data,
        len,
        pdMS_TO_TICKS(100)
    );
}

/* BMP280 */
esp_err_t global_bmp280_bsp_write(uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t buffer[len + 1];
    buffer[0] = reg;
    memcpy(&buffer[1], data, len);

    return i2c_master_write_to_device(
        BSP_I2C_PORT,
        BSP_BMP280_ADDR,
        buffer,
        len + 1,
        pdMS_TO_TICKS(100)
    );
}

esp_err_t global_bmp280_bsp_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        BSP_I2C_PORT,
        BSP_BMP280_ADDR,
        &reg,
        1,
        data,
        len,
        pdMS_TO_TICKS(100)
    );
}

/* SHT3x */
esp_err_t global_sht3x_bsp_write(uint16_t cmd)
{
    uint8_t buffer[2];
    buffer[0] = cmd >> 8;
    buffer[1] = cmd & 0xFF;

    return i2c_master_write_to_device(
        BSP_I2C_PORT,
        BSP_SHT3X_ADDR,
        buffer,
        2,
        pdMS_TO_TICKS(100)
    );
}

esp_err_t global_sht3x_bsp_read(uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(
        BSP_I2C_PORT,
        BSP_SHT3X_ADDR,
        data,
        len,
        pdMS_TO_TICKS(100)
    );
}

/* Private function prototypes -------------------------------------------------------------------*/
static esp_err_t global_bsp_i2c_check_addr(uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(
        BSP_I2C_PORT,
        cmd,
        pdMS_TO_TICKS(50)
    );

    i2c_cmd_link_delete(cmd);

    return ret;
}

/* END OF FILE ************************************************************************************/
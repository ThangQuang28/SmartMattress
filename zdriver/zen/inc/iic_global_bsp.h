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

#ifndef _IIC_GLOBAL_BSP_H
#define _IIC_GLOBAL_BSP_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void global_i2c_bsp_open(void);

/* OLED */
esp_err_t global_oled_bsp_write_cmd(uint8_t cmd);
esp_err_t global_oled_bsp_write_data(uint8_t *data, size_t len);

/* AHT20 */
esp_err_t global_aht20_bsp_write_cmd(uint8_t cmd, uint8_t *params, size_t len);
esp_err_t global_aht20_bsp_read_data(uint8_t *data, size_t len);

/* BMP280 */
esp_err_t global_bmp280_bsp_write(uint8_t reg, uint8_t *data, size_t len);
esp_err_t global_bmp280_bsp_read(uint8_t reg, uint8_t *data, size_t len);

/* SHT3x */
esp_err_t global_sht3x_bsp_write(uint16_t cmd);
esp_err_t global_sht3x_bsp_read(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _IIC_GLOBAL_BSP_H */

/* END OF FILE ************************************************************************************/

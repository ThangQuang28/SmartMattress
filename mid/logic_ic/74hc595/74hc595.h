/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : 74hc595_bsp.h                                                                            
 * @dissaption      : 74HC595 driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef Z74HC595_H
#define Z74HC595_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "74hc595_bsp.h"

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
typedef struct
{
    h595_bsp_t bsp;
    uint8_t *p_data;
    uint8_t chip_num;
    bool is_open;
} z74hc595_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

zerr_t 
z74hc595_open(z74hc595_t *p_dev, gpio_num_t pin_data, gpio_num_t pin_clock, gpio_num_t pin_latch, uint8_t chip_num);

zerr_t 
z74hc595_close(z74hc595_t *p_dev);

zerr_t
z74hc595_set_pin(z74hc595_t *p_dev, uint16_t pin, bool level);

zerr_t 
z74hc595_get_pin(z74hc595_t *p_dev, uint16_t pin, bool *p_level);

zerr_t 
z74hc595_write_byte(z74hc595_t *p_dev, uint8_t chip, uint8_t value);

zerr_t 
z74hc595_write_bytes(z74hc595_t *p_dev, const uint8_t *p_data, uint8_t chip_num);

zerr_t 
z74hc595_write_all(z74hc595_t *p_dev, bool level);

zerr_t 
z74hc595_update(z74hc595_t *p_dev);

uint16_t 
z74hc595_get_pin_count(const z74hc595_t *p_dev);

#ifdef __cplusplus
}
#endif

#endif
/* END OF FILE ************************************************************************************/

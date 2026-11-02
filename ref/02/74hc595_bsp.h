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


#ifndef _74HC595_BSP_H_
#define _74HC595_BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include "zerr.h"
#include "driver/gpio.h"

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
typedef struct
{
    gpio_num_t pin_data;
    gpio_num_t pin_clock;
    gpio_num_t pin_latch;
    bool is_open;
} h595_bsp_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

zerr_t 
h595_bsp_open(h595_bsp_t *p_bsp, gpio_num_t pin_data, gpio_num_t pin_clock, gpio_num_t pin_latch);

zerr_t 
h595_bsp_close(h595_bsp_t *p_bsp);

zerr_t 
h595_bsp_shift_out(h595_bsp_t *p_bsp, uint8_t data);

zerr_t 
h595_bsp_latch(h595_bsp_t *p_bsp);

#ifdef __cplusplus
}
#endif

#endif
/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : btn_bsp.h                                                                            
 * @dissaption      : Button driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _BTN_BSP_H
#define _BTN_BSP_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/

#define BTN_BSP_ID_MAX 2

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef enum
{
    BTN_ID_BOOT = 0,
    BTN_ID_ON,
    BTN_ID_NEXT_VIEW
} btn_bsp_id_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void btn_bsp_open(void);
gpio_num_t btn_bsp_get_gpio(btn_bsp_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* _BTN_BSP_H */

/* END OF FILE ************************************************************************************/

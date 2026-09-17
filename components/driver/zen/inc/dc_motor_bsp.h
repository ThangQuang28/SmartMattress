/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : dc_motor_bsp.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _DC_MOTOR_BSP_H
#define _DC_MOTOR_BSP_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

/* Middleware */
#include "dc_motor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void dc_motor_bsp_open(dc_motor_type_t type);

void dc_motor_bsp_forward(dc_motor_type_t type, uint8_t speed);

void dc_motor_bsp_backward(dc_motor_type_t type, uint8_t speed);

void dc_motor_bsp_stop(dc_motor_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* _DC_MOTOR_BSP_H */

/* END OF FILE ************************************************************************************/

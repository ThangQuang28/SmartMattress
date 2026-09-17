
/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : dc_motor.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _DC_MOTOR_H
#define _DC_MOTOR_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/* Middleware */
#include "package.h"

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef enum
{
    DC_MOTOR_TYPE_1 = 0u,
    DC_MOTOR_TYPE_2,
} dc_motor_type_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void dc_motor_open(void);

void dc_motor_forward(dc_motor_type_t type, uint8_t speed);

void dc_motor_backward(dc_motor_type_t type, uint8_t speed);

void dc_motor_stop(dc_motor_type_t type);

pkg_err_t dc_motor_handle(pkg_t *pkg);

#ifdef __cplusplus
}
#endif

#endif /* _DC_MOTOR_BSP_H */

/* END OF FILE ************************************************************************************/

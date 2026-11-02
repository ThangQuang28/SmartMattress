/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : flash_bsp.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _IIC_GLOBAL_BSP_H
#define _IIC_GLOBAL_BSP_H

/* Public macros ---------------------------------------------------------------------------------*/

#define BSP_I2C_PORT                I2C_NUM_0
#define BSP_I2C_SDA_PIN             21
#define BSP_I2C_SCL_PIN             22
/* AGR12 datasheet section 7: I2C clock max 30 kHz. SCL passes through the TCA9548A to every
   sensor, so the whole bus must stay below that limit (25 kHz keeps a margin). */
#define BSP_I2C_FREQ                25000

/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"
#include "zerr.h"

#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

zerr_t global_i2c_bsp_open(void);
i2c_port_t global_i2c_bsp_get_port(void);

#ifdef __cplusplus
}
#endif

#endif /* _IIC_GLOBAL_BSP_H */

/* END OF FILE ************************************************************************************/

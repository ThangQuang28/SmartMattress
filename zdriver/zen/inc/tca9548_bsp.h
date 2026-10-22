/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : tca9548_bsp.h
 * @description     : TCA9548A I2C multiplexer API.
***************************************************************************************************************/

#ifndef __TCA9548_BSP_H__
#define __TCA9548_BSP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#include "zerr.h"

#include "tca9548.h"

/* Definitions -----------------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

zerr_t tca9548_bsp_open(void);
zerr_t tca9548_bsp_write(uint8_t addr, uint8_t channel_mask);
zerr_t tca9548_bsp_read(uint8_t addr, uint8_t *p_channel_mask);
zerr_t tca9548_bsp_scan_addr_channel(void);
zerr_t tca9548_bsp_reset(tca9548_num_t num);

/* Public function prototypes --------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __TCA9548_BSP_H__ */

/* END OF FILE ************************************************************************************/

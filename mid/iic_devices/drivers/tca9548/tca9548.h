/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : tca9548.h
 * @description     : TCA9548A I2C multiplexer API.
***************************************************************************************************************/

#ifndef __TCA9548_H__
#define __TCA9548_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/

#include "esp_err.h"
#include "driver/i2c.h"
#include <stdint.h>
#include <stdbool.h>

#include "zerr.h"

/* Definitions -----------------------------------------------------------------------------------*/

#define TCA9548_I2C_ADDR_DEFAULT     0x70U
/* Address = 0x70 | A2A1A0 (TCA9548A datasheet table 7-1). Both muxes sit on one bus, so their
   address pins must differ. Assumed wiring: TCA one A2..A0 = 000, TCA two = 001. */
#define TCA9548_I2C_ADDR_ONE         0x70U
#define TCA9548_I2C_ADDR_TWO         0x71U
#define TCA9548_CHANNEL_NUM          8U

/* Public data types -----------------------------------------------------------------------------*/

typedef enum {
    TCA_ONE=0u,
    TCA_TWO
} tca9548_num_t;

typedef struct
{
    tca9548_num_t num;
    uint8_t addr;
    uint8_t channel_mask;
    bool is_open;
} tca9548_t;

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t tca9548_open(tca9548_t *p_dev, uint8_t addr, tca9548_num_t number);

zerr_t tca9548_select(tca9548_t *p_dev, uint8_t channel);

zerr_t tca9548_select_mask(tca9548_t *p_dev, uint8_t channel_mask);

zerr_t tca9548_disable_all(tca9548_t *p_dev);

zerr_t tca9548_get_channel_mask(tca9548_t *p_dev, uint8_t *p_channel_mask);

zerr_t tca9548_scan_addr_channel(void);

zerr_t tca958_reset(tca9548_t *p_dev);

zerr_t tca9548_close(tca9548_t *p_dev);

#ifdef __cplusplus
}
#endif

#endif /* __TCA9548_H__ */

/* END OF FILE ************************************************************************************/

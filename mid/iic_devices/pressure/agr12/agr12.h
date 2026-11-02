
/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : agr12.h
 * @description     : AGR12 I2C pressure sensor driver.
***************************************************************************************************************/

#ifndef AGR12_H
#define AGR12_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/

#include <stdint.h>
#include "zerr.h"
#include "driver/i2c.h"

/* Definitions -----------------------------------------------------------------------------------*/
#define AGR12_MEASURE_DELAY_MS      80U     /* Datasheet 7.2.1. */

/* Type definitions ------------------------------------------------------------------------------*/

typedef struct
{
    uint8_t address;
    uint32_t timeout_ms;
    uint8_t is_open;
} agr12_t;

typedef struct
{
    int16_t pressure_raw;
    float pressure_kpa;
    uint8_t crc;
} agr12_data_t;

/* Public functions ------------------------------------------------------------------------------*/

zerr_t agr12_open(agr12_t *p_sensor);
zerr_t agr12_read(agr12_t *p_sensor, agr12_data_t *p_data);
/* Non-blocking pair: trigger, wait AGR12_MEASURE_DELAY_MS, then fetch. Lets the caller start
   several sensors and wait once. */
zerr_t agr12_trigger(agr12_t *p_sensor);
zerr_t agr12_fetch(agr12_t *p_sensor, agr12_data_t *p_data);
zerr_t agr12_get_pressure(agr12_t *p_sensor, float *p_pressure_kpa);
zerr_t agr12_close(agr12_t *p_sensor);

#ifdef __cplusplus
}
#endif

#endif /* AGR12_H */

/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : pressure_array.h
 * @description     : 13 AGR12 pressure sensors behind two TCA9548A multiplexers.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef PRESSURE_ARRAY_H
#define PRESSURE_ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include <stdint.h>
#include "zerr.h"

/* Definitions -----------------------------------------------------------------------------------*/
#define PRESSURE_SENSOR_NUM         13U

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief  Open the I2C bus, both multiplexers and all sensors.
 * @retval ZERR_OK when both multiplexers answer. Sensors are checked on each read.
 */
zerr_t pressure_array_open(void);

/**
 * @brief  Read every sensor in one pass: trigger all, wait 80 ms once, then fetch all.
 * @param  p_kpa    Array of PRESSURE_SENSOR_NUM values in kPa (a failed sensor keeps NAN).
 * @param  p_ok_mask Bit n set when sensor n was read with a valid CRC. May be NULL.
 * @retval ZERR_OK when every sensor was read, ZERR_ERROR when at least one failed.
 */
zerr_t pressure_array_read_all(float *p_kpa, uint32_t *p_ok_mask);

/**
 * @brief  Read one sensor (blocking, about 80 ms).
 */
zerr_t pressure_array_read(uint8_t index, float *p_kpa);

#ifdef __cplusplus
}
#endif

#endif /* PRESSURE_ARRAY_H */

/* END OF FILE ************************************************************************************/

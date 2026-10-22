
/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : agr12_bsp.h
 * @description     : AGR12 I2C pressure sensor hardware interface.
***************************************************************************************************************/

#ifndef AGR12_BSP_H
#define AGR12_BSP_H

#include "zcfg.h"

/* Definitions -----------------------------------------------------------------------------------*/

#define AGR12_BSP_I2C_ADDR             0x50U
#define AGR12_BSP_CMD_MEASURE          0xACU
#define AGR12_BSP_CMD_PRESSURE         0x12U
#define AGR12_BSP_MEASURE_DELAY_MS     200U
#define AGR12_BSP_DATA_SIZE            3U

/* Types -----------------------------------------------------------------------------------------*/

typedef struct
{
    uint16_t pressure_raw;
    uint8_t crc;
} agr12_bsp_data_t;

/* Public Functions ------------------------------------------------------------------------------*/

zerr_t agr12_bsp_open(void);

zerr_t agr12_bsp_read(uint8_t address, uint32_t timeout_ms, agr12_bsp_data_t *p_data);

#endif /* AGR12_BSP_H */

/* END OF FILE ************************************************************************************/

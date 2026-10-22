
/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : agr12.c
 * @description     : AGR12 I2C pressure sensor middleware.
***************************************************************************************************************/

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"
#include "agr12.h"
#include "agr12_bsp.h"

/* Definitions -----------------------------------------------------------------------------------*/

#define AGR12_READ_RETRY_MAX       100U
#define AGR12_READ_RETRY_DELAY     100U
#define AGR12_DEFAULT_TIMEOUT_MS  1000U

/* Public Functions ------------------------------------------------------------------------------*/

/**
 * @brief Open AGR12 sensor.
 */
zerr_t agr12_open(agr12_t *p_sensor)
{
    zerr_t ret;

    if (NULL == p_sensor)
    {
        return ZERR_INVALID_PARAM;
    }

    ret = agr12_bsp_open();
    if (ZERR_OK != ret)
    {
        return ret;
    }

    memset(p_sensor, 0, sizeof(*p_sensor));

    p_sensor->address = AGR12_BSP_I2C_ADDR;
    p_sensor->timeout_ms = AGR12_DEFAULT_TIMEOUT_MS;
    p_sensor->is_open = 1U;

    ZTRACE_DBG(TR_TCA, ("AGR12 opened: addr=0x%02X", p_sensor->address));

    return ZERR_OK;
}

/**
 * @brief Trigger measurement and read pressure data.
 */
zerr_t agr12_read(agr12_t *p_sensor, agr12_data_t *p_data)
{
    zerr_t ret;
    agr12_bsp_data_t bsp_data = {0};

    if ((NULL == p_sensor) || (NULL == p_data) || (0U == p_sensor->is_open))
    {
        return ZERR_INVALID_PARAM;
    }

    ret = agr12_bsp_read(p_sensor->address, p_sensor->timeout_ms, &bsp_data);
    if (ZERR_OK != ret)
    {
        return ret;
    }

    p_data->pressure_raw = (int16_t)bsp_data.pressure_raw;
    p_data->pressure_kpa = (float)p_data->pressure_raw / 10.0f;
    p_data->crc = bsp_data.crc;

    return ZERR_OK;
}

/**
 * @brief Read pressure in kPa with retry.
 */
zerr_t agr12_get_pressure(agr12_t *p_sensor, float *p_pressure_kpa)
{
    zerr_t ret = ZERR_ERROR;
    agr12_data_t data = {0};

    if ((NULL == p_sensor) || (NULL == p_pressure_kpa))
    {
        return ZERR_INVALID_PARAM;
    }

    if (0U == p_sensor->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    ret = agr12_read(p_sensor, &data);

    if (ZERR_OK == ret)
    {
        *p_pressure_kpa = data.pressure_kpa;
        return ZERR_OK;
    }

    return ret;
}

/**
 * @brief Close AGR12 driver.
 */
zerr_t agr12_close(agr12_t *p_sensor)
{
    if (NULL == p_sensor)
    {
        return ZERR_INVALID_PARAM;
    }

    memset(p_sensor, 0, sizeof(*p_sensor));

    return ZERR_OK;
}

/* END OF FILE ************************************************************************************/

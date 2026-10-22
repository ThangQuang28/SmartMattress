
/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : tca9548.c
 * @description     : TCA9548A middleware implementation.
***************************************************************************************************************/

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"
#include "tca9548_bsp.h"
#include "iic_global_bsp.h"

/* Public Functions ------------------------------------------------------------------------------*/

/**
 * @brief Initialize TCA9548A software context and disable all channels.
 */
zerr_t tca9548_open(tca9548_t *p_dev, uint8_t addr, tca9548_num_t number)
{
    zerr_t ret;

    if (NULL == p_dev)
    {
        return ZERR_INVALID_PARAM;
    }

    ret = tca9548_bsp_open();
    if (ZERR_OK != ret)
    {
        return ZERR_NOT_OPEN;
    }

    p_dev->addr = addr;
    p_dev->channel_mask = 0U;
    p_dev->is_open = false;
    p_dev->num = number;

    ret = tca9548_bsp_write(p_dev->addr, 0U);
    if (ZERR_OK != ret)
    {
        ZTRACE_DBG(TR_TCA, ("Could not open TCA addr=0x%02X, channel mask=0x%02X, number =%d", 
                           p_dev->addr, p_dev->channel_mask, (size_t)number));
        return ret;
    }

    p_dev->is_open = true;

    return ZERR_OK;
}

/**
 * @brief Select one channel (0..7).
 */
zerr_t tca9548_select(tca9548_t *p_dev, uint8_t channel)
{
    if ((NULL == p_dev) ||
        (false == p_dev->is_open) ||
        (channel >= TCA9548_CHANNEL_NUM))
    {
        return ZERR_INVALID_PARAM;
    }

    return tca9548_select_mask(p_dev, (uint8_t)(1U << channel));
}

/**
 * @brief Select one or multiple channels using a bit mask.
 */
zerr_t tca9548_select_mask(tca9548_t *p_dev, uint8_t channel_mask)
{
    zerr_t ret;

    if ((NULL == p_dev) || (false == p_dev->is_open))
    {
        return ZERR_INVALID_PARAM;
    }

    ret = tca9548_bsp_write(p_dev->addr, channel_mask);
    if (ZERR_OK != ret)
    {
        return ret;
    }

    p_dev->channel_mask = channel_mask;

    ZTRACE_DBG(TR_TCA, ("TCA addr=0x%02X, channel mask=0x%02X",
                           p_dev->addr, channel_mask));

    return ZERR_OK;
}

/**
 * @brief Disable all channels.
 */
zerr_t tca9548_disable_all(tca9548_t *p_dev)
{
    return tca9548_select_mask(p_dev, 0U);
}

/**
 * @brief Scan I2C devices on the currently selected channel.
 */
zerr_t tca9548_scan_addr_channel(void)
{
    return tca9548_bsp_scan_addr_channel();
}

/**
 * @brief Read the actual channel mask from TCA9548A.
 */
zerr_t tca9548_get_channel_mask(tca9548_t *p_dev, uint8_t *p_channel_mask)
{
    zerr_t ret;

    if ((NULL == p_dev) ||
        (false == p_dev->is_open) ||
        (NULL == p_channel_mask))
    {
        return ZERR_INVALID_PARAM;
    }

    ret = tca9548_bsp_read(p_dev->addr, p_channel_mask);
    if (ZERR_OK != ret)
    {
        return ret;
    }

    p_dev->channel_mask = *p_channel_mask;

    return ZERR_OK;
}

zerr_t tca958_reset(tca9548_t *p_dev)
{
    zerr_t ret;

    if ((NULL == p_dev) || (false == p_dev->is_open))
    {
        return ZERR_INVALID_PARAM;
    }

    ret = tca9548_bsp_reset(p_dev->num);
    if (ZERR_OK != ret)
    {
        return ret;
    }

    p_dev->channel_mask = 0U;

    return ZERR_OK;
}
/**
 * @brief Close TCA9548A software context and disable all channels.
 */
zerr_t tca9548_close(tca9548_t *p_dev)
{
    zerr_t ret;

    if ((NULL == p_dev) || (false == p_dev->is_open))
    {
        return ZERR_INVALID_PARAM;
    }

    ret = tca9548_disable_all(p_dev);
    if (ZERR_OK != ret)
    {
        return ret;
    }

    p_dev->is_open = false;

    return ZERR_OK;
}

/* END OF FILE ************************************************************************************/

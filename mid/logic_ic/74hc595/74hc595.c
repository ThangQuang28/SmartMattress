/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : 74hc595.c                                                                           
 * @dissaption      : 74HC595 driver board support package.                                                      
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"

/* Middle */
#include "74hc595.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t 
z74hc595_open(z74hc595_t *p_dev, gpio_num_t pin_data, gpio_num_t pin_clock, gpio_num_t pin_latch, uint8_t chip_num)
{
    zerr_t ret = ZERR_OK;

    if (p_dev == NULL || chip_num == 0)
    {
        return ZERR_INVALID_PARAM;
    }

    memset(p_dev, 0, sizeof(*p_dev));

    p_dev->p_data = calloc(chip_num, sizeof(uint8_t));
    if (p_dev->p_data == NULL)
    {
        return ZERR_MEM_OUT;
    }

    ret = h595_bsp_open(&p_dev->bsp, pin_data, pin_clock, pin_latch);
    if (ZERR_OK != ret)
    {
        free(p_dev->p_data);
        p_dev->p_data = NULL;
        return ZERR_NOT_OPEN;
    }

    p_dev->chip_num = chip_num;
    p_dev->is_open = true;

    ret = z74hc595_update(p_dev);
    if (ZERR_OK != ret)
    {
        h595_bsp_close(&p_dev->bsp);
        free(p_dev->p_data);
        p_dev->p_data = NULL;
        p_dev->chip_num = 0;
        p_dev->is_open = false;
        return ret;
    }

    return ESP_OK;
}

zerr_t z74hc595_close(z74hc595_t *p_dev)
{
    if (p_dev == NULL || !p_dev->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    zerr_t ret = h595_bsp_close(&p_dev->bsp);

    free(p_dev->p_data);
    p_dev->p_data = NULL;
    p_dev->chip_num = 0;
    p_dev->is_open = false;

    return ret;
}

zerr_t z74hc595_set_pin(z74hc595_t *p_dev, uint16_t pin, bool level)
{
    if (p_dev == NULL || !p_dev->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    if (pin >= (uint16_t)p_dev->chip_num * 8U)
    {
        return ZERR_INVALID_PARAM;
    }

    uint8_t chip = pin / 8U;
    uint8_t bit = pin % 8U;

    if (level)
    {
        p_dev->p_data[chip] |= (uint8_t)(1U << bit);
    }
    else
    {
        p_dev->p_data[chip] &= (uint8_t)~(1U << bit);
    }

    return z74hc595_update(p_dev);
}

zerr_t z74hc595_get_pin(z74hc595_t *p_dev, uint16_t pin, bool *p_level)
{
    if (p_dev == NULL || !p_dev->is_open || p_level == NULL)
    {
        return ZERR_INVALID_PARAM;
    }

    if (pin >= (uint16_t)p_dev->chip_num * 8U)
    {
        return ZERR_INVALID_PARAM;
    }

    uint8_t chip = pin / 8U;
    uint8_t bit = pin % 8U;

    *p_level = ((p_dev->p_data[chip] >> bit) & 0x01U) != 0;

    return ZERR_OK;
}

zerr_t z74hc595_write_byte(z74hc595_t *p_dev, uint8_t chip, uint8_t value)
{
    if (p_dev == NULL || !p_dev->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    if (chip >= p_dev->chip_num)
    {
        return ZERR_INVALID_PARAM;
    }

    p_dev->p_data[chip] = value;

    return z74hc595_update(p_dev);
}

zerr_t z74hc595_write_bytes(z74hc595_t *p_dev, const uint8_t *p_data, uint8_t chip_num)
{
    if (p_dev == NULL || !p_dev->is_open || p_data == NULL)
    {
        return ZERR_INVALID_PARAM;
    }

    if (chip_num != p_dev->chip_num)
    {
        return ZERR_INVALID_PARAM;
    }

    memcpy(p_dev->p_data, p_data, p_dev->chip_num);

    return z74hc595_update(p_dev);
}

zerr_t z74hc595_write_all(z74hc595_t *p_dev, bool level)
{
    if (p_dev == NULL || !p_dev->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    memset(p_dev->p_data,
           level ? 0xFF : 0x00,
           p_dev->chip_num);

    return z74hc595_update(p_dev);
}

zerr_t z74hc595_update(z74hc595_t *p_dev)
{
    if (p_dev == NULL || !p_dev->is_open || p_dev->p_data == NULL)
    {
        return ZERR_INVALID_PARAM;
    }

    /*
     * IC cuoi nhan du lieu truoc.
     * IC dau tien nhan du lieu sau cung.
     */
    for (int16_t i = (int16_t)p_dev->chip_num - 1; i >= 0; i--)
    {
        esp_err_t ret = h595_bsp_shift_out(&p_dev->bsp,
                                           p_dev->p_data[i]);
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    return h595_bsp_latch(&p_dev->bsp);
}

uint16_t z74hc595_get_pin_count(const z74hc595_t *p_dev)
{
    if (p_dev == NULL || !p_dev->is_open)
    {
        return 0;
    }

    return (uint16_t)p_dev->chip_num * 8U;
}

//Use Guide
// ESP_ERROR_CHECK(z74hc595_open(
//     &gh_74hc595,
//     GPIO_NUM_23,    /* DS */
//     GPIO_NUM_18,    /* SHCP */
//     GPIO_NUM_5,     /* STCP */
//     4               /* Number of ICs */
// ));

// uint8_t data[4] = {
//     0x01,   /* IC 0: Q0 ON */
//     0x55,   /* IC 1: Q0, Q2, Q4, Q6 ON */
//     0xAA,   /* IC 2: Q1, Q3, Q5, Q7 ON */
//     0xF0    /* IC 3: Q4, Q5, Q6, Q7 ON */
// };

// ESP_ERROR_CHECK(z74hc595_write_bytes(
//     &gh_74hc595,
//     data,
//     sizeof(data)
// ));

// ESP_LOGI(TAG, "74HC595 initialized: %u outputs",
//          (unsigned int)z74hc595_get_pin_count(&gh_74hc595));

// while (1)
// {
//     /* Toggle output 0 */
//     ESP_ERROR_CHECK(z74hc595_set_pin(&gh_74hc595, 0, false));
//     vTaskDelay(pdMS_TO_TICKS(500));

//     ESP_ERROR_CHECK(z74hc595_set_pin(&gh_74hc595, 0, true));
//     vTaskDelay(pdMS_TO_TICKS(500));
// }


/* END OF FILE ************************************************************************************/
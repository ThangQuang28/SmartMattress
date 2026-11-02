/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : 74hc595_bsp.c                                                                           
 * @dissaption      : 74HC595 driver board support package.                                                      
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/

/* Global */
#include "zcfg.h"

/* BSP. */
#include "74hc595_bsp.h"
#include "esp_rom_sys.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Half period of SHCP/STCP. The ref/02 driver that switched valves one by one on this board logged
   every bit, which made each clock phase last milliseconds; 1 us phases make one valve drive the
   whole chain (long wires / slow edges). 50 us keeps a 32-bit write under 10 ms. */
#define H595_BSP_HALF_PERIOD_US     50U

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/
static zerr_t h595_bsp_set_level(gpio_num_t pin, uint32_t level);

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t 
h595_bsp_open(h595_bsp_t *p_bsp, gpio_num_t pin_data, gpio_num_t pin_clock, gpio_num_t pin_latch)
{
    if (p_bsp == NULL ||
        pin_data < 0 || pin_data >= GPIO_NUM_MAX ||
        pin_clock < 0 || pin_clock >= GPIO_NUM_MAX ||
        pin_latch < 0 || pin_latch >= GPIO_NUM_MAX ||
        pin_data == pin_clock ||
        pin_data == pin_latch ||
        pin_clock == pin_latch)
    {
        return ZERR_INVALID_PARAM;
    }

    memset(p_bsp, 0, sizeof(*p_bsp));

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin_data) |
                        (1ULL << pin_clock) |
                        (1ULL << pin_latch),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        return ZERR_NOT_OPEN;
    }

    p_bsp->pin_data = pin_data;
    p_bsp->pin_clock = pin_clock;
    p_bsp->pin_latch = pin_latch;
    p_bsp->is_open = true;

    ret = h595_bsp_set_level(pin_data, 0);
    if (ret == ESP_OK)
    {
        ret = h595_bsp_set_level(pin_clock, 0);
    }
    if (ret == ESP_OK)
    {
        ret = h595_bsp_set_level(pin_latch, 0);
    }

    if (ret != ESP_OK)
    {
        h595_bsp_close(p_bsp);
        return ZERR_NOT_OPEN;
    }

    return ZERR_OK;
}

zerr_t h595_bsp_close(h595_bsp_t *p_bsp)
{
    if (p_bsp == NULL || !p_bsp->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    gpio_reset_pin(p_bsp->pin_data);
    gpio_reset_pin(p_bsp->pin_clock);
    gpio_reset_pin(p_bsp->pin_latch);

    p_bsp->is_open = false;

    return ZERR_OK;
}

zerr_t h595_bsp_shift_out(h595_bsp_t *p_bsp, uint8_t data)
{
    if (p_bsp == NULL || !p_bsp->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    for (int8_t bit = 7; bit >= 0; bit--)
    {
        esp_err_t ret = h595_bsp_set_level(p_bsp->pin_clock, 0);
        if (ret != ESP_OK)
        {
            return ret;
        }
        esp_rom_delay_us(H595_BSP_HALF_PERIOD_US); /* Hold: data changes only after SHCP is low. */

        ret = h595_bsp_set_level(p_bsp->pin_data,
                                 (data >> bit) & 0x01U);
        if (ret != ESP_OK)
        {
            return ZERR_ERROR;
        }

        esp_rom_delay_us(H595_BSP_HALF_PERIOD_US); /* Data setup before the rising edge. */
        ret = h595_bsp_set_level(p_bsp->pin_clock, 1);
        if (ret != ESP_OK)
        {
            return ZERR_ERROR;
        }
        esp_rom_delay_us(H595_BSP_HALF_PERIOD_US); /* SHCP high time. */
    }

    esp_err_t ret = h595_bsp_set_level(p_bsp->pin_clock, 0);
    esp_rom_delay_us(H595_BSP_HALF_PERIOD_US);  /* Last bit settled before the latch. */
    return ret;
}

zerr_t h595_bsp_latch(h595_bsp_t *p_bsp)
{
    if (p_bsp == NULL || !p_bsp->is_open)
    {
        return ZERR_INVALID_PARAM;
    }

    esp_err_t ret = h595_bsp_set_level(p_bsp->pin_latch, 0);
    if (ret != ESP_OK)
    {
        return ZERR_ERROR;
    }

    esp_rom_delay_us(H595_BSP_HALF_PERIOD_US);
    ret = h595_bsp_set_level(p_bsp->pin_latch, 1);
    if (ret != ESP_OK)
    {
        return ZERR_ERROR;
    }

    esp_rom_delay_us(H595_BSP_HALF_PERIOD_US);
    return h595_bsp_set_level(p_bsp->pin_latch, 0);
}

/* Private function prototypes -------------------------------------------------------------------*/
static zerr_t h595_bsp_set_level(gpio_num_t pin, uint32_t level)
{
    return (ESP_OK == gpio_set_level(pin, level)) ? ZERR_OK : ZERR_ERROR;
}

/* END OF FILE ************************************************************************************/

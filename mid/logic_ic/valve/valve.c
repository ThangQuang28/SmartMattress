/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : valve.c
 * @description     : Valve outputs on a chain of 4 x 74HC595.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "valve.h"
#include "74hc595.h"
#include "flash.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Default wiring (ref/02/74hc595.c). The web page can change it at run time; the choice is kept
   in NVS, so a wrong pin guess does not need a new firmware. */
#define VALVE_PIN_DS                GPIO_NUM_23     /* Serial data   -> DS   (pin 14) */
#define VALVE_PIN_SHCP              GPIO_NUM_18     /* Shift clock   -> SHCP (pin 11) */
#define VALVE_PIN_STCP              GPIO_NUM_5      /* Latch clock   -> STCP (pin 12) */
#define VALVE_NVS_KEY_PINS          "v_pins"

/* Outputs are active HIGH: logical ON = Q pin HIGH (user test, 22-Oct-2026). */

/* Private variables -----------------------------------------------------------------------------*/

static z74hc595_t gh_595;
static SemaphoreHandle_t g_lock = NULL;
static uint32_t g_mask = 0U;
static uint8_t g_pins[3] = {VALVE_PIN_DS, VALVE_PIN_SHCP, VALVE_PIN_STCP};

static const char *TAG = "VALVE";

/* Private function prototypes -------------------------------------------------------------------*/

static zerr_t valve_write(uint32_t mask);
static zerr_t valve_open_pins(void);
static bool valve_pin_ok(uint8_t pin);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t valve_open(void)
{
    if (NULL != g_lock)
    {
        return ZERR_OK;
    }

    g_lock = xSemaphoreCreateMutex();
    if (NULL == g_lock)
    {
        return ZERR_MEM_OUT;
    }

    uint8_t pins[3];
    size_t len = 0U;
    if ((ZERR_OK == flash_read(VALVE_NVS_KEY_PINS, pins, sizeof(pins), &len)) &&
        (sizeof(pins) == len) && valve_pin_ok(pins[0]) && valve_pin_ok(pins[1]) &&
        valve_pin_ok(pins[2]) && (pins[0] != pins[1]) && (pins[0] != pins[2]) &&
        (pins[1] != pins[2]))
    {
        memcpy(g_pins, pins, sizeof(g_pins));
    }

    zerr_t ret = valve_open_pins();
    if (ZERR_OK != ret)
    {
        ESP_LOGE(TAG, "open failed: %d", (int)ret);
        vSemaphoreDelete(g_lock);
        g_lock = NULL;
        return ret;
    }

    g_mask = 0U;
    ESP_LOGI(TAG, "open: %u outputs, all off, DS=GPIO%u SHCP=GPIO%u STCP=GPIO%u",
             (unsigned int)VALVE_OUTPUT_NUM, g_pins[0], g_pins[1], g_pins[2]);

    return ZERR_OK;
}

zerr_t valve_set_pins(uint8_t ds, uint8_t shcp, uint8_t stcp)
{
    if (!valve_pin_ok(ds) || !valve_pin_ok(shcp) || !valve_pin_ok(stcp) || (ds == shcp) ||
        (ds == stcp) || (shcp == stcp))
    {
        return ZERR_INVALID_PARAM;
    }
    if (NULL == g_lock)
    {
        return ZERR_NOT_OPEN;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    (void)valve_write(0U);                  /* Old pins: everything off first. */
    (void)z74hc595_close(&gh_595);
    g_pins[0] = ds;
    g_pins[1] = shcp;
    g_pins[2] = stcp;
    zerr_t ret = valve_open_pins();         /* New pins start with everything off. */
    g_mask = 0U;
    if (ZERR_OK == ret)
    {
        ret = flash_write(VALVE_NVS_KEY_PINS, g_pins, sizeof(g_pins));
    }
    xSemaphoreGive(g_lock);

    ESP_LOGI(TAG, "pins DS=GPIO%u SHCP=GPIO%u STCP=GPIO%u (%d)", ds, shcp, stcp, (int)ret);
    return ret;
}

void valve_get_pins(uint8_t *p_pins)
{
    memcpy(p_pins, g_pins, sizeof(g_pins));
}

zerr_t valve_set(uint8_t index, bool on)
{
    if (index >= VALVE_OUTPUT_NUM)
    {
        return ZERR_INVALID_PARAM;
    }
    if (NULL == g_lock)
    {
        return ZERR_NOT_OPEN;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    uint32_t mask = on ? (g_mask | (1UL << index)) : (g_mask & ~(1UL << index));
    zerr_t ret = valve_write(mask);
    xSemaphoreGive(g_lock);

    return ret;
}

zerr_t valve_set_mask(uint32_t mask)
{
    if (NULL == g_lock)
    {
        return ZERR_NOT_OPEN;
    }

    xSemaphoreTake(g_lock, portMAX_DELAY);
    zerr_t ret = valve_write(mask);
    xSemaphoreGive(g_lock);

    return ret;
}

zerr_t valve_all_off(void)
{
    return valve_set_mask(0U);
}

uint32_t valve_get_mask(void)
{
    return g_mask;
}

zerr_t valve_step_test(uint32_t on_ms)
{
    zerr_t ret = ZERR_OK;

    for (uint8_t i = 0U; (i < VALVE_OUTPUT_NUM) && (ZERR_OK == ret); i++)
    {
        ret = valve_set_mask(1UL << i);
        if (ZERR_OK == ret)
        {
            ESP_LOGI(TAG, "test: output %02u (IC%u Q%u) ON", (unsigned int)(i + 1U),
                     (unsigned int)(i / 8U + 1U), (unsigned int)(i % 8U));
            vTaskDelay(pdMS_TO_TICKS(on_ms));
        }
    }

    /* Always end with everything off, also after an error. */
    zerr_t off = valve_all_off();
    ESP_LOGI(TAG, "test: done, all off (%d)", (int)((ZERR_OK != ret) ? ret : off));

    return (ZERR_OK != ret) ? ret : off;
}

/* Private functions -----------------------------------------------------------------------------*/

/**
 * @brief Shift the 4 bytes out (IC0 = bits 0-7) and latch. Called with g_lock held.
 */
static zerr_t valve_write(uint32_t mask)
{
    uint8_t data[VALVE_IC_NUM];

    for (uint8_t ic = 0U; ic < VALVE_IC_NUM; ic++)
    {
        data[ic] = (uint8_t)(mask >> (ic * 8U));
    }

    zerr_t ret = z74hc595_write_bytes(&gh_595, data, (uint8_t)VALVE_IC_NUM);
    if (ZERR_OK == ret)
    {
        g_mask = mask;
    }

    return ret;
}

/**
 * @brief  Open the 595 chain on g_pins with every output off (all bits LOW).
 */
static zerr_t valve_open_pins(void)
{
    return z74hc595_open_initial(&gh_595, (gpio_num_t)g_pins[0], (gpio_num_t)g_pins[1],
                                 (gpio_num_t)g_pins[2], (uint8_t)VALVE_IC_NUM, 0x00U);
}

/**
 * @brief  Output-capable GPIO not used by the flash, I2C (21/22), TCA resets (16/17) or UART0.
 */
static bool valve_pin_ok(uint8_t pin)
{
    static const uint8_t allowed[] = {4, 5, 13, 14, 18, 19, 23, 25, 26, 27, 32, 33};

    for (size_t i = 0U; i < sizeof(allowed); i++)
    {
        if (allowed[i] == pin)
        {
            return true;
        }
    }
    return false;
}

/* END OF FILE ************************************************************************************/

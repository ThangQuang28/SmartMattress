/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : esp_bsp.c                                                                            
 * @dissaption      : Esp driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/** BSP. */
#include "esp_bsp.h"

#include "driver/uart.h"
#include "driver/gpio.h"

/* Define ----------------------------------------------------------------------------------------*/

#define ESP_UART_PORT      UART_NUM_1
#define ESP_UART_TX_PIN    GPIO_NUM_6
#define ESP_UART_RX_PIN    GPIO_NUM_7
#define ESP_UART_BAUDRATE  9600

/* Private data types ----------------------------------------------------------------------------*/

static esp_bsp_uart_rx_callback_t s_bsp_rx_callback = NULL;

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static void esp_bsp_uart_task(void *argument);

/* Public function prototypes --------------------------------------------------------------------*/

bool esp_bsp_uart_open(void)
{
    uart_config_t uart_config =
    {
        .baud_rate = ESP_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(
        ESP_UART_PORT,
        ESP_UART_RX_BUFFER_SIZE,
        ESP_UART_TX_BUFFER_SIZE,
        0,
        NULL,
        0));

    ESP_ERROR_CHECK(uart_param_config(
        ESP_UART_PORT,
        &uart_config));

    ESP_ERROR_CHECK(uart_set_pin(
        ESP_UART_PORT,
        ESP_UART_TX_PIN,
        ESP_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));

    xTaskCreate(
        esp_bsp_uart_task,
        "esp_bsp_uart_task",
        4096,
        NULL,
        5,
        NULL);

    return true;
}

uint16_t esp_bsp_uart_send(const uint8_t *data, uint16_t length)
{
    return (uint16_t)uart_write_bytes(ESP_UART_PORT, (const char *)data, length);
}

void esp_uart_bsp_register_rx_callback(esp_bsp_uart_rx_callback_t callback)
{
    s_bsp_rx_callback = callback;
}

/* Private function prototypes -------------------------------------------------------------------*/

static void esp_bsp_uart_task(void *argument)
{
    uint8_t buffer[256];

    while (true)
    {
        int length = uart_read_bytes(ESP_UART_PORT, buffer, sizeof(buffer), pdMS_TO_TICKS(100));

        if ((length > 0) && (s_bsp_rx_callback != NULL))
        {
            s_bsp_rx_callback(buffer, (uint16_t)length);
        }
    }
}

/* END OF FILE ************************************************************************************/
/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : esp_at.c                                                                            
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

/* Middleware */
#include "esp_at.h"

/* BSP. */
#include "esp_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static esp_at_rx_callback_t s_at_rx_callback = NULL;

static void esp_at_uart_receive_callback(const uint8_t *data, uint16_t length);

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

bool esp_at_open(void)
{
    esp_bsp_uart_open();
    esp_uart_bsp_register_rx_callback(esp_at_uart_receive_callback);
    return true;
}

bool esp_at_send_command(const char *command)
{
    if (command == NULL)
    {
        return false;
    }

    esp_bsp_uart_send((const uint8_t *)command, strlen(command));
    esp_bsp_uart_send((const uint8_t *)"\r\n",2U);
    return true;
}

uint16_t esp_at_send(const uint8_t *data, uint16_t length)
{
    return esp_bsp_uart_send(data,length);
}

void esp_at_register_rx_callback(esp_at_rx_callback_t callback)
{
    s_at_rx_callback = callback;
}

/* Private function prototypes -------------------------------------------------------------------*/

static void esp_at_uart_receive_callback(const uint8_t *data, uint16_t length)
{
    if (s_at_rx_callback != NULL)
    {
        s_at_rx_callback(data, length);
    }
}

/* END OF FILE ************************************************************************************/
/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : esp_bsp.h                                                                            
 * @dissaption      : Esp driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _ESP_BSP_H
#define _ESP_BSP_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/

#define ESP_UART_RX_BUFFER_SIZE    1024U
#define ESP_UART_TX_BUFFER_SIZE    1024U

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef void (*esp_bsp_uart_rx_callback_t)(const uint8_t *data, uint16_t length);

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief Initialize UART driver.
 *
 * @return true  Success.
 * @return false Failed.
 */
bool esp_bsp_uart_open(void);

/**
 * @brief Send data.
 *
 * @param data   Data buffer.
 * @param length Data length.
 *
 * @return Number of bytes sent.
 */
uint16_t esp_bsp_uart_send(const uint8_t *data, uint16_t length);

/**
 * @brief Register receive callback.
 *
 * @param callback Receive callback.
 */
void esp_uart_bsp_register_rx_callback(esp_bsp_uart_rx_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_BSP_H */

/* END OF FILE ************************************************************************************/

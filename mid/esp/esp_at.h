/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : esp_at.h                                                                            
 * @dissaption      : Esp driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _ESP_AT_H
#define _ESP_AT_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/

#define ESP_AT_RX_BUFFER_SIZE      1024U

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef void (*esp_at_rx_callback_t)(const uint8_t *data, uint16_t length);

typedef enum
{
    FAN_MODE_AUTO = 0,
    FAN_MODE_MANUAL,
    FAN_MODE_SLEEP
} fan_mode_t;

typedef enum
{
    FAN_STATE_OFF = 0,
    FAN_STATE_ON
} fan_state_t;

typedef struct
{
    uint8_t hum;
    uint8_t temp;

    fan_mode_t mode;
    uint8_t speed;

    fan_state_t state;
    fan_state_t swing_state;
    fan_state_t sleep_state;

} fan_realtime_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief Initialize AT driver.
 */
bool esp_at_open(void);

/**
 * @brief Send raw AT command.
 *
 * @param command Command string.
 *
 * @return true Success.
 * @return false Failed.
 */
bool esp_at_send_command(const char *command);

/**
 * @brief Send raw data.
 *
 * @param data Data buffer.
 * @param length Data length.
 *
 * @return Number of bytes sent.
 */
uint16_t esp_at_send(const uint8_t *data, uint16_t length);

/**
 * @brief Register receive callback.
 *
 * @param callback Receive callback.
 */
void esp_at_register_rx_callback(esp_at_rx_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_AT_H */

/* END OF FILE ************************************************************************************/

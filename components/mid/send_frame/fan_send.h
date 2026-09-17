/***************************************************************************************************************
 * @auth            : phucdh                                                                                
 * @day             : 17-Jun-2026
 * @file            : fan_send.h                                                                            
 * @dissaption      : Time service driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _FAN_SEND_H
#define _FAN_SEND_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/
void fan_send_open(void);
void fan_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len);
void esp_at_receive(const uint8_t *data, uint16_t length);
void mqtt_app_receive(const char *topic, const char *payload);

#ifdef __cplusplus
}
#endif

#endif /* _FAN_SEND_H */

/* END OF FILE ************************************************************************************/

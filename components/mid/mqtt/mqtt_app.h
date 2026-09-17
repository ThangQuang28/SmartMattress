/***************************************************************************************************************
 * @auth            : duongdh                                                                                  
 * @day             : 4-Apr-2025
 * @file            : mqtt.h                                                                            
 * @dissaption      : mqtt component create FW-Core                                    
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _MQTT_H
#define _MQTT_H

#ifdef __cplusplus__
extern "C" {
#endif

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

/* Global configuration */

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

typedef void (*mqtt_rx_callback_t)(const char *topic, const char *payload);

void mqtt_app_start(void);

bool mqtt_app_is_connected(void);

bool mqtt_app_publish(const char *topic, const char *payload, int qos, bool retain);

int mqtt_app_subscribe(const char *topic, int qos);

void mqtt_app_register_callback(mqtt_rx_callback_t callback);

#ifdef __cplusplus__
}
#endif

#endif /* _MQTT_H */
/* END OF FILE ************************************************************************************/
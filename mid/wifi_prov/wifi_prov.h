/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : wifi_prov.h                                                                            
 * @dissaption      : Wi-Fi Provisioning Manager board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _WIFI_PROV_H
#define _WIFI_PROV_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef struct
{
    const char *service_name;
    const char *service_key;
    const char *pop;
} wifi_prov_config_t;


/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief Initialize WiFi provisioning service
 */
esp_err_t wifi_prov_open(void);

/**
 * @brief Start provisioning if device not provisioned
 */
esp_err_t wifi_prov_start(void);

/**
 * @brief Start STA mode using stored credentials
 */
esp_err_t wifi_prov_connect(void);

/**
 * @brief Reset provisioning information
 */
esp_err_t wifi_prov_reset(void);

/**
 * @brief Check provisioning status
 */
bool wifi_prov_is_provisioned(void);

void wifi_prov_wait_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* _WIFI_PROV_H */

/* END OF FILE ************************************************************************************/

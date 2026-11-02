/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : webui.h
 * @description     : WiFi (own access point + optional home WiFi) and the HTTP server that serves the
 *                    web app and its JSON API.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef WEBUI_H
#define WEBUI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include "zerr.h"

/* Definitions -----------------------------------------------------------------------------------*/

/* Fallback WiFi "SmartMattress-XXXX" (XXXX = end of the MAC), only on when no home WiFi is saved or
   it cannot be joined. A phone or laptop joins it and the page opens at http://192.168.4.1. */
#define WEBUI_AP_PASS               "12345678"
#define WEBUI_AP_CHANNEL            6U
#define WEBUI_AP_MAX_CONN           4U

/* On home WiFi the web app is at http://smartmattress.local (mDNS) or at the IP on the OLED. */
#define WEBUI_HOSTNAME              "smartmattress"

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief  Start WiFi (AP + STA) and the HTTP server.
 */
zerr_t webui_open(void);

/**
 * @brief  Save home WiFi credentials and connect to them. An empty ssid forgets them.
 */
zerr_t webui_set_sta(const char *p_ssid, const char *p_pass);

/**
 * @brief  Short lines (<= 15 chars) telling how to open the web app, for the OLED.
 */
size_t webui_net_text(char *p_buf, size_t size);

/**
 * @brief  Write the WiFi part of the state as a JSON object, e.g. {"ap_ssid":..,"sta_ip":..}.
 */
size_t webui_wifi_json(char *p_buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* WEBUI_H */

/* END OF FILE ************************************************************************************/

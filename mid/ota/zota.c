/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Mar-2026
 * @file            : ota.c
 * @dissaption      : ota module.
 ***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Include ---------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/* Middle */
#include "zota.h"

/* C standard library */
#include <string.h>

/* ESP-IDF */
#include "esp_check.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static size_t g_ota_rx_size = 0;

typedef struct
{
    zota_state_t state;
    uint8_t      b_is_open;
} zota_h_t;

static zota_h_t gh_zota = {.state = ZOTA_STATE_CLOSED, .b_is_open = 0};

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static esp_err_t zota_http_event_handler(esp_http_client_event_t* p_event);

/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief Open OTA module.
 * @return ZERR_OK if successful.
 */
zerr_t zota_open(void)
{
    if (1 == gh_zota.b_is_open)
    {
        ZTRACE_DBG(TR_OTA, ("%s: ota alreadly opened..", __FUNCTION__));
        /* code */
    }
    
    gh_zota.state = ZOTA_STATE_OPEN;
    gh_zota.b_is_open = 1;

    ZTRACE_DBG(TR_OTA, ("%s: OTA open..", __FUNCTION__));

    return ZERR_OK;
}

/**
 * @brief Start OTA update.
 * @param[in] p_url HTTPS URL of firmware image.
 * @return OTA result.
 */
zerr_t zota_start(const char* p_url)
{
    esp_http_client_config_t http_config;
    esp_https_ota_config_t  ota_config;
    esp_err_t ret;

    ZTRACE_ERR(TR_OTA, ("ERROR %s: Must open OTA first", __FUNCTION__), (1 == gh_zota.b_is_open), return ZERR_NOT_OPEN;)
    ZTRACE_ERR(TR_OTA, ("ERROR %s: Invalid param", __FUNCTION__), (NULL != p_url), return ZERR_INVALID_PARAM;)

    if (ZOTA_STATE_DOWNLOADING == gh_zota.state) {
        return ZERR_ERROR;
    }

    memset(&http_config, 0, sizeof(http_config));
    memset(&ota_config, 0, sizeof(ota_config));

    http_config.url = p_url;
    http_config.event_handler = zota_http_event_handler;
    http_config.keep_alive_enable = true;
    http_config.crt_bundle_attach = esp_crt_bundle_attach;

    ota_config.http_config = &http_config;
    gh_zota.state = ZOTA_STATE_DOWNLOADING;

    ZTRACE_DBG(TR_OTA, ("DEBUG : %s OTA start: %s", __FUNCTION__, p_url))

    ret = esp_https_ota(&ota_config);

    if (ESP_OK == ret) {
        gh_zota.state = ZOTA_STATE_SUCCESS;

        ZTRACE_DBG(TR_OTA, ("DEBUG : %s OTA success", __FUNCTION__));
        return ZERR_OK;
    }

    gh_zota.state = ZERR_ERROR;

    ZTRACE_DBG(TR_OTA, ("DEBUG : %s OTA failed: 0x%x", __FUNCTION__, ret));

    return ZERR_ERROR;
}

zerr_t zota_update(const char* p_url)
{
    zerr_t ret = ZERR_OK;

    ret = zota_start(p_url);

    if (ZERR_OK == ret) {
        ZTRACE_DBG(TR_OTA, ("DEBUG : %s: OTA reboot..", __FUNCTION__));
        esp_restart();
    }

    return ret;
}

zerr_t zota_get_state(void)
{
    return gh_zota.state;
}

bool zota_is_running(void)
{
    return (gh_zota.state == ZOTA_STATE_DOWNLOADING);
}

/* Private function prototypes -------------------------------------------------------------------*/

static esp_err_t zota_http_event_handler(esp_http_client_event_t* p_event)
{
    if (p_event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (p_event->event_id) {
        case HTTP_EVENT_ERROR:
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP error..", __FUNCTION__));
            break;

        case HTTP_EVENT_ON_CONNECTED:
            g_ota_rx_size = 0;
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP connected..", __FUNCTION__));
            break;

        case HTTP_EVENT_HEADER_SENT:
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP header sent..", __FUNCTION__));
            break;

        case HTTP_EVENT_ON_HEADER:
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP header: %s=%s..", __FUNCTION__, p_event->header_key, p_event->header_value));
            break;

        case HTTP_EVENT_ON_DATA:
            g_ota_rx_size += p_event->data_len;
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP data: %d bytes, total: %lu bytes..", __FUNCTION__, p_event->data_len, (unsigned long)g_ota_rx_size));
            break;

        case HTTP_EVENT_ON_FINISH:
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP header sent..", __FUNCTION__));
            break;

        case HTTP_EVENT_DISCONNECTED:
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP disconnected..", __FUNCTION__));
            break;

        case HTTP_EVENT_REDIRECT:
            ZTRACE_DBG(TR_OTA, ("DEBUG %s: HTTP redirect..", __FUNCTION__));
            break;

        default:
            break;
    }

    return ESP_OK;
}
/* END OF FILE ************************************************************************************/
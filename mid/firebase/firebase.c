/***************************************************************************************************************
 * @auth            : duongdh
 * @day             : 4-Apr-2025
 * @file            : firebase.c
 * @dissaption      : File to create FW-Core . Coding in free time
 ***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/
#include "zcfg.h"

#include "firebase.h"

/* ESP-IDF includes */
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_crt_bundle.h"

#include "esp_mac.h"

#include <netdb.h>

/* Define ----------------------------------------------------------------------------------------*/

#define FIREBASE_PATH "https://mstore-b0ee9-default-rtdb.firebaseio.com/devices/gsh/"
#define FIREBASE_URL_SIZE    256U
#define FIREBASE_RX_BUFFER_SIZE    1024U

static char g_firebase_rx_buffer[FIREBASE_RX_BUFFER_SIZE];
static uint16_t g_firebase_rx_len = 0U;

/* Private data types ----------------------------------------------------------------------------*/

#define FIREBASE_CERT_BUNDLE 1

#if defined (FIREBASE_CERT_PEM)
extern const char firebase_cert_pem_start[] asm("_binary_firebase_cert_pem_start");
extern const char firebase_cert_pem_end[] asm("_binary_firebase_cert_pem_end");
#endif

static void wifi_get_mac(char *p_mac, size_t size);

/* Public data types -----------------------------------------------------------------------------*/

typedef struct
{
    char    device_id[18];
    char    firebase_url[FIREBASE_URL_SIZE];
    uint8_t b_wifi_connected;
    uint8_t b_rev_mater;
    uint16_t last_rx_len;

} firebase_t;

/* Private function prototypes -------------------------------------------------------------------*/

static firebase_t gh_firebase = {0};
static firebase_callback_t gp_firebase_callback = NULL;

static esp_err_t firebase_https_event_handler(esp_http_client_event_t *evt);


/* Public function prototypes --------------------------------------------------------------------*/

zerr_t firebase_open(void)
{
    zerr_t ret = ZERR_OK;

    wifi_get_mac(gh_firebase.device_id, sizeof(gh_firebase.device_id));

    ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: Device ID: %s", __FUNCTION__, gh_firebase.device_id));

    snprintf(gh_firebase.firebase_url, sizeof(gh_firebase.firebase_url), "%s%s.json", 
        FIREBASE_PATH,
        gh_firebase.device_id
    );

    return ret;
}
/* Get data */
void firebase_get_data(void)
{
    esp_http_client_config_t config = {
        .url = gh_firebase.firebase_url,
        .event_handler = firebase_https_event_handler,
#if defined (FIREBASE_CERT_PEM)
        .cert_pem = firebase_cert_pem_start,
#endif
#if defined (FIREBASE_CERT_BUNDLE)
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
    }
    else
    {
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s:HTTP GET request failed: %s", __FUNCTION__, esp_err_to_name(err)));
    }

    esp_http_client_cleanup(client);
}

/* Push data to Firebase */
void firebase_put_data(const char *json_string)
{
    esp_http_client_config_t config = {
        .url = gh_firebase.firebase_url,
        .method = HTTP_METHOD_PUT,
#if defined (FIREBASE_CERT_PEM)
        .cert_pem = firebase_cert_pem_start,
#endif
#if defined (FIREBASE_CERT_BUNDLE)
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif
        .event_handler = firebase_https_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: HTTP PUT Success, Status = %d", __FUNCTION__, esp_http_client_get_status_code(client)));
    }
    else
    {
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: HTTP PUT request failed: %s", __FUNCTION__, esp_err_to_name(err)));
    }

    esp_http_client_cleanup(client);
}

void firebase_set_reg(firebase_callback_t callback)
{
    gp_firebase_callback = callback;
}
/* Private function prototypes -------------------------------------------------------------------*/

/* Firebase handle */
static esp_err_t firebase_https_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
    {
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: HTTP connected", __FUNCTION__));
    }
        break;
    case HTTP_EVENT_ON_HEADER:
    {
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: Header %s: %s", __FUNCTION__, evt->header_key, evt->header_value));
    }
    break;
    case HTTP_EVENT_ON_DATA:
    {
        uint16_t copy_len;
        uint16_t remain_len;

        ZTRACE_DBG(TR_FIREBASE,("DEBUG %s: Received data len = %d", __FUNCTION__, evt->data_len));

        /*
         * Save received data into local buffer.
         */
        if ((NULL != evt->data) && (evt->data_len > 0))
        {
            ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: Data = %.*s", __FUNCTION__, evt->data_len, (const char *)evt->data));

            if (g_firebase_rx_len < (FIREBASE_RX_BUFFER_SIZE - 1U))
            {
                remain_len = (FIREBASE_RX_BUFFER_SIZE - 1U) - g_firebase_rx_len;

                copy_len = (uint16_t)evt->data_len;

                if (copy_len > remain_len)
                {
                    copy_len = remain_len;

                    ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: Response truncated", __FUNCTION__));
                }

                memcpy(&g_firebase_rx_buffer[g_firebase_rx_len], evt->data, copy_len);

                g_firebase_rx_len += copy_len;

                g_firebase_rx_buffer[g_firebase_rx_len] = '\0';
            }
        }
    }
        break;
    case HTTP_EVENT_ON_FINISH:
    {
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: HTTP receive finished, len = %u", __FUNCTION__, g_firebase_rx_len));
        ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: Full data = %s", __FUNCTION__, g_firebase_rx_buffer));

        if (NULL != gp_firebase_callback)
        {
            gp_firebase_callback(g_firebase_rx_buffer, (uint32_t)g_firebase_rx_len);
        }
        else
        {
            ZTRACE_DBG(TR_FIREBASE, ("DEBUG %s: Callback NULL", __FUNCTION__));
        }
    }
    break;
    default:
        break;
    }
    return ESP_OK;
}

static void wifi_get_mac(char *p_mac, size_t size)
{
    uint8_t mac[6];

    if ((NULL == p_mac) || (size < 18U)) {
        return;
    }

    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf(p_mac, size, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
/* END OF FILE ************************************************************************************/
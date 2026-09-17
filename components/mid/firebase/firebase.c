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

#include <netdb.h>

/* Define ----------------------------------------------------------------------------------------*/

#define MAX_JSON_BUFFER 245

#define STATIC_IP_ADDR "192.168.20.1.132"
#define STATIC_NETMASK_ADDR "255.255.255.0"
#define STATIC_GW_ADDR "192.168.20.1"
#define MAIN_DNS_SERVER "8.8.8.8"
#define BACKUP_DNS_SERVER "1.1.1.1"

#define WIFI_SSID "meomeo"
#define WIFI_PASS "12345678"
#define MAXIMUM_RETRY 50

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define FIREBASE_WIFI_READY_BIT BIT2
#define FIREBASE_HTTP_DONE_BIT BIT3
#define FIREBASE_HTTP_ERROR_BIT BIT4

// #define FIREBASE_PATH "https://freelancer-bad45-default-rtdb.firebaseio.com/duonghd.json"

#define FIREBASE_PATH "https://ionic-wcat-62c46-default-rtdb.firebaseio.com/mimiZang.json"
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "Firebase";

static int s_retry_num = 0;

static int fb_data_len = 0;

/* Private data types ----------------------------------------------------------------------------*/

extern const char firebase_cert_pem_start[] asm("_binary_firebase_cert_pem_start");
extern const char firebase_cert_pem_end[] asm("_binary_firebase_cert_pem_end");

static TaskHandle_t firebase_task_handle = NULL;
static TaskHandle_t rx_task_handle = NULL;

/* Public data types -----------------------------------------------------------------------------*/

typedef struct
{
    uint8_t b_wifi_connected;
    uint8_t b_rev_mater;
    uint16_t last_rx_len;

} firebase_t;

/* Private function prototypes -------------------------------------------------------------------*/

static firebase_t gh_firebase = {0};

static esp_err_t set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type);
static void set_static_ip(esp_netif_t *netif);
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_init_sta(void);

static esp_err_t firebase_https_event_handler(esp_http_client_event_t *evt);

static void firebase_task(void *arg);

/* Public function prototypes --------------------------------------------------------------------*/

/* Open firebase */
void firebase_open(void)
{
    wifi_init_sta();
}

/* Get data */
void firebase_get_data(void)
{
    esp_http_client_config_t config = {
        .url = FIREBASE_PATH,
        .event_handler = firebase_https_event_handler,
        .cert_pem = firebase_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        xEventGroupSetBits(s_wifi_event_group, FIREBASE_HTTP_ERROR_BIT);
    }

    esp_http_client_cleanup(client);
}

/* Push data to Firebase */
void firebase_put_data(const char *json_string)
{
    esp_http_client_config_t config = {
        .url = FIREBASE_PATH,
        .method = HTTP_METHOD_PUT,
        .cert_pem = firebase_cert_pem_start,
        .event_handler = firebase_https_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_string, strlen(json_string));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP PUT Success, Status = %d", esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/* Private function prototypes -------------------------------------------------------------------*/

static void firebase_task(void *arg)
{

    ESP_LOGI(TAG, "Firebase task started");

    /* Chờ WiFi connect */
    xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);

    while (1)
    {
        if (gh_firebase.b_wifi_connected)
        {
            //ESP_LOGI(TAG, "Firebase task: GET data");
            firebase_get_data();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Firebase handle */
static esp_err_t firebase_https_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:

        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            if (fb_data_len + evt->data_len < MAX_JSON_BUFFER)
            {

            }
            else
            {
                ESP_LOGW(TAG, "Received data exceeds buffer size, truncating");
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
    {

    }
    break;
    default:
        break;
    }
    return ESP_OK;
}

/* Wifi handle */
static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            // .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            // .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            // .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

static esp_err_t set_dns_server(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE))
    {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

static void set_static_ip(esp_netif_t *netif)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(STATIC_IP_ADDR);
    ip.netmask.addr = ipaddr_addr(STATIC_NETMASK_ADDR);
    ip.gw.addr = ipaddr_addr(STATIC_GW_ADDR);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }
    ESP_LOGD(TAG, "Success to set static ip: %s, netmask: %s, gw: %s", STATIC_IP_ADDR, STATIC_NETMASK_ADDR, STATIC_GW_ADDR);
    ESP_ERROR_CHECK(set_dns_server(netif, ipaddr_addr(MAIN_DNS_SERVER), ESP_NETIF_DNS_MAIN));
    ESP_ERROR_CHECK(set_dns_server(netif, ipaddr_addr(BACKUP_DNS_SERVER), ESP_NETIF_DNS_BACKUP));
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    // else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    //     set_static_ip(arg);
    // }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        gh_firebase.b_wifi_connected = 0;
        if (s_retry_num < MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;

        gh_firebase.b_wifi_connected = 1;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        if (firebase_task_handle == NULL)
        {
            xTaskCreate(firebase_task,
                        "firebase_task",
                        4250, // TLS stack
                        NULL,
                        5,
                        &firebase_task_handle);
        }
    }
}

/* END OF FILE ************************************************************************************/
/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Mar-2026
 * @file            : wifi_prov.c
 * @dissaption      : Wi-Fi Provisioning Manager driver board support package.
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

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

/* Define ----------------------------------------------------------------------------------------*/

#define WIFI_CONNECTED_BIT BIT0

#define PROV_POP "mz12345678"
#define PROV_TRANSPORT "softap"
#define PROV_QR_VERSION "v1"

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "wifi_prov";
static EventGroupHandle_t s_wifi_event_group;
const char *wifi_service_name = "mzService";

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static void wifi_prov_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_prov_print_qr(const char *service_name, const char *pop);

static void wifi_init_sta(void);

static void wifi_prov_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/* Public function prototypes --------------------------------------------------------------------*/

esp_err_t wifi_prov_open(void)
{
    ESP_ERROR_CHECK(
        esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default());

    s_wifi_event_group =
        xEventGroupCreate();

    esp_netif_create_default_wifi_sta();

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(
            &cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_PROV_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_prov_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_prov_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_prov_event_handler,
            NULL));
    wifi_prov_mgr_config_t config =
    {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    return ESP_OK;
}

bool wifi_prov_is_provisioned(void)
{
    bool provisioned = false;

    ESP_ERROR_CHECK(
        wifi_prov_mgr_is_provisioned(
            &provisioned));

    return provisioned;
}

esp_err_t wifi_prov_start(void)
{
    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    wifi_prov_security1_params_t *sec_params = PROV_POP;

    ESP_ERROR_CHECK(
        wifi_prov_mgr_start_provisioning(
            security,
            sec_params,
            wifi_service_name,
            NULL));

    wifi_prov_print_qr(
        wifi_service_name,
        PROV_POP);

    ESP_LOGI(
        TAG,
        "Provisioning AP: %s",
        wifi_service_name);

    return ESP_OK;
}

esp_err_t wifi_prov_connect(void)
{
    wifi_prov_mgr_deinit();

    wifi_init_sta();

    return ESP_OK;
}

esp_err_t wifi_prov_reset(void)
{
    return wifi_prov_mgr_reset_provisioning();
}

void wifi_prov_wait_connected(void)
{
    xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdTRUE,
        pdTRUE,
        portMAX_DELAY);
}

/* Private function prototypes -------------------------------------------------------------------*/

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_STA));

    ESP_ERROR_CHECK(
        esp_wifi_start());
}

static void wifi_prov_print_qr(
    const char *service_name,
    const char *pop)
{
    char payload[200];

    snprintf(
        payload,
        sizeof(payload),
        "{\"ver\":\"%s\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
        PROV_QR_VERSION,
        service_name,
        pop,
        PROV_TRANSPORT);

    ESP_LOGI(TAG, "QR Payload:");
    ESP_LOGI(TAG, "%s", payload);
}

static void wifi_prov_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;

        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *cfg =
                (wifi_sta_config_t *)event_data;

            ESP_LOGI(
                TAG,
                "SSID: %s",
                (char *)cfg->ssid);

            break;
        }

        case WIFI_PROV_CRED_FAIL:
            ESP_LOGE(
                TAG,
                "Provisioning failed");
            break;

        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(
                TAG,
                "Provisioning success");
            break;

        case WIFI_PROV_END:
            wifi_prov_mgr_deinit();
            break;

        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(
                TAG,
                "Reconnect WiFi");

            esp_wifi_connect();
            break;

        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(
                TAG,
                "Phone connected");
            break;

        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(
                TAG,
                "Phone disconnected");
            break;

        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            xEventGroupSetBits(
                s_wifi_event_group,
                WIFI_CONNECTED_BIT);

            ESP_LOGI(
                TAG,
                "WiFi connected");
        }
    }
}
/* END OF FILE ************************************************************************************/
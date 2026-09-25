/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Mar-2026
 * @file            : smart_config.c
 * @dissaption      : smart config driver board support package.
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
#include "esp_smartconfig.h"
#include "esp_wifi.h"

/* Define ----------------------------------------------------------------------------------------*/

#define SMART_CONFIG_CONNECTED_BIT BIT0
#define SMART_CONFIG_DONE_BIT BIT1

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "SMART_CONFIG";
static EventGroupHandle_t s_wifi_event_group;

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static void smart_config_task(void *pv_parameters);
static void smart_config_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/* Public function prototypes --------------------------------------------------------------------*/

void smart_config_open(void)
{
    esp_netif_t *sta_netif = NULL;

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif != NULL);

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            smart_config_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            smart_config_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            SC_EVENT,
            ESP_EVENT_ANY_ID,
            smart_config_event_handler,
            NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}
/* Private function prototypes -------------------------------------------------------------------*/

static void smart_config_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START))
    {
        xTaskCreate(
            smart_config_task,
            "smart_config_task",
            4096,
            NULL,
            3,
            NULL);
    }
    else if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED))
    {
        esp_wifi_connect();

        xEventGroupClearBits(
            s_wifi_event_group,
            SMART_CONFIG_CONNECTED_BIT);
    }
    else if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP))
    {
        ESP_LOGI(TAG, "WiFi connected");

        xEventGroupSetBits(
            s_wifi_event_group,
            SMART_CONFIG_CONNECTED_BIT);
    }
    else if ((event_base == SC_EVENT) && (event_id == SC_EVENT_SCAN_DONE))
    {
        ESP_LOGI(TAG, "Scan done");
    }
    else if ((event_base == SC_EVENT) && (event_id == SC_EVENT_FOUND_CHANNEL))
    {
        ESP_LOGI(TAG, "Channel found");
    }
    else if ((event_base == SC_EVENT) && (event_id == SC_EVENT_GOT_SSID_PSWD))
    {
        smartconfig_event_got_ssid_pswd_t *evt =
            (smartconfig_event_got_ssid_pswd_t *)event_data;

        wifi_config_t wifi_cfg = {0};

        memcpy(
            wifi_cfg.sta.ssid,
            evt->ssid,
            sizeof(wifi_cfg.sta.ssid));

        memcpy(
            wifi_cfg.sta.password,
            evt->password,
            sizeof(wifi_cfg.sta.password));

        ESP_LOGI(TAG, "SSID     : %s", evt->ssid);
        ESP_LOGI(TAG, "PASSWORD : %s", evt->password);

        ESP_ERROR_CHECK(esp_wifi_disconnect());

        ESP_ERROR_CHECK(
            esp_wifi_set_config(
                WIFI_IF_STA,
                &wifi_cfg));

        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if ((event_base == SC_EVENT) && (event_id == SC_EVENT_SEND_ACK_DONE))
    {
        xEventGroupSetBits(
            s_wifi_event_group,
            SMART_CONFIG_DONE_BIT);
    }
}

static void smart_config_task(void *pv_parameters)
{
    EventBits_t bits;

    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));

    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

    while (true)
    {
        bits = xEventGroupWaitBits(
            s_wifi_event_group,
            SMART_CONFIG_CONNECTED_BIT |
                SMART_CONFIG_DONE_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        if (bits & SMART_CONFIG_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "Connected to AP");
        }

        if (bits & SMART_CONFIG_DONE_BIT)
        {
            ESP_LOGI(TAG, "SmartConfig finished");

            esp_smartconfig_stop();

            vTaskDelete(NULL);
        }
    }
}

/* END OF FILE ************************************************************************************/
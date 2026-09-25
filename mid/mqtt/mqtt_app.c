/***************************************************************************************************************
 * @auth            : duongdh
 * @day             : 4-Apr-2025
 * @file            : mqtt.c
 * @dissaption      : mqtt component create FW-Core
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

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "mqtt_client.h"

#include "mqtt_app.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;
static mqtt_rx_callback_t s_rx_callback = NULL;

/* Lưu trữ Client ID và Topic riêng cho từng thiết bị */
static char s_client_id[32] = "05092026A3";
static char s_sub_topic[64] = {0};

// extern const uint8_t emqx_ca_crt_start[] asm("_binary_emqx_ca_crt_start");
// extern const uint8_t emqx_ca_crt_end[]   asm("_binary_emqx_ca_crt_end");
extern const uint8_t device_crt_start[] asm("_binary_device_crt_start");
extern const uint8_t device_crt_end[]   asm("_binary_device_crt_end");

extern const uint8_t device_key_start[] asm("_binary_device_key_start");
extern const uint8_t device_key_end[]   asm("_binary_device_key_end");

extern const uint8_t rootca_pem_start[] asm("_binary_rootca_pem_start");
extern const uint8_t rootca_pem_end[]   asm("_binary_rootca_pem_end");

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_generate_dynamic_id(void);

/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief  Automatically generate Client ID and Topic to receive commands based on the hardware's MAC address.
 */
static void mqtt_generate_dynamic_id(void)
{
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, mac);


    /* Định dạng Topic nhận lệnh riêng: fan/FAN_XXYYZZ */
    snprintf(s_sub_topic, sizeof(s_sub_topic), "v1/devices/me/rpc/request/+");

    ESP_LOGI(TAG, "Device Client ID : %s", s_client_id);
    ESP_LOGI(TAG, "Device Sub Topic : %s", s_sub_topic);
}

void mqtt_app_start(void)
{
    /* Automatically generate a unique ID before connecting to the Broker. */
    mqtt_generate_dynamic_id();

    esp_mqtt_client_config_t mqtt_cfg =
    {
    //     .broker.address.uri = "mqtts://se66ec11.ala.us-east-1.emqxsl.com:8883",
    //     .broker.verification.certificate = (const char *)emqx_ca_crt_start,
    //     .credentials.username = "emqx_online_test_1b1ae11f",
    //     .credentials.authentication.password = "ce35>4dHc59C4393N_#Zf2?8b9fG045a",
    //     .credentials.client_id = s_client_id, /* Use dynamic Client ID */
        .broker.address.uri = "mqtts://mqtt.dev.happibot.net:8883",
        /* Server certificate */
        .broker.verification.certificate = (const char *)rootca_pem_start,

        /* Device identity */
        .credentials.authentication.certificate = (const char *)device_crt_start,

        .credentials.authentication.key = (const char *)device_key_start,
        .credentials.client_id = s_client_id,
    };

    ESP_LOGI(TAG, "Root CA length=%d",
                (int)(rootca_pem_end - rootca_pem_start));

    ESP_LOGI(TAG, "Client cert length=%d",
                (int)(device_crt_end - device_crt_start));


    s_client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        s_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL);

    esp_mqtt_client_start(s_client);

    ESP_LOGI(TAG, "MQTT START");
}

bool mqtt_app_is_connected(void)
{
    return s_connected;
}

bool mqtt_app_publish(
    const char *topic,
    const char *payload,
    int qos,
    bool retain)
{
    if (!s_connected)
    {
        return false;
    }

    int msg_id = esp_mqtt_client_publish(
            s_client,
            topic,
            payload,
            0,
            qos,
            retain);

    return (msg_id >= 0);
}

int mqtt_app_subscribe(
    const char *topic,
    int qos)
{
    if (!s_connected)
    {
        return -1;
    }

    return esp_mqtt_client_subscribe(
        s_client,
        topic,
        qos);
}

void mqtt_app_register_callback(mqtt_rx_callback_t callback)
{
    s_rx_callback = callback;
}

const char* mqtt_app_get_client_id(void)
{
    return s_client_id;
}

/* Private function prototypes -------------------------------------------------------------------*/

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:

        ESP_LOGI(TAG, "CONNECTED");
        s_connected = true;

        mqtt_app_subscribe(s_sub_topic, 1);

        break;

    case MQTT_EVENT_DISCONNECTED:

        ESP_LOGW(TAG, "DISCONNECTED");
        s_connected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:

        ESP_LOGI(TAG, "SUBSCRIBED msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:

        ESP_LOGI(TAG, "UNSUBSCRIBED msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "PUBLISHED msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
    {
        char topic[128] = {0};
        char payload[512] = {0};

        int topic_len = event->topic_len < sizeof(topic) - 1
                        ? event->topic_len
                        : sizeof(topic) - 1;

        int data_len = event->data_len < sizeof(payload) - 1
                        ? event->data_len
                        : sizeof(payload) - 1;

        memcpy(topic, event->topic, topic_len);
        memcpy(payload, event->data, data_len);

        if (s_rx_callback != NULL)
        {
            s_rx_callback(topic, payload);
        }

        break;
    }

    case MQTT_EVENT_ERROR:

        ESP_LOGE(TAG, "ERROR");
        break;

    default:
        break;
    }
}

/* END OF FILE ************************************************************************************/
/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Mar-2026
 * @file            : zapp.c
 * @dissaption      : zapp module.
 * auther           : duonghd
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
#include "esp_ota_ops.h"
#include "ble_ota.h"
/** Middleware */
#include "wifi_prov.h"
#include "mqtt_app.h"
#include "esp_at.h"
#include "ztime.h"
#include "zschedule.h"
#include "fan_send.h"

/* Json */
#include "cJSON.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

typedef struct
{
    fan_realtime_t fan_data;
} sys_t;

static const char *TAG = "ZAPP";

static sys_t gh_sys = { 0 };

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

// static void esp_at_receive(const uint8_t *data, uint16_t length);
// static void mqtt_app_receive(const char *topic, const char *payload);

static const char *fan_mode_to_string(fan_mode_t mode);
static const char *fan_state_to_string(fan_state_t state);
static fan_mode_t fan_mode_from_string(const char *mode);
static fan_state_t fan_state_from_string(const char *state);

static bool fan_realtime_parse(const char *json_string, fan_realtime_t *fan);
static char *fan_realtime_to_json(const fan_realtime_t *fan);



static void print_running_partition(void)
{
    const esp_partition_t *partition = esp_ota_get_running_partition();

    if (partition == NULL) {
        ESP_LOGE("OTA", "Cannot get running partition");
        return;
    }

    ESP_LOGI("OTA", "Running partition:");
    ESP_LOGI("OTA", "  Label : %s", partition->label);
    ESP_LOGI("OTA", "  Type  : %d", partition->type);
    ESP_LOGI("OTA", "  SubType: %d", partition->subtype);
    ESP_LOGI("OTA", "  Offset: 0x%lx", partition->address);
    ESP_LOGI("OTA", "  Size  : 0x%lx", partition->size);
}
/**
 * @brief      Open zapp.
 * @param[in]  type: type to be open.
 * @retval     None.
 */
static void zapp_task(void *p_para)
{
    print_running_partition();
    //ESP_ERROR_CHECK(ble_ota_start());
    // smart_config_open();
    // check wifi/pass -> reset

    ESP_ERROR_CHECK(wifi_prov_open());
    //wifi_prov_reset();
    bool provisioned = wifi_prov_is_provisioned();

    ESP_LOGI(TAG,"provisioned=%d",provisioned);

    if (!provisioned)
    {
        ESP_LOGI(TAG, "START PROVISION");
        wifi_prov_start();
    }
    else
    {
        ESP_LOGI(TAG, "CONNECT WIFI");
        wifi_prov_connect();
    }

    /* Setting wifi with Apliction */
    wifi_prov_wait_connected();

    /* Chờ thêm vài giây để DNS ổn định */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Khởi động module thời gian */
    ztime_open();
    fan_send_open();
    /** MQTT open and start */
    mqtt_app_start();
    mqtt_app_register_callback(mqtt_app_receive);


    /** ESP Communication with UART Protocol */
    esp_at_open();
    esp_at_register_rx_callback(esp_at_receive);

    esp_at_send_command("AT");
    esp_at_send_command("AT+GMR");
    esp_at_send_command("AT+CWMODE?");

    uint32_t counter = 0;

    while (true)
    {
        if (mqtt_app_is_connected())
        {
            // char payload[64];
            // snprintf(payload, sizeof(payload), "duonghd %lu", (unsigned long)counter++);
            // mqtt_app_publish("fanSB350/status", payload, 1, false);
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief      Open task.
 * @param[in]  type: type to be configured.
 * @retval     None.
 */
void zapp_open(void)
{

    xTaskCreate(
        zapp_task,             /* Function that implements the task. */
        "zapp task",           /* Text name for the task. */
        ZAPP_STACK_DEEP_START, /* Stack deep in words, NOT bytes. */
        NULL,                  /* Parameter passed into the task. */
        ZAPP_PRIO_START,       /* Priority at which the task is created. */
        NULL                   /* Used to pass out the created task's handle.*/
    );
}

/* Private function prototypes -------------------------------------------------------------------*/


static const char *fan_mode_to_string(fan_mode_t mode)
{
    switch (mode)
    {
        case FAN_MODE_AUTO:
            return "auto";

        case FAN_MODE_MANUAL:
            return "manual";

        case FAN_MODE_SLEEP:
            return "sleep";

        default:
            return "auto";
    }
}

static const char *fan_state_to_string(fan_state_t state)
{
    switch (state)
    {
        case FAN_STATE_OFF:
            return "off";

        case FAN_STATE_ON:
            return "on";

        default:
            return "off";
    }
}

static fan_mode_t fan_mode_from_string(const char *mode)
{
    if (mode == NULL)
    {
        return FAN_MODE_AUTO;
    }

    if (strcmp(mode, "auto") == 0)
    {
        return FAN_MODE_AUTO;
    }

    if (strcmp(mode, "manual") == 0)
    {
        return FAN_MODE_MANUAL;
    }

    if (strcmp(mode, "sleep") == 0)
    {
        return FAN_MODE_SLEEP;
    }

    return FAN_MODE_AUTO;
}

static fan_state_t fan_state_from_string(const char *state)
{
    if (state == NULL)
    {
        return FAN_STATE_OFF;
    }

    if (strcmp(state, "on") == 0)
    {
        return FAN_STATE_ON;
    }

    return FAN_STATE_OFF;
}

// static bool fan_realtime_parse(const char *json_string, fan_realtime_t *fan)
// {
//     if ((json_string == NULL) ||
//         (fan == NULL))
//     {
//         return false;
//     }

//     cJSON *root = cJSON_Parse(json_string);

//     if (root == NULL)
//     {
//         return false;
//     }

//     cJSON *hum = cJSON_GetObjectItem(root, "hum");
//     cJSON *temp = cJSON_GetObjectItem(root, "temp");
//     cJSON *mode = cJSON_GetObjectItem(root, "mode");
//     cJSON *speed = cJSON_GetObjectItem(root, "speed");
//     cJSON *state = cJSON_GetObjectItem(root, "state");
//     cJSON *timer = cJSON_GetObjectItem(root, "timer");

//     if (!cJSON_IsNumber(hum) ||
//         !cJSON_IsNumber(temp) ||
//         !cJSON_IsString(mode) ||
//         !cJSON_IsNumber(speed) ||
//         !cJSON_IsString(state) ||
//         !cJSON_IsNumber(timer))
//     {
//         cJSON_Delete(root);
//         return false;
//     }

//     fan->hum = (uint8_t)hum->valueint;
//     fan->temp = (uint8_t)temp->valueint;

//     fan->speed = (uint8_t)speed->valueint;
//     fan->timer = (uint32_t)timer->valueint;

//     fan->mode = fan_mode_from_string(
//         mode->valuestring);

//     fan->state = fan_state_from_string(
//         state->valuestring);

//     cJSON_Delete(root);

//     return true;
// }

static char *fan_realtime_to_json(const fan_realtime_t *fan)
{
    if (fan == NULL)
    {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();

    if (root == NULL)
    {
        return NULL;
    }

    cJSON_AddNumberToObject(
        root,
        "hum",
        fan->hum);

    cJSON_AddNumberToObject(
        root,
        "temp",
        fan->temp);

    cJSON_AddStringToObject(
        root,
        "mode",
        fan_mode_to_string(fan->mode));

    cJSON_AddNumberToObject(
        root,
        "speed",
        fan->speed);

    cJSON_AddStringToObject(
        root,
        "power",
        fan_state_to_string(fan->state));
    
    cJSON_AddStringToObject(
        root,
        "swing",
        fan_state_to_string(fan->state));    
        
        
    cJSON_AddStringToObject(
        root,
        "sleep",
        fan_state_to_string(fan->state));

    char *json = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    return json;
}



/* END OF FILE ************************************************************************************/

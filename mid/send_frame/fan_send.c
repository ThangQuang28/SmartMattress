/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : aht20.c                                                                           
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 character
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"
/* Middleware */
#include "mqtt_app.h"
#include "esp_at.h"
#include "fan_send.h"
#include "ztime.h"
#include "zschedule.h"
/* BSP */
#include "esp_log.h"

/* Json */
#include "cJSON.h"
/* Define ----------------------------------------------------------------------------------------*/
#define STATUS_IDLE_TIMEOUT_MS      1000
#define MAX_CMD_DATA_LEN            8
/* Private data types ----------------------------------------------------------------------------*/
static const char *TAG = "FAN_SEND";
/* Public data types -----------------------------------------------------------------------------*/
typedef struct
{
    bool power;
    uint8_t total_speed;
    uint8_t speed;
    bool sleep;
    bool light;
    bool swing;
    bool buzzer;

    /* Các cờ đánh dấu (Flags) xem có cần đưa vào JSON không */
    bool update_power;
    bool update_total_speed;
    bool update_speed;
    bool update_sleep;
    bool update_light;
    bool update_swing;
    bool update_buzzer;
    bool update_temp;
    bool update_humi;
} fan_status_t;

typedef struct {
    uint8_t cmd;
    uint8_t data[MAX_CMD_DATA_LEN];
    uint8_t len;
} mcu_cmd_t;

static fan_status_t g_fan_state = {0};;
static bool gh_need_publish = false;
static TickType_t gh_last_update = 0;

/* Biến Hàng đợi */
static QueueHandle_t gh_mcu_cmd_queue = NULL;

/* Private function prototypes -------------------------------------------------------------------*/

static void publish_task(void *arg);

/* Public function prototypes --------------------------------------------------------------------*/
static void queue_mcu_command(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    if (gh_mcu_cmd_queue == NULL) return;

    mcu_cmd_t new_cmd;
    new_cmd.cmd = cmd;
    new_cmd.len = len > MAX_CMD_DATA_LEN ? MAX_CMD_DATA_LEN : len;
    memcpy(new_cmd.data, data, new_cmd.len);

    /* Đẩy vào Queue, chờ tối đa 100ms nếu Queue đang đầy */
    if (xQueueSend(gh_mcu_cmd_queue, &new_cmd, pdMS_TO_TICKS(100)) != pdPASS)
    {
        ESP_LOGE(TAG, "MCU Command Queue is FULL! Dropped cmd: 0x%02X", cmd);
    }
}
/* Task lấy lệnh từ Hàng đợi và gửi xuống MCU lần lượt */
static void mcu_send_task(void *arg)
{
    mcu_cmd_t rx_cmd;
    while (1)
    {
        if (xQueueReceive(gh_mcu_cmd_queue, &rx_cmd, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "=> QUEUE POP: Send to MCU cmd=0x%02X", rx_cmd.cmd);
            
            fan_send_frame(rx_cmd.cmd, rx_cmd.data, rx_cmd.len);
            
            /* Tăng độ trễ lên 300ms nếu MCU của bạn tiếp nhận chậm */
            vTaskDelay(pdMS_TO_TICKS(2000)); 
        }
    }
}
static void fan_publish_state(void)
{
if (!mqtt_app_is_connected())
    {
        return;
    }
    
    cJSON *root = cJSON_CreateObject();
    bool has_data = false; // Biến kiểm tra xem có trường dữ liệu nào được add không

    /* Chỉ đưa vào JSON những thông số được MCU gửi lên (có cờ update = true) */
    if (g_fan_state.update_power) {
        cJSON_AddBoolToObject(root, "power", g_fan_state.power);
        g_fan_state.update_power = false; // Xóa cờ sau khi xử lý
        has_data = true;
    }
    if (g_fan_state.update_total_speed) {
        cJSON_AddNumberToObject(root, "total_speed", g_fan_state.total_speed);
        g_fan_state.update_total_speed = false;
        has_data = true;
    }
    if (g_fan_state.update_speed) {
        cJSON_AddNumberToObject(root, "speed", g_fan_state.speed);
        g_fan_state.update_speed = false;
        has_data = true;
    }
    if (g_fan_state.update_sleep) {
        cJSON_AddBoolToObject(root, "sleep", g_fan_state.sleep);
        g_fan_state.update_sleep = false;
        has_data = true;
    }
    if (g_fan_state.update_light) {
        cJSON_AddBoolToObject(root, "light", g_fan_state.light);
        g_fan_state.update_light = false;
        has_data = true;
    }
    if (g_fan_state.update_swing) {
        cJSON_AddBoolToObject(root, "oscillate", g_fan_state.swing);
        g_fan_state.update_swing = false;
        has_data = true;
    }
    if (g_fan_state.update_buzzer) {
        cJSON_AddBoolToObject(root, "buzzer", g_fan_state.buzzer);
        g_fan_state.update_buzzer = false;
        has_data = true;
    }

    /* Nếu có dữ liệu thì mới publish */
    if (has_data)
    {
        char *json = cJSON_PrintUnformatted(root);
        mqtt_app_publish(
            "v1/devices/me/telemetry",
            json,
            1,
            false);
        ESP_LOGI(TAG, "Publish aggregated: %s", json);
        cJSON_free(json);
    }
    
    cJSON_Delete(root);
}
static void status_changed(void)
{
    gh_need_publish = true;
    gh_last_update = xTaskGetTickCount();
}

void fan_send_open(void)
{
    /* Khởi tạo hàng đợi chứa tối đa 10 lệnh */
    gh_mcu_cmd_queue = xQueueCreate(10, sizeof(mcu_cmd_t));

    xTaskCreate(
            publish_task,
            "publish_task",
            4096,
            NULL,
            5,
            NULL);
    /* Task mới lấy dữ liệu từ hàng đợi gửi xuống MCU */
    xTaskCreate(mcu_send_task, "mcu_send_task", 3072, NULL, 6, NULL);
}

void fan_send_frame(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t frame[256];
    uint16_t index = 0;

    frame[index++] = 0xAA;
    frame[index++] = cmd;
    frame[index++] = len;

    memcpy(&frame[index],
           data,
           len);

    index += len;

    frame[index++] = 0xBB;

    // ESP_LOG_BUFFER_HEX(
    //     TAG,
    //     frame,
    //     index);

    esp_at_send(
        frame,
        index);
}

void esp_at_receive(const uint8_t *data, uint16_t length)
{
    ESP_LOG_BUFFER_HEX(TAG, data, length);
    
    uint16_t i = 0;
    
    /* Quét toàn bộ buffer để tìm tất cả các frame */
    while (i < length)
    {
        /* Tìm byte Header 0xAB */
        if (data[i] == 0xAB)
        {
            /* Kiểm tra xem chuỗi còn đủ dài cho 1 frame tối thiểu không (AB + cmd + len + BB = 4) */
            if (i + 3 >= length) {
                break; // Hết buffer
            }

            uint8_t cmd = data[i + 1];
            uint8_t payload_len = data[i + 2];

            /* Kiểm tra byte Tail 0xBB có nằm đúng vị trí kết thúc không */
            if (i + 3 + payload_len < length && data[i + 3 + payload_len] == 0xBB)
            {
                /* Lấy data (chỉ lấy 1 byte đầu tiên theo logic cũ của bạn) */
                uint8_t val = data[i + 3]; 

                switch (cmd)
                {
                    case 0x01:
                        g_fan_state.power = val;
                        g_fan_state.update_power = true;
                        status_changed();
                        break;

                    case 0x02:
                        g_fan_state.total_speed = val;
                        g_fan_state.update_total_speed = true;
                        status_changed();
                        break;

                    case 0x03:
                        g_fan_state.speed = val;
                        g_fan_state.update_speed = true;
                        status_changed();
                        break;

                    case 0x04:
                        g_fan_state.sleep = val;
                        g_fan_state.update_sleep = true;
                        status_changed();
                        break;

                    case 0x62:
                        g_fan_state.light = val;
                        g_fan_state.update_light = true;
                        status_changed();
                        break;

                    case 0x63:
                        g_fan_state.swing = val;
                        g_fan_state.update_swing = true;
                        status_changed();
                        break;

                    case 0x64:
                        g_fan_state.buzzer = val;
                        g_fan_state.update_buzzer = true;
                        status_changed();
                        break;

                    case 0x11:
                    {
                        ESP_LOGI(TAG, "TEMP=%u", val);
                        cJSON *root = cJSON_CreateObject();
                        cJSON_AddNumberToObject(root, "temp", val);
                        char *json = cJSON_PrintUnformatted(root);
                        mqtt_app_publish("v1/devices/me/telemetry", json, 1, false);
                        cJSON_free(json);
                        cJSON_Delete(root);
                        break;
                    }
                            
                    case 0x12:
                    {
                        ESP_LOGI(TAG, "HUMI=%u", val);
                        cJSON *root = cJSON_CreateObject();
                        cJSON_AddNumberToObject(root, "humi", val);
                        char *json = cJSON_PrintUnformatted(root);
                        mqtt_app_publish("v1/devices/me/telemetry", json, 1, false);
                        cJSON_free(json);
                        cJSON_Delete(root);
                        break;
                    }
                }

                /* Xử lý xong 1 frame, cộng dồn index để nhảy qua frame tiếp theo */
                i += (payload_len + 4);
            }
            else
            {
                /* Khung bị lỗi hoặc MCU gửi thiếu, nhích 1 byte để dò lại */
                i++;
            }
        }
        else
        {
            i++; /* Dò tìm byte 0xAB */
        }
    }
}

void mqtt_app_receive(const char *topic, const char *payload)
{
    ESP_LOGI(TAG,
             "MQTT RX: Topic=%s Payload=%s",
             topic,
             payload);

    cJSON *root = cJSON_Parse(payload);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "JSON Parse Failed");
        return;
    }

    cJSON *command = cJSON_GetObjectItem(root, "method");
    cJSON *value = cJSON_GetObjectItem(root, "params");

    if (!cJSON_IsString(command))
    {
        cJSON_Delete(root);
        return;
    }

    /* OTA: {"method":"ota","params":{"url":"https://host/fw.bin"}} */
    if (strcmp(command->valuestring, "ota") == 0)
    {
        cJSON *url = cJSON_IsObject(value) ? cJSON_GetObjectItem(value, "url") : value;

        if (!cJSON_IsString(url))
        {
            ESP_LOGW(TAG, "OTA request rejected");
        }

        cJSON_Delete(root);
        return;
    }

    /* REALTIME SCHEDULE */
    if (strcmp(command->valuestring, "realtime") == 0)
    {
        /* Bắt đầu nhận danh sách lịch */
        if (cJSON_IsBool(value))
        {
            if (cJSON_IsTrue(value))
            {
                ESP_LOGI(TAG,
                         "Realtime START -> clear schedule");

                zschedule_clear();
            }
            else
            {
                ESP_LOGI(TAG,
                         "Realtime END");
            }
        }
        /* Nhận từng phần tử lịch */
        else if (cJSON_IsObject(value))
        {
            cJSON *index_item =
                cJSON_GetObjectItem(value, "index");

            cJSON *start_item =
                cJSON_GetObjectItem(value, "start");

            cJSON *speed_item =
                cJSON_GetObjectItem(value, "speed");

            if (cJSON_IsNumber(index_item) &&
                cJSON_IsString(start_item) &&
                cJSON_IsNumber(speed_item))
            {
                uint8_t hour;
                uint8_t minute;

                if (sscanf(start_item->valuestring,
                           "%hhu:%hhu",
                           &hour,
                           &minute) == 2)
                {
                    zschedule_set(
                        (uint8_t)index_item->valueint,
                        hour,
                        minute,
                        (uint8_t)speed_item->valueint);

                    ESP_LOGI(
                        TAG,
                        "Schedule[%d] %02d:%02d Speed=%d",
                        index_item->valueint,
                        hour,
                        minute,
                        speed_item->valueint);
                }
            }
        }

        cJSON_Delete(root);
        return;
    }

    /* POWER */
    else if (strcmp(command->valuestring, "setPower") == 0)
    {
        cJSON *power = cJSON_GetObjectItem(value, "power");
        if (cJSON_IsBool(power))
        {
            uint8_t data[1];

            data[0] = cJSON_IsTrue(power) ? 0x01 : 0x00;
            ESP_LOGI(TAG,
                        "CMD FAN POWER=%s",
                            cJSON_IsTrue(power) ? "on" : "off");

            queue_mcu_command(
                0x01,
                data,
                sizeof(data));
        }
        cJSON_Delete(root);
        return;
    }
     /* SPEED */
    else if (strcmp(command->valuestring, "setSpeed") == 0)
    {
        cJSON *speed = cJSON_GetObjectItem(value, "speed");
        if (cJSON_IsNumber(speed))
        {
            uint8_t data[1];

            data[0] = (uint8_t)speed->valueint;

            queue_mcu_command(
                0x03,
                data,
                sizeof(data));
        }

        cJSON_Delete(root);
        return;
    }
    /* SWING */
    else if (strcmp(command->valuestring, "setOscillate") == 0)
    {
        cJSON *swing = cJSON_GetObjectItem(value, "oscillate");
        if (cJSON_IsBool(swing))
        {
            uint8_t data[1];

            data[0] = cJSON_IsTrue(swing) ? 0x01 : 0x00;
            ESP_LOGI(TAG,
                        "CMD FAN SWING=%s",
                            cJSON_IsTrue(swing) ? "on" : "off");

            queue_mcu_command(
                0x63,
                data,
                sizeof(data));
        }
        cJSON_Delete(root);
        return;
    } 

    /* SLEEPY */
    else if (strcmp(command->valuestring, "setMode") == 0)
    {
        cJSON *mode = cJSON_GetObjectItem(value, "mode");

        if (!cJSON_IsString(mode))
        {
            ESP_LOGW(TAG, "Invalid mode parameter");
            cJSON_Delete(root);
            return;
        }

        uint8_t data[1];

        if (strcmp(mode->valuestring, "sleep") == 0)
        {
            data[0] = 0x01;
            queue_mcu_command(0x04, data, sizeof(data));
        }
        else if (strcmp(mode->valuestring, "natural") == 0)
        {
            data[0] = 0x01;
            queue_mcu_command(0x03, data, sizeof(data));
        }
        else if (strcmp(mode->valuestring, "turbo") == 0)
        {
            data[0] = 0x0C;
            queue_mcu_command(0x03, data, sizeof(data));

        }
        else if (strcmp(mode->valuestring, "normal") == 0)
        {
            data[0] = 0x06;
            queue_mcu_command(0x03, data, sizeof(data));

        }
        else
        {
            ESP_LOGW(TAG, "Unknown mode=%s", mode->valuestring);
            cJSON_Delete(root);
            return;
        }
        ESP_LOGI(TAG,
                    "CMD FAN MODE=%s -> 0x%02X",
                    mode->valuestring,
                    data[0]);
        cJSON_Delete(root);
        return;
    } 

    /* LIGHT */
    else if (strcmp(command->valuestring, "setLight") == 0)
    {
        cJSON *light = cJSON_GetObjectItem(value, "light");
        if (cJSON_IsBool(light))
        {
            uint8_t data[1];

            data[0] = cJSON_IsTrue(light) ? 0x01 : 0x00;
            ESP_LOGI(TAG,
                        "CMD FAN LIGHT=%s",
                            cJSON_IsTrue(light) ? "on" : "off");

            queue_mcu_command(
                0x62,
                data,
                sizeof(data));
        }
        cJSON_Delete(root);
        return;
    }

    /* BUZZER */
    else if (strcmp(command->valuestring, "setBuzzer") == 0)
    {
        cJSON *buzzer = cJSON_GetObjectItem(value, "buzzer");
        if (cJSON_IsBool(buzzer))
        {
            uint8_t data[1];

            data[0] = cJSON_IsTrue(buzzer) ? 0x01 : 0x00;
            ESP_LOGI(TAG,
                        "CMD FAN BUZZER=%s",
                            cJSON_IsTrue(buzzer) ? "on" : "off");

            queue_mcu_command(
                0x64,
                data,
                sizeof(data));
        }
        cJSON_Delete(root);
        return;
    }
     /* TIMER */
    else if (strcmp(command->valuestring, "setTimer") == 0)
    {
        cJSON *timerOff = cJSON_GetObjectItem(value, "timerOff");
        if (cJSON_IsNumber(timerOff))
        {
            uint32_t delay;

            delay = (uint32_t)timerOff->valueint;
            if(delay == 0)
            {
                fan_timer_stop();
                ESP_LOGI(TAG, "Timer stopped");
            } 
            else 
            {
                fan_timer_start(delay);
                ESP_LOGI(TAG, "Timer: %lu minute", delay);
            }

           
        }

        cJSON_Delete(root);
        return;
    }
    else if (strcmp(command->valuestring, "setfunctionId") == 0)
    {
        cJSON *functionId = cJSON_GetObjectItem(value, "functionId");
        
        /* Kiểm tra nếu params có chứa functionId và giá trị là "2" (dạng chuỗi) */
        if (cJSON_IsString(functionId) && strcmp(functionId->valuestring, "2") == 0)
        {
            ESP_LOGI(TAG, "CMD FUNCTION ID = 2 -> Publish test roomTemp");
            
            /* Tạo đối tượng JSON mới để publish */
            cJSON *pub_root = cJSON_CreateObject();
            
            /* Add nhiệt độ test (ví dụ số 27) */
            cJSON_AddNumberToObject(pub_root, "roomTemp", 27); 
            
            /* In ra chuỗi JSON và gửi đi */
            char *json = cJSON_PrintUnformatted(pub_root);
            if (json != NULL)
            {
                mqtt_app_publish(
                    "v1/devices/me/telemetry",
                    json,
                    1,
                    false);
                ESP_LOGI(TAG, "Published test: %s", json);
                cJSON_free(json); // Giải phóng RAM chuỗi
            }
            cJSON_Delete(pub_root); // Giải phóng RAM Object JSON
        }
        else if (cJSON_IsString(functionId) && strcmp(functionId->valuestring, "12") == 0)
        {
            ESP_LOGI(TAG, "CMD FUNCTION ID = 2 -> Publish test roomTemp");
            
            /* Tạo đối tượng JSON mới để publish */
            cJSON *pub_root = cJSON_CreateObject();
            
            /* Add nhiệt độ test (ví dụ số 20) */
            cJSON_AddNumberToObject(pub_root, "roomTemp", 20); 
            
            /* In ra chuỗi JSON và gửi đi */
            char *json = cJSON_PrintUnformatted(pub_root);
            if (json != NULL)
            {
                mqtt_app_publish(
                    "v1/devices/me/telemetry",
                    json,
                    1,
                    false);
                ESP_LOGI(TAG, "Published test: %s", json);
                cJSON_free(json); // Giải phóng RAM chuỗi
            }
            cJSON_Delete(pub_root); // Giải phóng RAM Object JSON
        }
        else
        {
            ESP_LOGW(TAG, "Invalid or missing functionId");
        }
        
        cJSON_Delete(root);
        return;
    }

    else
    {
        ESP_LOGW(TAG,
                "Unknown Command=%s",
                 command->valuestring);

    }

    cJSON_Delete(root);
}

static void publish_task(void *arg)
{
    while (1)
    {
        if (gh_need_publish)
        {
            TickType_t now = xTaskGetTickCount();

            /* Chờ timeout (mặc định 1000ms theo marco STATUS_IDLE_TIMEOUT_MS) */
            if ((now - gh_last_update) >= pdMS_TO_TICKS(STATUS_IDLE_TIMEOUT_MS))
            {
                fan_publish_state();

                gh_need_publish = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* END OF FILE ************************************************************************************/

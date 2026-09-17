#include "ota.h"

#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OTA_URL_MAX_LENGTH 256
#define OTA_TASK_STACK_SIZE 8192

static const char *TAG = "OTA";
static bool s_in_progress;
static char s_url[OTA_URL_MAX_LENGTH];

static void ota_task(void *arg)
{
    esp_http_client_config_t http_config = {
        .url = s_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    ESP_LOGI(TAG, "Downloading update from %s", s_url);
    esp_err_t err = esp_https_ota(&ota_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Update validated; restarting into the new firmware");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }

    ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
    s_in_progress = false;
    vTaskDelete(NULL);
}

bool ota_start(const char *url)
{
    if (url == NULL || strncmp(url, "https://", 8) != 0 ||
        strlen(url) >= sizeof(s_url)) {
        ESP_LOGE(TAG, "OTA URL must be a non-empty HTTPS URL shorter than %u bytes",
                 (unsigned int)sizeof(s_url));
        return false;
    }

    if (s_in_progress) {
        ESP_LOGW(TAG, "OTA request ignored: an update is already running");
        return false;
    }

    memcpy(s_url, url, strlen(url) + 1);
    s_in_progress = true;

    if (xTaskCreate(ota_task, "ota", OTA_TASK_STACK_SIZE, NULL,
                    tskIDLE_PRIORITY + 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Unable to create OTA task");
        s_in_progress = false;
        return false;
    }

    return true;
}

bool ota_is_in_progress(void)
{
    return s_in_progress;
}

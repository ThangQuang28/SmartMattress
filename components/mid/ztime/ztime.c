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
#include <time.h>

/* Middleware */
#include "zschedule.h"
#include "ztime.h"
#include "fan_send.h"

/* BSP */
#include "esp_log.h"
#include "esp_netif_sntp.h"
/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "TIME";

/* Public data types -----------------------------------------------------------------------------*/
typedef struct {
	uint8_t 		cmd;		
	uint8_t  		data;
	TimerHandle_t     	timer;
} ztimer_h_t;

static ztimer_h_t gh_ztimer = {0};
/* Private function prototypes -------------------------------------------------------------------*/
static SemaphoreHandle_t g_ntp_sync_sem;

void fan_timer_open(void);

static void countdown_timer_callback(TimerHandle_t xTimer);

/* Public function prototypes --------------------------------------------------------------------*/
static void countdown_timer_callback(TimerHandle_t xTimer)
{
    uint8_t val = 0x00;
    fan_send_frame(
        0x01,
        (const uint8_t *)&val,
        1);

    // ESP_LOGI(TAG,
    //          "Countdown timeout -> send cmd=0x%02X data=%d",
    //          gh_ztimer.cmd,
    //          gh_ztimer.data);
}

static void time_task(void *arg)
{
   // Danh sách các NTP server dự phòng
    const char* ntp_servers[] = {
        "time.google.com",
        "pool.ntp.org",
        "vn.pool.ntp.org"
    };
    const uint8_t NUM_SERVERS = sizeof(ntp_servers) / sizeof(ntp_servers[0]);
    const uint8_t MAX_RETRIES = 2; // Số lần thử tối đa cho mỗi server

    uint8_t current_server = 0;
    bool time_synced = false;

    while (!time_synced) {
        ESP_LOGI(TAG, "Initializing SNTP with server: %s", ntp_servers[current_server]);

        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(ntp_servers[current_server]);
        esp_netif_sntp_init(&config);

        // Thử đồng bộ tối đa MAX_RETRIES lần
        for (uint8_t retry = 1; retry <= MAX_RETRIES; retry++) {
            if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(5000)) == ESP_OK) {
                time_synced = true;
                break; // Đồng bộ thành công, thoát vòng lặp for
            }
            ESP_LOGW(TAG, "SNTP sync failed, retry %d/%d...", retry, MAX_RETRIES);
        }

        if (time_synced) {
            break; // Đã đồng bộ thành công, thoát vòng lặp while để đi tiếp
        }

        // Nếu hết số lần retry mà vẫn chưa lấy được time
        ESP_LOGE(TAG, "Failed to sync with %s. Deinitializing and switching server...", ntp_servers[current_server]);
        
        // Bắt buộc phải deinit trước khi khởi tạo lại với config hoặc server mới
        esp_netif_sntp_deinit(); 
        
        // Chuyển vòng tròn sang server tiếp theo trong danh sách
        current_server = (current_server + 1) % NUM_SERVERS; 
    }

    /* GMT+7 */
    setenv("TZ", "ICT-7", 1);
    tzset();

    ESP_LOGI(TAG, "Time synchronized");
    xSemaphoreGive(g_ntp_sync_sem);

    while (1)
    {
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);


        // ESP_LOGI(TAG,
        //          "%02d:%02d:%02d",
        //          timeinfo.tm_hour,
        //          timeinfo.tm_min,
        //          timeinfo.tm_sec);

        fan_schedule_t schedule;   
        for (uint8_t i = 0; i < MAX_SCHEDULE; i++)
        {
            if (!zschedule_get(i, &schedule))
            {
                ESP_LOGI(TAG, "Schedule[%d] invalid", i);
                continue;
            }

            if (!schedule.enable)
            {
                continue;
            }

            if ((!schedule.executed) &&
                (timeinfo.tm_hour == schedule.hour) &&
                (timeinfo.tm_min == schedule.minute))
            {
                uint8_t speed = schedule.speed;

                fan_send_frame(
                    0x03,
                    &speed,
                    sizeof(speed));

                ESP_LOGI(TAG,
                        "Schedule[%d] Send speed=%d",
                        i,
                        speed);

                zschedule_set_executed(
                    i,
                    true);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void ztime_open(void)
{
    g_ntp_sync_sem = xSemaphoreCreateBinary();

    if (g_ntp_sync_sem == NULL)
    {
        ESP_LOGE(TAG, "Failed to create NTP semaphore");
        return;
    }
    fan_timer_open();
    xTaskCreate(
            time_task,
            "time_task",
            ZAPP_STACK_ZTIME,
            NULL,
            ZAPP_PRIO_ZTIME,
            NULL);
    /* Chờ cho đến khi NTP sync thành công */
    xSemaphoreTake(g_ntp_sync_sem, portMAX_DELAY);

    ESP_LOGI(TAG, "ztime_open: NTP OKIEEEEEEEEEE");
}
void fan_timer_open(void)
{
    gh_ztimer.timer = xTimerCreate(
        "fan_timer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        NULL,
        countdown_timer_callback);

    if (0 == gh_ztimer.timer)
    {
        ESP_LOGE(TAG, "Create timer failed");
    }
}

void fan_timer_start(uint32_t delay)
{
    if (gh_ztimer.timer == NULL)
    {
        return;
    }

    // gh_ztimer.cmd = cmd;
    // gh_ztimer.data = data;

    if (xTimerStop(gh_ztimer.timer, 0) != pdPASS)
    {
        ESP_LOGW(TAG, "Stop timer failed");
    }

    if (xTimerChangePeriod(
            gh_ztimer.timer,
            pdMS_TO_TICKS(delay * 60UL * 1000UL),
            0) != pdPASS)
    {
        ESP_LOGW(TAG, "Change period failed");
    }

    if (xTimerStart(
            gh_ztimer.timer,
            0) != pdPASS)
    {
        ESP_LOGW(TAG, "Start timer failed");
    }
}

void fan_timer_stop(void)
{
    if (gh_ztimer.timer != NULL)
    {
        xTimerStop(
            gh_ztimer.timer,
            0);
    }
}
/* END OF FILE ************************************************************************************/
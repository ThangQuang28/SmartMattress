/***************************************************************************************************************
 * @auth            : duonghd                                                                            
 * @day             : 10-Mar-2026
 * @file            : zapp.c                                                                          
 * @dissaption      :   ESPNOW to send and receive data. The device can send
 *                      broadcast ESPNOW data or unicast ESPNOW data.
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

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/* Midware */
#include "espnow.h"
#include "package.h"
#include "dc_motor.h"

/* ESP-IDF drivers */
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

/* Include ---------------------------------------------------------------------------------------*/
/* Define ----------------------------------------------------------------------------------------*/

static const char *TAG = "ESPNOW";

//uint8_t peer_mac[6] = {0x20,0xE7,0xC8,0x67,0x8A,0x90}; // 20:e7:c8:67:8a:90
uint8_t peer_mac[6] = {0x88,0x13,0xBF,0x6C,0x77,0xCC}; // 20:e7:c8:67:8a:90

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static void espnow_send_cb(const uint8_t *mac, esp_now_send_status_t status);
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len);

static void espnow_task(void *pvParameter);

/* Public function prototypes --------------------------------------------------------------------*/

esp_err_t espnow_open(void)
{
    esp_err_t ret = ESP_OK;

    /* Create a task to handle ESP-NOW communication */
    xTaskCreate(
        espnow_task, 
        "espnow_task", 
        ZAPP_STACK_ESPNOW, 
        NULL,
        ZAPP_PRIO_ESPNOW,
        NULL);

    return ret;
}

/* ESPNOW Send */
static void espnow_send_cb(const uint8_t *mac, esp_now_send_status_t status)
{
    ESP_LOGI(TAG, "Send status: %d", status);
}

/* ESPNOW Receive */
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    /* Print RAW DATA */
    ESP_LOGI(TAG, "RAW LEN: %d", len);

    printf("RAW: ");
    for(int i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");

    /* Parse package */
    pkg_t pkt;

    int err = pkg_parse(&pkt, (uint8_t*)data, len);
    if(err != 0)
    {
        ESP_LOGE(TAG, "PKG ERROR, Code : %d", err);
        return;
    }

    pkg_print(&pkt);

    if(pkt.cmd == 0x01)
    {
        pkg_err_t ret = PKG_ERR_OK;

        ret = dc_motor_handle(&pkt);

        uint8_t buf[32];
        
        char *msg = (ret = PKG_ERR_OK) ? "OK" : "ERR";

        int size = pkg_build(buf, 0x02, (uint8_t*)msg, strlen(msg));

        //pkg_dump_buf(TAG, buf, size);

        esp_now_send(info->src_addr, buf, size);
    }
}

/* Private function definitions ------------------------------------------------------------------*/
static void espnow_task(void *pvParameter)
{
    /* Wifi init */
    esp_netif_init();
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();


    /* ESPNOW Init */
    esp_now_init();

    esp_now_register_send_cb(espnow_send_cb);
    esp_now_register_recv_cb(espnow_recv_cb);

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, peer_mac, 6);
    peer.channel = 0;
    peer.encrypt = false;

    esp_now_add_peer(&peer);

    uint8_t buf[64];
    char msg[] = "duonghd";
    int size = pkg_build(buf, 0x01, (uint8_t *)msg, strlen(msg));

    pkg_dump_buf(TAG, buf, size);

    for (;;) {
        //esp_now_send(peer_mac, buf, size);
        // ESP_LOGI("ESP-NOW", "ESP-NOW task is running...\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/* END OF FILE ************************************************************************************/
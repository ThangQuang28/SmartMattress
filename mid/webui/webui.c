/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : webui.c
 * @description     : WiFi AP + STA and the HTTP server for the web app.
 *                    GET  /            -> web app (smart_mattress_chart/index.html, embedded)
 *                    GET  /api/state   -> JSON state (pressures, offsets, valves, WiFi)
 *                    POST /api/cmd     -> JSON command, see devctl_command()
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "mdns.h"

#include "webui.h"
#include "devctl.h"
#include "captive_dns.h"

/* Define ----------------------------------------------------------------------------------------*/

#define WEBUI_STA_RETRY_MAX         3U      /* Quick retries before falling back to the AP. */
#define WEBUI_STA_RETRY_WAIT_US     (60LL * 1000000LL)  /* Then try home WiFi again every 60 s. */
#define WEBUI_AP_OFF_DELAY_US       (60LL * 1000000LL)  /* AP stays 60 s after home WiFi is up. */
#define WEBUI_CMD_MAX               256U

/* Private variables -----------------------------------------------------------------------------*/

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

static httpd_handle_t g_server = NULL;
static esp_netif_t *gp_netif_sta = NULL;
static char g_ap_ssid[33];
static char g_sta_ssid[33];
static esp_ip4_addr_t g_sta_ip;
static bool g_sta_connected = false;
static uint8_t g_sta_retry = 0U;
static volatile bool g_ap_on = false;
static esp_timer_handle_t g_retry_timer = NULL;
static esp_timer_handle_t g_ap_off_timer = NULL;

static const char *TAG = "WEBUI";

/* Private function prototypes -------------------------------------------------------------------*/

static void webui_event_handler(void *p_arg, esp_event_base_t base, int32_t id, void *p_data);
static zerr_t webui_http_open(void);
static void webui_cors(httpd_req_t *p_req);
static esp_err_t webui_get_index(httpd_req_t *p_req);
static esp_err_t webui_get_state(httpd_req_t *p_req);
static esp_err_t webui_post_cmd(httpd_req_t *p_req);
static esp_err_t webui_get_cfgs(httpd_req_t *p_req);
static esp_err_t webui_options(httpd_req_t *p_req);
static esp_err_t webui_redirect(httpd_req_t *p_req, httpd_err_code_t err);
static esp_err_t webui_apple_success(httpd_req_t *p_req);
static size_t webui_json_str(char *p_buf, size_t size, const char *p_str);
static void webui_ap_enable(bool on);
static void webui_retry_cb(void *p_arg);
static void webui_ap_off_cb(void *p_arg);
static void webui_mdns_open(void);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t webui_open(void)
{
    uint8_t mac[6];
    esp_err_t err;

    err = esp_netif_init();
    if (ESP_OK != err)
    {
        return ZERR_NOT_OPEN;
    }
    err = esp_event_loop_create_default();
    if ((ESP_OK != err) && (ESP_ERR_INVALID_STATE != err))
    {
        return ZERR_NOT_OPEN;
    }

    (void)esp_netif_create_default_wifi_ap();
    gp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (ESP_OK != esp_wifi_init(&init_cfg))
    {
        return ZERR_NOT_OPEN;
    }

    (void)esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, webui_event_handler, NULL);
    (void)esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, webui_event_handler, NULL);

    (void)esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    (void)snprintf(g_ap_ssid, sizeof(g_ap_ssid), "SmartMattress-%02X%02X", mac[4], mac[5]);

    wifi_config_t ap_cfg = {0};
    memcpy(ap_cfg.ap.ssid, g_ap_ssid, strlen(g_ap_ssid));
    ap_cfg.ap.ssid_len = (uint8_t)strlen(g_ap_ssid);
    memcpy(ap_cfg.ap.password, WEBUI_AP_PASS, strlen(WEBUI_AP_PASS));
    ap_cfg.ap.channel = WEBUI_AP_CHANNEL;
    ap_cfg.ap.max_connection = WEBUI_AP_MAX_CONN;
    ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;

    /* The AP config can only be set while the AP interface exists. */
    if ((ESP_OK != esp_wifi_set_mode(WIFI_MODE_APSTA)) ||
        (ESP_OK != esp_wifi_set_config(WIFI_IF_AP, &ap_cfg)))
    {
        return ZERR_NOT_OPEN;
    }

    /* Home WiFi saved earlier by esp_wifi_set_config() lives in NVS. */
    wifi_config_t sta_cfg = {0};
    if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &sta_cfg))
    {
        memcpy(g_sta_ssid, sta_cfg.sta.ssid, sizeof(g_sta_ssid) - 1U);
        g_sta_ssid[sizeof(g_sta_ssid) - 1U] = '\0';
    }

    /* Home WiFi is the normal way in (http://smartmattress.local). The device's own WiFi is only a
       fallback for setup: on when no home WiFi is saved or it cannot be joined. */
    g_ap_on = ('\0' == g_sta_ssid[0]);
    if (ESP_OK != esp_wifi_set_mode(g_ap_on ? WIFI_MODE_APSTA : WIFI_MODE_STA))
    {
        return ZERR_NOT_OPEN;
    }

    const esp_timer_create_args_t retry_args = {.callback = webui_retry_cb, .name = "sta retry"};
    const esp_timer_create_args_t ap_off_args = {.callback = webui_ap_off_cb, .name = "ap off"};
    (void)esp_timer_create(&retry_args, &g_retry_timer);
    (void)esp_timer_create(&ap_off_args, &g_ap_off_timer);

    if (ESP_OK != esp_wifi_start())
    {
        return ZERR_NOT_OPEN;
    }

    webui_mdns_open();

    if (g_ap_on)
    {
        ESP_LOGI(TAG, "no home WiFi saved: join \"%s\" (pass %s) -> http://192.168.4.1",
                 g_ap_ssid, WEBUI_AP_PASS);
    }

    return webui_http_open();
}

zerr_t webui_set_sta(const char *p_ssid, const char *p_pass)
{
    wifi_config_t sta_cfg = {0};

    strncpy((char *)sta_cfg.sta.ssid, p_ssid, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, p_pass, sizeof(sta_cfg.sta.password));
    sta_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;

    (void)esp_wifi_disconnect();
    if (ESP_OK != esp_wifi_set_config(WIFI_IF_STA, &sta_cfg))
    {
        return ZERR_ERROR;
    }

    strncpy(g_sta_ssid, p_ssid, sizeof(g_sta_ssid) - 1U);
    g_sta_ssid[sizeof(g_sta_ssid) - 1U] = '\0';
    g_sta_connected = false;
    g_sta_retry = 0U;

    (void)esp_timer_stop(g_retry_timer);
    (void)esp_timer_stop(g_ap_off_timer);

    if ('\0' != g_sta_ssid[0])
    {
        ESP_LOGI(TAG, "connecting to \"%s\"", g_sta_ssid);
        (void)esp_wifi_connect();
    }
    else
    {
        webui_ap_enable(true);
    }

    return ZERR_OK;
}

size_t webui_net_text(char *p_buf, size_t size)
{
    if (g_sta_connected)
    {
        char ssid[16];

        (void)snprintf(ssid, sizeof(ssid), "%.15s", g_sta_ssid);
        return (size_t)snprintf(p_buf, size, "Mo web:\nsmartmattress\n.local\nhoac:\n" IPSTR
                                             "\nWiFi:\n%s",
                                IP2STR(&g_sta_ip), ssid);
    }

    return (size_t)snprintf(p_buf, size, "WiFi dem:\n%.13s\n%s\nMK %s\nMo web:\n192.168.4.1\n%s",
                            g_ap_ssid, &g_ap_ssid[13], WEBUI_AP_PASS,
                            ('\0' != g_sta_ssid[0]) ? "WiFi nha: loi" : "Chua cai WiFi");
}

size_t webui_wifi_json(char *p_buf, size_t size)
{
    wifi_sta_list_t clients = {0};
    char ip[16] = "";
    size_t len;

    (void)esp_wifi_ap_get_sta_list(&clients);
    if (g_sta_connected)
    {
        (void)snprintf(ip, sizeof(ip), IPSTR, IP2STR(&g_sta_ip));
    }

    len = (size_t)snprintf(p_buf, size, "{\"ap_ssid\":\"%s\",\"ap_ip\":\"192.168.4.1\","
                                        "\"clients\":%d,\"sta_ssid\":",
                           g_ap_ssid, clients.num);
    if (len < size)
    {
        len += webui_json_str(&p_buf[len], size - len, g_sta_ssid);
    }
    if (len < size)
    {
        len += (size_t)snprintf(&p_buf[len], size - len,
                                ",\"connected\":%s,\"sta_ip\":\"%s\",\"ap_on\":%s,"
                                "\"host\":\"" WEBUI_HOSTNAME ".local\"}",
                                g_sta_connected ? "true" : "false", ip, g_ap_on ? "true" : "false");
    }

    return (len < size) ? len : (size - 1U);
}

/* Private functions -----------------------------------------------------------------------------*/

static void webui_event_handler(void *p_arg, esp_event_base_t base, int32_t id, void *p_data)
{
    (void)p_arg;

    if ((WIFI_EVENT == base) && (WIFI_EVENT_STA_START == id))
    {
        if ('\0' != g_sta_ssid[0])
        {
            ESP_LOGI(TAG, "connecting to \"%s\"", g_sta_ssid);
            (void)esp_wifi_connect();
        }
    }
    else if ((WIFI_EVENT == base) && (WIFI_EVENT_STA_DISCONNECTED == id))
    {
        g_sta_connected = false;
        (void)esp_timer_stop(g_ap_off_timer);
        if (('\0' != g_sta_ssid[0]) && (g_sta_retry < WEBUI_STA_RETRY_MAX))
        {
            g_sta_retry++;
            (void)esp_wifi_connect();
        }
        else if ('\0' != g_sta_ssid[0])
        {
            ESP_LOGW(TAG, "could not join \"%s\": device WiFi \"%s\" on, retry in 60 s",
                     g_sta_ssid, g_ap_ssid);
            webui_ap_enable(true);
            (void)esp_timer_stop(g_retry_timer);
            (void)esp_timer_start_once(g_retry_timer, WEBUI_STA_RETRY_WAIT_US);
        }
    }
    else if ((IP_EVENT == base) && (IP_EVENT_STA_GOT_IP == id))
    {
        const ip_event_got_ip_t *p_evt = (const ip_event_got_ip_t *)p_data;

        g_sta_ip = p_evt->ip_info.ip;
        g_sta_connected = true;
        g_sta_retry = 0U;
        ESP_LOGI(TAG, "home WiFi \"%s\" -> http://" WEBUI_HOSTNAME ".local  http://" IPSTR,
                 g_sta_ssid, IP2STR(&g_sta_ip));
        if (g_ap_on)
        {
            (void)esp_timer_stop(g_ap_off_timer);
            (void)esp_timer_start_once(g_ap_off_timer, WEBUI_AP_OFF_DELAY_US);
        }
    }
    else if ((WIFI_EVENT == base) && (WIFI_EVENT_AP_STACONNECTED == id))
    {
        ESP_LOGI(TAG, "web client joined the device WiFi");
    }
}

static zerr_t webui_http_open(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();

    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.lru_purge_enable = true;
    cfg.stack_size = 6144;
    cfg.max_uri_handlers = 16;      /* Default 8 is too few for the list below. */

    if (ESP_OK != httpd_start(&g_server, &cfg))
    {
        ESP_LOGE(TAG, "http server start failed");
        return ZERR_NOT_OPEN;
    }

    const httpd_uri_t uris[] = {
        {.uri = "/api/state", .method = HTTP_GET,     .handler = webui_get_state},
        {.uri = "/api/cmd",   .method = HTTP_POST,    .handler = webui_post_cmd},
        {.uri = "/api/cfgs",  .method = HTTP_GET,     .handler = webui_get_cfgs},
        {.uri = "/api/*",     .method = HTTP_OPTIONS, .handler = webui_options},
        {.uri = "/",          .method = HTTP_GET,     .handler = webui_get_index},
        {.uri = "/index.html",.method = HTTP_GET,     .handler = webui_get_index},
        /* iOS/macOS captive check: answer "Success" so the iPhone stays on the mattress WiFi
           instead of opening the captive sheet, which drops the WiFi when it is closed. */
        {.uri = "/hotspot-detect.html",        .method = HTTP_GET, .handler = webui_apple_success},
        {.uri = "/library/test/success.html",  .method = HTTP_GET, .handler = webui_apple_success},
    };

    for (size_t i = 0U; i < (sizeof(uris) / sizeof(uris[0])); i++)
    {
        (void)httpd_register_uri_handler(g_server, &uris[i]);
    }

    /* Captive portal: every other URL (Android generate_204, Apple hotspot-detect, Windows
       connecttest ...) is redirected to the web app, so the OS opens it right after joining. */
    (void)httpd_register_err_handler(g_server, HTTPD_404_NOT_FOUND, webui_redirect);

    if (ZERR_OK != captive_dns_open())
    {
        ESP_LOGW(TAG, "captive portal DNS not started");
    }

    return ZERR_OK;
}

/**
 * @brief  Let the page call the API when it is opened from a file or another host.
 */
static void webui_cors(httpd_req_t *p_req)
{
    (void)httpd_resp_set_hdr(p_req, "Access-Control-Allow-Origin", "*");
    (void)httpd_resp_set_hdr(p_req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    (void)httpd_resp_set_hdr(p_req, "Access-Control-Allow-Headers", "Content-Type");
    (void)httpd_resp_set_hdr(p_req, "Access-Control-Allow-Private-Network", "true");
    (void)httpd_resp_set_hdr(p_req, "Cache-Control", "no-store");
}

static esp_err_t webui_get_index(httpd_req_t *p_req)
{
    /* EMBED_TXTFILES adds a trailing '\0'. */
    size_t len = (size_t)(index_html_end - index_html_start) - 1U;

    (void)httpd_resp_set_type(p_req, "text/html; charset=utf-8");
    /* Always serve the page from the board, never an old copy from the browser cache. */
    (void)httpd_resp_set_hdr(p_req, "Cache-Control", "no-store");
    return httpd_resp_send(p_req, index_html_start, (ssize_t)len);
}

static esp_err_t webui_get_state(httpd_req_t *p_req)
{
    char buf[DEVCTL_JSON_MAX];
    size_t len = devctl_state_json(buf, sizeof(buf));

    devctl_touch();

    webui_cors(p_req);
    (void)httpd_resp_set_type(p_req, "application/json");
    return httpd_resp_send(p_req, buf, (ssize_t)len);
}

static esp_err_t webui_get_cfgs(httpd_req_t *p_req)
{
    char buf[DEVCTL_CFG_JSON_MAX];
    size_t len = devctl_cfgs_json(buf, sizeof(buf));

    webui_cors(p_req);
    (void)httpd_resp_set_type(p_req, "application/json");
    return httpd_resp_send(p_req, buf, (ssize_t)len);
}

static esp_err_t webui_post_cmd(httpd_req_t *p_req)
{
    char body[WEBUI_CMD_MAX];
    char reply[160];
    int total = 0;

    webui_cors(p_req);

    if (p_req->content_len >= sizeof(body))
    {
        return httpd_resp_send_err(p_req, HTTPD_400_BAD_REQUEST, "command too long");
    }

    while (total < (int)p_req->content_len)
    {
        int got = httpd_req_recv(p_req, &body[total], p_req->content_len - (size_t)total);
        if (got <= 0)
        {
            if (HTTPD_SOCK_ERR_TIMEOUT == got)
            {
                continue;
            }
            return ESP_FAIL;
        }
        total += got;
    }
    body[total] = '\0';

    zerr_t ret = devctl_command(body, reply, sizeof(reply));
    if (ZERR_OK != ret)
    {
        (void)httpd_resp_set_status(p_req, "400 Bad Request");
    }
    (void)httpd_resp_set_type(p_req, "application/json");
    return httpd_resp_sendstr(p_req, reply);
}

static esp_err_t webui_apple_success(httpd_req_t *p_req)
{
    (void)httpd_resp_set_type(p_req, "text/html");
    (void)httpd_resp_set_hdr(p_req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(p_req, "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
}

static esp_err_t webui_redirect(httpd_req_t *p_req, httpd_err_code_t err)
{
    (void)err;
    (void)httpd_resp_set_status(p_req, "302 Found");
    (void)httpd_resp_set_hdr(p_req, "Location", "http://192.168.4.1/");
    (void)httpd_resp_set_hdr(p_req, "Cache-Control", "no-store");
    return httpd_resp_send(p_req, "Redirect", HTTPD_RESP_USE_STRLEN);
}

/**
 * @brief  Switch the device's own WiFi (fallback AP) on or off, keeping the STA side.
 */
static void webui_ap_enable(bool on)
{
    if (on != g_ap_on)
    {
        if (ESP_OK == esp_wifi_set_mode(on ? WIFI_MODE_APSTA : WIFI_MODE_STA))
        {
            g_ap_on = on;
            ESP_LOGI(TAG, "device WiFi \"%s\" %s", g_ap_ssid, on ? "on" : "off");
        }
    }
}

static void webui_retry_cb(void *p_arg)
{
    (void)p_arg;
    if (!g_sta_connected && ('\0' != g_sta_ssid[0]))
    {
        g_sta_retry = 0U;
        (void)esp_wifi_connect();
    }
}

static void webui_ap_off_cb(void *p_arg)
{
    (void)p_arg;
    if (g_sta_connected)
    {
        webui_ap_enable(false);
    }
}

/**
 * @brief  Announce http://smartmattress.local on the home network.
 */
static void webui_mdns_open(void)
{
    if (ESP_OK != mdns_init())
    {
        ESP_LOGW(TAG, "mDNS not started");
        return;
    }
    (void)mdns_hostname_set(WEBUI_HOSTNAME);
    (void)mdns_instance_name_set("Smart Mattress");
    (void)mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}

static esp_err_t webui_options(httpd_req_t *p_req)
{
    webui_cors(p_req);
    (void)httpd_resp_set_status(p_req, "204 No Content");
    return httpd_resp_send(p_req, NULL, 0);
}

/**
 * @brief  Write p_str as a quoted JSON string.
 */
static size_t webui_json_str(char *p_buf, size_t size, const char *p_str)
{
    size_t len = 0U;

    if (size < 3U)
    {
        return 0U;
    }

    p_buf[len++] = '"';
    for (const char *p = p_str; ('\0' != *p) && ((len + 3U) < size); p++)
    {
        if (('"' == *p) || ('\\' == *p))
        {
            p_buf[len++] = '\\';
            p_buf[len++] = *p;
        }
        else if ((unsigned char)*p >= 0x20U)
        {
            p_buf[len++] = *p;
        }
    }
    p_buf[len++] = '"';
    p_buf[len] = '\0';

    return len;
}

/* END OF FILE ************************************************************************************/

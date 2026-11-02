/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : captive_dns.c
 * @description     : Captive portal DNS: answers every name with the AP address (192.168.4.1), so a
 *                    phone or laptop that joins the device WiFi opens the web app by itself.
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/* Includes --------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#include "captive_dns.h"

/* Define ----------------------------------------------------------------------------------------*/

#define CAPTIVE_DNS_PORT            53
#define CAPTIVE_DNS_BUF             512
#define CAPTIVE_DNS_HDR_LEN         12U
#define CAPTIVE_DNS_ANSWER_LEN      16U
#define CAPTIVE_DNS_TTL_S           60U
#define CAPTIVE_DNS_STACK           3072U

/* Private variables -----------------------------------------------------------------------------*/

static const uint8_t g_ap_ip[4] = {192U, 168U, 4U, 1U};

static const char *TAG = "DNS";

/* Private function prototypes -------------------------------------------------------------------*/

static void captive_dns_task(void *p_para);
static int captive_dns_answer(uint8_t *p_buf, int len);

/* Public functions ------------------------------------------------------------------------------*/

zerr_t captive_dns_open(void)
{
    if (pdPASS != xTaskCreate(captive_dns_task, "captive dns", CAPTIVE_DNS_STACK, NULL,
                              tskIDLE_PRIORITY + 1U, NULL))
    {
        return ZERR_MEM_OUT;
    }

    return ZERR_OK;
}

/* Private functions -----------------------------------------------------------------------------*/

static void captive_dns_task(void *p_para)
{
    static uint8_t s_buf[CAPTIVE_DNS_BUF];
    struct sockaddr_in addr = {0};
    int sock;

    (void)p_para;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CAPTIVE_DNS_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((sock < 0) || (0 != bind(sock, (struct sockaddr *)&addr, sizeof(addr))))
    {
        ESP_LOGE(TAG, "cannot open UDP port 53");
        if (sock >= 0)
        {
            close(sock);
        }
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "captive portal DNS on port 53");

    while (true)
    {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        int len = recvfrom(sock, s_buf, sizeof(s_buf) - CAPTIVE_DNS_ANSWER_LEN, 0,
                           (struct sockaddr *)&from, &from_len);

        if (len > 0)
        {
            len = captive_dns_answer(s_buf, len);
            if (len > 0)
            {
                (void)sendto(sock, s_buf, (size_t)len, 0, (struct sockaddr *)&from, from_len);
            }
        }
    }
}

/**
 * @brief  Turn the query in p_buf into a reply in place. A queries get 192.168.4.1, other types an
 *         empty answer. Returns the reply length, or 0 to drop the packet.
 */
static int captive_dns_answer(uint8_t *p_buf, int len)
{
    size_t pos = CAPTIVE_DNS_HDR_LEN;

    /* Only plain queries (QR = 0, opcode 0) with exactly one question. */
    if ((len < (int)CAPTIVE_DNS_HDR_LEN) || (0U != (p_buf[2] & 0xF8U)) || (0U != p_buf[4]) ||
        (1U != p_buf[5]))
    {
        return 0;
    }

    while ((pos < (size_t)len) && (0U != p_buf[pos]))
    {
        if (0U != (p_buf[pos] & 0xC0U))
        {
            return 0;   /* No compression pointers in a question. */
        }
        pos += (size_t)p_buf[pos] + 1U;
    }
    pos += 1U + 4U;     /* Root label, QTYPE, QCLASS. */
    if (pos > (size_t)len)
    {
        return 0;
    }

    bool is_a = (0x00U == p_buf[pos - 4U]) && (0x01U == p_buf[pos - 3U]);

    p_buf[2] = (uint8_t)(0x80U | (p_buf[2] & 0x01U));  /* QR = 1, keep RD. */
    p_buf[3] = 0x80U;                                  /* RA = 1, RCODE = 0. */
    p_buf[6] = 0U;
    p_buf[7] = is_a ? 1U : 0U;                         /* ANCOUNT */
    memset(&p_buf[8], 0, 4U);                          /* NSCOUNT, ARCOUNT */

    if (!is_a)
    {
        return (int)pos;
    }

    const uint8_t answer[CAPTIVE_DNS_ANSWER_LEN] = {
        0xC0U, 0x0CU,                   /* Name: pointer to the question. */
        0x00U, 0x01U, 0x00U, 0x01U,     /* Type A, class IN. */
        0x00U, 0x00U, 0x00U, (uint8_t)CAPTIVE_DNS_TTL_S,
        0x00U, 0x04U,
        g_ap_ip[0], g_ap_ip[1], g_ap_ip[2], g_ap_ip[3],
    };
    memcpy(&p_buf[pos], answer, sizeof(answer));

    return (int)(pos + sizeof(answer));
}

/* END OF FILE ************************************************************************************/

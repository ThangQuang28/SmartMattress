/***************************************************************************************************************
 * @auth            : duonghd                                                                            
 * @day             : 16-Mar-2026
 * @file            : package.c                                                                          
 * @dissaption      : package functions for the application.                                    
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

/* Middleware */
#include "package.h"

/* Include ---------------------------------------------------------------------------------------*/

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "PACKAGE";

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static uint8_t pkg_crc(pkg_t *pkt);

/* Public function prototypes --------------------------------------------------------------------*/

int pkg_build(uint8_t *buf, uint8_t cmd, uint8_t *data, uint8_t len)
{
    buf[0] = PKG_SOF;
    buf[1] = cmd;
    buf[2] = len;

    memcpy(&buf[3], data, len);

    uint8_t crc = 0;
    crc ^= buf[0];
    crc ^= buf[1];
    crc ^= buf[2];

    for(int i = 0; i < len; i++)
        crc ^= buf[3 + i];

    buf[3 + len] = crc;

    return len + 4;
}

int pkg_parse(pkg_t *pkt, uint8_t *buf, int len)
{
    if(pkt == NULL || buf == NULL)
        return -1;

    if(len < 4) // SOF CMD LEN CRC
        return -2;

    if(buf[0] != PKG_SOF)
        return -3;

    uint8_t data_len = buf[2];

    if(data_len > PKG_MAX_DATA)
        return -4;

    if(len < (data_len + 4))
        return -5;

    pkt->sof = buf[0];
    pkt->cmd = buf[1];
    pkt->len = data_len;

    memcpy(pkt->data, &buf[3], data_len);

    pkt->crc = buf[3 + data_len];

    if(pkg_crc(pkt) != pkt->crc)
        return -6;

    return 0;
}

void pkg_print(pkg_t *pkt)
{
    if(pkt == NULL)
        return;

    ESP_LOGI(TAG, "------ PKG ------");
    ESP_LOGI(TAG, "SOF : 0x%02X", pkt->sof);
    ESP_LOGI(TAG, "CMD : 0x%02X", pkt->cmd);
    ESP_LOGI(TAG, "LEN : %d", pkt->len);

    /* HEX */
    printf("DATA HEX : ");
    for(int i = 0; i < pkt->len; i++)
    {
        printf("%02X ", pkt->data[i]);
    }
    printf("\n");

    /* ASCII */
    printf("DATA STR : ");
    for(int i = 0; i < pkt->len; i++)
    {
        char c = pkt->data[i];
        if(c >= 32 && c <= 126)
            printf("%c", c);
        else
            printf(".");
    }
    printf("\n");

    ESP_LOGI(TAG, "CRC : 0x%02X", pkt->crc);
    ESP_LOGI(TAG, "-----------------");
}

void pkg_dump_buf(const char *tag, const uint8_t *buf, int len)
{
    if(buf == NULL || len <= 0)
        return;

    char log[256] = {0};
    int offset = 0;

    for(int i = 0; i < len; i++)
    {
        offset += snprintf(log + offset,
                           sizeof(log) - offset,
                           "%02X ",
                           buf[i]);

        if(offset >= sizeof(log))
            break;
    }

    ESP_LOGI(tag, "BUF[%d]: %s", len, log);
}

/*
 * uint8_t data[3];
 * uint8_t buf[32];
 * motor_pack(data,
            0,1,0,100,   // M1: forward 100
            1,1,0,50);   // M2: backward 50

 * int size = pkg_build(buf, 0x10, data, 3);
 * esp_now_send(peer_mac, buf, size);
 */
void motor_build_pack(uint8_t *data,
                uint8_t dir1, uint8_t en1, uint8_t brk1, uint8_t speed1,
                uint8_t dir2, uint8_t en2, uint8_t brk2, uint8_t speed2)
{
    data[0] = 0;

    data[0] |= (dir1 & 0x01) << 0;
    data[0] |= (dir2 & 0x01) << 1;
    data[0] |= (en1  & 0x01) << 2;
    data[0] |= (en2  & 0x01) << 3;
    data[0] |= (brk1 & 0x01) << 4;
    data[0] |= (brk2 & 0x01) << 5;

    data[1] = speed1;
    data[2] = speed2;
}

/* Private function prototypes -------------------------------------------------------------------*/

static uint8_t pkg_crc(pkg_t *pkt)
{
    uint8_t crc = 0;

    crc ^= pkt->sof;
    crc ^= pkt->cmd;
    crc ^= pkt->len;

    for(int i = 0; i < pkt->len; i++)
    {
        crc ^= pkt->data[i];
    }

    return crc;
}

/* END OF FILE ************************************************************************************/
/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : bmp20.h                                                                            
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
#include "bmp280.h"

/* BSP. */
#include "iic_global_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

#define BMP280_REG_ID          0xD0
#define BMP280_REG_RESET       0xE0
#define BMP280_REG_STATUS      0xF3
#define BMP280_REG_CTRL_MEAS   0xF4
#define BMP280_REG_CONFIG      0xF5
#define BMP280_REG_PRESS_MSB   0xF7

#define BMP280_CMD_RESET       0xB6

/* Private data types ----------------------------------------------------------------------------*/

static const char *TAG = "BMP280";

typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    int32_t t_fine;
} bmp280_calib_t;

static bmp280_calib_t calib;

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static float bmp280_compensate_T(int32_t adc_T);

static float bmp280_compensate_P(int32_t adc_P);

/* Public function prototypes --------------------------------------------------------------------*/

void bmp280_open(void)
{
    uint8_t ctrl_meas = 0x27; // temp x1, press x1, normal mode
    uint8_t config = 0xA0;    // standby + filter

    global_bmp280_bsp_write(BMP280_REG_CTRL_MEAS, &ctrl_meas, 1);
    global_bmp280_bsp_write(BMP280_REG_CONFIG, &config, 1);

    vTaskDelay(pdMS_TO_TICKS(100));

    /* Read calib */
    uint8_t data[24];
    global_bmp280_bsp_read(0x88, data, 24);

    calib.dig_T1 = (data[1] << 8) | data[0];
    calib.dig_T2 = (data[3] << 8) | data[2];
    calib.dig_T3 = (data[5] << 8) | data[4];

    calib.dig_P1 = (data[7] << 8) | data[6];
    calib.dig_P2 = (data[9] << 8) | data[8];
    calib.dig_P3 = (data[11] << 8) | data[10];
    calib.dig_P4 = (data[13] << 8) | data[12];
    calib.dig_P5 = (data[15] << 8) | data[14];
    calib.dig_P6 = (data[17] << 8) | data[16];
    calib.dig_P7 = (data[19] << 8) | data[18];
    calib.dig_P8 = (data[21] << 8) | data[20];
    calib.dig_P9 = (data[23] << 8) | data[22];
}

void bmp280_reset(void)
{

}

void bmp280_measure(void){}

bmp280_data_t bmp280_read(void)
{
    uint8_t data[6];
    bmp280_data_t bmp280_data = {0};

    if (global_bmp280_bsp_read(BMP280_REG_PRESS_MSB, data, 6) != ESP_OK)
    {
        ESP_LOGE(TAG, "Read failed");
        return bmp280_data;
    }

    int32_t adc_p = (int32_t)((data[0] << 12) | (data[1] << 4) | (data[2] >> 4));
    int32_t adc_t = (int32_t)((data[3] << 12) | (data[4] << 4) | (data[5] >> 4));

    float temp = bmp280_compensate_T(adc_t) / 100.0f;
    float press = bmp280_compensate_P(adc_p);

    bmp280_data.temp = temp;
    bmp280_data.press = press;

    return bmp280_data;
}

/* Private function prototypes -------------------------------------------------------------------*/

static float bmp280_compensate_T(int32_t adc_T)
{
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) *
            ((int32_t)calib.dig_T2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
            ((int32_t)calib.dig_T3)) >> 14;

    calib.t_fine = var1 + var2;

    return (calib.t_fine * 5 + 128) >> 8;
}

static float bmp280_compensate_P(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);

    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) +
           ((var1 * (int64_t)calib.dig_P2) << 12);

    var1 = (((((int64_t)1) << 47) + var1)) *
           ((int64_t)calib.dig_P1) >> 33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);

    return (float)p / 256.0f;
}

/* END OF FILE ************************************************************************************/
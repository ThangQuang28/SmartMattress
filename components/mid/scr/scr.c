/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : scr.c                                                                           
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
#include "oled_096.h"
#include "scr.h"
#include "bitmaps.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static const uint8_t icon_temp[8] =
{
    0x18,
    0x24,
    0x24,
    0x24,
    0x24,
    0x24,
    0x3C,
    0x18
};

static const uint8_t icon_hum[8] =
{
    0x10,
    0x18,
    0x3C,
    0x3C,
    0x7E,
    0x7E,
    0x3C,
    0x18
};

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/

static const char* get_temp_level(float t);
static const char* get_hum_level(float h);

static void draw_icon8x8(uint8_t x, uint8_t y, const uint8_t *icon);

/* Public function prototypes --------------------------------------------------------------------*/

void scr_open(void)
{
    oled_open();
    oled_clear();
    oled_update();    
}

void scr_draw_bitmap(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    const uint8_t *bitmap) {
    
    oled_draw_bitmap(x, y, width, height, bitmap);
}

void scr_update(scr_type_t type, scr_env_t *env)
{
    char buf[32];
    
    scr_cls();
    switch (type)
    {
    case SCR_TEMP:
        /* ===== TEMP ===== */
        sprintf(buf, "TEMP");
        oled_draw_string_x2(0, 0, buf);

        sprintf(buf, "%.1f", env->temperature);
        oled_draw_string_x2(0, 25, buf);

        draw_icon8x8(2, 53, icon_temp);
        sprintf(buf, "%s", get_temp_level(env->temperature));
        oled_draw_string_x2(12, 50, buf);

        scr_draw_bitmap(60, 0, 64, 64, temp_bitmap);

        break;
    case SCR_HUM:

        scr_draw_bitmap(0, 0, 64, 64, hum_bitmap);
        /** HUM */
        sprintf(buf, "HUM");
        oled_draw_string_x2(65, 0, buf);

        sprintf(buf, "%.1f%c", env->humidity, '%');
        oled_draw_string_x2(65, 25, buf);

        draw_icon8x8(65, 53, icon_hum);
        sprintf(buf, "%s", get_hum_level(env->humidity));
        oled_draw_string_x2(75, 50, buf);        
        break;
    default:
        break;
    }

    oled_update();
}

void scr_write(void) {
    oled_update();
}
/* Private function prototypes -------------------------------------------------------------------*/

static void draw_icon8x8(uint8_t x, uint8_t y, const uint8_t *icon)
{
    for (uint8_t row = 0; row < 8; row++)
    {
        for (uint8_t col = 0; col < 8; col++)
        {
            if (icon[row] & (1 << col))
            {
                oled_draw_pixel(x + col, y + row, 1);
            }
        }
    }
}

static const char* get_temp_level(float t)
{
    if (t < 20) return "COLD";
    else if (t < 30) return "OK";
    else return "HOT";
}

static const char* get_hum_level(float h)
{
    if (h < 40) return "DRY";
    else if (h < 70) return "OK";
    else return "WET";
}

void scr_cls(void)
{
    oled_clear();
}
/* END OF FILE ************************************************************************************/
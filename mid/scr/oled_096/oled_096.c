/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : OLED_096.c                                                                           
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

/* BSP. */
#include "iic_global_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

#define OLED_WIDTH     128
#define OLED_HEIGHT    64
#define OLED_PAGE_NUM  8

#define OLED_CMD_DISPLAY_OFF                 0xAE
#define OLED_CMD_DISPLAY_ON                  0xAF

#define OLED_CMD_SET_MEMORY_MODE             0x20
#define OLED_CMD_MEMORY_MODE_HORIZONTAL      0x00
#define OLED_CMD_MEMORY_MODE_VERTICAL        0x01
#define OLED_CMD_MEMORY_MODE_PAGE            0x02

#define OLED_CMD_SET_PAGE_START              0xB0  // + page (0-7)

#define OLED_CMD_SET_COM_SCAN_DEC            0xC8
#define OLED_CMD_SET_COM_SCAN_INC            0xC0

#define OLED_CMD_SET_LOW_COLUMN              0x00
#define OLED_CMD_SET_HIGH_COLUMN             0x10

#define OLED_CMD_SET_START_LINE              0x40  // + line (0-63)

#define OLED_CMD_SET_CONTRAST                0x81

#define OLED_CMD_SET_SEG_REMAP               0xA1  // or 0xA0

#define OLED_CMD_NORMAL_DISPLAY              0xA6
#define OLED_CMD_INVERT_DISPLAY              0xA7

#define OLED_CMD_SET_MULTIPLEX              0xA8

#define OLED_CMD_DISPLAY_RAM                0xA4
#define OLED_CMD_DISPLAY_ALL_ON             0xA5

#define OLED_CMD_SET_DISPLAY_OFFSET         0xD3

#define OLED_CMD_SET_DISPLAY_CLOCK_DIV      0xD5

#define OLED_CMD_SET_PRECHARGE              0xD9

#define OLED_CMD_SET_COM_PINS               0xDA

#define OLED_CMD_SET_VCOMH_DESELECT         0xDB

#define OLED_CMD_CHARGE_PUMP                0x8D
#define OLED_CMD_CHARGE_PUMP_ENABLE         0x14
#define OLED_CMD_CHARGE_PUMP_DISABLE        0x10

#define OLED_PAGE(page)        (OLED_CMD_SET_PAGE_START + ((page) & 0x07))
#define OLED_COLUMN_LOW(col)   (OLED_CMD_SET_LOW_COLUMN  | ((col) & 0x0F))
#define OLED_COLUMN_HIGH(col)  (OLED_CMD_SET_HIGH_COLUMN | ((col) >> 4))
#define OLED_START_LINE(line)  (OLED_CMD_SET_START_LINE  | ((line) & 0x3F))

/* Private data types ----------------------------------------------------------------------------*/

static const uint8_t font5x7[96][5] = {
{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
{0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
{0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},

{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},

{0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},
{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
{0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},

{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},

{0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
{0x00,0x03,0x07,0x08,0x00},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
{0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
{0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x7F,0x10,0x28,0x44,0x00},
{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},

{0x7C,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
{0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},
{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},
{0x00,0x00,0x7F,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},{0x10,0x08,0x08,0x10,0x08},{0x00,0x06,0x09,0x09,0x06}
};

static uint8_t g_oled_buffer[OLED_WIDTH * OLED_PAGE_NUM];

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

static void oled_set_cursor(uint8_t page, uint8_t column);

/* Public function prototypes --------------------------------------------------------------------*/

void oled_open(void)
{

    global_i2c_bsp_open();

    global_oled_bsp_write_cmd(OLED_CMD_DISPLAY_OFF);

    global_oled_bsp_write_cmd(OLED_CMD_SET_MEMORY_MODE);
    global_oled_bsp_write_cmd(OLED_CMD_MEMORY_MODE_HORIZONTAL);

    global_oled_bsp_write_cmd(OLED_CMD_SET_COM_SCAN_DEC);

    global_oled_bsp_write_cmd(OLED_CMD_SET_START_LINE | 0x00);

    global_oled_bsp_write_cmd(OLED_CMD_SET_CONTRAST);
    global_oled_bsp_write_cmd(0xFF);

    global_oled_bsp_write_cmd(OLED_CMD_SET_SEG_REMAP);
    global_oled_bsp_write_cmd(OLED_CMD_NORMAL_DISPLAY);

    global_oled_bsp_write_cmd(OLED_CMD_SET_MULTIPLEX);
    global_oled_bsp_write_cmd(0x3F);

    global_oled_bsp_write_cmd(OLED_CMD_DISPLAY_RAM);

    global_oled_bsp_write_cmd(OLED_CMD_SET_DISPLAY_OFFSET);
    global_oled_bsp_write_cmd(0x00);

    global_oled_bsp_write_cmd(OLED_CMD_SET_DISPLAY_CLOCK_DIV);
    global_oled_bsp_write_cmd(0xF0);

    global_oled_bsp_write_cmd(OLED_CMD_SET_PRECHARGE);
    global_oled_bsp_write_cmd(0x22);

    global_oled_bsp_write_cmd(OLED_CMD_SET_COM_PINS);
    global_oled_bsp_write_cmd(0x12);

    global_oled_bsp_write_cmd(OLED_CMD_SET_VCOMH_DESELECT);
    global_oled_bsp_write_cmd(0x20);

    global_oled_bsp_write_cmd(OLED_CMD_CHARGE_PUMP);
    global_oled_bsp_write_cmd(OLED_CMD_CHARGE_PUMP_ENABLE);

    global_oled_bsp_write_cmd(OLED_CMD_DISPLAY_ON);

    memset(g_oled_buffer, 0x00, sizeof(g_oled_buffer));
}

/* Update screen */
void oled_update(void)
{
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++)
    {
        oled_set_cursor(page, 0);

        global_oled_bsp_write_data(
            &g_oled_buffer[page * OLED_WIDTH],
            OLED_WIDTH
        );
    }
}

/* Clear buffer */
void oled_clear(void)
{
    memset(g_oled_buffer, 0x00, sizeof(g_oled_buffer));
}

/* Fill buffer */
void oled_fill(uint8_t data)
{
    memset(g_oled_buffer, data, sizeof(g_oled_buffer));
}

void oled_on(void){
    global_oled_bsp_write_cmd(0xAF); // ON
}
void oled_off(void){
    global_oled_bsp_write_cmd(0xAE); // OFF
}
/* Draw pixel */
void oled_draw_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;

    if (color)
        g_oled_buffer[index] |= (1 << (y % 8));
    else
        g_oled_buffer[index] &= ~(1 << (y % 8));
}

void oled_draw_bitmap(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    const uint8_t *bitmap)
{
    uint16_t byte_width = (width + 7) / 8;

    for (uint8_t j = 0; j < height; j++)
    {
        for (uint8_t i = 0; i < width; i++)
        {
            uint16_t byte_index =
                j * byte_width + (i / 8);

            uint8_t byte = bitmap[byte_index];

            if (byte & (0x80 >> (i % 8)))
            {
                oled_draw_pixel(x + i, y + j, 1);
            }
        }
    }
}

/* Draw char */
void oled_draw_char(uint8_t x, uint8_t y, char c)
{
    if (c < 32 || c > 127) return;

    const uint8_t *ch = font5x7[c - 32];

    for (uint8_t i = 0; i < 5; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            if (ch[i] & (1 << j))
            {
                oled_draw_pixel(x + i, y + j, 1);
            }
        }
    }
}
void oled_draw_char_x2(uint8_t x, uint8_t y, char c)
{
    if (c < 32 || c > 127) return;

    const uint8_t *bitmap = font5x7[c - 32];

    for (int col = 0; col < 5; col++)
    {
        uint8_t line = bitmap[col];

        for (int row = 0; row < 7; row++)
        {
            if (line & (1 << row))
            {
                oled_draw_pixel(x + col*2    , y + row*2    , 1);
                oled_draw_pixel(x + col*2 + 1, y + row*2    , 1);
                oled_draw_pixel(x + col*2    , y + row*2 + 1, 1);
                oled_draw_pixel(x + col*2 + 1, y + row*2 + 1, 1);
            }
        }
    }
}

/* Draw string */
void oled_draw_string(uint8_t x, uint8_t y, const char *str)
{
    while (*str)
    {
        oled_draw_char(x, y, *str++);
        x += 6;
    }
}
void oled_draw_string_x2(uint8_t x, uint8_t y, const char *str)
{
    while (*str)
    {
        oled_draw_char_x2(x, y, *str);
        x += 12; // 10 pixel + spacing
        str++;
    }
}
/* Private function prototypes -------------------------------------------------------------------*/

static void oled_set_cursor(uint8_t page, uint8_t column)
{
    global_oled_bsp_write_cmd(OLED_PAGE(page));
    global_oled_bsp_write_cmd(OLED_COLUMN_LOW(column));
    global_oled_bsp_write_cmd(OLED_COLUMN_HIGH(column));
}

/* END OF FILE ************************************************************************************/
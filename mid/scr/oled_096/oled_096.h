/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : OLED_096.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _OLED_096_H
#define _OLED_096_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void oled_open(void);
void oled_clear(void);
void oled_fill(uint8_t data);
void oled_update(void);
void oled_draw_pixel(uint8_t x, uint8_t y, uint8_t color);
void oled_draw_bitmap(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    const uint8_t *bitmap);
void oled_draw_char(uint8_t x, uint8_t y, char c);
void oled_draw_string(uint8_t x, uint8_t y, const char *str);
void oled_draw_char_x2(uint8_t x, uint8_t y, char c);
void oled_draw_string_x2(uint8_t x, uint8_t y, const char *str);

void oled_on(void);
void oled_off(void);

#ifdef __cplusplus
}
#endif

#endif /* _OLED_096_H */

/* END OF FILE ************************************************************************************/

/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Oct-2026
 * @file            : oled_lvgl.h
 * @dissaption      : OLED LVGL middleware interface.
***************************************************************************************************************/

#ifndef _OLED_LVGL_H
#define _OLED_LVGL_H

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"
#include <stdbool.h>

#include "zerr.h"

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t oled_lvgl_open(void);
zerr_t oled_lvgl_clear(void);
zerr_t oled_lvgl_set_display(bool enable);

zerr_t oled_lvgl_show_spinner(void);
zerr_t oled_lvgl_hide_spinner(void);

/* Text */
zerr_t oled_lvgl_set_text(const char *p_text);
zerr_t oled_lvgl_blink_text(const char *p_text, uint32_t period_ms);
zerr_t oled_lvgl_stop_blink(void);

/* Picture */
zerr_t oled_lvgl_show_picture(const char *p_data, size_t data_size, uint16_t width, uint16_t height);
zerr_t oled_lvgl_show_gif(const uint8_t *p_data, size_t data_size);
zerr_t oled_lvgl_hide_picture(void);

#endif /* _OLED_LVGL_H */

/* END OF FILE ************************************************************************************/
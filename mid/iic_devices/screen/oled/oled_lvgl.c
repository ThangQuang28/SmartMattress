/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Oct-2026
 * @file            : oled_lvgl.c
 * @description     : OLED and LVGL middleware.
***************************************************************************************************************/

/* Includes --------------------------------------------------------------------------------------*/

#include "zcfg.h"
#include "iic_global_bsp.h"
#include "oled_lvgl.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "esp_lcd_panel_sh1106.h"

/* Defines ---------------------------------------------------------------------------------------*/

#define OLED_BSP_SH1106             1

#define OLED_I2C_ADDR               0x3C
#define OLED_WIDTH                  128
#define OLED_HEIGHT                 64
#define OLED_RST_PIN                (-1)

/* Private variables -----------------------------------------------------------------------------*/

/* Struct defintition */
typedef struct
{
    bool initialized;

    esp_lcd_panel_io_handle_t p_io;
    esp_lcd_panel_handle_t p_panel;

    lv_disp_t *p_disp;
    lv_obj_t *p_label;
    lv_obj_t *p_spinner;

    lv_timer_t *p_blink_timer;
    bool blink_visible;

    lv_obj_t *p_picture;
#if LVGL_VERSION_MAJOR >= 9
    lv_image_dsc_t svg_picture;
#else
    lv_img_dsc_t svg_picture;
#endif

} oled_lvgl_t;

static oled_lvgl_t gh_oled = {0};

/* Private function prototypes -------------------------------------------------------------------*/

static zerr_t oled_lvgl_init(void);
static void lv_obj_set_hidden(lv_obj_t *p_obj, bool hidden);
static void oled_lvgl_blink_timer_cb(lv_timer_t *p_timer);

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief Open.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_open(void)
{
    zerr_t ret = oled_lvgl_init();

    if (ret != ZERR_OK)
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("OLED LVGL open failed: %d", ret));
        return ret;
    }

    return ret;
}

/**
 * @brief Clear.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_clear(void)
{
    return oled_lvgl_set_text("");
}


/**
 * @brief Set display.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_set_display(bool enable)
{
    if (gh_oled.p_panel == NULL)
    {
        return ZERR_NOT_OPEN;
    }

    esp_err_t err = esp_lcd_panel_disp_on_off(gh_oled.p_panel, enable);
    if (err != ESP_OK)
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("OLED display control failed: %s",
                                  esp_err_to_name(err)));
        return ZERR_ERROR;
    }

    return ZERR_OK;
}

/**
 * @brief Set the text displayed on the OLED.
 *
 * @param p_text Pointer to the null-terminated text string.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_set_text(const char *p_text)
{
    if ((p_text == NULL) || (gh_oled.p_disp == NULL) || (gh_oled.p_label == NULL))
    {
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

    lv_label_set_text(gh_oled.p_label, p_text);
    lv_obj_align(gh_oled.p_label, LV_ALIGN_CENTER, 0, 0);

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Show multi-line text in the small 8 px font, left aligned from the top-left corner.
 *
 * @param p_text Pointer to the null-terminated text string.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_set_text_small(const char *p_text)
{
    if ((p_text == NULL) || (gh_oled.p_disp == NULL) || (gh_oled.p_label == NULL))
    {
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

#if LV_FONT_UNSCII_8
    lv_obj_set_style_text_font(gh_oled.p_label, &lv_font_unscii_8, LV_PART_MAIN);
#endif
    lv_obj_set_style_text_align(gh_oled.p_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_text(gh_oled.p_label, p_text);
    lv_obj_align(gh_oled.p_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Display blinking text on the OLED.
 *
 * @param p_text Pointer to the null-terminated text string.
 * @param period_ms Blinking period in milliseconds.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_blink_text(const char *p_text, uint32_t period_ms)
{
    if ((p_text == NULL) || (gh_oled.p_label == NULL))
    {
        return ZERR_NOT_OPEN;
    }

    if (period_ms < 2U)
    {
        return ZERR_INVALID_PARAM;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

    lv_label_set_text(gh_oled.p_label, p_text);
    lv_obj_align(gh_oled.p_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_hidden(gh_oled.p_label, false);

    gh_oled.blink_visible = true;

    if (gh_oled.p_blink_timer != NULL)
    {
#if LVGL_VERSION_MAJOR >= 9
        lv_timer_delete(gh_oled.p_blink_timer);
#else
        lv_timer_del(gh_oled.p_blink_timer);
#endif
        gh_oled.p_blink_timer = NULL;
    }

    gh_oled.p_blink_timer = lv_timer_create(
        oled_lvgl_blink_timer_cb,
        period_ms / 2U,
        NULL);

    if (gh_oled.p_blink_timer == NULL)
    {
        lvgl_port_unlock();
        return ZERR_ERROR;
    }

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Stop blinking text and restore its visibility.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_stop_blink(void)
{
    if (gh_oled.p_label == NULL)
    {
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

    if (gh_oled.p_blink_timer != NULL)
    {
#if LVGL_VERSION_MAJOR >= 9
        lv_timer_delete(gh_oled.p_blink_timer);
#else
        lv_timer_del(gh_oled.p_blink_timer);
#endif
        gh_oled.p_blink_timer = NULL;
    }

    gh_oled.blink_visible = true;
    lv_obj_set_hidden(gh_oled.p_label, false);

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Show the loading spinner.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_show_spinner(void)
{
    if (gh_oled.p_disp == NULL)
    {
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

    if (gh_oled.p_spinner == NULL)
    {
        lv_obj_t *p_screen = lv_disp_get_scr_act(gh_oled.p_disp);

#if LVGL_VERSION_MAJOR >= 9
        gh_oled.p_spinner = lv_spinner_create(p_screen);
        if (gh_oled.p_spinner != NULL)
        {
            lv_spinner_set_anim_params(gh_oled.p_spinner, 1000, 270);
        }
#else
        gh_oled.p_spinner = lv_spinner_create(p_screen, 1000, 270);
#endif

        if (gh_oled.p_spinner == NULL)
        {
            lvgl_port_unlock();
            return ZERR_ERROR;
        }

        lv_obj_set_size(gh_oled.p_spinner, 32, 32);
        lv_obj_set_style_arc_width(gh_oled.p_spinner, 4, LV_PART_MAIN);
        lv_obj_set_style_arc_width(gh_oled.p_spinner, 4, LV_PART_INDICATOR);
        lv_obj_align(gh_oled.p_spinner, LV_ALIGN_CENTER, 0, -8);
    }
    else
    {
        lv_obj_set_hidden(gh_oled.p_spinner, false);
    }

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Hide the loading spinner.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_hide_spinner(void)
{
    if ((gh_oled.p_disp == NULL) || (gh_oled.p_spinner == NULL))
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s : code: %d", __FUNCTION__, ZERR_NOT_OPEN));
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s : code: %d", __FUNCTION__, ZERR_NOT_OPEN));
        return ZERR_NOT_OPEN;
    }

    lv_obj_set_hidden(gh_oled.p_spinner, true);

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Display a picture on the OLED.
 *
 * @param p_data Pointer to the picture data.
 * @param data_size Size of the picture data in bytes.
 * @param width Picture width in pixels.
 * @param height Picture height in pixels.
 *
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_show_picture(const char *p_data, size_t data_size, uint16_t width, uint16_t height)
{
    if ((p_data == NULL) || (data_size == 0U) ||
        (width == 0U) || (height == 0U) || (gh_oled.p_disp == NULL))
    {
        ZTRACE_DBG(TR_OLED_LVGL,
                ("ERROR %s: data=%p, data_size=%u, width=%u, height=%u, disp=%p",
                    __FUNCTION__,
                    p_data,
                    (unsigned int)data_size,
                    width,
                    height,
                    gh_oled.p_disp));
        return ZERR_INVALID_PARAM;
    }

// #if LVGL_VERSION_MAJOR >= 9
// #if !LV_USE_SVG || !LV_USE_VECTOR_GRAPHIC
//     ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s: SVG support is disabled", __FUNCTION__));
//     return ZERR_ERROR;
// #endif
// #endif

    if (!lvgl_port_lock(0))
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s : code: %d", __FUNCTION__, ZERR_NOT_OPEN));
        return ZERR_NOT_OPEN;
    }

#if LVGL_VERSION_MAJOR >= 9
    gh_oled.svg_picture.header.magic = LV_IMAGE_HEADER_MAGIC;
    gh_oled.svg_picture.header.cf = LV_COLOR_FORMAT_RAW;
    gh_oled.svg_picture.header.w = width;
    gh_oled.svg_picture.header.h = height;
#else
    gh_oled.svg_picture.header.always_zero = 0;
    gh_oled.svg_picture.header.cf = LV_IMG_CF_RAW;
    gh_oled.svg_picture.header.w = width;
    gh_oled.svg_picture.header.h = height;
#endif
    gh_oled.svg_picture.data_size = data_size;
    gh_oled.svg_picture.data = (const uint8_t *)p_data;

    if (gh_oled.p_picture == NULL)
    {
        lv_obj_t *p_screen = lv_disp_get_scr_act(gh_oled.p_disp);

#if LVGL_VERSION_MAJOR >= 9
        gh_oled.p_picture = lv_image_create(p_screen);
#else
        gh_oled.p_picture = lv_img_create(p_screen);
#endif

        if (gh_oled.p_picture == NULL)
        {
            lvgl_port_unlock();
            return ZERR_ERROR;
        }
    }

#if LVGL_VERSION_MAJOR >= 9
    lv_image_set_src(gh_oled.p_picture, &gh_oled.svg_picture);
#else
    lv_img_set_src(gh_oled.p_picture, &gh_oled.svg_picture);
#endif

    lv_obj_set_size(gh_oled.p_picture, width, height);
    lv_obj_align(gh_oled.p_picture, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_hidden(gh_oled.p_picture, false);

    if (gh_oled.p_label != NULL)
    {
        lv_obj_set_hidden(gh_oled.p_label, true);
    }

    ZTRACE_DBG(TR_OLED_LVGL, ("DEBUG %s: data_size=%u, size=%ux%u",
                __FUNCTION__, (unsigned int)data_size, width, height));

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 * @brief Hide the currently displayed picture.
 * @return ZERR_OK on success, otherwise an error code.
 */
zerr_t oled_lvgl_hide_picture(void)
{
    if ((gh_oled.p_disp == NULL) || (gh_oled.p_picture == NULL))
    {
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

    lv_obj_set_hidden(gh_oled.p_picture, true);

    if (gh_oled.p_label != NULL)
    {
        lv_obj_set_hidden(gh_oled.p_label, false);
    }

    lvgl_port_unlock();

    return ZERR_OK;
}

/**
 ***************************************************************************************************
 * @brief   Display an embedded GIF image.
 * @param   p_data      Pointer to embedded GIF data.
 * @param   data_size   Size of GIF data in bytes.
 * @return  ZERR_OK on success, otherwise an error code.
 ***************************************************************************************************
 */
zerr_t oled_lvgl_show_gif(const uint8_t *p_data, size_t data_size)
{
    if ((p_data == NULL) || (data_size == 0U) || (gh_oled.p_disp == NULL))
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s: invalid parameter", __FUNCTION__));
        return ZERR_INVALID_PARAM;
    }

#if !LV_USE_GIF
    ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s: LV_USE_GIF is disabled", __FUNCTION__));
    return ZERR_SUPPORT;
#else

    if (!lvgl_port_lock(0))
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("ERROR %s: failed to acquire LVGL lock", __FUNCTION__));
        return ZERR_ERROR;
    }

    lv_obj_t *p_screen = lv_display_get_screen_active(gh_oled.p_disp);

    if (p_screen == NULL)
    {
        lvgl_port_unlock();
        return ZERR_ERROR;
    }

    if (gh_oled.p_picture != NULL)
    {
        lv_obj_delete(gh_oled.p_picture);
        gh_oled.p_picture = NULL;
    }

    gh_oled.p_picture = lv_gif_create(p_screen);

    if (gh_oled.p_picture == NULL)
    {
        lvgl_port_unlock();
        return ZERR_ERROR;
    }

#if (LVGL_VERSION_MAJOR > 9) || ((LVGL_VERSION_MAJOR == 9) && (LVGL_VERSION_MINOR >= 3))
    lv_gif_set_color_format(gh_oled.p_picture, LV_COLOR_FORMAT_ARGB8888);
#endif

    /*
     * p_data is a raw GIF file buffer embedded with EMBED_FILES.
     * lv_gif_set_src() does not accept this buffer directly.
     * A compatible image descriptor or filesystem source is required.
     */

    lv_obj_center(gh_oled.p_picture);

    if (gh_oled.p_label != NULL)
    {
        lv_obj_set_hidden(gh_oled.p_label, true);
    }

    lvgl_port_unlock();

    return ZERR_OK;
#endif
}

/* Private functions -----------------------------------------------------------------------------*/

static zerr_t oled_lvgl_init(void)
{
    if (gh_oled.initialized)
    {
        return ZERR_OK;
    }

    zerr_t ret = global_i2c_bsp_open();
    if (ret != ZERR_OK)
    {
        return ret;
    }

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;

    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_I2C_ADDR,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
    };

    esp_err_t err = esp_lcd_new_panel_io_i2c_v1(
        global_i2c_bsp_get_port(),
        &io_config,
        &io_handle);

    if (err != ESP_OK)
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("Create OLED IO failed: %s", esp_err_to_name(err)));
        return ZERR_NOT_OPEN;
    }

    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = OLED_RST_PIN,
    };

#if OLED_BSP_SH1106
    err = esp_lcd_new_panel_sh1106(io_handle, &panel_config, &panel_handle);
#else
    err = esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle);
#endif

    if (err != ESP_OK)
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("Create OLED panel failed: %s", esp_err_to_name(err)));
        esp_lcd_panel_io_del(io_handle);
        return ZERR_NOT_OPEN;
    }

    err = esp_lcd_panel_reset(panel_handle);
    if (err == ESP_OK)
    {
        err = esp_lcd_panel_init(panel_handle);
    }

    if (err == ESP_OK)
    {
        err = esp_lcd_panel_disp_on_off(panel_handle, true);
    }

    if (err != ESP_OK)
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("Initialize OLED panel failed: %s", esp_err_to_name(err)));
        esp_lcd_panel_del(panel_handle);
        esp_lcd_panel_io_del(io_handle);
        return ZERR_NOT_OPEN;
    }

    const lvgl_port_cfg_t lvgl_config = ESP_LVGL_PORT_INIT_CONFIG();

    err = lvgl_port_init(&lvgl_config);
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE))
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("Initialize LVGL failed: %s", esp_err_to_name(err)));
        esp_lcd_panel_del(panel_handle);
        esp_lcd_panel_io_del(io_handle);
        return ZERR_NOT_OPEN;
    }

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = OLED_WIDTH * OLED_HEIGHT,
        .double_buffer = true,
        .hres = OLED_WIDTH,
        .vres = OLED_HEIGHT,
        .monochrome = true,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_I1,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = false,
#endif
            .sw_rotate = false,
        },
    };

    lv_disp_t *p_disp = lvgl_port_add_disp(&display_config);
    if (p_disp == NULL)
    {
        ZTRACE_DBG(TR_OLED_LVGL, ("Add LVGL display failed"));
        esp_lcd_panel_del(panel_handle);
        esp_lcd_panel_io_del(io_handle);
        return ZERR_NOT_OPEN;
    }

    if (!lvgl_port_lock(0))
    {
        return ZERR_NOT_OPEN;
    }

    lv_obj_t *p_screen = lv_disp_get_scr_act(p_disp);
    lv_obj_t *p_label = lv_label_create(p_screen);

    if (p_label == NULL)
    {
        lvgl_port_unlock();
        return ZERR_ERROR;
    }

    lv_label_set_text(p_label, "");

#if LVGL_VERSION_MAJOR >= 9
    lv_obj_set_width(p_label, lv_display_get_horizontal_resolution(p_disp));
#else
    lv_obj_set_width(p_label, p_disp->driver->hor_res);
#endif

    lv_obj_set_style_text_align(p_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(p_label, LV_ALIGN_CENTER, 0, 0);

    lvgl_port_unlock();

    gh_oled.p_io = io_handle;
    gh_oled.p_panel = panel_handle;
    gh_oled.p_disp = p_disp;
    gh_oled.p_label = p_label;
    gh_oled.initialized = true;

    return ZERR_OK;
}

/**
 * @brief LVGL 8/9 removed lv_obj_set_hidden(); hide or show through the HIDDEN flag instead.
 */
static void lv_obj_set_hidden(lv_obj_t *p_obj, bool hidden)
{
    if (hidden)
    {
        lv_obj_add_flag(p_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(p_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void oled_lvgl_blink_timer_cb(lv_timer_t *p_timer)
{
    (void)p_timer;

    if (gh_oled.p_label == NULL)
    {
        return;
    }

    gh_oled.blink_visible = !gh_oled.blink_visible;

    if (gh_oled.blink_visible)
    {
        lv_obj_set_hidden(gh_oled.p_label, false);
    }
    else
    {
        lv_obj_set_hidden(gh_oled.p_label, true);
    }
}

/* END OF FILE ************************************************************************************/
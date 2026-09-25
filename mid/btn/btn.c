/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : btn.c                                                                            
 * @dissaption      : Button driver board support package.                                           
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

#include "esp_timer.h"

#include "btn.h"

/* Define ----------------------------------------------------------------------------------------*/

#define SCAN_PERIOD_MS      10
#define DEBOUNCE_MS         20
#define LONG_PRESS_MS       800
#define DOUBLE_CLICK_MS     400

/* Private data types ----------------------------------------------------------------------------*/

typedef struct
{
    uint8_t stable_level;
    uint8_t last_level;

    int     debounce_cnt;

    int64_t press_time;
    int64_t last_release;

    bool wait_double;

} btn_state_t;

static btn_state_t          states[BTN_BSP_ID_MAX];
static btn_callback_t       callback;
static esp_timer_handle_t   btn_timer;

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/

static void btn_scan(void *arg);

/* Public function prototypes --------------------------------------------------------------------*/

void btn_open(btn_callback_t cb) {
    
    callback = cb;
    btn_bsp_open();

    const esp_timer_create_args_t timer_args =
    {
        .callback = btn_scan,
        .name = "btn_scan"
    };

    esp_timer_create(&timer_args, &btn_timer);
    
    esp_timer_start_periodic(btn_timer, SCAN_PERIOD_MS * 1000);
}

/* Private function prototypes -------------------------------------------------------------------*/
static void btn_scan(void *arg)
{
    int64_t now = esp_timer_get_time() / 1000;

    for (int id = 0; id < BTN_BSP_ID_MAX; id++)
    {
        gpio_num_t gpio = btn_bsp_get_gpio(id);

        int level = gpio_get_level(gpio);
        btn_state_t *st = &states[id];

        if (level != st->last_level)
        {
            st->debounce_cnt = 0;
            st->last_level = level;
        }
        else
        {
            if (st->debounce_cnt < (DEBOUNCE_MS / SCAN_PERIOD_MS))
                st->debounce_cnt++;
            else
            {
                if (st->stable_level != level)
                {
                    st->stable_level = level;

                    if (level == 0)
                    {
                        st->press_time = now;
                    }
                    else
                    {
                        int press_time = now - st->press_time;

                        if (press_time > LONG_PRESS_MS)
                        {
                            callback(id, BTN_EVENT_LONG);
                        }
                        else
                        {
                            if (st->wait_double &&
                                (now - st->last_release) < DOUBLE_CLICK_MS)
                            {
                                st->wait_double = false;
                                callback(id, BTN_EVENT_DOUBLE);
                            }
                            else
                            {
                                st->wait_double = true;
                                st->last_release = now;
                                callback(id, BTN_EVENT_SHORT);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* END OF FILE ************************************************************************************/
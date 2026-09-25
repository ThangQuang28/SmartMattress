/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : btn_bsp.c                                                                            
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

#include "btn_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

static const gpio_num_t btn_gpio_map[BTN_BSP_ID_MAX] =
{
    GPIO_NUM_4,   
    GPIO_NUM_5  
};

/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

void btn_bsp_open(void)
{
    for (int i = 0; i < BTN_BSP_ID_MAX; i++)
    {
        gpio_config_t io_conf =
        {
            .pin_bit_mask = (1ULL << btn_gpio_map[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .intr_type = GPIO_INTR_ANYEDGE
        };

        gpio_config(&io_conf);
    }
}

gpio_num_t btn_bsp_get_gpio(btn_bsp_id_t id)
{
    return btn_gpio_map[id];
}

/* END OF FILE ************************************************************************************/
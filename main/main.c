/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 28-mar-2026
 * @file            : main.c                                                                         
 * @dissaption      : File to create FW-Core . Coding in free time                                   
***************************************************************************************************************/

/*  Indent using spaces
    Tab width   : 4 spaces
    Line width  : 100 characters
    Line ending : LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/* Applicationware */
#include "zapp.h"

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    zapp_open();
}

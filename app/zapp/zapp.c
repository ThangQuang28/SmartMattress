/***************************************************************************************************************
 * @auth            : duonghd
 * @day             : 10-Mar-2026
 * @file            : zapp.c
 * @dissaption      : zapp module.
 * auther           : duonghd
 ***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

/* Include ---------------------------------------------------------------------------------------*/

/* Global configuration */
#include "zcfg.h"

/** Middleware */
#include "wifi_prov.h"
#include "mqtt_app.h"
#include "esp_at.h"
#include "ztime.h"
#include "zschedule.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

/**
 * @brief      Open zapp.
 * @param[in]  type: type to be open.
 * @retval     None.
 */
static void zapp_task(void *p_para)
{

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/* Public function prototypes --------------------------------------------------------------------*/

/**
 * @brief      Open task.
 * @param[in]  type: type to be configured.
 * @retval     None.
 */
void zapp_open(void)
{
    xTaskCreate(
        zapp_task,             /* Function that implements the task. */
        "zapp task",           /* Text name for the task. */
        ZAPP_STACK_DEEP_START, /* Stack deep in words, NOT bytes. */
        NULL,                  /* Parameter passed into the task. */
        ZAPP_PRIO_START,       /* Priority at which the task is created. */
        NULL                   /* Used to pass out the created task's handle.*/
    );
}

/* Private function prototypes -------------------------------------------------------------------*/

/* END OF FILE ************************************************************************************/

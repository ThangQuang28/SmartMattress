/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : aht20.c                                                                           
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
#include <time.h>

/* Middleware */
#include "zschedule.h"
/* BSP */
#include "esp_log.h"
#include "esp_netif_sntp.h"
/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
static fan_schedule_t gh_schedule[MAX_SCHEDULE];
static uint8_t gh_schedule_count = 0;
/* Public function prototypes --------------------------------------------------------------------*/

void zschedule_set(
    uint8_t index,
    uint8_t hour,
    uint8_t minute,
    uint8_t speed)
{
    if (index >= MAX_SCHEDULE)
    {
        return;
    }

    gh_schedule[index].hour = hour;
    gh_schedule[index].minute = minute;
    gh_schedule[index].speed = speed;
    gh_schedule[index].enable = true;
    gh_schedule[index].executed = false;

    if (index >= gh_schedule_count)
    {
        gh_schedule_count = index + 1;
    }
}


bool zschedule_get(
    uint8_t index,
    fan_schedule_t *schedule)
{
    if (index >= MAX_SCHEDULE)
    {
        return false;
    }

    *schedule = gh_schedule[index];

    return true;
}

void zschedule_set_executed(
    uint8_t index,
    bool executed)
{
    if (index >= MAX_SCHEDULE)
    {
        return;
    }

    gh_schedule[index].executed = executed;
}

void zschedule_clear(void)
{
    memset(
        gh_schedule,
        0,
        sizeof(gh_schedule));

    gh_schedule_count = 0;
}

/* END OF FILE ************************************************************************************/
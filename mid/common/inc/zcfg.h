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

#ifndef _ZCFG_H
#define _ZCFG_H

/* Public macros ---------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/

/* Include ---------------------------------------------------------------------------------------*/

/* ------------------------------
    ------- Common headers -------
    ------------------------------ */

/* C standard library. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <math.h>

/* RTOS. */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "ztrace.h"

/* -------------------------------------
    ------- Task priority options -------
    ------------------------------------- */

#define ZAPP_PRIO_BLUETOOTH         (tskIDLE_PRIORITY + 5u) /* Highest priority */
#define ZAPP_PRIO_FIREBASE          (tskIDLE_PRIORITY + 3u)
#define ZAPP_PRIO_ESPNOW            (tskIDLE_PRIORITY + 3u)
#define ZAPP_PRIO_START             (tskIDLE_PRIORITY + 2u) 
#define ZAPP_PRIO_ZTIME             (tskIDLE_PRIORITY + 2u)

/* IDLE task has the lowest priority (priority = 0) in the system. */

/* ---------------------------------------
    ------- Task stack deep options -------
    --------------------------------------- */
#define ZAPP_STACK_DEEP_START       (configMINIMAL_STACK_SIZE * 15u)
#define ZAPP_STACK_BLUETOOTH        (configMINIMAL_STACK_SIZE * 5u)
#define ZAPP_STACK_FIREBASE         (configMINIMAL_STACK_SIZE * 10u)
#define ZAPP_STACK_ESPNOW           (configMINIMAL_STACK_SIZE * 5u)
#define ZAPP_STACK_ZTIME             (configMINIMAL_STACK_SIZE * 2u)


#ifdef __cplusplus
}
#endif

#endif  /* _ZCFG_H */

/* END OF FILE ************************************************************************************/

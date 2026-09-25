/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : btn.h                                                                            
 * @dissaption      : Button driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _BTN_H
#define _BTN_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#include "btn_bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef enum
{
    BTN_EVENT_SHORT,
    BTN_EVENT_LONG,
    BTN_EVENT_DOUBLE
} btn_event_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

typedef void (*btn_callback_t)(btn_bsp_id_t id, btn_event_t event);

void btn_open(btn_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif /* _BTN_H */

/* END OF FILE ************************************************************************************/

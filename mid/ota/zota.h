/***************************************************************************************************************
 * @auth            : duonghd                                                                            
 * @day             : 10-Mar-2026
 * @file            : ota.h                                                                          
 * @dissaption      : Ota module.                                 
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _ZOTA_H
#define _ZOTA_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include "zerr.h"

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/

typedef enum {
    ZOTA_STATE_CLOSED = 0,
    ZOTA_STATE_OPEN,
    ZOTA_STATE_DOWNLOADING,
    ZOTA_STATE_SUCCESS,
    ZOTA_STATE_ERROR
} zota_state_t;

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t zota_open(void);

zerr_t zota_start(const char* p_url);

zerr_t zota_update(const char* p_url);

zerr_t zota_get_state(void);

bool zota_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* _ZOTA_H */

/* END OF FILE ************************************************************************************/
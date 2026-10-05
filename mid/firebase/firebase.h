/***************************************************************************************************************
 * @auth            : duongdh                                                                                  
 * @day             : 4-Apr-2025
 * @file            : firebase.h                                                                            
 * @dissaption      : File to create FW-Core . Coding in free time                                              
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _FIREBASE_H
#define _FIREBASE_H

#ifdef __cplusplus__
extern "C" {
#endif

#include "zerr.h"
#include "esp_http_client.h"

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

/* Global configuration */

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef void (*firebase_callback_t)(const char *p_data, uint32_t data_len);

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

zerr_t firebase_open(void);

void firebase_set_reg(firebase_callback_t callback);

void firebase_get_data(void);
void firebase_put_data(const char *json_string);

#ifdef __cplusplus__
}
#endif

#endif /* _FIREBASE_H */
/* END OF FILE ************************************************************************************/
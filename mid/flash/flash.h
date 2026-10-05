/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : flash.h                                                                            
 * @dissaption      : Flash driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _FLASH_H
#define _FLASH_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "zerr.h"

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

zerr_t flash_open(void);

zerr_t flash_write(const char *p_key, const void *p_data, size_t size);

zerr_t flash_read(const char *p_key, void *p_data, size_t size, size_t *p_read_size);

zerr_t flash_delete(const char *p_key);

#ifdef __cplusplus
}
#endif

#endif /* _FLASH_H */

/* END OF FILE ************************************************************************************/

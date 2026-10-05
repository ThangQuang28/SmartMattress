/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : flash_bsp.h                                                                            
 * @dissaption      : DC motor driver board support package.                                           
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _FLASH_BSP_H
#define _FLASH_BSP_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stdint.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "zerr.h"
/* Define ----------------------------------------------------------------------------------------*/

#define FLASH_BSP_NAMESPACE       "config"
#define FLASH_BSP_KEY_JSON        "json"

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

zerr_t flash_bsp_open(void);

zerr_t flash_bsp_write(const char *p_key, const void *p_data, size_t size);

zerr_t flash_bsp_read(const char *p_key, void *p_data, size_t size, size_t *p_read_size);

zerr_t flash_bsp_delete(const char *p_key);

#ifdef __cplusplus
}
#endif

#endif /* _FLASH_BSP_H */

/* END OF FILE ************************************************************************************/

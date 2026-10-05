/***************************************************************************************************************
 * @auth            : duonghd                                                                                
 * @day             : 10-Mar-2026
 * @file            : flash.c                                                                           
 * @dissaption      : Flash driver board support package.                                           
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

/* BSP. */
#include "flash_bsp.h"

/* Define ----------------------------------------------------------------------------------------*/

/* Private data types ----------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Private function prototypes -------------------------------------------------------------------*/

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t flash_open(void)
{
    return flash_bsp_open();
}

zerr_t flash_write(const char *p_key, const void *p_data, size_t size)
{
    return flash_bsp_write(p_key, p_data, size);
}

zerr_t flash_read(const char *p_key, void *p_data, size_t size, size_t *p_read_size)
{
    return flash_bsp_read(p_key, p_data, size, p_read_size);
}

zerr_t flash_delete(const char *p_key)
{
    return flash_bsp_delete(p_key);
}

/* Private function prototypes -------------------------------------------------------------------*/

/* END OF FILE ************************************************************************************/
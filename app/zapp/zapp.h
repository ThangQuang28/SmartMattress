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

#ifndef _ZAPP_H
#define _ZAPP_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/
/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/
/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

/**
     * @brief      Open task.
     * @param[in]  type: type to be configured.
     * @retval     None.
     */
void zapp_open(void);

#ifdef __cplusplus
}
#endif

#endif /* _ZAPP_H */

/* END OF FILE ************************************************************************************/
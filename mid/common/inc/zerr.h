/**
  **************************************************************************************************
  * @file   zerr.h
  * @brief  Header file for zerr module.
  * @by     duonghd | 22-Step-2026 |
  **************************************************************************************************
  */

/*  Indent using spaces
    Tab width   : 4 spaces
    Line width  : 100 characters
    Line ending : LF (0x0A) */

#ifndef _ZERR_H
#define _ZERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Public macros ---------------------------------------------------------------------------------*/
#define ZERR(x)             zerr_get_str_err(x)

/* Public data types -----------------------------------------------------------------------------*/

/* Module definition. */
typedef enum
{
    ZERR_MODULE_EVTCAP,
    ZERR_MODULE_MIFARE,
    ZERR_MODULE_FINGER,
    ZERR_MODULE_TBED,

    ZERR_MODULE_XXX
} zerr_module_t;

/* Object definition. */
typedef enum
{
    ZERR_OBJECT_MIFARE,
    ZERR_OBJECT_FINGER,

    ZERR_OBJECT_RFU
} zerr_object_t;

/* Error code definition. */
typedef enum
{
    /* Common error code. */
    ZERR_OK                         = 0u,   /* Success. */
    ZERR_OPEN_ALREADY               = 1u,   /* Has been already opened. */
    ZERR_INVALID_PARAM              = 2u,   /* Parameter is NULL. */
    ZERR_MEM_OUT,                           /* Out of memory. */
    ZERR_TIME_OUT,                          /* Out of time. */
    ZERR_NOT_OPEN,                          /* Has not been opened yet or unable to open. */
    ZERR_STORAGE_OUT,                       /* Out of storage. */
    ZERR_EXIST,                             /* Item exists. */
    ZERR_ERROR,

    /* After adding an error code, MUST go to zerr.c to add a string representation for it. */

    ZERR_EMPTY,                             /**< Storage is empty */
    ZERR_FULL,                              /**< Storage is full */
    ZERR_TX,                                /**< Unable to transmit */
    ZERR_RX,                                /**< Unable to receive */
    ZERR_NOT_FOUND,                         /**< Not found */
    ZERR_UNPLUG,                            /**< Unplug */

    ZERR_SUPPORT                            /**< Don't support */
} zerr_t;

/* Public function prototypes --------------------------------------------------------------------*/

/**
  * @brief      Convert an error to a string representation.
  * @param[in]  error_code: an integer indicates an error.
  * @retval     A string representation for such error.
  */
const char*
zerr_get_str_err (zerr_t error_code);

/**
  * @brief      Display system error on LCD and suspend the calling task.
  * @param[in]  module_code: module gets error.
  * @param[in]  object_code: object in evtcap gets error.
  * @param[in]  error_code: error code.
  * @retval     None.
  */
void
zerr_suspend (zerr_module_t module_code, zerr_object_t object_code, zerr_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* _ZERR_H */

/* END OF FILE ************************************************************************************/

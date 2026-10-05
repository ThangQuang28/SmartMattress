/**
  **************************************************************************************************
  * @file   zerr.c
  * @brief  Header file for zerr module.
  * @by     duonghd | 22-Step-2026 |
  **************************************************************************************************
  */

/*  Indent using spaces
    Tab width   : 4 spaces
    Line width  : 100 characters
    Line ending : LF (0x0A) */


/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration. */
#include "zcfg.h"

#include "zerr.h"

/* Private macros --------------------------------------------------------------------------------*/
#define ZERR_MAX_LEN_STR_ERR          8 /* Maximum length of error string displayed on LCD. */

/* Private data types ----------------------------------------------------------------------------*/

/* Error information definition. */
typedef struct
{
    uint32_t offset;    /* Position of the string representation. */
    zerr_t   code;      /* Error code. */
} zerr_info_t;

/* Private variables -----------------------------------------------------------------------------*/

const uint32_t g_num_err = 11;    /* The number of elements in g_err_info. */

/* Error information. */
const zerr_info_t g_err_info[] = {
    {0, ZERR_OK},
    {1, ZERR_OPEN_ALREADY},
    {2, ZERR_INVALID_PARAM},
    {3, ZERR_MEM_OUT},
    {4, ZERR_TIME_OUT},
    {5, ZERR_NOT_OPEN},
    {6, ZERR_STORAGE_OUT},
    {7, ZERR_EXIST},

    /* Modify g_num_err variable if add more any elements. */
};

/* String representation for error. */
static const char* gp_str_err[] = {
    /*  0  */ "ok.",
    /*  1  */ "has been already opened.",
    /*  2  */ "invalid parameter!",
    /*  3  */ "out of memory!",
    /*  4  */ "out of time!",
    /*  5  */ "not opened yet!",
    /*  6  */ "out of storage!",
    /*  7  */ "exist!",
    /*  8  */ "",
    /*  9  */ "",
    /*  10 */ "",
    /*  11 */ "",
    /*  12 */ "",
    /*  13 */ "",
    /*  14 */ "",
    /*  15 */ "",
    /*  16 */ "",
    /*  17 */ "",
    /*  18 */ "",
    /*  19 */ "",
    /* Add a string representation for an appropriate error with respect for it's order. */
};

/* Private function prototypes -------------------------------------------------------------------*/

/* Public function bodies ------------------------------------------------------------------------*/

/**
  * @brief      Convert an error to a string representation.
  * @param[in]  error_code: an integer indicates an error.
  * @retval     A string representation for such error.
  */
const char*
zerr_get_str_err (zerr_t error_code)
{
    uint32_t i = 0;
    const char* str_err = "unknown error!";

    /* Look up the array of error information. */
    for (; i < g_num_err; ++i)
    {
        if (g_err_info[i].code == error_code)
        {
            /* Found error code. */
            str_err = gp_str_err[g_err_info[i].offset];
            break;
        }
    }

    return str_err;
}

/**
  * @brief      Display system error on LCD and suspend the calling task.
  * @param[in]  module_code: module gets error.
  * @param[in]  object_code: object in evtcap gets error.
  * @param[in]  error_code: error code.
  * @retval     None.
  */
void
zerr_suspend (zerr_module_t module_code, zerr_object_t object_code, zerr_t error_code)
{
    char str_err[ZERR_MAX_LEN_STR_ERR] = {0};
    uint8_t parse_module_code = 0;
    uint8_t parse_error_code = 0;
    uint8_t parse_object_code = 0;

    str_err[0] = 'S'; /* Indicator for system error. */

    /* Fill module code. */
    switch (module_code)
    {
        case ZERR_MODULE_EVTCAP:
            parse_module_code = 0x01;
        break;

        case ZERR_MODULE_MIFARE:
            parse_module_code = 0x02;
        break;

        case ZERR_MODULE_FINGER:
            parse_module_code = 0x03;
        break;

        default:
            configASSERT(0);
        break;
    }
    snprintf(str_err + 1, ZERR_MAX_LEN_STR_ERR - 1, "%02x", parse_module_code);

    /* Fill object code. */
    if (0x01 != parse_module_code) /* Not evtcap module. */
    {
        parse_object_code = 0xFF; /* Indicate for no meaning. */
    }
    else
    {
        switch (object_code)
        {
            case ZERR_OBJECT_MIFARE:
                parse_object_code = 0x01;
            break;
            case ZERR_OBJECT_FINGER:
                parse_object_code = 0x02;
            break;

            default:
                configASSERT(0);
            break;
        }
    }
    snprintf(str_err + 3, ZERR_MAX_LEN_STR_ERR - 3, "%02x", parse_object_code);

    /* Fill error code. */
    switch (error_code)
    {
        case ZERR_INVALID_PARAM:
            parse_error_code = 0x01;
        break;

        case ZERR_MEM_OUT:
            parse_error_code = 0x02;
        break;

        case ZERR_NOT_OPEN:
            parse_error_code = 0x03;
        break;

        default:
            configASSERT(0);
        break;
    }
    snprintf(str_err + 5, ZERR_MAX_LEN_STR_ERR - 5, "%02x", parse_error_code);
    // evtcap_set_error_script((uint8_t*)str_err);
    // scr_clr();
    // scr_write("Error: ", 0);
    // scr_write((const uint8_t*)str_err, 0);

    vTaskSuspend(NULL);
}

/* Private function bodies -----------------------------------------------------------------------*/

/* END OF FILE ************************************************************************************/

/**
  **************************************************************************************************
  * @file   evtcap.h
  * @author duonghd
  * @date   22-Step-2026
  * @brief  Header file
  *         Dependences: error.h
  **************************************************************************************************
  */

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef _EVTCAP_H
#define _EVTCAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zerr.h"

/* Public macros ---------------------------------------------------------------------------------*/

/* Public data types -----------------------------------------------------------------------------*/

/* Public function prototypes --------------------------------------------------------------------*/

zerr_t eventcap_open(void);

zerr_t open_http_offline(void);

#ifdef __cplusplus
}
#endif

#endif /* _EVTCAP_H */

/* END OF FILE ************************************************************************************/
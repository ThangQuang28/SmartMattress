/***************************************************************************************************************
 * @auth            : duonghd
 * @file            : captive_dns.h
 * @description     : Captive portal DNS for the device WiFi (every name -> 192.168.4.1).
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

#ifndef CAPTIVE_DNS_H
#define CAPTIVE_DNS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------------*/
#include "zerr.h"

/* Public functions ------------------------------------------------------------------------------*/

/**
 * @brief  Start the DNS task on UDP port 53.
 */
zerr_t captive_dns_open(void);

#ifdef __cplusplus
}
#endif

#endif /* CAPTIVE_DNS_H */

/* END OF FILE ************************************************************************************/

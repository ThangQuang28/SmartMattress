/***************************************************************************************************************
 * @auth            : duonghd                                                                            
 * @day             : 16-Mar-2026
 * @file            : package.c                                                                          
 * @dissaption      : package module.                                     
***************************************************************************************************************/

/*  Indent using spaces
    Tab width: 4 spaces
    Line width: 100 characters
    Line ending: LF (0x0A) */

/*  Global configuration
    C standard library
    Applicationware
    wCat_FWCore V1.00 */

#ifndef _PACKAGE_H
#define _PACKAGE_H

/* Public macros ---------------------------------------------------------------------------------*/
/* Include ---------------------------------------------------------------------------------------*/

#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------------*/

#define PKG_SIZE(pkt) ((pkt)->len + 4)

#define PKG_SOF        0xAA
#define PKG_MAX_DATA   64

/* Private data types ----------------------------------------------------------------------------*/
/* Public data types -----------------------------------------------------------------------------*/

typedef struct __attribute__((packed))
{
    uint8_t sof;                 // 0xAA
    uint8_t cmd;                 // command
    uint8_t len;                 // length data
    uint8_t data[PKG_MAX_DATA];  // payload
    uint8_t crc;                 // checksum
} pkg_t;

typedef enum {
    PKG_ERR_OK=0u,
    PKG_ERR_NULL,
    PKG_ERR_LEN,
    PKG_ERR_UNKOWN
}pkg_err_t;

/* Private function prototypes -------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

int pkg_build(uint8_t *buf, uint8_t cmd, uint8_t *data, uint8_t len);

int pkg_parse(pkg_t *pkt, uint8_t *buf, int len);

void pkg_print(pkg_t *pkt);

void pkg_dump_buf(const char *tag, const uint8_t *buf, int len);

void motor_build_pack(uint8_t *data,
                uint8_t dir1, uint8_t en1, uint8_t brk1, uint8_t speed1,
                uint8_t dir2, uint8_t en2, uint8_t brk2, uint8_t speed2);
                
#ifdef __cplusplus
}
#endif

#endif /* _PACKAGE_H */

/* END OF FILE ************************************************************************************/
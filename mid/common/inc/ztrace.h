/**
  **************************************************************************************************
  * @file   trace.h
  * @brief  Starting point of this project.
  * @by     duonghd | 31-Dec-2024 | Launch for WhiteCat-FWCore.
  **************************************************************************************************
  * This file is part of WhiteCat-FWCore.
  **************************************************************************************************
  */

/*  Indent using spaces
    Tab width   : 4 spaces
    Line width  : 100 characters
    Line ending : LF (0x0A) */

#ifndef _ZTRACE_H
#define _ZTRACE_H

/* Includes --------------------------------------------------------------------------------------*/

/* Global configuration. */

/* Middleware. */

#ifdef __cplusplus
extern "C" {
#endif

#if defined (ZASSERT)
#define ZTRACE_ASSERT(expr)     if ((expr) == 0) { ztrace_assert(); }
#else   /* WASSERT */
#define ZTRACE_ASSERT(expr)
#endif  /* WASSERT */

#define ZTRACE_ON       0x80
#define ZTRACE_OFF      0x00

#define ZTRACE_UNWRAP(...) __VA_ARGS__

/* Per-module debug switches used by ZTRACE_DBG / ZTRACE_ERR (active only with ZDEBUG / ZERROR). */
#define TR_EVTCAP       ZTRACE_ON
#define TR_FIREBASE     ZTRACE_ON
#define TR_IIC_BSP      ZTRACE_ON
#define TR_OLED_LVGL    ZTRACE_ON
#define TR_OTA          ZTRACE_ON
#define TR_TCA          ZTRACE_ON

#define ZTRACE_LOGI(tag, args)              \
    do {                                    \
        ESP_LOGI(tag, ZTRACE_UNWRAP args); \
    } while (0)

#define ZTRACE_LOGE(tag, args)              \
do {                                    \
    ESP_LOGE(tag, ZTRACE_UNWRAP args); \
} while (0)

#if defined (ZERROR)
/* If "expr" isn't true, then print "msg" and execute "hdl" expression. */
#define ZTRACE_ERR(dbg, msg, expr, hdl)     do {                                                \
												if (!(expr))                                    \
												{                                               \
													if ((dbg) & ZTRACE_ON)                      \
													{                                           \
														zlog_printf msg;						\
													}                                           \
													hdl;                                        \
												}                                               \
											} while (0);
#else   /* VERROR */
/* If "expr" isn't true, then execute "hdl" expression. */
#define ZTRACE_ERR(dbg, msg, expr, hdl)     do {                                                \
												if (!(expr))                                    \
												{                                               \
                                                    ZTRACE_LOGE("", msg);							\
													hdl;                                        \
												}                                               \
											} while (0);
#endif  /* VERROR */

#if defined (ZDEBUG)
/* If "dbg" flag is ZTRACE_ON, then print "msg". */

#define ZTRACE_DBG(dbg, msg)    do {                                                            \
									if ((dbg) & ZTRACE_ON)                                      \
									{                                                           \
										ZTRACE_LOGI("", msg);										\
									}                                                           \
								} while (0);

/* If "dbg" flag is ZTRACE_ON, then print "msg", use in isr context. */
#define ZTRACE_DBG_ISR(dbg, msg)    do {                                                        \
										if ((dbg) & ZTRACE_ON)                                  \
										{                                                       \
										}                                                       \
									} while (0);
#else   /* ZDEBUG */
#define ZTRACE_DBG(dbg, msg)
#define ZTRACE_DBG_ISR(dbg, msg)
#endif  /* ZDEBUG */

/* Public data types -----------------------------------------------------------------------------*/
/* Public function prototypes --------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _TRACE_H */

/* END OF FILE ************************************************************************************/
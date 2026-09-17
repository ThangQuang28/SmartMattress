/*
 * MyMKL46Z.h
 *
 *  Created on: Oct 20, 2023
 *      Author: phucc
 */

#ifndef MYMKL64Z_H_
#define MYMKL64Z_H_
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef int int32_t;

typedef enum {
	true,
	false
}bool;
/* IO definitions (access restrictions to peripheral registers) */
#ifdef __cplusplus
  #define   __I     volatile/*!< Defines 'read only' permissions */
#else
  #define   __I     volatile const/*!< Defines 'read only' permissions */
#endif
#define     __O     volatile/*!< Defines 'write only' permissions */
#define     __IO    volatile/*!< Defines 'read / write' permissions */

/* following defines should be used for structure members */
#define     __IM     volatile const/*! Defines 'read only' structure member permissions */
#define     __OM     volatile/*! Defines 'write only' structure member permissions */
#define     __IOM    volatile/*! Defines 'read / write' structure member permissions */

/*@} end of group ARMv8MBL */
/* ----------------------------------------------------------------------------
   -- GPIO Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/** GPIO - Register Layout Typedef */
typedef struct {
  __IO uint32_t PDOR;/**< Port Data Output Register, offset: 0x0 */
  __O  uint32_t PSOR;/**< Port Set Output Register, offset: 0x4 */
  __O  uint32_t PCOR;/**< Port Clear Output Register, offset: 0x8 */
  __O  uint32_t PTOR;/**< Port Toggle Output Register, offset: 0xC */
  __I  uint32_t PDIR;/**< Port Data Input Register, offset: 0x10 */
  __IO uint32_t PDDR;/**< Port Data Direction Register, offset: 0x14 */
} GPIO_Type;

/* ----------------------------------------------------------------------------
   -- GPIO Register Masks
   ---------------------------------------------------------------------------- */

/*! @name PDOR - Port Data Output Register */
#define GPIO_PDOR_PDO_MASK                       (0xFFFFFFFFU)
#define GPIO_PDOR_PDO_SHIFT                      (0U)
#define GPIO_PDOR_PDO(x)                         (((uint32_t)(((uint32_t)(x)) << GPIO_PDOR_PDO_SHIFT)) & GPIO_PDOR_PDO_MASK)

/*! @name PSOR - Port Set Output Register */
#define GPIO_PSOR_PTSO_MASK                      (0xFFFFFFFFU)
#define GPIO_PSOR_PTSO_SHIFT                     (0U)
#define GPIO_PSOR_PTSO(x)                        (((uint32_t)(((uint32_t)(x)) << GPIO_PSOR_PTSO_SHIFT)) & GPIO_PSOR_PTSO_MASK)

/*! @name PCOR - Port Clear Output Register */
#define GPIO_PCOR_PTCO_MASK                      (0xFFFFFFFFU)
#define GPIO_PCOR_PTCO_SHIFT                     (0U)
#define GPIO_PCOR_PTCO(x)                        (((uint32_t)(((uint32_t)(x)) << GPIO_PCOR_PTCO_SHIFT)) & GPIO_PCOR_PTCO_MASK)

/*! @name PTOR - Port Toggle Output Register */
#define GPIO_PTOR_PTTO_MASK                      (0xFFFFFFFFU)
#define GPIO_PTOR_PTTO_SHIFT                     (0U)
#define GPIO_PTOR_PTTO(x)                        (((uint32_t)(((uint32_t)(x)) << GPIO_PTOR_PTTO_SHIFT)) & GPIO_PTOR_PTTO_MASK)

/*! @name PDIR - Port Data Input Register */
#define GPIO_PDIR_PDI_MASK                       (0xFFFFFFFFU)
#define GPIO_PDIR_PDI_SHIFT                      (0U)
#define GPIO_PDIR_PDI(x)                         (((uint32_t)(((uint32_t)(x)) << GPIO_PDIR_PDI_SHIFT)) & GPIO_PDIR_PDI_MASK)

/*! @name PDDR - Port Data Direction Register */
#define GPIO_PDDR_PDD_MASK                       (0xFFFFFFFFU)
#define GPIO_PDDR_PDD_SHIFT                      (0U)
#define GPIO_PDDR_PDD(x)                         (((uint32_t)(((uint32_t)(x)) << GPIO_PDDR_PDD_SHIFT)) & GPIO_PDDR_PDD_MASK)

/* end of group GPIO_Register_Masks */

/* GPIO - Peripheral instance base addresses */
/** Peripheral GPIOA base address */
#define GPIOA_BASE                               (0x400FF000u)
/** Peripheral GPIOA base pointer */
#define GPIOA                                    ((GPIO_Type *)GPIOA_BASE)
/** Peripheral GPIOB base address */
#define GPIOB_BASE                               (0x400FF040u)
/** Peripheral GPIOB base pointer */
#define GPIOB                                    ((GPIO_Type *)GPIOB_BASE)
/** Peripheral GPIOC base address */
#define GPIOC_BASE                               (0x400FF080u)
/** Peripheral GPIOC base pointer */
#define GPIOC                                    ((GPIO_Type *)GPIOC_BASE)
/** Peripheral GPIOD base address */
#define GPIOD_BASE                               (0x400FF0C0u)
/** Peripheral GPIOD base pointer */
#define GPIOD                                    ((GPIO_Type *)GPIOD_BASE)
/** Peripheral GPIOE base address */
#define GPIOE_BASE                               (0x400FF100u)
/** Peripheral GPIOE base pointer */
#define GPIOE                                    ((GPIO_Type *)GPIOE_BASE)
/** Array initializer of GPIO peripheral base addresses */
#define GPIO_BASE_ADDRS                          { GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE, GPIOE_BASE }
/** Array initializer of GPIO peripheral base pointers */
#define GPIO_BASE_PTRS                           { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE }

/* end of group GPIO_Peripheral_Access_Layer */

/* ----------------------------------------------------------------------------
   -- SIM Peripheral Access Layer
   ---------------------------------------------------------------------------- */
/** SIM - Register Layout Typedef */

typedef struct {
  __IO uint32_t SOPT1;/**< System Options Register 1, offset: 0x0 */
  __IO uint32_t SOPT1CFG;/**< SOPT1 Configuration Register, offset: 0x4 */
       uint8_t RESERVED_0[4092];
  __IO uint32_t SOPT2;/**< System Options Register 2, offset: 0x1004 */
       uint8_t RESERVED_1[4];
  __IO uint32_t SOPT4;/**< System Options Register 4, offset: 0x100C */
  __IO uint32_t SOPT5;/**< System Options Register 5, offset: 0x1010 */
       uint8_t RESERVED_2[4];
  __IO uint32_t SOPT7;/**< System Options Register 7, offset: 0x1018 */
       uint8_t RESERVED_3[8];
  __I  uint32_t SDID;/**< System Device Identification Register, offset: 0x1024 */
       uint8_t RESERVED_4[12];
  __IO uint32_t SCGC4;/**< System Clock Gating Control Register 4, offset: 0x1034 */
  __IO uint32_t SCGC5;/**< System Clock Gating Control Register 5, offset: 0x1038 */
  __IO uint32_t SCGC6;/**< System Clock Gating Control Register 6, offset: 0x103C */
  __IO uint32_t SCGC7;/**< System Clock Gating Control Register 7, offset: 0x1040 */
  __IO uint32_t CLKDIV1;/**< System Clock Divider Register 1, offset: 0x1044 */
       uint8_t RESERVED_5[4];
  __IO uint32_t FCFG1;/**< Flash Configuration Register 1, offset: 0x104C */
  __I  uint32_t FCFG2;/**< Flash Configuration Register 2, offset: 0x1050 */
       uint8_t RESERVED_6[4];
  __I  uint32_t UIDMH;/**< Unique Identification Register Mid-High, offset: 0x1058 */
  __I  uint32_t UIDML;/**< Unique Identification Register Mid Low, offset: 0x105C */
  __I  uint32_t UIDL;/**< Unique Identification Register Low, offset: 0x1060 */
       uint8_t RESERVED_7[156];
  __IO uint32_t COPC;/**< COP Control Register, offset: 0x1100 */
  __O  uint32_t SRVCOP;/**< Service COP, offset: 0x1104 */
} SIM_Type;

/* ----------------------------------------------------------------------------
   -- SIM Register Masks
   ---------------------------------------------------------------------------- */

/*! @name SOPT1 - System Options Register 1 */
#define SIM_SOPT1_OSC32KSEL_MASK                 (0xC0000U)
#define SIM_SOPT1_OSC32KSEL_SHIFT                (18U)
#define SIM_SOPT1_OSC32KSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_OSC32KSEL_SHIFT)) & SIM_SOPT1_OSC32KSEL_MASK)
#define SIM_SOPT1_USBVSTBY_MASK                  (0x20000000U)
#define SIM_SOPT1_USBVSTBY_SHIFT                 (29U)
#define SIM_SOPT1_USBVSTBY(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_USBVSTBY_SHIFT)) & SIM_SOPT1_USBVSTBY_MASK)
#define SIM_SOPT1_USBSSTBY_MASK                  (0x40000000U)
#define SIM_SOPT1_USBSSTBY_SHIFT                 (30U)
#define SIM_SOPT1_USBSSTBY(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_USBSSTBY_SHIFT)) & SIM_SOPT1_USBSSTBY_MASK)
#define SIM_SOPT1_USBREGEN_MASK                  (0x80000000U)
#define SIM_SOPT1_USBREGEN_SHIFT                 (31U)
#define SIM_SOPT1_USBREGEN(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1_USBREGEN_SHIFT)) & SIM_SOPT1_USBREGEN_MASK)

/*! @name SOPT1CFG - SOPT1 Configuration Register */
#define SIM_SOPT1CFG_URWE_MASK                   (0x1000000U)
#define SIM_SOPT1CFG_URWE_SHIFT                  (24U)
#define SIM_SOPT1CFG_URWE(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1CFG_URWE_SHIFT)) & SIM_SOPT1CFG_URWE_MASK)
#define SIM_SOPT1CFG_UVSWE_MASK                  (0x2000000U)
#define SIM_SOPT1CFG_UVSWE_SHIFT                 (25U)
#define SIM_SOPT1CFG_UVSWE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1CFG_UVSWE_SHIFT)) & SIM_SOPT1CFG_UVSWE_MASK)
#define SIM_SOPT1CFG_USSWE_MASK                  (0x4000000U)
#define SIM_SOPT1CFG_USSWE_SHIFT                 (26U)
#define SIM_SOPT1CFG_USSWE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT1CFG_USSWE_SHIFT)) & SIM_SOPT1CFG_USSWE_MASK)

/*! @name SOPT2 - System Options Register 2 */
#define SIM_SOPT2_RTCCLKOUTSEL_MASK              (0x10U)
#define SIM_SOPT2_RTCCLKOUTSEL_SHIFT             (4U)
#define SIM_SOPT2_RTCCLKOUTSEL(x)                (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_RTCCLKOUTSEL_SHIFT)) & SIM_SOPT2_RTCCLKOUTSEL_MASK)
#define SIM_SOPT2_CLKOUTSEL_MASK                 (0xE0U)
#define SIM_SOPT2_CLKOUTSEL_SHIFT                (5U)
#define SIM_SOPT2_CLKOUTSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_CLKOUTSEL_SHIFT)) & SIM_SOPT2_CLKOUTSEL_MASK)
#define SIM_SOPT2_PLLFLLSEL_MASK                 (0x10000U)
#define SIM_SOPT2_PLLFLLSEL_SHIFT                (16U)
#define SIM_SOPT2_PLLFLLSEL(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_PLLFLLSEL_SHIFT)) & SIM_SOPT2_PLLFLLSEL_MASK)
#define SIM_SOPT2_USBSRC_MASK                    (0x40000U)
#define SIM_SOPT2_USBSRC_SHIFT                   (18U)
#define SIM_SOPT2_USBSRC(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_USBSRC_SHIFT)) & SIM_SOPT2_USBSRC_MASK)
#define SIM_SOPT2_TPMSRC_MASK                    (0x3000000U)
#define SIM_SOPT2_TPMSRC_SHIFT                   (24U)
#define SIM_SOPT2_TPMSRC(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_TPMSRC_SHIFT)) & SIM_SOPT2_TPMSRC_MASK)
#define SIM_SOPT2_UART0SRC_MASK                  (0xC000000U)
#define SIM_SOPT2_UART0SRC_SHIFT                 (26U)
#define SIM_SOPT2_UART0SRC(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT2_UART0SRC_SHIFT)) & SIM_SOPT2_UART0SRC_MASK)

/*! @name SOPT4 - System Options Register 4 */
#define SIM_SOPT4_TPM1CH0SRC_MASK                (0xC0000U)
#define SIM_SOPT4_TPM1CH0SRC_SHIFT               (18U)
#define SIM_SOPT4_TPM1CH0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM1CH0SRC_SHIFT)) & SIM_SOPT4_TPM1CH0SRC_MASK)
#define SIM_SOPT4_TPM2CH0SRC_MASK                (0x100000U)
#define SIM_SOPT4_TPM2CH0SRC_SHIFT               (20U)
#define SIM_SOPT4_TPM2CH0SRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM2CH0SRC_SHIFT)) & SIM_SOPT4_TPM2CH0SRC_MASK)
#define SIM_SOPT4_TPM0CLKSEL_MASK                (0x1000000U)
#define SIM_SOPT4_TPM0CLKSEL_SHIFT               (24U)
#define SIM_SOPT4_TPM0CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM0CLKSEL_SHIFT)) & SIM_SOPT4_TPM0CLKSEL_MASK)
#define SIM_SOPT4_TPM1CLKSEL_MASK                (0x2000000U)
#define SIM_SOPT4_TPM1CLKSEL_SHIFT               (25U)
#define SIM_SOPT4_TPM1CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM1CLKSEL_SHIFT)) & SIM_SOPT4_TPM1CLKSEL_MASK)
#define SIM_SOPT4_TPM2CLKSEL_MASK                (0x4000000U)
#define SIM_SOPT4_TPM2CLKSEL_SHIFT               (26U)
#define SIM_SOPT4_TPM2CLKSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT4_TPM2CLKSEL_SHIFT)) & SIM_SOPT4_TPM2CLKSEL_MASK)

/*! @name SOPT5 - System Options Register 5 */
#define SIM_SOPT5_UART0TXSRC_MASK                (0x3U)
#define SIM_SOPT5_UART0TXSRC_SHIFT               (0U)
#define SIM_SOPT5_UART0TXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART0TXSRC_SHIFT)) & SIM_SOPT5_UART0TXSRC_MASK)
#define SIM_SOPT5_UART0RXSRC_MASK                (0x4U)
#define SIM_SOPT5_UART0RXSRC_SHIFT               (2U)
#define SIM_SOPT5_UART0RXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART0RXSRC_SHIFT)) & SIM_SOPT5_UART0RXSRC_MASK)
#define SIM_SOPT5_UART1TXSRC_MASK                (0x30U)
#define SIM_SOPT5_UART1TXSRC_SHIFT               (4U)
#define SIM_SOPT5_UART1TXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART1TXSRC_SHIFT)) & SIM_SOPT5_UART1TXSRC_MASK)
#define SIM_SOPT5_UART1RXSRC_MASK                (0x40U)
#define SIM_SOPT5_UART1RXSRC_SHIFT               (6U)
#define SIM_SOPT5_UART1RXSRC(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART1RXSRC_SHIFT)) & SIM_SOPT5_UART1RXSRC_MASK)
#define SIM_SOPT5_UART0ODE_MASK                  (0x10000U)
#define SIM_SOPT5_UART0ODE_SHIFT                 (16U)
#define SIM_SOPT5_UART0ODE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART0ODE_SHIFT)) & SIM_SOPT5_UART0ODE_MASK)
#define SIM_SOPT5_UART1ODE_MASK                  (0x20000U)
#define SIM_SOPT5_UART1ODE_SHIFT                 (17U)
#define SIM_SOPT5_UART1ODE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART1ODE_SHIFT)) & SIM_SOPT5_UART1ODE_MASK)
#define SIM_SOPT5_UART2ODE_MASK                  (0x40000U)
#define SIM_SOPT5_UART2ODE_SHIFT                 (18U)
#define SIM_SOPT5_UART2ODE(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_SOPT5_UART2ODE_SHIFT)) & SIM_SOPT5_UART2ODE_MASK)

/*! @name SOPT7 - System Options Register 7 */
#define SIM_SOPT7_ADC0TRGSEL_MASK                (0xFU)
#define SIM_SOPT7_ADC0TRGSEL_SHIFT               (0U)
#define SIM_SOPT7_ADC0TRGSEL(x)                  (((uint32_t)(((uint32_t)(x)) << SIM_SOPT7_ADC0TRGSEL_SHIFT)) & SIM_SOPT7_ADC0TRGSEL_MASK)
#define SIM_SOPT7_ADC0PRETRGSEL_MASK             (0x10U)
#define SIM_SOPT7_ADC0PRETRGSEL_SHIFT            (4U)
#define SIM_SOPT7_ADC0PRETRGSEL(x)               (((uint32_t)(((uint32_t)(x)) << SIM_SOPT7_ADC0PRETRGSEL_SHIFT)) & SIM_SOPT7_ADC0PRETRGSEL_MASK)
#define SIM_SOPT7_ADC0ALTTRGEN_MASK              (0x80U)
#define SIM_SOPT7_ADC0ALTTRGEN_SHIFT             (7U)
#define SIM_SOPT7_ADC0ALTTRGEN(x)                (((uint32_t)(((uint32_t)(x)) << SIM_SOPT7_ADC0ALTTRGEN_SHIFT)) & SIM_SOPT7_ADC0ALTTRGEN_MASK)

/*! @name SDID - System Device Identification Register */
#define SIM_SDID_PINID_MASK                      (0xFU)
#define SIM_SDID_PINID_SHIFT                     (0U)
#define SIM_SDID_PINID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_PINID_SHIFT)) & SIM_SDID_PINID_MASK)
#define SIM_SDID_DIEID_MASK                      (0xF80U)
#define SIM_SDID_DIEID_SHIFT                     (7U)
#define SIM_SDID_DIEID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_DIEID_SHIFT)) & SIM_SDID_DIEID_MASK)
#define SIM_SDID_REVID_MASK                      (0xF000U)
#define SIM_SDID_REVID_SHIFT                     (12U)
#define SIM_SDID_REVID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_REVID_SHIFT)) & SIM_SDID_REVID_MASK)
#define SIM_SDID_SRAMSIZE_MASK                   (0xF0000U)
#define SIM_SDID_SRAMSIZE_SHIFT                  (16U)
#define SIM_SDID_SRAMSIZE(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SDID_SRAMSIZE_SHIFT)) & SIM_SDID_SRAMSIZE_MASK)
#define SIM_SDID_SERIESID_MASK                   (0xF00000U)
#define SIM_SDID_SERIESID_SHIFT                  (20U)
#define SIM_SDID_SERIESID(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SDID_SERIESID_SHIFT)) & SIM_SDID_SERIESID_MASK)
#define SIM_SDID_SUBFAMID_MASK                   (0xF000000U)
#define SIM_SDID_SUBFAMID_SHIFT                  (24U)
#define SIM_SDID_SUBFAMID(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SDID_SUBFAMID_SHIFT)) & SIM_SDID_SUBFAMID_MASK)
#define SIM_SDID_FAMID_MASK                      (0xF0000000U)
#define SIM_SDID_FAMID_SHIFT                     (28U)
#define SIM_SDID_FAMID(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SDID_FAMID_SHIFT)) & SIM_SDID_FAMID_MASK)

/*! @name SCGC4 - System Clock Gating Control Register 4 */
#define SIM_SCGC4_I2C0_MASK                      (0x40U)
#define SIM_SCGC4_I2C0_SHIFT                     (6U)
#define SIM_SCGC4_I2C0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_I2C0_SHIFT)) & SIM_SCGC4_I2C0_MASK)
#define SIM_SCGC4_I2C1_MASK                      (0x80U)
#define SIM_SCGC4_I2C1_SHIFT                     (7U)
#define SIM_SCGC4_I2C1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_I2C1_SHIFT)) & SIM_SCGC4_I2C1_MASK)
#define SIM_SCGC4_UART0_MASK                     (0x400U)
#define SIM_SCGC4_UART0_SHIFT                    (10U)
#define SIM_SCGC4_UART0(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART0_SHIFT)) & SIM_SCGC4_UART0_MASK)
#define SIM_SCGC4_UART1_MASK                     (0x800U)
#define SIM_SCGC4_UART1_SHIFT                    (11U)
#define SIM_SCGC4_UART1(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART1_SHIFT)) & SIM_SCGC4_UART1_MASK)
#define SIM_SCGC4_UART2_MASK                     (0x1000U)
#define SIM_SCGC4_UART2_SHIFT                    (12U)
#define SIM_SCGC4_UART2(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_UART2_SHIFT)) & SIM_SCGC4_UART2_MASK)
#define SIM_SCGC4_USBOTG_MASK                    (0x40000U)
#define SIM_SCGC4_USBOTG_SHIFT                   (18U)
#define SIM_SCGC4_USBOTG(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_USBOTG_SHIFT)) & SIM_SCGC4_USBOTG_MASK)
#define SIM_SCGC4_CMP_MASK                       (0x80000U)
#define SIM_SCGC4_CMP_SHIFT                      (19U)
#define SIM_SCGC4_CMP(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_CMP_SHIFT)) & SIM_SCGC4_CMP_MASK)
#define SIM_SCGC4_SPI0_MASK                      (0x400000U)
#define SIM_SCGC4_SPI0_SHIFT                     (22U)
#define SIM_SCGC4_SPI0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_SPI0_SHIFT)) & SIM_SCGC4_SPI0_MASK)
#define SIM_SCGC4_SPI1_MASK                      (0x800000U)
#define SIM_SCGC4_SPI1_SHIFT                     (23U)
#define SIM_SCGC4_SPI1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC4_SPI1_SHIFT)) & SIM_SCGC4_SPI1_MASK)

/*! @name SCGC5 - System Clock Gating Control Register 5 */
#define SIM_SCGC5_LPTMR_MASK                     (0x1U)
#define SIM_SCGC5_LPTMR_SHIFT                    (0U)
#define SIM_SCGC5_LPTMR(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_LPTMR_SHIFT)) & SIM_SCGC5_LPTMR_MASK)
#define SIM_SCGC5_TSI_MASK                       (0x20U)
#define SIM_SCGC5_TSI_SHIFT                      (5U)
#define SIM_SCGC5_TSI(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_TSI_SHIFT)) & SIM_SCGC5_TSI_MASK)
#define SIM_SCGC5_PORTA_MASK                     (0x200U)
#define SIM_SCGC5_PORTA_SHIFT                    (9U)
#define SIM_SCGC5_PORTA(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTA_SHIFT)) & SIM_SCGC5_PORTA_MASK)
#define SIM_SCGC5_PORTB_MASK                     (0x400U)
#define SIM_SCGC5_PORTB_SHIFT                    (10U)
#define SIM_SCGC5_PORTB(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTB_SHIFT)) & SIM_SCGC5_PORTB_MASK)
#define SIM_SCGC5_PORTC_MASK                     (0x800U)
#define SIM_SCGC5_PORTC_SHIFT                    (11U)
#define SIM_SCGC5_PORTC(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTC_SHIFT)) & SIM_SCGC5_PORTC_MASK)
#define SIM_SCGC5_PORTD_MASK                     (0x1000U)
#define SIM_SCGC5_PORTD_SHIFT                    (12U)
#define SIM_SCGC5_PORTD(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTD_SHIFT)) & SIM_SCGC5_PORTD_MASK)
#define SIM_SCGC5_PORTE_MASK                     (0x2000U)
#define SIM_SCGC5_PORTE_SHIFT                    (13U)
#define SIM_SCGC5_PORTE(x)                       (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_PORTE_SHIFT)) & SIM_SCGC5_PORTE_MASK)
#define SIM_SCGC5_SLCD_MASK                      (0x80000U)
#define SIM_SCGC5_SLCD_SHIFT                     (19U)
#define SIM_SCGC5_SLCD(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC5_SLCD_SHIFT)) & SIM_SCGC5_SLCD_MASK)

/*! @name SCGC6 - System Clock Gating Control Register 6 */
#define SIM_SCGC6_FTF_MASK                       (0x1U)
#define SIM_SCGC6_FTF_SHIFT                      (0U)
#define SIM_SCGC6_FTF(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_FTF_SHIFT)) & SIM_SCGC6_FTF_MASK)
#define SIM_SCGC6_DMAMUX_MASK                    (0x2U)
#define SIM_SCGC6_DMAMUX_SHIFT                   (1U)
#define SIM_SCGC6_DMAMUX(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_DMAMUX_SHIFT)) & SIM_SCGC6_DMAMUX_MASK)
#define SIM_SCGC6_I2S_MASK                       (0x8000U)
#define SIM_SCGC6_I2S_SHIFT                      (15U)
#define SIM_SCGC6_I2S(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_I2S_SHIFT)) & SIM_SCGC6_I2S_MASK)
#define SIM_SCGC6_PIT_MASK                       (0x800000U)
#define SIM_SCGC6_PIT_SHIFT                      (23U)
#define SIM_SCGC6_PIT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_PIT_SHIFT)) & SIM_SCGC6_PIT_MASK)
#define SIM_SCGC6_TPM0_MASK                      (0x1000000U)
#define SIM_SCGC6_TPM0_SHIFT                     (24U)
#define SIM_SCGC6_TPM0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TPM0_SHIFT)) & SIM_SCGC6_TPM0_MASK)
#define SIM_SCGC6_TPM1_MASK                      (0x2000000U)
#define SIM_SCGC6_TPM1_SHIFT                     (25U)
#define SIM_SCGC6_TPM1(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TPM1_SHIFT)) & SIM_SCGC6_TPM1_MASK)
#define SIM_SCGC6_TPM2_MASK                      (0x4000000U)
#define SIM_SCGC6_TPM2_SHIFT                     (26U)
#define SIM_SCGC6_TPM2(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_TPM2_SHIFT)) & SIM_SCGC6_TPM2_MASK)
#define SIM_SCGC6_ADC0_MASK                      (0x8000000U)
#define SIM_SCGC6_ADC0_SHIFT                     (27U)
#define SIM_SCGC6_ADC0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_ADC0_SHIFT)) & SIM_SCGC6_ADC0_MASK)
#define SIM_SCGC6_RTC_MASK                       (0x20000000U)
#define SIM_SCGC6_RTC_SHIFT                      (29U)
#define SIM_SCGC6_RTC(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_RTC_SHIFT)) & SIM_SCGC6_RTC_MASK)
#define SIM_SCGC6_DAC0_MASK                      (0x80000000U)
#define SIM_SCGC6_DAC0_SHIFT                     (31U)
#define SIM_SCGC6_DAC0(x)                        (((uint32_t)(((uint32_t)(x)) << SIM_SCGC6_DAC0_SHIFT)) & SIM_SCGC6_DAC0_MASK)

/*! @name SCGC7 - System Clock Gating Control Register 7 */
#define SIM_SCGC7_DMA_MASK                       (0x100U)
#define SIM_SCGC7_DMA_SHIFT                      (8U)
#define SIM_SCGC7_DMA(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_SCGC7_DMA_SHIFT)) & SIM_SCGC7_DMA_MASK)

/*! @name CLKDIV1 - System Clock Divider Register 1 */
#define SIM_CLKDIV1_OUTDIV4_MASK                 (0x70000U)
#define SIM_CLKDIV1_OUTDIV4_SHIFT                (16U)
#define SIM_CLKDIV1_OUTDIV4(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV4_SHIFT)) & SIM_CLKDIV1_OUTDIV4_MASK)
#define SIM_CLKDIV1_OUTDIV1_MASK                 (0xF0000000U)
#define SIM_CLKDIV1_OUTDIV1_SHIFT                (28U)
#define SIM_CLKDIV1_OUTDIV1(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_CLKDIV1_OUTDIV1_SHIFT)) & SIM_CLKDIV1_OUTDIV1_MASK)

/*! @name FCFG1 - Flash Configuration Register 1 */
#define SIM_FCFG1_FLASHDIS_MASK                  (0x1U)
#define SIM_FCFG1_FLASHDIS_SHIFT                 (0U)
#define SIM_FCFG1_FLASHDIS(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_FLASHDIS_SHIFT)) & SIM_FCFG1_FLASHDIS_MASK)
#define SIM_FCFG1_FLASHDOZE_MASK                 (0x2U)
#define SIM_FCFG1_FLASHDOZE_SHIFT                (1U)
#define SIM_FCFG1_FLASHDOZE(x)                   (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_FLASHDOZE_SHIFT)) & SIM_FCFG1_FLASHDOZE_MASK)
#define SIM_FCFG1_PFSIZE_MASK                    (0xF000000U)
#define SIM_FCFG1_PFSIZE_SHIFT                   (24U)
#define SIM_FCFG1_PFSIZE(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_FCFG1_PFSIZE_SHIFT)) & SIM_FCFG1_PFSIZE_MASK)

/*! @name FCFG2 - Flash Configuration Register 2 */
#define SIM_FCFG2_MAXADDR1_MASK                  (0x7F0000U)
#define SIM_FCFG2_MAXADDR1_SHIFT                 (16U)
#define SIM_FCFG2_MAXADDR1(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_MAXADDR1_SHIFT)) & SIM_FCFG2_MAXADDR1_MASK)
#define SIM_FCFG2_MAXADDR0_MASK                  (0x7F000000U)
#define SIM_FCFG2_MAXADDR0_SHIFT                 (24U)
#define SIM_FCFG2_MAXADDR0(x)                    (((uint32_t)(((uint32_t)(x)) << SIM_FCFG2_MAXADDR0_SHIFT)) & SIM_FCFG2_MAXADDR0_MASK)

/*! @name UIDMH - Unique Identification Register Mid-High */
#define SIM_UIDMH_UID_MASK                       (0xFFFFU)
#define SIM_UIDMH_UID_SHIFT                      (0U)
#define SIM_UIDMH_UID(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_UIDMH_UID_SHIFT)) & SIM_UIDMH_UID_MASK)

/*! @name UIDML - Unique Identification Register Mid Low */
#define SIM_UIDML_UID_MASK                       (0xFFFFFFFFU)
#define SIM_UIDML_UID_SHIFT                      (0U)
#define SIM_UIDML_UID(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_UIDML_UID_SHIFT)) & SIM_UIDML_UID_MASK)

/*! @name UIDL - Unique Identification Register Low */
#define SIM_UIDL_UID_MASK                        (0xFFFFFFFFU)
#define SIM_UIDL_UID_SHIFT                       (0U)
#define SIM_UIDL_UID(x)                          (((uint32_t)(((uint32_t)(x)) << SIM_UIDL_UID_SHIFT)) & SIM_UIDL_UID_MASK)

/*! @name COPC - COP Control Register */
#define SIM_COPC_COPW_MASK                       (0x1U)
#define SIM_COPC_COPW_SHIFT                      (0U)
#define SIM_COPC_COPW(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPW_SHIFT)) & SIM_COPC_COPW_MASK)
#define SIM_COPC_COPCLKS_MASK                    (0x2U)
#define SIM_COPC_COPCLKS_SHIFT                   (1U)
#define SIM_COPC_COPCLKS(x)                      (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPCLKS_SHIFT)) & SIM_COPC_COPCLKS_MASK)
#define SIM_COPC_COPT_MASK                       (0xCU)
#define SIM_COPC_COPT_SHIFT                      (2U)
#define SIM_COPC_COPT(x)                         (((uint32_t)(((uint32_t)(x)) << SIM_COPC_COPT_SHIFT)) & SIM_COPC_COPT_MASK)

/*! @name SRVCOP - Service COP */
#define SIM_SRVCOP_SRVCOP_MASK                   (0xFFU)
#define SIM_SRVCOP_SRVCOP_SHIFT                  (0U)
#define SIM_SRVCOP_SRVCOP(x)                     (((uint32_t)(((uint32_t)(x)) << SIM_SRVCOP_SRVCOP_SHIFT)) & SIM_SRVCOP_SRVCOP_MASK)

/* end of group SIM_Register_Masks */


/* SIM - Peripheral instance base addresses */
/** Peripheral SIM base address */
#define SIM_BASE                                 (0x40047000u)
/** Peripheral SIM base pointer */
#define SIM                                      ((SIM_Type *)SIM_BASE)
/** Array initializer of SIM peripheral base addresses */
#define SIM_BASE_ADDRS                           { SIM_BASE }
/** Array initializer of SIM peripheral base pointers */
#define SIM_BASE_PTRS                            { SIM }

/* end of group SIM_Peripheral_Access_Layer */
/* ----------------------------------------------------------------------------
   -- PORT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/** PORT - Register Layout Typedef */
typedef struct {
  __IO uint32_t PCR[32];/**< Pin Control Register n, array offset: 0x0, array step: 0x4 */
  __O  uint32_t GPCLR;/**< Global Pin Control Low Register, offset: 0x80 */
  __O  uint32_t GPCHR;/**< Global Pin Control High Register, offset: 0x84 */
       uint8_t RESERVED_0[24];
  __IO uint32_t ISFR;/**< Interrupt Status Flag Register, offset: 0xA0 */
} PORT_Type;

/* ----------------------------------------------------------------------------
   -- PORT Register Masks
   ---------------------------------------------------------------------------- */

/*! @name PCR - Pin Control Register n */
#define PORT_PCR_PS_MASK                         (0x1U)
#define PORT_PCR_PS_SHIFT                        (0U)
#define PORT_PCR_PS(x)                           (((uint32_t)(((uint32_t)(x)) << PORT_PCR_PS_SHIFT)) & PORT_PCR_PS_MASK)
#define PORT_PCR_PE_MASK                         (0x2U)
#define PORT_PCR_PE_SHIFT                        (1U)
#define PORT_PCR_PE(x)                           (((uint32_t)(((uint32_t)(x)) << PORT_PCR_PE_SHIFT)) & PORT_PCR_PE_MASK)
#define PORT_PCR_SRE_MASK                        (0x4U)
#define PORT_PCR_SRE_SHIFT                       (2U)
#define PORT_PCR_SRE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_SRE_SHIFT)) & PORT_PCR_SRE_MASK)
#define PORT_PCR_PFE_MASK                        (0x10U)
#define PORT_PCR_PFE_SHIFT                       (4U)
#define PORT_PCR_PFE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_PFE_SHIFT)) & PORT_PCR_PFE_MASK)
#define PORT_PCR_DSE_MASK                        (0x40U)
#define PORT_PCR_DSE_SHIFT                       (6U)
#define PORT_PCR_DSE(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_DSE_SHIFT)) & PORT_PCR_DSE_MASK)
#define PORT_PCR_MUX_MASK                        (0x700U)
#define PORT_PCR_MUX_SHIFT                       (8U)
#define PORT_PCR_MUX(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_MUX_SHIFT)) & PORT_PCR_MUX_MASK)
#define PORT_PCR_IRQC_MASK                       (0xF0000U)
#define PORT_PCR_IRQC_SHIFT                      (16U)
#define PORT_PCR_IRQC(x)                         (((uint32_t)(((uint32_t)(x)) << PORT_PCR_IRQC_SHIFT)) & PORT_PCR_IRQC_MASK)
#define PORT_PCR_ISF_MASK                        (0x1000000U)
#define PORT_PCR_ISF_SHIFT                       (24U)
#define PORT_PCR_ISF(x)                          (((uint32_t)(((uint32_t)(x)) << PORT_PCR_ISF_SHIFT)) & PORT_PCR_ISF_MASK)

/* The count of PORT_PCR */
#define PORT_PCR_COUNT                           (32U)

/*! @name GPCLR - Global Pin Control Low Register */
#define PORT_GPCLR_GPWD_MASK                     (0xFFFFU)
#define PORT_GPCLR_GPWD_SHIFT                    (0U)
#define PORT_GPCLR_GPWD(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCLR_GPWD_SHIFT)) & PORT_GPCLR_GPWD_MASK)
#define PORT_GPCLR_GPWE_MASK                     (0xFFFF0000U)
#define PORT_GPCLR_GPWE_SHIFT                    (16U)
#define PORT_GPCLR_GPWE(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCLR_GPWE_SHIFT)) & PORT_GPCLR_GPWE_MASK)

/*! @name GPCHR - Global Pin Control High Register */
#define PORT_GPCHR_GPWD_MASK                     (0xFFFFU)
#define PORT_GPCHR_GPWD_SHIFT                    (0U)
#define PORT_GPCHR_GPWD(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCHR_GPWD_SHIFT)) & PORT_GPCHR_GPWD_MASK)
#define PORT_GPCHR_GPWE_MASK                     (0xFFFF0000U)
#define PORT_GPCHR_GPWE_SHIFT                    (16U)
#define PORT_GPCHR_GPWE(x)                       (((uint32_t)(((uint32_t)(x)) << PORT_GPCHR_GPWE_SHIFT)) & PORT_GPCHR_GPWE_MASK)

/*! @name ISFR - Interrupt Status Flag Register */
#define PORT_ISFR_ISF_MASK                       (0xFFFFFFFFU)
#define PORT_ISFR_ISF_SHIFT                      (0U)
#define PORT_ISFR_ISF(x)                         (((uint32_t)(((uint32_t)(x)) << PORT_ISFR_ISF_SHIFT)) & PORT_ISFR_ISF_MASK)

/* end of group PORT_Register_Masks */

/* PORT - Peripheral instance base addresses */
/** Peripheral PORTA base address */
#define PORTA_BASE                               (0x40049000u)
/** Peripheral PORTA base pointer */
#define PORTA                                    ((PORT_Type *)PORTA_BASE)
/** Peripheral PORTB base address */
#define PORTB_BASE                               (0x4004A000u)
/** Peripheral PORTB base pointer */
#define PORTB                                    ((PORT_Type *)PORTB_BASE)
/** Peripheral PORTC base address */
#define PORTC_BASE                               (0x4004B000u)
/** Peripheral PORTC base pointer */
#define PORTC                                    ((PORT_Type *)PORTC_BASE)
/** Peripheral PORTD base address */
#define PORTD_BASE                               (0x4004C000u)
/** Peripheral PORTD base pointer */
#define PORTD                                    ((PORT_Type *)PORTD_BASE)
/** Peripheral PORTE base address */
#define PORTE_BASE                               (0x4004D000u)
/** Peripheral PORTE base pointer */
#define PORTE                                    ((PORT_Type *)PORTE_BASE)
/** Array initializer of PORT peripheral base addresses */
#define PORT_BASE_ADDRS                          { PORTA_BASE, PORTB_BASE, PORTC_BASE, PORTD_BASE, PORTE_BASE }
/** Array initializer of PORT peripheral base pointers */
#define PORT_BASE_PTRS                           { PORTA, PORTB, PORTC, PORTD, PORTE }
/** Interrupt vectors for the PORT peripheral type */
#define PORT_IRQS                                { PORTA_IRQn, NotAvail_IRQn, PORTC_PORTD_IRQn, PORTC_PORTD_IRQn, NotAvail_IRQn }

/* ----------------------------------------------------------------------------
   -- PIT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/** PIT - Register Layout Typedef */
typedef struct {
  __IO uint32_t MCR;/**< PIT Module Control Register, offset: 0x0 */
       uint8_t RESERVED_0[220];
  __I  uint32_t LTMR64H;/**< PIT Upper Lifetime Timer Register, offset: 0xE0 */
  __I  uint32_t LTMR64L;/**< PIT Lower Lifetime Timer Register, offset: 0xE4 */
       uint8_t RESERVED_1[24];
  struct { /* offset: 0x100, array step: 0x10 */
    __IO uint32_t LDVAL;/**< Timer Load Value Register, array offset: 0x100, array step: 0x10 */
    __I  uint32_t CVAL;/**< Current Timer Value Register, array offset: 0x104, array step: 0x10 */
    __IO uint32_t TCTRL;/**< Timer Control Register, array offset: 0x108, array step: 0x10 */
    __IO uint32_t TFLG;/**< Timer Flag Register, array offset: 0x10C, array step: 0x10 */
  } CHANNEL[2];
} PIT_Type;

/* ----------------------------------------------------------------------------
   -- PIT Register Masks
   ---------------------------------------------------------------------------- */

/*! @name MCR - PIT Module Control Register */
#define PIT_MCR_FRZ_MASK                         (0x1U)
#define PIT_MCR_FRZ_SHIFT                        (0U)
#define PIT_MCR_FRZ(x)                           (((uint32_t)(((uint32_t)(x)) << PIT_MCR_FRZ_SHIFT)) & PIT_MCR_FRZ_MASK)
#define PIT_MCR_MDIS_MASK                        (0x2U)
#define PIT_MCR_MDIS_SHIFT                       (1U)
#define PIT_MCR_MDIS(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_MCR_MDIS_SHIFT)) & PIT_MCR_MDIS_MASK)

/*! @name LTMR64H - PIT Upper Lifetime Timer Register */
#define PIT_LTMR64H_LTH_MASK                     (0xFFFFFFFFU)
#define PIT_LTMR64H_LTH_SHIFT                    (0U)
#define PIT_LTMR64H_LTH(x)                       (((uint32_t)(((uint32_t)(x)) << PIT_LTMR64H_LTH_SHIFT)) & PIT_LTMR64H_LTH_MASK)

/*! @name LTMR64L - PIT Lower Lifetime Timer Register */
#define PIT_LTMR64L_LTL_MASK                     (0xFFFFFFFFU)
#define PIT_LTMR64L_LTL_SHIFT                    (0U)
#define PIT_LTMR64L_LTL(x)                       (((uint32_t)(((uint32_t)(x)) << PIT_LTMR64L_LTL_SHIFT)) & PIT_LTMR64L_LTL_MASK)

/*! @name LDVAL - Timer Load Value Register */
#define PIT_LDVAL_TSV_MASK                       (0xFFFFFFFFU)
#define PIT_LDVAL_TSV_SHIFT                      (0U)
#define PIT_LDVAL_TSV(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_LDVAL_TSV_SHIFT)) & PIT_LDVAL_TSV_MASK)

/* The count of PIT_LDVAL */
#define PIT_LDVAL_COUNT                          (2U)

/*! @name CVAL - Current Timer Value Register */
#define PIT_CVAL_TVL_MASK                        (0xFFFFFFFFU)
#define PIT_CVAL_TVL_SHIFT                       (0U)
#define PIT_CVAL_TVL(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_CVAL_TVL_SHIFT)) & PIT_CVAL_TVL_MASK)

/* The count of PIT_CVAL */
#define PIT_CVAL_COUNT                           (2U)

/*! @name TCTRL - Timer Control Register */
#define PIT_TCTRL_TEN_MASK                       (0x1U)
#define PIT_TCTRL_TEN_SHIFT                      (0U)
#define PIT_TCTRL_TEN(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_TCTRL_TEN_SHIFT)) & PIT_TCTRL_TEN_MASK)
#define PIT_TCTRL_TIE_MASK                       (0x2U)
#define PIT_TCTRL_TIE_SHIFT                      (1U)
#define PIT_TCTRL_TIE(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_TCTRL_TIE_SHIFT)) & PIT_TCTRL_TIE_MASK)
#define PIT_TCTRL_CHN_MASK                       (0x4U)
#define PIT_TCTRL_CHN_SHIFT                      (2U)
#define PIT_TCTRL_CHN(x)                         (((uint32_t)(((uint32_t)(x)) << PIT_TCTRL_CHN_SHIFT)) & PIT_TCTRL_CHN_MASK)

/* The count of PIT_TCTRL */
#define PIT_TCTRL_COUNT                          (2U)

/*! @name TFLG - Timer Flag Register */
#define PIT_TFLG_TIF_MASK                        (0x1U)
#define PIT_TFLG_TIF_SHIFT                       (0U)
#define PIT_TFLG_TIF(x)                          (((uint32_t)(((uint32_t)(x)) << PIT_TFLG_TIF_SHIFT)) & PIT_TFLG_TIF_MASK)

/* The count of PIT_TFLG */
#define PIT_TFLG_COUNT                           (2U)

/* end of group PIT_Register_Masks */

/* PIT - Peripheral instance base addresses */
/** Peripheral PIT base address */
#define PIT_BASE                                 (0x40037000u)
/** Peripheral PIT base pointer */
#define PIT                                      ((PIT_Type *)PIT_BASE)
/** Array initializer of PIT peripheral base addresses */
#define PIT_BASE_ADDRS                           { PIT_BASE }
/** Array initializer of PIT peripheral base pointers */
#define PIT_BASE_PTRS                            { PIT }
/** Interrupt vectors for the PIT peripheral type */
#define PIT_IRQS                                 { { PIT_IRQn, PIT_IRQn, PIT_IRQn, PIT_IRQn } }

/* end of group PIT_Peripheral_Access_Layer */

/******NVIC_EnableIRQ ************************************************/

/* ----------------------------------------------------------------------------
   -- Interrupt vector numbers
   ---------------------------------------------------------------------------- */

/** Interrupt Number Definitions */
#define NUMBER_OF_INT_VECTORS 48/**< Number of interrupts in the Vector table */

typedef enum IRQn {
/* Auxiliary constants */
  NotAvail_IRQn                = -128,/**< Not available device specific interrupt */
/* Core interrupts */
  NonMaskableInt_IRQn          = -14,/**< Non Maskable Interrupt */
  HardFault_IRQn               = -13,/**< Cortex-M0 SV Hard Fault Interrupt */
  SVCall_IRQn                  = -5,/**< Cortex-M0 SV Call Interrupt */
  PendSV_IRQn                  = -2,/**< Cortex-M0 Pend SV Interrupt */
  SysTick_IRQn                 = -1,/**< Cortex-M0 System Tick Interrupt */
/* Device specific interrupts */
  DMA0_IRQn                    = 0,/**< DMA channel 0 transfer complete and error interrupt */
  DMA1_IRQn                    = 1,/**< DMA channel 1 transfer complete and error interrupt */
  DMA2_IRQn                    = 2,/**< DMA channel 2 transfer complete and error interrupt */
  DMA3_IRQn                    = 3,/**< DMA channel 3 transfer complete and error interrupt */
  Reserved20_IRQn              = 4,/**< Reserved interrupt */
  FTFA_IRQn                    = 5,/**< FTFA command complete and read collision */
  LVD_LVW_IRQn                 = 6,/**< Low-voltage detect, low-voltage warning */
  LLWU_IRQn                    = 7,/**< Low Leakage Wakeup */
  I2C0_IRQn                    = 8,/**< I2C0 interrupt */
  I2C1_IRQn                    = 9,/**< I2C1 interrupt */
  SPI0_IRQn                    = 10,/**< SPI0 single interrupt vector for all sources */
  SPI1_IRQn                    = 11,/**< SPI1 single interrupt vector for all sources */
  UART0_IRQn                   = 12,/**< UART0 status and error */
  UART1_IRQn                   = 13,/**< UART1 status and error */
  UART2_IRQn                   = 14,/**< UART2 status and error */
  ADC0_IRQn                    = 15,/**< ADC0 interrupt */
  CMP0_IRQn                    = 16,/**< CMP0 interrupt */
  TPM0_IRQn                    = 17,/**< TPM0 single interrupt vector for all sources */
  TPM1_IRQn                    = 18,/**< TPM1 single interrupt vector for all sources */
  TPM2_IRQn                    = 19,/**< TPM2 single interrupt vector for all sources */
  RTC_IRQn                     = 20,/**< RTC alarm interrupt */
  RTC_Seconds_IRQn             = 21,/**< RTC seconds interrupt */
  PIT_IRQn                     = 22,/**< PIT single interrupt vector for all channels */
  I2S0_IRQn                    = 23,/**< I2S0 Single interrupt vector for all sources */
  USB0_IRQn                    = 24,/**< USB0 OTG */
  DAC0_IRQn                    = 25,/**< DAC0 interrupt */
  TSI0_IRQn                    = 26,/**< TSI0 interrupt */
  MCG_IRQn                     = 27,/**< MCG interrupt */
  LPTMR0_IRQn                  = 28,/**< LPTMR0 interrupt */
  LCD_IRQn                     = 29,/**< Segment LCD interrupt */
  PORTA_IRQn                   = 30,/**< PORTA pin detect */
  PORTC_PORTD_IRQn             = 31/**< Single interrupt vector for PORTC and PORTD pin detect */
} IRQn_Type;

/**
brief  Structure type to access the Nested Vectored Interrupt Controller (NVIC).
 */
typedef struct
{
  __IOM uint32_t ISER[1U];/*!< Offset: 0x000 (R/W)  Interrupt Set Enable Register */
        uint32_t RESERVED0[31U];
  __IOM uint32_t ICER[1U];/*!< Offset: 0x080 (R/W)  Interrupt Clear Enable Register */
        uint32_t RSERVED1[31U];
  __IOM uint32_t ISPR[1U];/*!< Offset: 0x100 (R/W)  Interrupt Set Pending Register */
        uint32_t RESERVED2[31U];
  __IOM uint32_t ICPR[1U];/*!< Offset: 0x180 (R/W)  Interrupt Clear Pending Register */
        uint32_t RESERVED3[31U];
        uint32_t RESERVED4[64U];
  __IOM uint32_t IP[8U];/*!< Offset: 0x300 (R/W)  Interrupt Priority Register */
}  NVIC_Type;

/*@} end of group CMSIS_NVIC */
/* end of group Interrupt_vector_numbers */
#define SCS_BASE            (0xE000E000UL)
#define NVIC_BASE           (SCS_BASE +  0x0100UL)/*!< NVIC Base Address */
#define NVIC                ((NVIC_Type      *)     NVIC_BASE        )/*!< NVIC configuration struct */
#define NVIC_EnableIRQ              __NVIC_EnableIRQ
#define __STATIC_INLINE                        static inline
/**
  \brief   Enable Interrupt
  \details Enables a device specific interrupt in the NVIC interrupt controller.
  \param [in]      IRQn  Device specific interrupt number.
  \note    IRQn must not be negative.
 */
__STATIC_INLINE void __NVIC_EnableIRQ(IRQn_Type IRQn)
{
  if ((int32_t)(IRQn) >= 0)
  {
    NVIC->ISER[(((uint32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
  }
}
/*!
 * @}
 */ /* end of group PORT_Peripheral_Access_Layer */
#endif /* MYMKL64Z_H_ */

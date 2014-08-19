/***********************************************************************
*
*             Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       iar_stm32f429ii_sk_defs.h
*
*   DESCRIPTION
*
*       This file contains platform definitions for the IAR STM32F429II-SK
*       platform.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
***********************************************************************/

#ifndef         IAR_STM32F429II_SK_DEFS_H
#define         IAR_STM32F429II_SK_DEFS_H

/*--------------------------------------------------------------------
 * Customizing BSP using STM CUBE code gen output
 *
 */
/* Configuration file from the STMicro CUBE SDK tool */
#include "stm32f4xx_hal_conf.h"

#include "gpio_defs.h"

/*--------------------------------------------------------------------*/


/* Define the board clock rate (in hertz).
   NOTE: This clock rate is used to calculate the rate of the OS timer.
         Therefore, if multiple clock sources are fed to the processor,
         this clock rate value must represent the source used
         by the on-chip timer unit. */
#define         ESAL_DP_REF_CLOCK_RATE                  8000000UL

/* Define number of memory regions contained on the given development platform */
#define         ESAL_DP_MEM_NUM_REGIONS                 4

/* Define if an interrupt controller (off processor) exists on the board and
   controlling / handling of interrupts from this interrupt controller must
   be accommodated for.  Setting this to NU_FALSE means off-chip interrupts
   will NOT be controlled or handled.  Setting this to NU_TRUE means off-chip
   interrupts will be controlled and handled */
#define         ESAL_DP_INTERRUPTS_AVAILABLE            NU_FALSE

/* Define the base address of on-chip peripheral registers */
#define         ESAL_DP_PERIPH_BASE                     0x40000000

/* Define for Reset and Clock Control (RCC) module base address */
#define         ESAL_DP_RCC_BASE                       (ESAL_DP_PERIPH_BASE + 0x00023800)

/* RCC register offsets */
#define         ESAL_DP_RCC_CR_OFFSET                   0x000       /* Clock control register */
#define         ESAL_DP_RCC_CFGR_OFFSET                 0x008       /* Clock configuration register */
#define         ESAL_DP_RCC_CIR_OFFSET                  0x00C       /* Clock interrupt register */
#define         ESAL_DP_RCC_AHB1RSTR_OFFSET             0x010       /* AHB1 peripheral reset register */
#define         ESAL_DP_RCC_AHB2RSTR_OFFSET             0x014       /* AHB2 peripheral reset register */
#define         ESAL_DP_RCC_AHB3RSTR_OFFSET             0x018       /* AHB3 peripheral reset register */
#define         ESAL_DP_RCC_APB1RSTR_OFFSET             0x020       /* APB1 peripheral reset register */
#define         ESAL_DP_RCC_APB2RSTR_OFFSET             0x024       /* APB2 peripheral reset register */
#define         ESAL_DP_RCC_AHB1ENR_OFFSET              0x030       /* AHB1 peripheral enable register */
#define         ESAL_DP_RCC_AHB2ENR_OFFSET              0x034       /* AHB2 peripheral enable register */
#define         ESAL_DP_RCC_AHB3ENR_OFFSET              0x038       /* AHB3 peripheral enable register */
#define         ESAL_DP_RCC_APB1ENR_OFFSET              0x040       /* APB1 peripheral enable register */
#define         ESAL_DP_RCC_APB2ENR_OFFSET              0x044       /* APB2 peripheral enable register */
#define         ESAL_DP_RCC_BDCR_OFFSET                 0x070       /* Backup domain control register */
#define         ESAL_DP_RCC_CSR_OFFSET                  0x074       /* Control/status register */
#define         ESAL_DP_RCC_PLLSAICFGR_OFFSET           0x088
#define         ESAL_DP_RCC_DCKCFGR_OFFSET              0x08c

/* Defines for Peripheral IDs mapped on AHB1 */
#define         AHB1_GPIOA_PERIPHERAL_ID                0
#define         AHB1_GPIOB_PERIPHERAL_ID                1
#define         AHB1_GPIOC_PERIPHERAL_ID                2
#define         AHB1_GPIOD_PERIPHERAL_ID                3
#define         AHB1_GPIOE_PERIPHERAL_ID                4
#define         AHB1_GPIOF_PERIPHERAL_ID                5
#define         AHB1_GPIOG_PERIPHERAL_ID                6
#define         AHB1_GPIOH_PERIPHERAL_ID                7
#define         AHB1_GPIOI_PERIPHERAL_ID                8
#define         AHB1_CRC_PERIPHERAL_ID                  12
#define         AHB1_DMA1_PERIPHERAL_ID                 21
#define         AHB1_DMA2_PERIPHERAL_ID                 22
#define         AHB1_DMA2D_PERIPHERAL_ID                23
#define         AHB1_ETHMAC_PERIPHERAL_ID               25
#define         AHB1_ETHMACTX_PERIPHERAL_ID             26
#define         AHB1_ETHMACRX_PERIPHERAL_ID             27
#define         AHB1_ETHMACPTP_PERIPHERAL_ID            28
#define         AHB1_OTGHS_PERIPHERAL_ID                29
#define         AHB1_OTGHSULPI_PERIPHERAL_ID            30

/* Defines for Peripheral IDs mapped on AHB2 */
#define         AHB2_DCMI_PERIPHERAL_ID                 0
#define         AHB2_CRYP_PERIPHERAL_ID                 4
#define         AHB2_HASH_PERIPHERAL_ID                 5
#define         AHB2_RNG_PERIPHERAL_ID                  6
#define         AHB2_OTGFS_PERIPHERAL_ID                7

/* Defines for Peripheral IDs mapped on AHB3 */
#define         AHB3_FSMC_PERIPHERAL_ID                 0

/* Defines for Peripheral IDs mapped on APB1 */
#define         APB1_TIM2_PERIPHERAL_ID                 0
#define         APB1_TIM3_PERIPHERAL_ID                 1
#define         APB1_TIM4_PERIPHERAL_ID                 2
#define         APB1_TIM5_PERIPHERAL_ID                 3
#define         APB1_TIM6_PERIPHERAL_ID                 4
#define         APB1_TIM7_PERIPHERAL_ID                 5
#define         APB1_TIM12_PERIPHERAL_ID                6
#define         APB1_TIM13_PERIPHERAL_ID                7
#define         APB1_TIM14_PERIPHERAL_ID                8
#define         APB1_WWDG_PERIPHERAL_ID                 11
#define         APB1_SPI1_PERIPHERAL_ID                 14
#define         APB1_SPI2_PERIPHERAL_ID                 15
#define         APB1_USART2_PERIPHERAL_ID               17
#define         APB1_USART3_PERIPHERAL_ID               18
#define         APB1_USART4_PERIPHERAL_ID               19
#define         APB1_USART5_PERIPHERAL_ID               20
#define         APB1_I2C0_PERIPHERAL_ID                 21
#define         APB1_I2C1_PERIPHERAL_ID                 22
#define         APB1_I2C2_PERIPHERAL_ID                 23
#define         APB1_CAN1_PERIPHERAL_ID                 25
#define         APB1_CAN2_PERIPHERAL_ID                 26
#define         APB1_PWR_PERIPHERAL_ID                  28
#define         APB1_DAC_PERIPHERAL_ID                  29

/* Defines for Peripheral IDs mapped on APB2 */
#define         APB2_TIM1_PERIPHERAL_ID                 0
#define         APB2_TIM8_PERIPHERAL_ID                 1
#define         APB2_USART1_PERIPHERAL_ID               4
#define         APB2_USART6_PERIPHERAL_ID               5
#define         APB2_ADC_PERIPHERAL_ID                  8
#define         APB2_SDIO_PERIPHERAL_ID                 11
#define         APB2_SPI0_PERIPHERAL_ID                 12
#define         APB2_SYSCFG_PERIPHERAL_ID               14
#define         APB2_TIM9_PERIPHERAL_ID                 16
#define         APB2_TIM10_PERIPHERAL_ID                17
#define         APB2_TIM11_PERIPHERAL_ID                18
#define         APB2_SPI5_PERIPHERAL_ID                 20
#define         APB2_LTDC_PERIPHERAL_ID                 26

/* Defines for GPIO base addresses */
#define         ESAL_DP_GPIOA_BASE                      (ESAL_DP_PERIPH_BASE + 0x00020000) /* GPIO A base address */
#define         ESAL_DP_GPIOB_BASE                      (ESAL_DP_PERIPH_BASE + 0x00020400) /* GPIO B base address */
#define         ESAL_DP_GPIOC_BASE                      (ESAL_DP_PERIPH_BASE + 0x00020800) /* GPIO C base address */
#define         ESAL_DP_GPIOD_BASE                      (ESAL_DP_PERIPH_BASE + 0x00020C00) /* GPIO D base address */
#define         ESAL_DP_GPIOE_BASE                      (ESAL_DP_PERIPH_BASE + 0x00021000) /* GPIO E base address */
#define         ESAL_DP_GPIOF_BASE                      (ESAL_DP_PERIPH_BASE + 0x00021400) /* GPIO F base address */
#define         ESAL_DP_GPIOG_BASE                      (ESAL_DP_PERIPH_BASE + 0x00021800) /* GPIO G base address */
#define         ESAL_DP_GPIOH_BASE                      (ESAL_DP_PERIPH_BASE + 0x00021C00) /* GPIO H base address */
#define         ESAL_DP_GPIOI_BASE                      (ESAL_DP_PERIPH_BASE + 0x00022000) /* GPIO I base address */

/* Defines for GPIO register offsets */
#define         ESAL_DP_GPIO_MODER                      0x000       /* GPIO port mode register */
#define         ESAL_DP_GPIO_OTYPER                     0x004       /* GPIO output type register */
#define         ESAL_DP_GPIO_OSPEED                     0x008       /* GPIO output speed register */
#define         ESAL_DP_GPIO_PUPDR                      0x00C       /* GPIO pull-up/pull-down register */
#define         ESAL_DP_GPIO_IDR                        0x010       /* GPIO input data register */
#define         ESAL_DP_GPIO_ODR                        0x014       /* GPIO output data register */
#define         ESAL_DP_GPIO_BSRR                       0x018       /* GPIO bit set/reset register */
#define         ESAL_DP_GPIO_LCKR                       0x01C       /* GPIO configuration lock register */
#define         ESAL_DP_GPIO_AFRL                       0x020       /* GPIO alternate function low register */
#define         ESAL_DP_GPIO_AFRH                       0x024       /* GPIO alternate function high register */

/* Define for AFIO base address */
#define         ESAL_DP_AFIO_BASE                       (ESAL_DP_PERIPH_BASE + 0x00010000)

/* Defines for AFIO register offsets */
#define         ESAL_DP_AFIO_EVCR                       0x000       /* AFIO event control register */
#define         ESAL_DP_AFIO_MAPR                       0x004       /* AFIO remap and debug configuration register */
#define         ESAL_DP_AFIO_EXTLCR1                    0x008       /* AFIO external interrupt configuration register 1 */
#define         ESAL_DP_AFIO_EXTLCR2                    0x00C       /* AFIO external interrupt configuration register 2 */
#define         ESAL_DP_AFIO_EXTLCR3                    0x010       /* AFIO external interrupt configuration register 3 */
#define         ESAL_DP_AFIO_EXTLCR4                    0x014       /* AFIO external interrupt configuration register 4 */

/* Define for EXTl base address */
#define         ESAL_DP_EXTL_BASE                       (ESAL_DP_PERIPH_BASE + 0x00010400)

/* Defines for EXTl register offsets */
#define         ESAL_DP_EXTL_IMR                        0x000       /* EXTl interrupt mask register */
#define         ESAL_DP_EXTL_EMR                        0x004       /* EXTl event mask register */
#define         ESAL_DP_EXTL_RTSR                       0x008       /* EXTl rising trigger selection register */
#define         ESAL_DP_EXTL_FTSR                       0x00C       /* EXTl falling trigger selection register */
#define         ESAL_DP_EXTL_SWIER                      0x010       /* EXTl software interrupt enable register */
#define         ESAL_DP_EXTL_PR                         0x014       /* EXTl interrupt pending register */

/* Macros to Read/Write GPIO and AFIO registers */

/* Write to GPIO register */
#define         GPIO_WRITE(GPIO,REG,MASK)                                                    \
                   ESAL_GE_MEM_WRITE32(ESAL_DP_GPIO##GPIO##_BASE + ESAL_DP_GPIO_##REG, MASK)

/* Read to GPIO register */
#define         GPIO_READ(GPIO,REG)                                                          \
                   ESAL_GE_MEM_READ32(ESAL_DP_GPIO##GPIO##_BASE + ESAL_DP_GPIO_##REG)

/* Write to AFIO register */
#define         AFIO_WRITE(REG,MASK)                                                         \
                   ESAL_GE_MEM_WRITE32(ESAL_DP_AFIO_BASE + ESAL_DP_AFIO_##REG, MASK)

/* Read to AFIO register */
#define         AFIO_READ(REG)                                                               \
                   ESAL_GE_MEM_READ32(ESAL_DP_AFIO_BASE + ESAL_DP_AFIO_##REG)

/* Enable peripheral clock */
#define         ENABLE_PERIPHERAL_CLOCK(BUS,PERIPH)                                          \
{{                                                                                           \
    /* Read current value of AHB2ENR register */                                             \
    UINT32 _reg_val = ESAL_GE_MEM_READ32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##ENR_OFFSET);  \
    /* Set clock enable bit */                                                               \
    _reg_val |= (1 << BUS##_##PERIPH##_PERIPHERAL_ID);                                       \
    /* Write updated value */                                                                \
    ESAL_GE_MEM_WRITE32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##ENR_OFFSET, _reg_val);         \
}}

/* Disable peripheral clock */
#define         DISABLE_PERIPHERAL_CLOCK(BUS,PERIPH)                                         \
{{                                                                                           \
    /* Read current value of AHB2ENR register */                                             \
    UINT32 _reg_val = ESAL_GE_MEM_READ32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##ENR_OFFSET);  \
    /* Set clock enable bit */                                                               \
    _reg_val &= ~(1 << BUS##_##PERIPH##_PERIPHERAL_ID);                                      \
    /* Write updated value */                                                                \
    ESAL_GE_MEM_WRITE32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##ENR_OFFSET, _reg_val);         \
}}

/* Reset peripheral */
#define         RESET_PERIPHERAL(BUS,PERIPH)                                                 \
{{                                                                                           \
    /* Read current value of AHB2RSTR register */                                            \
    UINT32 _reg_val = ESAL_GE_MEM_READ32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##RSTR_OFFSET); \
    /* Reset peripheral by setting '1' */                                                    \
    _reg_val |= (1 << BUS##_##PERIPH##_PERIPHERAL_ID);                                       \
    /* Write updated value */                                                                \
    ESAL_GE_MEM_WRITE32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##RSTR_OFFSET, _reg_val);        \
    /* Take peripheral out of reset by setting '0' */                                        \
    _reg_val &= ~(1 << BUS##_##PERIPH##_PERIPHERAL_ID);                                      \
    /* Write updated value */                                                                \
    ESAL_GE_MEM_WRITE32(ESAL_DP_RCC_BASE + ESAL_DP_RCC_##BUS##RSTR_OFFSET, _reg_val);        \
}}

/*
 * MAX_TICKS_PER_INTERVAL is the maximum multiple of system ticks
 * to which the tick hardware timer can be set to.
 * MAX_TICKS_PER_INTERVAL = 2^(number of bits in tick counter)-1 / (counts per tick)
*/
#define         MAX_TICKS_PER_INTERVAL                  12

#endif  /* IAR_STM32F429II_SK_DEFS_H */


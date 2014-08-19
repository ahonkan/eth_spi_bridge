/*************************************************************************
*
*            Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*        cpu_tgt.h
*
*   DESCRIPTION
*
*        Contains data structures and prototypes of the CPU driver
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nu_services.h
*
*************************************************************************/
#ifndef CPU_TGT_H
#define CPU_TGT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "services/nu_services.h"

#define CPU_IDLE_IOCTL_BASE                 (DV_IOCTL0 + 1)
#define CPU_DVFS_IOCTL_BASE                 (CPU_IDLE_IOCTL_BASE + CPU_IDLE_IOCTL_TOTAL)

/* Values for OP indexing. */
#define CPU_MAX_OP_COUNT                    10
#define CPU_OP_ARRAY_SIZE                   (CPU_MAX_OP_COUNT + 1)
#define CPU_STARTUP_OP_INDEX                CPU_MAX_OP_COUNT

/* System clock frequencies. */
#define CPU_SYSCLK_FREQ                     "SYSCLK"
#define CPU_PLLCLK_FREQ                     "PLLCLK"
#define CPU_AHBCLK_FREQ                     "AHBCLK"
#define CPU_APB1CLK_FREQ                    "APB1CLK"
#define CPU_APB2CLK_FREQ                    "APB2CLK"
#define CPU_PLL48CLK_FREQ                   "PLL48CLK"
#define CPU_PLLI2SCLK_FREQ                  "PLLI2SCLK"
#define CPU_RTCCLK_FREQ                     "RTCCLK"
#define CPU_IWDGCLK_FREQ                    "IWDGCLK"
#define CPU_MACCLK_FREQ                     "MACCLK"
#define CPU_USBHSCLK_FREQ                   "USBHSCLK"

/* Macro to make accessing the startup OP easier to read. */
#define CPU_GET_OP_INDEX(index)             ((index == PM_STARTUP_OP) ? CPU_STARTUP_OP_INDEX : index)

/* CPU clock sources. */
typedef enum _cpu_clock_enum
{
    CPU_HSI = 0,
    CPU_HSE = 1,
    CPU_PLL = 2,

} CPU_CLOCK;

/* AHB clock divisors. */
typedef enum _cpu_ahb_div
{
    AHB_DIV_1   = 1,
    AHB_DIV_2   = 2,
    AHB_DIV_4   = 4,
    AHB_DIV_8   = 8,
    AHB_DIV_16  = 16,
    AHB_DIV_64  = 64,
    AHB_DIV_128 = 128,
    AHB_DIV_256 = 256,
    AHB_DIV_512 = 512,

} CPU_AHB_DIV;

/* APB clock divisors. */
typedef enum _cpu_apb_div
{
    APB_DIV_1  = 1,
    APB_DIV_2  = 2,
    APB_DIV_4  = 4,
    APB_DIV_8  = 8,
    APB_DIV_16 = 16,

} CPU_APB_DIV;

/* DVFS operating points structure. */
typedef struct _cpu_dvfs_op_cb
{
    CPU_CLOCK    pm_clock;                  /* Clock to be used     */
    UINT16       pm_ahb_divisor;            /* AHB Clock divisor    */
    UINT16       pm_apb1_divisor;           /* APB1 Clock divisor   */
    UINT16       pm_apb2_divisor;           /* APB2 Clock divisor   */
    UINT32       pm_ahb_frequency;          /* AHB Frequency in Hz  */
    UINT32       pm_apb1_frequency;         /* APB1 Frequency in Hz */
    UINT32       pm_apb2_frequency;         /* APB2 Frequency in Hz */

} CPU_DVFS_OP;

/* CPU scaling structure. */
typedef struct _cpu_scale_cb
{
    UINT32       pm_numerator;
    UINT32       pm_denominator;

} CPU_SCALE;

typedef struct _cpu_dvfs_op_info_cb
{
    UINT32       pm_master_freq;

} CPU_DVFS_OP_INFO;

typedef struct _cpu_register_cb
{
    UINT32        pm_count;
    CPU_DVFS_OP  *pm_op_list;

} CPU_REGISTER;

/* Reset and Clock Control (RCC) module base address. */
#define CPU_RCC_BASE                        0x40023800

/* Reset and Clock Control (RCC) module registers. */
#define CPU_RCC_CR                          0x00
#define CPU_RCC_PLLCFGR                     0x04
#define CPU_RCC_CFGR                        0x08
#define CPU_RCC_CIR                         0x0C
#define CPU_RCC_APB1ENR                     0x40

/* CR register field. */
#define CPU_RCC_CR_HSION                    (1 << 0)
#define CPU_RCC_CR_HSIRDY                   (1 << 1)
#define CPU_RCC_CR_HSEON                    (1 << 16)
#define CPU_RCC_CR_HSERDY                   (1 << 17)
#define CPU_RCC_CR_HSEBYP                   (1 << 18)
#define CPU_RCC_CR_CSSON                    (1 << 19)
#define CPU_RCC_CR_PLLON                    (1 << 24)
#define CPU_RCC_CR_PLLRDY                   (1 << 25)
#define CPU_RCC_CR_PLLI2SON                 (1 << 26)
#define CPU_RCC_CR_PLLI2SRDY                (1 << 27)

/* PLLCFGR register field. */
#define CPU_RCC_PLLCFGR_PLLM0               0
#define CPU_RCC_PLLCFGR_PLLM1               1
#define CPU_RCC_PLLCFGR_PLLM2               2
#define CPU_RCC_PLLCFGR_PLLM3               3
#define CPU_RCC_PLLCFGR_PLLM4               4
#define CPU_RCC_PLLCFGR_PLLM5               5
#define CPU_RCC_PLLCFGR_PLLN0               6
#define CPU_RCC_PLLCFGR_PLLN1               7
#define CPU_RCC_PLLCFGR_PLLN2               8
#define CPU_RCC_PLLCFGR_PLLN3               9
#define CPU_RCC_PLLCFGR_PLLN4               10
#define CPU_RCC_PLLCFGR_PLLN5               11
#define CPU_RCC_PLLCFGR_PLLN6               12
#define CPU_RCC_PLLCFGR_PLLN7               13
#define CPU_RCC_PLLCFGR_PLLN8               14
#define CPU_RCC_PLLCFGR_PLLP0               16
#define CPU_RCC_PLLCFGR_PLLP1               17
#define CPU_RCC_PLLCFGR_PLLSRC              22
#define CPU_RCC_PLLCFGR_PLLQ0               24
#define CPU_RCC_PLLCFGR_PLLQ1               25
#define CPU_RCC_PLLCFGR_PLLQ2               26
#define CPU_RCC_PLLCFGR_PLLQ3               27

/* CFGR register field. */
#define CPU_RCC_CFGR_SW_SHIFT               0
#define CPU_RCC_CFGR_SWS_SHIFT              2
#define CPU_RCC_CFGR_HPRE_SHIFT             4
#define CPU_RCC_CFGR_PPRE1_SHIFT            10
#define CPU_RCC_CFGR_PPRE2_SHIFT            13
#define CPU_RCC_CFGR_RTCPRE_SHIFT           16
#define CPU_RCC_CFGR_MCO1_SHIFT             21
#define CPU_RCC_CFGR_I2SSRC_SHIFT           23
#define CPU_RCC_CFGR_MCO1PRE_SHIFT          24
#define CPU_RCC_CFGR_MCO2PRE_SHIFT          27
#define CPU_RCC_CFGR_MCO2_SHIFT             30
#define CPU_RCC_CFGR_SW_MASK                0x00000003
#define CPU_RCC_CFGR_SWS_MASK               0x0000000C
#define CPU_RCC_CFGR_HPRE_MASK              0x000000F0
#define CPU_RCC_CFGR_PPRE1_MASK             0x00001C00
#define CPU_RCC_CFGR_PPRE2_MASK             0x0000E000
#define CPU_RCC_CFGR_RTCPRE_MASK            0x001F0000
#define CPU_RCC_CFGR_MCO1_MASK              0x00600000
#define CPU_RCC_CFGR_I2SSRC_MASK            0x00800000
#define CPU_RCC_CFGR_MCO1PRE_MASK           0x07000000
#define CPU_RCC_CFGR_MCO2PRE_MASK           0x38000000
#define CPU_RCC_CFGR_MCO2_MASK              0xC0000000

/* CPU frequency defines. */
#define CPU_FREQ_180MHZ                     180000000
#define CPU_FREQ_120MHZ                     120000000
#define CPU_FREQ_168MHZ                     168000000
#define CPU_FREQ_60MHZ                      60000000
#define CPU_FREQ_30MHZ                      30000000
#define CPU_FREQ_16MHZ                      16000000
#define CPU_FREQ_15MHZ                      15000000
#define CPU_FREQ_8MHZ                        8000000
#define CPU_FREQ_PLL48CLK                   48000000
#define CPU_FREQ_MACCLK                     25000000        /* 25MHz for 100MBPS */
#define CPU_FREQ_USBHSCLK                   24000000 
#define CPU_FREQ_RTCCLK                     32000000 


/* Function prototypes. */
STATUS CPU_Register(const CHAR * key, INT startstop, DV_DEV_LABEL *user_labels, INT user_label_cnt, DV_DEV_ID *dev_id);
STATUS CPU_Unregister(DV_DEV_ID dev_id);
STATUS CPU_Tgt_Get_Frequency(UINT32 *master_freq, CHAR *identifier);

#ifdef __cplusplus
}
#endif

#endif /* CPU_TGT_H */


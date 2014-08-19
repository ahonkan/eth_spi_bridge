/***********************************************************************
*
*             Copyright 2011 Mentor Graphics Corporation
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
*       stm32_defs.h
*
*   DESCRIPTION
*
*       This file contains all definitions, structures, etc for the
*       STM32 series of processors.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       arm_defs.h
*
***********************************************************************/

#ifndef         STM32_DEFS_H
#define         STM32_DEFS_H

#include        "arm_defs.h"


/* Define processor execution endianess
   (ESAL_LITTLE_ENDIAN or ESAL_BIG_ENDIAN) */
#define         ESAL_PR_ENDIANESS                    ESAL_LITTLE_ENDIAN

/* Define processor cache availability
   NOTE:  A differentiation is made in ESAL between cache that
          is contained on a processor and cache that is
          inherent as part of a core (L2 vs L1 cache). */
#define         ESAL_PR_CACHE_AVAILABLE              NU_FALSE

/* Define if an interrupt controller exists on the processor and
   controlling / handling of interrupts from this interrupt controller must
   be accommodated for.  Setting this to NU_FALSE means processor level interrupts
   will NOT be controlled or handled.  Setting this to NU_TRUE means processor level
   interrupts will be controlled and handled */
#define         ESAL_PR_INTERRUPTS_AVAILABLE         NU_TRUE

/* Used to disable all interrupt sources */
#define         ESAL_PR_ALL_INTS_MASK                0xFFFFFFFF

/* Define ESAL interrupt vector IDs for this processor.
   These IDs match up with processor interrupts.
   Values correspond to the index of entries in ESAL_GE_ISR_Interrupt_Handler[].
   Names are of the form ESAL_PR_<Name>_INT_VECTOR_ID, where <Name> comes
   directly from the hardware documentation */

/* STM32 interrupt vectors */
/* Define number of priority bits for this processor */
#define         ESAL_PR_INT_NUM_PRIORITY_BITS        4

#define         ESAL_PR_WWDG_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 0)
#define         ESAL_PR_PVD_INT_VECTOR_ID            (ESAL_AR_INT_VECTOR_ID_DELIMITER + 1)
#define         ESAL_PR_TAMPER_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 2)
#define         ESAL_PR_RTC_INT_VECTOR_ID            (ESAL_AR_INT_VECTOR_ID_DELIMITER + 3)
#define         ESAL_PR_FLASH_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 4)
#define         ESAL_PR_RCC_INT_VECTOR_ID            (ESAL_AR_INT_VECTOR_ID_DELIMITER + 5)
#define         ESAL_PR_EXTL0_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 6)
#define         ESAL_PR_EXTL1_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 7)
#define         ESAL_PR_EXTL2_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 8)
#define         ESAL_PR_EXTL3_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 9)
#define         ESAL_PR_EXTL4_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 10)
#define         ESAL_PR_DMA1_CH0_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 11)
#define         ESAL_PR_DMA1_CH1_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 12)
#define         ESAL_PR_DMA1_CH2_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 13)
#define         ESAL_PR_DMA1_CH3_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 14)
#define         ESAL_PR_DMA1_CH4_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 15)
#define         ESAL_PR_DMA1_CH5_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 16)
#define         ESAL_PR_DMA1_CH6_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 17)
#define         ESAL_PR_ADC1_2_3_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 18)
#define         ESAL_PR_CAN1_TX_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 19)
#define         ESAL_PR_CAN1_RX0_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 20)
#define         ESAL_PR_CAN1_RX1_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 21)
#define         ESAL_PR_CAN1_SCE_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 22)
#define         ESAL_PR_EXTL5_9_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 23)
#define         ESAL_PR_TIM1_BRK_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 24)
#define         ESAL_PR_TIM1_UP_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 25)
#define         ESAL_PR_TIM1_TRG_COM_INT_VECTOR_ID   (ESAL_AR_INT_VECTOR_ID_DELIMITER + 26)
#define         ESAL_PR_TIM1_CC_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 27)
#define         ESAL_PR_TIM2_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 28)
#define         ESAL_PR_TIM3_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 29)
#define         ESAL_PR_TIM4_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 30)
#define         ESAL_PR_I2C1_EV_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 31)
#define         ESAL_PR_I2C1_ER_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 32)
#define         ESAL_PR_I2C2_EV_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 33)
#define         ESAL_PR_I2C2_ER_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 34)
#define         ESAL_PR_SPI1_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 35)
#define         ESAL_PR_SPI2_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 36)
#define         ESAL_PR_USART1_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 37)
#define         ESAL_PR_USART2_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 38)
#define         ESAL_PR_USART3_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 39)
#define         ESAL_PR_EXTL15_10_INT_VECTOR_ID      (ESAL_AR_INT_VECTOR_ID_DELIMITER + 40)
#define         ESAL_PR_RTC_ALARM_INT_VECTOR_ID      (ESAL_AR_INT_VECTOR_ID_DELIMITER + 41)
#define         ESAL_PR_OTG_FS_WKUP_INT_VECTOR_ID    (ESAL_AR_INT_VECTOR_ID_DELIMITER + 42)
#define         ESAL_PR_TIM8_BRK_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 43)
#define         ESAL_PR_TIM8_UP_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 44)
#define         ESAL_PR_TIM8_TRG_COM_INT_VECTOR_ID   (ESAL_AR_INT_VECTOR_ID_DELIMITER + 45)
#define         ESAL_PR_TIM8_CC_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 46)
#define         ESAL_PR_DMA1_CH7_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 47)
#define         ESAL_PR_FSMC_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 48)
#define         ESAL_PR_SDIO_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 49)
#define         ESAL_PR_TIM5_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 50)
#define         ESAL_PR_SPI3_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 51)
#define         ESAL_PR_UART4_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 52)
#define         ESAL_PR_UART5_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 53)
#define         ESAL_PR_TIM6_DAC_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 54)
#define         ESAL_PR_TIM7_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 55)
#define         ESAL_PR_DMA2_CH0_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 56)
#define         ESAL_PR_DMA2_CH1_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 57)
#define         ESAL_PR_DMA2_CH2_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 58)
#define         ESAL_PR_DMA2_CH3_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 59)
#define         ESAL_PR_DMA2_CH4_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 60)
#define         ESAL_PR_ETH_INT_VECTOR_ID            (ESAL_AR_INT_VECTOR_ID_DELIMITER + 61)
#define         ESAL_PR_ETH_WKUP_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 62)
#define         ESAL_PR_CAN2_TX_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 63)
#define         ESAL_PR_CAN2_RX0_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 64)
#define         ESAL_PR_CAN2_RX1_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 65)
#define         ESAL_PR_CAN2_SCE_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 66)
#define         ESAL_PR_OTG_FS_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 67)
#define         ESAL_PR_DMA2_CH5_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 68)
#define         ESAL_PR_DMA2_CH6_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 69)
#define         ESAL_PR_DMA2_CH7_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 70)
#define         ESAL_PR_USART6_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 71)
#define         ESAL_PR_I2C3_EV_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 72)
#define         ESAL_PR_I2C3_ER_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 73)
#define         ESAL_PR_OTG_HS_EP1_OUT_INT_VECTOR_ID (ESAL_AR_INT_VECTOR_ID_DELIMITER + 74)
#define         ESAL_PR_OTG_HS_EP1_IN_INT_VECTOR_ID  (ESAL_AR_INT_VECTOR_ID_DELIMITER + 75)
#define         ESAL_PR_OTG_HS_WKUP_INT_VECTOR_ID    (ESAL_AR_INT_VECTOR_ID_DELIMITER + 76)
#define         ESAL_PR_OTG_HS_INT_VECTOR_ID         (ESAL_AR_INT_VECTOR_ID_DELIMITER + 77)
#define         ESAL_PR_DCMI_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 78)
#define         ESAL_PR_CRYP_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 79)
#define         ESAL_PR_HASH_RNG_INT_VECTOR_ID       (ESAL_AR_INT_VECTOR_ID_DELIMITER + 80)

#if     defined(CFG_NU_BSP_STM32429I_EVAL1_ENABLE) || defined (CFG_NU_BSP_IAR_STM32F429II_SK_ENABLE)
#define         ESAL_PR_FPU_INT_VECTOR_ID            (ESAL_AR_INT_VECTOR_ID_DELIMITER + 81)
#define         ESAL_PR_UART7_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 82)
#define         ESAL_PR_UART8_INT_VECTOR_ID          (ESAL_AR_INT_VECTOR_ID_DELIMITER + 83)
#define         ESAL_PR_SPI4_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 84)
#define         ESAL_PR_SPI5_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 85)
#define         ESAL_PR_SPI6_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 86)
#define         ESAL_PR_SAI1_INT_VECTOR_ID           (ESAL_AR_INT_VECTOR_ID_DELIMITER + 87)
#define         ESAL_PR_LCD_TFT_INT_VECTOR_ID        (ESAL_AR_INT_VECTOR_ID_DELIMITER + 88)

/* Define the last ESAL interrupt vector ID for this processor + 1 */
#define         ESAL_PR_INT_VECTOR_ID_DELIMITER      (ESAL_PR_LCD_TFT_INT_VECTOR_ID + 1)
#else
#define         ESAL_PR_INT_VECTOR_ID_DELIMITER      (ESAL_PR_HASH_RNG_INT_VECTOR_ID + 1)
#endif  /*CFG_NU_BSP_STM32429I_EVAL1_ENABLE */

/* Define PLL multiplier / divider values */
#if defined(CFG_NU_BSP_STM3240G_EVAL_ENABLE) || defined(CFG_NU_BSP_STM32429I_EVAL1_ENABLE)
#define         ESAL_PR_PLL_MULT                     336

#elif defined (CFG_NU_BSP_IAR_STM32F429II_SK_ENABLE)
#define         ESAL_PR_PLL_MULT                     1000

#elif defined (CFG_NU_BSP_STM3210E_EVAL_ENABLE)
#define         ESAL_PR_PLL_MULT                     9

#else
#define         ESAL_PR_PLL_MULT                     240

#endif

#if defined (CFG_NU_BSP_STM3210E_EVAL_ENABLE)
#define         ESAL_PR_PLL_DIV                      1

#else
#define         ESAL_PR_PLL_DIV                      50

#endif

#if defined (CFG_NU_BSP_STM3210E_EVAL_ENABLE)
/* Define the clock rate for the OS timer */
#define         ESAL_PR_TMR_OS_CLOCK_RATE            (UINT32)((ESAL_DP_REF_CLOCK_RATE * ESAL_PR_PLL_MULT) / ESAL_PR_PLL_DIV)

#else
/* Define the clock rate for the OS timer */
#define         ESAL_PR_TMR_OS_CLOCK_RATE            (UINT32)((ESAL_DP_REF_CLOCK_RATE / ESAL_PR_PLL_DIV) * ESAL_PR_PLL_MULT)

#endif
/* Define the clock prescaler for the OS timer
   NOTE:  The prescaler is used to adjust the processor clock rate to a lower clock
          rate suitable for the timer */
#define         ESAL_PR_TMR_OS_CLOCK_PRESCALE        1

#if defined (CFG_NU_BSP_STM3210E_EVAL_ENABLE)

#if (ESAL_GE_ROM_SUPPORT_ENABLED == ESAL_TRUE)

/* Vector table address when running from ROM */
#define         ESAL_PR_ISR_VECTOR_TABLE_DEST_ADDR   0x00001800UL

#else

/* Vector table address when running from RAM */
#define         ESAL_PR_ISR_VECTOR_TABLE_DEST_ADDR   0x20000000UL

#endif

#endif
/* Define to disable systick. */
#define         ESAL_AR_SYSTICK_DISABLE()                           \
{{                                                                  \
    UINT32 _ctrl_32;                                                \
    _ctrl_32 = ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL);        \
    /* Clear all bits except clock source. */                       \
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CTRL,                   \
           (_ctrl_32 & ESAL_AR_TMR_SYSTICK_CTRL_CLKSRC_BIT));       \
}}


/* Define to enable systick. */
#define         ESAL_AR_SYSTICK_ENABLE()                            \
{{                                                                  \
    UINT32 _ctrl_32;                                                \
    _ctrl_32 = ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL);        \
    /* Enable sys tick.  */                                         \
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CTRL,                   \
           (_ctrl_32 | ESAL_AR_TMR_SYSTICK_CTRL_ENABLE_BIT |        \
             ESAL_AR_TMR_SYSTICK_CTRL_TICKINT_BIT));                \
}}

/* Defines for delay function. */

/* Define max timer count. */
#define     ESAL_PR_MAX_TIMER_COUNT                                      \
            ESAL_GE_TMR_COUNT_CALC(ESAL_GE_TMR_OS_CLOCK_RATE,            \
                                   ESAL_GE_TMR_OS_CLOCK_PRESCALE,        \
                                   NU_PLUS_TICKS_PER_SEC)

/* Macro for determining timer maximum count. */
/* The timer used is count down timer. */
#define     ESAL_PR_CALC_TIME(start,end)                                    \
            ((start < end) ? ((ESAL_PR_MAX_TIMER_COUNT - end) + start) + 1: \
            (start - end))

/* GPIO Configuration Mode enumeration */   
typedef enum
{ 
    GPIO_Mode_IN   = 0x00, /* GPIO Input Mode */
    GPIO_Mode_OUT  = 0x01, /* GPIO Output Mode */
    GPIO_Mode_AF   = 0x02, /* GPIO Alternate function Mode */
    GPIO_Mode_AN   = 0x03  /* GPIO Analog Mode */
  
}GPIOMode_TypeDef;

/* GPIO Output type enumeration */  
typedef enum
{ 
    GPIO_OType_PP = 0x00,
    GPIO_OType_OD = 0x01
}GPIOOType_TypeDef;

/* GPIO Output Maximum frequency enumeration */  
typedef enum
{ 
    GPIO_Speed_2MHz   = 0x00, /* Low speed */
    GPIO_Speed_25MHz  = 0x01, /* Medium speed */
    GPIO_Speed_50MHz  = 0x02, /* Fast speed */
    GPIO_Speed_100MHz = 0x03  /* High speed on 30 pF (80 MHz Output max speed on 15 pF) */
}GPIOSpeed_TypeDef;

/* GPIO Configuration PullUp PullDown enumeration */ 
typedef enum
{ 
    GPIO_PuPd_NOPULL = 0x00,
    GPIO_PuPd_UP     = 0x01,
    GPIO_PuPd_DOWN   = 0x02
}GPIOPuPd_TypeDef;

/* GPIO Bit SET and Bit RESET enumeration */ 
typedef enum
{ 
    Bit_RESET = 0,
    Bit_SET
}BitAction;


/* Timing parameters for FMC SDRAM Banks */
typedef struct
{
    UINT32 FMC_LoadToActiveDelay;       
    UINT32 FMC_ExitSelfRefreshDelay;    
    UINT32 FMC_SelfRefreshTime;               
    UINT32 FMC_RowCycleDelay;                 
    UINT32 FMC_WriteRecoveryTime;             
    UINT32 FMC_RPDelay;                       
    UINT32 FMC_RCDDelay;                      
}FMC_SDRAM_Timing_Init;

/* Command parameters for FMC SDRAM Banks */
typedef struct
{
    UINT32 FMC_CommandMode;                   
    UINT32 FMC_CommandTarget;                 
    UINT32 FMC_AutoRefreshNumber;                                                                              
    UINT32 FMC_ModeRegisterDefinition; 
  
}FMC_SDRAM_Command_Type;

/* FMC SDRAM Init structure definition */
typedef struct
{
    UINT32 FMC_Bank;         
    UINT32 FMC_ColumnBitsNumber;                                            
    UINT32 FMC_RowBitsNumber;                 
    UINT32 FMC_SDMemoryDataWidth;             
    UINT32 FMC_InternalBankNumber;            
    UINT32 FMC_CASLatency;                    
    UINT32 FMC_WriteProtection;               
    UINT32 FMC_SDClockPeriod;                 
    UINT32 FMC_ReadBurst;                     
    UINT32 FMC_ReadPipeDelay;                 
    FMC_SDRAM_Timing_Init* FMC_SDRAMTimingStruct;  
  
}FMC_SDRAM_Init;

/* GPIO Init structure definition */ 
typedef struct
{
    UINT32 GPIO_Pin;          
    GPIOMode_TypeDef GPIO_Mode;
    GPIOSpeed_TypeDef GPIO_Speed; 
    GPIOOType_TypeDef GPIO_OType; 
    GPIOPuPd_TypeDef GPIO_PuPd;   
}GPIO_Init;

/* FMC SDRAM Mode definition register defines */
#define FMC_Bank1_SDRAM                         ((UINT32)0x00000000)
#define FMC_Bank2_SDRAM                         ((UINT32)0x00000001)
#define SDRAM_BANK_ADDR                         ((UINT32)0xD0000000)          
#define SDRAM_MEMORY_WIDTH                      FMC_SDMemory_Width_16b      
#define SDRAM_CAS_LATENCY                       FMC_CAS_Latency_3             
#define SDCLOCK_PERIOD                          FMC_SDClock_Period_2          
#define SDRAM_READBURST                         FMC_Read_Burst_Disable        
#define SDRAM_MODEREG_BURST_LENGTH_2            ((UINT16)0x0001)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL     ((UINT16)0x0000)
#define SDRAM_MODEREG_CAS_LATENCY_3             ((UINT16)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD   ((UINT16)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE    ((UINT16)0x0200)      

/* FMC Register Bits Defines */
#define FMC_ColumnBits_Number_8b             ((UINT32)0x00000000)
#define FMC_RowBits_Number_12b               ((UINT32)0x00000004)
#define FMC_SDMemory_Width_16b               ((UINT32)0x00000010)
#define FMC_InternalBank_Number_4            ((UINT32)0x00000040)
#define FMC_CAS_Latency_3                    ((UINT32)0x00000180)
#define FMC_Write_Protection_Disable         ((UINT32)0x00000000)
#define FMC_SDClock_Period_2                 ((UINT32)0x00000800)
#define FMC_Read_Burst_Disable               ((UINT32)0x00000000)
#define FMC_ReadPipe_Delay_1                 ((UINT32)0x00002000)
#define FMC_Command_Mode_CLK_Enabled         ((UINT32)0x00000001)
#define FMC_Command_Mode_PALL                ((UINT32)0x00000002)
#define FMC_Command_Mode_AutoRefresh         ((UINT32)0x00000003)
#define FMC_Command_Mode_LoadMode            ((UINT32)0x00000004)
#define FMC_Command_Target_bank2             ((UINT32)0x00000008)
/*
#define FMC_SDSR_BUSY                        ((UINT32)0x00000020)        
*/
#define FMC_FLAG_Busy                        FMC_SDSR_BUSY

#define FMC_BANK2_BASE_ADDRESS               (0xA0000000 + 0x0060)
#define FMC_BANK3_BASE_ADDRESS               (0xA0000000 + 0x0080)
#define FMC_BANK4_BASE_ADDRESS               (0xA0000000 + 0x00A0)
#define FMC_BANK5_6_BASE_ADDRESS             (0xA0000000 + 0x0140)      
                                  
/* GPIO pins define */ 
#define GPIO_Pin_0                           ((UINT16)0x0001)  
#define GPIO_Pin_1                           ((UINT16)0x0002)  
#define GPIO_Pin_2                           ((UINT16)0x0004)  
#define GPIO_Pin_3                           ((UINT16)0x0008)  
#define GPIO_Pin_4                           ((UINT16)0x0010)  
#define GPIO_Pin_5                           ((UINT16)0x0020)  
#define GPIO_Pin_6                           ((UINT16)0x0040)  
#define GPIO_Pin_7                           ((UINT16)0x0080)  
#define GPIO_Pin_8                           ((UINT16)0x0100)  
#define GPIO_Pin_9                           ((UINT16)0x0200)  
#define GPIO_Pin_10                          ((UINT16)0x0400)  
#define GPIO_Pin_11                          ((UINT16)0x0800)  
#define GPIO_Pin_12                          ((UINT16)0x1000)  
#define GPIO_Pin_13                          ((UINT16)0x2000)  
#define GPIO_Pin_14                          ((UINT16)0x4000)  
#define GPIO_Pin_15                          ((UINT16)0x8000)  
#define GPIO_Pin_All                         ((UINT16)0xFFFF)  

/* GPIO Pin sources */ 
#define GPIO_PinSource0                      ((UINT8)0x00)
#define GPIO_PinSource1                      ((UINT8)0x01)
#define GPIO_PinSource2                      ((UINT8)0x02)
#define GPIO_PinSource3                      ((UINT8)0x03)
#define GPIO_PinSource4                      ((UINT8)0x04)
#define GPIO_PinSource5                      ((UINT8)0x05)
#define GPIO_PinSource6                      ((UINT8)0x06)
#define GPIO_PinSource7                      ((UINT8)0x07)
#define GPIO_PinSource8                      ((UINT8)0x08)
#define GPIO_PinSource9                      ((UINT8)0x09)
#define GPIO_PinSource10                     ((UINT8)0x0A)
#define GPIO_PinSource11                     ((UINT8)0x0B)
#define GPIO_PinSource12                     ((UINT8)0x0C)
#define GPIO_PinSource13                     ((UINT8)0x0D)
#define GPIO_PinSource14                     ((UINT8)0x0E)
#define GPIO_PinSource15                     ((UINT8)0x0F)

/* GPIO Register Bit Defines */
/*
#define GPIO_MODER_MODER0                    ((UINT32)0x00000003)
#define GPIO_OTYPER_OT_0                     ((UINT32)0x00000001)
#define GPIO_OSPEEDER_OSPEEDR0               ((UINT32)0x00000003)
#define GPIO_PUPDR_PUPDR0                    ((UINT32)0x00000003)
*/
#define GPIO_AF_LTDC                         ((UINT8)0x0E)  /* LCD-TFT Alternate Function mapping */                       

/* GPIO Base Addresses */ 
#define GPIOA_BASE_ADDRESS                   0x40020000
#define GPIOB_BASE_ADDRESS                   0x40020400
#define GPIOC_BASE_ADDRESS                   0x40020800
#define GPIOD_BASE_ADDRESS                   0x40020C00
#define GPIOE_BASE_ADDRESS                   0x40021000
#define GPIOF_BASE_ADDRESS                   0x40021400
#define GPIOG_BASE_ADDRESS                   0x40021800
#define GPIOH_BASE_ADDRESS                   0x40021C00
#define GPIOI_BASE_ADDRESS                   0x40022000
#define GPIOJ_BASE_ADDRESS                   0x40022400
#define GPIOK_BASE_ADDRESS                   0x40022800

/* RCC Defines */     
#define RCC_AHB1Periph_GPIOA                 ((UINT32)0x00000001)
#define RCC_AHB1Periph_GPIOB                 ((UINT32)0x00000002)
#define RCC_AHB1Periph_GPIOC                 ((UINT32)0x00000004)
#define RCC_AHB1Periph_GPIOD                 ((UINT32)0x00000008)
#define RCC_AHB1Periph_GPIOE                 ((UINT32)0x00000010)
#define RCC_AHB1Periph_GPIOF                 ((UINT32)0x00000020)
#define RCC_AHB1Periph_GPIOG                 ((UINT32)0x00000040)
#define RCC_AHB1Periph_GPIOH                 ((UINT32)0x00000080)
#define RCC_AHB1Periph_GPIOI                 ((UINT32)0x00000100) 
#define RCC_AHB1Periph_GPIOJ                 ((UINT32)0x00000200)
#define RCC_AHB1Periph_GPIOK                 ((UINT32)0x00000400)
#define RCC_AHB1Periph_DMA2D                 ((UINT32)0x00800000)
#define RCC_APB2Periph_SPI5                  ((UINT32)0x00100000)
#define RCC_APB2Periph_LTDC                  ((UINT32)0x04000000)
#define RCC_FLAG_PLLSAIRDY                   ((UINT8)0x3D)
/*
#define RCC_DCKCFGR_PLLSAIDIVR               ((UINT32)0x00030000)
*/
#define RCC_PLLSAIDivR_Div8                  ((UINT32)0x00020000)

#define RCC_BASE_ADDRESS                     0x40023800
#define RCC_PLLSAICFGR                       0x88
#define RCC_DCKCFGR                          0x8C
#define RCC_AHB1ENR                          0x30
#define RCC_AHB3ENR                          0x38
#define RCC_APB2ENR                          0x44
#define RCC_APB1RSTR                         0x20
#define RCC_APB2RSTR                         0x24
#define RCC_CR                               0x00
#define RCC_BDCR                             0x70
#define RCC_CSR                              0x74

/* Peripheral Base Addresses */
/*
#define PERIPH_BASE                          ((UINT32)0x40000000) 
#define PERIPH_BB_BASE                       ((UINT32)0x42000000) 
#define AHB1PERIPH_BASE                      (PERIPH_BASE + 0x00020000)
#define RCC_BASE                             (AHB1PERIPH_BASE + 0x3800)    
*/
/* RCC Offset */
#define RCC_OFFSET                           (RCC_BASE - PERIPH_BASE)

/* Alias word address of HSION bit */
//#define CR_OFFSET                            (RCC_OFFSET + 0x00)

/* Alias word address of PLLSAION bit */
//#define PLLSAION_BitNumber                   0x1C
//#define CR_PLLSAION_BB                       (PERIPH_BB_BASE + (CR_OFFSET * 32) + (PLLSAION_BitNumber * 4))

/* RCC Flag Mask */
#define FLAG_MASK                            ((UINT8)0x1F)
#define RCC_AHB3Periph_FMC                   ((UINT32)0x00000001)
#define GPIO_AF_FMC                          ((UINT8)0xC)  /* FMC Alternate Function mapping */


/* Microsecond delay function. */
VOID        ESAL_PR_Delay_USec (UINT32 no_of_usec);
VOID        RCC_PLL_SAI_Configuration(UINT32 PLLSAIN, UINT32 PLLSAIQ, UINT32 PLLSAIR);
VOID        RCC_PLL_SAI_Enable(FunctionalState NewState);
VOID        RCC_LTDC_Clock_Div_Configuration(UINT32 RCC_PLLSAIDivR);
VOID        RCC_AHB1_Periph_Clock_Enable(UINT32 RCC_AHB1Periph, FunctionalState NewState);
VOID        RCC_APB2_Periph_Clock_Enable(UINT32 RCC_APB2Periph, FunctionalState NewState);
VOID        RCC_APB1_Periph_Reset(UINT32 RCC_APB1Periph, FunctionalState NewState);
VOID        RCC_APB2_Periph_Reset(UINT32 RCC_APB2Periph, FunctionalState NewState);
VOID        GPIO_Initialization(UINT32 GPIOx, GPIO_Init* GPIO_InitStruct);
VOID        GPIO_Set_Bits(UINT32 GPIOx, UINT16 GPIO_Pin);
VOID        GPIO_Reset_Bits(UINT32 GPIOx, UINT16 GPIO_Pin);
VOID        GPIO_Write_Bit(UINT32 GPIOx, UINT16 GPIO_Pin, BitAction BitVal);
VOID        GPIO_Pin_Configuration(UINT32 GPIOx, UINT16 GPIO_PinSource, UINT8 GPIO_AF);
VOID        FMC_SDRAM_Initialization(FMC_SDRAM_Init* FMC_SDRAMInitStruct);
VOID        FMC_SDRAM_Command_Configuratoin(FMC_SDRAM_Command_Type* FMC_SDRAMCommandStruct);
VOID        FMC_Set_Refresh_Count(UINT32 FMC_Count);
FlagStatus  FMC_Get_Flag_Status(UINT32 FMC_Bank, UINT32 FMC_FLAG);
FlagStatus  RCC_Get_Flag_Status(UINT8 RCC_FLAG);
VOID        Delay(volatile UINT32 nCount);

#endif  /* STM32_DEFS_H */

/***********************************************************************
*
*             Copyright 2012 Mentor Graphics Corporation
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
*       stm32.c
*
*   DESCRIPTION
*
*       This file contains the functionality for the STM32 series
*       of processors
*
*   FUNCTIONS
*
*       ESAL_PR_ISR_Initialize
*       ESAL_PR_INT_All_Disable
*       ESAL_PR_INT_Enable
*       ESAL_PR_INT_Disable
*       ESAL_PR_TMR_OS_Timer_Start
*       ESAL_PR_Delay_USec
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*
***********************************************************************/

/* Include required header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"

#if (ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_PR_ISR_Initialize
*
*   DESCRIPTION
*
*       This function is the processor ISR initialization
*       function.  It is responsible for registering any ISRs for
*       processor related interrupts, initializing processor
*       specific data for interrupt handling, etc.
*
*   CALLED BY
*
*       ESAL_GE_ISR_Initialize
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_PR_ISR_Initialize(VOID)
{

}

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_PR_INT_All_Disable
*
*   DESCRIPTION
*
*       This function disables all interrupt sources for the given
*       processor
*
*   CALLED BY
*
*       ESAL_GE_INT_All_Disable
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    ESAL_PR_INT_All_Disable(VOID)
{
    INT i;

    /* Loop through all possible clear registers */
    for (i=0;i < ESAL_AR_INT_NVIC_MAX_CLR_REGS; i++)
    {
        /* Disable all interrupt sources on the processor. */
        ESAL_GE_MEM_WRITE32(ESAL_AR_INT_NVIC_BASE_ADDR +
                            ESAL_AR_INT_NVIC_CLRENA0_OFFSET + (i * 4),
                            ESAL_AR_INT_NVIC_DISABLE_VALUE);
    }
}

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_PR_INT_Enable
*
*   DESCRIPTION
*
*       This function enables the interrupt source on the processor
*       that is associated with the specified vector ID
*
*       NOTE:  This does not enable the interrupt at the source (ie
*              a device may need to enable it's interrupt within
*              a device control register).  This enables the
*              interrupt at the processor's interrupt
*              controller.
*
*   CALLED BY
*
*       ESAL_GE_INT_Enable
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       vector_id                           Vector ID of interrupt
*                                           source being enabled
*       trigger_type                        Trigger method for given
*                                           interrupt vector ID
*       priority                            Priority for the given
*                                           interrupt vector ID
*
*   OUTPUTS
*
*       INT                                 vector_id if passed
*
***********************************************************************/
INT     ESAL_PR_INT_Enable(INT                     vector_id,
                           ESAL_GE_INT_TRIG_TYPE   trigger_type,
                           INT                     priority)
{
    INT     bit_num;
    UINT32  reg_addr;
    UINT32  priority_reg_val;
    UINT32  priority_bit_shift;


    /* Get bit number for this external interrupt's vector ID */
    bit_num = vector_id - ESAL_AR_INT_VECTOR_ID_DELIMITER;

    /* Determine set-enable register to use for this vector ID. */
    reg_addr = ESAL_AR_INT_NVIC_SETENA0_OFFSET +
              ((bit_num/ESAL_AR_INT_NVIC_ENABITS_PER_REG) * ESAL_AR_INT_NVIC_REG_SIZE);

    /* Enable / unmask interrupt by setting the appropriate bit in the
       determined set-enable register */
    ESAL_GE_MEM_WRITE32(ESAL_AR_INT_NVIC_BASE_ADDR + reg_addr,
                        ESAL_GE_MEM_32BIT_SET(bit_num & ESAL_AR_INT_NVIC_ENABIT_MASK));

    /* Determine priority register to use for this vector ID */
    reg_addr = ESAL_AR_INT_NVIC_PRIORITY_OFFSET +
               ((bit_num/ESAL_AR_INT_NVIC_PRI_PER_REG) * ESAL_AR_INT_NVIC_REG_SIZE);

    /* Read current priority register value */
    priority_reg_val = ESAL_GE_MEM_READ32(ESAL_AR_INT_NVIC_BASE_ADDR + reg_addr);

    /* Calculate the priority bit shift value for this vector */
    priority_bit_shift = (ESAL_AR_INT_NVIC_PRI_BITS * (bit_num & ESAL_AR_INT_NVIC_PRI_SHIFT_MASK));

    /* Clear current priority bits for this vector ID */
    priority_reg_val &= ~(ESAL_AR_INT_NVIC_PRI_BIT_MASK << priority_bit_shift);

    /* Ensure passed-in priority is within range of processor supported values */
    priority &= ((1 << ESAL_PR_INT_NUM_PRIORITY_BITS) - 1);

    /* Shift the passed-in priority to appropriate position within the priority field */
    priority <<= (ESAL_AR_INT_NVIC_PRI_BITS-ESAL_PR_INT_NUM_PRIORITY_BITS);

    /* Set new priority bits for this vector ID */
    priority_reg_val |= (priority << priority_bit_shift);

    /* Write the new priority register value back */
    ESAL_GE_MEM_WRITE32(ESAL_AR_INT_NVIC_BASE_ADDR + reg_addr, priority_reg_val);

    /* Return the vector ID */
    return (vector_id);
}

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_PR_INT_Disable
*
*   DESCRIPTION
*
*       This function disables the interrupt source on the processor
*       that is associated with the specified vector ID
*
*       NOTE:  This does not disable the interrupt at the source (ie
*              a device may need to enable it's interrupt within
*              a device control register).  This disables the
*              interrupt at the processor's interrupt controller.
*
*   CALLED BY
*
*       ESAL_GE_INT_Disable
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       vector_id                           Vector ID of interrupt
*                                           source being disabled
*
*   OUTPUTS
*
*       INT                                 vector_id if passed
*
***********************************************************************/
INT     ESAL_PR_INT_Disable(INT vector_id)
{
    INT     bit_num;
    UINT32  reg_addr;

    /* Get bit number for this external interrupt's vector ID */
    bit_num = vector_id - ESAL_AR_INT_VECTOR_ID_DELIMITER;

    /* Determine clear-enable register to use for this vector ID. */
    reg_addr = ESAL_AR_INT_NVIC_CLRENA0_OFFSET +
               ((bit_num/ESAL_AR_INT_NVIC_ENABITS_PER_REG) * ESAL_AR_INT_NVIC_REG_SIZE);

    /* Disable / mask interrupt by setting the appropriate bit in the
       determined clear-enable register */
    ESAL_GE_MEM_WRITE32(ESAL_AR_INT_NVIC_BASE_ADDR + reg_addr,
                        ESAL_GE_MEM_32BIT_SET(bit_num & ESAL_AR_INT_NVIC_ENABIT_MASK));

    /* Return the vector ID */
    return (vector_id);
}

#endif  /* ESAL_PR_INTERRUPTS_AVAILABLE == NU_TRUE */

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_PR_TMR_OS_Timer_Start
*
*   DESCRIPTION
*
*       This function starts the Operating System utilized timer.
*       This includes determining the required timer count for the
*       specified period, enabling the timer to produce this period,
*       and enabling the interrupt associated with this timer.
*
*   CALLED BY
*
*       ESAL_GE_TMR_OS_Timer_Start
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       ticks_per_sec                       Number of timer interrupts
*                                           that occur in a 1 sec period
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID ESAL_PR_TMR_OS_Timer_Start(UINT32 ticks_per_sec)
{

}

/*************************************************************************
*
* FUNCTION
*
*       ESAL_PR_Delay_USec
*
* DESCRIPTION
*
*       Wait for the specified microseconds.
*
* INPUTS
*
*       no_of_usec                          No of microseconds to sleep.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID  ESAL_PR_Delay_USec (UINT32 no_of_usec)
{
    UNSIGNED            timer_ticks;
    UNSIGNED            start_time;
    UNSIGNED            end_time;
    UNSIGNED            loop_counter = 1;

    /* Check if required delay is more than 1ms. */
    if (no_of_usec > 1000)
    {
        /* Calculate loop counter. */
        loop_counter = (no_of_usec / 1000) + 1;

        /* Reevaluate count. */
        no_of_usec = no_of_usec % 1000;
    }

    do
    {
        /* Retrieve Start Time value */
        NU_Retrieve_Hardware_Clock(start_time);

        /* Calculate number of timer ticks for count */
        timer_ticks = ((NU_HW_Ticks_Per_Second/100)*no_of_usec)/10000;

        do
        {
            /* Retrieve Clock value */
            NU_Retrieve_Hardware_Clock(end_time);

        } while (ESAL_PR_CALC_TIME(start_time,end_time)  < timer_ticks);

        /* Reset count. */
        no_of_usec = 1000;

        /* Decrement loop counter. */
        loop_counter--;

    } while (loop_counter > 0);
}

/*************************************************************************
*
*   FUNCTION
*
*       FMC_SDRAM_Initialization
*
*   DESCRIPTION
*
*       This function initialize SDRAM with FMC.
*
*   INPUT
*
*       fmc_init                      - FMC SDRAM initialization structure 
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID FMC_SDRAM_Initialization(FMC_SDRAM_Init* fmc_init)
{ 
    UINT32 temp1;
    UINT32 temp2;
    UINT32 temp3;
    UINT32 temp4;

    /* SDRAM bank control register configuration */ 
    temp1 =  (UINT32)fmc_init->FMC_ColumnBitsNumber |
             fmc_init->FMC_RowBitsNumber |
             fmc_init->FMC_SDMemoryDataWidth |
             fmc_init->FMC_InternalBankNumber |           
             fmc_init->FMC_CASLatency |
             fmc_init->FMC_WriteProtection |
             fmc_init->FMC_SDClockPeriod |
             fmc_init->FMC_ReadBurst | 
             fmc_init->FMC_ReadPipeDelay;
            
    if(fmc_init->FMC_Bank == FMC_Bank1_SDRAM )
    {
        ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x04, temp1);
    }
    else   /* SDCR2 "don't care" bits configuration */
    {
        temp3 = (UINT32)fmc_init->FMC_SDClockPeriod |
                 fmc_init->FMC_ReadBurst | 
                 fmc_init->FMC_ReadPipeDelay;

        ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x00, temp3);
        ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x04, temp1);
    }

    /* SDRAM bank timing register configuration */
    if(fmc_init->FMC_Bank == FMC_Bank1_SDRAM )
    {
        temp2 =   (UINT32)((fmc_init->FMC_SDRAMTimingStruct->FMC_LoadToActiveDelay)-1) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_ExitSelfRefreshDelay)-1) << 4) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_SelfRefreshTime)-1) << 8) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_RowCycleDelay)-1) << 12) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_WriteRecoveryTime)-1) << 16) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_RPDelay)-1) << 20) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_RCDDelay)-1) << 24);
            
            ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x0C, temp2);
    }
    /* SDTR "don't care bits configuration */
    else   
    {
        temp2 =   (UINT32)((fmc_init->FMC_SDRAMTimingStruct->FMC_LoadToActiveDelay)-1) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_ExitSelfRefreshDelay)-1) << 4) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_SelfRefreshTime)-1) << 8) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_WriteRecoveryTime)-1) << 16);
            
        temp4 =   (UINT32)(((fmc_init->FMC_SDRAMTimingStruct->FMC_RowCycleDelay)-1) << 12) |
            (((fmc_init->FMC_SDRAMTimingStruct->FMC_RPDelay)-1) << 20);
            
            ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x08, temp4);
            ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x0C, temp2);
    }  
}

/*************************************************************************
*
*   FUNCTION
*
*       FMC_SDRAM_Command_Configuratoin
*
*   DESCRIPTION
*
*       This function does SDRAM Control configurations 
*
*   INPUT
*
*       fmc                      - FMC SDRAM Command structure 
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID FMC_SDRAM_Command_Configuratoin(FMC_SDRAM_Command_Type* fmc)
{
    UINT32 temp32;
      
    temp32 =   (UINT32)(fmc->FMC_CommandMode | fmc->FMC_CommandTarget | (((fmc->FMC_AutoRefreshNumber)-1)<<5) | ((fmc->FMC_ModeRegisterDefinition)<<9));

    ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x10, temp32);
}

/*************************************************************************
*
*   FUNCTION
*
*       FMC_Set_Refresh_Count
*
*   DESCRIPTION
*
*       This function sets FMC refresh count
*
*   INPUT
*
*       fmc_count                      - FMC Refresh Count  
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID FMC_Set_Refresh_Count(UINT32 fmc_count)
{
    ESAL_GE_MEM_WRITE32(FMC_BANK5_6_BASE_ADDRESS + 0x14, ESAL_GE_MEM_READ32(FMC_BANK5_6_BASE_ADDRESS + 0x14) | (fmc_count<<1));
}

/*************************************************************************
*
*   FUNCTION
*
*       FMC_Get_Flag_Status
*
*   DESCRIPTION
*
*       This function returns flag status
*
*   INPUT
*
*       bank                      - FMC Bank Number  
*       flag                      - Flag value 
*
*   OUTPUT
*
*       FlagStatus                - Flag status 
*
*************************************************************************/
FlagStatus FMC_Get_Flag_Status(UINT32 bank, UINT32 flag)
{
    FlagStatus bitstatus = RESET;
    UINT32 temp32;

    temp32 = ESAL_GE_MEM_READ32(FMC_BANK5_6_BASE_ADDRESS + 0x18);

    /* Get the flag status */
    if ((temp32 & flag) != flag )
    {
        bitstatus = RESET;
    }
    else
    {
        bitstatus = SET;
    }
    
    /* Return the flag status */
    return bitstatus;
}

/*************************************************************************
*
*   FUNCTION
*
*       GPIO_Initialization
*
*   DESCRIPTION
*
*       This function initialize GPIO
*
*   INPUT
*
*       gpiox                      - GPIO Base address   
*       gpio_init                  - GPIO Init Structure  
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID GPIO_Initialization(UINT32 gpiox, GPIO_Init* gpio_init)
{
    UINT32 pinpos, pos, currentpin;

    /* GPIO Mode Configuration */
    for (pinpos = 0; pinpos < 0x10; pinpos++)
    {
        pos = ((UINT32)0x01) << pinpos;
        
        /* Get the port pins position */
        currentpin = (gpio_init->GPIO_Pin) & pos;

        if (currentpin == pos)
        {
            ESAL_GE_MEM_WRITE32(gpiox + 0x00, ESAL_GE_MEM_READ32(gpiox + 0x00) & ~(GPIO_MODER_MODER0 << (pinpos * 2)));
            ESAL_GE_MEM_WRITE32(gpiox + 0x00, ESAL_GE_MEM_READ32(gpiox + 0x00) | (((UINT32)gpio_init->GPIO_Mode) << (pinpos * 2)));

            if ((gpio_init->GPIO_Mode == GPIO_Mode_OUT) || (gpio_init->GPIO_Mode == GPIO_Mode_AF))
            {
                /* Speed mode configuration */
                ESAL_GE_MEM_WRITE32(gpiox + 0x08, ESAL_GE_MEM_READ32(gpiox + 0x08) & ~(GPIO_OSPEEDER_OSPEEDR0 << (pinpos * 2)));
                ESAL_GE_MEM_WRITE32(gpiox + 0x08, ESAL_GE_MEM_READ32(gpiox + 0x08) | ((UINT32)(gpio_init->GPIO_Speed) << (pinpos * 2)));

                /* Output mode configuration*/
                ESAL_GE_MEM_WRITE32(gpiox + 0x04, ESAL_GE_MEM_READ32(gpiox + 0x04) & ~((GPIO_OTYPER_OT_0) << ((UINT16)pinpos)));
                ESAL_GE_MEM_WRITE32(gpiox + 0x04, ESAL_GE_MEM_READ32(gpiox + 0x04) | (UINT16)(((UINT16)gpio_init->GPIO_OType) << ((UINT16)pinpos)));
              }

            /* Pull-up Pull down resistor configuration*/
            ESAL_GE_MEM_WRITE32(gpiox + 0x0C, ESAL_GE_MEM_READ32(gpiox + 0x0C) & ~(GPIO_PUPDR_PUPDR0 << ((UINT16)pinpos * 2)));
            ESAL_GE_MEM_WRITE32(gpiox + 0x0C, ESAL_GE_MEM_READ32(gpiox + 0x0C) | (((UINT32)gpio_init->GPIO_PuPd) << (pinpos * 2)));
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       GPIO_Set_Bits
*
*   DESCRIPTION
*
*       This function set GPIO pin 
*
*   INPUT
*
*       gpiox                     - GPIO Base address   
*       gpio_pin                  - GPIO pin number   
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID GPIO_Set_Bits(UINT32 gpiox, UINT16 gpio_pin)
{
    ESAL_GE_MEM_WRITE16(gpiox + 0x18, gpio_pin);
}

/*************************************************************************
*
*   FUNCTION
*
*       GPIO_Reset_Bits
*
*   DESCRIPTION
*
*       This function reset GPIO pin
*
*   INPUT
*
*       gpiox                     - GPIO Base address   
*       gpio_pin                  - GPIO pin number   
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID GPIO_Reset_Bits(UINT32 gpiox, UINT16 gpio_pin)
{
    ESAL_GE_MEM_WRITE16(gpiox + 0x1A, gpio_pin);
}

/*************************************************************************
*
*   FUNCTION
*
*       GPIO_Write_Bit
*
*   DESCRIPTION
*
*       This function write GPIO pin
*
*   INPUT
*
*       gpiox                     - GPIO Base address   
*       gpio_pin                  - GPIO pin number   
*       val                       - Set or Reset option value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID GPIO_Write_Bit(UINT32 gpiox, UINT16 gpio_pin, BitAction val)
{
    if (val != Bit_RESET)
    {
        ESAL_GE_MEM_WRITE16(gpiox + 0x18, gpio_pin);
    }
    else
    {
        ESAL_GE_MEM_WRITE16(gpiox + 0x1A, gpio_pin);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       GPIO_Pin_Configuration
*
*   DESCRIPTION
*
*       This function reset GPIO pin
*
*   INPUT
*
*       gpiox                     - GPIO Base address   
*       gpio_pin_source           - GPIO pin source    
*       value                     - value to be written 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID GPIO_Pin_Configuration(UINT32 gpiox, UINT16 gpio_pin_source, UINT8 value)
{
    UINT32 temp1;
    UINT32 temp2;

    temp1 = ((UINT32)(value) << ((UINT32)((UINT32)gpio_pin_source & (UINT32)0x07) * 4)) ;
    ESAL_GE_MEM_WRITE32(gpiox + 0x20 + (gpio_pin_source >> 0x03) * 4, ESAL_GE_MEM_READ32(gpiox + 0x20 + (gpio_pin_source >> 0x03) * 4) & ~((UINT32)0xF << ((UINT32)((UINT32)gpio_pin_source & (UINT32)0x07) * 4)));
    temp2 = ESAL_GE_MEM_READ32(gpiox + 0x20 + (gpio_pin_source >> 0x03) * 4) | temp1;
    ESAL_GE_MEM_WRITE32(gpiox + 0x20 + (gpio_pin_source >> 0x03) * 4, temp2);
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_PLL_SAI_Configuration
*
*   DESCRIPTION
*
*       This function sets dividing factors of PLL SAI    
*
*   INPUT
*
*       PLLSAI_N                      - N Factor     
*       PLLSAI_Q                      - Q Factor 
*       PLLSAI_R                      - R Factor 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_PLL_SAI_Configuration(UINT32 PLLSAI_N, UINT32 PLLSAI_Q, UINT32 PLLSAI_R)
{
    ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_PLLSAICFGR, (PLLSAI_N << 6) | (PLLSAI_Q << 24) | (PLLSAI_R << 28) );
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_PLL_SAI_Enable
*
*   DESCRIPTION
*
*       This function enable/disable PLL SAI    
*
*   INPUT
*
*       option                      - Enable or Disable value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_PLL_SAI_Enable(FunctionalState value)
{
    ESAL_GE_MEM_WRITE32(CR_PLLSAION_BB, (UINT32)value );
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_LTDC_Clock_Div_Configuration
*
*   DESCRIPTION
*
*       This function set clock div configuration of PLL SAI    
*
*   INPUT
*
*       div                      - Divider value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_LTDC_Clock_Div_Configuration(UINT32 div)
{
    UINT32 temp;

    temp = ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_DCKCFGR);

    /* Clear PLLSAIDIVR[2:0] bits */
    temp &= ~RCC_DCKCFGR_PLLSAIDIVR;

    /* Set PLLSAIDIVR values */
    temp |= div;

    /* Store the new value */
    ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_DCKCFGR, temp);
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_AHB1_Periph_Clock_Enable
*
*   DESCRIPTION
*
*       This function enables/disables RCC AHB1 peripheral 
*
*   INPUT
*
*       pher                      - Peripheral to be enable/disable 
*       option                    - Enable / Disable value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_AHB1_Periph_Clock_Enable(UINT32 pher, FunctionalState option)
{
    if (option != DISABLE)
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_AHB1ENR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_AHB1ENR) | pher);
    }
    else
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_AHB1ENR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_AHB1ENR) & ~pher);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_AHB3_Periph_Clock_Enable 
*
*   DESCRIPTION
*
*       This function enables/disables RCC AHB3 peripheral 
*
*   INPUT
*
*       pher                      - Peripheral to be enable/disable 
*       option                    - Enable / Disable value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_AHB3_Periph_Clock_Enable (UINT32 pher, FunctionalState option)
{
    if (option != DISABLE)
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_AHB3ENR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_AHB3ENR) | pher);
    }
    else
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_AHB3ENR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_AHB3ENR) & ~pher);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_APB2_Periph_Clock_Enable
*
*   DESCRIPTION
*
*       This function enables/disables RCC APB2 peripheral 
*
*   INPUT
*
*       pher                      - Peripheral to be enable/disable 
*       option                    - Enable / Disable value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_APB2_Periph_Clock_Enable(UINT32 pher, FunctionalState option)
{
    if (option != DISABLE)
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_APB2ENR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_APB2ENR) | pher);
    }
    else
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_APB2ENR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_APB2ENR) & ~pher);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_APB1_Periph_Reset
*
*   DESCRIPTION
*
*       This function reset RCC APB1 peripheral 
*
*   INPUT
*
*       pher                      - Peripheral to be enable/disable 
*       option                    - Enable / Disable value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_APB1_Periph_Reset(UINT32 pher, FunctionalState option)
{
    if (option != DISABLE)
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_APB1RSTR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_APB1RSTR) | pher);
    }
    else
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_APB1RSTR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_APB1RSTR) & ~pher);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_APB2_Periph_Reset
*
*   DESCRIPTION
*
*       This function reset RCC APB2 peripheral 
*
*   INPUT
*
*       pher                      - Peripheral to be enable/disable 
*       option                    - Enable / Disable value 
*
*   OUTPUT
*
*       None 
*
*************************************************************************/
VOID RCC_APB2_Periph_Reset(UINT32 pher, FunctionalState option)
{
    if (option != DISABLE)
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_APB2RSTR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_APB2RSTR) | pher);
    }
    else
    {
        ESAL_GE_MEM_WRITE32(RCC_BASE_ADDRESS + RCC_APB2RSTR, ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_APB2RSTR) & ~pher);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       RCC_Get_Flag_Status
*
*   DESCRIPTION
*
*       This function returns flag status 
*
*   INPUT
*
*       flag                      - flag value 
*
*   OUTPUT
*
*       bitstatus                 - flag status  
*
*************************************************************************/
FlagStatus RCC_Get_Flag_Status(UINT8 flag)
{
    UINT32 temp = 0;
    UINT32 status = 0;
    FlagStatus bitstatus = RESET;

    /* Get the RCC register index */
    temp = flag >> 5;
    
    if (temp == 1)               /* The flag to check is in CR register */
    {
        status = ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_CR);
    }
    else if (temp == 2)          /* The flag to check is in BDCR register */
    {
        status = ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_BDCR);
    }
    else                       /* The flag to check is in CSR register */
    {
        status = ESAL_GE_MEM_READ32(RCC_BASE_ADDRESS + RCC_CSR);
    }

    /* Get the flag position */
    temp = flag & FLAG_MASK;
    if ((status & ((UINT32)1 << temp)) != (UINT32)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }
    
    /* Return the flag status */
    return bitstatus;
}

/*************************************************************************
*
*   FUNCTION
*
*       Delay
*
*   DESCRIPTION
*
*       This function delays for a specified value      
*
*   INPUT
*
*       value                  - delay value 
*
*   OUTPUT
*
*       None  
*
*************************************************************************/
VOID Delay(volatile UINT32 vlaue)
{
    volatile UINT32 index = 0; 

    for(index = vlaue; index != 0; index--)
    {

    }
}
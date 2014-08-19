/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       cpu_tgt.c
*
*   COMPONENT
*
*       CPU DVFS, Idle and Wakeup CPU Driver
*
*   DESCRIPTION
*
*       Open, Close and Ioctl driver functions
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       nu_bsp_drvr_cpu_stm3220g_init 
*       CPU_Tgt_Get_Prescalar_Value
*       CPU_Tgt_Calculate_Scale
*       CPU_Tgt_Idle
*       CPU_Tgt_Wakeup
*       CPU_Tgt_Set_OP
*       CPU_Tgt_Get_Spec_Frequency
*       CPU_Tgt_Frequency_Initialize
*       CPU_Tgt_Frequency_Initialize
*       CPU_Tgt_Get_OP_Frequency
*       CPU_Tgt_OP_Additional
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       cpu_tgt.h
*       <string.h>
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "bsp/drivers/cpu/stm32f2x/cpu_tgt.h"
#include <string.h>

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/* User implementation of CPU idle and wakeup */
VOID    CPU_Tgt_Idle(UINT32 expected_idle_time, UINT32 wakeup_constraint);
VOID    CPU_Tgt_Wakeup(VOID);

/* Local function to set new op */
STATUS  CPU_Tgt_Get_Spec_Frequency(INT op_index, UINT32 *specific_info,
                                  CPU_DVFS_SPECIFIC *specific_info_ioctl);
VOID    CPU_Tgt_Calculate_Scale(VOID);
VOID    CPU_Tgt_Set_OP(UINT8 new_op);
UINT32  CPU_Tgt_Switch_Clock_Time(UINT8 from_op, UINT8 to_op);
UINT32  CPU_Tgt_Get_Prescalar_Value(UINT16 *table, UINT16 table_len, UINT16 compare_val);

/* CPU registration information */
CPU_REGISTER       CPU_register_info;
static CPU_SCALE   CPU_OP_Scale[CPU_OP_ARRAY_SIZE];

#endif /* end if CFG_NU_OS_SVCS_PWR_ENABLE IS defined */

/* Array for DVFS OP */
CPU_DVFS_OP CPU_OP_Set[CPU_OP_ARRAY_SIZE];
UINT32      CPU_OP_Count = 0;
UINT8       CPU_Current_OP = 255;

/* Voltage is constant in this driver */
UINT32      CPU_VOLTAGE = 1;

/* AHB clock prescalar table. */
static UINT16      AHBPRE_Table[16] = {1,1,1,1,1,1,1,1,2,4,8,16,64,128,256,512};

/* APB clock prescalar table. */
static UINT16      APBPRE_Table[8] = {1,1,1,1,2,4,8,16};

/* Local functions */
static VOID   CPU_Tgt_Set_OP_Frequency(CPU_DVFS_OP *op_ptr, UINT32 main_osc);
static STATUS CPU_Tgt_Frequency_Initialize(VOID);

/* Clock source frequencies. */
#define CPU_HSI_CYCLES                      CPU_FREQ_16MHZ  /* Internal OSC clock cycles in Hz. */
#define CPU_HSE_CYCLES                      CFG_IAR_STM32F429II_SK_CPU0_EXT_CLOCK  /* External OSC clock cycles in Hz. */
#define CPU_PLL_CYCLES                      CFG_IAR_STM32F429II_SK_CPU0_CORE_CLOCK /* PLL clock cycles in Hz. */


/***********************************************************************
*
*   FUNCTION
*
*       nu_bsp_drvr_cpu_stm32f2x_init
*
*   DESCRIPTION
*
*       Provides a place to attach target-specific labels to the component
*       and calls the component driver registration function.
*
*   CALLED BY
*
*       System Registry
*
*   CALLS
*
*       CPU_Register
*       CPU_Unregister
*
*   INPUTS
*
*       key                                 Path to registry
*       startorstop                         Option to Register or Unregister
*
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_bsp_drvr_cpu_stm32f2x_init (const CHAR * key, INT startstop)
{
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    VOID        (*setup_fn)(VOID);

    if(startstop)
    {
        /* If there is a setup function, call it */
        STATUS status = REG_Get_UINT32_Value(key, "/setup", (UINT32 *)&setup_fn);
        if (status == NU_SUCCESS)
        {
            if (setup_fn != NU_NULL)
            {
                setup_fn();
            }
        }
    
        /* Setup the DVFS OP table */
        CPU_OP_Count = CPU_register_info.pm_count;
        memcpy(&CPU_OP_Set[0], CPU_register_info.pm_op_list, (sizeof(CPU_DVFS_OP) * CPU_OP_Count));

#endif
        /* Compute the frequencies and initialize the clock structure */
        (VOID)CPU_Tgt_Frequency_Initialize();

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

        /* Register the device, if a stop command is passed and the device was
           previously registered unregister it otherwise fall through
           the function doing nothing */
        /* Register the CPU driver */
        (VOID)CPU_Dv_Register(key, NU_NULL);
    }
    else
    {
        CPU_Dv_Unregister(key, startstop, NU_NULL);
    }

#endif
}

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Get_OP_Frequency
*
*   DESCRIPTION
*
*       Returns the frequency of the requested operating point.
*
*   INPUT
*
*       op_index                                Information index
*
*   OUTPUT
*
*       op_frequency                            The operating point frequency
*
*************************************************************************/
UINT32 CPU_Tgt_Get_OP_Frequency(INT op_index)
{
    return CPU_OP_Set[op_index].pm_ahb_frequency;
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Get_OP_Additional
*
*   DESCRIPTION
*
*       Calculates numerator and denominator to be used in scaling CPU
*       utilization numbers.
*
*   INPUT
*
*       op_index                                Information index
*       pm_info                                 Return info
*
*   OUTPUT
*
*       status
*
*************************************************************************/
STATUS CPU_Tgt_Get_OP_Additional(INT op_index, VOID *pm_info)
{
    STATUS              status = ~NU_SUCCESS;
    CPU_DVFS_OP_INFO    *additional_info = ((CPU_DVFS_OP_INFO*)pm_info);

    if (((op_index < CPU_OP_ARRAY_SIZE) && (op_index >= 0)) &&
        (additional_info != NU_NULL))
    {
        additional_info -> pm_master_freq = CPU_OP_Set[op_index].pm_ahb_frequency;
        status = NU_SUCCESS;
    }
    
    /* Execute ARM WFI instruction. */
    ESAL_TS_RTE_WFI_EXECUTE();

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Get_Spec_Frequency
*
*   DESCRIPTION
*
*       Returns the frequency of a specific clock that is specified.
*
*   INPUT
*
*       op_index                                Information index
*       specific_info                           Return info
*       specific_info_ioctl                     Requestion information
*
*   OUTPUT
*
*       status
*
*************************************************************************/
STATUS CPU_Tgt_Get_Spec_Frequency(INT op_index, UINT32 *specific_info,
                              CPU_DVFS_SPECIFIC *specific_info_ioctl)
{
    STATUS status = NU_SUCCESS;

    if (specific_info != NU_NULL)
    {
        /* Compare the string */
        if (strcmp(CPU_AHBCLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return AHB clock frequency. */
            *specific_info = CPU_OP_Set[op_index].pm_ahb_frequency;
        }
        else if (strcmp(CPU_APB1CLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return APB1 clock frequency. */
            *specific_info = CPU_OP_Set[op_index].pm_apb1_frequency;
        }
        else if (strcmp(CPU_APB2CLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return APB2 clock frequency. */
            *specific_info = CPU_OP_Set[op_index].pm_apb2_frequency;
        }
        else if (strcmp(CPU_PLL48CLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return PLL48CLK clock frequency. */
            *specific_info = CPU_FREQ_PLL48CLK;
        }
        else if (strcmp(CPU_MACCLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return MACCLK clock frequency. */
            *specific_info = CPU_FREQ_MACCLK;
        }
        else if (strcmp(CPU_USBHSCLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return USBHSCLK clock frequency. */
            *specific_info = CPU_FREQ_USBHSCLK;
        }
        else if (strcmp(CPU_RTCCLK_FREQ, specific_info_ioctl -> pm_identifier) == 0)
        {
            /* Return RTCCLK clock frequency. */
            *specific_info = CPU_FREQ_RTCCLK;
        }
        else
        {
            status = ~(NU_SUCCESS);
        }
    }

    else
    {
        status = ~(NU_SUCCESS);
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Calculate_Scale
*
*   DESCRIPTION
*
*       Calculates numerator and denominator to be used in scaling CPU
*       utilization numbers.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID CPU_Tgt_Calculate_Scale(VOID)
{
    INT     op_index, high_op_index = 0;
    UINT32  highest_freq, current_freq;
    UINT32  denominator, numerator, remainder;

    /* Check if valid operating point count value. */
    if ((CPU_OP_Count != 0) && (CPU_OP_Count < CPU_OP_ARRAY_SIZE))
    {
        /* Highest op index is the count - 1. */
        high_op_index = CPU_OP_Count - 1;
        highest_freq = CPU_OP_Set[high_op_index].pm_ahb_frequency;

        /* Update the highest OP with 1. */
        CPU_OP_Scale[high_op_index].pm_numerator = 1;
        CPU_OP_Scale[high_op_index].pm_denominator = 1;

        /* Loop through all of the op's to calculate the scaling numbers
           except the highest as it is a known value of 1 */
        for (op_index = 0; op_index < high_op_index; op_index++)
        {
            /* Set initial values */
            current_freq = CPU_OP_Set[op_index].pm_ahb_frequency;
            numerator = highest_freq;
            denominator = current_freq;

            /* Apply Euclid's algorithm for GCD */
            while (denominator != 0)
            {
                remainder = numerator % denominator;
                numerator = denominator;
                denominator = remainder;
            }

            /* Calculate and save the new scaling fraction */
            CPU_OP_Scale[op_index].pm_denominator = current_freq / numerator;
            CPU_OP_Scale[op_index].pm_numerator = highest_freq / numerator;
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Idle
*
*   DESCRIPTION
*
*       User implementation of CPU idle
*
*   INPUT
*
*       expected_idle_time      Not currently used by this driver
*       wakeup_constraint       Not currently used by this driver
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID CPU_Tgt_Idle(UINT32 expected_idle_time, UINT32 wakeup_constraint)
{
    /* Avoid warnings from unused parameters */
    NU_UNUSED_PARAM(expected_idle_time);
    NU_UNUSED_PARAM(wakeup_constraint);

    /* Execute ARM WFI instruction. */
    ESAL_TS_RTE_WFI_EXECUTE();
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Wakeup
*
*   DESCRIPTION
*
*       User implementation of CPU Wakeup
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID CPU_Tgt_Wakeup(VOID)
{
    /* Nothing to do. */
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Set_OP
*
*   DESCRIPTION
*
*       User implementation of set op, updates the frequency/clock and
*       scales the timer tick.
*
*   INPUT
*
*       new_op                  New OP to set
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID CPU_Tgt_Set_OP(UINT8 new_op)
{
    UINT32    piv;
    INT       old_int_level;
    UINT32    old_piv;
    UINT32    new_interval;
    UINT32    old_interval;
    UINT32    new_hw_timer;
    UINT32    old_hw_timer;
    UINT32    cfgr;
    UINT32    hpre;
    UINT32    ppre1;
    UINT32    ppre2;


    /* Disable interrupts during the actual OP change */
    old_int_level = NU_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Read the Interval counter to clear any interrupts */
    piv = ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL);

    /* Poll until counter reduce to zero (when enabled).
       This happens when the COUNT flag gets set. */
    while (!(piv & ESAL_AR_TMR_SYSTICK_CTRL_COUNT_FLAG) && (piv & ESAL_AR_TMR_SYSTICK_CTRL_ENABLE_BIT))
    {
        piv = ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYSTICK_CTRL);
    }

    /* Disable timer. */
    ESAL_AR_SYSTICK_DISABLE();

    /* Clear current count value. */
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CURRENT, 0);

    /* Read Clock Configuration register (CFGR). */
    cfgr = ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CFGR);

    hpre  = CPU_Tgt_Get_Prescalar_Value(AHBPRE_Table, 16, CPU_OP_Set[new_op].pm_ahb_divisor);
    ppre1 = CPU_Tgt_Get_Prescalar_Value(APBPRE_Table, 16, CPU_OP_Set[new_op].pm_apb1_divisor);
    ppre2 = CPU_Tgt_Get_Prescalar_Value(APBPRE_Table, 16, CPU_OP_Set[new_op].pm_apb2_divisor);

    /* Update clock divisors for new OP. */
    cfgr &= ~(CPU_RCC_CFGR_HPRE_MASK | CPU_RCC_CFGR_PPRE1_MASK | CPU_RCC_CFGR_PPRE2_MASK);
    cfgr |= ((hpre << CPU_RCC_CFGR_HPRE_SHIFT) |
             (ppre1 << CPU_RCC_CFGR_PPRE1_SHIFT) |
             (ppre2 << CPU_RCC_CFGR_PPRE2_SHIFT));
    ESAL_GE_MEM_WRITE32(CPU_RCC_BASE + CPU_RCC_CFGR, cfgr);

    /* Calculate a new PIV */
    piv = CPU_OP_Set[new_op].pm_ahb_frequency / (ESAL_PR_TMR_OS_CLOCK_PRESCALE * NU_PLUS_TICKS_PER_SEC);

    /* Scale the tick interval */
    old_piv = PMS_GET_SINGLE_TICK();
    old_interval = PMS_GET_TICK_INTERVAL();

    /* This is calculated very precisely with the number of ticks and the tick value to avoid error
       between the interval value and the tick value.  Some round off error may occur in the remainder
       calculation but it will not cause any major issues  */
    new_interval = ((old_interval / old_piv) * piv) +
                    ((((piv * 100) / old_piv) * (old_interval % old_piv)) / 100);

    /* Scale the HW timer value */
    old_hw_timer = PMS_GET_LAST_HW_TIMER();
    new_hw_timer = ((old_hw_timer / old_piv) * piv) +
                    ((((piv * 100) / old_piv) * (old_hw_timer % old_piv)) / 100);

    /* Update the value of a single tick */
    PMS_SET_SINGLE_TICK(piv);

    /* Update the tick interval */
    PMS_SET_TICK_INTERVAL(new_interval);

    /* Update the HW tick */
    ESAL_GE_TMR_PMS_SET_HW_TICK_VALUE(new_interval);

    /* Update the HW Timer Value */
    PMS_SET_LAST_HW_TIMER(new_hw_timer);

    /* Update the scaling factors */
    PMS_SET_SCALING_FACTORS(CPU_OP_Scale[new_op].pm_numerator, CPU_OP_Scale[new_op].pm_denominator);

    /* Save new op index */
    CPU_Current_OP = new_op;

    /* A tick was missed as a result of the tick change, process it now */
    ESAL_GE_ISR_HANDLER_EXECUTE(ESAL_GE_TMR_OS_VECTOR);

    /* Enable timer. */
    ESAL_AR_SYSTICK_ENABLE();

    /* Restore interrupts */
    (VOID)NU_Control_Interrupts(old_int_level);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Get_Prescalar_Value
*
*   DESCRIPTION
*
*       This function returns the value from the prescalar table.
*
*   INPUT
*
*       table                               Pointer to the table containing
*                                           prescalar values
*       table_len                           Length of the table
*       compare_val                         Value to be compared in the table
*
*   OUTPUT
*
*       UINT32                             Return prescalar value
*
*************************************************************************/
UINT32 CPU_Tgt_Get_Prescalar_Value(UINT16 *table, UINT16 table_len, UINT16 compare_val)
{
    UINT32 value = 0;
    INT    i;

    for (i = 0; i < table_len; i++)
    {
        if (table[i] == compare_val)
        {
            value = i;
            break;
        }
    }

    return (value);
}

/*************************************************************************
*
* FUNCTION
*
*       CPU_Tgt_Switch_Clock_Time
*
* DESCRIPTION
*
*       Runs the appropriate formula for the changing of clocks and returns
*       switch time in nanoseconds
*
* INPUTS
*
*       from_op                       OP switching from
*       to_op                         OP switching to
*
* OUTPUTS
*
*       Switch time in nanoseconds    Will Always be Zero 
*
*************************************************************************/
UINT32 CPU_Tgt_Switch_Clock_Time(UINT8 from_op, UINT8 to_op)
{
    /* Avoid warnings from unused parameters */
    NU_UNUSED_PARAM(from_op);
    NU_UNUSED_PARAM(to_op);
    
    return (0);
}

#endif  /* CFG_NU_OS_SVCS_PWR_ENABLE */

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Get_Frequency
*
*   DESCRIPTION
*
*
*   INPUT
*
*
*   OUTPUT
*
*
*************************************************************************/
STATUS CPU_Tgt_Get_Frequency(UINT32 *master_freq, CHAR *identifier)
{
    STATUS status = NU_SUCCESS;

    if (master_freq != NU_NULL)
    {
        /* Compare the string */
        if (strcmp(CPU_AHBCLK_FREQ, identifier) == 0)
        {
            /* Return AHB clock frequency. */
            *master_freq = CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_ahb_frequency;
        }
        else if (strcmp(CPU_APB1CLK_FREQ, identifier) == 0)
        {
            /* Return APB1 clock frequency. */
            *master_freq = CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_apb1_frequency;
        }
        else if (strcmp(CPU_APB2CLK_FREQ, identifier) == 0)
        {
            /* Return APB2 clock frequency. */
            *master_freq = CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_apb2_frequency;
        }
        else if (strcmp(CPU_PLL48CLK_FREQ, identifier) == 0)
        {
            /* Return PLL48CLK clock frequency. */
            *master_freq = CPU_FREQ_PLL48CLK;
        }
        else if (strcmp(CPU_MACCLK_FREQ, identifier) == 0)
        {
            /* Return MACCLK clock frequency. */
            *master_freq = CPU_FREQ_MACCLK;
        }
        else if (strcmp(CPU_USBHSCLK_FREQ, identifier) == 0)
        {
            /* Return USBHSCLK clock frequency. */
            *master_freq = CPU_FREQ_USBHSCLK;
        }
        else if (strcmp(CPU_RTCCLK_FREQ, identifier) == 0)
        {
            /* Return RTCCLK clock frequency. */
            *master_freq = CPU_FREQ_RTCCLK;
        }
        else
        {
            status = ~(NU_SUCCESS);
        }
    }

    else
    {
        status = ~(NU_SUCCESS);
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Frequency_Initialize
*
*   DESCRIPTION
*
*       Calculate the AHB, APB1 and APB2 dividers and calculate operating
*       point frequencies.
*
*   INPUT
*
*
*   OUTPUT
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS CPU_Tgt_Frequency_Initialize(VOID)
{
    UINT8        op_index;
    CPU_CLOCK    source_clock;
    UINT32       cfgr;
    UINT16       ahbdiv;
    UINT16       apb1div;
    UINT16       apb2div;

    /* Read Clock Configuration register (CFGR). */
    cfgr = ESAL_GE_MEM_READ32(CPU_RCC_BASE + CPU_RCC_CFGR);

    /* Determine the source system clock to generate PLL clock. */
    source_clock = ((cfgr & CPU_RCC_CFGR_SWS_MASK) >> CPU_RCC_CFGR_SWS_SHIFT);

    /* Get the AHB, APB1 and APB2 clock divisor values. */
    ahbdiv  = AHBPRE_Table[(UINT16)((cfgr & CPU_RCC_CFGR_HPRE_MASK) >> CPU_RCC_CFGR_HPRE_SHIFT)];
    apb1div = APBPRE_Table[(UINT16)((cfgr & CPU_RCC_CFGR_PPRE1_MASK) >> CPU_RCC_CFGR_PPRE1_SHIFT)];
    apb2div = APBPRE_Table[(UINT16)((cfgr & CPU_RCC_CFGR_PPRE2_MASK) >> CPU_RCC_CFGR_PPRE2_SHIFT)];

    /* Save the startup clock setting */
    CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_clock = source_clock;

    /* Save the startup AHB divisor value. */
    CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_ahb_divisor = ahbdiv;

    /* Save the startup APB1 divisor value. */
    CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_apb1_divisor = apb1div;

    /* Save the startup APB2 divisor value. */
    CPU_OP_Set[CPU_STARTUP_OP_INDEX].pm_apb2_divisor = apb2div;

    /* Update the frequency values in the startup OP control block. */
    CPU_Tgt_Set_OP_Frequency(&CPU_OP_Set[CPU_STARTUP_OP_INDEX], source_clock);

    /* Update each OP with the frequency that would be used based on the oscillator. */
    for(op_index = 0; op_index < CPU_OP_Count; op_index++)
    {
        /* Update the OP frequency values. */
        CPU_Tgt_Set_OP_Frequency(&CPU_OP_Set[op_index], source_clock);

        /* Check if this is the current OP. */
        if ((source_clock == CPU_OP_Set[op_index].pm_clock) &&
            (ahbdiv == CPU_OP_Set[op_index].pm_ahb_divisor) &&
            (apb1div == CPU_OP_Set[op_index].pm_apb1_divisor) &&
            (apb2div == CPU_OP_Set[op_index].pm_apb2_divisor))
        {
            /* Set the op index. */
            CPU_Current_OP = op_index;
        }
    }

    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Tgt_Set_OP_Frequency
*
*   DESCRIPTION
*
*       Calculates the frequency of the passed in OP.
*
*   INPUT
*
*       op_ptr                              Operating point to be updated
*       main_osc                            Main oscillator freq. 
*
*   OUTPUT
*
*       None
*
*************************************************************************/
static VOID CPU_Tgt_Set_OP_Frequency(CPU_DVFS_OP *op_ptr, UINT32 main_osc)
{
    /* Based on the clock each frequency calculation is different */

    switch (op_ptr->pm_clock)
    {
        case CPU_HSI:
            op_ptr->pm_ahb_frequency  = CPU_HSI_CYCLES / op_ptr->pm_ahb_divisor;
            break;

        case CPU_HSE:
            op_ptr->pm_ahb_frequency  = CPU_HSE_CYCLES / op_ptr->pm_ahb_divisor;
            break;

        case CPU_PLL:
            op_ptr->pm_ahb_frequency  = CPU_PLL_CYCLES / op_ptr->pm_ahb_divisor;
            break;

        default:
            break;
    }

    op_ptr->pm_apb1_frequency = op_ptr->pm_ahb_frequency / op_ptr->pm_apb1_divisor;
    op_ptr->pm_apb2_frequency = op_ptr->pm_ahb_frequency / op_ptr->pm_apb2_divisor;
}


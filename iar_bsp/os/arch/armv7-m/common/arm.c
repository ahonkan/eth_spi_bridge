/***********************************************************************
*
*             Copyright 2010 Mentor Graphics Corporation
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
*       arm.c
*
*   DESCRIPTION
*
*       This file contains the core ARM architecture functions
*
*   FUNCTIONS
*
*
*
*   DEPENDENCIES
*
*       nucleus.h
*
***********************************************************************/

/* Include required header files */
#include            "nucleus.h"

/* External variable declarations */
extern  VOID *                          ESAL_AR_ISR_Vector_Table;

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_STK_Unsolicited_Set
*
*   DESCRIPTION
*
*       This function populates a stack frame as required by the given
*       architecture (contains all registers that must be preserved
*       across an interrupt context switch).
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       start_addr                          Start address of stack mem
*       end_addr                            End address of stack mem
*       entry_function                      Address of execution entry
*
*   OUTPUTS
*
*       Stack Frame Stack Pointer
*
***********************************************************************/
VOID    *ESAL_AR_STK_Unsolicited_Set(VOID *start_addr, VOID *end_addr,
                                     VOID (*entry_function)(VOID))
{
    NU_REG    ESAL_AR_STK     *stack_frame;


    /* Ensure unused parameter doesn't cause tool warnings */
    NU_UNUSED_PARAM(start_addr);

    /* Set address of interrupt stack frame */
    stack_frame = (ESAL_AR_STK *)((VOID_CAST)end_addr - ESAL_GE_STK_MAX_FRAME_SIZE);

    /* Set stack type */
    stack_frame -> stack_type = ESAL_GE_STK_AR_TYPE;

    /* Set xSPR */
    (stack_frame -> min_stack).xpsr = ESAL_AR_STK_XPSR_INIT;

    /* Set program counter entry point */
    (stack_frame -> min_stack).rtn_address = (UINT32)entry_function;

    /* Zeroize lr within stack frame (allows nicer debugging environment - call stack terminated) */
    (stack_frame -> min_stack).lr = 0x00000000;

#if (ESAL_TS_PIC_PID_SUPPORT == NU_TRUE)

    /* Ensure PIC / PID base address is set in stack frame */
    ESAL_TS_RTE_PIC_PID_BASE_GET(&(stack_frame -> r9), NU_NULL);

#endif  /* ESAL_TS_PIC_PID_SUPPORT == NU_TRUE */

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    /* Set FPSCR to 0 */
    stack_frame -> fpscr = 0;

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    /* Return stack pointer to caller */
    return ((VOID *)stack_frame);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_TS_STK_Solicited_Set
*
*   DESCRIPTION
*
*       This function populates a solicited stack frame as required
*       by the toolset for this architecture.  A solicited stack
*       frame is one that contains all the registers required by
*       the given toolset to be preserved across a function call
*       boundary (i.e. registers that must be unchanged after making
*       a function call within C code).
*
*   CALLED BY
*
*       Operating System Services
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       start_addr                          Start address of stack mem
*       end_addr                            End address of stack mem
*       entry_function                      Address of exec. entry
*
*   OUTPUTS
*
*       Stack Frame Stack Pointer
*
***********************************************************************/
VOID    *ESAL_TS_STK_Solicited_Set(VOID *start_addr, VOID *end_addr,
                                   VOID (*entry_function)(VOID))
{
    NU_REG ESAL_TS_STK     *stack_frame;


    /* Ensure unused parameter doesn't cause tool warnings */
    NU_UNUSED_PARAM(start_addr);

    /* Set address of stack frame */
    stack_frame = (ESAL_TS_STK *)((UINT32)end_addr - ESAL_GE_STK_MIN_FRAME_SIZE);

    /* Set stack type within stack frame */
    stack_frame -> stack_type = ESAL_GE_STK_TS_TYPE;

    /* Set return address / entry point within stack frame */
    stack_frame -> rtn_address = (UINT32)entry_function;

#if (ESAL_TS_PIC_PID_SUPPORT == NU_TRUE)

    /* Ensure PIC / PID base address is set in stack frame */
    ESAL_TS_RTE_PIC_PID_BASE_GET(&(stack_frame -> r9), NU_NULL);

#endif  /* ESAL_TS_PIC_PID_SUPPORT == NU_TRUE */

#if (CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0)

    /* Initialize FPSCR to enable 'NaN' and 'Flush-to-Zero' in FPU controller. */
    stack_frame -> fpscr = 0x03000000;

#endif  /* CFG_NU_OS_ARCH_ARMV7_M_COM_FPU_SUPPORT > 0 */

    /* Return stack pointer to caller */
    return ((VOID *)stack_frame);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_TS_MEM_First_Avail_Get
*
*   DESCRIPTION
*
*       This function returns the first available RAM memory address
*       available for use.  This value is usually obtained from
*       linker produced addresses and labels and it is a static value
*       (i.e. it won't be updated at run-time to reflect use of available
*       memory - it only represents the first available memory
*       address that existed at link time)
*
*   CALLED BY
*
*       ESAL_GE_MEM_Initialize
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
*       VOID *                              Pointer to first available
*                                           RAM memory usable
*
***********************************************************************/
VOID    *ESAL_TS_MEM_First_Avail_Get(VOID)
{
    /* Return the address of the first memory available for use
       by an application. */
    return (TOOLSET_BSS_END_ADDR);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_TS_MEM_BSS_Clear
*
*   DESCRIPTION
*
*       This function clears the BSS for the given toolset
*
*   CALLED BY
*
*       ESAL_GE_RTE_Initialize
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
VOID    ESAL_TS_MEM_BSS_Clear(VOID)
{
    /* Clear all BSS / Zero-initialized memory */
    ESAL_GE_MEM_Clear(TOOLSET_BSS_START_ADDR, TOOLSET_BSS_SIZE);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_TS_MEM_ROM_To_RAM_Copy
*
*   DESCRIPTION
*
*       This function copies initialized data from ROM to RAM for the
*       given toolset
*
*   CALLED BY
*
*       ESAL_GE_RTE_Initialize
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
VOID    ESAL_TS_MEM_ROM_To_RAM_Copy(VOID)
{
#if (( CFG_NU_OS_KERN_PLUS_CORE_ROM_SUPPORT == NU_TRUE ) || ( ESAL_TS_ROM_TO_RAM_COPY_SUPPORT == NU_TRUE ))

    /* Check destination address (RAM data start) is not equal to
       source address (ROM data start). */
    if (TOOLSET_DATA_DST_ADDR != TOOLSET_DATA_SRC_ADDR)
    {
        /* Copy initialized data from ROM to RAM */
        ESAL_GE_MEM_Copy(TOOLSET_DATA_DST_ADDR, TOOLSET_DATA_SRC_ADDR, TOOLSET_DATA_SIZE);
    }

#endif
}

#if (ESAL_AR_ISR_INIT_REQUIRED == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_ISR_Initialize
*
*   DESCRIPTION
*
*       This function performs any interrupt servicing related
*       initialization  necessary for the given architecture.
*       This includes setting-up interrupt related stacks, initializing
*       data used during interrupt handling, etc
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
VOID    ESAL_AR_ISR_Initialize(VOID)
{
    /* Initialize the interrupts to a known state */
    ESAL_GE_MEM_WRITE32(ESAL_AR_INT_NVIC_BASE_ADDR + ESAL_AR_ISR_NVIC_AIRCR_OFFSET,
                        ESAL_AR_ISR_NVIC_AIRCR_INIT);

    /* Enable usage, bus, and memory faults */
    ESAL_GE_MEM_WRITE32(ESAL_AR_INT_SHCSR_ADDR,(ESAL_AR_INT_SHCSR_USGFAULTENA |
                                                ESAL_AR_INT_SHCSR_BUSFAULTENA |
                                                ESAL_AR_INT_SHCSR_MEMFAULTENA));
}

#endif  /* ESAL_AR_ISR_INIT_REQUIRED == NU_TRUE */


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_ISR_Vector_Table_Install
*
*   DESCRIPTION
*
*       This function installs the vector table as required for the
*       given architecture
*
*   CALLED BY
*
*       ESAL_GE_ISR_Initialize
*
*   CALLS
*
*       ESAL_GE_MEM_Copy
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
VOID    ESAL_AR_ISR_Vector_Table_Install(VOID)
{
	/* Update vector table address in NVIC. */
	ESAL_GE_MEM_WRITE32(ESAL_AR_INT_NVIC_BASE_ADDR + ESAL_AR_ISR_NVIC_VECTTBL_OFFSET,
				   	    (UINT32)(&ESAL_AR_ISR_Vector_Table));
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_INT_Enable
*
*   DESCRIPTION
*
*       This function enables the interrupt source on the architecture
*       that is associated with the specified vector ID
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
*       vector_id                           vector ID of interrupt
*                                           source being enabled
*       trigger_type                        Trigger method for given
*                                           interrupt vector ID
*       priority                            Priority for the given
*                                           interrupt vector ID
*
*   OUTPUTS
*
*       INT                                 vector_id if passed
*                                           ESAL_DP_INT_VECTOR_ID_DELIMITER
*                                           if failed
*
***********************************************************************/
INT     ESAL_AR_INT_Enable(INT                     vector_id,
                           ESAL_GE_INT_TRIG_TYPE   trigger_type,
                           INT                     priority)
{
    /* Return the vector ID */
    return (vector_id);
}


/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_INT_Disable
*
*   DESCRIPTION
*
*       This function disables the interrupt source on the architecture
*       that is associated with the specified vector ID
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
*                                           ESAL_DP_INT_VECTOR_ID_DELIMITER
*                                           if failed
*
***********************************************************************/
INT     ESAL_AR_INT_Disable(INT vector_id)
{
    /* Return the vector ID */
    return (vector_id);
}



#if (ESAL_AR_OS_TIMER_USED == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       ESAL_AR_TMR_OS_Timer_Start
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
VOID ESAL_AR_TMR_OS_Timer_Start(UINT32 ticks_per_sec)
{
    UINT32  timer_count;
    UINT32  temp32;


    /* Calculate count of timer to get required timer ticks per second */
    timer_count = ESAL_GE_TMR_COUNT_CALC(ESAL_PR_TMR_OS_CLOCK_RATE,
                                         ESAL_PR_TMR_OS_CLOCK_PRESCALE,
                                         ticks_per_sec);

    /* Set the timer interrupt priority (read-modify-write priority register value) */
    temp32 = ESAL_GE_MEM_READ32(ESAL_AR_TMR_SYS_HANDLER_PRI);
    temp32 |= (ESAL_AR_TMR_SYSTICK_PRIORITY << ESAL_AR_TMR_SYS_HANDLER_PRI_SHIFT);
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYS_HANDLER_PRI, temp32);

    /* Load timer count value */
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_RELOAD, timer_count);

    /* Enable the timer and the timer interrupt */
    ESAL_GE_MEM_WRITE32(ESAL_AR_TMR_SYSTICK_CTRL,
                        (ESAL_AR_TMR_SYSTICK_CTRL_ENABLE_BIT |
                         ESAL_AR_TMR_SYSTICK_CTRL_TICKINT_BIT |
                         ESAL_AR_TMR_SYSTICK_CTRL_CLKSRC_BIT));
}

#endif  /* ESAL_AR_OS_TIMER_USED == NU_TRUE */


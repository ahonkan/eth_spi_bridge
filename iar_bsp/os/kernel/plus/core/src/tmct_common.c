/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
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
*       tmct_common.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains timer management core functions with target
*       dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TMCT_Adjust_Timer                   Adjust count-down timer
*       TMCT_Enable_Timer                   Enable count-down timer
*       NU_Set_Clock                        Set system clock
*       [NU_Retrieve_Clock]                 Retrieve system clock
*       NU_Set_Clock64                      Set system clock offset
*       NU_Retrieve_Clock64                 Retrieve system clock 64-bit
*       [TMCT_Increment_Clock]              Increment system clock
*       [TMCT_Read_Timer]                   Read count-down timer
*       [TMCT_Retrieve_TS_Task]             Retrieve time-sliced task
*                                           ptr
*       TMCT_Timer_Interrupt                Timer interrupt handler
*       NU_Ticks_To_Time                    Convert 64-bit tick value to
*                                           calendar time
*       NU_Time_To_Ticks                    Convert calendar time to a
*                                           64-bit value
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       timer.h                             Timer functions
*       [power_core.h]                      PMS
*       [idle_scheduler.h]                  CPU Utilization
*
***********************************************************************/
/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
#include        "services/power_core.h"
#include        "os/services/power/core/inc/idle_scheduler.h"
#endif

#if (NU_32BIT_ACCESS == 1)

/* Macro definitions for architectures supporting single instruction
   access to 32-bit values */
#define         TMCT_Increment_Clock()          TMD_System_Clock++

#else

/* Define function prototypes */
static VOID     TMCT_Increment_Clock(VOID);

#endif  /* NU_32BIT_ACCESS == 1 */

/* Define external inner-component global data references */
extern TC_HCB               TMD_HISR;
extern UINT64               TMD_System_Clock_Offset;
extern UINT64               TMD_Last_Time_Stamp;

/***********************************************************************
*
*   FUNCTION
*
*       TMCT_Adjust_Timer
*
*   DESCRIPTION
*
*       This function adjusts the count-down timer with the specified
*       value- if the new value is less than the current.
*
*   CALLED BY
*
*       TMC_Start_Timer                     Start timer function
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       time                                New count-down time
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMCT_Adjust_Timer(R1 UNSIGNED time)
{
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* See if new time value is less than current time value.  */
    if (time < TMD_Timer)
    {
        /* Adjust timer.  */
        TMD_Timer = time;
    }

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();
}


/***********************************************************************
*
*   FUNCTION
*
*       TMCT_Enable_Timer
*
*   DESCRIPTION
*
*       This function enables the count-down timer with the specified
*       value.
*
*   CALLED BY
*
*       TMC_Start_Timer                     Start timer function
*       TMC_Timer_Expiration                Timer expiration task
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       time                                New count-down time
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMCT_Enable_Timer(R1 UNSIGNED time)
{
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Place the new time value into the count-down timer */
    TMD_Timer = time;

    /* Indicate that the timer is active */
    TMD_Timer_State = TM_ACTIVE;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Set_Clock
*
*   DESCRIPTION
*
*       This function sets the system clock to the specified value.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       new_value                           New value for the clock
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_Set_Clock(R1 UNSIGNED new_value)
{
    ESAL_GE_INT_CONTROL_VARS
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Set the system clock to the specified value */
    TMD_System_Clock = new_value;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return to user mode */
    NU_USER_MODE();
}

#if ((NU_32BIT_ACCESS > 1) || (NU_PLUS_INLINING == 0) || defined(CFG_NU_OS_KERN_PROCESS_ENABLE))
/***********************************************************************
*
*   FUNCTION
*
*       NU_Retrieve_Clock
*
*   DESCRIPTION
*
*       This function returns the current value of the system clock.
*
*   CALLED BY
*
*       Application
*
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
*       TMD_System_Clock                    Value of system clock
*
***********************************************************************/
UNSIGNED NU_Retrieve_Clock(VOID)
{
    UNSIGNED    clock_value;
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Get system clock value */
    clock_value = TMD_System_Clock;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return the current value of the system clock */
    return (clock_value);
}

#endif  /* ((NU_32BIT_ACCESS > 1) || (NU_PLUS_INLINING == 0) || defined(CFG_NU_OS_KERN_PROCESS_ENABLE)) */

#if (NU_32BIT_ACCESS > 1)
/***********************************************************************
*
*   FUNCTION
*
*       TMCT_Increment_Clock
*
*   DESCRIPTION
*
*       This function increments the system clock by one
*
*   CALLED BY
*
*       TMCT_Timer_Interrupt
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
static VOID  TMCT_Increment_Clock(VOID)
{
    ESAL_GE_INT_CONTROL_VARS

    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Increment the system clock */
    TMD_System_Clock++;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();
}


/***********************************************************************
*
*   FUNCTION
*
*       TMCT_Read_Timer
*
*   DESCRIPTION
*
*       This function returns the current value of the count-down timer.
*
*   CALLED BY
*
*       TMC_Start_Timer                     Start timer function
*       NU_Get_Remaining_Time               Returns the remaining time
*
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
*       TMD_Timer                           Value of count-down timer
*
***********************************************************************/
UNSIGNED  TMCT_Read_Timer(VOID)
{
    UNSIGNED    timer_value;
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Get value of count-down timer */
    timer_value = TMD_Timer;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return the current value of the count-down timer */
    return (timer_value);
}

#endif  /* NU_32BIT_ACCESS > 1 */


/***********************************************************************
*
*   FUNCTION
*
*       NU_Set_Clock64
*
*   DESCRIPTION
*
*       This function sets both the TMD_System_Clock_Offset calculated
*       from the specified value.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       new_value                           New value for the clock
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_Set_Clock64(UINT64 new_value)
{
    ESAL_GE_INT_CONTROL_VARS
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE_ISR();

    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Set the TMD_System_Clock_Offset */
    TMD_System_Clock_Offset = new_value - (((UINT64)TMD_System_Clock_Upper<<32) + (UINT64)TMD_System_Clock);

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return to user mode */
    NU_USER_MODE_ISR();
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Retrieve_Clock64
*
*   DESCRIPTION
*
*       This function returns the current value of the system clock.
*
*   CALLED BY
*
*       Application
*
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
*       clock_value                         Value of system clock
*
***********************************************************************/
UINT64 NU_Retrieve_Clock64(VOID)
{
    UINT64    clock_value;
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Get system clock value */
    clock_value = ((UINT64)TMD_System_Clock_Upper<<32) + (UINT64)TMD_System_Clock + TMD_System_Clock_Offset;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return the current value of the system clock */
    return (clock_value);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Ticks_To_Time
*
*   DESCRIPTION
*
*       This function converts a Nucleus 64-bit tick count to a time_t
*       "calendar time" value.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       ticks                               64-bit value of system clock
*
*   OUTPUTS
*
*       cal_time                            Calendar time of time_t type
*
***********************************************************************/
time_t NU_Ticks_To_Time(UINT64 ticks)
{
    time_t cal_time = 0;

    /* Make sure input is non-zero for division */
    if (ticks != 0)
    {
        /* Type time_t is "long int" which is a 32-bit value.  Therefore only
           the LSB of ticks/NU_PLUS_TICKS_PER_SEC is returned. */
        cal_time =  (time_t)(ticks / NU_PLUS_TICKS_PER_SEC);
    }

    return (cal_time);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Time_To_Ticks
*
*   DESCRIPTION
*
*       This function converts a calendar time to a 64-bit tick value.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       cal_time                            Calendar time of time_t type
*
*   OUTPUTS
*
*       ticks                               64-bit value of system clock
*
***********************************************************************/
UINT64 NU_Time_To_Ticks(time_t cal_time)
{
    UINT64 ticks = (UINT64)cal_time * NU_PLUS_TICKS_PER_SEC;

    return (ticks);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Get_Time_Stamp
*
*   DESCRIPTION
*
*       Calculate and return trace time stamp.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       time                                - Time stamp value
*
***********************************************************************/
UINT64 NU_Get_Time_Stamp(VOID)
{
    static UINT32     TMD_Last_OS_Tick = 0;
    static UINT32     TMD_Last_HW_Tick = 0;
    UINT64            os_tick_count;
    UINT32            hw_tick_count;
    UINT64            time_stamp;
    ESAL_GE_INT_CONTROL_VARS

    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Retrieve system clock */
    os_tick_count = NU_Retrieve_Clock64();

    /* Retrieve current hardware clock */
    NU_Retrieve_Hardware_Clock(hw_tick_count);

    /* Compensate for scenario where two times-stamp calls occur in quick succession
     * without progression of HW timer count. */
    if((TMD_Last_OS_Tick == os_tick_count) && (TMD_Last_HW_Tick == hw_tick_count))
    {
        time_stamp = TMD_Last_Time_Stamp + 1;
    }
    else
    {
        /* Calculate time stamp based on count up timer or count down timer */
#ifdef NU_COUNT_DOWN

        time_stamp = (os_tick_count * NU_HW_Ticks_Per_SW_Tick) +
                     (NU_HW_Ticks_Per_SW_Tick - hw_tick_count);

#else

        time_stamp = (os_tick_count * NU_HW_Ticks_Per_SW_Tick) + hw_tick_count;

#endif /* NU_COUNT_DOWN */

        /* If current time stamp is less than the previous a timer overflow
         * has occurred (this can be caused due to interrupts being locked
         * out by the application for a time period more than OS tick
         * time period -  This scenario can occur if the time-stamp API is
         * called within this very large interrupt lock-out section in the
         * application). We try to minimize error by adding NU_HW_Ticks_Per_SW_Tick
         * to the time stamp  - this ensures the current time stamp will be greater
         * than the last time stamp taken though it will not necessarily be an
         * accurate time-stamp due to the indeterminate nature of this scenario.*/
        if(time_stamp <= TMD_Last_Time_Stamp)
        {
            time_stamp += NU_HW_Ticks_Per_SW_Tick;
        }
    }

    /* Update globals that track previous states */
    TMD_Last_OS_Tick = os_tick_count;
    TMD_Last_HW_Tick = hw_tick_count;
    TMD_Last_Time_Stamp = time_stamp;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return the time stamp */
    return (time_stamp);
}


#if (NU_PTR_ACCESS > 1)
/***********************************************************************
*
*   FUNCTION
*
*       TMCT_Retrieve_TS_Task
*
*   DESCRIPTION
*
*       This function returns the time-sliced task pointer.
*
*   CALLED BY
*
*       TMC_Timer_HISR                      Timer HISR
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
*       TMD_Time_Slice_Task                 Time sliced task pointer
*
***********************************************************************/
NU_TASK  *TMCT_Retrieve_TS_Task(VOID)
{
    NU_TASK     *slice_task;
    ESAL_GE_INT_CONTROL_VARS


    /* Disable interrupts during critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* Get time-sliced task pointer */
    slice_task = (NU_TASK *) TMD_Time_Slice_Task;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();

    /* Return time-sliced task pointer */
    return (slice_task);
}

#endif  /* NU_PTR_ACCESS > 1 */


/***********************************************************************
*
*   FUNCTION
*
*       TMCT_Timer_Interrupt
*
*   DESCRIPTION
*
*       This is the interrupt service routine for the Nucleus OS timer
*       interrupt
*
*   CALLED BY
*
*       ESAL OS Timer Interrupt
*
*   CALLS
*
*       NU_Activate_HISR
*
*   INPUTS
*
*       vector                              Vector number of OS timer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    TMCT_Timer_Interrupt(INT vector)
{
    R1  TC_TCB  *current_thread;

    /* Perform OS timer end-of-interrupt
       NOTE:  This resets the OS timer interrupt as required */
    ESAL_GE_TMR_OS_TIMER_EOI(vector);

    T_PC_SAMPLE();

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION== NU_TRUE))

    /* Some cores need tick settings updated,
       for cores not affected nothing happens
       here */
    PMS_UPDATE_TICK();

#endif

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

    /* Keep track of CPU timer counters for total and
       idle time */
    PMS_Update_Cpu_Utilization_Counters(NU_TRUE);

#endif

    /* Increment system clock */
    TMCT_Increment_Clock();

    /* Check for a timer overflow */
    if (NU_Retrieve_Clock() == 0)
    {
        /* An overflow has occurred */
        TMD_System_Clock_Upper++;
    }

    /* Check if application timer is enabled */
    if (TMD_Timer_State == TM_ACTIVE)
    {
        /* Decrement application timer and determine if expired */
        if (--TMD_Timer == 0)
        {
            /* Set timer state to expired */
            TMD_Timer_State = TM_EXPIRED;
        }
    }

    /* Get a pointer to the current thread */
    current_thread = (TC_TCB *)TCCT_Current_Thread();

    /* Check if a thread is currently executing */
    if (current_thread)
    {
        /* Check if thread has time-slicing enabled */
        if (current_thread -> tc_cur_time_slice)
        {
            /* Decrement time-slice count */
            current_thread -> tc_cur_time_slice--;

            /* Check if time-slice has expired */
            if (!(current_thread -> tc_cur_time_slice))
            {
                /* Set time-slice task pointer to the current executing thread */
                TMD_Time_Slice_Task = current_thread;

                /* Give a full time-slice to current thread */
                current_thread -> tc_cur_time_slice = current_thread -> tc_time_slice;
            }
        }
    }

    /* Check if timer HISR needs to be activated */
    if ( (TMD_Timer_State == TM_EXPIRED) || (TMD_Time_Slice_Task) )
    {
        /* Activate the timer HISR */
        NU_Activate_HISR(&TMD_HISR);
    }

    /* Suppress warnings */
    NU_UNUSED_PARAM(vector);
}

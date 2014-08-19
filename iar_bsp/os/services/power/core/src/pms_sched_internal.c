/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       pms_sched_internal.c
*
*   COMPONENT
*
*       SCHED - Idle scheduling component
*       TS    - Tick Suppression internal component
*
*   DESCRIPTION
*
*       The following is the implementation of Power Management scheduling
*       and Tick Suppression Service internal component
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMS_TS_Get_Hw_Counter_Timestamp
*       PMS_SCHED_Get_Elapsed_Time
*       PMS_TS_Adjust_For_Suppressed_Ticks
*       PMS_TS_Suppress_Ticks
*       PMS_TS_Unsuppress_Ticks
*       PMS_CPU_Idle
*       PMS_TS_Get_Suppressed_Ticks_Passed
*       PMS_TS_Wakeup
*       PMS_CPU_Wakeup
*       PMS_Update_Cpu_Utilization_Counters
*       PMS_SCHED_Update_Cpu_Utilization_Counters
*       PMS_SCHED_Update_Sampling_Window
*
*   DEPENDENCIES
*
*       idle_scheduler.h
*       tick_suppression.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/idle_scheduler.h"
#include "os/services/power/core/inc/tick_suppression.h"
#include "os/kernel/plus/core/inc/timer.h"

/* Single tick value, which is by default set to NU_HW_TIMER_TICKS_PER_SW_TICK */
UINT32          PMS_Single_Tick                     = PM_INITIAL_SINGLE_TICK;

/* A tick interval by default is set to SINGLE_TICK */
UINT32          PMS_Tick_Interval                   = PM_INITIAL_SINGLE_TICK;

/* Number of hardware ticks per second */
UINT32          PMS_HW_Timer_Ticks_Per_Sec          = PM_INITIAL_HW_TICK_SEC;

/* Last hardware timer value before going idle. The value will be updated to current value
when the CPU wakes up from idle. */
UINT32          PMS_Last_HW_Timer_Value;            /* Does not care about init value*/

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)))

/* Scaling numerator for CPU Utilization */
UINT32   PMS_Scaling_Numerator                      = 1;

/* Scaling denominator for CPU Utilization */
UINT32   PMS_Scaling_Denominator                    = 1;

/* Last value of tick interval by default is set to SINGLE_TICK */
UINT32   PMS_Last_Tick_Interval                     = PM_INITIAL_SINGLE_TICK;

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))) */

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

/* Global variable to keep track of CPU utilization counters */
PM_CPU_UTILIZATION_COUNTERS                         PM_CPU_Util_Counters;

/* CPU usage if PMF is not being included will be set to PMS_SAMPLE_TIME_NOT_SET */
static UINT8    PMS_Cpu_Usage                       = PMS_SAMPLE_TIME_NOT_SET;

#endif  /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)) */

#if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION == NU_TRUE))

/* Global variable to keep track of CPU condition (idle/wakeup state) */
extern          PM_CPU_SET_CPU_COND                 PM_CPU_Cond_Fn_Ptr;

/* User level API global variable to enable tick suppression */
BOOLEAN         PMS_TS_Enabled_Flag                 = NU_FALSE;

/* Max threshold value check to see if tick needs to be suppressed or not  */
UINT32          PMS_TS_Threshold                    = 1;

#if ((ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN) && !defined(ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE))

/* Reload tick interval flag */
static BOOLEAN  PMS_Reload_Interval                 = NU_FALSE;

#endif

/* Simple check to see if this is a timer interrupt or not */
#define         PMS_IS_TIMER_INTERRUPT(vector_id)   ((vector_id == ESAL_GE_TMR_OS_VECTOR) ? NU_TRUE : NU_FALSE)

/*************************************************************************
*
*   FUNCTION
*
*       PMS_TS_Adjust_For_Suppressed_Ticks
*
*   DESCRIPTION
*
*       This function adjusts the OS timer tick suppression
*
*   INPUT
*
*       suppressed_ticks_passed     - Number of suppressed ticks
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_TS_Adjust_For_Suppressed_Ticks(UINT32 suppressed_ticks_passed)
{
    if(suppressed_ticks_passed >= TMD_Timer)
    {
        /* The code below is a safety recovery - this should never happen
           (and it doesn't in our tests) but, if it does, it compensates
           and ensures continuing operation of the task scheduling*/
        if(TMD_Timer > 1)
        {
            suppressed_ticks_passed = TMD_Timer - 1;
        }
        else
        {
            suppressed_ticks_passed = 0;
        }
    }
    
    /* Update PLUS software clock and timer */
    TMD_Timer -= suppressed_ticks_passed;
    TMD_System_Clock += suppressed_ticks_passed;

}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_TS_Suppress_Ticks
*
*   DESCRIPTION
*
*       This function suppresses the ticks
*
*   INPUT
*
*       ticks_to_suppress     - Number of ticks to suppress
*       single_tick           - HW tick interval value for 1 sw tick interval
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_TS_Suppress_Ticks(UINT32 ticks_to_suppress,UINT32 single_tick)
{

/* This function has a different code for UP and DOWN counters */
#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
    UINT32  current_hw_timer_value;
    UINT32  check_hw_timer_value;
    UINT32  tick_value;
    UINT32  next_tick_value;

    /* PMS_TS_Suppress_Ticks is called when tick interval is
       single_tick and it needs to be increased */

    /* Assumption - we are less than 1 tick away from the tick interrupt
                    meaning to ticks are being suppressed at this point  */

    /* Calculate the next theoretical tick value */
    current_hw_timer_value = ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE();
    tick_value = current_hw_timer_value + (single_tick - (current_hw_timer_value % single_tick));
    next_tick_value = tick_value + (ticks_to_suppress * single_tick);

    /* Check for valid range */
    if ((next_tick_value < tick_value) || (next_tick_value > (single_tick * MAX_TICKS_PER_INTERVAL)))
    {
        /* cap the max interval */
        next_tick_value = (MAX_TICKS_PER_INTERVAL * single_tick);
    }

    if(next_tick_value != PMS_Tick_Interval)
    {
        /* Adjusting from current position */
        PMS_Tick_Interval = next_tick_value;
        ESAL_GE_TMR_PMS_SET_HW_TICK_INTERVAL(PMS_Tick_Interval);
        check_hw_timer_value = ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE();

        if(next_tick_value < PMS_Tick_Interval)
        {
            /* Check if we missed the tick already */
            if(check_hw_timer_value > PMS_Tick_Interval)
            {
                /* If missed it, set it to the very next tick */
                PMS_Tick_Interval += single_tick;
            }
        }
        else
        {
            /* Check if tick happened already and counter was reset */
            if(check_hw_timer_value < current_hw_timer_value)
            {
                /* if so, just set the new tick interval based on counter at reset */
                PMS_Tick_Interval = (ticks_to_suppress + 1) * single_tick;
            }
        }

        ESAL_GE_TMR_PMS_SET_HW_TICK_INTERVAL(PMS_Tick_Interval);
    }

#else /* if ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */

    /* Simply add the suppressed ticks time to the existing counter
     * NOTE: assumption is that hardware counter is <= single_tick
     *       whenever PMS_TS_Suppress_Ticks is called since
     *       PMS_TS_Unsuppress_Ticks is always called on wakeup
     */

#ifdef ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE
    ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE(ticks_to_suppress * single_tick);
#else
    ESAL_GE_TMR_PMS_SET_HW_TICK_VALUE(ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE() + (ticks_to_suppress * single_tick));
#endif /* ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE */

    PMS_Tick_Interval = (ticks_to_suppress + 1) * single_tick;
    /* NOTE: Tick reload value of the HW counter
     * will ALWAYS be equal to single_tick so it never has to be
     * set anywhere */

#endif  /* ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */
    
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_TS_Unsuppress_Ticks
*
*   DESCRIPTION
*
*       This function unsuppressed any remaining suppressed ticks by
*       adjusting the hardware timer interval down to the nearest tick
*
*   INPUT
*
*       current_hw_timer_value  - hardware timer value
*       single_tick             - HW tick interval value for 1 sw tick interval
*
*   OUTPUT
*
*       current_hw_timer_value  - current hardware timer value
*
*************************************************************************/
UINT32 PMS_TS_Unsuppress_Ticks(UINT32 current_hw_timer_value,
                               UINT32 single_tick)
{

/* This function has a different code for UP and DOWN counters */
#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
    UINT32  next_tick_value;
    UINT32  check_hw_timer_value;

    /* Check if already closer that 1 single_tick away from
       an interrupt, no need to adjust */
    if((PMS_Tick_Interval - current_hw_timer_value) > single_tick)
    {
        /* Otherwise set the timer to the next available tick */
        /* NOTE: Assumption here this is run in the LISR so it
           will not get pre-emptied by a timer tick interrupt */
        /* How close to closest suppressed tick */
        next_tick_value = current_hw_timer_value + (single_tick - (current_hw_timer_value%single_tick));

        if(next_tick_value < PMS_Tick_Interval)
        {
            PMS_Tick_Interval = next_tick_value;
            ESAL_GE_TMR_PMS_SET_HW_TICK_INTERVAL(PMS_Tick_Interval);

            /* Check if we missed the tick already */
            check_hw_timer_value = ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE();

            if(check_hw_timer_value > PMS_Tick_Interval)
            {
                /* If missed it, set it to the very next tick */
                PMS_Tick_Interval += single_tick;
                ESAL_GE_TMR_PMS_SET_HW_TICK_INTERVAL(PMS_Tick_Interval);

                /* and adjust the current HwTimer value to count the missed tick  */
                current_hw_timer_value = check_hw_timer_value;
            }
        }
    }
#else /* if ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */
    UINT32  check_hw_timer_value;

    /* Check if already closer that 1 single_tick away from
       an interrupt, no need to adjust */
    if(current_hw_timer_value > single_tick)
    {
        /* Read hardware timer */
        check_hw_timer_value = ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE();

        /* Otherwise set the timer to the next available tick */
        /* NOTE: Assumption here this is run in the LISR so it
           will not get pre-emptied by a timer tick interrupt */
#ifdef ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE
        ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE(-((check_hw_timer_value/single_tick)*single_tick));
#else
        /* Next set of instructions are executed first and Right next
           to each other to minimize tick drift */
        ESAL_GE_TMR_PMS_SET_HW_TICK_VALUE(check_hw_timer_value%single_tick);
        
        /* Force a reload on the next expiration */
        PMS_Reload_Interval = NU_TRUE;
#endif

        /* Restore to a single tick when suppresion is disabled */
        PMS_Tick_Interval = single_tick;

        /* NOTE: the reload value of the HW counter
           will ALWAYS be equal to single_tick so it never has to be
           set anywhere */

        /* check to make sure we didn't just miss a tick, if so, adjust */
        if((check_hw_timer_value/single_tick) != (current_hw_timer_value/single_tick))
        {
            current_hw_timer_value = check_hw_timer_value;
        }
    }
#endif /* ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */

    return(current_hw_timer_value);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_CPU_Idle
*
*   DESCRIPTION
*
*       This function performs the CPU Idle operation
*
*   INPUT
*
*       None.
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID PMS_CPU_Idle(VOID)
{
    UINT32 ticks_to_be_suppressed = 0;
    UINT32 expected_idle_time;
    UINT32 single_tick = PMS_Single_Tick;

    /* Suppress ticks if possible */
    if(PMS_TS_Enabled_Flag)
    {
        /* If the timer is greater than 1 tick, ticks can be suppressed */
        if (TMD_Timer > 1)
        {
            /* If the next timer will go off in less
               time than the largest possible interval
               set the interval to the next timer */
            if (TMD_Timer < MAX_TICKS_PER_INTERVAL)
            {
                ticks_to_be_suppressed = TMD_Timer - 1;
            }
            else
            {
                /* Use the max suppression value */
                ticks_to_be_suppressed = MAX_TICKS_PER_INTERVAL - 1;
            }
        }
        /* If the timer is inactive, ticks can be suppressed */
        else if (TMD_Timer_State == TM_NOT_ACTIVE)
        {
            ticks_to_be_suppressed = MAX_TICKS_PER_INTERVAL - 1;
        }
        
        /* We only enter tick suppression when we are idle longer than the
           threshold value */
        if(ticks_to_be_suppressed >= PMS_TS_Threshold)
        {
            PMS_TS_Suppress_Ticks(ticks_to_be_suppressed,single_tick);
        }
    }

    PMS_Last_HW_Timer_Value = ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE();

#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
    expected_idle_time = (PMS_GET_TICK_INTERVAL() - PMS_Last_HW_Timer_Value) / HW_COUNTER_TICKS_PER_US(single_tick);
#else
    expected_idle_time = PMS_Last_HW_Timer_Value / HW_COUNTER_TICKS_PER_US(single_tick);
#endif /* ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */
    PM_CPU_Cond_Fn_Ptr.cpu_idle_flag = NU_TRUE;
    PM_CPU_Cond_Fn_Ptr.cpu_idle_flagTS = NU_TRUE;

    (*PM_CPU_Cond_Fn_Ptr.cpu_idle_ptr)
    (expected_idle_time,NU_PLUS_WAKEUP_CONSTRAINTS);

}


/*************************************************************************
*
*   FUNCTION
*
*       PMS_TS_Get_Suppressed_Ticks_Passed
*
*   DESCRIPTION
*
*       This function gets the suppressed ticks passed
*
*   INPUT
*
*       last_hw_counter_value   - Last hardware counter value
*       elapsed_time            - Time elapsed or delta time of last and
*                                 new time
*       this_is_a_tick_flag     - Timer interrupt flag
*       single_tick             - HW tick interval value for 1 sw tick interval
*
*   OUTPUT
*
*       ticks_passed            - Number of ticks passed from the last
*                                 timer tick interrupt
*
*************************************************************************/
UINT32 PMS_TS_Get_Suppressed_Ticks_Passed(UINT32 last_hw_counter_value,
                                          UINT32 elapsed_time,
                                          BOOLEAN this_is_a_tick_flag,
                                          UINT32 single_tick)
{
    UINT32 time_to_first_tick;
    UINT32 ticks_passed = 0;

#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
    time_to_first_tick = single_tick-(last_hw_counter_value%single_tick);
#else
    time_to_first_tick = (last_hw_counter_value%single_tick);
#endif

    if(elapsed_time > time_to_first_tick)
    {
        ticks_passed++;
        
        /* Need to subtract it to avoid double counting */
        elapsed_time -= time_to_first_tick;
    }
    ticks_passed += (elapsed_time/single_tick);

    if((this_is_a_tick_flag == NU_TRUE) && (ticks_passed))
    {
        /* If this is an actual tick (vs. other interrupt),
           meaning standard tick processing will occur
           then the current tick is NOT suppressed.
           NOTE:  If this is a tick, elapsed_time MUST be
                  greater than single_tick */
        return(ticks_passed - 1);
    }
    else
    {
        return(ticks_passed);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_TS_Wakeup
*
*   DESCRIPTION
*
*       This function performs any necessary Tick Suppression adjustments
*       when waking up from idle (should only be called if waking up from idle)
*
*   INPUT
*
*       wakeup_from_hw_timer_int_flag       - this is NU_TRUE if this
*                                             wakeup was caused by
*                                             hardware timer interrupt
*                                             (meaning tick processing
*                                             will occur before any other
*                                             wakeup happens)
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID PMS_TS_Wakeup(INT vector_id)
{
    UINT32  current_hw_timer_value;
    UINT32  compensated_hw_timer_value;
    UINT32  tick_interval;
    UINT32  elapsed_time;
    UINT32  suppressed_ticks_passed;
    BOOLEAN wakeup_from_hw_timer_int_flag;
    BOOLEAN hw_counter_was_reset_flag;
    UINT32  single_tick = PMS_Single_Tick;

    /* Check to see if it is a timer interrupt */
    wakeup_from_hw_timer_int_flag = PMS_IS_TIMER_INTERRUPT(vector_id);

    /* Performance optimization: Only need to process if processing a tick interrupt
     * or if coming from idle */
    if( (wakeup_from_hw_timer_int_flag == NU_TRUE) || (PM_CPU_Cond_Fn_Ptr.cpu_idle_flagTS == NU_TRUE) )
    {
        PM_CPU_Cond_Fn_Ptr.cpu_idle_flagTS = NU_FALSE;
        tick_interval            = PMS_Tick_Interval;
        
        /* Save this value for counters and anyone else using PMS_SCHED_Get_Elapsed_Time after TS_Wakeup */
        PMS_Last_Tick_Interval   = tick_interval;

        /* Get current HW timer value but adjusted if tick occurred since the
         * beginning of this interrupt  */
        current_hw_timer_value     = PMS_TS_Get_Hw_Counter_Timestamp(wakeup_from_hw_timer_int_flag, tick_interval);

        /* even if tick suppression enabled, if single ticking then no suppression compensation needed */
        if(PMS_TS_Enabled_Flag && (tick_interval != single_tick))
        {
            hw_counter_was_reset_flag   = wakeup_from_hw_timer_int_flag;
            if(hw_counter_was_reset_flag == NU_FALSE)
            {
                /* Need to check here for a race condition, if timer lapsed and reset
                   since this interrupt happened (if this is not a tick interrupt). */
                
                /* If a race condition does occur the hardware timer should be set back
                   to before hardware reset (tick interrupt should be queued up very soon 
                   as soon as LISR is done) */
#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
                if (PMS_Last_HW_Timer_Value > current_hw_timer_value)
                {
                    current_hw_timer_value = tick_interval - 1;
                }
#else
                if (PMS_Last_HW_Timer_Value < current_hw_timer_value)
                {
                    current_hw_timer_value = 1;
                }
#endif
            }
            
            elapsed_time                = PMS_SCHED_Get_Elapsed_Time(PMS_Last_HW_Timer_Value, current_hw_timer_value, hw_counter_was_reset_flag,tick_interval);
            suppressed_ticks_passed     = PMS_TS_Get_Suppressed_Ticks_Passed(PMS_Last_HW_Timer_Value, elapsed_time, hw_counter_was_reset_flag,single_tick);
            PMS_TS_Adjust_For_Suppressed_Ticks(suppressed_ticks_passed);
        }

        /* Change back to single tick, change current_hw_timer_value if needed to compensate for pending tick */
        compensated_hw_timer_value = PMS_TS_Unsuppress_Ticks(current_hw_timer_value,single_tick);
        if( compensated_hw_timer_value != current_hw_timer_value)
        {
            /* If missed a tick while processing */
            current_hw_timer_value = compensated_hw_timer_value;

            /* adjust for a tick just missed */
            PMS_TS_Adjust_For_Suppressed_Ticks(1);
        }
        
        /* Set the PMS_Last_HW_Timer_Value since anything has been accounted for until this time */
        PMS_Last_HW_Timer_Value = current_hw_timer_value;
    }

}


/*************************************************************************
*
*   FUNCTION
*
*       PMS_CPU_Wakeup
*
*   DESCRIPTION
*
*       This function performs the CPU wakeup
*
*   INPUT
*
*       vector_id               - The vector number (currently not being
*                                 used)
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID PMS_CPU_Wakeup(INT vector_id)
{
    /* Check if the CPU needs to transition from idle to
       wakeup and set the idle/wakeup flag. */
    if(PM_CPU_Cond_Fn_Ptr.cpu_idle_flag == NU_TRUE)
    {
        PM_CPU_Cond_Fn_Ptr.cpu_idle_flag = NU_FALSE;

        /* Reference any unused parameters to ensure no tool set warnings */
        NU_UNUSED_PARAM(vector_id);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

        /* A flag to let the PMS know that the CPU has just wakeup from idle
           so that idle time can be updated properly. It is being used to 
           update the idle counter from the previous idle to the time of the
           wakeup within the scheduler (PMS_Update_Cpu_Utilization_Counters).
           Setting the flag means the CPU has just come back purely from idle
           which is different than PM_CPU_Cond_Fn_Ptr.cpu_idle_flag, which is
           being used to keep track of idle/wakeup state. This flag is 
           important for the case where the CPU never goes to idle scheduler
           (always busy) where the total counter will keep getting updated 
           without idle counter being updated.  */
        PM_CPU_Util_Counters.hw_idle_tmr_flag = NU_TRUE;

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)) */

        (*PM_CPU_Cond_Fn_Ptr.cpu_wakeup_ptr)();
    }
}
#if ((ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN) && !defined(ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE))
/*************************************************************************
*
*   FUNCTION
*
*       PMS_Reset_Interval
*
*   DESCRIPTION
*
*       This function reset the tick value to a single tick interval.
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
VOID PMS_Reset_Interval(VOID)
{
    if (PMS_Reload_Interval == NU_TRUE)
    {
        ESAL_GE_TMR_PMS_SET_HW_TICK_VALUE(PMS_Tick_Interval);
        PMS_Reload_Interval = NU_FALSE;
    }
}
#endif


#endif  /* ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION == NU_TRUE)) */

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Update_Cpu_Utilization_Counters
*
*   DESCRIPTION
*
*       This function updates the CPU utilization counters. This API
*       will be exposed to PLUS generic.
*
*   INPUT
*
*       timer_interrupt     - Hardware timer interrupt flag
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID PMS_Update_Cpu_Utilization_Counters(BOOLEAN timer_interrupt)
{
    PMS_SCHED_Update_Cpu_Utilization_Counters(timer_interrupt,
                                              &PM_CPU_Util_Counters.hw_idle_tmr_flag,
                                              &PM_CPU_Util_Counters.old_hw_tmr_value,
                                              &PM_CPU_Util_Counters.idle_time,
                                              &PM_CPU_Util_Counters.total_time,
                                              &PMS_Cpu_Usage);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SCHED_Update_Cpu_Utilization_Counters
*
*   DESCRIPTION
*
*       This function updates the CPU utilization counters. This function
*       will be internal to PMS.
*
*   INPUT
*
*       timer_interrupt     - Hardware timer interrupt flag
*       hw_idle_tmr_flag    - Idle timer flag
*       old_hw_tmr_value    - Old idle hardware timer flag
*       idle_time           - Hardware counter idle time
*       total_time          - Hardware counter total time
*       current_cpu_usage   - CPU usage percentage for the previous window
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID PMS_SCHED_Update_Cpu_Utilization_Counters(BOOLEAN timer_interrupt,
                                               BOOLEAN *hw_idle_tmr_flag,
                                               UINT32  *old_hw_tmr_value,
                                               UINT32  *idle_time,
                                               UINT32  *total_time,
                                               UINT8   *current_cpu_usage)
{
    UINT32      current_hw_value;
    UINT32      time_elapsed;

    /* The idea is to use four variables to keep track of
       the following the counters: old, current, idle and total */

    /* Get the current PLUS timer. */
    current_hw_value = PMS_TS_Get_Hw_Counter_Timestamp(timer_interrupt, PMS_Last_Tick_Interval);
    
    /* Get elapsed time */
    time_elapsed = PMS_SCHED_Get_Elapsed_Time(*old_hw_tmr_value, current_hw_value,
                                              timer_interrupt, PMS_Last_Tick_Interval);

    /* Keep track of the old counter value. */
    *old_hw_tmr_value = current_hw_value;
    
    /* Scale the value */
    if ((PMS_Scaling_Denominator != 0) && (PMS_Scaling_Numerator != 1))
    {
        time_elapsed = ((PMS_Scaling_Numerator / PMS_Scaling_Denominator) * time_elapsed) +
                        ((time_elapsed / PMS_Scaling_Denominator) * (PMS_Scaling_Numerator % PMS_Scaling_Denominator));
    }
    
    /* A flag is being used to determine that an idle has
       previously occurred. */
    if(*hw_idle_tmr_flag)
    {
        /* Reset the hardware timer interrupt flag */
        *hw_idle_tmr_flag = NU_FALSE;

        /* Increment the idle time */
        *idle_time += time_elapsed;
    }

    /* Add time elapsed to the total time */
    *total_time += time_elapsed;
}

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)) */

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)))

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SCHED_Update_Scaling_Factors
*
*   DESCRIPTION
*
*       This function will update the numerator/denominator used in CPU
*       utilization scaling.
*
*   INPUT
*
*       numerator
*       denominator
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_SCHED_Update_Scaling_Factors(UINT32 numerator, UINT32 denominator)
{
    if (denominator != 0)
    {
        PMS_Scaling_Numerator = numerator;
        PMS_Scaling_Denominator = denominator;
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_TS_Get_Hw_Counter_Timestamp
*
*   DESCRIPTION
*
*       This function returns hardware counter value adjusted if
*       no timer lapse was expected but detected
*
*   INPUT
*
*       hw_counter_was_reset_flag_ptr   - NU_TRUE- wake up from timer
*                                         hardware interrupt.
*                                         NU_FALSE- wake up from other
*                                         hardware interrupt
*       tick_interval                   - Hardware timer tick interval
*
*   OUTPUT
*
*       current_hw_counter_value        - Current hardware counter value
*
*************************************************************************/
UINT32 PMS_TS_Get_Hw_Counter_Timestamp(BOOLEAN hw_counter_was_reset_flag_ptr,
                                       UINT32 tick_interval)
{
    UINT32  current_hw_counter_value;

    /* read current value */
    current_hw_counter_value = ESAL_GE_TMR_PMS_GET_HW_TICK_CNT_VALUE();

    if((hw_counter_was_reset_flag_ptr == NU_FALSE) && (ESAL_GE_TMR_PMS_IS_TIMER_INT_PENDING() == NU_TRUE))
    {
        /* if not expecting a reset counter but the counter reset recently
         * then roll back the counter to account only for time until the reset */
#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
        current_hw_counter_value = tick_interval - 1;
#else
        current_hw_counter_value = 1;
#endif /* ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */

    }

    return(current_hw_counter_value);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_SCHED_Get_Elapsed_Time
*
*   DESCRIPTION
*
*       This function gets the number of elapsed time from the time of
*       the first interrupt until the next interrupt
*
*   INPUT
*
*       last_hw_counter_value       - Previous saved hardware counter value
*       current_hw_counter_value    - Current hardware counter value
*       hw_counter_was_reset_flag   - Hardware counter reset flag
*       timer_interval              - Total tick timer interval
*
*   OUTPUT
*
*       time_elapsed                - Value of time elapsed
*
*************************************************************************/
UINT32 PMS_SCHED_Get_Elapsed_Time(UINT32 last_hw_counter_value,
                                  UINT32 current_hw_counter_value,
                                  BOOLEAN hw_counter_was_reset_flag,
                                  UINT32 timer_interval)
{
    UINT32 time_elapsed;

    if(hw_counter_was_reset_flag == NU_FALSE)
    {
        timer_interval = 0;
    }

#if(ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_UP)
    time_elapsed = (timer_interval + current_hw_counter_value) - last_hw_counter_value;
#else
    time_elapsed = (timer_interval + last_hw_counter_value) - current_hw_counter_value;
#endif /* ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN */

    return(time_elapsed);
}

#endif /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))) */


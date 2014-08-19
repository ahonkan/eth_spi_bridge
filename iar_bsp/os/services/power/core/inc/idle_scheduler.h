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
* FILE NAME
*
*      idle_scheduler.h
*
* COMPONENT
*
*      PMS - Idle Scheduler
*
* DESCRIPTION
*
*      Private interface of Idle Scheduler
*
* DATA STRUCTURES
*
*      PMS_CPU_SAMPLE_ERRNO
*      PM_CPU_SET_CPU_COND
*      PM_CPU_UTILIZATION_COUNTERS
*
* FUNCTIONS
*
*      NU_PM_CPU_Idle
*      NU_PM_CPU_Wakeup
*      NU_PM_CPU_Set_CPU_Idle_Fn 
*      PMS_CPU_Presched_Initialize
*      PMS_CPU_Post_Sched_Initialize
*      PMS_Update_Cpu_Utilization_Counters
*      PMS_CPU_Wakeup
*      PMS_CPU_Idle
*      PMS_Get_Tick_Time
*      PMS_SCHED_Update_Cpu_Utilization_Counters
*      PMS_SCHED_Get_Elapsed_Time 
*      PMS_SCHED_Update_Scaling_Factors 
*
*************************************************************************/

#ifndef PMS_IDLE_SCHEDULER_H
#define PMS_IDLE_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Number of maximum CPU driver */
#define PM_CPU_MAX_CPU_COUNT                        1

#define PM_INITIAL_HW_TICK_SEC                      (ESAL_GE_TMR_OS_CLOCK_RATE /    \
                                                     ESAL_GE_TMR_OS_CLOCK_PRESCALE)

#define PM_INITIAL_SINGLE_TICK                      (PM_INITIAL_HW_TICK_SEC /    \
                                                     NU_PLUS_TICKS_PER_SEC)

/* a single software tick equals to NU_HW_TIMER_TICKS_PER_SW_TICK hardware ticks */
#define SINGLE_TICK                                 NU_HW_TIMER_TICKS_PER_SW_TICK

/* Wakeup constraints for waking up from idle. This is a wakeup
   constraint (in us) which dictates the time the CPU must be able
   to return to current state after it is awakened. This is used 
   to ensure certain interrupt response guarantees. */
#define     NU_PLUS_WAKEUP_CONSTRAINTS              100

/* CPU error number for sampling window */
typedef enum
{
    PMS_NOT_A_FULL_WINDOW   =   255,
    PMS_SAMPLE_TIME_NOT_SET =   254
}PMS_CPU_SAMPLE_ERRNO;

/* CPU condition function pointer for wakeup and idle */
typedef struct PM_CPU_SET_CPU_COND_STRUCT
{
    VOID            (*cpu_wakeup_ptr)(VOID);                        /* Function pointer to CPU wakeup */
    VOID            (*cpu_idle_ptr)(UINT32 expected_idle_time,      /* Function pointer to CPU idle */
                                    UINT32 wakeup_constraint);
    BOOLEAN         cpu_idle_flag;                                  /* CPU Idle flag */
    BOOLEAN         cpu_idle_flagTS;                                /* Same as cpu_idle_flag but
                                                                       for TickSuppression use (reset independently) */
#if     PAD_2
    DATA_ELEMENT    cpu_padding[PAD_2];                             /* Padding */       
#endif /* #endif */
}PM_CPU_SET_CPU_COND;

/* CPU utilization counters data stricture to keep 
   track of the old and current hardware tick 
   for total time and idle time. */
typedef struct PM_CPU_UTILIZATION_COUNTERS_STRUCT
{
    UINT32          idle_time;                              /* Total hardware idle time */
    UINT32          total_time;                             /* Total hardware time */
    UINT32          old_hw_tmr_value;                       /* Old total hardware timer value */
    BOOLEAN         hw_idle_tmr_flag;                       /* Hardware idle timer flag */
#if     PAD_3
    DATA_ELEMENT    util_padding[PAD_3];                    /* Padding */
#endif /* #endif */
}PM_CPU_UTILIZATION_COUNTERS;

VOID        NU_PM_CPU_Idle(UINT32 expected_idle_time,
                           UINT32 wakeup_constraint);
VOID        NU_PM_CPU_Wakeup(VOID);
STATUS      NU_PM_CPU_Set_CPU_Idle_Fn(VOID (*cpu_wakeup_ptr)(VOID), 
                          VOID (*cpu_idle_ptr)
                          (UINT32 expected_idle_time,
                          UINT32 wakeup_constraint));
STATUS      PMS_CPU_Presched_Initialize(NU_MEMORY_POOL* mem_pool);
VOID        PMS_CPU_Post_Sched_Initialize(NU_MEMORY_POOL* mem_pool);

VOID        PMS_Update_Cpu_Utilization_Counters(BOOLEAN timer_interrupt);
VOID        PMS_CPU_Wakeup(INT vector_id);
VOID        PMS_CPU_Idle(VOID);
UINT32      PMS_Get_Tick_Time(VOID);

VOID        PMS_SCHED_Update_Cpu_Utilization_Counters(BOOLEAN timer_interrupt,
                                     BOOLEAN *hw_idle_tmr_flag,
                                     UINT32  *old_hw_tmr_value,
                                     UINT32  *idle_time, 
                                     UINT32  *total_time,
                                     UINT8   *current_cpu_usage);

UINT32      PMS_SCHED_Get_Elapsed_Time(UINT32 last_hw_counter_value, 
                                UINT32 current_hw_counter_value,
                                BOOLEAN hw_counter_was_reset_flag, 
                                UINT32 timer_interval);

#if ((ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN) && !defined(ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE))
VOID PMS_Reset_Interval(VOID);
#define     PMS_UPDATE_TICK()   PMS_Reset_Interval()
#else
#define     PMS_UPDATE_TICK()
#endif	/* ((ESAL_GE_TMR_PMS_COUNT_METHOD == ESAL_COUNT_DOWN) && !defined(ESAL_GE_TMR_PMS_ADJUST_HW_TICK_VALUE)) */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef PMS_IDLE_SCHEDULER_H */



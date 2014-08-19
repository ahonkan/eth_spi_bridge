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
*       pms_sched_set_cpu_idle_fn.c
*
*   COMPONENT
*
*       SCHED - PMS idle scheduling and CPU utilization components
*
*   DESCRIPTION
*
*       The following is the implementation of Power Management Idle 
*       Scheduler service 
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_CPU_Set_CPU_Idle_Fn
*
*   DEPENDENCIES
*
*       idle_scheduler.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/idle_scheduler.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE)

/* Global variables */
PM_CPU_SET_CPU_COND     PM_CPU_Cond_Fn_Ptr = {NU_PM_CPU_Wakeup, NU_PM_CPU_Idle, NU_FALSE, NU_FALSE};


/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_CPU_Set_CPU_Idle_Fn
*
*   DESCRIPTION
*
*       This function registers CPU wake up and idle functions by 
*       replacing the preset default implementation of NU_PM_CPU_Idle()
*       and NU_PM_CPU_Wakeup() functions.
*
*   INPUT
*
*       cpu_wakeup_ptr      - Pointer to the CPU wake up function
*       cpu_idle_ptr        - Pointer to the CPU idle function
*
*   OUTPUT
*
*       NU_SUCCESS          - Function returns success
*
*       PM_INVALID_POINTER  - Invalid pointer error
*
*************************************************************************/
STATUS NU_PM_CPU_Set_CPU_Idle_Fn(VOID(*cpu_wakeup_ptr)(VOID), 
                                 VOID (*cpu_idle_ptr)
                                (UINT32 expected_idle_time,
                                 UINT32 wakeup_constraint))
{
    STATUS status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES 
    
    if((cpu_wakeup_ptr == NU_NULL) || (cpu_idle_ptr == NU_NULL))
    {
        status = PM_INVALID_POINTER;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* keep track of all the function pointers  */
        PM_CPU_Cond_Fn_Ptr.cpu_idle_flag    = NU_FALSE;
        PM_CPU_Cond_Fn_Ptr.cpu_idle_flagTS  = NU_FALSE;
        PM_CPU_Cond_Fn_Ptr.cpu_wakeup_ptr   = cpu_wakeup_ptr;
        PM_CPU_Cond_Fn_Ptr.cpu_idle_ptr     = cpu_idle_ptr;
        
        /* Return to user mode */
        NU_USER_MODE();
    }
    
    return(status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) */



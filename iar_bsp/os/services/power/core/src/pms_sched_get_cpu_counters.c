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
*       pms_sched_get_cpu_counters.c
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
*       NU_PM_Get_CPU_Counters
*
*   DEPENDENCIES
*
*       idle_scheduler.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include 	"os/services/power/core/inc/idle_scheduler.h"

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

extern PM_CPU_UTILIZATION_COUNTERS          PM_CPU_Util_Counters;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_CPU_Counters
*
*   DESCRIPTION
*
*       This function retrieves the free running CPU Utilization Counters.
*       Applications may poll these at desired intervals and calculate CPU 
*       utilization since the last poll 
*       CPU_Idle_Percentage = 100 * (idle_time - last_idle_time)/
*                                   (total_time - last_total_time)
*       CPU_Utilization_Percentage = 100 - CPU_Idle_Percentage;
*       where total_time = *total_time_ptr, idle_time = *idle_time_ptr, 
*       last_idle_time and last_total_time are values obtained on the 
*       previous poll.
*
*   INPUT
*
*       total_time_ptr      - This is a pointer where the TotalTime 
*                             counter value is to be retrieved to
*       idle_time_ptr       - This is a pointer where the Idle 
*                             Time counter value is to be retrieved to
*
*   OUTPUT
*
*       NU_SUCCESS          - This indicates successful retrieval
*
*       PM_INVALID_POINTER  - Invalid pointer error
*
*************************************************************************/
STATUS NU_PM_Get_CPU_Counters(UINT32 *total_time_ptr,UINT32 *idle_time_ptr)
{
    STATUS status = NU_SUCCESS;
    
    /* Check for invalid NULL pointers */
    if((total_time_ptr == NU_NULL) || (idle_time_ptr == NU_NULL))
    {
        status = PM_INVALID_POINTER;
    }
    else
    {
        /* Get the total time and idle time */
        *idle_time_ptr = PM_CPU_Util_Counters.idle_time;
        *total_time_ptr = PM_CPU_Util_Counters.total_time;
    }
    
    return(status);
}

#endif  /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)) */



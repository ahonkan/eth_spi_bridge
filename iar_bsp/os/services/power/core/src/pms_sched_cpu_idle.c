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
*       pms_sched_cpu_idle.c
*
*   COMPONENT
*
*       SCHED – PMS idle scheduling and CPU utilization components
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
*       NU_PM_CPU_Idle
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

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_CPU_Idle
*
*   DESCRIPTION
*
*       This function is called by the task scheduler whenever the 
*       scheduler has no other tasks to be scheduled for a period of time.
*       This file is deliberately left black as the real implementation 
*       is located within CPU driver.
*
*   INPUT
*
*       Expected_idle_time      - This an expected time (in us) the 
*                                 scheduler is not expecting any tasks 
*                                 to be scheduled.
*       wakeup_contraint        - This is a wakeup constraint (in us) which 
*                                 dictates the time the CPU must be able to
*                                 return to current state after it is 
*                                 awakened. This is used to ensure certain 
*                                 interrupt response guarantees.
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID NU_PM_CPU_Idle(UINT32 expected_idle_time,UINT32 wakeup_constraint){}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) */



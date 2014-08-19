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
*       pms_sched_cpu_wakeup.c
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
*       NU_PM_CPU_Wakeup
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
*       NU_PM_CPU_Wakeup
*
*   DESCRIPTION
*
*       This function is executed whenever the CPU wakes up from 
*       executing NU_PM_CPU_Idle().
*       This file is deliberately left black as the real implementation 
*       is located within CPU driver.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None.
*
*************************************************************************/
VOID NU_PM_CPU_Wakeup(VOID){}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) */



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
*       pms_suppress.c
*
*   COMPONENT
*
*       SUPPRESS – PMS Tick Suppression component
*
*   DESCRIPTION
*
*       The following is the implementation of Power Management Tick 
*       Suppression Service
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Start_Tick_Suppress
*       NU_PM_Stop_Tick_Suppress
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       power_core.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "services/nu_trace_os_mark.h"

extern BOOLEAN PMS_TS_Enabled_Flag;
/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Start_Tick_Suppress
*
*   DESCRIPTION
*
*       This function activates the OS timer tick suppression 
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS          – Function returns success
*
*************************************************************************/
STATUS NU_PM_Start_Tick_Suppress (VOID)
{
#if (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION == NU_TRUE)

    NU_SUPERV_USER_VARIABLES 
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Tick suppression enabled flag macro */
    PMS_TS_Enabled_Flag = NU_TRUE;
    
    /* Trace log */
    T_TICK_SUPPRESS(PMS_TS_Enabled_Flag);
    
    /* Return to user mode */
    NU_USER_MODE();

#endif /* #if (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION == NU_TRUE) */

    return(NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Stop_Tick_Suppress
*
*   DESCRIPTION
*
*       This function deactivates the OS timer tick suppression
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       NU_SUCCESS          – Function returns success
*
*************************************************************************/
STATUS NU_PM_Stop_Tick_Suppress (VOID)
{
#if (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION == NU_TRUE)

    NU_SUPERV_USER_VARIABLES 
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Tick suppression enabled flag macro */
    PMS_TS_Enabled_Flag = NU_FALSE;
    
    /* Trace log */
    T_TICK_SUPPRESS(PMS_TS_Enabled_Flag);
        
    /* Return to user mode */
    NU_USER_MODE();

#endif /* #if (CFG_NU_OS_KERN_PLUS_CORE_TICK_SUPPRESSION == NU_TRUE) */

    return(NU_SUCCESS);
}


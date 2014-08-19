/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
*
* FILE NAME
*
*      pms_watchdog_reset_watchdog.c
*
* COMPONENT
*
*      PMS - Watchdog Services
*
* DESCRIPTION
*
*      This file resets a watchdog element.
*
* DATA STRUCTURES
*
*      None
*
* DEPENDENCIES
*
*      power_core.h
*      watchdog.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/watchdog.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

/* Main Task Process Event Group control block */
extern NU_EVENT_GROUP   PM_WD_Process_Event;

/* Circular array that contains watchdog elements */
extern PM_WD *PM_Watchdog_Array[PM_WATCHDOG_ARRAY_SIZE];

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Reset_Watchdog
*
* DESCRIPTION
*
*      This function resets a watchdog timer
*
* INPUTS
*
*      handle               Handle of watchdog to be reset
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_INVALID_WD_HANDLE This error indicates the provided
*                           handle is invalid
*
*************************************************************************/
STATUS NU_PM_Reset_Watchdog(PM_WD_HANDLE handle)
{
    STATUS  pm_status = NU_SUCCESS;
    PM_WD   *temp_handle;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Convert handle into type PM_WD */
    temp_handle = (PM_WD *)handle;
        
    if ((temp_handle == NU_NULL) || (temp_handle->wd_id != PM_WATCHDOG_ID))
    {
        pm_status = PM_INVALID_WD_HANDLE;        
    } 
    else
    {   
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Update the value of latest activity timestamp */
        temp_handle->latest_activity_timestamp = NU_Retrieve_Clock(); 
        
        /* Are we resetting an expired watchdog? */
        if (temp_handle->expired_flag == NU_TRUE)
        {
             /* Main task is not scheduled to wake up, wake it up to process this now active wd */
            (VOID)NU_Set_Events(&PM_WD_Process_Event, (UNSIGNED)PM_WD_WAKEUP_MAIN_TASK, (OPTION)NU_OR); 
        }
        
        /* Return to user mode */
        NU_USER_MODE();
    }
    
    return(pm_status);

}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */



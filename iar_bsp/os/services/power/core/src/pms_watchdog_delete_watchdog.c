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
*      pms_watchdog_delete_watchdog.c
*
* COMPONENT
*
*      PMS - Watchdog Services
*
* DESCRIPTION
*
*      This file deletes a watchdog element.
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
#include    "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

/* EQM Instance. */
extern EQM_EVENT_QUEUE System_Eqm;

/* Main Task Process Event Group control block */
extern NU_EVENT_GROUP   PM_WD_Process_Event;

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Get_Delete_Watchdog
*
* DESCRIPTION
*

*      This function deletes the created watchdog. It also releases any
*      blocked NU_PM_Is_Watchdog_Expired calls for the deleted watchdog
*
* INPUTS
*
*      handle               Watchdog to be deleted
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_INVALID_WD_HANDLE This error indicates the provided
*                           handle is invalid
*      PM_UNEXPECTED_ERROR  This indicates an unexpected error
*                           has occurred (should never happen)
*
*************************************************************************/
STATUS NU_PM_Delete_Watchdog(PM_WD_HANDLE handle)
{
    STATUS              pm_status = NU_SUCCESS; 
    PM_WD               *wd_element;
    STATUS              eqm_status;
    PM_WD_NOTIFICATION  wd_event;
    
    NU_SUPERV_USER_VARIABLES

    wd_element = (PM_WD *)handle;
     
    /* Check input parameters */
    if ((wd_element == NU_NULL) || (wd_element->wd_id != PM_WATCHDOG_ID))
    {
        pm_status = PM_INVALID_WD_HANDLE;
        
        /* Trace log */
        T_WD_DELETE(wd_element, pm_status);
    }    
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Set the timeout of this element to 0 */
        wd_element->timeout = 0;
        
        /* Trace log */
        T_WD_DELETE(wd_element, pm_status);
        
        /* Check if notification are on and delete notifications are set  */
        if (wd_element->is_notifications & PM_DELETED_NOTIFICATIONS)
        {
            /* Assign the watchdog deleted notification to the event type. */
            wd_event.pm_event_type = PM_DELETED_NOTIFICATIONS;

            /* Assign the sender device ID into the watchdog notification structure. */
            wd_event.pm_dev_id = wd_element->notification_dev;
            
            /* Send a notification to listeners now that watchdog is deleted */
            eqm_status = NU_EQM_Post_Event(&System_Eqm, (EQM_EVENT*)(&wd_event),
                                           sizeof(PM_WD_NOTIFICATION), NU_NULL);

            if (eqm_status != NU_SUCCESS)
            {
                pm_status = PM_UNEXPECTED_ERROR;
            }
        }
        
        /* Now, if the main task is not scheduled to wake up, wake it up to clean up */
        (VOID)NU_Set_Events(&PM_WD_Process_Event, (UNSIGNED)PM_WD_WAKEUP_MAIN_TASK, (OPTION)NU_OR); 
        
        /* Return to user mode */
        NU_USER_MODE();
    }
    
    return(pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */



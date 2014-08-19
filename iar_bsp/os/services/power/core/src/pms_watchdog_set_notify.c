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
*      pms_watchdog_set_notify.c
*
* COMPONENT
*
*      PMS - Watchdog Services
*
* DESCRIPTION
*
*      This file sets notifications.
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

/* Circular array that contains watchdog elements */
extern PM_WD *PM_Watchdog_Array[PM_WATCHDOG_ARRAY_SIZE];

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Set_Watchdog_Notify
*
* DESCRIPTION
*
*      This function has been deprecated as the parameter
*      notify_type is no longer used.  This function serves
*      as a temporary bridge to the new API.
*
* INPUTS
*
*      handle               Handle of watchdog 
*      wd_status            Status which should trigger 
*                           the notification send. The only
*                           valid values are:
*                           PM_WD_EXPIRED - notification to 
*                           be sent when watchdog expires
*                           PM_WD_NOT_EXPIRED - notification
*                           to be sent when watchdog becomes 
*                           active
*                           PM_WD_DELETED - notification to 
*                           be sent when watchdog is deleted
*      sender_dev_id        Device id to use as the sender
*                           of the notification
*      notify_type          Notification type to be sent
*
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_INVALID_WD_HANDLE This error indicates the provided
*                           handle is invalid
*      PM_INVALID_WD_STATUS This indicates the status is not
*                           supported
*
*************************************************************************/
STATUS NU_PM_Set_Watchdog_Notify(PM_WD_HANDLE handle, STATUS wd_status,
                                DV_DEV_ID sender_dev_id, UINT32 notify_type)
{
    /* Avoid warnings with unneeded paramter */
    NU_UNUSED_PARAM(notify_type);
    
    /* Call the replacement function */
    return(NU_PM_Set_Watchdog_Notification(handle, wd_status, sender_dev_id));
}

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Set_Watchdog_Notification
*
* DESCRIPTION
*
*      This function sets watchdog notifications
*
* INPUTS
*
*      handle               Handle of watchdog 
*      wd_status            Status which should trigger 
*                           the notification send. The only
*                           valid values are:
*                           PM_WD_EXPIRED - notification to 
*                           be sent when watchdog expires
*                           PM_WD_NOT_EXPIRED - notification
*                           to be sent when watchdog becomes 
*                           active
*                           PM_WD_DELETED - notification to 
*                           be sent when watchdog is deleted
*      sender_dev_id        Device id to use as the sender
*                           of the notification
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_INVALID_WD_HANDLE This error indicates the provided
*                           handle is invalid
*      PM_INVALID_WD_STATUS This indicates the status is not
*                           supported
*
*************************************************************************/
STATUS NU_PM_Set_Watchdog_Notification(PM_WD_HANDLE handle, STATUS wd_status,
                                      DV_DEV_ID sender_dev_id)
{
    STATUS  pm_status = NU_SUCCESS;
    PM_WD   *temp_handle;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Convert handle into type PM_WD */
    temp_handle = (PM_WD *)handle;
    
    /* Check input parameters */    
    if ((temp_handle == NU_NULL) || (temp_handle->wd_id != PM_WATCHDOG_ID))
    {
        pm_status = PM_INVALID_WD_HANDLE;        
    } 
    
    /* Check notification type */
    else if ((wd_status == PM_WD_EXPIRED) || (wd_status == PM_WD_NOT_EXPIRED) ||
             (wd_status == PM_WD_DELETED))
    {    
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Check for expiry notifications */
        if (wd_status == PM_WD_EXPIRED)
        {
            temp_handle->is_notifications |= PM_INACTIVE_NOTIFICATIONS;
        }
        /* Check for active notifications */
        if (wd_status == PM_WD_NOT_EXPIRED)
        {
            temp_handle->is_notifications |= PM_ACTIVE_NOTIFICATIONS;
        }
        /* Check for deleted notifications */
        if (wd_status == PM_WD_DELETED)
        {
            temp_handle->is_notifications |= PM_DELETED_NOTIFICATIONS;
        }
        
        /* Update sender device ID */
        temp_handle->notification_dev = sender_dev_id;
        
        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        pm_status = PM_INVALID_WD_STATUS;
    }
    
    return(pm_status);

}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */



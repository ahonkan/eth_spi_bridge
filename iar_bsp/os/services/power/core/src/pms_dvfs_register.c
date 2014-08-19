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
*       pms_dvfs_register.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for DVFS registering and notification
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_DVFS_Register
*       NU_PM_DVFS_Update_MPL
*       PMS_DVFS_Calculate_Transition
*       PMS_DVFS_Check_Registry
*
*   DEPENDENCIES
*
*       power_core.h
*       dvfs.h
*       initialization.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/dvfs.h"
#include "os/services/power/core/inc/initialization.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern NU_MEMORY_POOL  System_Memory;
extern CS_NODE        *PM_DVFS_Device_List;
extern NU_PROTECT      PM_DVFS_List_Protect;
extern UINT32          PM_DVFS_Total_Park;
extern UINT32          PM_DVFS_Total_Resume;
extern UINT32          PM_DVFS_Smallest_Duration;
extern UINT32          PM_DVFS_Device_Count;

static STATUS PMS_DVFS_Check_Registry(PM_DVFS_HANDLE dvfs_handle);
VOID PMS_DVFS_Calculate_Transition(VOID);

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_DVFS_Register
*
*   DESCRIPTION
*
*       This function is called by the driver to register for DVFS
*       notification/synchronization
*
*   INPUT
*
*       dvfs_handle           Pointer to where new handle will be written
*       instance_handle       Pointer to the device instance handle 
*       dvfs_notify_cb        Callback function for notification
*
*   OUTPUT
*
*       NU_SUCCESS            Successful registration
*       PM_INVALID_POINTER    The provided pointer is invalid
*       PM_ALREADY_REGISTERED Driver is already registered
*       PM_UNEXPECTED_ERROR   Unexpected error has occurred
*
*************************************************************************/
STATUS NU_PM_DVFS_Register(PM_DVFS_HANDLE *dvfs_handle, VOID *instance_handle, PM_NOTIFY_FUNC dvfs_notify_cb)
{
    STATUS         status;
    PM_DVFS_REG   *reg_entry;
    STATUS         pm_status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Verify parameters */
    if ((dvfs_handle == NU_NULL) || (dvfs_notify_cb == NU_NULL))
    {
        pm_status = PM_INVALID_POINTER;
    }
    else if (PMS_DVFS_Check_Registry(*dvfs_handle) == NU_SUCCESS)
    {
        pm_status = PM_ALREADY_REGISTERED;
    }
    else
    {                
        /* Allocate a registry control block */
        status = NU_Allocate_Memory(&System_Memory, (VOID *)&reg_entry, sizeof(PM_DVFS_REG), NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Clear the allocated memory */
            memset(reg_entry, 0, sizeof(PM_DVFS_REG));

            /* Populate the registry control block */
            reg_entry -> pm_notify_func = dvfs_notify_cb;
            reg_entry -> pm_instance = instance_handle;

            /* Default to notify off */
            reg_entry -> pm_notify = PM_NOTIFY_OFF;

            /* Set initialization value for MPL duration to max.
               Park and resume times have been set to 0 in previously
               called memset */
            reg_entry -> pm_mpl.pm_duration = PM_MAX_DURATION;

            /* Protect the list while adding an item */
            NU_Protect(&PM_DVFS_List_Protect);

            /* Mark ID as valid */
            reg_entry -> pm_id = PM_DVFS_REG_ID;

            /* Set priority to duration */
            reg_entry -> pm_node.cs_priority = reg_entry -> pm_mpl.pm_duration;

            /* Place on the registry list */
            NU_Priority_Place_On_List(&PM_DVFS_Device_List, &(reg_entry -> pm_node));

            /* Increment the number of registered drivers */
            PM_DVFS_Device_Count++;

            /* Remove list protection */
            NU_Unprotect();

            /* Return the registry entry pointer as the handle */
            *dvfs_handle = (PM_DVFS_HANDLE)reg_entry;
        }
        else
        {
            /* Out of memory */
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }
                   
    /* Return to user mode */
    NU_USER_MODE();
    
    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_DVFS_Update_MPL
*
*   DESCRIPTION
*
*       This function is called by the driver to update the Maximum
*       Parking Latencies
*
*   INPUT
*
*       dev_handle            Handle of the device updating
*       mpl                   Structure containing the MPL parameters
*       dvfs_notify           Enables or disables callback during DVFS
*
*   OUTPUT
*
*       NU_SUCCESS            Successful registration
*       PM_INVALID_MPL        Duration is less than park/resume
*       PM_INVALID_POINTER    The provided pointer is invalid
*       PM_INVALID_REG_HANDLE The dvfs_handle is invalid
*       PM_UNEXPECTED_ERROR   Unexpected error has occurred
*
*************************************************************************/
STATUS NU_PM_DVFS_Update_MPL(PM_DVFS_HANDLE dvfs_handle, PM_MPL *mpl, PM_DVFS_NOTIFY dvfs_notify)
{
    STATUS       pm_status = NU_SUCCESS;
    PM_DVFS_REG *reg_entry;
    
    NU_SUPERV_USER_VARIABLES

    /* Error check the MPL pointer */
    if (mpl == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else if ((dvfs_notify != PM_NOTIFY_ON) && (dvfs_notify != PM_NOTIFY_OFF))
    {
        pm_status = PM_UNEXPECTED_ERROR;
    }
    else
    {        
        /* Check for a valid MPL, if the park + resume is greater
           than the duration a transition would never be allowed
           to occur */
        if (((mpl -> pm_park_time + mpl -> pm_resume_time) >= mpl -> pm_duration) && (dvfs_notify == PM_NOTIFY_ON))
        {
            pm_status = PM_INVALID_MPL;
        }
        else
        {
            /* Find the Device */
            reg_entry = (PM_DVFS_REG *)dvfs_handle;

            /* Error check the registry entry */
            if (reg_entry == NU_NULL)
            {
                pm_status = PM_INVALID_REG_HANDLE;
            }
            else if (reg_entry -> pm_id != PM_DVFS_REG_ID)
            {
                pm_status = PM_INVALID_REG_HANDLE;
            }
        }

        /* MPL is valid and registry entry is valid */
        if (pm_status == NU_SUCCESS)
        {                    
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Protect the list while updating an item */
            NU_Protect(&PM_DVFS_List_Protect);
            
            /* Update the MPL */
            reg_entry -> pm_mpl.pm_duration = mpl -> pm_duration;
            reg_entry -> pm_mpl.pm_park_time = mpl -> pm_park_time;
            reg_entry -> pm_mpl.pm_resume_time = mpl -> pm_resume_time;

            /* Save the notification type */
            reg_entry -> pm_notify = dvfs_notify;

            /* Remove the item from the current list */
            NU_Remove_From_List(&PM_DVFS_Device_List, &(reg_entry -> pm_node));

            /* Update the priority field */
            reg_entry -> pm_node.cs_priority = PM_MAX_DURATION - reg_entry -> pm_mpl.pm_duration;

            /* Add back the list */
            NU_Priority_Place_On_List(&PM_DVFS_Device_List, &(reg_entry -> pm_node));
                                    
            /* Calculate if a transition is ever possible and the
               smallest amount of time that must be considered for
               transition time */
            PMS_DVFS_Calculate_Transition();
            
            /* Release protection */
            NU_Unprotect();
            
            /* Return to user mode */
            NU_USER_MODE();
        }
    }

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Check_Registry
*
*   DESCRIPTION
*
*       This function is called to determine if a handle exists in the
*       registry.
*
*   INPUT
*
*       dev_handle          Handle of the device updating
*
*   OUTPUT
*
*       NU_SUCCESS          Successful registration
*       PM_NOT_REGISTERED   Driver is not registered for updates
*       PM_INVALID_POINTER  The provided pointer is invalid
*
*************************************************************************/
static STATUS PMS_DVFS_Check_Registry(PM_DVFS_HANDLE dvfs_handle)
{
    STATUS       pm_status =  NU_SUCCESS;
    BOOLEAN      node_found = NU_FALSE;
    CS_NODE     *node_ptr;

    /* Protect the list while searching */
    NU_Protect(&PM_DVFS_List_Protect);

    /* Error check the reg_ptr */
    if (dvfs_handle == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else if (PM_DVFS_Device_List == NU_NULL)
    {
        /* No devices are registered */
        pm_status = PM_NOT_REGISTERED;
    }
    else
    {
        /* Search the registry for the device */
        node_ptr = PM_DVFS_Device_List;
        while (node_found == NU_FALSE)
        {
            /* Test to see if it is the correct registry entry */
            if (node_ptr == dvfs_handle)
            {
                /* End the loop */
                node_found = NU_TRUE;
            }
            else
            {
                /* Position the node pointer to the next node.  */
                node_ptr = node_ptr -> cs_next;

                /* Determine if the pointer is at the head of the list.  */
                if (node_ptr == PM_DVFS_Device_List)
                {
                    /* The list search is complete.  */
                    node_found = NU_TRUE;

                    /* This device is not registered */
                    pm_status = PM_NOT_REGISTERED;
                }
            }
        }
    }
    
    /* Remove protection */
    NU_Unprotect();

    return (pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Calculate_Transition
*
*   DESCRIPTION
*
*       This function is called to find the smallest number needed to
*       properly calculate if a transition is possible.  The number saved
*       here will be compared against a value generated by the CPU driver
*       to switch operating points.
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
VOID PMS_DVFS_Calculate_Transition(VOID)
{
    INT          list_index;
    CS_NODE     *node_ptr;
    PM_DVFS_REG *reg_entry;
    UINT32       total_duration = 0;
    UINT32       remaining;

    /* Initialize the node list
       NOTE: the lists are mirrors so only need to work
             with one side */
    node_ptr = PM_DVFS_Device_List;

    /* Reset smallest duration to max */
    PM_DVFS_Smallest_Duration = PM_MAX_DURATION;

    /* Loop through the registered devices */
    for (list_index = 0; list_index < PM_DVFS_Device_Count; list_index++)
    {
        /* Reset remaining time */
        remaining = 0;

        /* Read the registry entries */
        reg_entry = (PM_DVFS_REG *)node_ptr;

        /* Only calculate if notification is enabled */
        if (reg_entry -> pm_notify == PM_NOTIFY_ON)
        {
            /* Update the duration with new park/resume times */
            total_duration += reg_entry -> pm_mpl.pm_park_time + reg_entry -> pm_mpl.pm_resume_time;

            /* Calculate the remaining time from duration */
            if (total_duration <= reg_entry -> pm_mpl.pm_duration)
            {
                remaining = (reg_entry -> pm_mpl.pm_duration) - total_duration;
            }

            /* Check to see if the new remaining time is the lowest */
            if (PM_DVFS_Smallest_Duration > remaining)
            {
                PM_DVFS_Smallest_Duration = remaining;
            }
        }

        /* Position the node pointer to the next node */
        node_ptr = node_ptr -> cs_next;
    }
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */




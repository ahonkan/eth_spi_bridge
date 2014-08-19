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
*      pms_watchdog_create_watchdog.c
*
* COMPONENT
*
*      PMS - Watchdog Services
*
* DESCRIPTION
*
*      This file creates a new watchdog element.
*
* DATA STRUCTURES
*
*      None
*
* DEPENDENCIES
*
*      power_core.h
*      watchdog.h
*      nucleus.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/watchdog.h"
#include    "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

extern NU_MEMORY_POOL  System_Memory;

/* Main Task Process Event Group control block */
extern NU_EVENT_GROUP   PM_WD_Process_Event;

/* Circular array that contains watchdog elements */
PM_WD *PM_Watchdog_Array[PM_WATCHDOG_ARRAY_SIZE];

/* Protection structure for the circular array / WD ring */
static NU_PROTECT  PM_Watchdog_Array_Protect;

/* Write index */
UINT32 PM_Watchdog_Array_WIndex = 0;

/* Read Index */
UINT32 PM_Watchdog_Array_RIndex = 0;

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Create_Watchdog
*
* DESCRIPTION
*
*      This function creates a watchdog timer with specified timeout
*      duration
*
* INPUTS
*
*      timeout_value        Desired timeout value in 100’s of ms
*                           (value of 1 = 100ms timeout (or 10 regular PLUS ticks))
*      handle_ptr           Pointer to location of returned watchdog handle
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_INVALID_POINTER   This error indicates the provided
*                           pointer is invalid
*      PM_UNEXPECTED_ERROR  This indicates an unexpected error
*                           has occurred (should never happen)
*
*************************************************************************/
STATUS NU_PM_Create_Watchdog(UINT16 timeout_value, PM_WD_HANDLE *handle_ptr)
{
    STATUS      pm_status = NU_SUCCESS; 
    STATUS      status;
    PM_WD       *wd_element;
    
    NU_SUPERV_USER_VARIABLES

    /* Check input parameters */
    if (handle_ptr == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;     
    }  
    /* Check if the watchdog has been deleted */
    else if (timeout_value == 0)
    {
        pm_status = PM_UNEXPECTED_ERROR;
    }  
    else
    {                       
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Allocate memory for the device structure */
        status = NU_Allocate_Memory(&System_Memory, (VOID*)&wd_element,
                                    sizeof(PM_WD), NU_NO_SUSPEND);
                                    
        if (status == NU_SUCCESS)
        {
            /* Clear the memory we just allocated */
            (VOID)memset((VOID*)wd_element, 0, sizeof(PM_WD));
            
            /* Populate the structure */
            wd_element->wd_id            = PM_WATCHDOG_ID;
            wd_element->timeout          = timeout_value;  
            wd_element->is_notifications = PM_NO_NOTIFICATIONS;  
            wd_element->is_rollover      = NU_FALSE;  
            
            /* Ensure we reset the watchdog at least once */
            pm_status = NU_PM_Reset_Watchdog((PM_WD_HANDLE *)wd_element);
            
            if (pm_status == NU_SUCCESS)
            {
                /* If the wd element is created in the first tick, initialize 
                   latest activity timestamp to some default value so that 
                   it is processed as a new element in the main task */
                if (wd_element->latest_activity_check == 0)
                {
                    wd_element->latest_activity_check = PM_WD_INACTIVITY_INIT_VAL;
                } 
                
                /* Protect against access to the ring */
                NU_Protect(&PM_Watchdog_Array_Protect);
        
                /* Load the watchdog element ring */
                PM_Watchdog_Array[PM_Watchdog_Array_WIndex] = wd_element;                             
                
                if (PM_Watchdog_Array_WIndex == (PM_WATCHDOG_ARRAY_SIZE - 1))
                {
                    /* Set write index to 0 if we 
                       reached the end of the ring */
                    PM_Watchdog_Array_WIndex = 0;
                }
                else
                {
                    /* Increment the index */
                    PM_Watchdog_Array_WIndex++;
                }
               
                /* Release protection against access to the WD Ring */
                NU_Unprotect();
                
                /* Return a handle to the watchdog element */
                *handle_ptr = (PM_WD_HANDLE *)wd_element;
    
                /* Main task is not scheduled to wake up, wake it up to process this new wd */
                (VOID)NU_Set_Events(&PM_WD_Process_Event, (UNSIGNED)PM_WD_WAKEUP_MAIN_TASK, (OPTION)NU_OR); 
            }
            else
            {
                /* De-allocate memory */
                (VOID)NU_Deallocate_Memory (wd_element);
            }
        }
        else
        {
            pm_status = PM_UNEXPECTED_ERROR;           
        }
                                                
        /* Return to user mode */
        NU_USER_MODE();
    }
    
    if (pm_status == NU_SUCCESS)
    {
        /* Trace log */
        T_WD_CREATE((VOID*)wd_element, pm_status);
    }
    else
    {
        /* Trace log */
        T_WD_CREATE((VOID*)handle_ptr, pm_status);
    }
       
    return(pm_status);

}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */



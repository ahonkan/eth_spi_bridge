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
* FILE NAME
*
*       pms_error.c
*
* COMPONENT
*
*       PMS - Power Error/Exception Handling
*
* DESCRIPTION
*
*       This file contains functions for registering the handler with the
*       system and calling the handler.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_PM_Register_Error_Handler
*       NU_PM_Throw_Error
*       PMS_Error_Entry
*       PMS_Default_Error_Handler
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       power_core.h                         Power Management Services 
*       initialization.h                     PMS Initialization
*       error_management.h                   PMS error management
*       thread_control.h                     Thread Control Services
*       initialization.h                     PLUS initialization
*       error_management.h                   PLUS error management
*       stdio.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/supplement/inc/error_management.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/initialization.h"
#include "os/services/power/core/inc/error_management.h"
#include "os/kernel/plus/core/inc/thread_control.h"      /* Thread control functions  */
#include "os/kernel/plus/core/inc/initialization.h"      /* Initialization functions  */
#include <stdio.h>

/* Global structures used in this file */
extern VOID (*PMS_Error_Handler)(STATUS, VOID *, VOID *, UINT32);
extern NU_PROTECT PMS_Error_Protect;
extern NU_QUEUE PMS_Error_Queue;
extern INT INC_Initialize_State;

/*************************************************************************
*
* FUNCTION
*
*       NU_PM_Register_Error_Handler
*
* DESCRIPTION
*
*       This function registers the desired handler with the
*       system.  If NULL is passed as the new exception the
*       function will not return success.  If NULL is passed
*       as the old exception nothing will be returned.
*
* INPUTS
*
*       error_entry         New error handler
*       old_error           Previous error handler
*
* OUTPUTS
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_POINTER  The provided pointer is invalid
*
*************************************************************************/
STATUS NU_PM_Register_Error_Handler(VOID(*error_entry)(STATUS, VOID *, VOID *, UINT32),
                                       VOID(**old_error)(STATUS, VOID *, VOID *, UINT32))
{
    STATUS pm_status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES

    /* New error pointer must be valid */
    if (error_entry == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else
    {        
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* protect the global error data */
        NU_Protect(&PMS_Error_Protect);

        /* Write original error for return */
        if (old_error != NU_NULL)
        {
            *old_error = PMS_Error_Handler;
        }

        /* Update the new error handler */
        PMS_Error_Handler = error_entry;

        /* Update is done now remove the protection */
        NU_Unprotect();
                
        /* Return to user mode */
        NU_USER_MODE();
    }

    return(pm_status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_PM_Throw_Error
*
* DESCRIPTION
*
*       This function sets up and calls the error handler registered with
*       the system.  If an error is set before the scheduler starts it
*       gets called directly otherwise a message is passed to an error
*       task.
*
* INPUTS
*
*       status              error code
*       info_ptr            additional information on the error
*       length              length of the additional information
*
* OUTPUTS
*
*      None
*
*************************************************************************/
VOID NU_PM_Throw_Error(STATUS pm_status, VOID *info_ptr, UINT32 length)
{
    PM_ERROR  message;
    STATUS    status;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* If the pointer to the exception handler is good
       call it */
    if (PMS_Error_Handler != NU_NULL)
    {
        /* Check to see if the scheduler has started */
        if (INC_Initialize_State != INC_END_INITIALIZE)
        {
            /* Call the error handler directly */
            (*(PMS_Error_Handler)) (pm_status, NU_NULL, info_ptr, length);
        }
        else
        {
            /* Notify the error thread that an error needs to be handled */
            
            /* Read the current thread */
            message.pm_thread = TCCT_Current_Thread();
            
            /* Setup the rest of the message */
            message.pm_status = pm_status;
            message.pm_info = info_ptr;
            message.pm_length = length;
            
            /* Mark message as valid */
            message.pm_error_id = PM_ERROR_ID;
            
            /* Send the message */
            status = NU_Send_To_Queue(&PMS_Error_Queue, (VOID *)&message, 
                                      sizeof(PM_ERROR)/sizeof(UNSIGNED), NU_SUSPEND);
            if (status != NU_SUCCESS)
            {
                /* Can't send the message; give up. */
                ERC_System_Error(NU_UNHANDLED_EXCEPTION);
            }
        }   
    }
    else
    {
        /* There is no handler for this error we cannot continue. */
        ERC_System_Error(NU_UNHANDLED_EXCEPTION);
    }
    
    /* Do not return */
    for (;;){};
    
    /* No need to return to user mode because of the infinite loop.    */
    /* Returning to user mode will cause warnings with some compilers. */
}

/*************************************************************************
*
* FUNCTION
*
*       PMS_Error_Entry
*
* DESCRIPTION
*
*       This function serves as the entry point for the error
*       task.
*
* INPUTS
*
*       mem_pool_ptr
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PMS_Error_Entry(NU_MEMORY_POOL *mem_pool_ptr)
{
    PM_ERROR error_message;
    UNSIGNED size = 0;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Suspend while waiting on error messages */
    while (NU_Receive_From_Queue(&PMS_Error_Queue, (VOID *)&error_message, sizeof(error_message), &size, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Check for a valid error message, valid handler function, and valid message size */
        if ((error_message.pm_error_id == PM_ERROR_ID) && (PMS_Error_Handler != NU_NULL) && (size == sizeof(PM_ERROR)))
        {
            /* Call the error handler */
            (*PMS_Error_Handler)(error_message.pm_status, error_message.pm_thread, 
                                 error_message.pm_info, error_message.pm_length);
        }
    }
    
    /* Return to user mode */
    NU_USER_MODE();
}

/*************************************************************************
*
* FUNCTION
*
*       PMS_Default_Error_Handler
*
* DESCRIPTION
*
*       This function serves as the default error handler.  It attempt to 
*       print the task name that caused a failure and will halt
*       the system.
*
* INPUTS
*
*       pm_status
*       thread
*       info_ptr
*       length
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PMS_Default_Error_Handler(STATUS pm_status, VOID *thread, VOID *info_ptr, UINT32 length)
{
    NU_SUPERV_USER_VARIABLES 
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Determine if the thread pointer is null */
    if (thread != NU_NULL)
    {
        /* Determine if the thread pointer is a task */
        if (((NU_TASK *)thread) -> tc_id == TC_TASK_ID)
        {
            /* The task is in violation and will be terminated */
            (VOID)NU_Terminate_Task((NU_TASK *)thread);
            
             /* Disallow all current tasks in the system from running
                while a message is printed */
            (VOID)NU_Change_Time_Slice(NU_Current_Task_Pointer(), 0);
            (VOID)NU_Change_Priority(NU_Current_Task_Pointer(), 0);
            (VOID)NU_Change_Preemption(NU_NO_PREEMPT);
            
            /* Print to serial port message about the offending thread name */
            printf("\r\nIn default power error handler!\r\n %s Failed!\r\n", ((NU_TASK *)thread) -> tc_name);
            
            /* Wait on the serial output */
            NU_Sleep(1);
            
            /* Stop the system completely */
            (VOID)NU_Control_Interrupts(NU_DISABLE_INTERRUPTS);
        }
    }
    
    /* No need to return to user mode because of the infinite loop.    */
    /* Returning to user mode will cause warnings with some compilers. */
    
    /* Do not return */
    for (;;){};
}


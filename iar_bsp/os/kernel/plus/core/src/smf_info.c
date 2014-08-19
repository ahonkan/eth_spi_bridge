/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       smf_info.c
*
*   COMPONENT
*
*       SM - Semaphore Management
*
*   DESCRIPTION
*
*       This file contains routines to obtain Information facts about
*       the Semaphore Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Semaphore_Information            Retrieve semaphore info
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                           Thread Control functions
*       semaphore.h                           Semaphore functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/semaphore.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Semaphore_Information
*
*   DESCRIPTION
*
*       This function returns information about the specified semaphore.
*       However, if the supplied semaphore pointer is invalid, the
*       function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect semaphore
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       semaphore_ptr                       Pointer to the semaphore
*       name                                Destination for the name
*       current_count                       Destination for semaphore
*                                           instance count
*       suspend_type                        Destination for the type of
*                                           suspension
*       tasks_waiting                       Destination for the tasks
*                                           waiting count
*       first_task                          Destination for the pointer
*                                           to the first task waiting
*
*   OUTPUTS
*
*       completion
*           NU_SUCCESS                      If a valid semaphore pointer
*                                           is supplied
*           NU_INVALID_SEMAPHORE            If semaphore pointer invalid
*
***********************************************************************/
STATUS NU_Semaphore_Information(NU_SEMAPHORE *semaphore_ptr, CHAR *name,
                                UNSIGNED *current_count, OPTION *suspend_type,
                                UNSIGNED *tasks_waiting, NU_TASK **first_task)
{
    SM_SCB          *semaphore;             /* Semaphore control block ptr */
    STATUS          completion;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this semaphore id is valid.  */
    if ((semaphore != NU_NULL) && (semaphore -> sm_id == SM_SEMAPHORE_ID))
    {
        /* Setup protection of the semaphore.  */
        TCCT_Schedule_Lock();

        /* The semaphore pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        completion =  NU_SUCCESS;

        /* Copy the semaphore's name.  */
        strncpy(name, semaphore -> sm_name, NU_MAX_NAME);

        /* Determine the suspension type.  */
        *suspend_type = semaphore -> sm_suspend_type;

        /* Return the current semaphore available instance count.  */
        *current_count =  semaphore -> sm_semaphore_count;

        /* Retrieve the number of tasks waiting and the pointer to the
           first task waiting.  */
        *tasks_waiting =  semaphore -> sm_tasks_waiting;
        if (semaphore -> sm_suspension_list)
        {
            /* There is a task waiting.  */
            *first_task = (NU_TASK *)
                (semaphore -> sm_suspension_list) -> sm_suspended_task;
        }
        else
        {
            /* There are no tasks waiting.  */
            *first_task =  NU_NULL;
        }

        /* Release protection of the semaphore.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the semaphore pointer is invalid.   */
        completion =  NU_INVALID_SEMAPHORE;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return(completion);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Get_Semaphore_Owner
*
*   DESCRIPTION
*
*       This function returns the owner of the  specified semaphore.
*       However, if the supplied semaphore pointer is invalid, the
*       function simply returns an error status.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect semaphore
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       semaphore_ptr                       Pointer to the semaphore
*       task                                Destination for owner
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If a valid semaphore pointer
*                                           is supplied
*           NU_INVALID_SEMAPHORE            If semaphore pointer invalid
*
***********************************************************************/
STATUS NU_Get_Semaphore_Owner (NU_SEMAPHORE *semaphore_ptr, NU_TASK **task)
{
    SM_SCB          *semaphore;             /* Semaphore control block ptr */
    STATUS          status;                 /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if this semaphore id is valid and a PI Semaphore.  */
    if ((semaphore != NU_NULL) && (semaphore -> sm_id == SM_SEMAPHORE_ID) &&
        (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT))
    {
        /* Setup protection of the semaphore.  */
        TCCT_Schedule_Lock();

        /* The semaphore pointer is valid.  Reflect this in the completion
           status and fill in the actual information.  */
        status =  NU_SUCCESS;

        *task = (NU_TASK*) semaphore -> sm_semaphore_owner;

        /* Release protection of the semaphore.  */
        TCCT_Schedule_Unlock();
    }
    else
    {
        /* Indicate that the semaphore pointer is invalid.   */
        status =  NU_INVALID_SEMAPHORE;
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the appropriate completion status.  */
    return (status);
}

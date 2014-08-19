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
*       smc_common.c
*
*   COMPONENT
*
*       SM - Semaphore Management
*
*   DESCRIPTION
*
*       This file contains the core common routines for the Semaphore
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Semaphore                 Create a semaphore
*       NU_Obtain_Semaphore                 Obtain instance of semaphore
*       NU_Release_Semaphore                Release instance of
*                                           semaphore
*       SMC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       common_services.h                   Common service constants
*       semaphore.h                         Semaphore functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "os/kernel/plus/core/inc/semaphore.h"
#include        "services/nu_trace_os_mark.h"
#include        <string.h>

/* Define external inner-component global data references.  */

extern CS_NODE         *SMD_Created_Semaphores_List;
extern UNSIGNED         SMD_Total_Semaphores;

/* Define internal component function prototypes.  */
VOID    SMC_Cleanup(VOID *information);
STATUS  SMC_Free_PI_Semaphore (SM_SCB *pi_semaphore, TC_TCB *task);

/* Internal function prototypes */
static STATUS   smc_release_semaphore(SM_SCB *semaphore, NU_TASK * task);

/* External functions */
extern STATUS TCS_Change_Priority (TC_TCB *task, OPTION new_priority, BOOLEAN app_call);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Semaphore
*
*   DESCRIPTION
*
*       This function creates a semaphore and then places it on the list
*       of created semaphores.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Add node to linked-list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Data structure protect
*       TCCT_Schedule_Unlock                Un-protect data structure
*
*   INPUTS
*
*       semaphore_ptr                       Semaphore control block ptr
*       name                                Semaphore name
*       initial_count                       Initial semaphore instance
*                                           count
*       suspend_type                        Suspension type
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_SEMAPHORE                Semaphore control block ptr
*                                           is NULL
*       NU_INVALID_SUSPEND                  Semaphore suspension is
*                                           invalid
*       NU_INVALID_COUNT                    Semaphore count is not
*                                           valid
*
***********************************************************************/
STATUS NU_Create_Semaphore(NU_SEMAPHORE *semaphore_ptr, CHAR *name,
                           UNSIGNED initial_count, OPTION suspend_type)
{
    R1 SM_SCB       *semaphore;             /* Semaphore control block ptr */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Check for a NULL semaphore pointer or an already created semaphore */
    NU_ERROR_CHECK(((semaphore == NU_NULL) || (semaphore -> sm_id == SM_SEMAPHORE_ID)), status, NU_INVALID_SEMAPHORE);

    /* Verify a valid suspension type */
    NU_ERROR_CHECK(((suspend_type != NU_FIFO) && (suspend_type != NU_PRIORITY) && (suspend_type != NU_PRIORITY_INHERIT)), status, NU_INVALID_SUSPEND);

    /* Verify a valid count for PI Semaphore */
    NU_ERROR_CHECK(((suspend_type == NU_PRIORITY_INHERIT) && (initial_count != 1)), status, NU_INVALID_COUNT);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Move input semaphore pointer into internal pointer.  */
        semaphore =  (SM_SCB *) semaphore_ptr;

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(semaphore, SM_SCB);

        /* Fill in the semaphore name. */
        strncpy(semaphore -> sm_name, name, (NU_MAX_NAME - 1));

        /* Setup the initial semaphore instance count.  */
        semaphore -> sm_semaphore_count =  initial_count;

        /* Setup the semaphore suspension type.  */
        semaphore -> sm_suspend_type = suspend_type;

        /* Protect against access to the list of created semaphores.  */
        TCCT_Schedule_Lock();

        /* At this point the semaphore is completely built.  The ID can now be
           set and it can be linked into the created semaphore list.  */
        semaphore -> sm_id =  SM_SEMAPHORE_ID;

        /* Link the semaphore into the list of created semaphores and increment the
           total number of semaphores in the system.  */
        NU_Place_On_List(&SMD_Created_Semaphores_List,&(semaphore -> sm_created));
        SMD_Total_Semaphores++;

        /* Trace log */
        T_SEM_CREATE((VOID*)semaphore, name, initial_count, suspend_type, OBJ_ACTION_SUCCESS);

        /* Release protection against access to the list of created semaphores.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SEM_CREATE((VOID*)semaphore, name, initial_count, suspend_type, status);
    }

    /* Return successful completion.  */
    return(status);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Obtain_Semaphore
*
*   DESCRIPTION
*
*       This function obtains an instance of the semaphore.  An instance
*       corresponds to decrementing the counter by 1.  If the counter is
*       greater than zero at the time of this call, this function can be
*       completed immediately.  Otherwise, suspension is possible.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Place on suspend list
*       NU_Priority_Place_On_List           Place on priority list
*       TCC_Suspend_Task                    Suspend calling task
*       TCC_Task_Priority                   Obtain task's priority
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Current_Thread                 Pickup current thread
*                                           pointer
*       TCCT_Schedule_Lock                  Protect semaphore
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       semaphore_ptr                       Semaphore control block ptr
*       suspend                             Suspension option if full
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      If service is successful
*           NU_UNAVAILABLE                  If an instance of the
*                                           semaphore is not available
*           NU_TIMEOUT                      If timeout on service
*           NU_SEMAPHORE_DELETED            If semaphore deleted during
*                                           suspension
*           NU_SEMAPHORE_RESET              If semaphore reset during
*                                           suspension
*           NU_SEMAPHORE_ALREADY_OWNED      If task owning semaphore
*                                           tries to obtain it
*           NU_INVALID_SEMAPHORE            Invalid semaphore pointer
*           NU_INVALID_SUSPEND              Suspension from non-task
*
***********************************************************************/
STATUS NU_Obtain_Semaphore(NU_SEMAPHORE *semaphore_ptr, UNSIGNED suspend)
{
    R1 SM_SCB       *semaphore;             /* Semaphore control block ptr */
    R2 SM_SUSPEND   *suspend_ptr;           /* Suspend block pointer     */
    SM_SUSPEND      suspend_block;          /* Allocate suspension block */
    TC_TCB          *task;                  /* Task pointer              */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Determine if semaphore pointer is invalid */
    NU_ERROR_CHECK((semaphore == NU_NULL), status, NU_INVALID_SEMAPHORE);

    /* Determin if the semaphore pointer is valid */
    NU_ERROR_CHECK((semaphore -> sm_id != SM_SEMAPHORE_ID), status, NU_INVALID_SEMAPHORE);

    /* Determine if suspension is available */
    NU_ERROR_CHECK(((suspend) && (TCCE_Suspend_Error())), status, NU_INVALID_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Get a pointer the currently executing task */
        task = (TC_TCB *) TCCT_Current_Thread();

        /* Protect against simultaneous access to the semaphore.  */
        TCCT_Schedule_Lock();

        /* Determine if the semaphore has an instance (can be decremented). */
        if (semaphore -> sm_semaphore_count)
        {
            /* Semaphore available.  Decrement and return to the caller.  */
            semaphore -> sm_semaphore_count--;

            /* Semaphore is not owned, so this task may obtain it */
            if (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT)
            {
                /* Increment the task PI resource count */
                task -> tc_semaphore_count++;

                /* Check to see if previous owner died while owning the mutex */
                if (semaphore -> sm_owner_killed == NU_TRUE)
                {
                    /* Set status to show previous owner is dead */
                    status = NU_SEMAPHORE_OWNER_DEAD;

                    /* Reset flag */
                    semaphore -> sm_owner_killed = NU_FALSE;
                }

                /* Make this task the semaphore owner */
                semaphore -> sm_semaphore_owner = task;

                /* Place this semaphore on the tasks semaphore list */
                NU_Place_On_List((CS_NODE **)&(task -> tc_semaphore_list),
                                 &(semaphore->sm_semaphore_list));
            }

            /* Trace log */
            T_SEM_OBTAIN((VOID*)semaphore, (VOID*)task, suspend, status);
        }
        /* Semaphore is not available to the task */
        else
        {
            /* Semaphore is not available.  Determine if suspension is required. */
            if (suspend)
            {
                 /* Ensure the PI semaphore is not already owned by this task */
                if ((semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT) &&
                    (semaphore -> sm_semaphore_owner == task))
                {
                    status = NU_SEMAPHORE_ALREADY_OWNED;

                    /* Trace log */
                    T_SEM_OBTAIN((VOID*)semaphore, (VOID*) task, suspend, status);
                }
                else
                {
                    /* Suspension is selected.  */
                    /* Increment the number of tasks waiting.  */
                    semaphore -> sm_tasks_waiting++;

                    /* Setup the suspend block and suspend the calling task.  */
                    suspend_ptr =  &suspend_block;
                    suspend_ptr -> sm_semaphore =                semaphore;
                    suspend_ptr -> sm_suspend_link.cs_next =     NU_NULL;
                    suspend_ptr -> sm_suspend_link.cs_previous = NU_NULL;
                    suspend_ptr -> sm_suspended_task =           task;

                    /* Determine if priority or FIFO suspension is associated with the
                       semaphore.  */
                    if (semaphore -> sm_suspend_type == NU_FIFO)
                    {

                        /* FIFO suspension is required.  Link the suspend block into
                           the list of suspended tasks on this semaphore.  */
                        NU_Place_On_List((CS_NODE **)
                                         &(semaphore -> sm_suspension_list),
                                         &(suspend_ptr -> sm_suspend_link));
                    }
                    else
                    {

                        /* Get the priority of the current thread so the suspend block
                           can be placed in the appropriate place.  */
                        suspend_ptr -> sm_suspend_link.cs_priority =
                                                            TCC_Task_Priority(task);

                        NU_Priority_Place_On_List((CS_NODE **)
                                                  &(semaphore -> sm_suspension_list),
                                                  &(suspend_ptr -> sm_suspend_link));

                        /* Check if this is a PI semaphore and priority of calling task is higher
                           (lower value in PLUS) than owner task */
                        if ((semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT) &&
                             (task -> tc_priority < semaphore -> sm_semaphore_owner -> tc_priority))
                        {
                            /* Let the owner task inherit the priority of this high priority task */
                            (VOID) TCS_Change_Priority (semaphore -> sm_semaphore_owner,
                                                           task -> tc_priority, NU_FALSE);
                        }
                    }

                    /* Trace log */
                    T_SEM_OBTAIN((VOID*)semaphore, (VOID*)task, suspend, NU_SEMAPHORE_SUSPEND);

                    /* Finally, suspend the calling task. Note that the suspension call
                       automatically clears the protection on the semaphore.  */
                    TCC_Suspend_Task((NU_TASK *) task, NU_SEMAPHORE_SUSPEND,
                                                SMC_Cleanup, suspend_ptr, suspend);

                    /* Pickup the return status.  */
                    status =  suspend_ptr -> sm_return_status;

                    /* Check to see if status shows previous owner killed */
                    if (status == NU_SEMAPHORE_OWNER_DEAD)
                    {
                        /* Clear flag showing previous owner killed */
                        semaphore -> sm_owner_killed = NU_FALSE;
                    }
                }
            }
            else
            {
                /* No suspension requested.  Simply return an error status.  */
                status =  NU_UNAVAILABLE;

                /* Trace log */
                T_SEM_OBTAIN((VOID*)semaphore, (VOID*) task, suspend, status);
            }
        }

        /* Release protection against access to the semaphore.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SEM_OBTAIN((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), suspend, status);
    }

    /* Return the completion status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Release_Semaphore
*
*   DESCRIPTION
*
*       This function releases a previously obtained semaphore.  If one
*       or more tasks are waiting, the first task is given the released
*       instance of the semaphore.  Otherwise, the semaphore instance
*       counter is simply incremented.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove from suspend list
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect semaphore
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       semaphore_ptr                       Semaphore control block ptr
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Success
*           NU_SEMAPHORE_INVALID_OWNER      Semaphore is being released
*                                           by task other than owner
*           NU_INVALID_SEMAPHORE            Invalid semaphore pointer
*           NU_SEMAPHORE_COUNT_ROLLOVER     Count Rollover
*
***********************************************************************/
STATUS NU_Release_Semaphore (NU_SEMAPHORE *semaphore_ptr)
{
    R1 SM_SCB       *semaphore;             /* Semaphore control block ptr  */
    STATUS          status = NU_SUCCESS;    /* Status                       */
    NU_SUPERV_USER_VARIABLES


    /* Move input semaphore pointer into internal pointer.  */
    semaphore =  (SM_SCB *) semaphore_ptr;

    /* Determine if semaphore pointer is valid */
    NU_ERROR_CHECK((semaphore == NU_NULL), status, NU_INVALID_SEMAPHORE);

    /* Semaphore pointer is invalid, indicate in completion status.  */
    NU_ERROR_CHECK((semaphore -> sm_id != SM_SEMAPHORE_ID), status, NU_INVALID_SEMAPHORE);

    /* Semaphore count is about to roll-over, indicate in completion status. */
    NU_ERROR_CHECK(((1 + semaphore -> sm_semaphore_count) == 0), status, NU_SEMAPHORE_COUNT_ROLLOVER);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the semaphore.  */
        TCCT_Schedule_Lock();

        /* Call internal funciton to release semaphore */
        status = smc_release_semaphore(semaphore, (NU_TASK *) TCCT_Current_Thread());

        /* Release protection against access to the semaphore.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_SEM_RELEASE((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), status);
    }

    /* Return the completion status.  */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       smc_release_semaphore
*
*   DESCRIPTION
*
*       This internal function is responsible for the main work necessary
*       to release a semaphore or PI mutex.  This function expects
*       necessary critical section protection to be in place before
*       calling.
*
*       NOTE:  The kernel lock must be owned by the caller of this
*              function.
*
*   CALLED BY
*
*       NU_Release_Semaphore                Release semaphore
*       SMC_Kill_Semaphore_Owner            Killing PI mutex owner
*
*   CALLS
*
*       NU_Remove_From_List                 Remove from suspend list
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*
*   INPUTS
*
*       semaphore                           Semaphore control block ptr
*       task                                Current task pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_SEMAPHORE_INVALID_OWNER
*
***********************************************************************/
static STATUS   smc_release_semaphore(SM_SCB *semaphore, NU_TASK * task)
{
    R2 SM_SUSPEND   *suspend_ptr;           /* Pointer to suspend block     */
    STATUS          preempt;                /* Status for preemption        */
    STATUS          status = NU_SUCCESS;


    /* Initialize preempt variables */
    preempt = NU_FALSE;

    /* Check if this is a PI Semaphore */
    if (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT)
    {
        /* Ensure that the PI semaphore has been obtained previously */
        /* Ensure calling task is the owner */
        if (semaphore -> sm_semaphore_owner != task)
        {
            status = NU_SEMAPHORE_INVALID_OWNER;

            /* Trace log */
            T_SEM_RELEASE((VOID*)semaphore, (VOID*)task, status);
        }
        else
        {
            /* Free Semaphore and adjust priority of the task
               resetting the semaphore */
            preempt = SMC_Free_PI_Semaphore (semaphore, task);

            /* Clear the owner field of the semaphore */
            semaphore -> sm_semaphore_owner = NU_NULL;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Determine if another task is waiting on the semaphore.  */
        if (semaphore -> sm_tasks_waiting)
        {
            /* Yes, another task is waiting for an instance of the semaphore.  */

            /* Decrement the number of tasks waiting counter.  */
            semaphore -> sm_tasks_waiting--;

            /* Remove the first suspended block from the list.  */
            suspend_ptr =  semaphore -> sm_suspension_list;
            NU_Remove_From_List((CS_NODE **) &(semaphore -> sm_suspension_list),
                                &(suspend_ptr -> sm_suspend_link));

            /* Setup the appropriate return value.  */
            suspend_ptr -> sm_return_status =  NU_SUCCESS;

            if (semaphore -> sm_suspend_type == NU_PRIORITY_INHERIT)
            {
                /* Increment the task PI resource count */
                suspend_ptr -> sm_suspended_task -> tc_semaphore_count++;

                /* Make this task the semaphore owner */
                semaphore -> sm_semaphore_owner = suspend_ptr -> sm_suspended_task;

                /* Place this semaphore on the tasks semaphore list */
                NU_Place_On_List((CS_NODE **)&(suspend_ptr -> sm_suspended_task ->
                                 tc_semaphore_list), &(semaphore->sm_semaphore_list));

                /* Check to see if previous owner is dead */
                if (semaphore -> sm_owner_killed == NU_TRUE)
                {
                    /* Set status to show this error */
                    suspend_ptr -> sm_return_status =  NU_SEMAPHORE_OWNER_DEAD;
                }
            }

            /* Trace log */
            T_SEM_RELEASE((VOID*)semaphore, (VOID*)task, OBJ_UNBLKD_CTXT);

            /* Resume the suspended task.  */
            preempt = preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> sm_suspended_task,
                                                           NU_SEMAPHORE_SUSPEND);

            /* Trace log */
            T_SEM_RELEASE((VOID*)semaphore, (VOID*)task, OBJ_ACTION_SUCCESS);
        }
        /* No tasks are waiting, indicate semaphore is available */
        else
        {
            /* Increment the semaphore instance counter.  */
            semaphore -> sm_semaphore_count++;

            /* Trace log */
            T_SEM_RELEASE((VOID*)semaphore, (VOID*)TCCT_Current_Thread(), OBJ_ACTION_SUCCESS);
        }

        /* Determine if a preempt condition is present.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to the system if the resumed task function
               detects a preemption condition.  */
            TCCT_Control_To_System();
        }
    }

    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       SMC_Cleanup
*
*   DESCRIPTION
*
*       This function is responsible for removing a suspension block
*       from a semaphore.  It is not called unless a timeout or a task
*       terminate is in progress.  Note that protection is already in
*       effect - the same protection at suspension time.  This routine
*       must be called from Supervisor mode in Supervisor/User mode
*       switching kernels.
*
*   CALLED BY
*
*       TCC_Task_Timeout                    Task timeout
*       NU_Terminate_Task                   Task terminate
*
*   CALLS
*
*       NU_Remove_From_List                 Remove suspend block from
*                                           the suspension list
*
*   INPUTS
*
*       information                         Pointer to suspend block
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  SMC_Cleanup (VOID *information)
{
    SM_SUSPEND      *suspend_ptr;           /* Suspension block pointer  */

    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use the information pointer as a suspend pointer.  */
    suspend_ptr =  (SM_SUSPEND *) information;

    /* By default, indicate that the service timed-out.  It really does not
       matter if this function is called from a terminate request since
       the task does not resume.  */
    suspend_ptr -> sm_return_status =  NU_TIMEOUT;

    /* Decrement the number of tasks waiting counter.  */
    (suspend_ptr -> sm_semaphore) -> sm_tasks_waiting--;

    /* Unlink the suspend block from the suspension list.  */
    NU_Remove_From_List((CS_NODE **)
                        &((suspend_ptr -> sm_semaphore) -> sm_suspension_list),
                        &(suspend_ptr -> sm_suspend_link));

    /* Return to user mode */
    NU_USER_MODE();
}


/***********************************************************************
*
*   FUNCTION
*
*       SMC_Free_PI_Semaphore
*
*   DESCRIPTION
*
*       This function frees a PI semaphore and adjust owner task priority.
*
*   CALLED BY
*
*       NU_Reset_Semaphore                  Resets a semaphore
*       NU_Delete_Semaphore                 Deletes a semaphore
*       NU_Release_Semaphore                Releases a semaphore
*
*   CALLS
*
*       NU_Remove_From_List                 Removes a semaphore from task
*       TCS_Change_Priority                 Changes priority of task (protected)
*
*   INPUTS
*
*       pi_semaphore                        Semaphore control block ptr
*       task                                Pointer to calling Task
*
*   OUTPUTS
*
*       pi_preempt
*           NU_TRUE                         If preempt condition exists
*           NU_FALSE                        If no preempt condition exists
*
***********************************************************************/
STATUS  SMC_Free_PI_Semaphore (SM_SCB *pi_semaphore, TC_TCB *task)
{
    STATUS          pi_preempt = NU_FALSE;  /* Preemption Status */

    /* Decrement task resource count */
    task -> tc_semaphore_count--;

    /* Remove this PI Semaphore from the task semaphore list */
    NU_Remove_From_List((CS_NODE **)&(task -> tc_semaphore_list),
                        &(pi_semaphore->sm_semaphore_list));

    /* Check if this task owns any other PI semaphores and if
       task priority needs to be set back to its base value */
    if ((task -> tc_semaphore_count == 0) &&
        (task -> tc_base_priority != task -> tc_priority))
    {
        /* Set the priority of this task back to its base priority value */
        pi_preempt = TCS_Change_Priority (task, task -> tc_base_priority, NU_FALSE);
    }

    return (pi_preempt);
}


/***********************************************************************
*
*   FUNCTION
*
*       SMC_Kill_Semaphore_Owner
*
*   DESCRIPTION
*
*       This function will forcibly release a PI mutex owned by a task that
*       is being terminated.  This functionalty is similar to "robust"
*       mutexes supported in POSIX and the next task to obtain the
*       PI mutex will get an error returned that the previous owner
*       was killed while owning the mutex
*
*   CALLED BY
*
*       NU_Terminate_Task
*
*   CALLS
*
*       smc_release_semaphore               Release the semaphore
*
*   INPUTS
*
*       semaphore_ptr                       Pointer to semaphore to release
*       owning_task                         Pointer to task owning the semaphore
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    SMC_Kill_Semaphore_Owner(NU_SEMAPHORE *semaphore_ptr, NU_TASK *owning_task)
{
    /* Ensure owning task passed-in actually still owns the mutex */
    if (semaphore_ptr->sm_semaphore_owner == (TC_TCB *)owning_task)
    {
        /* Set flags showing that owner is dead */
        semaphore_ptr->sm_owner_killed = NU_TRUE;

        /* Call internal function to release the semaphore */
        (VOID)smc_release_semaphore(semaphore_ptr, owning_task);
    }
}

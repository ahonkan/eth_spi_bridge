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
*       tcs_priority.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the supplemental Change Priority routine
*       for the Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCS_Change_Priority                 Change task's priority
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       nu_trace_os_mark.h                  Trace markers
*       proc_core.h                         Process defines
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"
#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
#include        "os/kernel/process/core/proc_core.h"
#endif

/* Define external inner-component global data references.  */

extern TC_TCB               *TCD_Priority_List[TC_PRIORITIES+1];
extern UNSIGNED             TCD_Priority_Groups;
extern DATA_ELEMENT         TCD_Sub_Priority_Groups[TC_MAX_GROUPS];
extern UNSIGNED_CHAR        TCD_Lowest_Set_Bit[];
extern INT                  TCD_Highest_Priority;

/* Internal function prototypes */
STATUS TCS_Change_Priority (TC_TCB *task, OPTION new_priority, BOOLEAN app_call);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Change_Priority
*
*   DESCRIPTION
*
*       This function changes the priority of the specified task.  The
*       priority of a suspended or a ready task can be changed.  If the
*       new priority necessitates a context switch, control is
*       transferred back to the system.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect scheduling data
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TCCT_Schedule_Unlock                Release protection of data
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       new_priority                        New priority for task
*
*   OUTPUTS
*
*       old_priority                        Original task priority, or
*                                           if an error is detected the
*                                           priority passed.
*
***********************************************************************/
OPTION NU_Change_Priority(NU_TASK *task, OPTION new_priority)
{
    STATUS status = NU_SUCCESS;             /* Preemption status */
    OPTION old_priority = new_priority;     /* Initialized to new_priority
                                               in case an error is detected */
    NU_SUPERV_USER_VARIABLES

    /* Determine if the task pointer is valid. */
    NU_ERROR_CHECK(((task == NU_NULL) || ((task -> tc_id != TC_TASK_ID))), status, NU_INVALID_TASK);

#ifdef  CFG_NU_OS_KERN_PROCESS_CORE_ENABLE

    /* User process task requested priority value less than minimum priority. */
    NU_ERROR_CHECK(((((PROC_CB *)task -> tc_process) -> kernel_mode == NU_FALSE) && (new_priority < PROC_TASK_MIN_PRIORITY)), status, NU_INVALID_PRIORITY);

#endif  /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against multiple access to the scheduling list.  */
        TCCT_Schedule_Lock();

        /* Save the old priority of the task.  */
        old_priority =  task -> tc_priority;

        /* Trace log */
        T_TASK_CHG_PRIORITY((VOID*)task, new_priority, old_priority);

        /* Check if a task priority being changed to new priority */
        if (task -> tc_priority != new_priority)
        {
            /* Call TCS_Change_Priority to change the task priority */
            status = TCS_Change_Priority (task, new_priority, NU_TRUE);

            /* Check is task switch is needed */
            if (status == NU_TRUE)
            {
                /* Trace log */
                T_TASK_SUSPEND((VOID*)TCCT_Current_Thread(), NU_PURE_SUSPEND);

                /* Transfer control to system to facilitate preemption.  */
                TCCT_Control_To_System();
            }
        }

        /* Release the protection of the scheduling list.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TASK_CHG_PRIORITY((VOID*)task, new_priority, old_priority);
    }

    /* Return the old priority.  */
    return (old_priority);
}


/***********************************************************************
*
*   FUNCTION
*
*       TCS_Change_Priority
*
*   DESCRIPTION
*
*       This function changes the priority of the specified task.  The
*       priority of a suspended or a ready task can be changed.
*       It returns NU_TRUE if the control needs to be transfered to the
*       system (because a task switch needs to happen).
*
*   CALLED BY
*
*       NU_Change_Priority                  Priority change service
*
*   CALLS
*
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       NU_Obtain_Semaphore                 Obtains a semaphore
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       new_priority                        New priority for task
*       app_call                            NU_TRUE if service called by
*                                           application
*
*   OUTPUTS
*
*       status
*           NU_TRUE                         Task switch needed
*           NU_FALSE                        No task switch needed
*
***********************************************************************/
STATUS   TCS_Change_Priority (TC_TCB *task, OPTION new_priority, BOOLEAN app_call)
{
    R2 TC_TCB       *head;                  /* Head list pointer         */
    R3 INT          index;                  /* Working index variable    */
    DATA_ELEMENT    temp;                   /* Temporary variable        */
    STATUS          status;                 /* Preemption status         */

    /* Initialize the status variable */
    status = NU_FALSE;

    /* Ensure the task gets a full time-slice next time
       it is scheduled */
    task -> tc_cur_time_slice = task -> tc_time_slice;

    /* Check to see if the task is currently ready.  */
    if (task -> tc_status == NU_READY)
    {
       /* Check if the task owns semaphores and the new priority is lower (higher value in
          PLUS) than the task's existing priority. This condition will never pass if this
          function is called internally from the Semaphore routine. */
        if ((task -> tc_semaphore_count != 0) && (new_priority > task -> tc_priority))
        {
            /* Update task base priority only */
            task -> tc_base_priority = new_priority;
        }
        else
        {
            /* Remove the task from the ready list.  */

            /* Determine if the task is the only one on the list.  */
            if (task -> tc_ready_next == task)
            {
                /* Only task on the list.  Clear the task's pointers and
                   clear the entry in the priority table.  */
                task -> tc_ready_next =        NU_NULL;
                task -> tc_ready_previous =    NU_NULL;
                *(task -> tc_priority_head) =  NU_NULL;

                /* Clear the sub-priority group.  */
                *(task -> tc_sub_priority_ptr) &= (DATA_ELEMENT)(~(task -> tc_sub_priority));

                /* Determine if the main priority group needs to be cleared.
                   This is only true if there are no other bits set in this
                   sub-priority.  */
                if (*(task -> tc_sub_priority_ptr) == 0)
                {
                    /* Clear the main priority group bit.  */
                    TCD_Priority_Groups = TCD_Priority_Groups & ~(task -> tc_priority_group);
                }
            }
            else
            {

                /* Not the only task ready at the same priority level.  */

                /* Remove from the linked-list.  */
                (task -> tc_ready_previous) -> tc_ready_next = task -> tc_ready_next;
                (task -> tc_ready_next) -> tc_ready_previous = task -> tc_ready_previous;

                /* Check if the task is at the head of its priority list */
                if (*(task -> tc_priority_head) ==  task)
                {
                    /* Update the head pointer.  */
                    *(task -> tc_priority_head) =  task -> tc_ready_next;
                }

                /* Clear the next and previous pointers.  */
                task -> tc_ready_next =        NU_NULL;
                task -> tc_ready_previous =    NU_NULL;
            }

            /* Now add in the task at the new priority.  */
            task -> tc_priority =  new_priority;

            /* We do not want to change the task base priority if this function is
               called internally from the semaphore routine. Update base priority
               only if an application has called this service routine. */
            if (app_call == NU_TRUE)
            {
                /* Update base priority */
                task -> tc_base_priority = new_priority;
            }

            /* Build the other priority information.  */
            /* There are two bit maps associated with each task. The first bit map
               indicates which group of 8-priorities it is. The second bit map
               indicates the actual priority within the group.  */
            task -> tc_priority_head = &(TCD_Priority_List[new_priority]);
            task -> tc_sub_priority =  (DATA_ELEMENT) (1 << (new_priority & 7));
            task -> tc_priority_group =   ((UNSIGNED) 1) << (new_priority >> 3);
            task -> tc_sub_priority_ptr = &(TCD_Sub_Priority_Groups[(new_priority >> 3)]);

            /* Link the task into the new priority list.  */
            head =  *(task -> tc_priority_head);

            /* Determine if the list is non-empty.  */
            if (head)
            {

                /* Add the TCB to the end of the ready list.  */
                task -> tc_ready_previous =      head -> tc_ready_previous;
                (task -> tc_ready_previous) -> tc_ready_next =  task;
                task -> tc_ready_next =          head;
                (task -> tc_ready_next) -> tc_ready_previous =  task;

                /* Note that the priority bit map does not need to be
                   modified since there are other active tasks at the
                   same priority.  */
            }
            else
            {

                /* Add the TCB to an empty list.  */
                task -> tc_ready_previous =  task;
                task -> tc_ready_next =      task;
                *(task -> tc_priority_head)= task;

                /* Update the priority group bit map to indicate that this
                   priority now has a task ready.  */
                TCD_Priority_Groups = TCD_Priority_Groups | (task -> tc_priority_group);

                /* Update the sub-priority bit map to show that this priority
                   is ready.  */
                *(task -> tc_sub_priority_ptr) |= (DATA_ELEMENT) task -> tc_sub_priority;
            }

            /* Determine the highest priority task in the system.  */
            if (TCD_Priority_Groups & TC_HIGHEST_MASK)
            {
                /* Base of sub-group is 0.  */
                index =  0;
            }
            else if (TCD_Priority_Groups & TC_NEXT_HIGHEST_MASK)
            {
                /* Base of sub-group is 8.  */
                index =  8;
            }
            else if (TCD_Priority_Groups & TC_NEXT_LOWEST_MASK)
            {
                /* Base of sub-group is 16.  */
                index =  16;
            }
            else
            {
                /* Base of sub-group is 24.  */
                index =  24;
            }

            /* Calculate the highest available priority.  */
            index =  index + TCD_Lowest_Set_Bit[(INT)
                       ((TCD_Priority_Groups >> index) & TC_HIGHEST_MASK)];

            /* Verify index is within bounds */
            if (index >= TC_MAX_GROUPS)
            {
                index = TC_MAX_GROUPS - 1;
            }

            /* Get the mask of the priority within the group of 8 priorities.  */
            temp =  TCD_Sub_Priority_Groups[index];

            /* Calculate the actual priority.  */
            TCD_Highest_Priority =  (index << 3) + TCD_Lowest_Set_Bit[temp];

            /* Check for preemption.  */
            if ((TCD_Highest_Priority <= ((INT) TCD_Execute_Task -> tc_priority)) &&
                (TCD_Execute_Task -> tc_preemption))
            {

                /* Update the current task pointer.  */
                TCCT_Set_Execute_Task(TCD_Priority_List[TCD_Highest_Priority]);

                /* Now, check and see if the current thread is a task.
                   If so, return a status that indicates a context
                   switch is needed.  */
                if ((TCD_Current_Thread) &&
                    (((TC_TCB *) TCD_Current_Thread) -> tc_id == TC_TASK_ID))
                {
                    /* Transfer control to the system.  */
                    status = NU_TRUE;
                }
            }
        }
    }
    else
    {
        /* Reset base priority to its old value */
        task -> tc_base_priority = task -> tc_priority;
        
        /* Modify the priority.  */
        task -> tc_priority = new_priority;

        /* Build the other priority information.  */
        /* There are two bit maps associated with each task.
        The first bit map indicates which group of 8-priorities it is.
        The second bit map indicates the actual priority within the group.  */
        task -> tc_priority_head = &(TCD_Priority_List[new_priority]);
        task -> tc_sub_priority =  (DATA_ELEMENT) (1 << (new_priority & 7));
        task -> tc_priority_group =   ((UNSIGNED) 1) << (new_priority >> 3);
        task -> tc_sub_priority_ptr = &(TCD_Sub_Priority_Groups[(new_priority >> 3)]);
    }


    /* Return whether task switch is needed  */
    return (status);
}


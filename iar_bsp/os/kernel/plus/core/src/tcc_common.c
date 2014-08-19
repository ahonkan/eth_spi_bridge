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
*       tcc_common.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core common routines for the Thread Control
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCC_Resume_Task                     Resume a task
*       TCC_Suspend_Task                    Suspend a task
*       TCC_Task_Timeout                    Task timeout function
*       TCC_Time_Slice                      Process task time-slice
*       TCC_Task_Shell                      Task execution shell
*       TCC_Unhandled_Interrupt             Handler for unregistered
*                                           interrupts
*       TCC_Unhandled_Exception             Handler for unregistered
*                                           exceptions
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       thread_control.h                    Task Control functions
*       initialization.h                    Initialization functions
*       timer.h                             Timer Control function
*       dynamic_memory.h                    Deallocate Memory Control
*                                           functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/initialization.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "os/kernel/plus/supplement/inc/error_management.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#ifdef  CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
#include        "os/kernel/process/core/proc_core.h"
#endif
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern TC_TCB               *TCD_Priority_List[TC_PRIORITIES+1];
extern UNSIGNED             TCD_Priority_Groups;
extern DATA_ELEMENT         TCD_Sub_Priority_Groups[TC_MAX_GROUPS];
extern UNSIGNED_CHAR        TCD_Lowest_Set_Bit[];
extern INT                  TCD_Highest_Priority;
extern INT                  TCD_Unhandled_Interrupt;
extern INT                  TCD_Unhandled_Exception;
extern VOID                 *TCD_Unhandled_Exception_SP;

/***********************************************************************
*
*   FUNCTION
*
*       TCC_Resume_Task
*
*   DESCRIPTION
*
*       This function resumes a previously suspended task.  The task
*       task must currently be suspended for the same reason indicated
*       by this request.  If the task resumed is higher priority than
*       the calling task and the current task is preemptable, this
*       function returns a value of NU_TRUE.  Otherwise, if no
*       preemption is required, a NU_FALSE is returned.  This routine
*       must be called from Supervisor mode in a Supervisor/User mode
*       switching kernel.
*
*   CALLED BY
*
*       NU_Deallocate_Memory                Deallocates a previously
*                                           allocated dynamic memory block
*       NU_Delete_Memory_Pool               Deletes a dynamic memory pool
*       NU_Delete_Event_Group               Deletes a event group
*       NU_Set_Events                       Sets event flags within the
*                                           specified event flag group
*       NU_Delete_Mailbox                   Delete a mailbox
*       NU_Receive_From_Mailbox             Receives a 4-word message from a
*                                           specified mailbox
*       NU_Send_To_Mailbox                  Sends a 4-word message to the
*                                           specified mailbox
*       NU_Broadcast_To_Mailbox             Sends a message to all tasks
*                                           currently waiting for a message
*                                           from the mailbox
*       NU_Reset_Mailbox                    Resets a mailbox back to the
*                                           initial state
*       NU_Delete_Pipe                      Delete a pipe
*       NU_Receive_From_Pipe                Receives a message from a specified pipe
*       NU_Send_To_Pipe                     Sends a message to the specified pipe
*       NU_Broadcast_To_Pipe                Sends a message to all tasks
*                                           currently waiting for a message
*                                           from the pipes
*       NU_Reset_Pipe                       Resets a pipe back to the initial state
*       NU_Send_To_Front_Of_Pipe            Send a message to the front of a pipe
*       NU_Deallocate_Partition             Deallocates a previously
*                                           allocated dynamic memory block
*       NU_Delete_Partition_Pool            Delete memory partition pool
*       NU_Delete_Queue                     Delete a queue
*       NU_Receive_From_Queue               Receives a message from a specified queue
*       NU_Send_To_Queue                    Sends a message to the specified queue
*       NU_Broadcast_To_Queue               Sends a message to all tasks
*                                           currently waiting for a message
*                                           from the queue
*       NU_Reset_Queue                      Resets a queue back to the initial state
*       NU_Send_To_Front_Of_Queue           Send a message to the front of a queue
*       NU_Delete_Semaphore                 Delete a semaphore
*       NU_Release_Semaphore                Release specified semaphore
*       NU_Reset_Semaphore                  Resets a semaphore back to the
*                                           initial state
*       NU_Resume_Task                      Interface identical to the application
*                                           service call to resume a task
*       NU_Task_Timeout                     Processes task suspension
*                                           timeout conditions
*       NU_Create_Task                      Create a task
*       NU_Send_Signals                     sends the specified task the
*                                           specified signals
*
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TMC_Stop_Task_Timer                 Stop timer
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       suspend_type                        Type of suspension to lift
*
*   OUTPUTS
*
*       status
*           NU_TRUE                         A higher priority task is
*                                           ready to execute
*           NU_FALSE                        No change in the task to
*                                           execute
*
***********************************************************************/
STATUS  TCC_Resume_Task(NU_TASK *task_ptr, OPTION suspend_type)
{
    R1 TC_TCB       *task;                  /* Task control block ptr    */
    R2 TC_TCB       *head;                  /* Pointer to priority list  */
    STATUS          status = NU_FALSE;      /* Status variable           */

    /* Move task pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Check to see if the task is suspended for the reason that this
       resume is attempting to clear.  */
    if (task -> tc_status == suspend_type)
    {
        /* Yes, this resume call is valid.  */

        /* Trace log */
        T_TASK_READY(task);

        /* If signals are not active, clear any suspend or cleanup
           information */
        if (!task -> tc_signal_active)
        {
            task -> tc_cleanup =                NU_NULL;
            task -> tc_cleanup_info =           NU_NULL;
        }

        /* Determine if there is a timer active and the task is not being
           resumed to handle a signal.  */
        if ((task -> tc_timer_active) && (!task -> tc_signal_active))
        {
            /* Stop the task timer */
            TMC_Stop_Task_Timer(&(task -> tc_timer_control));

            /* Clear the timer active flag.  */
            task -> tc_timer_active =  NU_FALSE;
        }


        /* Check to see if there is a pending debug suspension first.  If so
           change the cause of the suspension and leave in a suspended
           state. */
        if (task -> tc_debug_suspend != NU_READY)
        {
            /* Leave suspended but change the task's status and clear the
               debug suspension. */
            task -> tc_status = task -> tc_debug_suspend;
            task -> tc_debug_suspend = NU_READY;

        }
        /* Check to see if there is a pending suspension.  If so, change
           the cause of the suspension and leave in a suspended state.  */
        else if (task -> tc_delayed_suspend == NU_TRUE)
        {
            /* Leave suspended but change the task's status and clear the
               delayed suspension.  */
            task -> tc_delayed_suspend =  NU_FALSE;
            task -> tc_status =  NU_PURE_SUSPEND;
        }
        else
        {
            /* Lift the suspension of the specified task.  */

            /* Clear the status of the task.  */
            task -> tc_status =  NU_READY;

            /* Link the task into the appropriate priority list.  */
            head =  *(task -> tc_priority_head);

            /* Determine if the list is non-empty.  */
            if (head)
            {

                /* Add the new TCB to the end of the ready list.  */
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

                /* Add the new TCB to an empty list.  */
                task -> tc_ready_previous =  task;
                task -> tc_ready_next =      task;
                *(task -> tc_priority_head)= task;

                /* Update the priority group bit map to indicate that this
                   priority now has a task ready.  */
                TCD_Priority_Groups =
                        TCD_Priority_Groups | (task -> tc_priority_group);

                /* Update the sub-priority bit map to show that this priority
                   is ready.  */
                *(task -> tc_sub_priority_ptr) |= (DATA_ELEMENT)task -> tc_sub_priority;

                /* Determine if this newly ready task is higher priority
                   than the current task.  */
                if ((INT) (task -> tc_priority) < TCD_Highest_Priority)
                {

                    /* Update the highest priority field.  */
                    TCD_Highest_Priority = (INT) task -> tc_priority;

                    /* See if there is a task to execute.  */
                    if (TCD_Execute_Task == NU_NULL)
                    {
                        /* Make this task the current.  */
                        TCCT_Set_Execute_Task(task);
                    }

                    /* Check to see if the task to execute is preemptable. */
                    else if ((TCD_Execute_Task -> tc_preemption) ||
                             (INC_Initialize_State == INC_START_INITIALIZE))
                    {

                        /* Trace log */
                        T_TASK_READY((VOID*)TCD_Execute_Task);

                        /* Yes, the task to execute is preemptable.  Replace
                           it with the new task.  */
                        TCCT_Set_Execute_Task(task);

                        /* Now, check and see if the current thread is a task.
                           If so, return a status that indicates a context
                           switch is needed.  */
                        if ((TCD_Current_Thread) &&
                           (((TC_TCB *) TCD_Current_Thread) -> tc_id ==
                                TC_TASK_ID))
                        {
                            /* Yes, a context switch is needed.  */
                            status =  NU_TRUE;
                        }

                    }
                }
            }
        }
    }
    else
    {
        /* Check for a resumption of a delayed pure suspend
           or debug suspend.  */
        if (suspend_type == NU_PURE_SUSPEND)
        {
            /* Clear the delayed suspension.  */
            task -> tc_delayed_suspend =  NU_FALSE;

            /* Trace log */
            T_TASK_SUSPEND(task, NU_PURE_SUSPEND);
        }
        else if (suspend_type == NU_DEBUG_SUSPEND)
        {
            /* Clear the delayed debug suspension.  */
            task -> tc_debug_suspend =  NU_READY;

            /* Trace log */
            T_TASK_READY(task);
        }

        /* Check for a signal active and the saved status the same as
           the resume request.  */
        if ((suspend_type == task -> tc_saved_status) &&
            (task -> tc_signal_active))
        {

            /* Indicate the saved status as ready.  */
            task -> tc_saved_status =  NU_READY;

            /* Determine if the task's timer is active.  */
            if (task -> tc_timer_active)
            {
                /* Stop the task timer */
                TMC_Stop_Task_Timer(&(task -> tc_timer_control));

                /* Clear the timer active flag.  */
                task -> tc_timer_active =  NU_FALSE;
            }
        }
    }

    /* Return back the status.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       TCC_Suspend_Task
*
*   DESCRIPTION
*
*       This function suspends the specified task.  If the specified
*       task is the calling task, control is transferred back to the
*       system.
*
*   CALLED BY
*
*       NU_Allocate_Memory                  Allocates memory from specified dynamic
*                                           memory pool.
*       NU_Allocate_Aligned_Memory          Allocates memory from specified dynamic
*                                           memory pool.
*       NU_Retrieve_Events                  Retrieves various combinations of event
*                                           flags from specified event group.
*       NU_Receive_From_Mailbox             Receives a 4-word message from a
*                                           specified mailbox
*       NU_Send_To_Mailbox                  Sends a 4-word message to the
*                                           specified mailbox
*       NU_Broadcast_To_Mailbox             Sends a message to all tasks currently
*                                           waiting for a message from the mailbox.
*       NU_Receive_From_Pipe                Receives a message from a specified pipe
*       NU_Send_To_Pipe                     Sends a message to the specified pipe
*       NU_Broadcast_To_Pipe                Sends a message to all tasks
*                                           currently waiting for a message
*                                           from the pipes
*       NU_Send_To_Front_Of_Pipe            Send a message to the front of a pipe
*       NU_Allocate_Partition               Allocate a partition from a
*                                           pool
*       NU_Send_To_Queue                    Sends a message to the specified queue
*       NU_Broadcast_To_Queue               Sends a message to all tasks
*                                           currently waiting for a message
*                                           from the queue
*       NU_Send_To_Front_Of_Queue           Send a message to the front of a queue
*       NU_Obtain_Semaphore                 Obtain instance of semaphore
*       NU_Suspend_Task                     Task suspend service
*       [TCC_Debug_Suspend_Service]         Debug suspend service
*                                           (conditionally compiled)
*       TCC_Task_Shell                      Task execution shell
*       NU_Sleep                            Provides task sleep suspensions.
*       NU_Terminate_Task                   Terminate the specified task
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect system structures
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TCCT_Schedule_Unlock                Release system protection
*       TMC_Start_Task_Timer                Start a timer
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       suspend_type                        Type of suspension to lift
*       cleanup                             Cleanup routine
*       information                         Information for cleanup
*       timeout                             Timeout on the suspension
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TCC_Suspend_Task(NU_TASK *task_ptr, OPTION suspend_type,
                       VOID (*cleanup) (VOID *), VOID *information,
                       UNSIGNED timeout)
{
    R1 TC_TCB       *task;                  /* Task control block ptr    */
    R2 INT          index;                  /* Working index variable    */
    DATA_ELEMENT    temp;                   /* Temporary variable        */


    /* Move input task pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Trace log */
    T_TASK_SUSPEND(task, suspend_type);

    /* Determine if there is a timeout to initiate.  */
    if (timeout != NU_SUSPEND)
    {

        /* Indicate that a task timer is active.  */
        task -> tc_timer_active =  NU_TRUE;

        /* Start a timeout on the suspension.  */
        TMC_Start_Task_Timer(&(task -> tc_timer_control),timeout);

    }

    /* Check to see if the task is currently ready.  */
    if (task -> tc_status == NU_READY)
    {

        /* Mark the task with the appropriate suspension code.  */
        task -> tc_status =        suspend_type;

        /* Store off termination information in the tasks control block. */
        task -> tc_cleanup =       cleanup;
        task -> tc_cleanup_info =  information;

        /* Remove the task from the ready list.  */

        /* Ensure the task gets a full time-slice next time
           it is scheduled */
        task -> tc_cur_time_slice = task -> tc_time_slice;

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
                TCD_Priority_Groups =
                    TCD_Priority_Groups & ~(task -> tc_priority_group);
            }

            /* Determine if this priority group was the highest in the
               system.  */
            if (task -> tc_priority == (DATA_ELEMENT) TCD_Highest_Priority)
            {

                /* Determine the highest priority task in the system.  */
                if (TCD_Priority_Groups == 0)
                {

                    /* Re-initialize the highest priority variable and
                       clear the current task pointer.  */
                    TCD_Highest_Priority =  TC_PRIORITIES;
                }
                else
                {

                    /* Find the next highest priority task.  */
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

                    /* Get the mask of the priority within the group of
                       8 priorities.  */
                    temp =  TCD_Sub_Priority_Groups[index];

                    /* Calculate the actual priority.  */
                    TCD_Highest_Priority =
                        (index << 3) + TCD_Lowest_Set_Bit[temp];
                }
            }
        }
        else
        {

            /* Not the only task ready at the same priority level.  */

            /* Remove from the linked-list.  */
            (task -> tc_ready_previous) -> tc_ready_next =
                                                 task -> tc_ready_next;
            (task -> tc_ready_next) -> tc_ready_previous =
                                                 task -> tc_ready_previous;

            /* See if the task being suspended is the current.  */
            if (*(task -> tc_priority_head) == task)
            {
                /* Update the head of this priority list.  */
                *(task -> tc_priority_head) =  task -> tc_ready_next;
            }

            /* Clear the task's pointers.  */
            task -> tc_ready_next =        NU_NULL;
            task -> tc_ready_previous =    NU_NULL;
        }

        /* Determine if this task the highest priority task.  */
        if ((task == TCD_Execute_Task) && (TCD_Highest_Priority <= TC_PRIORITIES))
        {
            /* Set next task to execute in TCD_Execute_Task */
            TCCT_Set_Execute_Task(TCD_Priority_List[TCD_Highest_Priority]);
        }

        /* See if the suspending task is the current thread and this is
           not a debug suspension request. Prevent returning to the scheduler
           for debug suspension during exception handling. */
        if ((task == (TC_TCB *) TCD_Current_Thread) &&
            (suspend_type != NU_DEBUG_SUSPEND))
        {
            /* Check to see if this task needs to be removed from the
               system. */
            if ((task -> tc_auto_clean == NU_TRUE) &&
                ((suspend_type == NU_FINISHED) ||
                 (suspend_type == NU_TERMINATED)))
            {
                /* Disable interrupts to avoid any issues with the current
                   stack and deallocation. */
                ESAL_GE_INT_FAST_ALL_DISABLE();

                /* Release protection.  */
                TCCT_Schedule_Unlock();

                /* Delete the task */
                (VOID)NU_Delete_Task(task);
            }

            /* Leave the task, transfer control to the system.  */
            TCCT_Control_To_System();
        }

    }
    else
    {
        /* Check for a pure suspension request and ensure task isn't
           already in pure suspend state.  If present, the delayed
           suspension flag is set. Check for a debug suspension request
          and ensure task isn't already in debug suspend state. */
        if ( (suspend_type == NU_PURE_SUSPEND) &&
             (task -> tc_status != NU_PURE_SUSPEND) )
        {
            /* Setup the delayed suspension flag.  */
            task -> tc_delayed_suspend =  NU_TRUE;
        }
        else if ( (suspend_type == NU_DEBUG_SUSPEND) &&
                (task -> tc_status != NU_DEBUG_SUSPEND) )
        {
            /* Setup a delayed debug suspension. */
            task -> tc_debug_suspend = NU_DEBUG_SUSPEND;

        }

    }
}


/***********************************************************************
*
*   FUNCTION
*
*       TCC_Task_Timeout
*
*   DESCRIPTION
*
*       This function processes task suspension timeout conditions.
*       Note that task sleep requests are also considered a timeout
*       condition.  This routine must be called from Supervisor mode in
*       a Supervisor/User mode switching kernel.
*
*   CALLED BY
*
*       TMC_Timer_Expiration                Timer expiration task
*
*   CALLS
*
*       Caller's cleanup function
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect scheduling list
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TCC_Task_Timeout(NU_TASK *task_ptr)
{
    R1 TC_TCB        *task;                 /* Task control block ptr    */
    DATA_ELEMENT     task_status;           /* Task status variable      */

    /* Move task control block pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Protect system data structures.  */
    TCCT_Schedule_Lock();

    /* Is a signal handler currently running? */
    if (task -> tc_signal_active)
    {
        /* Use the saved status for current task status */
        task_status =      task -> tc_saved_status;
    }
    else
    {
        /* Just use the current task status */
        task_status =      task -> tc_status;
    }

    /* Determine if the task is still suspended in the same manner.  */
    if ((task -> tc_status == task_status) ||
      ((task -> tc_signal_active) && (task -> tc_saved_status == task_status)))
    {

        /* Make sure that this timeout processing is still valid. */
        if ((task -> tc_timer_active) &&
                 (task -> tc_timer_control.tm_remaining_time == 0))
        {

            /* Clear the timer active flag.  */
            task -> tc_timer_active =  NU_FALSE;

            /* Call the cleanup function, if there is one.  */
            if (task -> tc_cleanup)
            {
                /* Call cleanup function.  */
                (*(task -> tc_cleanup)) (task -> tc_cleanup_info);
            }

            /* Resume the task.  */
            TCC_Resume_Task(task_ptr, task_status);
        }
    }

    /* Release current protection.  */
    TCCT_Schedule_Unlock();
}


/************************************************************************
*
*   FUNCTION
*
*       TCC_Time_Slice
*
*   DESCRIPTION
*
*       This function moves the specified task to the end of the other
*       tasks at the same priority level.  If the specified task is no
*       longer ready, this request is ignored.  This routine must be
*       called from Supervisor mode in a Supervisor/User mode
*       switching kernel.
*
*   CALLED BY
*
*       TMC_Timer_HISR                      Time-slice interrupt
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect the scheduling data
*       TCCT_Set_Execute_Task               Set TCD_Execute_Task pointer
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       task                                Task control block pointer
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID  TCC_Time_Slice(NU_TASK *task_ptr)
{
    R1 TC_TCB       *task;                  /* Task control block ptr    */
    R1 TC_TCB       *tail_task;


    /* Move input task control block pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Protect against multiple access to the system structures.  */
    TCCT_Schedule_Lock();

    /* Determine if another task is ready to run.  */
    if (((task -> tc_status == NU_READY) && (task -> tc_preemption)) &&
        ((task -> tc_ready_next != task) ||
         (task -> tc_priority != (DATA_ELEMENT) TCD_Highest_Priority)))
    {
        /* Trace log */
        T_TASK_READY((VOID*)task_ptr);

        /* Check if task is the current head */
        if (task == *(task -> tc_priority_head))
        {
            /* Move the executing task to the end of tasks having the same
               priority. */
            *(task -> tc_priority_head) =  task -> tc_ready_next;

            /* Setup the next task to execute.  */
            TCCT_Set_Execute_Task(TCD_Priority_List[TCD_Highest_Priority]);
        }
        /* Ensure task is not the current tail */
        else if (task != (*(task -> tc_priority_head)) -> tc_ready_previous)
        {
            /* Get a pointer to the tail task */
            tail_task = (*(task -> tc_priority_head)) -> tc_ready_previous;

            /* Remove the task from the ready list */
            (task -> tc_ready_previous) -> tc_ready_next = task -> tc_ready_next;
            (task -> tc_ready_next) -> tc_ready_previous = task -> tc_ready_previous;

            /* Set previous pointer for the new tail to the old tail */
            task -> tc_ready_previous = tail_task;

            /* Set the next pointer for the new tail to the head */
            task -> tc_ready_next = *(task -> tc_priority_head);

            /* Set the old tail's next pointer to the new tail */
            tail_task -> tc_ready_next = task;

            /* Set the head's previous pointer to the new tail */
            (*(task -> tc_priority_head)) -> tc_ready_previous = task;
        }
    }

    /* Release protection of the system structures.  */
    TCCT_Schedule_Unlock();
}


/***********************************************************************
*
*   FUNCTION
*
*       TCC_Task_Shell
*
*   DESCRIPTION
*
*       This function is shell from which all application tasks are
*       initially executed.  The shell causes the task to finish when
*       control is returned from the application task.  Also, the shell
*       passes argc and argv arguments to the task's entry function.
*
*   CALLED BY
*
*       NU_Create_Task                      Create Task
*       NU_Reset_Task                       Reset the specified task
*
*   CALLS
*
*       Task Entry Function
*       TCC_Suspend_Task                    Suspend task when finished
*       TCCT_Schedule_Lock                  Protect system structures
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TCC_Task_Shell(VOID)
{
#if defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE)
    /* Determine if task is not part kernel process */
    if (PROC_Scheduled_CB -> kernel_mode == NU_FALSE)
    {
        /* Run task in user mode. */
        PROC_AR_User_Mode();
    }
#endif

    /* Call the task's entry function with the argc and argv parameters
       supplied during task creation or reset.  */
    (*(TCD_Execute_Task -> tc_entry)) ((TCD_Execute_Task -> tc_argc),
                                       (TCD_Execute_Task -> tc_argv));

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
    
#if (CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE)
    /* Determine if task is part of kernel process */
    if (PROC_Scheduled_CB -> kernel_mode == NU_FALSE)
    {
        /* Switch to supervisor mode */
        PROC_SUPERVISOR_MODE();
    }
#endif  /* (CFG_NU_OS_KERN_PROCESS_CORE_SUP_USER_MODE == NU_TRUE) */

    if (PROC_Scheduled_CB != PROC_Kernel_CB)
    {
        /* Notify processes that a task has finished */
        PROC_Stopped();
    }

#endif  /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Protect system data structures.  */
    TCCT_Schedule_Lock();

    /* If the task returns, suspend it in a finished state.  Note that
       the task cannot execute again until it is reset.  Therefore, this
       call never returns.  */
    TCC_Suspend_Task((NU_TASK *) TCD_Execute_Task, NU_FINISHED,
                                NU_NULL, NU_NULL, NU_SUSPEND);

    /* Return to user mode */
    NU_USER_MODE();
}


/***********************************************************************
*
*   FUNCTION
*
*       TCC_Unhandled_Interrupt
*
*   DESCRIPTION
*
*       This function catches all unhandled interrupts and
*       traps them in the system error handling routine.
*
*   CALLED BY
*
*       ISR
*
*   CALLS
*
*       ERC_System_Error                    Unhandled interrupt error
*
*   INPUTS
*
*       unhandled_vector                    Vector number of interrupt
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    TCC_Unhandled_Interrupt(INT unhandled_vector)
{
    /* Save unhandled vector number in TCD_Unhandled_Interrupt.  */
    TCD_Unhandled_Interrupt = unhandled_vector;

    /* System error, unhandled interrupt.  */
    ERC_System_Error(NU_UNHANDLED_INTERRUPT);
}


/***********************************************************************
*
*   FUNCTION
*
*       TCC_Unhandled_Exception
*
*   DESCRIPTION
*
*       This function catches all unhandled exceptions and
*       traps them in the system error handling routine.
*
*   CALLED BY
*
*       ISR
*
*   CALLS
*
*       ERC_System_Error                    Unhandled exception error
*
*   INPUTS
*
*       unhandled_vector                    Vector number of exception
*       stack_ptr                           Stack pointer with
*                                           exception information
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID    TCC_Unhandled_Exception(INT unhandled_vector, VOID *stack_ptr)
{
    /* Save unhandled exception vector number in TCD_Unhandled_Exception.  */
    TCD_Unhandled_Exception = unhandled_vector;

    /* Save unhandled exception stack in TCD_Unhandled_Exception_SP */
    TCD_Unhandled_Exception_SP = stack_ptr;

    /* System error, unhandled exception.  */
    ERC_System_Error(NU_UNHANDLED_EXCEPTION);
}


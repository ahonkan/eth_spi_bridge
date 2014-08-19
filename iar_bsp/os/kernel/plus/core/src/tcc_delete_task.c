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
*       tcc_delete_task.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core delete task routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Task                      Delete a task
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE              *TCD_Created_Tasks_List;
extern UNSIGNED             TCD_Total_Tasks;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Task
*
*   DESCRIPTION
*
*       This function deletes a task and removes it from the list of
*       created tasks.  It is assumed by this function that the task is
*       in a finished or terminated state.  Note that this function
*       does not free memory associated with the task's control block or
*       its stack.  This is the responsibility of the application.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove node from list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created task list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_TASK                     Task pointer is invalid
*       NU_INVALID_DELETE                   Task not in a finished or
*                                           terminated state or owns
*                                           PI Semaphores.
*
***********************************************************************/
STATUS NU_Delete_Task(NU_TASK *task)
{
    STATUS status = NU_SUCCESS;             /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Determine if the supplied task pointer is valid.  */
    NU_ERROR_CHECK(((task == NU_NULL) || (task -> tc_id != TC_TASK_ID)), status, NU_INVALID_TASK);

    /* Verify the task is in the finished or terminated state */
    NU_ERROR_CHECK(((task -> tc_status != NU_FINISHED) && (task -> tc_status != NU_TERMINATED)), status, NU_INVALID_DELETE);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
        
        /* Remove the the task binding */
        status = PROC_Unbind_Task(task);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

        /* Protect the list of created tasks.

           In the case of a finished auto clean
           task, calling this would be less overhead
           than checking for the task state  */
        TCCT_Schedule_Lock();

        /* Log trace data */
        T_TASK_DELETED((VOID*)task, status);

        /* Remove the task from the list of created tasks.  */
        NU_Remove_From_List(&TCD_Created_Tasks_List, &(task -> tc_created));

        /* Decrement the total number of created tasks.  */
        TCD_Total_Tasks--;

        /* Clear the task ID just in case.  */
        task -> tc_id =  0;

        /* Remove this task from the application list
           if it is an application task */
        if(task->tc_grp_id == TC_GRP_ID_APP)
        {
            TCC_Application_Task_Remove(task);
        }

        /* Release protection.  */
        TCCT_Schedule_Unlock();
            
        /* If this is a finished or terminated auto clean task the scheduler
           needs to remain locked and we don't need to worry about the
           mode. */
        if (task -> tc_auto_clean != NU_TRUE)
        {
            /* Return to user mode */
            NU_USER_MODE();
        }
        else
        {
            /* Deallocate the task control block (which will also
                deallocate the stack).

                NOTE:  If this is being called in the same context as
                the deleted task, the deallocation will use the first 8 bytes of
                the memory pointer that was just deallocated. The deallocation
                will occur in what was the task control block.  The
                current stack will be updated before interrupts
                are re-enabled */
            (VOID)NU_Deallocate_Memory(task);
            
            /* Maintain lock on scheduler */
            TCCT_Schedule_Lock();
        }
    }
    else
    {
        /* Log trace data */
        T_TASK_DELETED((VOID*)task, status);
    }

    /* Return a successful completion.  */
    return(status);
}

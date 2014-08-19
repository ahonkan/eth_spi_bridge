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
*       pmc_delete.c
*
*   COMPONENT
*
*       PM - Partition Memory Management
*
*   DESCRIPTION
*
*       This file contains the core Delete routine for the
*       Partition Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Partition_Pool            Delete a Partition Pool
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       partition_memory.h                  Partition functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/supplement/inc/partition_memory.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *PMD_Created_Pools_List;
extern UNSIGNED         PMD_Total_Pools;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Partition_Pool
*
*   DESCRIPTION
*
*       This function deletes a memory partition pool and removes it
*       from the list of created partition pools.  All tasks suspended
*       on the partition pool are resumed with the appropriate error
*       status.  Note that this function does not free any memory
*       associated with either the pool area or the pool control block.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove node from list
*       TCC_Resume_Task                     Resume a suspended task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect created list
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pool_ptr                            Partition pool control block
*                                           pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid
*
***********************************************************************/
STATUS NU_Delete_Partition_Pool(NU_PARTITION_POOL *pool_ptr)
{
    R1 PM_PCB       *pool;                  /* Pool control block ptr    */
    PM_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    PM_SUSPEND      *next_ptr;              /* Next suspend block        */
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;    /* Completion status         */
    NU_SUPERV_USER_VARIABLES

    /* Move input pool pointer into internal pointer.  */
    pool =  (PM_PCB *) pool_ptr;

    /* Determine if the partition pool pointer is valid.  */
    NU_ERROR_CHECK(((pool == NU_NULL) || (pool -> pm_id != PM_PARTITION_ID)), status, NU_INVALID_POOL);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the partition pool.  */
        TCCT_Schedule_Lock();

        /* Clear the partition pool ID.  */
        pool -> pm_id =  0;

        /* Remove the partition pool from the list of created partition pools.  */
        NU_Remove_From_List(&PMD_Created_Pools_List, &(pool -> pm_created));

        /* Decrement the total number of created partition pools.  */
        PMD_Total_Pools--;

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  pool -> pm_suspension_list;

        /* Walk the chain task(s) currently suspended on the partition pool.  */
        preempt =  0;
        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_POOL_DELETED.  */
            suspend_ptr -> pm_return_pointer =  NU_NULL;
            suspend_ptr -> pm_return_status =   NU_POOL_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (PM_SUSPEND *) (suspend_ptr -> pm_suspend_link.cs_next);

            /* Trace log */
            T_PMEM_DELETE(pool, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> pm_suspended_task,
                                                    NU_PARTITION_SUSPEND);

            /* Determine if the next is the same as the current pointer.  */
            if (next_ptr == pool -> pm_suspension_list)
            {
                /* Clear the suspension pointer to signal the end of the list
                   traversal.  */
                suspend_ptr =  NU_NULL;
            }
            else
            {
                /* Move the next pointer into the suspend block pointer.  */
                suspend_ptr =  next_ptr;
            }
        }

        /* Trace log */
        T_PMEM_DELETE(pool, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the list of created partition
           pools. */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_PMEM_DELETE(pool, status);
    }

    /* Return a successful completion.  */
    return(status);
}

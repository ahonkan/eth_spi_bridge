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
*       dmc_delete.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains the core Delete routine for the Dynamic Memory
*       Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_Memory_Pool               Delete a dynamic memory pool
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       dynamic_memory.h                    Dynamic memory functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/dynamic_memory.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *DMD_Created_Pools_List;
extern UNSIGNED         DMD_Total_Pools;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_Memory_Pool
*
*   DESCRIPTION
*
*       This function deletes a dynamic memory pool and removes it from
*       the list of created memory pools.  All tasks suspended on the
*       memory pool are resumed with the appropriate error status.
*       Note that this function does not free any memory associated with
*       either the pool area or the pool control block.
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
*       pool_ptr                            Memory pool control block
*                                           pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_POOL                     Indicates the pool pointer
*                                           is invalid
*
***********************************************************************/
STATUS NU_Delete_Memory_Pool(NU_MEMORY_POOL *pool_ptr)
{
    R1 DM_PCB       *pool;                  /* Pool control block ptr    */
    DM_SUSPEND      *suspend_ptr;           /* Suspend block pointer     */
    DM_SUSPEND      *next_ptr;              /* Next suspend block        */
    STATUS          preempt;                /* Status for resume call    */
    STATUS          status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Move input pool pointer into internal pointer.  */
    pool =  (DM_PCB *) pool_ptr;

    /* Determine if dynamic memory pool pointer is invalid.  */
    NU_ERROR_CHECK((pool == NU_NULL), status, NU_INVALID_POOL);

    /* Ensure dynamic memory pool pointer is valid */
    NU_ERROR_CHECK((pool -> dm_id != DM_DYNAMIC_ID), status, NU_INVALID_POOL);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect against simultaneous access to the memory pool.  */
        TCCT_Schedule_Lock();

        /* Clear the memory pool ID.  */
        pool -> dm_id =  0;

        /* Remove the memory pool from the list of created memory pools.  */
        NU_Remove_From_List(&DMD_Created_Pools_List, &(pool -> dm_created));

        /* Decrement the total number of created memory pools.  */
        DMD_Total_Pools--;

        /* Pickup the suspended task pointer list.  */
        suspend_ptr =  pool -> dm_suspension_list;

        /* Walk the chain task(s) currently suspended on the memory pool.  */
        preempt =  0;

        while (suspend_ptr)
        {
            /* Resume the suspended task.  Insure that the status returned is
               NU_POOL_DELETED.  */
            suspend_ptr -> dm_return_pointer =  NU_NULL;
            suspend_ptr -> dm_return_status =   NU_POOL_DELETED;

            /* Point to the next suspend structure in the link.  */
            next_ptr =  (DM_SUSPEND *) (suspend_ptr -> dm_suspend_link.cs_next);

            /* Trace log */
            T_MEM_POOL_DELETE((VOID*)pool, OBJ_UNBLKD_CTXT);

            /* Resume the specified task.  */
            preempt =  preempt |
                TCC_Resume_Task((NU_TASK *) suspend_ptr -> dm_suspended_task,
                                                    NU_MEMORY_SUSPEND);

            /* Determine if the next is the same as the current pointer.  */
            if (next_ptr == pool -> dm_suspension_list)
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
        T_MEM_POOL_DELETE((VOID*)pool, OBJ_ACTION_SUCCESS);

        /* Determine if preemption needs to occur.  */
        if (preempt)
        {
            /* Trace log */
            T_TASK_READY((VOID*)TCCT_Current_Thread());

            /* Transfer control to system to facilitate preemption.  */
            TCCT_Control_To_System();
        }

        /* Release protection against access to the list of created memory
           pools. */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_MEM_POOL_DELETE((VOID*)pool, status);
    }

    /* Return a successful completion.  */
    return(status);
}

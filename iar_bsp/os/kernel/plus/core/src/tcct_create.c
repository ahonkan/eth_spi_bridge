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
*       tcct_create.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains core create (Task / HISR) thread control /
*       scheduling functions with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Create_Task                      Create Task
*       NU_Create_HISR                      Create HISR
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       timer.h                             Timer service constants
*       common_services.h                   Common Service constants
*
***********************************************************************/

/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "os/kernel/plus/core/inc/common_services.h"
#include        "services/nu_trace_os_mark.h"
#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
#include        "os/kernel/process/core/proc_core.h"
#endif
#include        <string.h>

/* Define external inner-component global data references */
extern CS_NODE              *TCD_Created_Tasks_List;
extern UNSIGNED             TCD_Total_Tasks;
extern TC_TCB               *TCD_Priority_List[TC_PRIORITIES+1];
extern DATA_ELEMENT         TCD_Sub_Priority_Groups[TC_MAX_GROUPS];
extern CS_NODE              *TCD_Created_HISRs_List;
extern UNSIGNED             TCD_Total_HISRs;

/* Define external function references */
extern VOID                 TCC_Task_Shell(VOID);
extern VOID                 TCCT_HISR_Shell(VOID);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Task
*
*   DESCRIPTION
*
*       This function creates a task and then places it on the list of
*       created tasks.  All the resources necessary to create the task
*       are supplied to this routine.  If specified, the newly created
*       task is started.  Otherwise, it is left in a suspended state.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Place_On_List                    Add TCB to linked-list
*       ESAL_GE_STK_Unsolicited_Set         Populates unsolicited
*                                           stack frame
*       [PROC_Bind_Task]                    Processes
*       TCC_Resume_Task                     Start the created task
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Control_To_System              Transfer control to system
*       TCCT_Schedule_Lock                  Protect created task list
*       TCCT_Schedule_Unlock                Release protection of list
*       TMC_Init_Task_Timer                 Initialize the task's timer
*
*   INPUTS
*
*       task_ptr                            Task control block pointer
*       name                                Task name
*       task_entry                          Entry function of the task
*       argc                                Optional task parameter
*       argv                                Optional task parameter
*       stack_address                       Pointer to start of stack
*       stack_size                          Size of task stack in bytes
*       priority                            Task priority
*       time_slice                          Task time slice
*       preempt                             Task preemptability flag
*       auto_start                          Automatic task start
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Successful request
*           NU_INVALID_TASK                 Task control block pointer
*                                           is NULL
*           NU_INVALID_ENTRY                Task entry function is NULL
*           NU_INVALID_MEMORY               Stack pointer is NULL
*           NU_INVALID_SIZE                 Stack size is too small
*           NU_INVALID_PRIORITY             Invalid task priority
*           NU_INVALID_PREEMPT              Invalid preemption selection
*           NU_INVALID_START                Invalid start selection
*           NU_INVALID_PRIORITY             Priority value too high
*
***********************************************************************/
STATUS NU_Create_Task(NU_TASK *task, CHAR *name,
                      VOID (*task_entry)(UNSIGNED, VOID *),
                      UNSIGNED argc, VOID *argv,
                      VOID *stack_address, UNSIGNED stack_size,
                      OPTION priority, UNSIGNED time_slice,
                      OPTION preempt, OPTION auto_start)
{
    STATUS          status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Check each parameter.  */

    /* Invalid task control block pointer.  */
    NU_ERROR_CHECK(((task == NU_NULL) || (task -> tc_id == TC_TASK_ID)), status, NU_INVALID_TASK);

    /* Invalid task entry function pointer.  */
    NU_ERROR_CHECK((task_entry == NU_NULL), status, NU_INVALID_ENTRY);

    /* Invalid stack starting address.  */
    NU_ERROR_CHECK((stack_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Invalid stack size.  */
    NU_ERROR_CHECK((stack_size < NU_MIN_STACK_SIZE), status, NU_INVALID_SIZE);

    /* Invalid preemption.  */
    NU_ERROR_CHECK(((preempt != NU_PREEMPT) && (preempt != NU_NO_PREEMPT)), status, NU_INVALID_PREEMPT);

    /* Invalid start selection. */
    NU_ERROR_CHECK(((auto_start != NU_START) && (auto_start != NU_NO_START)), status, NU_INVALID_START);

    /* Priority value exceeds maximum priority. */
    NU_ERROR_CHECK((priority >= TC_PRIORITIES), status, NU_INVALID_PRIORITY);

#ifdef  CFG_NU_OS_KERN_PROCESS_CORE_ENABLE

    /* User process task priority value less than minimum priority. */
    NU_ERROR_CHECK(((PROC_Scheduled_CB -> kernel_mode == NU_FALSE) && (priority < PROC_TASK_MIN_PRIORITY)), status, NU_INVALID_PRIORITY);

#endif  /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(task, NU_TASK);

        /* Fill in the task name. */
        strncpy(task -> tc_name, name, (NU_MAX_NAME - 1));

        /* Fill in the basic task information.  */
        task -> tc_entry =                  task_entry;
        task -> tc_argc =                   argc;
        task -> tc_argv =                   argv;
        task -> tc_status =                 NU_PURE_SUSPEND;
        task -> tc_time_slice =             time_slice;
        task -> tc_cur_time_slice =         time_slice;

        /* Setup task's preemption posture.  */
        if (preempt == NU_PREEMPT)
        {
            task -> tc_preemption =  NU_TRUE;
        }

        /* Fill in information about the task's stack.  */
        task -> tc_stack_start =            stack_address;
        task -> tc_stack_size =             stack_size;

        /* Setup priority information for the task.  There are two bit maps
           associated with each task.  The first bit map indicates which group
           of 8-priorities it is.  The second bit map indicates the actual
           priority within the group.  */
        task -> tc_priority =               priority;
        task -> tc_priority_head =          &(TCD_Priority_List[priority]);
        task -> tc_sub_priority =           (DATA_ELEMENT) (1 << (priority & 7));
        priority =                          (OPTION)(priority >> 3);
        task -> tc_priority_group =         ((UNSIGNED) 1) << priority;
        task -> tc_sub_priority_ptr =       &(TCD_Sub_Priority_Groups[priority]);

#if (NU_STACK_FILL == NU_TRUE)

        /* Fill stack with pattern */
        ESAL_GE_MEM_Set(task -> tc_stack_start, NU_STACK_FILL_PATTERN, task -> tc_stack_size);

#endif  /* NU_STACK_FILL == NU_TRUE */

        /* Calculate stack end address */
        task -> tc_stack_end = (VOID *)((VOID_CAST)task -> tc_stack_start +
                                                   task -> tc_stack_size);

        /* Align the stack end address as required */
        task -> tc_stack_end = ESAL_GE_STK_ALIGN(task -> tc_stack_end);

        /* Populate unsolicited stack frame */
        task -> tc_stack_pointer = ESAL_GE_STK_Unsolicited_Set(task -> tc_stack_start,
                                                               task -> tc_stack_end,
                                                               TCC_Task_Shell);
#if (NU_STACK_CHECKING == NU_TRUE)

        /* Save the minimum amount of remaining stack memory remaining
           on stack */
        task -> tc_stack_minimum =  (UNSIGNED)((VOID_CAST)task -> tc_stack_pointer -
                                               (VOID_CAST)task -> tc_stack_start);

#endif  /* NU_STACK_CHECKING == NU_TRUE */

        /* Initialize the task timer.  */
        TMC_Init_Task_Timer(&(task -> tc_timer_control), (VOID *) task);

       /* Set task base priority for new PI Semaphore feature */
        task -> tc_base_priority =          task -> tc_priority;

        /* Protect the list of created tasks.  */
        TCCT_Schedule_Lock();

        /* At this point the task is completely built.  The ID can now be
           set and it can be linked into the created task list.  */
        task -> tc_id =                     TC_TASK_ID;

        /* Link the task into the list of created tasks and increment the
           total number of tasks in the system.  */
        NU_Place_On_List(&TCD_Created_Tasks_List, &(task -> tc_created));
        TCD_Total_Tasks++;

        /* Set the group flag based on the parent task */
        if ((TCD_Current_Thread != NU_NULL) && (((TC_TCB *)TCD_Current_Thread)->tc_grp_id == TC_GRP_ID_APP))
        {
            /* Add this task to the list of application tasks */
            TCC_Application_Task_Add(task);
        }

        /* Trace log */
        T_TASK_CREATED((VOID*)task, (VOID*)task_entry, (VOID*)stack_address, name, stack_size,
        time_slice, task -> tc_priority, preempt, auto_start, status);

        /* Release the protection.  */
        TCCT_Schedule_Unlock();

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
                
        /* Bind the task to the current process */
        status = PROC_Bind_Task(task);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

        /* Determine if the task should be automatically started.  */
        if (auto_start == NU_START)
        {
            /* Protect the system data structures.  */
            TCCT_Schedule_Lock();

            /* Start the task by resuming it.  If the preemption is required,
               leave the current task.  */
            if (TCC_Resume_Task(task, NU_PURE_SUSPEND))
            {
                /* Trace log */
                T_TASK_SUSPEND((VOID*)TCCT_Current_Thread(), NU_PURE_SUSPEND);

                /* Transfer control back to the system.  */
                TCCT_Control_To_System();
            }

            /* Release the protection.  */
            TCCT_Schedule_Unlock();
        }
        else
        {
            /* Trace log */
            T_TASK_SUSPEND((VOID*)task, NU_PURE_SUSPEND);
        }

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_TASK_CREATED((VOID*)task, (VOID*)task_entry, (VOID*)stack_address, name, stack_size,
        time_slice, priority, preempt, auto_start, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_HISR
*
*   DESCRIPTION
*
*       This function creates a High-Level Interrupt Service Routine
*       (HISR) and then places it on the list of created HISRs.  All
*       the resources necessary to create the HISR are supplied to this
*       routine.  HISRs are always created in a dormant state.
*
*   CALLED BY
*
*       Application
*       TMIT_Initialize                     Initializes the data structures
*                                           that control the operation of
*                                           the timer component (TM).
*
*   CALLS
*
*       NU_Place_On_List                    Add TCB to linked-list
*       ESAL_GE_STK_Solicited_Set           Populates solicited stackframe
*       [PROC_Bind_HISR]                    Processes
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created HISR list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       hisr_ptr                            HISR control block pointer
*       name                                HISR name
*       hisr_entry                          Entry function of the HISR
*       priority                            Task priority
*       stack_address                       Pointer to start of stack
*       stack_size                          Size of HISR stack in bytes
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Successful request
*           NU_INVALID_HISR                 Invalid HISR pointer
*           NU_INVALID_ENTRY                Invalid HISR entry point
*           NU_INVALID_PRIORITY             Invalid HISR priority
*           NU_INVALID_MEMORY               Indicates stack pointer NULL
*           NU_INVALID_SIZE                 Indicates stack size is too
*                                           small
*
***********************************************************************/
STATUS NU_Create_HISR(NU_HISR *hisr, CHAR *name,
                      VOID (*hisr_entry)(VOID), OPTION priority,
                      VOID *stack_address, UNSIGNED stack_size)
{
    STATUS          status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Check each parameter.  */

    /* Invalid HISR control block pointer.  */
    NU_ERROR_CHECK(((hisr == NU_NULL) || (hisr -> tc_id == TC_HISR_ID)), status, NU_INVALID_HISR);

    /* Invalid HISR entry function pointer.  */
    NU_ERROR_CHECK((hisr_entry == NU_NULL), status, NU_INVALID_ENTRY);

    /* Invalid stack starting address.  */
    NU_ERROR_CHECK((stack_address == NU_NULL), status, NU_INVALID_MEMORY);

    /* Invalid stack size.  */
    NU_ERROR_CHECK((stack_size < NU_MIN_STACK_SIZE), status, NU_INVALID_SIZE);

    /* Invalid HISR priority.  */
    NU_ERROR_CHECK((((INT) priority) >= TC_HISR_PRIORITIES), status, NU_INVALID_PRIORITY);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Clear the control block */
        CSC_Clear_CB(hisr, NU_HISR);

        /* Fill in the HISR name. */
        strncpy(hisr -> tc_name, name, (NU_MAX_NAME - 1));

        /* Fill in the basic HISR information.  */
        hisr -> tc_entry =                  hisr_entry;

        /* Fill in information about the HISR's stack.  */
        hisr -> tc_stack_start =            stack_address;
        hisr -> tc_stack_size =             stack_size;

        /* Setup priority information for the HISR.  Priorities range from 0 to
           TC_HISR_PRIORITIES - 1.  */
        hisr -> tc_priority =               priority;

#ifdef  CFG_NU_OS_KERN_PROCESS_CORE_ENABLE

        /* Bind all HISRs to a process */
        (VOID)PROC_Bind_HISR(hisr);

#endif  /* CFG_NU_OS_KERN_PROCESS_CORE_ENABLE */

#if (NU_STACK_FILL == NU_TRUE)

        /* Fill stack with pattern */
        ESAL_GE_MEM_Set(hisr -> tc_stack_start, NU_STACK_FILL_PATTERN, hisr -> tc_stack_size);

#endif  /* NU_STACK_FILL == NU_TRUE */

        /* Calculate stack end address */
        hisr -> tc_stack_end = (VOID *)((VOID_CAST)hisr -> tc_stack_start +
                                                   hisr -> tc_stack_size);

        /* Align the stack end address as required */
        hisr -> tc_stack_end = ESAL_GE_STK_ALIGN(hisr -> tc_stack_end);

        /* Populate solicited stack frame */
        hisr -> tc_stack_pointer = ESAL_GE_STK_Solicited_Set(hisr -> tc_stack_start,
                                                             hisr -> tc_stack_end,
                                                             TCCT_HISR_Shell);

#if (NU_STACK_CHECKING == NU_TRUE)

        /* Save the minimum amount of remaining stack memory remaining
           on stack */
        hisr -> tc_stack_minimum =  (UNSIGNED)((VOID_CAST)hisr -> tc_stack_pointer -
                                               (VOID_CAST)hisr -> tc_stack_start);

#endif  /* NU_STACK_CHECKING == NU_TRUE */


        /* Protect the list of created HISRs.  */
        TCCT_Schedule_Lock();

        /* At this point the HISR is completely built.  The ID can now be
           set and it can be linked into the created HISR list.  */
        hisr -> tc_id =                     TC_HISR_ID;

        /* Link the HISR into the list of created HISRs and increment the
           total number of HISRs in the system.  */
        NU_Place_On_List(&TCD_Created_HISRs_List, &(hisr -> tc_created));
        TCD_Total_HISRs++;

        /* Get the group flag from the parent task */
        hisr->tc_grp_id = TC_GRP_ID_SYS;

        /* Release the protection.  */
        TCCT_Schedule_Unlock();

        /* Trace log */
        T_HISR_CREATED((VOID*)hisr, (VOID*)hisr_entry, (VOID*)stack_address,
        name, stack_size, priority, status);

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_HISR_CREATED((VOID*)hisr, (VOID*)hisr_entry, (VOID*)stack_address,
        name, stack_size, priority, status);
    }

    /* Return successful completion.  */
    return(status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Create_Auto_Clean_Task
*
*   DESCRIPTION
*
*       This function creates a task that upon completion will free
*       all memory resources and remove itself from the system.
*
*   CALLS
*
*       NU_Allocate_Memory
*       NU_Deallocate_Memory
*       NU_Create_Task
*       memset
*       NU_Resume_Task
*
*   INPUTS
*
*       task_ptr                            Return pointer
*       name                                Task name
*       task_entry                          Entry function of the task
*       argc                                Optional task parameter
*       argv                                Optional task parameter
*       pool_ptr                            Pool to use for allocations
*       stack_size                          Size of task stack in bytes
*       priority                            Task priority
*       time_slice                          Task time slice
*       preempt                             Task preemptability flag
*       auto_start                          Automatic task start
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*
***********************************************************************/
STATUS NU_Create_Auto_Clean_Task(NU_TASK **task_ptr, CHAR *name,
                                 VOID (*task_entry)(UNSIGNED, VOID *),
                                 UNSIGNED argc, VOID *argv,
                                 NU_MEMORY_POOL *pool_ptr,
                                 UNSIGNED stack_size,
                                 OPTION priority, UNSIGNED time_slice,
                                 OPTION preempt, OPTION auto_start)
{
    STATUS   status = NU_SUCCESS;
    VOID    *stack;
    NU_TASK *task;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Allocate memory to be used for both the task control block
       and the stack */
    status = NU_Allocate_Memory(pool_ptr, (VOID *)&task,
                                (stack_size + sizeof(NU_TASK)),
                                NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Verify the auto start option is valid. Before using
           NU_NO_START in NU_Create_Task */
        if ((auto_start != NU_START) && (auto_start != NU_NO_START))
        {
            /* Invalid start selection. */
            status =  NU_INVALID_START;
        }
        else
        {
            /* Clear the memory to be utilized by the control block */
            (VOID)memset(task, 0, sizeof(NU_TASK));

            /* Get the stack base */
            stack = (VOID *)((UNSIGNED)task + sizeof(NU_TASK));

            /* Create the task with NU_NO_START regardless of
            the auto start settings */
            status = NU_Create_Task(task, name, task_entry, argc,
                                    argv, stack,
                                    stack_size, priority, time_slice, preempt,
                                    NU_NO_START);
        }

        if (status == NU_SUCCESS)
        {
            /* Set the auto clean flag in the control block */
            task -> tc_auto_clean = NU_TRUE;

            /* If a valid task pointer is available return the
               task control block pointer */
            if (task_ptr != NU_NULL)
            {
                *task_ptr = task;
            }

            /* Start the task if needed */
            if (auto_start == NU_START)
            {
                (VOID)NU_Resume_Task(task);
            }

        }
        else
        {
            /* Create task failed, free the memory */
            (VOID)NU_Deallocate_Memory((VOID *)task);
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    /* Return successful completion.  */
    return(status);
}

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
*   FILE NAME
*
*       pms_init.c
*
*   COMPONENT
*
*       PMS - Power Management Services
*
*   DESCRIPTION
*
*       This file contains the required functions for PMS initialization.
*
*   DATA STRUCTURES
*
*       PMS_Initialization_Status
*       PMS_Cleanup_Task
*       PMS_Cleanup_Queue
*
*   FUNCTIONS
*
*       NU_PM_Initialize
*       PMS_Initialization_Status_Check
*       PMS_Component_Initialize
*       PMS_Create_Initialization_Task
*       PMS_Create_Cleanup_Task
*       PMS_Create_Cleanup_Queue
*       PMS_Init_Task_Entry
*       PMS_Cleanup_Task_Entry
*       nu_os_svcs_pwrcore_init
*
*   DEPENDENCIES
*
*       power_core.h
*       initialization.h
*       reg_api.h
*       string.h
*
*************************************************************************/

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/initialization.h"
#include "services/reg_api.h"
#include <string.h>
#include "services/nu_trace_os_mark.h"

/* EQM Instance. */
EQM_EVENT_QUEUE System_Eqm;

/* Queue size is number of events that can be stored in the EQM. */
#define PM_QUEUE_SIZE          10

/* It defines the maximum size of event data of any event type. */
#define PM_EVENT_DATA_SIZE     8

/* Local copy of initialization status */
static STATUS PMS_Initialization_Status = PM_NOT_INITIALIZED;

/* Cleanup task and queue control blocks */
static NU_TASK   PMS_Cleanup_Task;
static NU_QUEUE  PMS_Cleanup_Queue;

/* Memory pool */
extern NU_MEMORY_POOL  System_Memory;

/* Initialization function prototypes for error handling */
STATUS PMS_Error_Initialize(NU_MEMORY_POOL* mem_pool);
VOID PMS_Error_Entry(NU_MEMORY_POOL *mem_pool_ptr);

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
/* Initialization function prototypes for DVFS */
STATUS PMS_DVFS_Pre_Initialize(NU_MEMORY_POOL* mem_pool_ptr);
VOID PMS_DVFS_Post_Initialize(NU_MEMORY_POOL* mem_pool_ptr);
#endif

#if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE))
/* Initialization function prototypes for peripherals */
VOID PMS_Peripheral_Initialize(NU_MEMORY_POOL* mem_pool);
#endif

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE)
extern BOOLEAN PMS_TS_Enabled_Flag;
/* Initialization function prototypes for CPU */
VOID PMS_CPU_Post_Sched_Initialize(NU_MEMORY_POOL* mem_pool);
STATUS PMS_CPU_Presched_Initialize(NU_MEMORY_POOL* mem_pool);
#endif

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
/* Initialization function prototypes for Watchdog */
VOID PMS_Watchdog_Initialize(NU_MEMORY_POOL* mem_pool);
#endif


#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE)
/* Initialization function prototypes for self-refresh */
VOID PMS_SelfRefresh_Initialize(NU_MEMORY_POOL* mem_pool);
#endif

/* Local prototypes */
static VOID PMS_Init_Task_Entry(UNSIGNED argc, VOID *argv);
static VOID PMS_Cleanup_Task_Entry(UNSIGNED argc, VOID *argv);
static STATUS PMS_Component_Initialize(NU_MEMORY_POOL *mem_pool_ptr,
                                          STATUS (*pm_pre_schedule_ptr)(NU_MEMORY_POOL *),
                                          VOID (*pm_post_schedule_ptr)(NU_MEMORY_POOL *),
                                          CHAR *name);
static STATUS PMS_Create_Initialization_Task(NU_MEMORY_POOL *mem_pool_ptr,
                                                VOID (*pm_post_schedule_ptr)(NU_MEMORY_POOL *),
                                                CHAR *name);
static STATUS PMS_Create_Cleanup_Task(NU_MEMORY_POOL *mem_pool_ptr);
static STATUS PMS_Create_Cleanup_Queue(NU_MEMORY_POOL *mem_pool_ptr);

/***********************************************************************
*
*   FUNCTION
*
*       NU_PM_Initialize
*
*   DESCRIPTION
*
*       This function is the entry point for Power Management Services.
*
*   INPUTS
*
*       mem_pool                            Pointer to memory
*                                           pool (cached or uncached)
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID NU_PM_Initialize(NU_MEMORY_POOL* mem_pool)
{
    STATUS          pm_status = PM_NOT_INITIALIZED;
    STATUS          eqm_status;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Create cleanup queue */
    status = PMS_Create_Cleanup_Queue(mem_pool);

    /* Create cleanup task */
    if (status == NU_SUCCESS)
    {
        status = PMS_Create_Cleanup_Task(mem_pool);
    }

    /* Initialize the error handler first so that other initialization
       can utilize the functionality */
    if (status == NU_SUCCESS)
    {
        pm_status = PMS_Component_Initialize(mem_pool, PMS_Error_Initialize, PMS_Error_Entry, "PMError");

    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE)
        /* Call the initialization for CPU idle */
        if (pm_status == NU_SUCCESS)
        {                      
            pm_status = PMS_Component_Initialize(mem_pool, PMS_CPU_Presched_Initialize, PMS_CPU_Post_Sched_Initialize, "CPU");
        }
        /* Trace log */
        T_TICK_SUPPRESS(PMS_TS_Enabled_Flag);
    #endif

    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)
        /* Call the initialization for DVFS */
        if (pm_status == NU_SUCCESS)
        {
            pm_status = PMS_Component_Initialize(mem_pool, PMS_DVFS_Pre_Initialize, PMS_DVFS_Post_Initialize, "DVFS");
        }
    #endif

    #if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE))
        /* Call the initialization for Peripherals */
        if (pm_status == NU_SUCCESS)
        {
            pm_status = PMS_Component_Initialize(mem_pool, NU_NULL, PMS_Peripheral_Initialize, "PERIPH");
        }
    #endif

    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
        /* Call the initialization for Watchdog */
        if (pm_status == NU_SUCCESS)
        {
            pm_status = PMS_Component_Initialize(mem_pool, NU_NULL, PMS_Watchdog_Initialize, "WATCHD");
        }
    #endif

    #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SELFREFRESH == NU_TRUE)
        /* Call the initialization for self-refresh */
        if (pm_status == NU_SUCCESS)
        {
            pm_status = PMS_Component_Initialize(mem_pool, NU_NULL, PMS_SelfRefresh_Initialize, "SELFR");
        }
    #endif
    }

    /* Check to see if any initialization has failed. */
    if ((pm_status == NU_SUCCESS) && (status == NU_SUCCESS))
    {
        /* Create Event Queue Manager instance. */
        eqm_status = NU_EQM_Create(&System_Eqm, PM_QUEUE_SIZE, PM_EVENT_DATA_SIZE, mem_pool);

        if (eqm_status == NU_SUCCESS)
        {
            /* Update initialization status to complete */
            PMS_Initialization_Status = NU_SUCCESS;
        }
    }

    /* Return to user mode */
    NU_USER_MODE();
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Initialization_Status_Check
*
*   DESCRIPTION
*
*       This function returns initialization status
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS          Function returns success
*       PM_NOT_INITIALIZED  PMS is not initialized
*
***********************************************************************/
STATUS PMS_Initialization_Status_Check(VOID)
{
    /* Return the initialization status */
    return (PMS_Initialization_Status);
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Component_Initialize
*
*   DESCRIPTION
*
*       Allows for unified way to call initialization.  There is pre
*       and post scheduler initialization available, if either is passed
*       NU_NULL it will be ignored.
*
*   INPUTS
*
*       mem_pool_ptr                        Used for memory allocation
*       pm_pre_schedule_ptr                 Initialization that occurs
*                                           before the scheduler starts
*                                           NOTE: This blocks all other
*                                           initialization.
*       pm_post_schedule_ptr                Initialization that occurs
*                                           after the scheduler starts
*                                           NOTE: NMI initialization
*                                           will be marked as complete
*                                           before this initialization
*                                           runs.
*       name                                Text string used in naming
*                                           post schedule task
*
*   OUTPUTS
*
*       STATUS
*
***********************************************************************/
static STATUS PMS_Component_Initialize(NU_MEMORY_POOL *mem_pool_ptr,
                                          STATUS (*pm_pre_schedule_ptr)(NU_MEMORY_POOL *),
                                          VOID (*pm_post_schedule_ptr)(NU_MEMORY_POOL *),
                                          CHAR *name)
{
    STATUS pm_status = NU_SUCCESS;

    /* Check if there is pre-schedule initialization to be done */
    if (pm_pre_schedule_ptr != NU_NULL)
    {
        /* Call the init function */
        pm_status = (*pm_pre_schedule_ptr)(mem_pool_ptr);
    }

    /* Check if there is schedule time initialization to be done */
    if ((pm_post_schedule_ptr != NU_NULL) && (pm_status == NU_SUCCESS))
    {
        /* For "CPU","DVFS" and "PERIPH" component do not create independent tasks
         * as these involve device discovery through DM interface. Instead of making
         * a task just call their post-schedule function where a callback with
         * respective label is registered with DM. */
        if( !(strncmp(name,"CPU",3))  ||
            !(strncmp(name,"DVFS",4)) ||
            !(strncmp(name,"PERIPH",6)) )
        {
            (VOID)(*pm_post_schedule_ptr)(mem_pool_ptr);
        }
        else
        {
            /* Create an initialization task that will call the post schedule init function */
            pm_status = PMS_Create_Initialization_Task(mem_pool_ptr, pm_post_schedule_ptr, name);
        }
    }

    return (pm_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Create_Initialization_Task
*
*   DESCRIPTION
*
*       Creates the task that will call post scheduler initialization
*       functions.
*
*   INPUTS
*
*       mem_pool_ptr                        Used for memory allocation
*       pm_post_schedule_ptr                Initialization that occurs
*                                           after the scheduler starts
*       name                                Text string used in naming
*                                           post schedule task
*
*   OUTPUTS
*
*       STATUS
*
***********************************************************************/
static STATUS PMS_Create_Initialization_Task(NU_MEMORY_POOL *mem_pool_ptr,
                                                VOID (*pm_post_schedule_ptr)(NU_MEMORY_POOL *),
                                                CHAR *name)
{
    PM_INIT     *init_cb;
    VOID        *stack;
    STATUS      pm_status = NU_SUCCESS;
    STATUS      status = NU_SUCCESS;
    NU_TASK     *task_cb;

    /* Set all allocation pointers to NU_NULL */
    init_cb = NU_NULL;
    stack = NU_NULL;
    task_cb = NU_NULL;

    /* Allocate memory for initialization control block */
    status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&init_cb, sizeof(PM_INIT), NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Clear contents of init control block */
        memset(init_cb, 0, sizeof(PM_INIT));

        /* Allocate memory for task stack */
        status = NU_Allocate_Memory(mem_pool_ptr, &stack, PMS_INIT_TASK_STACK_SIZE, NU_NO_SUSPEND);

        /* Check to see if previous operation successful */
        if (status == NU_SUCCESS)
        {
            /* Clear contents of task stack */
            memset(stack, 0, PMS_INIT_TASK_STACK_SIZE);

            /* Allocate memory for task control block */
            status = NU_Allocate_Memory(mem_pool_ptr, (VOID *)&task_cb, sizeof(NU_TASK), NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Clear contents task control block */
                memset(task_cb, 0, sizeof(NU_TASK));

                /* Setup passed in parameters */
                init_cb -> pm_mem_pool = mem_pool_ptr;
                init_cb -> pm_task_entry = pm_post_schedule_ptr;
                init_cb -> pm_stack = stack;
                init_cb -> pm_task = task_cb;

                /* Create init task, pass the init control block to the task in argv */
                status = NU_Create_Task(task_cb, name, PMS_Init_Task_Entry, 1, init_cb, stack,
                                        PMS_INIT_TASK_STACK_SIZE, PMS_INIT_TASK_PRIORITY,
                                        PMS_INIT_TASK_TIME_SLICE, NU_PREEMPT, NU_START);

                if (status == NU_SUCCESS)
                {
                    /* Bind this task to the kernel module */
                    status = NU_BIND_TASK_TO_KERNEL(task_cb);
                }
            }
        }
    }

    /* If there was a failure free any allocated memory */
    if (status != NU_SUCCESS)
    {
        /* Check to see if the init control block was allocated */
        if (init_cb != NU_NULL)
        {
            /* Deallocate the memory */
            (VOID)NU_Deallocate_Memory(init_cb);
        }

        /* Check to see if the task stack was allocated */
        if (stack != NU_NULL)
        {
            /* Deallocate the memory */
            (VOID)NU_Deallocate_Memory(stack);
        }

        /* Check to see if the task control block was allocated */
        if (task_cb != NU_NULL)
        {
            /* Deallocate the memory */
            (VOID)NU_Deallocate_Memory(task_cb);
        }
    }

    return (pm_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Create_Cleanup_Task
*
*   DESCRIPTION
*
*       Creates the task that will cleanup completed post scheduler
*       initialization tasks.
*
*   INPUTS
*
*       mem_pool_ptr                        Used for memory allocation
*
*   OUTPUTS
*
*       Return values for NU_Allocate_Memory and NU_Create_Task
*
***********************************************************************/
static STATUS PMS_Create_Cleanup_Task(NU_MEMORY_POOL *mem_pool_ptr)
{
    VOID   *stack;
    STATUS  status;

    /* Allocate memory for clean up task stack */
    status = NU_Allocate_Memory(mem_pool_ptr, &stack, PMS_INIT_TASK_STACK_SIZE, NU_NO_SUSPEND);

    /* Check to see if previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Clear contents of task stack */
        memset(stack, 0, PMS_INIT_TASK_STACK_SIZE);

        /* Create cleanup task */
        status = NU_Create_Task(&PMS_Cleanup_Task, "PMClean", PMS_Cleanup_Task_Entry,
                                0, NU_NULL, stack, PMS_INIT_TASK_STACK_SIZE,
                                PMS_INIT_TASK_PRIORITY, PMS_INIT_TASK_TIME_SLICE,
                                NU_PREEMPT, NU_START);

        if (status == NU_SUCCESS)
        {
            /* Bind this task to the kernel module */
            status = NU_BIND_TASK_TO_KERNEL(&PMS_Cleanup_Task);
        }
        else
        {
            /* Deallocate the stack */
            (VOID)NU_Deallocate_Memory(stack);
        }
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Create_Cleanup_Queue
*
*   DESCRIPTION
*
*       Creates the queue that will be used to notify the cleanup task
*       to free the initialization task memory and delete the tasks
*
*   INPUTS
*
*       mem_pool_ptr                        Used for memory allocation
*
*   OUTPUTS
*
*       Return values for NU_Allocate_Memory and NU_Create_Queue
*
***********************************************************************/
static STATUS PMS_Create_Cleanup_Queue(NU_MEMORY_POOL *mem_pool_ptr)
{
    VOID   *queue_memory;
    STATUS  status;

    /* Allocate memory for clean up queue */
    status = NU_Allocate_Memory(mem_pool_ptr, &queue_memory,
                                sizeof(PM_INIT), NU_NO_SUSPEND);

    /* Check to see if previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Clear contents of queue space */
        memset(queue_memory, 0, sizeof(PM_INIT));

        /* Create cleanup queue */
        status = NU_Create_Queue(&PMS_Cleanup_Queue, "PMClean", queue_memory,
                                 sizeof(PM_INIT)/sizeof(UNSIGNED), NU_FIXED_SIZE,
                                 sizeof(PM_INIT)/sizeof(UNSIGNED), NU_FIFO);
        if (status != NU_SUCCESS)
        {
            /* Deallocate the queue memory */
            (VOID)NU_Deallocate_Memory(queue_memory);
        }
    }

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Init_Task_Entry
*
*   DESCRIPTION
*
*       Calls the entry point of the initialization tasks and if the
*       task completes notifies the cleanup task.
*
*   INPUTS
*
*       argc                                unsigned value set at
*                                           task creation
*       argv                                void pointer set at task
*                                           creation
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID PMS_Init_Task_Entry(UNSIGNED argc, VOID *argv)
{
    PM_INIT *init_cb;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    NU_UNUSED_PARAM(argc);

    init_cb = (PM_INIT *)argv;

    /* Call the initialization function */
    init_cb -> pm_task_entry(init_cb -> pm_mem_pool);

    /* Notify a cleanup task, note it is possible that this isn't
       reached for all init tasks, some are intended to run indefinitely */
    (VOID)NU_Send_To_Queue(&PMS_Cleanup_Queue, init_cb,
                           sizeof(PM_INIT)/sizeof(UNSIGNED), NU_SUSPEND);

    /* Return to user mode */
    NU_USER_MODE();
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Cleanup_Task_Entry
*
*   DESCRIPTION
*
*       Waits on notification from initialization tasks and frees any
*       allocated memory and deletes the tasks.
*
*   INPUTS
*
*       argc                                unsigned value set at
*                                           task creation
*       argv                                void pointer set at task
*                                           creation
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID PMS_Cleanup_Task_Entry(UNSIGNED argc, VOID *argv)
{
    PM_INIT init_cb;
    UNSIGNED    size = 0;
    STATUS      status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);

    for (;;)
    {

        /* Wait for tasks to be added to the queue */
        status = NU_Receive_From_Queue(&PMS_Cleanup_Queue, &init_cb,
                                       sizeof(PM_INIT)/sizeof(UNSIGNED), &size,
                                       NU_SUSPEND);

        if ((status == NU_SUCCESS) && (size == sizeof(PM_INIT)/sizeof(UNSIGNED)))
        {
            /* The task may still be in a state of sending the
               cleanup message, wait until it has finished */
            while (init_cb.pm_task -> tc_status != NU_FINISHED)
            {
                NU_Sleep(1);
            }

            /* Delete the task */
            (VOID)NU_Delete_Task(init_cb.pm_task);

            /* Deallocate the stack */
            (VOID)NU_Deallocate_Memory(init_cb.pm_stack);

            /* Deallocate the control block */
            (VOID)NU_Deallocate_Memory(init_cb.pm_task);
        }
    }

    /* No need to return to user mode because of the infinite loop.    */
    /* Returning to user mode will cause warnings with some compilers. */
}

/***********************************************************************
*
*   FUNCTION
*
*       nu_os_svcs_pwrcore_init
*
*   DESCRIPTION
*
*       Entry point from the kernel to initialize power services.
*
*   INPUTS
*
*       key
*       startstop
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_os_svcs_pwr_core_init(const CHAR *key, INT startstop)
{
	if (startstop == RUNLEVEL_START)
	{
		/* Call the initialization function for power */
		NU_PM_Initialize(&System_Memory);
	}
}


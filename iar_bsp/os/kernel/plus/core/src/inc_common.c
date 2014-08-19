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
*       inc_common.c
*
*   COMPONENT
*
*       IN - Initialization
*
*   DESCRIPTION
*
*       This file contains initialization and setup routines associated
*       with the initialization component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       INC_Initialize                      Common system initialization
*       [INC_Complete]                      LV completion function
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       timer.h                             Timer functions
*       initialization.h                    Initialization functions
*       error_management.h                  Error management functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "os/kernel/plus/core/inc/initialization.h"
#include        "os/kernel/plus/supplement/inc/error_management.h"
#include        "os/services/init/inc/runlevel.h"
#include        "os/kernel/plus/core/inc/esal.h"
#include        "services/nu_trace_os_mark.h"

/* Release information function */
VOID    RLC_Initialize(VOID);

/* Define global variable that contains the state of initialization.  This
   flag is for information use only.  */
INT             INC_Initialize_State;

/* Define system memory pools

   NOTE:  Users are discouraged from relying referencing the global variables
          for these pools.  Future releases of Nucleus PLUS may change the
          use and / or names of these pools.  Applications should only use
          the pool pointer arguments passed to Pre_Kernel_Init_Hook(). */
NU_MEMORY_POOL  System_Memory;
#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))
NU_MEMORY_POOL  Uncached_System_Memory;
#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

#if (NU_PLUS_OBJECT_LISTS_INCLUDE == NU_TRUE)

/* Nucleus PLUS created lists for each of the
   Nucleus PLUS objects.  These lists may be
   required for kernel awareness support within
   certain third party debuggers. */

extern  CS_NODE *TCD_Created_HISRs_List;
extern  CS_NODE *TMD_Created_Timers_List;
extern  CS_NODE *TCD_Created_Tasks_List;
extern  CS_NODE *SMD_Created_Semaphores_List;
extern  CS_NODE *MBD_Created_Mailboxes_List;
extern  CS_NODE *PID_Created_Pipes_List;
extern  CS_NODE *QUD_Created_Queues_List;
extern  CS_NODE *EVD_Created_Event_Groups_List;
extern  CS_NODE *PMD_Created_Pools_List;
extern  CS_NODE *DMD_Created_Pools_List;

#endif  /* (NU_PLUS_OBJECT_LISTS_INCLUDE == NU_TRUE) */

#if (CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0)

/* Local Function Prototypes */
static  VOID        INC_Complete(UNSIGNED id);

/* Timer related variables */
extern  CS_NODE     *TMD_Created_Timers_List;
extern  UNSIGNED    TMD_Total_Timers;

#endif  /* CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0 */


/***********************************************************************
*
*   FUNCTION
*
*       INC_Initialize
*
*   DESCRIPTION
*
*       This function is the main initialization function of the system.
*       All components are initialized by this function.  After system
*       initialization is complete, the Pre_Kernel_Init_Hook routine
*       is called.  After all initialization is complete, this function
*       calls TCCT_Schedule to start scheduling tasks.
*
*   CALLED BY
*
*       INCT_Initialize                     Target dependent initialize
*
*   CALLS
*
*       Pre_Kernel_Init_Hook                Application initialize
*       [PROC_Kernel_Initialize]            Kernel Process Initialization
*       RLC_Initialize                      Release initialize
*       TCCT_Schedule                       Thread scheduling loop
*       TMIT_Initialize                     Timer Module Initialize
*
*   INPUTS
*
*       first_available_memory              Pointer to available memory
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  INC_Initialize(VOID *first_available_memory)
{

#if (NU_PLUS_OBJECT_LISTS_INCLUDE == NU_TRUE)

    /* Ensure all Nucleus PLUS object lists
       get included within the executable image */

    TCD_Created_HISRs_List =        NU_NULL;
    TMD_Created_Timers_List =       NU_NULL;
    TCD_Created_Tasks_List =        NU_NULL;
    SMD_Created_Semaphores_List =   NU_NULL;
    MBD_Created_Mailboxes_List =    NU_NULL;
    PID_Created_Pipes_List =        NU_NULL;
    QUD_Created_Queues_List =       NU_NULL;
    EVD_Created_Event_Groups_List = NU_NULL;
    PMD_Created_Pools_List =        NU_NULL;
    DMD_Created_Pools_List =        NU_NULL;

#endif  /* (NU_PLUS_OBJECT_LISTS_INCLUDE == NU_TRUE) */

#ifdef CFG_NU_OS_KERN_PLUS_SUPPLEMENT_ENABLE
    /* Initialize Release Information */
    RLC_Initialize();
#endif

#ifdef CFG_NU_OS_KERN_PROCESS_CORE_ENABLE
    /* Initialize Kernel Process */
    PROC_Kernel_Initialize();
#endif

#if (CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0)

    /* Check if first available memory pointer is aligned on an acceptable boundary */
    if (ESAL_GE_MEM_ALIGNED_CHECK(first_available_memory, sizeof(VOID *)) == NU_FALSE)
    {
        /* Align the first available memory pointer on an acceptable boundary */
        first_available_memory = ESAL_GE_MEM_PTR_ALIGN(first_available_memory, sizeof(VOID *));
    }

    /* Clear the block of memory for the timer control block */
    ESAL_GE_MEM_Clear(first_available_memory,sizeof(TM_APP_TCB));

    /* Create an application timer that, upon expiration, terminates the system. */
    NU_Create_Timer((TM_APP_TCB *)first_available_memory,
                    "TM_HISR", INC_Complete,
                    (UNSIGNED)first_available_memory,
                    (UNSIGNED)(NU_PLUS_TICKS_PER_SEC * 60 * CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT),
                    0, NU_ENABLE_TIMER);

    /* Set bit 0 in the timer expiration id to prevent future API calls to NU_Control_Timer() 
       from controlling/stopping the LV timer.  This MUST happen after the call to NU_Create_Timer() */
    ((TM_APP_TCB *)first_available_memory)-> tm_expiration_id |= 0x1;

    /* Remove the timer from the list of created timers to hide its creation.  */
    NU_Remove_From_List(&TMD_Created_Timers_List,
                        &(((TM_APP_TCB *)first_available_memory)-> tm_created));

    /* Decrement the total number of created timers.  */
    TMD_Total_Timers--;

    /* Adjust first available memory for timer control block */
    first_available_memory = (VOID *)((BYTE_PTR)first_available_memory + sizeof(TM_APP_TCB));

#endif  /* CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0 */

    /* Initialization System Memory Pools */
    INCT_Sys_Mem_Pools_Initialize(first_available_memory);

#if ((CFG_NU_OS_SVCS_TRACE_CORE_TRACE_SUPPORT == NU_TRUE) && (CFG_NU_OS_SVCS_TRACE_CORE_TRACE_KERNEL_BOOT_TIME == NU_TRUE))

    /* Initialize Nucleus Trace */
    NU_Trace_Initialize();

#endif /* ((CFG_NU_OS_SVCS_TRACE_CORE_TRACE_SUPPORT == NU_TRUE) && (CFG_NU_OS_SVCS_TRACE_CORE_TRACE_KERNEL_BOOT_TIME == NU_TRUE)) */

#ifdef CFG_NU_OS_SVCS_POSIX_CORE_ENABLE

    #include        "services/psx_extr.h"

    /* Call POSIX early init function */
    if (nu_os_svcs_posix_core_init_early() != NU_SUCCESS)
    {
        /* Call the system error handler */
        ERC_System_Error(NU_UNHANDLED_EXCEPTION);
    }
#endif  /* CFG_NU_OS_SVCS_POSIX_CORE_ENABLE */

    /* Initialize the Timer component. */
    TMIT_Initialize();

    /* Perform any additional toolset specific RTL initialization */
    ESAL_GE_RTL_Initialize();    

#ifdef CFG_NU_OS_SVCS_INIT_ENABLE

    /* Call run-level initialization routine */
    if (NU_RunLevel_Init(&System_Memory) != NU_SUCCESS)
    {
        /* Call the system error handler */
        ERC_System_Error(NU_RUNLEVEL_INIT_ERROR);
    }

#endif /* CFG_NU_OS_SVCS_INIT_ENABLE */

    /* Invoke the application-supplied initialization function. */
#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))
    Pre_Kernel_Init_Hook(&System_Memory, &Uncached_System_Memory);
#else
    Pre_Kernel_Init_Hook(&System_Memory, &System_Memory);
#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

    /* Indicate that initialization is finished. */
    INC_Initialize_State =  INC_END_INITIALIZE;

    /* Enable the global interrupt level before
       transferring control to the scheduler. */
    TCD_Interrupt_Level = NU_ENABLE_INTERRUPTS;

    /* Start scheduling threads of execution.
       NOTE:  Control never returns here */
    TCCT_Schedule();
}



#if (CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0)
/***********************************************************************
*
*   FUNCTION
*
*       INC_Complete
*
*   DESCRIPTION
*
*       This function terminates the system
*
*   CALLED BY
*
*       Timer expiration
*
*   CALLS
*
*       NU_Control_Interrupts               Enable / Disable interrupts
*       NU_Task_Pointers                    Get pointer to a TCB
*       NU_Terminate_Task                   Terminates a task
*       NU_Delete_Task                      Deletes a task
*       NU_HISR_Pointers                    Get pointer to a HCB
*       NU_Delete_HISR                      Deletes a HISR
*       NU_Register_LISR                    Registers LISRs
*
*   INPUTS
*
*       id                                  ID of timer expiring
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID INC_Complete(UNSIGNED id)
{
    TC_TCB      *task_ptr;
    TC_HCB      *hisr_ptr;
    UNSIGNED    task_in_system;
    UNSIGNED    hisr_in_system;
    INT         vector_num;
    VOID        (*old_lisr)(INT);


    /* Reference parameters to prevent toolset warnings */
    NU_UNUSED_PARAM(id);

    /* Disable all interrupts */
    NU_Control_Interrupts((INT)NU_DISABLE_INTERRUPTS);

    /* Terminate and delete all tasks */
    do
    {
        /* Get a task pointer */
        task_in_system = NU_Task_Pointers(&task_ptr,1);

        /* Ensure a task is still in the system */
        if (task_in_system)
        {
            /* Terminate the task */
            (VOID)NU_Terminate_Task(task_ptr);

            /* Delete the task */
            (VOID)NU_Delete_Task(task_ptr);
        }

    /* Loop while tasks are in the system */
    } while (task_in_system);

    /* Delete all HISRs */
    do
    {
        /* Get a HISR pointer */
        hisr_in_system = NU_HISR_Pointers(&hisr_ptr,1);

        /* Ensure a HISR is still in the system */
        if (hisr_in_system)
        {
            /* Delete the HISR */
            (VOID)NU_Delete_HISR(hisr_ptr);
        }

    /* Loop while HISRs are in the system */
    } while (hisr_in_system);

    /* De-register all interrupts */
    for (vector_num=0;vector_num <= ESAL_GE_INT_MAX_VECTOR_ID_GET(); vector_num++)
    {
        /* De-register this LISR */
        (VOID)NU_Register_LISR(vector_num,NU_NULL,&old_lisr);
    }
}
#endif  /* CFG_NU_OS_KERN_PLUS_CORE_LV_TIMEOUT > 0 */

/***********************************************************************
*
*   FUNCTION
*
*       Pre_Kernel_Init_Hook
*
*   DESCRIPTION
*
*       This function provides a default implementation of the kernel
*       hook which is executed before the scheduler is started.  Since
*       this function is weakly linked users can override this default
*       implementation.
*
*   INPUTS
*
*       system_memory                       pointer to system memory pool
*       uncached_system_memory              pointer to uncached system
*                                           memory pool
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
ESAL_TS_WEAK_DEF(VOID Pre_Kernel_Init_Hook(NU_MEMORY_POOL* system_memory, NU_MEMORY_POOL *uncached_system_memory))
{
   /* The default does nothing, but since 'Pre_Kernel_Init_Hook' is weakly
      linked the user can provide their own implementation. */

   /* Suppress warnings */
    NU_UNUSED_PARAM(system_memory);
    NU_UNUSED_PARAM(uncached_system_memory);
}


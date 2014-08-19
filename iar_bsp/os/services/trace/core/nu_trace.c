/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       nu_trace.c
*
*   COMPONENT
*
*       Nucleus Trace Service
*
*   DESCRIPTION
*
*       This module implements the Nucleus Trace Service user APIs
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_trace.h"
#include    "os/services/trace/core/trace.h"
#include    "services/nu_trace_os_mark.h"

#ifdef CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE

#include    "os/services/trace/comms/trace_comms.h"

/* Macros */
#define MIN(a,b)     (a<b)?a:b
STATUS               NU_Trace_Comms_Initialize(VOID);

#if((CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE))

/* Prototypes */
static VOID          Trace_Comms_Task_Entry(UNSIGNED argc, VOID *argv);

#endif /* ((CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE)) */
#endif /* CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE */

#ifdef CFG_NU_OS_SVCS_SHELL_ENABLE

#include    "services/shell_extern.h"

STATUS Trace_Flush(NU_SHELL * shell, INT x,CHAR **argv);
STATUS Trace_Info(NU_SHELL * shell, INT x,CHAR **argv);

#endif /* CFG_NU_OS_SVCS_SHELL_ENABLE */

/* Export appropriate trace symbols for use by processes. */
#if (defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_SVCS_TRACE_CORE_EXPORT_SYMBOLS == NU_TRUE))

NU_EXPORT_SYMBOL(NU_Trace_Arm);
NU_EXPORT_SYMBOL(NU_Trace_Disarm);
NU_EXPORT_SYMBOL(NU_Trace_Get_Mask);
NU_EXPORT_SYMBOL(Trace_Mark_I32);
NU_EXPORT_SYMBOL(Trace_Mark_U32);
NU_EXPORT_SYMBOL(Trace_Mark_Float);
NU_EXPORT_SYMBOL(Trace_Mark_String);
NU_EXPORT_SYMBOL(Trace_Mark);
NU_EXPORT_DATA_SYMBOL(Gbl_Trace_Mask);

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE))

NU_EXPORT_SYMBOL(Trace_Comms_Flush);

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != File_INTERFACE)

NU_EXPORT_SYMBOL(Trace_Comms_Start);
NU_EXPORT_SYMBOL(Trace_Comms_Stop);
NU_EXPORT_SYMBOL(Trace_Comms_Transmit_N_Packets);

#endif /*(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != File_INTERFACE)*/

#endif /*(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE))*/

#endif /*(defined(CFG_NU_OS_KERN_PROCESS_CORE_ENABLE) && (CFG_NU_OS_SVCS_TRACE_CORE_EXPORT_SYMBOLS == NU_TRUE))*/

/* Data structures */

/* Structure for holding target specific information. */ 
struct    _nu_trace_target_info_
{
    UINT32 byte_ordering;
    UINT32 clock_frequency;
};

typedef struct _nu_trace_target_info_    NU_TRACE_TARGET_INFO;

struct    _nu_trace_mgmt_
{
    UINT32           trace_init_flag;
    CHAR*            p_trace_buff;
    UINT32           trace_buff_size;
    CHAR*            p_scratch_buff;
    UINT32           scratch_buff_size;
    NU_TRACE_TARGET_INFO   target_info;
    UINT8            trace_comms_initialized;

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && \
    ((CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE)))
    NU_SEMAPHORE     trace_comms_mutex;
    NU_TASK          trace_comms_task;
    VOID*            trace_comms_task_sp;
    UINT32           trace_comms_tx_period;
    OPTION           trace_comms_tsk_priority;
    UINT8            trace_comms_running;
    UINT8            trace_user_deinit;

#else

    UINT8 pack[3];

#endif
};

typedef    struct    _nu_trace_mgmt_    NU_TRACE_MGMT;

static STATUS          Initialize_Target_Info(NU_TRACE_TARGET_INFO *target_info);

/* Globals variables for trace component */
UINT32  Gbl_Trace_Mask = 0;
static  NU_TRACE_MGMT   Trace_Mgmt;
static  BOOLEAN         User_Deinit = NU_FALSE;

/* External variables */
extern NU_MEMORY_POOL  System_Memory;
#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))
extern NU_MEMORY_POOL  Uncached_System_Memory;
#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

/***********************************************************************
*
*   FUNCTION
*
*       nu_os_svcs_trace_core_init
*
*   DESCRIPTION
*
*       Run level initialization of Nucleus Trace component.
*
*   INPUTS
*
*       CHAR    *key         - Path to registry
*       INT     startstop    - Option to Register or Unregister
*
*   OUTPUTS
*
*       NU_TRUE        Trace component successfully initialized
*       Otherwise      Error
*
***********************************************************************/
STATUS nu_os_svcs_trace_core_init (const CHAR * key, INT startstop)
{
    STATUS status;

    /* Initialize Nucleus trace component */
    status = NU_Trace_Initialize();

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE))

    /* Initialize the communications component */
    if((status == NU_SUCCESS)|| (status == NU_TRACE_ALREADY_INITIALIZED))
    {
        NU_Trace_Comms_Initialize();
    }

#ifdef CFG_NU_OS_SVCS_SHELL_ENABLE

	/* Register trace commands with the Nucleus shell. */
    {
        NU_SHELL *shl_session = NU_NULL;
        /* Get Shell session handle */
        status = NU_Get_Shell_Serial_Session_ID(&shl_session);
        if (status == NU_SUCCESS)
        {
            status = NU_Register_Command(shl_session,"trace_flush",&Trace_Flush);
            if ( status == NU_SUCCESS)
            {
                status = NU_Register_Command(shl_session,"trace_info",&Trace_Info);
            }
        }
    }

#endif /* CFG_NU_OS_SVCS_SHELL_ENABLE */

#endif

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Trace_Initialize
*
*   DESCRIPTION
*
*       Initialize the trace component. This entails;
*       1. Allocate memory for trace buffer
*       2. Allocate memory for scratch buffer
*       3. Initialize buffering system
*
*   INPUTS
*
*        None
*
*   OUTPUTS
*
*       NU_TRUE        Trace component successfully initialized
*       Otherwise      Error
*
*
***********************************************************************/
STATUS  NU_Trace_Initialize(VOID)
{
    STATUS              status = NU_SUCCESS;
    NU_MEMORY_POOL*     sys_pool_ptr;
    static              ESAL_AR_INT_CONTROL_VARS
    UINT32              sys_pool_available_mem;

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))
    UINT32              uncached_sys_pool_available_mem;
#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    if(!Trace_Mgmt.trace_init_flag)
    {
        /* Save initial System Pools trace information */
        sys_pool_available_mem = System_Memory.dm_available;

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))
        uncached_sys_pool_available_mem = Uncached_System_Memory.dm_available;
#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

        /* Initialize trace management data structure */
        memset((CHAR*)&Trace_Mgmt, 0, sizeof(NU_TRACE_MGMT));

        /* Get system memory pool */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        /* Initialize trace and scratch buffer sizes */
        Trace_Mgmt.trace_buff_size =
        CFG_NU_OS_SVCS_TRACE_CORE_TRACE_BUFFER_SIZE + sizeof(LINK_PKT);
        Trace_Mgmt.scratch_buff_size =
        CFG_NU_OS_SVCS_TRACE_CORE_MAX_TRACE_PKT_SIZE;

        /* Allocate memory for trace buffer */
        if (status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory(sys_pool_ptr,
                     (VOID**)&Trace_Mgmt.p_trace_buff,
                     Trace_Mgmt.trace_buff_size, NU_NO_SUSPEND);
        }

        /* Allocate memory for scratch buffer */
        if(status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory(sys_pool_ptr,
                     (VOID**)&Trace_Mgmt.p_scratch_buff,
                     Trace_Mgmt.scratch_buff_size, NU_NO_SUSPEND);
        }

        /* Initialize buffering system */
        if(status == NU_SUCCESS)
        {
            status = Initialize_Trace_Buffer(Trace_Mgmt.p_trace_buff,
                     Trace_Mgmt.trace_buff_size);
        }

        /* Initialize target specifc information. */
        if (status == NU_SUCCESS)
        {
        	status = Initialize_Target_Info(&Trace_Mgmt.target_info);
        }

        if(status == NU_SUCCESS)
        {
            Gbl_Trace_Mask = NU_TRACE_APP;

            /* Enforce default mask */
            Gbl_Trace_Mask |= (CFG_NU_OS_SVCS_TRACE_CORE_KERNEL_DEFAULT_TRACE_MASK & NU_TRACE_KERN_INFO);

            /* Check for middleware support */
#if (CFG_NU_OS_SVCS_TRACE_CORE_PMS_TRACE_SUPPORT == NU_TRUE)
            Gbl_Trace_Mask |= NU_TRACE_PMS_INFO;
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_PMS_TRACE_SUPPORT == NU_TRUE) */

#if (CFG_NU_OS_SVCS_TRACE_CORE_STORAGE_TRACE_SUPPORT == NU_TRUE)
            Gbl_Trace_Mask |= NU_TRACE_STOR_INFO;
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_STORAGE_TRACE_SUPPORT == NU_TRUE) */

#if (CFG_NU_OS_SVCS_TRACE_CORE_NET_TRACE_SUPPORT == NU_TRUE)
            Gbl_Trace_Mask |= NU_TRACE_NET_INFO;
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_NET_TRACE_SUPPORT == NU_TRUE) */

            /* Set trace initialized flag */
            Trace_Mgmt.trace_init_flag = NU_TRUE;
        }

#if (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE)
        Log_U64U64I(TIMER_LOAD_VAL, MAX_TIMER_COUNT, ESAL_GE_TMR_OS_CLOCK_RATE, ESAL_GE_TMR_OS_CLOCK_PRESCALE);
#endif /* CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD  == NU_TRUE */

        /* Log Nucleus cached and uncached memory */
        if (status == NU_SUCCESS) {

            /* Trace log - CPU activity */
            T_IDLE_ENTRY();
            T_IDLE_EXIT();

            /* Trace log - Cached and un-cached memory pool creation */
            T_MEM_POOL_CREATE((VOID*)&System_Memory, System_Memory.dm_start_address, System_Memory.dm_name,
                              System_Memory.dm_pool_size, sys_pool_available_mem, 32, NU_FIFO, NU_SUCCESS);

#if ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE))
            T_MEM_POOL_CREATE((VOID*)&Uncached_System_Memory, Uncached_System_Memory.dm_start_address,
                              Uncached_System_Memory.dm_name, Uncached_System_Memory.dm_pool_size,
                              uncached_sys_pool_available_mem, 32, NU_FIFO, NU_SUCCESS);
#endif /* ((ESAL_PR_CACHE_AVAILABLE == NU_TRUE) || (ESAL_CO_CACHE_AVAILABLE == NU_TRUE)) */

            /* Trace log */
            T_DMEM_ALLOCATE((VOID*)&System_Memory, (VOID*)Trace_Mgmt.p_trace_buff,
                            (VOID*)&NU_Trace_Initialize, (System_Memory.dm_pool_size),
                            (System_Memory.dm_available), Trace_Mgmt.trace_buff_size,
                            NU_NO_SUSPEND, status);

            /* Trace log */
            T_DMEM_ALLOCATE((VOID*)&System_Memory, (VOID*)Trace_Mgmt.p_scratch_buff,
                            (VOID*)&NU_Trace_Initialize, (System_Memory.dm_pool_size),
                            (System_Memory.dm_available), Trace_Mgmt.scratch_buff_size,
                            NU_NO_SUSPEND, status);
        }
    }
    else
    {
        /* Initialization error */
        status = NU_TRACE_ALREADY_INITIALIZED;
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE))

    /* If user de-initialized trace communications, re-initialize it */
    if(User_Deinit == NU_TRUE)
    {
        status = NU_Trace_Comms_Initialize();
    }

#endif

    return(status);
}

/***********************************************************************
*
*   FUNCTION
*
*       Initialize_Target_Info
*
*   DESCRIPTION
*
*       Internal function for initialization of target specific information.
*
*   INPUTS
*
*       NU_TARGET_INFO   Target Info control block to initialize.
*
*   OUTPUTS
*
*       NU_SUCCESS       Always.
*
***********************************************************************/
static STATUS Initialize_Target_Info(NU_TRACE_TARGET_INFO * target_info)
{

	target_info->byte_ordering = ESAL_PR_ENDIANESS;
	target_info->clock_frequency = ESAL_DP_REF_CLOCK_RATE;

	return (NU_SUCCESS);
}

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE))

/***********************************************************************
*
*   FUNCTION
*
*       NU_Trace_Comms_Initialize
*
*   DESCRIPTION
*
*       Internal API for initialization of trace communications
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS       Trace component finalization was successful.
*       Otherwise        error.
*
***********************************************************************/
STATUS  NU_Trace_Comms_Initialize(VOID)
{
    STATUS status = NU_SUCCESS;

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE)

    ESAL_AR_INT_CONTROL_VARS
    NU_MEMORY_POOL*     sys_pool_ptr;

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* If not already initialized initialize trace communications infrastructure */
    if(Trace_Mgmt.trace_comms_initialized == NU_FALSE)
    {
        /* Obtain communications task priority and data transmission frequency */
        Trace_Mgmt.trace_comms_tsk_priority = MIN(CFG_NU_OS_SVCS_TRACE_CORE_COMMS_TASK_PRIORITY,
                                              (CFG_NU_OS_KERN_PLUS_CORE_NUM_TASK_PRIORITIES-1));
        Trace_Mgmt.trace_comms_tx_period = CFG_NU_OS_SVCS_TRACE_CORE_DATA_TX_PERIOD;

        /* Get system memory pool */
        status = NU_System_Memory_Get(&sys_pool_ptr, NU_NULL);

        /* Allocate memory for trace communications task */
        status = NU_Allocate_Memory(sys_pool_ptr, &Trace_Mgmt.trace_comms_task_sp,
                                    TRACE_COMMS_STACK_SIZE, NU_NO_SUSPEND);

        /* Create trace communications task. */
        if (status == NU_SUCCESS)
        {
            status = NU_Create_Task(&Trace_Mgmt.trace_comms_task, "TRACE_COMMS_TASK",
                     Trace_Comms_Task_Entry, 0, NU_NULL, Trace_Mgmt.trace_comms_task_sp,
                     TRACE_COMMS_STACK_SIZE,
                     Trace_Mgmt.trace_comms_tsk_priority,
                     TRACE_COMMS_STACK_TIME_SLICE, NU_PREEMPT, NU_NO_START);
        }

        /* Create a semaphore for trace communications synchronization */
        if(status == NU_SUCCESS)
        {
            status = NU_Create_Semaphore (&Trace_Mgmt.trace_comms_mutex, "TRACE_COMMS",
                     (UNSIGNED)1, (OPTION)NU_PRIORITY_INHERIT);
        }

        /* Open communications interface */
        if(status == NU_SUCCESS)
        {
            status = NU_Trace_Comms_Open();
        }

        /* Set flag to indicate trace communications initialization is complete */
        if(status == NU_SUCCESS)
        {
            Trace_Mgmt.trace_comms_initialized = NU_TRUE;
        }
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

#else

    status = NU_Trace_Comms_Open();

    /* Set flag to indicate trace communications initialization is complete */
    if(status == NU_SUCCESS)
    {
        Trace_Mgmt.trace_comms_initialized = NU_TRUE;
    }

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE) */

    return status;
}

#endif /* CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) */

/***********************************************************************
*
*   FUNCTION
*
*       NU_Trace_Deinitialize
*
*   DESCRIPTION
*
*       User API to finalize NU_TRACE component.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS        Trace component finalization was successful.
*       Otherwise        error.
*
***********************************************************************/
STATUS  NU_Trace_Deinitialize(VOID)
{
    STATUS status = NU_SUCCESS;
    static ESAL_AR_INT_CONTROL_VARS

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    if(Trace_Mgmt.trace_init_flag == NU_TRUE)
    {
        /* Disable all trace hooks */
        Gbl_Trace_Mask = 0;

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && \
    ( (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE)))

        /* Stop trace communications if active */
        if((Trace_Mgmt.trace_comms_initialized == NU_TRUE)&&(Trace_Mgmt.trace_comms_running == NU_TRUE))
        {
            status = NU_Trace_Comms_Stop();
        }

        /* Terminate trace communications task */
        if(status == NU_SUCCESS)
        {
            status = NU_Terminate_Task(&Trace_Mgmt.trace_comms_task);
        }

        /* Delete trace communications task */
        if(status == NU_SUCCESS)
        {
            status = NU_Delete_Task(&Trace_Mgmt.trace_comms_task);
        }

        /* Reclaim trace communications task stack memory */
        if(status == NU_SUCCESS)
        {
            status = NU_Deallocate_Memory(Trace_Mgmt.trace_comms_task_sp);
        }

        /* Delete communications mutex */
        if(status == NU_SUCCESS)
        {
            status = NU_Delete_Semaphore(&Trace_Mgmt.trace_comms_mutex);
        }

        /* Close trace communications interface */
        if(status == NU_SUCCESS)
        {
            status = NU_Trace_Comms_Close();
        }

#endif /* ((CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE)) */

        /* Deallocate memory allocated for trace buffer */
        if(status == NU_SUCCESS)
        {
            status = NU_Deallocate_Memory(Trace_Mgmt.p_trace_buff);
        }

        /* Deallocate memory allocated for scratch buffer */
        if(status == NU_SUCCESS)
        {
            status = NU_Deallocate_Memory(Trace_Mgmt.p_scratch_buff);
        }

        if(status == NU_SUCCESS)
        {
            /* Mark trace service to be un-initialized */
            Trace_Mgmt.trace_init_flag = NU_FALSE;

            /* Set flag to indicate that the user de-initialized  trace */
            User_Deinit = NU_TRUE;
        }
    }
    else
    {
        status = NU_TRACE_NOT_INITIALIZED;
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Trace_Arm
*
*   DESCRIPTION
*
*       User API to arm components of kernel trace infrastructure.
*
*   INPUTS
*
*       UINT32        Mask that indicates the component to be armed.
*
*   OUTPUTS
*
*       NU_SUCCESS    Trace component finalization was successful.
*       Otherwise     error.
*
***********************************************************************/
STATUS  NU_Trace_Arm(UINT32 mask)
{
    STATUS status = NU_SUCCESS;
    static ESAL_AR_INT_CONTROL_VARS

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* If kernel hooks are being enabled ensure kernel trace is enable at boot time as well */
    if((mask & NU_TRACE_KERN) != 0 && (CFG_NU_OS_SVCS_TRACE_CORE_TRACE_SUPPORT == NU_FALSE))
    {
        status = NU_TRACE_KERN_NOT_AVAILABLE;
    }

    /* Update trace mask  to arm tracing of requested component */
    if(status == NU_SUCCESS)
    {
        Gbl_Trace_Mask |= mask;
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Trace_Disarm
*
*   DESCRIPTION
*
*       User API to disarm components of kernel trace infrastructure.
*
*   INPUTS
*
*       UINT32        Mask that indicates the component to be disarmed.
*
*   OUTPUTS
*
*       NU_SUCCESS        Trace component finalization was successful.
*       Otherwise        error.
*
***********************************************************************/
STATUS  NU_Trace_Disarm(UINT32 mask)
{
    STATUS status = NU_SUCCESS;
    static ESAL_AR_INT_CONTROL_VARS

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* If kernel hooks are being enabled ensure kernel trace is enable at boot time as well */
    if((mask & NU_TRACE_KERN) != 0 && (CFG_NU_OS_SVCS_TRACE_CORE_TRACE_SUPPORT == NU_FALSE))
    {
        status = NU_TRACE_KERN_NOT_AVAILABLE;
    }

    /* Update trace mask to disarm tracing of requested component */
    if(status == NU_SUCCESS)
    {
        Gbl_Trace_Mask &= ~mask;
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       NU_Trace_Get_Mask
*
*   DESCRIPTION
*
*       User API to get trace mask
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       UINT32           Current trace mask.
*       Otherwise        error.
*
***********************************************************************/
UINT32  NU_Trace_Get_Mask(VOID)
{
    UINT32 mask;
    static ESAL_AR_INT_CONTROL_VARS

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    mask = Gbl_Trace_Mask;

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

    return (mask);
}

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && \
    ( (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE)))

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Start
*
*   DESCRIPTION
*
*       User API to start trace communications
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS       Trace communications was started.
*       Otherwise        error.
*
***********************************************************************/
STATUS  Trace_Comms_Start(UINT8 comms_tsk_priority, UINT32 comms_tx_period)
{
    STATUS status = NU_SUCCESS;
    static ESAL_AR_INT_CONTROL_VARS

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* It trace is initialized start communications */
    if((Trace_Mgmt.trace_init_flag == NU_TRUE) &&
    (Trace_Mgmt.trace_comms_initialized == NU_TRUE))
    {
        if(Trace_Mgmt.trace_comms_running != NU_TRUE)
        {
            /* Update communications thread priority and tx period */
            Trace_Mgmt.trace_comms_tsk_priority = comms_tsk_priority;
            Trace_Mgmt.trace_comms_tx_period = comms_tx_period;
            NU_Change_Priority(&Trace_Mgmt.trace_comms_task, comms_tsk_priority);

            /* Resume trace communications task */
            status = NU_Resume_Task(&Trace_Mgmt.trace_comms_task);

            /* Set global to indicate trace communications task is running */
            if(status == NU_SUCCESS)
            {
                Trace_Mgmt.trace_comms_running = NU_TRUE;
            }
        }
        else
        {
            status = NU_TRACE_COMMS_ALREADY_STARTED;
        }
    }
    else
    {
        status = NU_TRACE_COMMS_NOT_INITIALIZED;
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Stop
*
*   DESCRIPTION
*
*       User API to stop trace communications
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS       Trace communications stopped.
*       Otherwise        error.
*
***********************************************************************/
STATUS  Trace_Comms_Stop(VOID)
{
    STATUS status;
    static ESAL_AR_INT_CONTROL_VARS

    /* Start critical section */
    ESAL_GE_INT_ALL_DISABLE();

    /* It trace is initialized start communications */
    if((Trace_Mgmt.trace_init_flag == NU_TRUE) &&
    (Trace_Mgmt.trace_comms_initialized == NU_TRUE))
    {
        /* If trace communications has been started, stop it */
        if(Trace_Mgmt.trace_comms_running == NU_TRUE)
        {
            /* Wait for communication mutex to ensure we are not in the
            *  middle of data transmission */
            status = NU_Obtain_Semaphore(&Trace_Mgmt.trace_comms_mutex, 
            NU_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Suspend trace communications  task */
                status = NU_Suspend_Task(&Trace_Mgmt.trace_comms_task);

                /* Clear trace communications running flag */
                if(status == NU_SUCCESS)
                {
                    Trace_Mgmt.trace_comms_running = NU_FALSE;
                }

                /* Release semaphore here */
                NU_Release_Semaphore(&Trace_Mgmt.trace_comms_mutex);
            }
        }
        else
        {
            status = NU_TRACE_COMMS_NOT_STARTED;
        }
    }
    else
    {
        status = NU_TRACE_COMMS_NOT_INITIALIZED;
    }

    /* End critical section */
    ESAL_GE_INT_ALL_RESTORE();

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Flush
*
*   DESCRIPTION
*
*       User API to Flush all available trace data to comms interface
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS       All trace data available has been flushed.
*       Otherwise        error.
*
***********************************************************************/
STATUS  Trace_Comms_Flush(VOID)
{
    STATUS    status;
    UINT32    orig_priority;
    UINT32    orig_trace_mask;
    UINT32    pkts_transmitted;
    NU_TASK*  p_curr_task;
    UINT8     orig_trace_comms_state;

    /* It trace is initialized start communications */
    if(Trace_Mgmt.trace_init_flag == NU_TRUE)
    {
        /* Obtain current state of trace communications thread */
        orig_trace_comms_state = Trace_Mgmt.trace_comms_running;

        /* If trace communications is active, stop it */
        if(orig_trace_comms_state == NU_TRUE)
        {
            NU_Trace_Comms_Stop();
        }

        /* Obtain current trace mask */
        orig_trace_mask = Gbl_Trace_Mask;

        /* Disable all trace instrumentation so we don't produce trace data 
        *  while flushing all logged data from trace buffers */
        Gbl_Trace_Mask = ~NU_TRACE_ALL;

        /* Elevate callers priority to default high priority cap */
        p_curr_task = NU_Current_Task_Pointer();
        orig_priority = NU_Change_Priority(p_curr_task,
        MIN(CFG_NU_OS_SVCS_TRACE_CORE_COMMS_FLUSH_DEFAULT_PRIORITY,
        CFG_NU_OS_KERN_PLUS_CORE_NUM_TASK_PRIORITIES-1));

        /* Transmit all available trace data */
        status  = NU_Trace_Comms_Transmit_N_Packets(NU_TRACE_TRANSMIT_ALL,
                  &pkts_transmitted);

        /* Expect a buffer empty error and set status to success */
        if(status == NU_TRACE_BUFF_EMPTY)
        {
            status = NU_SUCCESS;
        }

        /* Restore callers priority */
        NU_Change_Priority(p_curr_task, orig_priority);

        /* Restore trace mask */
        NU_Trace_Arm(orig_trace_mask);

        /* Start trace communications */
        if(orig_trace_comms_state == NU_TRUE)
        {
            NU_Trace_Comms_Start(Trace_Mgmt.trace_comms_tsk_priority,
            Trace_Mgmt.trace_comms_tx_period);
        }
    }
    else
    {
        status = NU_TRACE_NOT_INITIALIZED;
    }

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Transmit_N_Packets
*
*   DESCRIPTION
*
*       User API to transmit N trace packets. Note: this call will
*       block the caller till N trace data packets are transmitted
*       to the host. If the buffer is empty the call will return
*       control to caller.
*
*   INPUTS
*
*       num_packets      Number of trace data packets to transmit
*       num_pkts_transmitted    Pointer to number of trace data packets
*                               transmitted
*
*   OUTPUTS
*
*       NU_SUCCESS       All trace data available has been flushed.
*       Otherwise        error.
*
***********************************************************************/
STATUS  Trace_Comms_Transmit_N_Packets(UINT32 num_packets, UINT32* num_pkts_transmitted)
{
    STATUS          status = NU_SUCCESS;
    UINT32          pkts_transmitted=0;
    UINT16          size;
    INT             i;
    UINT32          orig_trace_mask;


    /* Ensure trace communications interface is up and ready */
    while(!NU_Trace_Comms_Is_Ready())
    {
        NU_Sleep(50);
    }

    /* Obtain current trace mask */
    orig_trace_mask = Gbl_Trace_Mask;

    /* Disable all trace instrumentation so we don't produce trace data
    *  while flushing all logged data from trace buffers */
    Gbl_Trace_Mask = ~NU_TRACE_ALL;

    /* Transmit number of packets requested by the user */
    for(i = 0; ((status == NU_SUCCESS) && (i < num_packets)); i++)
    {
        /* Fetch the trace packet */
        status = Get_Trace_Packet(Trace_Mgmt.p_scratch_buff, &size);

        /* If data is available for transmission */
        if(status == NU_SUCCESS)
        {
            /* Transmit trace packet */
            if(NU_Trace_Comms_Transmit(Trace_Mgmt.p_scratch_buff, size) == NU_SUCCESS)
            {
                pkts_transmitted++;
            }
            else
            {
                status = NU_TRACE_COMMS_ERROR;
            }
        }
        else
        {
            if(status == TRACE_BUF_EMPTY_ERR)
            {
                status = NU_TRACE_BUFF_EMPTY;
            }
        }
    }

    /* Return number of packets transmitted */
    *num_pkts_transmitted = pkts_transmitted;

    /* Restore trace mask */
    NU_Trace_Arm(orig_trace_mask);

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Task_Entry
*
*   DESCRIPTION
*
*       This task periodically fetches trace packets from the trace buffer
*       and transmits to host via communications interface.
*
***********************************************************************/
VOID    Trace_Comms_Task_Entry(UNSIGNED argc, VOID *argv)
{
    UINT32 pkts_transmitted;

    while(1)
    {
        /* Obtain communications mutex here */
        NU_Obtain_Semaphore(&Trace_Mgmt.trace_comms_mutex, NU_SUSPEND);

        /* Transmit 1 trace packet */
        NU_Trace_Comms_Transmit_N_Packets(1, &pkts_transmitted);

        /* Release communications mutex here */
        NU_Release_Semaphore(&Trace_Mgmt.trace_comms_mutex);

        /* Sleep for user specified number of OS ticks */
        NU_Sleep(Trace_Mgmt.trace_comms_tx_period);
    }
}


#endif /* defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && \
    	( (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != FILE_INTERFACE))) */


#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE))

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Comms_Flush
*
*   DESCRIPTION
*
*       User API to Flush all available trace data to file
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS       All trace data available has been flushed.
*       Otherwise        error.
*
***********************************************************************/
STATUS  Trace_Comms_Flush(VOID)
{
    STATUS    status;
    UINT32    orig_priority;
    UINT32    orig_trace_mask;
    NU_TASK*  p_curr_task;

    /* It trace is initialized start communications */
    if(Trace_Mgmt.trace_init_flag == NU_TRUE)
    {
        /* Ensure trace communications interface is up and ready */
        while(!NU_Trace_Comms_Is_Ready())
        {
            NU_Sleep(50);
        }

        /* Obtain current trace mask */
        orig_trace_mask = Gbl_Trace_Mask;

        /* Disable all trace instrumentation so we don't produce trace data
         *  while flushing all logged data from trace buffers.
         */

        NU_Trace_Disarm(NU_TRACE_ALL);

        /* Elevate callers priority to default high priority cap */
        p_curr_task = NU_Current_Task_Pointer();
        orig_priority = NU_Change_Priority(p_curr_task,
        MIN(CFG_NU_OS_SVCS_TRACE_CORE_COMMS_FLUSH_DEFAULT_PRIORITY,
        CFG_NU_OS_KERN_PLUS_CORE_NUM_TASK_PRIORITIES-1));

        /* Flush trace buffer to file */
        status = NU_Trace_Buffer_Flush();

        /* Restore callers priority */
        NU_Change_Priority(p_curr_task, orig_priority);

        /* Restore trace mask */
        NU_Trace_Arm(orig_trace_mask);

    }
    else
    {
        status = NU_TRACE_NOT_INITIALIZED;
    }

    return status;
}

#endif /* (defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE)) */

#if(defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (defined(CFG_NU_OS_SVCS_SHELL_ENABLE)) \
           && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE))

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Flush
*
*   DESCRIPTION
*
*       Implements trace buffer flush command for Nucleus Shell
*
*   INPUTS
*
*       Unused arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
***********************************************************************/
STATUS Trace_Flush(NU_SHELL * shell, INT x,CHAR **argv)
{
    STATUS status;

    NU_UNUSED_PARAM(shell);
    NU_UNUSED_PARAM(argv);

    status = Trace_Comms_Flush();

    if (status == NU_TRACE_NOT_INITIALIZED)
        printf("\r\nError : Trace is not initialized.\r\n");

    return status;
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Info
*
*   DESCRIPTION
*
*       Implements trace_info command for Nucleus Shell. The information
*       contains trace mask, active comm intf, buffer state and FS state
*       for File comm intf.
*
*   INPUTS
*
*       Unused arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
***********************************************************************/
STATUS Trace_Info(NU_SHELL * shell, INT x, CHAR **argv)
{
    STATUS     status      = NU_SUCCESS;
    UINT32     buffer_size = 0;
    UINT32     used_space  = 0;
    UINT       trace_mask  = 0;
    CHAR       comm_intf[16];

    NU_UNUSED_PARAM(shell);
    NU_UNUSED_PARAM(argv);

    if(Trace_Mgmt.trace_init_flag == NU_TRUE)
    {
        buffer_size = Get_Buffer_Size();
        used_space  = Get_Used_Buffer_Space();
        trace_mask  = Gbl_Trace_Mask;

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)
    strcpy(comm_intf,"Debug");
#elif (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == SERIAL_INTERFACE)
    strcpy(comm_intf,"Serial");
#elif (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == ETHERNET_INTERFACE)
    strcpy(comm_intf,"Ethernet");
#else
    strcpy(comm_intf, "FTP");
#endif

        printf("\r\n***********************************\r\n");
        printf("*           Trace Info            *\r\n");
        printf("***********************************\r\n\r\n");
        printf("Trace Status    : Enabled\r\n");
        printf("Trace Mask      : 0x%x\r\n", trace_mask);
        printf("Trace Comm Intf : %s\r\n", comm_intf);

    printf("\n\r*********** Buffer Info ***********\n\r\n\r");

        printf("Buffer Size  : %lu bytes\r\n", buffer_size);
        printf("Used Space   : %lu bytes\r\n", used_space);

        if (Get_Is_Buffer_Overflowed() == NU_TRUE)
        {
            printf("Buffer State : Overflowed\r\n");
        }
        else
        {
            printf("Buffer State : No Overflow\r\n");
        }

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE)

        {
            CHAR       *fs_info;
            printf("\r\n********* Trace FS Info ***********\r\n\r\n");
            FileIO_Get_File_System_Info(&fs_info);
            printf(fs_info);
            printf("\r\n***********************************\r\n");
        }

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE) */
    }
    else
    {
        printf("\r\nError : Trace is not initialized. \r\n");
        status = NU_TRACE_NOT_INITIALIZED;
    }
    return ( status );
}

#endif /* (defined(CFG_NU_OS_SVCS_TRACE_COMMS_ENABLE) && (defined(CFG_NU_OS_SVCS_SHELL_ENABLE)) \
        && (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL != DBG_INTERFACE)) */

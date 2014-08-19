/*************************************************************************
*
*             Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       proc_exception.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains basic exception handling
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Exception
*       Process_Exception_Handler (weak reference)
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       thread_control.h
*       proc_extern.h
*       proc_core.h
*       [nu_trace.h]
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "kernel/proc_extern.h"
#include "os/kernel/process/core/proc_core.h"

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE
#include "services/nu_trace.h"
#endif

/* External variable declarations */
extern TC_TCB              *TCD_Priority_List[TC_PRIORITIES+1];
extern INT                  TCD_Highest_Priority;

/* Currently nested exceptions are not supported, this data structure
   will contain the basic information for all exceptions that occur */
static NU_PROCESS_EXCEPTION proc_exception_cb;

/************************************************************************
*
* FUNCTION
*
*       PROC_Exception
*
* DESCRIPTION
*
*       This exception handler calls the provided exception handler and
*       returns the method to resume execution
*
* INPUTS
*
*       exception_address
*       exception_type
*       return_address
*       exception_information
*
* OUTPUTS
*
*       Method of exception exit
*
************************************************************************/
UNSIGNED PROC_Exception(VOID *exception_address, UNSIGNED exception_type,
                        VOID *return_address, VOID *exception_information)
{
    UNSIGNED exception_exit;
    BOOLEAN  preemption;
#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE
    UINT32   markers;
#endif

    /* Fill out the basic information for the control block */
    proc_exception_cb.pid = PROC_Scheduled_CB -> id;
    proc_exception_cb.task = TCD_Execute_Task;
    proc_exception_cb.address = exception_address;
    proc_exception_cb.return_address = return_address;
    proc_exception_cb.type = exception_type;
    proc_exception_cb.interrupt_context = ((ESAL_GE_ISR_EXECUTING()) || ((VOID *)TCD_Execute_HISR == TCD_Current_Thread));
    proc_exception_cb.kernel_process = (PROC_Scheduled_CB -> kernel_mode);
    proc_exception_cb.exception_information = exception_information;

    /* Disable pre-emption on the task that was executing when the exception
       occurred - this will allow certain process-level APIs to be executed
       as well as resuming a kernel process task to handle other parts of the
       exception handling. */
    preemption = TCD_Execute_Task->tc_preemption;
    TCD_Execute_Task->tc_preemption = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE
    /* Disable trace markers during exception handler */
    markers = NU_Trace_Get_Mask();
    (VOID)NU_Trace_Disarm(NU_TRACE_ALL);
#endif

    /* Call the exception handler */
    exception_exit = Process_Exception_Handler(&proc_exception_cb);

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE
    /* Restore trace markers */
    (VOID)NU_Trace_Arm(markers);
#endif

    /* Check return value to see if returning to the scheduler */
    if (exception_exit == NU_PROC_SCHEDULE)
    {
        /* Set execute task to highest priority ready task in the system to allow
           any scheduling changes made in the exception handler to occur when returning
           to the scheduler. */
        TCD_Execute_Task = TCD_Priority_List[TCD_Highest_Priority];
    }
    else
    {
        /* Restore pre-emption for the current task to put everything back to the
           same state as before the exception. */
        TCD_Execute_Task->tc_preemption = preemption;
    }

    return (exception_exit);
}

/***********************************************************************
*
*   FUNCTION
*
*       Process_Exception_Handler
*
*   DESCRIPTION
*
*       This function provides a default implementation of the memory
*       management exception handler.  It will terminate the offending
*       task and return to the scheduler.
*
*   INPUTS
*
*       exception_info
*
*   OUTPUTS
*
*       NU_PROC_SCHEDULE
*
***********************************************************************/
ESAL_TS_WEAK_DEF(UNSIGNED Process_Exception_Handler(NU_PROCESS_EXCEPTION *exception))
{
    UNSIGNED    return_value = NU_PROC_SCHEDULE;

    /* Determine if exception occurred in user process */
    if ((exception->interrupt_context == NU_FALSE) && (exception->kernel_process == NU_FALSE))
    {
        /* Stop the offending process and indicate an error condition */
        (VOID)NU_Stop(exception->pid, exception->type, NU_NO_SUSPEND);
    }
    else
    {
        /* Set return value to unrecoverable error */
        return_value = NU_PROC_UNRECOVERABLE;
    }

    /* Return with the appropriate value */
    return (return_value);
}

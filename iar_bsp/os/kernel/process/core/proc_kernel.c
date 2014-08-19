/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
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
*       proc_kernel.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains functionality that is called directly during
*       by the kernel.  This includes initialization of processes and
*       process updates during scheduling
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Kernel_Initialize
*       PROC_Schedule
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "proc_core.h"

extern NU_MEMORY_POOL  System_Memory;

extern NU_SYMBOL_ENTRY _nu_symbol_table_start[];
extern NU_SYMBOL_ENTRY _nu_ksymbol_table_start[];

static CHAR PROC_Task_Stack[PROC_KERNEL_STACK_SIZE];

STATUS PROC_AR_Kernel_Initialize(VOID);

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
STATUS PROC_Mem_Mgmt_Initialize(VOID);
VOID   PROC_AR_Schedule(PROC_CB *process, VOID* return_addr);
#endif

/*************************************************************************
*
* FUNCTION
*
*       PROC_Kernel_Initialize
*
* DESCRIPTION
*
*       This routine initializes the process global variables
*       and calls the target specific routine for additional process
*       initialization.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS          Routine executed successfully.
*       status of target specific complement.
*
*************************************************************************/
STATUS PROC_Kernel_Initialize(VOID)
{
    STATUS   status;

#if (CFG_NU_OS_KERN_PROCESS_CORE_EXPORT_SYMBOLS == NU_TRUE)

    /* Keep symbols for nu.os.kern.process */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_KERN_PROCESS_CORE);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_EXPORT_SYMBOLS */

    /* Initialize the kernel process */
    status = PROC_Initialize(&PROC_Kernel_CB, "kernel");

    /* Ensure successful */
    if (status == NU_SUCCESS)
    {
        NU_SYMBOL_ENTRY *symbol_table = &_nu_symbol_table_start[0];
        NU_SYMBOL_ENTRY *ksymbol_table = &_nu_ksymbol_table_start[0];

        /* Check if symbol table was found. */
        if (symbol_table)
        {
            /* Set Nucleus symbols table address */
            PROC_Kernel_CB -> symbols = symbol_table;
        }

        /* Check if kernel-mode symbol table was found */
        if (ksymbol_table)
        {
            /* Set Nucleus kernel mode symbols table address */
            PROC_Kernel_CB -> ksymbols = ksymbol_table;
        }

        /* Initialize the current process to be the kernel */
        PROC_Scheduled_CB = PROC_Kernel_CB;

        /* Mark the kernel process as kernel mode */
        PROC_Kernel_CB -> kernel_mode = NU_TRUE;

        /* This call will setup mode switching at the
           architecture level */
        status = PROC_AR_Kernel_Initialize();
    }

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
    if (status == NU_SUCCESS)
    {
        /* Initialize the memory management component */
        status = PROC_Mem_Mgmt_Initialize();
    }
#endif

    /* Create the kernel state queue */
    if (status == NU_SUCCESS)
    {
        status = NU_Create_Queue(&(PROC_Kernel_CB -> queue), "process", &(PROC_Kernel_CB -> buffer),
                                 PROC_QUEUE_SIZE, NU_FIXED_SIZE, PROC_MSG_SIZE, NU_FIFO);
    }

    /* Create the kernel thread */
    if (status == NU_SUCCESS)
    {
        status = NU_Create_Task(&(PROC_Kernel_CB -> root_task), "state",
                                PROC_Kernel_Task_Entry, 0, NU_NULL, &PROC_Task_Stack[0],
                                PROC_KERNEL_STACK_SIZE, PROC_KERNEL_PRIORITY,
                                PROC_KERNEL_TIMESLICE, NU_PREEMPT, NU_START);
    }

    if (status == NU_SUCCESS)
    {
        /* Set System_Memory as the process pool for the kernel */
        PROC_Kernel_CB -> pool = &System_Memory;

        /* Marked the kernel process as started */
        PROC_Kernel_CB -> state = PROC_STARTED_STATE;
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       PROC_Schedule
*
* DESCRIPTION
*
*       Update the global process if it has changed, if memory management
*       is enabled call the arch component to do any updates required
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID PROC_Schedule(VOID)
{
    PROC_CB    *process;

    /* Get pointer to process control block */
    process = (PROC_CB *)(((TC_HCB *)TCD_Current_Thread) -> tc_process);

    /* Check if current executing thread is part of the current process */
    if (process != PROC_Scheduled_CB)
    {
        /* Update current process */
        PROC_Scheduled_CB = process;

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE

        /* Do the architecture related updates */
        PROC_AR_Schedule(process, ((TC_TCB *)TCD_Current_Thread) -> tc_return_addr);

#endif /* CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE */
    }
}

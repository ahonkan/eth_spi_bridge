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
*       tcct_signal.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains core signal exit thread control / scheduling
*       function with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCCT_Signal_Exit                    Exit from signal handler
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

/***********************************************************************
*
*   FUNCTION
*
*       TCCT_Signal_Exit
*
*   DESCRIPTION
*
*       This function exits from a signal handler.  The primary purpose
*       of this function is to clear the scheduler protection and switch
*       the stack pointer back to the normal task's stack pointer.
*
*   CALLED BY
*
*       TCC_Signal_Shell                    Signal handling shell func
*
*   CALLS
*
*       TCCT_Schedule                       Schedule the next thread
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
VOID  TCCT_Signal_Exit(VOID)
{
    R1  TC_TCB  *current_thread;


    /* Disable interrupts for critical section */
    ESAL_GE_INT_FAST_ALL_DISABLE();

    /* Get pointer to current thread */
    current_thread = (TC_TCB *)TCD_Current_Thread;

    /* Clear thread's ownership of protection */
    TCCT_Schedule_Unlock();

    /* Switch back to the saved stack.  The saved stack pointer was saved
       before the signal frame was built */
    current_thread -> tc_stack_pointer = current_thread -> tc_saved_stack_ptr;

    /* Switch back to the saved return address */
    current_thread -> tc_return_addr = current_thread -> tc_saved_return_addr;

    /* Ensure task has a full time-slice for next time
       it gets scheduled */
    current_thread -> tc_cur_time_slice = current_thread -> tc_time_slice;

    /* Set stack pointer to system stack before returning to the scheduler */
    ESAL_GE_STK_SYSTEM_SP_SET();

    /* Transfer control to scheduler (control doesn't return from this call) */
    TCCT_Schedule();

    /* Code should never reach here.  This line ensures compiler doesn't try
       to optimize return from this function and cause a stack pointer
       problem. */
    ESAL_GE_STK_NO_RETURN();
}

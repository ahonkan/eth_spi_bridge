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
*       tcc_current_hisr.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core current HISR routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Current_HISR_Pointer             Retrieve current HISR ptr
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
*       NU_Current_HISR_Pointer
*
*   DESCRIPTION
*
*       This function returns the pointer of the currently executing
*       HISR.  If the current thread is not a HISR thread, a NU_NULL
*       is returned.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       HISR Pointer                        Active HISR pointer or
*                                           NU_NULL if not a HISR
*
***********************************************************************/
NU_HISR *NU_Current_HISR_Pointer(VOID)
{
    /* Determine if a HISR thread is executing.  */
    if ((TCD_Current_Thread) &&
        (((TC_HCB *) TCD_Current_Thread) -> tc_id == TC_HISR_ID) &&
        (ESAL_GE_ISR_EXECUTING() == NU_FALSE))
    {
        /* HISR thread is running, return the pointer.  */
        return((NU_HISR *) TCD_Current_Thread);
    }
    else
    {
        /* No, HISR thread is not running, return a NU_NULL.  */
        return(NU_NULL);
    }
}

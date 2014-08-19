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
*       tcc_delete_hisr.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains the core delete HISR routine for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Delete_HISR                      Delete HISR
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
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern CS_NODE              *TCD_Created_HISRs_List;
extern UNSIGNED             TCD_Total_HISRs;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Delete_HISR
*
*   DESCRIPTION
*
*       This function deletes a HISR and removes it from the list of
*       created HISRs.  It is assumed by this function that the HISR is
*       in a non-active state.  Note that this function does not free
*       memory associated with the HISR's control block or its stack.
*       This is the responsibility of the application.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Remove_From_List                 Remove node from list
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created HISR list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       hisr_ptr                            HISR control block pointer
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVALID_HISR                     Indicates HISR pointer is
*                                           invalid
*
***********************************************************************/
STATUS NU_Delete_HISR(NU_HISR *hisr_ptr)
{
    R1 TC_HCB   *hisr;                      /* HISR control block ptr    */
    STATUS       status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Move input HISR pointer into internal pointer.  */
    hisr =  (TC_HCB *) hisr_ptr;

    /* Determine if the supplied HISR pointer is valid */
    NU_ERROR_CHECK(((hisr == NU_NULL) || (hisr -> tc_id != TC_HISR_ID)), status, NU_INVALID_HISR);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Call stack checking function to check for an overflow condition.  */
        (VOID)NU_Check_Stack();

        /* Protect the list of created HISRs.  */
        TCCT_Schedule_Lock();

        /* Remove the HISR from the list of created HISRs.  */
        NU_Remove_From_List(&TCD_Created_HISRs_List, &(hisr -> tc_created));

        /* Decrement the total number of created HISRs.  */
        TCD_Total_HISRs--;

        /* Clear the HISR ID just in case.  */
        hisr -> tc_id =  0;

        /* Trace log */
        T_HISR_DELETED((VOID*)hisr, status);

        /* Release protection.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        /* Trace log */
        T_HISR_DELETED((VOID*)hisr, status);
    }

    /* Return a successful completion.  */
    return(status);
}

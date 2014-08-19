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
*       tcct_register.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains core register LISR thread control / scheduling
*       functions with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Register_LISR                    Register an LISR
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/* Define external function references */
extern VOID                 TCC_Unhandled_Interrupt(INT);

/***********************************************************************
*
*   FUNCTION
*
*       NU_Register_LISR
*
*   DESCRIPTION
*
*       This function registers the supplied LISR with the supplied
*       vector number.  If the supplied LISR is NU_NULL, the supplied
*       vector is de-registered.  The previously registered LISR is
*       returned to the caller, along with the completion status.  This
*       routine must be called from Supervisor mode in a Supervisor/
*       User mode switching kernel.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect LISR registration
*       TCCT_Schedule_Unlock                Release LISR protection
*
*   INPUTS
*
*       vector                              Vector number of interrupt
*       new_lisr                            New LISR function
*       old_lisr                            Previous LISR function ptr
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Successful registration
*           NU_INVALID_VECTOR               Invalid interrupt vector
*
***********************************************************************/
STATUS NU_Register_LISR(INT vector, VOID (*new_lisr)(INT),
                        VOID (**old_lisr)(INT))
{
    STATUS status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Determine if the vector is legal. */
    NU_ERROR_CHECK((vector > ESAL_GE_INT_MAX_VECTOR_ID_GET()), status, NU_INVALID_VECTOR);
    NU_ERROR_CHECK(((new_lisr == NU_NULL) && (ESAL_GE_ISR_HANDLER_GET(vector) == TCC_Unhandled_Interrupt)), status, NU_NOT_REGISTERED);

    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE_ISR();

        /* Protect against LISR registration list access.  */
        TCCT_Schedule_Lock();

        /* Copy the currently registered ISR into the old_lisr return area. */
        *old_lisr = ESAL_GE_ISR_HANDLER_GET(vector);

        /* Check if unhandled interrupt ISR is registered */
        if (*old_lisr == TCC_Unhandled_Interrupt)
        {
            /* return no previous ISR */
            *old_lisr = NU_NULL;
        }

        /* Determine if a registration or deregistration is requested.  This is
           determined by the value of new_lisr.  A NULL value indicates
           deregistration.  */
        if (new_lisr)
        {
            /* Place the new LISR into the list. */
            ESAL_GE_ISR_HANDLER_SET(vector, new_lisr);
        }
        else
        {
            /* Set-up LISR pointer to point to an unhandled interrupt. */
            ESAL_GE_ISR_HANDLER_SET(vector, TCC_Unhandled_Interrupt);
        }

        /* Trace log */
        T_LISR_REGISTERED(vector, NU_SUCCESS);

        /* Release protection on the LISR registration list.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE_ISR();
    }
    else
    {
        /* Trace log */
        T_LISR_REGISTERED(vector, status);
    }

    /* Return the completion status.  */
    return(status);
}

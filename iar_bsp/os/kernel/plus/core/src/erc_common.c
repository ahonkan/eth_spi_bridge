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
*       erc_common.c
*
*   COMPONENT
*
*       ER - Error Management
*
*   DESCRIPTION
*
*       This file contains the core routines for the Error management
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       ERC_System_Error                    System error function
*       ERC_Assert                          System assertion routine
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

/* Define external inner-component global data references.  */
extern  INT        ERD_Error_Code;

#if (NU_ASSERT_ENABLE == NU_TRUE)

extern UNSIGNED    ERD_Assert_Count;

#endif  /* NU_ASSERT_ENABLE == NU_TRUE */

#if (NU_ERROR_STRING == NU_TRUE)

extern const CHAR *ERD_Error_String;

const static CHAR  ERD_Create_Timer_HISR[]      = "Error Creating Timer HISR";
const static CHAR  ERD_Stack_Overflow_Task[]    = "Stack Overflow in task";
const static CHAR  ERD_Stack_Overflow_HISR[]    = "Stack Overflow in HISR";
const static CHAR  ERD_Unhandled_Interrupt[]    = "Unhandled interrupt error";
const static CHAR  ERD_Not_In_Supervisor_Mode[] = "Supervisor code run in user mode";
const static CHAR  ERD_Not_Enough_DTLBS[]       = "Not enough data TLBs";
const static CHAR  ERD_Not_Enough_ITLBS[]       = "Not enough instruction TLBs";
const static CHAR  ERD_Stack_Underflow_Task[]   = "Stack Underflow in task";
const static CHAR  ERD_Stack_Underflow_HISR[]   = "Stack Underflow in HISR";
const static CHAR  ERD_Unhandled_Exception[]    = "Unhandled exception error";
const static CHAR  ERD_Unsupported_Error[]      = "Unsupported error code";

#endif  /* NU_ERROR_STRING == NU_TRUE */

/***********************************************************************
*
*   FUNCTION
*
*       ERC_System_Error
*
*   DESCRIPTION
*
*       This function processes system errors detected by various
*       system components.  Typically an error of this type is
*       considered fatal.
*
*   CALLED BY
*
*       NU_Check_Stack                    Check current stack
*       TMIT_Initialize                     Timer Management Initialize
*       TCC_Unhandled_Interrupt             Unhandled interrupt entry
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       error_code                          Code of detected system error
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  ERC_System_Error(INT error_code)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First place the error code into the global variable.  */
    ERD_Error_Code =  error_code;

#if (NU_ERROR_STRING == NU_TRUE)

    /* Build string that corresponds to the error.  */
    switch(error_code)
    {
        case    NU_ERROR_CREATING_TIMER_HISR:
        {
            /* Set string that indicates an error
               occurred creating the timer HISR.  */
            ERD_Error_String = &ERD_Create_Timer_HISR[0];
            break;
        }

        case    NU_STACK_OVERFLOW:
        {
            /* Set string that indicates a stack
               overflow occurred.  */
            if (((TC_TCB *) TCD_Current_Thread) -> tc_id == TC_TASK_ID)
            {
                /* This is a task control block */
                ERD_Error_String = &ERD_Stack_Overflow_Task[0];
            }
            else
            {
                /* This is a HISR control block */
                ERD_Error_String = &ERD_Stack_Overflow_HISR[0];
            }
            break;
        }

        case    NU_UNHANDLED_INTERRUPT:
        {
            /* Set string that indicates an error occurred
               because of an unhandled interrupt.  */
            ERD_Error_String = &ERD_Unhandled_Interrupt[0];
            break;
        }

        case    NU_NOT_IN_SUPERVISOR_MODE:
        {
            /* Set string that indicates an error occurred
               because a function in user mode tried to
               execute a function that can only be run in
               supervisor mode.  */
            ERD_Error_String = &ERD_Not_In_Supervisor_Mode[0];
            break;
        }

        case    NU_NOT_ENOUGH_DTLBS:
        {
            /* Set string that indicates an error occurred
               because there are not enough data TLBs
               to continue.  */
            ERD_Error_String = &ERD_Not_Enough_DTLBS[0];
            break;
        }

        case    NU_NOT_ENOUGH_ITLBS:
        {
            /* Set string that indicates an error occurred
               because there are not enough instruction TLBs
               to continue.  */
            ERD_Error_String = &ERD_Not_Enough_ITLBS[0];
            break;
        }

        case    NU_STACK_UNDERFLOW:
        {
            /* Set string that indicates a stack
               underflow occurred.  */
            if (((TC_TCB *) TCD_Current_Thread) -> tc_id == TC_TASK_ID)
            {
                /* This is a task control block */
                ERD_Error_String = &ERD_Stack_Underflow_Task[0];
            }
            else
            {
                /* This is a HISR control block */
                ERD_Error_String = &ERD_Stack_Underflow_HISR[0];
            }
            break;
        }

        case    NU_UNHANDLED_EXCEPTION:
        {
            /* Set string that indicates an error occurred
               because of an unhandled exception.  */
            ERD_Error_String = &ERD_Unhandled_Exception[0];
            break;
        }

        default:
        {
            /* Set string that indicates the error code
               passed to this function is not supported.  */
            ERD_Error_String = &ERD_Unsupported_Error[0];
        }
    }

#endif /* NU_ERROR_STRING == NU_TRUE */

    /* This function cannot return, since the error is fatal.  */
    while(1){}

    /* No need to return to user mode because of the infinite loop. */
    /* Returning to user mode will cause warnings with some compilers. */
}


#if (NU_ASSERT_ENABLE == NU_TRUE)
/***********************************************************************
*
*   FUNCTION
*
*       ERC_Assert
*
*   DESCRIPTION
*
*       This public routine is called when an assertion made by the
*       NU_ASSERT (or NU_ASSERT2) macro fails.  By default, this routine
*       simply counts the number of failed assertions.  A breakpoint can
*       be set in the routine to observe failed assertions, or the
*       routine can be customized to perform some action, such as
*       printing the failed assertion as a message, etc.
*
*   CALLED BY
*
*       NU_ASSERT  macro
*       NU_ASSERT2 macro
*
*   CALLS
*
*
*   INPUTS
*
*       test               Pointer to string of failed assertion test
*       name               File name of file containing failed assertion
*       line               Location of failed assertion in above file
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID    ERC_Assert(CHAR *test, CHAR *name, UNSIGNED line)
{
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set breakpoint here to catch failed assertions. */
    ERD_Assert_Count += 1;

    /* Return to user mode */
    NU_USER_MODE();
}

#endif /* NU_ASSERT_ENABLE == NU_TRUE */

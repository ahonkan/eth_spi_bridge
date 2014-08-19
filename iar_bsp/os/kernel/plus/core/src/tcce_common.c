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
*       tcce_common.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains common error checking routines for the
*       functions in the Thread Control component.  This permits easy
*       removal of error checking logic when it is not needed.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCCE_Create_Task                    Create a task
*       TCCE_Create_HISR                    Create HISR
*       TCCE_Suspend_Error                  Check for suspend req error
*       TCCE_Activate_HISR                  Activate an HISR
*       TCCE_Validate_Resume                Validates resume requests
*       TCCE_Register_LISR                  Register LISR
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
*       TCCE_Suspend_Error
*
*   DESCRIPTION
*
*       This function checks for a suspend request error.  Suspension
*       requests are only allowed from task threads.  A suspend request
*       from any other thread is an error.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status
*           NU_TRUE                         If an error is detected
*           NU_FALSE                        If no error is detected
*
***********************************************************************/
INT   TCCE_Suspend_Error(VOID)
{
    TC_TCB           *task;                 /* Task pointer              */
    INT              status = NU_FALSE;     /* Initialize to no error    */


    /* Setup the task pointer.  */
    task =  (TC_TCB *) TCD_Current_Thread;

    /* Check for suspension errors.  */
    if (task == NU_NULL)
    {
        /* Error, suspend request probably from initialization.  */
        status =  NU_TRUE;
    }
    else if (task -> tc_id != TC_TASK_ID)
    {
        /* Control block is probably an HISR not a task.  */
        status =  NU_TRUE;
    }
    else if (task -> tc_signal_active)
    {
        /* Called from a signal handler.  */
        status =  NU_TRUE;
    }

    /* Return status to caller.  */
    return(status);
}

/***********************************************************************
*
*   FUNCTION
*
*       TCCE_Validate_Resume
*
*   DESCRIPTION
*
*       This function validates the resume service with
*       scheduling protection around the examination of the
*       task status.
*
*   CALLED BY
*
*       TCCE_Resume_Service                 Error checking function
*
*   CALLS
*
*       TCCT_Schedule_Lock                  Protect from system access
*       TCCT_Schedule_Unlock                Release current protection
*
*   INPUTS
*
*       resume_type                         Type of resume request
*       task_ptr                            Task control block pointer
*
*   OUTPUTS
*
*       status
*           NU_TRUE                         Invalid resume
*           NU_FALSE                        Valid resume
*
***********************************************************************/
STATUS  TCCE_Validate_Resume(OPTION resume_type, NU_TASK *task_ptr)
{
    R1 TC_TCB       *task;                  /* Task control block ptr    */
    STATUS          status;                 /* Return status variable    */
    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Move input task pointer into internal pointer.  */
    task =  (TC_TCB *) task_ptr;

    /* Protect the scheduling structures from multiple access.  */
    TCCT_Schedule_Lock();

    /* Does the resume type match the current status?  */
    if (task -> tc_status == resume_type)
    {
        /* Indicate that there is no error.  */
        status =  NU_FALSE;
    }
    /* Check for a resumption of a delayed pure suspend.  */
    else if ((resume_type == NU_PURE_SUSPEND) && (task -> tc_delayed_suspend))
    {
        /* Indicate that there is no error.  */
        status =  NU_FALSE;
    }
    /* Check for a signal active and the saved status the same as
       the resume request.  */
    else if ((resume_type == task -> tc_saved_status) &&
            (task -> tc_signal_active))
    {
        /* Indicate that there is no error.  */
        status =  NU_FALSE;
    }
    else
    {
        /* Indicate that there is an error.  */
        status =  NU_TRUE;
    }

    /* Release protection of system structures.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();

    /* Return status to caller.  */
    return(status);
}

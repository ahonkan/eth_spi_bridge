/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       net_s.c
*
*   DESCRIPTION
*
*       Nucleus NET Sleep module.  The call to NET_Sleep can be made
*       in place of a call to an operating system sleep routine.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NET_Sleep
*       NET_Resume
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

extern  TQ_EVENT    NET_Resume_Event;

/*************************************************************************
*
*   FUNCTION
*
*       NET_Sleep
*
*   DESCRIPTION
*
*       This function suspends the calling task and resumes it after
*       the specified number of ticks.
*
*   INPUTS
*
*       ticks                   The number of ticks to suspend the task.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NET_Sleep(UINT32 ticks)
{
    NU_TASK     *task_ptr;
    STATUS		status;

    /* Get a pointer to the current task */
    task_ptr = NU_Current_Task_Pointer();

    /* Obtain the TCP semaphore to protect the stack global variables */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
		/* Set the timer to resume this task */
		status = TQ_Timerset(NET_Resume_Event, (UNSIGNED)task_ptr, ticks, 0);

		/* Release the semaphore. */
		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
			NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
						   __FILE__, __LINE__);

		/* Ensure the wake up timer was set before suspending. */
		if (status == NU_SUCCESS)
		{
		    /* Suspend */
		    NU_Suspend_Task(task_ptr);
		}

		else
		{
			NLOG_Error_Log("Failed to set timer", NERR_SEVERE,
						   __FILE__, __LINE__);
		}
	}

} /* NET_Sleep */

/*************************************************************************
*
*   FUNCTION
*
*       NET_Resume
*
*   DESCRIPTION
*
*       This function resumes the specified task.
*
*   INPUTS
*
*       event                   The event that is being handled.
*       task_ptr                A pointer to the task to resume.
*       unused_parm             Unused extra data.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NET_Resume(TQ_EVENT event, UNSIGNED task_ptr, UNSIGNED unused_parm)
{
    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(unused_parm);

    /* Resume the specified task */
    NU_Resume_Task((NU_TASK*)task_ptr);

} /* NET_Resume */

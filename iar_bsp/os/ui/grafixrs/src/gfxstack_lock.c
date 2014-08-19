/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  gfxstack_lock.c                                                   
*
* DESCRIPTION
*
*  Contains lock related functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  GFX_GetScreenSemaphore
*  GFX_ReleaseScreenSemaphore
*
* DEPENDENCIES
*
*  rs_base.h
*  global.h
*  gfxStack.h
*
***************************************************************************/
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/gfxstack.h"

#ifndef     SINGLE_THREADED

NU_TASK     *task_semaphore = NU_NULL;
INT32        sema_count = 0;

extern NU_SEMAPHORE    ScreenSema;
extern VOID SetPort( rsPort *portPtr);

/***************************************************************************
* FUNCTION
*
*   GFX_GetScreenSemaphore
*
* DESCRIPTION
*
*   Function GFX_GetScreenSemaphore gets the screen semaphore.
*
* INPUTS
*
*   None.
*
* OUTPUTS
*
*   UINT32 - Returns the result of NU_Obtain_Semaphore.
*
***************************************************************************/
UINT32 GFX_GetScreenSemaphore(VOID)
{
    STATUS      status = NU_SUCCESS;
    NU_TASK     *curr_task;

    /* Get the current task */
    curr_task = NU_Current_Task_Pointer();

    /* Are we trying to get the semaphore again for this task? */
    if (curr_task == task_semaphore)
    {
        /* Yes, so just increment the semaphore count and exit */
        sema_count++;
    }

    /* If we go in here, then the semaphore is free */
    else if (sema_count == 0)
    {
        /* Obtain the semaphore */
        status = NU_Obtain_Semaphore(&ScreenSema,RS_SUSPEND_TIME);

        /* increase the semaphore nest count */
        sema_count++;
    }

    /* If we get here, then another task already has the semaphore */
    /* We'll see if the task will give it up */
    else
    {
        /* Attempt to obtain the semaphore */
        status = NU_Obtain_Semaphore(&ScreenSema,RS_SUSPEND_TIME);
    }

    /* Was the semaphore obtained? */
    if (sema_count == 1)
    {
        /* It was, so set the curr_task to the semaphore task flag */
        task_semaphore = curr_task;
    }

    /* return from the function */
    return (status);
}

/***************************************************************************
* FUNCTION
*
*   GFX_ReleaseScreenSemaphore
*
* DESCRIPTION
*
*   Function GFX_ReleaseScreenSemaphore releases the screen semaphore.
*
* INPUTS
*
*   None.
*
* OUTPUTS
*
*   UINT32 - Returns the result of NU_Release_Semaphore.
*
***************************************************************************/
UINT32 GFX_ReleaseScreenSemaphore(VOID)
{
    STATUS      status = NU_SUCCESS;
    NU_TASK     *curr_task;

    /* Get the current task */
    curr_task = NU_Current_Task_Pointer();

    /* Are we trying to release the semaphore prematurely for this task? */
    if ((curr_task == task_semaphore) && (sema_count == 1))
    {
        /* The correct task is trying to release, and it's not nested */
        /* Release the semaphore */
        status = NU_Release_Semaphore(&ScreenSema);

        /* Decrement the semaphore count */
        sema_count--;

        /* Reset the task semaphore pointer to null since nothing has the */
        /* Semaphore any longer */
        task_semaphore = NU_NULL;
    }

    /* This is a nested release attempt, so just decrement the semaphore counter */
    else if ((curr_task == task_semaphore) && (sema_count > 1))
    {
        /* Decrement the semaphore counter */
        sema_count--;
    }

    /* Another task is trying to release */
    else
    {
        /* Make an attempt to release */
        status = NU_Release_Semaphore(&ScreenSema);
    }       

    /* return from the function */
    return status;
}

#endif  /* SINGLE_THREADED */

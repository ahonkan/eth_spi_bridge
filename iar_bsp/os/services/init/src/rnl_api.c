/***********************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       rnl_api.c
*
*   DESCRIPTION
*
*       This file contains the run-level code for Nucleus.
*
*   FUNCTIONS
*
*       TBD
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       runlevel.h
*       runlevel.h
*       reg_api.h
*       string.h
*       stdio.h
*
***********************************************************************/

/* Include necessary services */
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "os/services/init/inc/runlevel.h"
#include    "services/runlevel_init.h"
#include    "services/reg_api.h"
#include    <string.h>
#include    <stdio.h>


/***********************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_Current
*
*   DESCRIPTION
*
*       This function returns the current run-level
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       runlevel                            Pointer to return runlevel
*
*   OUTPUTS
*
*       NU_SUCCESS                          Returned runlevel is valid
*       NU_NOT_PRESENT                      Run-level initialization hasn't started
*
***********************************************************************/
STATUS  NU_RunLevel_Current(INT * runlevel)
{
    STATUS      status;
    UNSIGNED    event_bits;
    INT         current_runlevel;


    /* Get any "started" run-level event group bits that are set */
    status =  NU_Retrieve_Events(&RunLevel_Started_Event, 0xFFFFFFFF,
                                 NU_OR,
                                 &event_bits, NU_NO_SUSPEND);

    /* Ensure a run-level bit is set (ie run-level processing has started) */
    if (status == NU_SUCCESS)
    {
        /* Initialize to run-level 0 */
        current_runlevel = 0;

        /* Shift event bits right a bit to start checking for leading 1 */
        event_bits >>= 1;

        /* Find current run-level (leading '1' value is current) */
        while (event_bits)
        {
            /* Shift event bits right a bit to start checking for leading 1 */
            event_bits >>= 1;

            /* Go to next runlevel */
            current_runlevel++;
        }

        /* Return current run-level */
        *runlevel = current_runlevel;
    }

    /* Resturn results of operation */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_Status
*
*   DESCRIPTION
*
*       Get the status of a given runlevel
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       runlevel                            Runlevel to retrieve status for
*
*   OUTPUTS
*
*       NU_SUCESS                           Run-level has completed initialization
*       NU_RUNLEVEL_NOT_STARTED             Run-level has not started initialization
*       NU_RUNLEVEL_IN_PROGRESS             Run-level is in process of initialization
*       NU_NOT_PRESENT                      No run-level exists for specified run-level
*
***********************************************************************/
STATUS  NU_RunLevel_Status(INT runlevel)
{
    STATUS      status = NU_NOT_PRESENT;
    UNSIGNED    event_bits;


    /* Ensure run-level requested is "valid" */
    if ((runlevel >= 0) && (runlevel <= NU_RUNLEVEL_MAX))
    {
        /* Start by seeing if this run-level is finished */
        status =  NU_Retrieve_Events(&RunLevel_Finished_Event, (1UL << runlevel),
                                     NU_OR,
                                     &event_bits, NU_NO_SUSPEND);

        /* Check if this run-level has completed */
        if (status == NU_SUCCESS)
        {
            /* Set return value to show run-level has completed */
            status = NU_SUCCESS;
        }
        else
        {
            /* See if this run-level is started */
            status =  NU_Retrieve_Events(&RunLevel_Started_Event, (1UL << runlevel),
                                         NU_OR,
                                         &event_bits, NU_NO_SUSPEND);

            /* Check if this run-level has completed */
            if (status == NU_SUCCESS)
            {
                /* Set return value to show run-level has completed */
                status = NU_RUNLEVEL_IN_PROGRESS;
            }
            else
            {
                /* Run-level hasn't started yet */
                status = NU_RUNLEVEL_NOT_STARTED;
            }
        }
    }

    /* Resturn results of operation */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_Complete_Wait
*
*   DESCRIPTION
*
*       Wait until the specified run-level has completed
*       (ie all components within the run-level have finished initialization)
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       runlevel                            Runlevel to wait upon
*       timeout                             Timeout (ticks), NU_SUSPEND, or
*                                           NU_NO_SUSPEND (immediate completion)
*
*   OUTPUTS
*
*       NU_TIMEOUT                          Timeout occurred before runlevel completed
*       NU_SUCCESS                          Run-level completed
*       NU_NOT_PRESENT                      Specified run-level doesn't exist
*
***********************************************************************/
STATUS  NU_RunLevel_Complete_Wait(INT runlevel, UNSIGNED timeout)
{
    STATUS      status;
    UNSIGNED    event_bits;


    /* Ensure run-level requested is "valid" */
    if ((runlevel >= 0) && (runlevel <= NU_RUNLEVEL_MAX))
    {
        /* Wait until the run-level has completed or the timeout expires */
        status =  NU_Retrieve_Events(&RunLevel_Finished_Event, (1UL << runlevel),
                                     NU_OR,
                                     &event_bits, timeout);
    }
    else
    {
        /* Set return status to not present */
        status = NU_NOT_PRESENT;
    }

    /* Resturn results of operation */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_Next_Start
*
*   DESCRIPTION
*
*       Can be used to start initialization of the "next" run-level
*       (ie 1 greater then current run-level). This will only be
*       applicable if run-levels exist that are not configured to
*       "automatically" start. This implies that a run-level "limit"
*       will exist within the system that allows only the 1st N
*       run-levels to be automatically run.
*
*   CALLED BY
*
*       Misc
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
*       NU_SUCCESS                          Next run-level started
*       NU_RUNLEVEL_IN_PROGRESS             The current run-level is
*                                           still being executed (ie not completed)
*       NU_NOT_PRESENT                      No run-level is "next"
*
***********************************************************************/
STATUS  NU_RunLevel_Next_Start(VOID)
{
    STATUS      status;
    INT         current_runlevel;
    UNSIGNED    event_bits;


    /* Get current run-level */
    status = NU_RunLevel_Current(&current_runlevel);

    /* Verify getting current run-level was successful and the current
       run-level isn't the maximum run-level */
    if ((status == NU_SUCCESS) && (current_runlevel < NU_RUNLEVEL_MAX))
    {
        /* See if the current run-level "finished" event bit is set */
        status =  NU_Retrieve_Events(&RunLevel_Finished_Event, (1UL << current_runlevel),
                                     NU_OR,
                                     &event_bits, NU_NO_SUSPEND);

        /* Ensure this event retrieval successful */
        if ((status == NU_SUCCESS) || (status == NU_NOT_PRESENT))
        {
            /* Determine if the current run-level is finshed (ie event bit is set) */
            if (event_bits)
            {
                /* Start the next run-level */
                RunLevel_Start(current_runlevel + 1);

                /* Set return status to NU_SUCCESS */
                status = NU_SUCCESS;
            }
            else
            {
                /* Set return status to show the current run-level is still being run */
                status = NU_RUNLEVEL_IN_PROGRESS;
            }
        }
    }
    else
    {
        /* Set return status to not present */
        status = NU_NOT_PRESENT;
    }

    /* Return results of operation */
    return (status);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_RunLevel_0_Init
*
*   DESCRIPTION
*
*       Can be used to manually initialize any run-level 0 component
*       or all run-level 0 components from an application.
*
*   CALLED BY
*
*       Misc
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       compregpath                         Component registry path
*                                           If NULL, initializes all
*                                           components in Run-Level 0
*
*   OUTPUTS
*
*       NU_SUCCESS                          Run-level 0 component(s)
*                                           initialization completed
*       NU_RUNLEVEL_IN_PROGRESS             Run-level 0 initialization
*                                           has already started
*       NU_NOT_PRESENT                      The specified component
*                                           is not a run-level 0 component
*
***********************************************************************/
STATUS  NU_RunLevel_0_Init(const CHAR * compregpath)
{
    STATUS      status;


    /* Check to see if run-level 0 is already in progress */
    status =  NU_RunLevel_Status(0);

    /* Check to see if this run-level has not started */
    if (status == NU_RUNLEVEL_NOT_STARTED)
    {
        /* Check if "all" run-level 0 components should be initialized */
        if (compregpath == NU_NULL)
        {
            /* Start initialization of the entire run-level */
            RunLevel_Start(0);
            
            /* Set return status appropriately */
            status = NU_SUCCESS;
        }
        else
        {
            /* Just start the specified component */
            status = NU_RunLevel_Component_Control(compregpath, RUNLEVEL_START);
        }
    }

    /* Resturn results of operation */
    return (status);
}

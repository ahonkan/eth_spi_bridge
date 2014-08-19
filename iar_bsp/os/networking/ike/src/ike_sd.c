/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_sd.c
*
* COMPONENT
*
*       IKE - Shutdown
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE shutdown
*       API function.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Shutdown
*       IKE_Terminate_Tasks
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*       ike_buf.h
*       ike_evt.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"
#include "networking/ike_buf.h"
#include "networking/ike_evt.h"

/* Local function prototypes. */
STATIC STATUS IKE_Terminate_Tasks(VOID);

/*************************************************************************
*
* FUNCTION
*
*       IKE_Shutdown
*
* DESCRIPTION
*
*       This function prepares IKE for a shutdown and de-initializes
*       all the IKE modules which were initialized by the
*       IKE_Initialize API function. IKE can be restarted after
*       this call by calling IKE_Initialize again.
*
*       This is an IKE API function.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_STATE       IKE daemon is not in running
*                               state to allow shutdown.
*
*************************************************************************/
STATUS IKE_Shutdown(VOID)
{
    STATUS          status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    IKE_DEBUG_LOG("Shutting down IKE");

    /* Terminate all running IKE tasks. */
    status = IKE_Terminate_Tasks();

    if(status == NU_SUCCESS)
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Delete the terminated task. */
            status = NU_Delete_Task(&IKE_Data.ike_task_cb);

            if(status == NU_SUCCESS)
            {
                /* Deallocate task stack memory. */
                if(NU_Deallocate_Memory(IKE_Data.ike_task_stack) !=
                   NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to delete IKE task",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* De-initialize the events component. This would
                 * also terminate the IKE event handler task.
                 */
                status = IKE_Deinitialize_Events();

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to de-initialize events",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* De-initialize the IKE pre-shared keys Database. */
                status = IKE_Deinitialize_Preshared_Keys();

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to de-initialize PSKs",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
#endif

            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* De-initialize the IKE Groups Database component. */
                status = IKE_Deinitialize_Groups();

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to de-initialize groups",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* De-initialize the IKE Buffer component. This component
                 * must be de-initialized only after the Groups component
                 * has been de-initialized.
                 */
                status = IKE_Deinitialize_Buffers();

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to de-initialize IKE buffers",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Delete the IKE wait events group. */
                status = NU_Delete_Event_Group(&IKE_Data.ike_event_group);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to delete event group",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Set the system memory pool to NULL. */
            IKE_Data.ike_memory = NU_NULL;

            /* Release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IKE semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to terminate the IKE tasks",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(status == NU_SUCCESS)
    {
        /* Delete the IKE Semaphore. */
        status = NU_Delete_Semaphore(&IKE_Data.ike_semaphore);

        if(status == NU_SUCCESS)
        {
            /* Finally, mark daemon as stopped. */
            IKE_Daemon_State = IKE_DAEMON_STOPPED;
        }

        else
        {
            NLOG_Error_Log("Failed to delete the IKE semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status. */
    return (status);

} /* IKE_Shutdown */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Terminate_Tasks
*
* DESCRIPTION
*
*       This is a utility function which terminates the
*       running IKE tasks. It is called when IKE is shutting
*       down.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_STATE       IKE daemon is not in running
*                               state to allow task termination.
*
*************************************************************************/
STATIC STATUS IKE_Terminate_Tasks(VOID)
{
    STATUS          status = NU_SUCCESS;

    /* Make sure daemon is in running state before shutdown. */
    if(IKE_Daemon_State != IKE_DAEMON_RUNNING)
    {
        status = IKE_INVALID_STATE;
    }

    else
    {
        /* Request IKE task to shutdown. */
        IKE_Daemon_State = IKE_DAEMON_STOPPING_LISTEN;

        /* Wait for IKE listen task to terminate. */
        for(;;)
        {
            /* Wait some time before polling daemon state. */
            NU_Sleep(TICKS_PER_SECOND);

            /* Check whether the task has terminated yet. The daemon
             * state would be updated by the task before it
             * terminates.
             */
            if(IKE_Daemon_State != IKE_DAEMON_STOPPING_LISTEN)
            {
                /* Give task enough time to terminate. */
                NU_Relinquish();

                /* Wait for IKE event handler task to terminate. */
                for(;;)
                {
                    /* Wait some time before polling daemon state. */
                    NU_Sleep(TICKS_PER_SECOND);

                    /* Check whether the task has terminated yet. The
                     * daemon state would be updated by the task before
                     * it terminates.
                     */
                    if(IKE_Daemon_State != IKE_DAEMON_STOPPING_EVENTS)
                    {
                        /* Give task enough time to terminate. */
                        NU_Relinquish();

                        break;
                    }
                }

                break;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Terminate_Tasks */

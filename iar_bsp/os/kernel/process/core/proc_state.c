/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       proc_state.c
*
* COMPONENT
*
*       PROC - Nucleus Processes
*
* DESCRIPTION
*
*       This file contains the primary internal API for controlling
*       process states, including the kernel state thread
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       PROC_Transition
*       PROC_Kernel_Task_Entry
*       PROC_Stopped
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       proc_core.h
*       thread_control.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "proc_core.h"
#include "os/kernel/plus/core/inc/thread_control.h"

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
BOOLEAN PROC_Mapped_Memory_In_Use(PROC_CB* process);
VOID PROC_Mapped_Memory_Clean_Up(PROC_CB* process);
#endif

/*************************************************************************
*
* FUNCTION
*
*       PROC_Transition
*
* DESCRIPTION
*
*       Transitions to a new state based on state passed
*
* INPUTS
*
*       id                              Process ID
*       trans_state                     Transitional "ing" state
*       trans_val                       Value that can be used by a given transition
*       suspend                         Suspension to be used on queues
*
* OUTPUTS
*
*       NU_SUCCESS                      Indicates successful operation
*       NU_INVALID_STATE                Cannot transition to new state
*       NU_INVALID_PROCESS              Process control block is invalid
*       NU_PROCESS_IN_TRANSITION        A process transition is already
*                                       being attempted
*       NU_SYMBOLS_IN_USE               Process being unloaded or killed
*                                       has symmbols being used by other
*                                       processes
*       NU_INVALID_OPERATION            Calls to transition can not be
*                                       made during process exit
*
*************************************************************************/
STATUS PROC_Transition(INT id, PROC_STATE trans_state, INT trans_val, UNSIGNED suspend)
{
    STATUS      status = NU_SUCCESS;
    UNSIGNED    message;
    UNSIGNED    size;
    PROC_CB    *process;

    /* Obtain the process control block */
    process = PROC_Get_Pointer(id);
    if (process == NU_NULL)
    {
        /* ID invalid */
        status = NU_INVALID_PROCESS;
    }

    if ((status == NU_SUCCESS) && (process -> exit_protect == NU_TRUE))
    {
        /* This process cannot transition to another state while protected */
        status = NU_INVALID_OPERATION;
    }

    if ((status == NU_SUCCESS) && (trans_state == PROC_KILLING_STATE) &&
        (process -> state == PROC_FAILED_STATE) && (process -> root_task.tc_id != TC_TASK_ID))
    {
        /* Update the transition state to unload as there isn't a root task
           available to complete a full kill transition */
        trans_state = PROC_UNLOADING_STATE;
    }

    if (status == NU_SUCCESS)
    {
        /* Lock access to the mutex */
        status = NU_Obtain_Semaphore(&(process -> semaphore), NU_NO_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        /* Error check the states */
        switch(trans_state)
        {
            case PROC_LOADING_STATE:

                /* A process must be created before it can be loaded */
                if (process -> state != PROC_CREATED_STATE)
                {
                    status = NU_INVALID_STATE;
                }

                break;

            case PROC_STARTING_STATE:

                /* To transition to a "starting" state the process
                   must already be in a "stopped" state */
                if (process -> state != PROC_STOPPED_STATE)
                {
                    status = NU_INVALID_STATE;
                }

                break;

            case PROC_DEINITIALIZING_STATE:

                /* To transition to a "stopping" state the process
                   must already be in a "started" state */
                if (process -> state != PROC_STARTED_STATE)
                {
                    status = NU_INVALID_STATE;
                }
                /* Processes with symbols in use cannot be stopped */
                else if (PROC_Symbols_In_Use(process) == NU_TRUE)
                {
                    status = NU_SYMBOLS_IN_USE;
                }
#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
                /* Processes with shared memory cannot be stopped */
                else if (PROC_Mapped_Memory_In_Use(process) == NU_TRUE)
                {
                    status = NU_MEMORY_IS_SHARED;
                }
#endif
                else
                {
                    /* Check the transition value passed in to see if
                       an abort() call was made or just "normal" exit */
                    if (trans_val == EXIT_ABORT)
                    {
                        /* Set process abort flag */
                        process -> abort_flag = NU_TRUE;
                    }
                    else
                    {
                        /* Set process exit code */
                        process -> exit_code = trans_val;
                    }
                }

                break;

            case PROC_STOPPING_STATE:

                /* Stopping state is a special case state that comes in when
                   the the deinit function is complete and the process
                   is ready to be stopped */
                if (process -> state != PROC_DEINITIALIZING_STATE)
                {
                    status = NU_INVALID_STATE;
                }

                break;

            case PROC_UNLOADING_STATE:

                /* To transition to an "unloading" state the process
                   must already be in a "stopped" or "failed" state */
                if ((process -> state != PROC_STOPPED_STATE) && (process -> state != PROC_FAILED_STATE))
                {
                    status = NU_INVALID_STATE;
                }

                break;

            case PROC_KILLING_STATE:

                /* For proper handling of killing a process a state will be
                   set to "killing" the state can be "failed", "stopped", "started",
                   or "killing" */
                if ((process -> state != PROC_STOPPED_STATE) && (process -> state != PROC_FAILED_STATE) &&
                    (process -> state != PROC_STARTED_STATE) && (process -> state != PROC_KILLING_STATE))
                {
                    status = NU_INVALID_STATE;
                }
                /* Processes with symbols in use cannot be killed */
                else if (PROC_Symbols_In_Use(process) == NU_TRUE)
                {
                    status = NU_SYMBOLS_IN_USE;
                }
#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
                /* Processes with shared memory cannot be killed */
                else if (PROC_Mapped_Memory_In_Use(process) == NU_TRUE)
                {
                    status = NU_MEMORY_IS_SHARED;
                }
#endif

                break;

            default:
                /* The requested transition state is not supported */
                status = NU_INVALID_STATE;

                break;
        }

        if (status == NU_SUCCESS)
        {
            /* Save the previous state */
            process -> prev_state = process -> state;

            /* Prepare the new state */
            process -> state = trans_state;
        }

        /* Release the mutex */
        (VOID)NU_Release_Semaphore(&(process -> semaphore));
    }
    else if (status == NU_UNAVAILABLE)
    {
        /* Transition is occurring return more specific error */
        status = NU_PROCESS_IN_TRANSITION;
    }

    if (status == NU_SUCCESS)
    {
        /* Setup the message */
        message = (UNSIGNED) id;

        /* Send message to kernel thread to handle next transition of the process */
        status = NU_Send_To_Queue(&(PROC_Kernel_CB -> queue), &message, PROC_MSG_SIZE, suspend);
    }

    /* Return status value from the kernel thread, in the case of
       stopping state the original caller is already waiting based
       on the first call to PROC_Transition with a deinit state */
    if ((status == NU_SUCCESS) && (trans_state != PROC_STOPPING_STATE))
    {
        status = NU_Receive_From_Queue(&(process -> queue), &message, PROC_MSG_SIZE,
                                       &size, suspend);

        if (status == NU_SUCCESS)
        {
            /* Return the status value from the kernel thread */
            status = (STATUS)message;
        }
        else if (((trans_state == PROC_KILLING_STATE) || (trans_state == PROC_UNLOADING_STATE)) && (status == NU_INVALID_QUEUE))
        {
            /* Invalid queue error comes from the deletion of the queue before trying to access it */
            status = NU_SUCCESS;
        }
        else if ((trans_state == PROC_KILLING_STATE) && (status == NU_QUEUE_DELETED))
        {
            /* Deleted queue error comes from the queue being deleted while suspended on it */
            status = NU_SUCCESS;
        }
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_Kernel_Task_Entry
*
*   DESCRIPTION
*
*       Primary task to handle state transitions
*
*   INPUTS
*
*       argc
*       argv
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID PROC_Kernel_Task_Entry(UNSIGNED argc, VOID * argv)
{
    STATUS      status;
    UNSIGNED    message;
    UNSIGNED    size;
    PROC_CB    *process;
    BOOLEAN     send_queue;
    BOOLEAN     release_mutex;

    for (;;)
    {
        /* Wait on state transition messages from user processes */
        status = NU_Receive_From_Queue(&(PROC_Kernel_CB -> queue), &message, PROC_MSG_SIZE,
                                       &size, NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* In most cases the status needs to be returned to the caller
               through the queue and the mutex needs to be released.  Set
               these to happen for all cases, when appropriate these will
               be set to not occur */
            send_queue = NU_TRUE;
            release_mutex = NU_TRUE;

            /* When a queue comes in the message will be the process ID */
            process = PROC_Get_Pointer(message);
            if (process != NU_NULL)
            {
                /* Lock access to the mutex */
                status = NU_Obtain_Semaphore(&(process -> semaphore), NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* In the case of killing the process, it may already be stopped.
                       In that case there is no need to wait for the deinit call to
                       complete, simply unload it */
                    if ((process -> state == PROC_KILLING_STATE) &&
                        ((process -> prev_state == PROC_STOPPED_STATE) ||
                         (process -> prev_state == PROC_KILLING_STATE)))
                    {
                        /* Update the state */
                        process -> state = PROC_UNLOADING_STATE;
                    }

                    /* The current state of the process will dictate
                       which state is next */
                    switch(process -> state)
                    {
                        case PROC_LOADING_STATE:
                        {
                            /* The loading state is the only state that can transition
                               to more than one state from here.  If a failure is detected
                               by the load state will be come failed */
                            message = PROC_Load(process);
                            if (message == NU_SUCCESS)
                            {
                                /* Loading is complete update the state */
                                process -> state = PROC_STOPPED_STATE;

#if (CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT == NU_TRUE)

                                /* Post notification that a process has
                                   been loaded successfully. */
                                (*PROC_Load_Notify_Ptr)(process -> name,
                                                        (UINT)process -> id,
                                                        (UINT)process -> load_addr);

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT */

                            }
                            else
                            {
                                /* Loading failed */
                                process -> state = PROC_FAILED_STATE;
                            }

                            break;
                        }
                        case PROC_STARTING_STATE:
                        {
                            /* Transition to a started state */
                            message = PROC_Start(process);
                            if (message == NU_SUCCESS)
                            {
                                /* The root thread for the process has been started */
                                process -> state = PROC_STARTED_STATE;
                            }
                            else
                            {
                                /* Something went wrong put the state back to stopped */
                                process -> state = PROC_STOPPED_STATE;
                            }

                            break;
                        }
                        case PROC_DEINITIALIZING_STATE:
                        case PROC_KILLING_STATE:
                        {
                            /* Deinitializing and killing states will require waiting
                               on the deinit function to complete, killing will continue
                               to finish unloading while the deinitializing state will pass
                               through to stopping state and updating of this state will
                               occur at that time */
                            message = PROC_Stop(process);
                            if (message != NU_SUCCESS)
                            {
                                /* With the stop call failing this process is now
                                   in an unknown state.  Mark as failed */
                                process -> state = PROC_FAILED_STATE;
                            }

                            /* If the return value is success then the deinit call has been
                               made and no return value can come from here, but if there is
                               a failure that value should be returned */
                            if (message == NU_SUCCESS)
                            {
                                /* Don't return value with the queue, for deinitializing
                                   that will occur from the stopping state and for killing
                                   will not occur at all */
                                send_queue = NU_FALSE;
                            }

                            break;
                        }
                        case PROC_STOPPING_STATE:
                        {
                            /* Updating the state is all that is
                               needed here */
                            process -> state = PROC_STOPPED_STATE;

                            /* Return success */
                            message = NU_SUCCESS;

                            break;
                        }
                        case PROC_UNLOADING_STATE:
                        {
                            message = PROC_Unload(process);

#if (CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT == NU_TRUE)

                            /* Post notification if a process has
                               successfully been unloaded.  In the case
                               that an unload fails the process is still
                               considered to be in the system and so no
                               notification should be posted. */
                            if (message == NU_SUCCESS)
                            {
                                (*PROC_Unload_Notify_Ptr)(process -> id);
                            }

#endif /* CFG_NU_OS_KERN_PROCESS_CORE_DEV_SUPPORT */

                            /* Don't return value with the queue, it won't exist */
                            send_queue = NU_FALSE;

                            /* Don't release the mutex, it won't exist */
                            release_mutex = NU_FALSE;

                            break;
                        }
                        default:
                        {
                            /* The state of the process is not supported
                               update the state to failed and return an
                               error */
                            process -> state = PROC_FAILED_STATE;
                            message = NU_UNAVAILABLE;
                            break;
                        }
                    }

                    /* Stopping state will send a message to the queue
                       when the deinit has completed, unloading state
                       will not have a valid process control block */
                    if (send_queue == NU_TRUE)
                    {
                        /* Return status value from the kernel thread */
                        (VOID)NU_Send_To_Queue(&(process -> queue), &message, PROC_MSG_SIZE, NU_NO_SUSPEND);
                    }

                    /* Release the mutex unless this in being unloaded, in that
                       case the mutex would have been deleted  */
                    if (release_mutex == NU_TRUE)
                    {
                        /* Release the mutex */
                        (VOID)NU_Release_Semaphore(&(process -> semaphore));
                    }
                }
            }
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       PROC_Stopped
*
*   DESCRIPTION
*
*       Called by the task shell function and finalizes stopping a process
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID PROC_Stopped(VOID)
{
    PROC_CB *process;

    /* Get the process control block from the task */
    process = (PROC_CB *)(TCD_Execute_Task -> tc_process);

    /* Determine if this is the root task, if this is
       a root task and the state is stopping update
       the state and send successful message to the
       process state queue */
    if (TCD_Execute_Task == &(process -> root_task))
    {
        /* Determine if this is main or deinit finishing.
           Nothing to do in the case of a finished main, but
           the state machine needs to be notified about deinit
           completion */
        if ((process -> state == PROC_DEINITIALIZING_STATE) ||
            (process -> state == PROC_KILLING_STATE))
        {
            if (process -> pool != NU_NULL)
            {
                /* Delete the memory pool, memory in the pool will
                   be unaffected and will not need to be deallocated.
                   If the process is restarted the pool will be
                   recreated and empty */
                (VOID)NU_Delete_Memory_Pool(process -> pool);
            }

#ifdef CFG_NU_OS_KERN_PROCESS_MEM_MGMT_ENABLE
            /* If there are any regions from memory mapping left in the
               process remove them */
            PROC_Mapped_Memory_Clean_Up(process);
#endif

            /* Process exit is complete, clear protection */
            process -> exit_protect = NU_FALSE;

            if (process -> state == PROC_DEINITIALIZING_STATE)
            {
                /* Message should contain the process ID to complete
                   the process transition, this must suspend on the
                   kernel queue send to ensure the transition completes */
                PROC_Transition(process -> id, PROC_STOPPING_STATE, 0, NU_SUSPEND);
            }
            else
            {
                /* In the case of killing a non-stopped process call the kill
                   a second time to finish unloading */
                PROC_Transition(process -> id, PROC_KILLING_STATE, 0, NU_SUSPEND);
            }
        }
    }
}

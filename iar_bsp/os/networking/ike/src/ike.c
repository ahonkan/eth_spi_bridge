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
*       ike.c
*
* COMPONENT
*
*       IKE - General
*
* DESCRIPTION
*
*       This file implements the general functions of IKE.
*
* DATA STRUCTURES
*
*       IKE_Data                Global IKE data used by various
*                               IKE components.
*       IKE_Daemon_State        Current state of the IKE service.
*
* FUNCTIONS
*
*       IKE_Initialize
*       IKE_Create_Service_Task
*       IKE_Get_Exchange_Index
*       IKE_Get_Algo_Index
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*       ike_buf.h
*       ike_evt.h
*       ike_task.h
*       rand.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"
#include "networking/ike_buf.h"
#include "networking/ike_evt.h"
#include "networking/ike_task.h"
#include "openssl/rand.h"

/* Local function prototypes. */
STATIC STATUS IKE_Create_Service_Task(VOID);

/**** Define IKE global variables. ****/

/* Common data used throughout IKE. */
IKE_STRUCT IKE_Data;

/* Flag variable to specify current state of the IKE daemon. */
UINT8 IKE_Daemon_State = IKE_DAEMON_STOPPED;

/*************************************************************************
*
* FUNCTION
*
*       IKE_Initialize
*
* DESCRIPTION
*
*       This function initializes the IKE module and creates
*       the IKE task which handles all IKE requests.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *memory_pool            Memory pool used by IKE for
*                               dynamic memory allocations.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_INVALID_SIZE         Task stack not large enough.
*       NU_INVALID_PREEMPT      Task preempt parameter is invalid.
*       NU_INVALID_START        Task auto-start parameter is invalid.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      The parameter is invalid.
*       IKE_INVALID_STATE       IKE daemon is not in stopped
*                               state to allow initialization.
*
*************************************************************************/
STATUS IKE_Initialize(NU_MEMORY_POOL *memory_pool)
{
    STATUS          status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the memory pool pointer is valid. */
    if(memory_pool == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Check to see if the daemon is already running. */
    else if(IKE_Daemon_State == IKE_DAEMON_RUNNING)
    {
        status = IKE_ALREADY_RUNNING;
    }

    /* Make sure daemon is in stopped state before initialization. */
    else if(IKE_Daemon_State != IKE_DAEMON_STOPPED)
    {
        status = IKE_INVALID_STATE;
    }

    else
    {
        /* Set the system memory pool. */
        IKE_Data.ike_memory = memory_pool;

        /* Set socket to a negative value to indicate
         * that it is not allocated.
         */
        IKE_Data.ike_socket = -1;

        /* Generate a random SPI. Return value can be ignored. */
        if (NU_TRUE != RAND_bytes((UINT8 *)&IKE_Data.ike_spi_index, sizeof(UINT32)))
        {
            NLOG_Error_Log("Unable to generate random data.",
                           NERR_FATAL, __FILE__, __LINE__);
        }

        if(IKE_Data.ike_spi_index <= IPSEC_SPI_END)
        {
            /* Ensure that it is greater than IPSEC_SPI_END. */
            IKE_Data.ike_spi_index = (IKE_Data.ike_spi_index |
                                      (IPSEC_SPI_END + 1));
        }

        /* Create IKE Semaphore. */
        status = NU_Create_Semaphore(&IKE_Data.ike_semaphore,
                                     "IKE_SEMA", 1, NU_FIFO);

        if(status == NU_SUCCESS)
        {
            /* Create the IKE event group. */
            status = NU_Create_Event_Group(&IKE_Data.ike_event_group,
                                           "IKE_EVNT");

            if(status == NU_SUCCESS)
            {
                /* Initialize the IKE Buffer component. */
                status = IKE_Initialize_Buffers();

                if(status == NU_SUCCESS)
                {
                    /* Initialize the IKE Groups Database component. */
                    status = IKE_Initialize_Groups();

                    if(status == NU_SUCCESS)
                    {
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
                        /* Initialize the IKE pre-shared keys Database. */
                        status = IKE_Initialize_Preshared_Keys();

                        if(status == NU_SUCCESS)
#endif
                        {
                            /* Initialize the events component. This would
                             * create an IKE event handler task to process
                             * timer events.
                             */
                            status = IKE_Initialize_Events();

                            if(status == NU_SUCCESS)
                            {
                                /* Create the IKE task. */
                                status = IKE_Create_Service_Task();

                                if(status == NU_SUCCESS)
                                {
                                    /* Change daemon state to running. */
                                    IKE_Daemon_State = IKE_DAEMON_RUNNING;
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                        "Unable to create IKE task",
                                        NERR_FATAL, __FILE__, __LINE__);

                                    if(IKE_Deinitialize_Events()
                                       != NU_SUCCESS)
                                    {
                                        NLOG_Error_Log(
                                          "Failed to de-initialize events",
                                          NERR_SEVERE, __FILE__, __LINE__);
                                    }
                                }
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to initialize IKE events",
                                    NERR_FATAL, __FILE__, __LINE__);
                            }

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
                            /* If any of the above calls failed. */
                            if(status != NU_SUCCESS)
                            {
                                if(IKE_Deinitialize_Preshared_Keys()
                                   != NU_SUCCESS)
                                {
                                    NLOG_Error_Log(
                                        "Failed to de-initialize IKE PSK",
                                        NERR_SEVERE, __FILE__, __LINE__);
                                }
                            }
#endif
                        }

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
                        else
                        {
                            NLOG_Error_Log(
                                "Failed to initialize pre-shared keys",
                                NERR_FATAL, __FILE__, __LINE__);
                        }
#endif

                        /* If any of the above calls failed. */
                        if(status != NU_SUCCESS)
                        {
                            /* De-initialize IKE groups. */
                            if(IKE_Deinitialize_Groups() != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to de-initialize IKE groups",
                                    NERR_SEVERE, __FILE__, __LINE__);
                            }
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Unable to initialize IKE groups",
                                       NERR_FATAL, __FILE__, __LINE__);
                    }

                    /* If any of the above calls failed. */
                    if(status != NU_SUCCESS)
                    {
                        /* De-initialize IKE buffers. */
                        if(IKE_Deinitialize_Buffers() != NU_SUCCESS)
                        {
                            NLOG_Error_Log(
                                "Failed to de-initialize IKE buffers",
                                NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }
                }

                else
                {
                    NLOG_Error_Log("Unable to initialize IKE buffers",
                                   NERR_FATAL, __FILE__, __LINE__);
                }

                /* If any of the above calls failed. */
                if(status != NU_SUCCESS)
                {
                    /* Delete the IKE event group. */
                    if(NU_Delete_Event_Group(&IKE_Data.ike_event_group)
                       != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to delete IKE event group",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Unable to create IKE event group",
                               NERR_FATAL, __FILE__, __LINE__);
            }

            /* If any of the above calls failed. */
            if(status != NU_SUCCESS)
            {
                /* Delete the IKE semaphore created above. */
                if(NU_Delete_Semaphore(&IKE_Data.ike_semaphore)
                   != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to delete IKE semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Unable to create IKE semaphore",
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status. */
    return (status);

} /* IKE_Initialize */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Create_Service_Task
*
* DESCRIPTION
*
*       This is a utility function which creates the IKE task
*       responsible for handling all IKE messages.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_INVALID_SIZE         Task stack not large enough.
*       NU_INVALID_PREEMPT      Task preempt parameter is invalid.
*       NU_INVALID_START        Task auto-start parameter is invalid.
*       NU_NO_MEMORY            Not enough memory available.
*
*************************************************************************/
STATIC STATUS IKE_Create_Service_Task(VOID)
{
    STATUS          status;
    VOID            *pointer;

    /* Allocate memory for the task stack. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&pointer,
                                IKE_TASK_STACK_SIZE, NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        pointer = TLS_Normalize_Ptr(pointer);

        /* Create IKE task which will handle all IKE messages. */
        status = NU_Create_Task(&IKE_Data.ike_task_cb, "NU_IKE",
                                IKE_Task_Entry, 0, NU_NULL, pointer,
                                IKE_TASK_STACK_SIZE, IKE_TASK_PRIORITY,
                                IKE_TASK_TIME_SLICE, IKE_TASK_PREEMPT,
                                NU_NO_START);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to create IKE task",
                           NERR_FATAL, __FILE__, __LINE__);

            /* Deallocate the memory allocated above. */
            if(NU_Deallocate_Memory(pointer) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate IKE memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            /* Store stack pointer in the global IKE data. */
            IKE_Data.ike_task_stack = pointer;

            /* Start the IKE task. */
            status = NU_Resume_Task(&IKE_Data.ike_task_cb);

            if(status == NU_SUCCESS)
            {
                /* Log debug message. */
                IKE_DEBUG_LOG("Created IKE task");
            }

            else
            {
                NLOG_Error_Log("Failed to start IKE task",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Unable to allocate IKE task stack",
                       NERR_FATAL, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Create_Service_Task */

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Get_Exchange_Index
*
* DESCRIPTION
*
*       This function takes a Phase 2 exchange message ID and
*       returns its index in the message ID and exchange
*       status arrays.
*
*       The caller is responsible for obtaining the IKE semaphore
*       before calling this function.
*
* INPUTS
*
*       msg_id                  Message ID to be searched. Set
*                               this to IKE_BLANK_MSG_ID to get
*                               a free slot in the array.
*       *index                  On return, this contains an
*                               index into the message ID and
*                               exchange status arrays.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      The parameters are invalid.
*       IKE_INDEX_NOT_FOUND     Specified message ID not found.
*
*************************************************************************/
STATUS IKE_Get_Exchange_Index(UINT32 msg_id, INT *index)
{
    STATUS          status = IKE_INDEX_NOT_FOUND;
    INT             i;

#if (IKE_DEBUG == NU_TRUE)
    if(index == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Loop for all items in the message ID array. */
    for(i = 0; i < IKE_MAX_WAIT_EVENTS; i++)
    {
        /* Check if a match is found. */
        if(IKE_Data.ike_msg_ids[i] == msg_id)
        {
            /* Return the current index. */
            *index = i;

            /* Set status to success and stop the search. */
            status = NU_SUCCESS;
            break;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Exchange_Index */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_Get_Algo_Index
*
* DESCRIPTION
*
*       This function returns the IKE encryption/hash/signature
*       algorithm array's index based on the IKE algorithm ID.
*
* INPUTS
*
*       ike_id                  The IKE hash algorithm ID.
*       *algos                  Pointer to IKE algorithms array
*                               of either hash, signature or
*                               encryption algorithms.
*       algos_size              Size of a single algorithm
*                               structure.
*       algos_no                Number of items in algorithm
*                               array.
*       *index                  On return, this contains an
*                               index of the algorithm array.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_UNSUPPORTED_ALGO    Algorithm is not supported by IKE.
*
*************************************************************************/
STATUS IKE_Get_Algo_Index(UINT16 ike_id, const VOID *algos, INT algos_size,
                          UINT16 algos_no, UINT16 *index)
{
    STATUS          status = IKE_UNSUPPORTED_ALGO;
    UINT16          i;

    /* Loop for all algorithms included in IKE. */
    for(i = 0; i < algos_no; i++)
    {
        /* Check if a match is found. */
        if(ike_id == (UINT16)((IKE_HASH_ALGO*)algos)->ike_algo_identifier)
        {
            /* Set the pointer to be returned. */
            *index = i;

            /* Set status to success and break out of loop. */
            status = NU_SUCCESS;
            break;
        }

        /* Move the algorithm pointer to next item in array. */
        algos = ((UINT8*)algos) + algos_size;
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Algo_Index */

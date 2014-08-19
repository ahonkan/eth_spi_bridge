/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp_dc.c
*
*   COMPONENT
*
*       DC - Direct Cable Protocol
*
*   DESCRIPTION
*
*       This file contains services for the Direct Connect Cable
*       Protocol for connecting PPP client and server through a
*       direct cable between the them.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PPP_DC_Initialize
*       PPP_DC_Hangup
*       PPP_DC_Wait
*       PPP_DC_Connect
*       PPP_DC_Get_String
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       ppp.h
*       ppp_dc.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"
#include "drivers/ppp.h"

#if (INCLUDE_PPP_DC_PROTOCOL == NU_TRUE)
#include "drivers/ppp_dc.h"

/*************************************************************************
* FUNCTION
*
*     PPP_DC_Initialize
*
* DESCRIPTION
*
*     This function initializes the Direct Cable and HDLC layer of the
*     protocol stack.
*
* INPUTS
*
*     *dev_ptr                  Device connected with the Direct Cable.
*
* OUTPUTS
*
*     NU_SUCCESS                Initialization successful.
*     NU_INVALID_LINK           Device passed in was invalid.
*
*************************************************************************/
STATUS PPP_DC_Initialize(DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declaring Variable. */
    STATUS       status;
    LINK_LAYER   *link_layer;
    
    /* Initialize the HDLC layer. */
    status = HDLC_Init(dev_ptr);

    /* If HDLC initialization is successful.*/
    if (status == NU_SUCCESS)
    {
        link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
        
        /* Store Direct Cable layer information. */
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.init = MDM_Init;
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.disconnect = PPP_DC_Hangup;
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.passive = PPP_DC_Wait;
        ((LINK_LAYER *)dev_ptr->dev_link_layer)->hwi.connect = PPP_DC_Connect;

        /* Initialize the MODEM module. */
        status = MDM_Init(dev_ptr);
    }
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to initialize the modem in PPP_DC_Initialize",NERR_SEVERE, __FILE__, __LINE__);
    }
    else
    {
        status = NU_Allocate_Memory(PPP_Memory, &(link_layer->ppp_serial_rx_task_mem), PPP_TASK_STACK_SIZE, NU_NO_SUSPEND);
        
        if (status == NU_SUCCESS)
        {
            status = NU_Create_Task(&(link_layer->serial_rx_task), "ser_rx",
                                    Serial_Rx_Task_Entry, 0, dev_ptr, (link_layer->ppp_serial_rx_task_mem),
                                    PPP_TASK_STACK_SIZE, PPP_TASK_PRIORITY, PPP_TASK_TIME_SLICE, PPP_TASK_PREEMPT, NU_NO_START);
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to create the task for the Modem to receive data in PPP_DC_Initialize",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            else
            {      
                /* Start the Serial Rx task. */
                status = NU_Resume_Task(&(link_layer->serial_rx_task));

                if(status != NU_SUCCESS)
                {
                    return (status);
                }
                
#if (HDLC_POLLED_TX == NU_FALSE)

                status = NU_Allocate_Memory(PPP_Memory, &(link_layer->ppp_serial_tx_task_mem), PPP_TASK_STACK_SIZE, NU_NO_SUSPEND);
                
                if(status == NU_SUCCESS)
                {
                    status = NU_Create_Task(&(link_layer->serial_tx_task), "ser_tx",
                                            Serial_Tx_Task_Entry, 0, dev_ptr, (link_layer->ppp_serial_tx_task_mem),
                                            PPP_TASK_STACK_SIZE, PPP_TASK_PRIORITY, PPP_TASK_TIME_SLICE, PPP_TASK_PREEMPT, NU_NO_START);
                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to create the task for the Modem to transmit data in PPP_DC_Initialize",
                                        NERR_SEVERE, __FILE__, __LINE__);
                    }
                    else
                    {
                        /* Start the Serial Tx task. */
                        status = NU_Resume_Task(&(link_layer->serial_tx_task));

                        if(status != NU_SUCCESS)
                        {
                            return (status);
                        }
                    }
                }
                else
                {
                    NLOG_Error_Log("Failed to allocate memory for serial_tx_task in PPP_DC_Initialize",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }    
#endif
            } 
        }
        else
        {
             NLOG_Error_Log("Failed to allocate memory for serial_rx_task in PPP_DC_Initialize",
                             NERR_SEVERE, __FILE__, __LINE__);
        } 
    }

    /* Return status.*/
    return (status);

} /* PPP_DC_Initialize. */

/*************************************************************************
* FUNCTION
*
*     PPP_DC_Hangup
*
* DESCRIPTION
*
*     This function hangs up the Direct Cable connection.
*
* INPUTS
*
*     *link_ptr                 Pointer to the PPP link to hang-up.
*     wait_for_modem            Should routine wait for hang-up
*                               completion.
*
* OUTPUTS
*
*     NU_SUCCESS                On successful hang-up.
*
*************************************************************************/
STATUS PPP_DC_Hangup(LINK_LAYER *link_ptr, UINT8 wait_for_modem)
{
    /* No need to do anything if the modem is already disconnected. */
    if(link_ptr->hwi.state != INITIAL)
    {
        /* First things first. Mark the device as not up. This
           will stop IP packets from trying to be transmitted. */
        link_ptr->hwi.dev_ptr->dev_flags &= (~DV_UP);

        /* Update the status of the PPP connection. */
        link_ptr->connection_status = NU_PPP_HANGING_UP;

        /* Change to the initial state after the modem has hung-up. */
        link_ptr->hwi.state = INITIAL;

        /* If caller not waiting for modem to hang-up. */
        if(!wait_for_modem)
        {
            /* Clear any pending hang-up events. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT,
                          (UNSIGNED)link_ptr->hwi.dev_ptr, MDM_HANGUP);
        }
    }

    return (NU_SUCCESS);

} /* PPP_DC_Hangup */

/*************************************************************************
* FUNCTION
*
*     PPP_DC_Wait
*
* DESCRIPTION
*
*     Waits for a direct Cable Connect Cable Connection.
*
* INPUTS
*
*     *dev_ptr                  Device connected with the Direct Cable.
*
* OUTPUTS
*
*     NU_SUCCESS                On success.
*     NU_INVALID_LINK           Device passed in was invalid.
*     NU_PPP_ATTEMPT_ABORTED    Attempt is aborted.
*
*************************************************************************/
STATUS PPP_DC_Wait(DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declaring Variables. */
    LINK_LAYER      *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    STATUS          status;
    CHAR            dstring[PPP_DC_CMD_MAX_SIZE];

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Reset the UART. */
    SDC_Reset(link_layer->uart);

    /* Set the state of the connection. */
    link_layer->hwi.state = STARTING;

    /* Change to terminal mode. */
    MDM_Change_Communication_Mode(MDM_TERMINAL_COMMUNICATION, dev_ptr);

    do
    {
        /* Receive a command from a client. */
        status = PPP_DC_Get_String(dstring, dev_ptr, PPP_DC_SERVER);

    /* Loop until we don't receive a "CLIENT" command or the attempt
     * is aborted.
     */
    }while ( (strcmp(dstring, PPP_DC_CLIENT_CMD) != 0) &&
             (status != NU_PPP_ATTEMPT_ABORTED) );

    if (status == NU_SUCCESS)
    {
       /* Send the response. */
        status = MDM_Control_String(PPP_DC_SERVER_COMMAND, dev_ptr);
    }

    if (status == NU_SUCCESS)
    {
        /* Change the link state to let PPP know the link going up */
        link_layer->hwi.state = OPENED;
    }

    /* Switch to PPP mode*/
    MDM_Change_Communication_Mode(MDM_NETWORK_COMMUNICATION, dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }
    /* Return status. */
    return (status);

} /* PPP_DC_Wait */

/*************************************************************************
* FUNCTION
*
*     PPP_DC_Connect
*
* DESCRIPTION
*
*     Connects over Direct Cable to the server.
*
* INPUTS
*
*    *unused                    Unused variable.
*    *dev_ptr                   Device attached to the Direct Cable.
*
* OUTPUTS
*
*     NU_SUCCESS                Successfully connected to the server.
*     NU_INVALID_LINK           Device passed in was invalid.
*
*************************************************************************/
STATUS PPP_DC_Connect(CHAR *unused, DV_DEVICE_ENTRY *dev_ptr)
{
    /* Declaring Variables. */
    LINK_LAYER      *link_layer = (LINK_LAYER *)dev_ptr->dev_link_layer;
    STATUS           status;
    CHAR             mstring[7];
    UINT8            num_try = PPP_DC_NUM_ATTEMPTS;

    /* Remove warnings. */
    UNUSED_PARAMETER(unused);

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Reset the UART. */
    SDC_Reset(link_layer->uart);

    /* Set the state of the device. */
    link_layer->hwi.state = STARTING;

    /* Set the state of the connection. */
    link_layer->connection_status = NU_PPP_DIALING;

    /* Change to terminal mode. */
    status =
        MDM_Change_Communication_Mode(MDM_TERMINAL_COMMUNICATION,
                                      dev_ptr);

    if (status == NU_SUCCESS)
    {
        /* Initialize status to indicate timeout. */
        status = NU_TIMEOUT;

        while ((num_try-- > 0) && (status == NU_TIMEOUT))
        {
            /* Send a client request. */
            status = MDM_Control_String(PPP_DC_CLIENT_COMMAND, dev_ptr);

            /* If a request has been sent. */
            if (status == NU_SUCCESS)
            {
                do
                {
                    /* Check for response. */
                    status = PPP_DC_Get_String(mstring, dev_ptr,
                                               PPP_DC_CLIENT);

                /* Loop until we don't receive a SERVER response or the
                 * attempt has not failed.
                 */
                } while ( (strcmp(mstring, PPP_DC_SERVER_CMD) != 0) &&
                          (status == NU_SUCCESS) );
            }
        }

        /* If connect was successful. */
        if (status == NU_SUCCESS)
        {
            /* Switch to PPP mode. */
            MDM_Change_Communication_Mode(MDM_NETWORK_COMMUNICATION,
                                          dev_ptr);

            /* Indicate that the connection has opened. */
            link_layer->hwi.state = OPENED;
        }
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return status. */
    return (status);

} /* PPP_DC_Connect */

/*************************************************************************
* FUNCTION
*
*     PPP_DC_Get_String
*
* DESCRIPTION
*
*     Gets terminal data from the port
*
* INPUTS
*
*     *response                 String (command) received.
*     *dev_ptr                  Pointer to the device from
*                               which to get data
*     mode                      Mode in which to operate.
*
* OUTPUTS
*
*     NU_SUCCESS                If a string has arrived
*     NU_PPP_ATTEMPT_ABORTED    If an attempt is aborted in
*                               the server case.
*
*************************************************************************/
STATUS PPP_DC_Get_String(CHAR *response, DV_DEVICE_ENTRY *dev_ptr,
                         UINT8 mode)
{
    /* Declaring Variables. */
    UINT8      connected = NU_FALSE, counter = 0;
    CHAR       chr;
    STATUS     status = NU_SUCCESS;
    UINT8      num_try  = 0;
    UINT32     index_save;

    /* Loop until we get the required data or the attempt is aborted in
    in the server case. */
    while ( (connected == NU_FALSE) &&
            (((LINK_LAYER *)dev_ptr->dev_link_layer)->connection_status !=
            NU_PPP_WAITING_ABORTED) )
    {
        /* Check if the port is ready with a character. */
        if (MDM_Data_Ready(dev_ptr))
        {
            /* Get the character. */
            MDM_Get_Char(&chr, dev_ptr);

            /* Check if C(lient) or S(erver) has arrived. */
            if ( (chr == 'C') || (chr == 'S') ||
                 (counter >= (PPP_DC_CMD_MAX_SIZE)-1) )
            {
                /* Start counting again. */
                counter = 0;
            }

            /* Store the character. */
            response[counter] = (CHAR)chr;
            response[++counter] = NU_NULL;

            /* Return if CLIENT or SERVER commands received. */
            if ( (strcmp(response, PPP_DC_CLIENT_CMD) == 0) ||
                 (strcmp(response, PPP_DC_SERVER_CMD) == 0) )
            {
                /* Connection  has established. */
                connected = NU_TRUE;
            }
        }
        else
        {
            /* Save the index before releasing the semaphore. */
            index_save = dev_ptr->dev_index;

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Check if it is PPP server case. */
            if (mode == PPP_DC_SERVER)
            {
                /* Wait for a short time before retrying. */
                NU_Sleep(5);
            }

            /* This is a client case */
            else
            {
                /* Retry until we have checked thrice for data. */
                if (num_try++ >= 2)
                {
                    /* Request has timed out. Set the status and return. */
                    status = NU_TIMEOUT;
                }

                /* Wait before retrying. */
                NU_Sleep(2 * TICKS_PER_SECOND);
            }

            /* Grab the semaphore again. */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Check if the device has not been removed. */
            if (DEV_Get_Dev_By_Index(index_save) == NU_NULL)
            {
                /* Device has been removed, so return from this
                function. */
                return (NU_INVALID_LINK);
            }

            /* Check if we have timed out. */
            if (status == NU_TIMEOUT)
                break;
        }
    }

    /* If the attempt is aborted in the server case then set the status.*/
    if (((LINK_LAYER *)dev_ptr->dev_link_layer)->connection_status
        == NU_PPP_WAITING_ABORTED)
    {
        status = NU_PPP_ATTEMPT_ABORTED;
    }

    /* Return status. */
    return (status);

} /* PPP_DC_Get_String */

#endif

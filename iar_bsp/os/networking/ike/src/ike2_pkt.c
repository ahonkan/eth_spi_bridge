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
*       ike2_pkt.c
*
* COMPONENT
*
*       IKEv2 - Packet Processing
*
* DESCRIPTION
*
*       This file contains functions used to transmit and retransmit
*       IKEv2 messages.
*
* FUNCTIONS
*
*       IKE2_Send_Packet
*       IKE2_Resend_Packet
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike_buf.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_buf.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Send_Packet
*
* DESCRIPTION
*
*       Sends the IKEv2 packet to the peer.
*
* INPUTS
*
*       *handle                 Exchange information for the message
*                               being sent.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Send_Packet(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS          status = NU_SUCCESS;
    UINT8           *buffer;
    UINT16          pload_len;
    INT32           bytes_sent;
    IKE2_MESSAGE    *enc_msg;
    IKE2_NOTIFY_PAYLOAD   *notify_pload;
    IKE2_NOTIFY_PAYLOAD   *next_notify_pload;
    IKE2_DELETE_PAYLOAD   *del_pload;
    IKE2_DELETE_PAYLOAD   *next_del_pload;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Sending out packet...");

    enc_msg = &handle->ike2_params->ike2_out;

    /* Since we don't need last saved message anymore, we can use the
     * same buffer. If we don't have a previous buffer, allocate a new one
     */
    if(handle->ike2_last_message != NU_NULL)
    {
        buffer = handle->ike2_last_message;
    }
    else
    {
        /* If we did not have a buffer previously, allocate a new one.
         * If allocated successfully, set the ike2_last_message pointer
         * to point to it too so that we can reuse this buffer for next
         * packet.
         */
        status = IKE_Allocate_Buffer(&buffer);

        if(status == NU_SUCCESS)
        {
            handle->ike2_last_message = buffer;
        }
    }

    if(status == NU_SUCCESS)
    {
        status = IKE2_Encode_Message(buffer, enc_msg);

        if(status == NU_SUCCESS)
        {
            /* Message has been encoded. Except for IKE_SA_INIT, all other
             * IKEv2 messages need to be encrypted beyond the IKE header.
             * Encrypt message if it needs to be encrypted.
             */
            if((enc_msg->ike2_hdr->ike2_exchange_type != IKE2_SA_INIT) &&
               (handle->ike2_sa->ike_encryption_key != NU_NULL))
            {
                /* We need to tell the encryption function the total length
                 * of data to be encrypted. IKE header is not encrypted,
                 * so subtract its length from total message length.
                 */
                pload_len = (UINT16)(enc_msg->ike2_hdr->ike2_length -
                                IKE2_HDR_TOTAL_LEN);

                status = IKE2_Encode_IKE_Encrypted_Payload(buffer,
                            IKE_MAX_BUFFER_LEN, pload_len, handle->ike2_sa,
                            enc_msg->ike2_hdr->ike2_next_payload);

                if(status == NU_SUCCESS)
                {
                    /* Update the length in IKEv2 header structure to be
                     * used by the sending call below.
                     */
                    enc_msg->ike2_hdr->ike2_length =
                                    GET32(buffer, IKE2_HDR_LEN_OFST);
                }

                else
                {
                    NLOG_Error_Log("Failed to encrypt the message",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            if(status == NU_SUCCESS)
            {
                /* Update handle socket as the IKE socket may have restarted*/
                handle->ike2_socket = IKE_Data.ike_socket;

                /* Now send the packet to the peer. We need to send the
                 * packet to the port from which we received the packet.
                 */
                bytes_sent = NU_Send_To(handle->ike2_socket, (CHAR*)buffer,
                                        (UINT16)(enc_msg->ike2_hdr->
                                        ike2_length), 0, &handle->ike2_sa->
                                        ike_node_addr, 0);

                /* Number of bytes sent should be returned. If some error
                 * occurred, this valued will be less than zero indicating
                 * error.
                 */
                if(bytes_sent < 0)
                {
                    /* If error occurred, set the error code returned. */
                    status = bytes_sent;

                    NLOG_Error_Log("Failed to send the packet",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
        else
        {
            NLOG_Error_Log("Failed to encode the message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate buffer", NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    }

    if(status != NU_SUCCESS)
    {
        /* If we had allocated a buffer but could not successfully process
         * and send the packet, we need to free the buffer allocated.
         */
        if(handle->ike2_last_message != NU_NULL)
        {
            if(IKE_Deallocate_Buffer(handle->ike2_last_message)
                == NU_SUCCESS)
            {
                handle->ike2_last_message = NU_NULL;
            }

            else
            {
                NLOG_Error_Log("Failed to deallocate buffer",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Some payloads in the outgoing packet are dynamically allocated
     * so we deallocate them here, after the packet has been sent.
     */
    if(enc_msg->ike2_notify != NU_NULL)
    {
        notify_pload = enc_msg->ike2_notify;

        while(notify_pload != NU_NULL)
        {
            next_notify_pload = notify_pload->ike2_next;
            if(NU_Deallocate_Memory(notify_pload) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate N payload memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            notify_pload = next_notify_pload;
        }

        enc_msg->ike2_notify = NU_NULL;
    }

    if(enc_msg->ike2_del != NU_NULL)
    {
        del_pload = enc_msg->ike2_del;

        while(del_pload != NU_NULL)
        {
            next_del_pload = del_pload->ike2_next;
            if(NU_Deallocate_Memory(del_pload) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate D payload memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            del_pload = next_del_pload;
        }

        enc_msg->ike2_del = NU_NULL;
    }

    if(enc_msg->ike2_auth != NU_NULL)
    {
        if(enc_msg->ike2_auth->ike2_auth_data != NU_NULL)
        {
            if(NU_Deallocate_Memory(enc_msg->ike2_auth->ike2_auth_data)
                != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate AUTH data",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            enc_msg->ike2_auth->ike2_auth_data = NU_NULL;
        }
    }

    return (status);

} /* IKE2_Send_Packet */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Resend_Packet
*
* DESCRIPTION
*
*       This function retransmits a previously buffered
*       packet to the destination. The parameter to this
*       function is an exchange handle.
*
* INPUTS
*
*       *handle                 Pointer to the Exchange Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       NU_INVALID_SOCKET       IKEv2 socket is invalid.
*       NU_NOT_CONNECTED        IKEv2 socket is not connected.
*       NU_SOCKET_CLOSED        IKEv2 socket has been closed.
*       NU_DEVICE_DOWN          Network device is down.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*       IKE_NOT_BUFFERED        Message is not buffered.
*
*************************************************************************/
STATUS IKE2_Resend_Packet(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    UINT32              msg_len;
    INT32               bytes_sent;
    IKE2_SA             *sa;

    /* Make sure all pointers are valid. */
    if(handle == NU_NULL)
    {
        status = IKE2_INVALID_PARAMS;
    }

    /* Make sure SA pointer in the Handle is valid. */
    else if(handle->ike2_sa == NU_NULL)
    {
        status = IKE2_INVALID_PARAMS;
    }

    /* Check if the previous message's buffer is present. */
    else if(handle->ike2_last_message == NU_NULL)
    {
        /* Message not buffered. */
        status = IKE_NOT_BUFFERED;
    }

    /* Make sure the resend count limit has not been exceeded. */
    else if(handle->ike2_resend_count == 0)
    {
        status = IKE2_INVALID_PARAMS;
    }

    else
    {
        /* Log debug message. */
        IKE2_DEBUG_LOG("<-- Retransmitting IKEv2 message");

        /* Set the SA pointer. */
        sa = handle->ike2_sa;

        /* Get the message length from the raw message. */
        msg_len = GET32(handle->ike2_last_message, IKE_HDR_LENGTH_OFFSET);

        /* Update handle socket as the IKE socket may have restarted. */
        handle->ike2_socket = IKE_Data.ike_socket;

        /* Send the message. */
        bytes_sent = NU_Send_To(handle->ike2_socket,
                        (CHAR*)handle->ike2_last_message,
                        (UINT16)msg_len, 0, &sa->ike_node_addr, 0);

        if(bytes_sent < 0)
        {
            /* Set status to the error code. */
            status = (STATUS)bytes_sent;

            NLOG_Error_Log("Failed to send UDP packet", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
        }

        /* Decrement the message re-send counter regardless of whether
         * the packet was successfully transmitted or not, because we
         * do not handle the retransmission of retransmissions.
         */
        handle->ike2_resend_count = handle->ike2_resend_count - 1;
    }

    /* Return the status. */
    return (status);

} /* IKE2_Resend_Packet */

#endif

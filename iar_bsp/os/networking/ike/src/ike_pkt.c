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
*       ike_pkt.c
*
* COMPONENT
*
*       IKE - Packet
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE Packet
*       component. It is responsible for encoding and sending
*       IKE packets to the destination. The packet receive and
*       decode logic is spread across several components, so
*       this component does not handle receives.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Send_Packet
*       IKE_Send_Phase2_Packet
*       IKE_Resend_Packet
*
* DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*       ips_api.h
*       ike_api.h
*       ike_enc.h
*       ike_buf.h
*       ike_oak.h
*       ike_evt.h
*       ike_crypto_wrappers.h
*
*************************************************************************/
#include "networking/nu_net.h"
#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"
#endif
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_enc.h"
#include "networking/ike_buf.h"
#include "networking/ike_oak.h"
#include "networking/ike_evt.h"
#include "networking/ike_crypto_wrappers.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_Send_Packet
*
* DESCRIPTION
*
*       This function encodes the chain of IKE Payloads into
*       a single message, encrypts the message (if required)
*       and sends it to the destination node. A copy of the
*       outgoing packet is saved in the Phase 1 Handle. If
*       the previous copy of the last message is not NULL,
*       the same buffer is used for the new message. Otherwise
*       a new buffer is allocated. The caller is responsible
*       for freeing this buffer, when it is no longer needed
*       (using IKE_Deallocate_Buffer).
*
*       The caller is responsible for obtaining the IKE
*       semaphore before calling this function.
*
* INPUTS
*
*       *phase1                 Pointer to Phase 1 Handle.
*                               All required parameters must be
*                               set in this Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       NU_INVALID_SOCKET       IKE socket is invalid.
*       NU_NOT_CONNECTED        IKE socket is not connected.
*       NU_SOCKET_CLOSED        IKE socket has been closed.
*       NU_DEVICE_DOWN          Network device is down.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*
*************************************************************************/
STATUS IKE_Send_Packet(IKE_PHASE1_HANDLE *phase1)
{
    STATUS              status = NU_SUCCESS;
    INT32               bytes_sent;
    UINT8               *buffer;
    UINT16              ploads_len;
    IKE_SA              *sa;
    IKE_ENC_HDR         *hdr;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the Handle pointer is valid. */
    if(phase1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("<-- Sending phase 1 IKE message");

    /* Set local pointers to commonly used data in the Handle. */
    sa  = phase1->ike_sa;
    hdr = phase1->ike_params->ike_out.ike_hdr;

    /* Check if the previous message's buffer is present. */
    if(phase1->ike_last_message != NU_NULL)
    {
        /* Use the previously allocated buffer. */
        buffer = phase1->ike_last_message;
    }

    else
    {
        /* Allocate a buffer for storing the encoded message. */
        status = IKE_Allocate_Buffer(&buffer);

        if(status == NU_SUCCESS)
        {
            /* Store pointer to message buffer in the Handle. */
            phase1->ike_last_message = buffer;
        }
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Encode the message. */
        status = IKE_Encode_Message(buffer, hdr);

        if(status == NU_SUCCESS)
        {
            /* Check if the message is to be encrypted. */
            if((hdr->ike_flags & IKE_HDR_ENC_MASK) != 0)
            {
                /* Set the payloads data length. */
                ploads_len = (UINT16)(hdr->ike_length - IKE_HDR_LEN);

                /* Encrypt the message. */
                status = IKE_Encrypt(sa, buffer + IKE_HDR_LEN,
                                     IKE_MAX_BUFFER_LEN - IKE_HDR_LEN,
                                     &ploads_len,
                                     sa->ike_encryption_iv,
                                     sa->ike_decryption_iv,
                                     IKE_ENCRYPT);

                if(status == NU_SUCCESS)
                {
                    /* Update length of encrypted message in header. */
                    hdr->ike_length = ploads_len + IKE_HDR_LEN;

                    /* Update length of encrypted message in buffer. */
                    PUT32(buffer, IKE_HDR_LENGTH_OFFSET, hdr->ike_length);
                }

                else
                {
                    NLOG_Error_Log("Failed to encrypt IKE message",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Send the message. */
                bytes_sent = NU_Send_To(IKE_Data.ike_socket, (CHAR*)buffer,
                                        (UINT16)hdr->ike_length, 0,
                                        &sa->ike_node_addr, 0);

                if(bytes_sent < 0)
                {
                    /* Set status to the error code. */
                    status = (STATUS)bytes_sent;

                    NLOG_Error_Log("Failed to send UDP packet",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Make sure this is not an informational exchange
                 * because there are no message re-sends in
                 * informational mode.
                 */
                else if(hdr->ike_exchange_type != IKE_XCHG_INFO)
                {
                    /* Reset the message re-send counter. */
                    phase1->ike_resend_count = IKE_RESEND_COUNT;

                    /* Remove any pending message re-send timers
                     * for this exchange.
                     */
                    status = IKE_Unset_Timer(IKE_Message_Reply_Event,
                                             (UNSIGNED)sa, 0);

                    if(status == NU_SUCCESS)
                    {
                        /* Register a new message re-send timer. */
                        status = IKE_Set_Timer(IKE_Message_Reply_Event,
                                               (UNSIGNED)sa, 0,
                                               IKE_RESEND_INTERVAL);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log(
                                "Failed to set message resend timer",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log(
                            "Failed to unset message resend timers",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to encode IKE message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* If an error occurred or if this is an informational exchange. */
    if((status != NU_SUCCESS) || (hdr->ike_exchange_type == IKE_XCHG_INFO))
    {
        /* If the last message buffer is allocated. */
        if(phase1->ike_last_message != NU_NULL)
        {
            /* Deallocate the buffer. */
            if(IKE_Deallocate_Buffer(phase1->ike_last_message) !=
               NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to free IKE buffer",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Set pointer to last message to NULL. */
            phase1->ike_last_message = NU_NULL;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Send_Packet */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Send_Phase2_Packet
*
* DESCRIPTION
*
*       This function encodes the chain of IKE Payloads into
*       a single message and if the first payload is Hash,
*       then it calculates the hash of the message and stores
*       it in the first payload. The message is then encrypted
*       (if required) and sent to the destination node. A copy
*       of the outgoing packet is saved in the IKE SA2. If the
*       previous copy of the last message is not NULL, the
*       same buffer is used for the new message. Otherwise a
*       new buffer is allocated. The caller is responsible for
*       freeing this buffer, when it is no longer needed
*       (using IKE_Deallocate_Buffer).
*
*       Note that if the first payload after the ISAKMP header
*       is a Hash payload then its Hash data size MUST be
*       equal to the digest size of the Hash function
*       negotiated in Phase 1.
*
*       The caller is responsible for obtaining the IKE
*       semaphore before calling this function.
*
* INPUTS
*
*       *phase2                 Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       NU_INVALID_SOCKET       IKE socket is invalid.
*       NU_NOT_CONNECTED        IKE socket is not connected.
*       NU_SOCKET_CLOSED        IKE socket has been closed.
*       NU_DEVICE_DOWN          Network device is down.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*
*************************************************************************/
STATUS IKE_Send_Phase2_Packet(IKE_PHASE2_HANDLE *phase2)
{
    STATUS          status = NU_SUCCESS;
    INT32           bytes_sent;
    UINT8           *buffer;
    UINT16          ploads_len;
    UINT8           *msg_dgst;
    UINT8           msg_dgst_len = 0;
    IKE_SA          *sa;
    IKE_ENC_HDR     *hdr;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the Handle pointer is valid. */
    if(phase2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("<-- Sending phase 2 IKE message");

    /* Set local pointers to commonly used data in the Handle. */
    sa  = phase2->ike_sa;
    hdr = phase2->ike_params->ike_out.ike_hdr;

    /* Check if the previous message's buffer is present. */
    if(phase2->ike_last_message != NU_NULL)
    {
        /* Use the previously allocated buffer. */
        buffer = phase2->ike_last_message;
    }

    else
    {
        /* Allocate a buffer for storing the encoded message. */
        status = IKE_Allocate_Buffer(&buffer);

        if(status == NU_SUCCESS)
        {
            /* Store pointer to message buffer in the Handle. */
            phase2->ike_last_message = buffer;
        }
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Encode the message. */
        status = IKE_Encode_Message(buffer, hdr);

        if(status == NU_SUCCESS)
        {
            /* If the first payload in the message is a Hash payload. */
            if(hdr->ike_hdr.ike_next_payload->ike_type ==
               IKE_HASH_PAYLOAD_ID)
            {
                /* Set pointer to the location where the HASH(x)
                 * digest would be stored.
                 */
                msg_dgst = buffer + IKE_HDR_LEN + IKE_MIN_HASH_PAYLOAD_LEN;

                /* Determine the current state of IKE State Machine.                     * The return value of the HASH(x) calculation
                * functions below can safely be ignored.
                */
                switch(phase2->ike_xchg_state)
                {
                    case IKE_SEND_HASH1_STATE:
                    case IKE_SEND_HASH4_STATE:
                        /* Calculate HASH(1) (same as HASH(4)). */
                        status = IKE_Hash_x(phase2, buffer,
                                     (UINT16)hdr->ike_length, msg_dgst,
                                     &msg_dgst_len, IKE_HASH_1);
                        break;

                    case IKE_SEND_HASH2_STATE:
                        /* Calculate HASH(2). */
                        status = IKE_Hash_x(phase2, buffer,
                                     (UINT16)hdr->ike_length, msg_dgst,
                                     &msg_dgst_len, IKE_HASH_2);
                        break;

                    case IKE_SEND_HASH3_STATE:
                        /* Calculate HASH(3). */
                        status = IKE_Hash_x(phase2, buffer,
                                     (UINT16)hdr->ike_length, msg_dgst,
                                     &msg_dgst_len, IKE_HASH_3);
                        break;
                }

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to calculate HASH(x)",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* If no error occurred and the message is to be encrypted. */
            if((status == NU_SUCCESS) &&
               ((hdr->ike_flags & IKE_HDR_ENC_MASK) != 0))
            {
                /* Set the payloads data length. */
                ploads_len = (UINT16)(hdr->ike_length - IKE_HDR_LEN);

                /* Encrypt the message. */
                status = IKE_Encrypt(sa, buffer + IKE_HDR_LEN,
                                     IKE_MAX_BUFFER_LEN - IKE_HDR_LEN,
                                     &ploads_len,
                                     phase2->ike_encryption_iv,
                                     phase2->ike_decryption_iv,
                                     IKE_ENCRYPT);

                if(status == NU_SUCCESS)
                {
                    /* Update length of encrypted message in header. */
                    hdr->ike_length = ploads_len + IKE_HDR_LEN;

                    /* Update length of encrypted message in buffer. */
                    PUT32(buffer, IKE_HDR_LENGTH_OFFSET, hdr->ike_length);
                }

                else
                {
                    NLOG_Error_Log("Failed to encrypt IKE message",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* Make sure no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Send the message. */
                bytes_sent = NU_Send_To(IKE_Data.ike_socket, (CHAR*)buffer,
                                        (UINT16)hdr->ike_length, 0,
                                        &phase2->ike_node_addr, 0);

                if(bytes_sent < 0)
                {
                    /* Set status to the error code. */
                    status = (STATUS)bytes_sent;

                    NLOG_Error_Log("Failed to send UDP packet",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Make sure this is not an informational exchange
                 * because there are no message re-sends in
                 * informational mode.
                 */
                else if(hdr->ike_exchange_type != IKE_XCHG_INFO)
                {
                    /* Reset the message re-send counter. */
                    phase2->ike_resend_count = IKE_RESEND_COUNT;

                    /* Remove any pending message re-send timers
                     * for this exchange.
                     */
                    status = IKE_Unset_Timer(IKE_Message_Reply_Event,
                                             (UNSIGNED)sa,
                                             (UNSIGNED)phase2);

                    if(status == NU_SUCCESS)
                    {
                        /* Register a new message re-send timer. */
                        status = IKE_Set_Timer(IKE_Message_Reply_Event,
                                               (UNSIGNED)sa,
                                               (UNSIGNED)phase2,
                                               IKE_RESEND_INTERVAL);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log(
                                "Failed to set message resend timer",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log(
                            "Failed to unset message resend timers",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to encode Phase 2 IKE message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* If an error occurred or if this is an informational exchange. */
    if((status != NU_SUCCESS) || (hdr->ike_exchange_type == IKE_XCHG_INFO))
    {
        /* If the last message buffer is allocated. */
        if(phase2->ike_last_message != NU_NULL)
        {
            /* Deallocate the buffer. */
            if(IKE_Deallocate_Buffer(phase2->ike_last_message) !=
               NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to free IKE buffer",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Set pointer to last message to NULL. */
            phase2->ike_last_message = NU_NULL;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Send_Phase2_Packet */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Resend_Packet
*
* DESCRIPTION
*
*       This function retransmits a previously buffered
*       packet to the destination. The parameter to this
*       function could be either a Phase 1 Handle or a
*       Phase 2 Handle. Both are supported due to the
*       similar retransmission interface.
*
* INPUTS
*
*       *phase1                 Pointer to the Phase 1 or
*                               Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       NU_INVALID_SOCKET       IKE socket is invalid.
*       NU_NOT_CONNECTED        IKE socket is not connected.
*       NU_SOCKET_CLOSED        IKE socket has been closed.
*       NU_DEVICE_DOWN          Network device is down.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_BUFFERED        Message is not buffered.
*
*************************************************************************/
STATUS IKE_Resend_Packet(IKE_PHASE1_HANDLE *phase1)
{
    STATUS              status = NU_SUCCESS;
    UINT32              msg_len;
    INT32               bytes_sent;
    IKE_SA              *sa;

    /* Make sure all pointers are valid. */
    if(phase1 == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Make sure SA pointer in the Handle is valid. */
    else if(phase1->ike_sa == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Check if the previous message's buffer is present. */
    else if(phase1->ike_last_message == NU_NULL)
    {
        /* Message not buffered. */
        status = IKE_NOT_BUFFERED;
    }

    /* Make sure the resend count limit has not been exceeded. */
    else if(phase1->ike_resend_count == 0)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("<-- Retransmitting IKE message");

        /* Set the SA pointer. */
        sa = phase1->ike_sa;

        /* Get the message length from the raw message. */
        msg_len = GET32(phase1->ike_last_message, IKE_HDR_LENGTH_OFFSET);

        /* Send the message. */
        bytes_sent = NU_Send_To(IKE_Data.ike_socket,
                                (CHAR*)phase1->ike_last_message,
                                (UINT16)msg_len, 0, &sa->ike_node_addr, 0);

        if(bytes_sent < 0)
        {
            /* Set status to the error code. */
            status = (STATUS)bytes_sent;

            NLOG_Error_Log("Failed to send UDP packet",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Decrement the message re-send counter regardless of whether
         * the packet was successfully transmitted or not, because we
         * do not handle the retransmission of retransmissions.
         */
        phase1->ike_resend_count = phase1->ike_resend_count - 1;
    }

    /* Return the status. */
    return (status);

} /* IKE_Resend_Packet */

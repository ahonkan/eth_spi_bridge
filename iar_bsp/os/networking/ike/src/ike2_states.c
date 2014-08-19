
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
*       ike2_states.c
*
* COMPONENT
*
*       IKEv2 - Exchange State Machine
*
* DESCRIPTION
*
*       State machine handler functions for IKEv2 state machine
*
* FUNCTIONS
*
*       IKE2_SA_INIT_I
*       IKE2_SA_INIT_R
*       IKE2_AUTH_I
*       IKE2_AUTH_R
*       IKE2_AUTH_I_Response
*       IKE2_AUTH_Receive
*       IKE2_CREATE_CHILD_SA_I
*       IKE2_CREATE_CHILD_SA_I_Response
*       IKE2_CREATE_CHILD_SA_R
*       IKE2_CREATE_CHILD_SA_Receive
*       IKE2_Construct_Header
*       IKE2_Process_IPsec_SA_Payloads
*       IKE2_Create_IPsec_SA_Payloads
*       IKE2_Save_Auth_Data
*       IKE2_Process_SA_INIT
*       IKE2_Replace_SA
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike_evt.h
*       ike2_evt.h
*       ike_ips.h
*       rand.h
*       hmac.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_evt.h"
#include "networking/ike_ips.h"
#include "openssl/rand.h"
#include "openssl/hmac.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#include "networking/ike2_evt.h"

/* State machine handler functions. */
STATIC STATUS IKE2_SA_INIT_I(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_SA_INIT_R(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_AUTH_I(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_AUTH_I_Response(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_AUTH_R(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_AUTH_Receive(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_CREATE_CHILD_SA_I(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_CREATE_CHILD_SA_I_Response(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_CREATE_CHILD_SA_R(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_CREATE_CHILD_SA_Receive(IKE2_EXCHANGE_HANDLE *handle);

/* State machine helper functions. */
STATIC STATUS IKE2_Process_IPsec_SA_Payloads(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_Create_IPsec_SA_Payloads(IKE2_EXCHANGE_HANDLE *handle);
STATIC STATUS IKE2_Save_Auth_Data(UINT8 **first_msg, UINT8 *message,
                                  UINT32 message_len);

STATIC
IKE2_STATE_MACHINE_FUNCS IKE2_State_Handler[IKE2_TOTAL_EXCHANGE_STATES] =
{
    IKE2_SA_INIT_I,
    IKE2_SA_INIT_R,
    IKE2_AUTH_I,
    IKE2_AUTH_R,
    IKE2_AUTH_I_Response,
    IKE2_CREATE_CHILD_SA_I,
    IKE2_CREATE_CHILD_SA_R,
    IKE2_CREATE_CHILD_SA_I_Response
};

/*************************************************************************
*
* FUNCTION
*
*       IKE2_SA_INIT_I
*
* DESCRIPTION
*
*       This function is the first in the state machine when we are the
*       initiator in the IKEv2 exchange. This function constructs and
*       sends the first IKE_SA_INIT message and then returns. The payloads
*       send by this function can be represented as:
*
*       *Initiator                          Responder
*       -----------                        -----------
*       HDR, SAi1, KEi, Ni   -->
*
* INPUTS
*
*       *handle                 Exchange handle representing the specific
*                               exchange under way.
*
* OUTPUTS
*
*       NU_SUCCESS              Message sent successfully.
*       IKE2_INVALID_PARAMS     Parameters to this function are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_SA_INIT_I(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status;
    IKE2_MESSAGE        *out_msg;

    /* Payload to be sent. */
    IKE2_HDR            hdr;
    IKE2_SA_PAYLOAD     sa;
    IKE2_KE_PAYLOAD     ke;
    IKE2_NONCE_PAYLOAD  nonce;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Set local pointers. */
    out_msg = &handle->ike2_params->ike2_out;

    /* Zero out the structure memory. */
    UTL_Zero(&sa, sizeof(IKE2_SA_PAYLOAD));
    UTL_Zero(&ke, sizeof(IKE2_KE_PAYLOAD));
    UTL_Zero(&nonce, sizeof(IKE2_NONCE_PAYLOAD));

    /* Set the payloads that need to be sent in this message. */
    IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr);
    IKE2_SET_OUTBOUND(out_msg->ike2_sa, &sa);
    IKE2_SET_OUTBOUND(out_msg->ike2_ke, &ke);

    IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr,
                    IKE2_NONE_PAYLOAD_ID);

    if(handle->ike2_flags & IKE2_USE_INIT_COOKIE)
    {
        /* Add the N(COOKIE) payload to the chain. */
        status = IKE2_Generate_Notify(handle, 0, IKE2_NOTIFY_COOKIE, handle->ike2_cookie,
                                      handle->ike2_cookie_len, 0, NU_NULL);
    }

    /* Extract the attributes for the set of parameters used with this
     * policy. This information is needed when computing the keying
     * material for this exchange.
     */
    status = IKE2_Extract_IKE_SA_Attribs(handle->ike2_params->
                                         ike2_policy->ike_xchg1_attribs,
                                         handle->ike2_sa);
    if(status == NU_SUCCESS)
    {
        /* Construct the proposal to be sent. */
        status = IKE2_Construct_Proposal(handle);

        if(status == NU_SUCCESS)
        {
            /* Generate they keying material for this exchange. */
            status = IKE2_Generate_KE_Data(handle, &ke);

            if(status == NU_SUCCESS)
            {
                /* Generate nonce data. */
                status = IKE2_Generate_Nonce_Data(handle, &nonce);

                if(status == NU_SUCCESS)
                {
                    /* Terminate the chain of outgoing payloads for this
                     * packet.
                     */
                    IKE2_END_CHAIN(out_msg->ike2_last);

                    /* Generate SPI for this exchange. */
                    if (NU_TRUE != RAND_bytes(handle->ike2_sa->ike2_local_spi, IKE2_SPI_LENGTH))
                    {
                        NLOG_Error_Log("Error generating random data.",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }

                    /* The payloads are ready. Now construct the header
                     * before sending the actual packet.
                     */
                    status = IKE2_Construct_Header(handle, IKE2_SA_INIT,
                                                   IKE2_HDR_INITIATOR_FLAG);

                    if(status == NU_SUCCESS)
                    {
                        /* Packet ready to be sent. Encode the payloads
                         * and send the packets.
                         */
                        status = IKE2_Send_Packet(handle);

                        if(status == NU_SUCCESS)
                        {
                            handle->ike2_state += IKE2_NEXT_STATE_INC;

                            /* Packet was sent successfully!
                             * Set the message reply timeout timer.
                             */
                            status = IKE_Set_Timer(IKE2_Message_Reply_Event,
                                                   (UNSIGNED)handle->ike2_sa,
                                                   (UNSIGNED)handle,
                                                   IKE2_MESSAGE_REPLY_TIMEOUT);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Message retransmission timer could not be added",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }

                        }
                        else
                        {
                            NLOG_Error_Log("Failed to send the packet!",
                                           NERR_RECOVERABLE, __FILE__,
                                           __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Unable to encode IKEv2 header",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to generate nonce",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to generate Key exchange data",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to generate SA payload",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to extract SA parameters",NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    }
    /* Save the message for use in authentication of IKE SA. */
    if(status == NU_SUCCESS)
    {
        handle->ike2_sa->ike2_local_auth_len = (UINT16)out_msg->ike2_hdr->
                                                ike2_length;
        status = IKE2_Save_Auth_Data(&handle->ike2_sa->ike2_local_auth_data,
                                     handle->ike2_last_message,
                                     out_msg->ike2_hdr->ike2_length);
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Could not save Authentication data.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_SA_INIT_R
*
* DESCRIPTION
*
*       First function in the state machine when we are the responder in
*       the exchange. This function receives the IKE_SA_INIT message and
*       sends the reply to the first message. The payloads exchanged can
*       be represented as:
*
*       Initiator                  *Responder
*       -----------                -----------
*       HDR, SAi1, KEi, Ni  -->
*                           <--    HDR, SAr1, KEr, Nr, [CERTREQ]
*
*
* INPUTS
*
*       handle                  Exchange handle representing the specific
*                               exchange under way.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_SA_INIT_R(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status;
    IKE2_MESSAGE        *in_msg;
    IKE2_MESSAGE        *out_msg;
    IKE2_STATE_PARAMS   *params;

    /* Payload structures for decoding the incoming message. */
    IKE2_SA_PAYLOAD         sa_i;
    IKE2_KE_PAYLOAD         ke_i;
    IKE2_NONCE_PAYLOAD      nonce_i;
    IKE2_ATTRIB             *attribs = NU_NULL;
    IKE2_VENDOR_ID_PAYLOAD  vid_i;

    /* Payload structures for encoding outgoing message. */
    IKE2_HDR            hdr_r;
    IKE2_SA_PAYLOAD     sa_r;
    IKE2_KE_PAYLOAD     ke_r;
    IKE2_NONCE_PAYLOAD  nonce_r;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE2_CERT_REQ_PAYLOAD   cert_req_i;
#endif

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("IKE2_SA_INIT_R: Processing incoming request.");

    in_msg = &handle->ike2_params->ike2_in;
    out_msg = &handle->ike2_params->ike2_out;
    params = handle->ike2_params;

    /* The following members are dynamically allocated so initialize
     * them to a known value here and we will need to deallocate them
     * when we're done processing.
     */
    ke_i.ike2_ke_data = NU_NULL;
    nonce_i.ike2_nonce_data = NU_NULL;
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    cert_req_i.ike2_ca = NU_NULL;
#endif

    /* Do sanity checks here on input. */
    if(in_msg->ike2_hdr->ike2_next_payload != IKE2_SA_PAYLOAD_ID)
    {
        status = IKE2_UNEXPECTED_PAYLOAD;
        NLOG_Error_Log("SA payload expected but not found",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        UTL_Zero(&sa_i, sizeof(IKE2_SA_PAYLOAD));

        IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_sa, &sa_i);
        IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_ke, &ke_i);
        IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_nonce, &nonce_i);
        IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_vid, &vid_i);

        IKE2_INIT_CHAIN(in_msg->ike2_last, in_msg->ike2_hdr, IKE2_NONE_PAYLOAD_ID);

        /* Receive and decode the incoming request. */
        status = IKE2_Decode_Message(params->ike2_packet->ike_data, in_msg);

        if(status == NU_SUCCESS)
        {
            UTL_Zero(&sa_r, sizeof(IKE2_SA_PAYLOAD));

            IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr_r);
            IKE2_SET_OUTBOUND(out_msg->ike2_sa, &sa_r);

            IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr,
                            IKE2_NONE_PAYLOAD_ID);

            /* Check if any information payloads have been received. */
            if(in_msg->ike2_notify != NU_NULL)
            {
                status = IKE2_Process_Notify(handle);

                if(status != NU_NULL)
                {
                    IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                    (UNSIGNED)handle->ike2_sa,
                                    (UNSIGNED)handle);

                    NLOG_Error_Log("Failed to process Notify payload",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            if((status == NU_SUCCESS) && (in_msg->ike2_del != NU_NULL))
            {
                status = IKE2_Process_Delete(handle);

                if(status != NU_NULL)
                {
                    NLOG_Error_Log("Failed to process Delete payload",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            if(status == NU_SUCCESS)
            {
                status = IKE2_Select_Proposal(handle, &attribs);

                if(status == NU_NULL)
                {
                    /* Flush incoming SA payload. */
                    IKE2_FLUSH_SA_PAYLOAD(in_msg);
                }
                else
                {
                    NLOG_Error_Log("No suitable proposal selected",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            if(status == NU_SUCCESS)
            {
                /* Allocate buffer for DH public values. */
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                    (VOID**)&handle->ike2_remote_dh,
                    in_msg->ike2_ke->ike2_ke_data_len, NU_NO_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    handle->ike2_remote_dh = TLS_Normalize_Ptr(
                        handle->ike2_remote_dh);

                    if(in_msg->ike2_ke->ike2_ke_data != NU_NULL)
                    {
                        NU_BLOCK_COPY(handle->ike2_remote_dh,
                            in_msg->ike2_ke->ike2_ke_data,
                            in_msg->ike2_ke->ike2_ke_data_len);
                    }

                    handle->ike2_remote_dh_len =
                        in_msg->ike2_ke->ike2_ke_data_len;

                    /* Allocate buffer for nonce. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                        (VOID**)&handle->ike2_peer_nonce,
                        in_msg->ike2_nonce->ike2_nonce_data_len, NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        handle->ike2_peer_nonce = TLS_Normalize_Ptr(
                            handle->ike2_peer_nonce);

                        if(in_msg->ike2_nonce->ike2_nonce_data != NU_NULL)
                        {
                            NU_BLOCK_COPY(handle->ike2_peer_nonce,
                                in_msg->ike2_nonce->ike2_nonce_data,
                                in_msg->ike2_nonce->ike2_nonce_data_len);
                        }

                        handle->ike2_peer_nonce_len =
                            (UINT8)(in_msg->ike2_nonce->ike2_nonce_data_len);

                        /* Process the received IKE SA payload and
                         * extract the values.
                         */
                        status = IKE2_Add_IKE_SA(&params->ike2_policy->ike_sa_list,
                                                 handle->ike2_sa);

                        if(status == NU_SUCCESS)
                        {
                            /* Copy the initiator's SPI. We need to send
                             * it back to the peer in responses.
                             */
                            NU_BLOCK_COPY(handle->ike2_sa->ike2_remote_spi,
                                in_msg->ike2_hdr->ike2_sa_spi_i, IKE2_SPI_LENGTH);

                            /* Copy the message ID. It should be same in
                             * response as in request.
                             */
                            handle->ike2_next_id_peer = in_msg->ike2_hdr->ike2_msg_id;

                            status = IKE2_Extract_IKE_SA_Attribs(attribs,
                                                        handle->ike2_sa);

                            if(status == NU_SUCCESS)
                            {
                                status = IKE2_Add_Exchange_Handle(
                                    &handle->ike2_sa->xchg_db,
                                    handle);
                                if(status != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Exchange handle could \
                                                   not be added to the list",
                                                   NERR_RECOVERABLE,
                                                   __FILE__, __LINE__);
                                }
                            }
                            else
                            {
                                NLOG_Error_Log("SA attribs could not be copied",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                        else
                        {
                            NLOG_Error_Log("SA could not be added to SADB",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                        /* We do not have ID payload yet (as we had in IKEv1
                         * so, we cannot authenticate the peer yet! We will
                         * authenticate the peer in next pair of messages.
                         */
                    }

                    else
                    {
                        NLOG_Error_Log("Memory allocation failed for peer's nonce",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Memory allocation failed for DH keys",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Suitable proposal NOT selected!",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Message could not be decoded", NERR_RECOVERABLE,
                __FILE__, __LINE__);
        }

    }

    /* We need to save this packet because the initiator's auth data will
     * be calculated using its contents.
     */
    if(status == NU_SUCCESS)
    {
        handle->ike2_sa->ike2_peer_auth_len =
            handle->ike2_params->ike2_packet->ike_data_len;

        status = IKE2_Save_Auth_Data(&handle->ike2_sa->ike2_peer_auth_data,
                        handle->ike2_params->ike2_packet->ike_data,
                        handle->ike2_params->ike2_packet->ike_data_len);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Authentication data could not be saved",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Incoming message has been decoded. Now construct the reply and
     * send it to the peer if all the decoding went fine!
     */

    if(status == NU_SUCCESS)
    {
        /* Set the outbound payloads. */
        /* SA payload has already been set in the out message when
         * selecting proposal.
         */
        IKE2_SET_OUTBOUND(out_msg->ike2_ke, &ke_r);
        IKE2_SET_OUTBOUND(out_msg->ike2_nonce, &nonce_r);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        /* If authentication by signature is enabled in build and chosen
         * in the policy, send a CERT_REQ.
         */
        if((handle->ike2_params->ike2_policy->ike_flags &
            IKE_INBAND_CERT_XCHG) !=0)
        {
            IKE2_SET_OUTBOUND(out_msg->ike2_cert_req, &cert_req_i);
        }
#endif

        /* Generate Key Exchange data and add the payload to the chain
         * of payloads being sent. The following function call performs
         * both these tasks.
         */
        status = IKE2_Generate_KE_Data(handle, out_msg->ike2_ke);

        if(status == NU_SUCCESS)
        {
            /* Generate Nonce and add the payload to the chain of
             * outgoing payloads. Nonce should be at least half the
             * size of key for negotiated PRF, so set the nonce length.
             */
            out_msg->ike2_nonce->ike2_nonce_data_len = HMAC_MAX_MD_CBLOCK / 2;

            status = IKE2_Generate_Nonce_Data(handle, out_msg->ike2_nonce);

            if(status == NU_SUCCESS)
            {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                if((handle->ike2_params->ike2_policy->ike_flags &
                    IKE_INBAND_CERT_XCHG) !=0)
                {
                    status = IKE2_Generate_CertReq_Data(handle,
                                                   out_msg->ike2_cert_req);
                }
#endif
                if(status == NU_SUCCESS)
                {
                    IKE2_END_CHAIN(out_msg->ike2_last);

                    /* Generate the responder cookie. */
                    status = RAND_bytes(handle->ike2_sa->ike2_local_spi, IKE2_SPI_LENGTH)
                             ? NU_SUCCESS : IKE_INTERNAL_ERROR;

                    if(status == NU_SUCCESS)
                    {
                        /* All needed payloads have been added. Now set values
                         * in IKE header and send the packet.
                         */
                        status = IKE2_Construct_Header(handle,
                            IKE2_SA_INIT, IKE2_HDR_RESPONSE_FLAG);

                        if(status == NU_SUCCESS)
                        {
                            /* All done! send packet now. */
                            status = IKE2_Send_Packet(handle);

                            if(status == NU_SUCCESS)
                            {
                                /* Flush outgoing SA payload. */
                                IKE2_FLUSH_SA_PAYLOAD(out_msg);

                                status = IKE2_Generate_KEYMAT(handle);

                                if(status == NU_SUCCESS)
                                {
                                    handle->ike2_state += IKE2_NEXT_STATE_INC;

                                    /* Save last sent message */
                                    handle->ike2_sa->ike2_local_auth_len =
                                        (UINT16)handle->ike2_params->
                                        ike2_out.ike2_hdr->ike2_length;

                                    status = IKE2_Save_Auth_Data(
                                        &handle->ike2_sa->ike2_local_auth_data,
                                        handle->ike2_last_message,
                                        handle->ike2_params->ike2_out.ike2_hdr->ike2_length);

                                    if(status == NU_SUCCESS)
                                    {
                                        /* Packet was sent successfully!
                                         * Set the message reply timeout timer.
                                         */
                                        status = IKE_Set_Timer(
                                            IKE2_Message_Reply_Event,
                                            (UNSIGNED)handle->ike2_sa,
                                            (UNSIGNED)handle,
                                            IKE2_MESSAGE_REPLY_TIMEOUT);

                                        if(status != NU_SUCCESS)
                                        {
                                            NLOG_Error_Log(
                                                "Message retransmission timer \
                                                could not be added",
                                                NERR_RECOVERABLE, __FILE__,
                                                __LINE__);
                                        }
                                    }

                                    else
                                    {
                                        NLOG_Error_Log("Authentication data could not be saved.",
                                            NERR_RECOVERABLE, __FILE__, __LINE__);
                                    }
                                }
                                else
                                {
                                    NLOG_Error_Log("Could not generate keying material",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);
                                }
                            }
                            else
                            {
                                NLOG_Error_Log("Packet could not be sent",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                        else
                        {
                            NLOG_Error_Log("Error occurred while encoding IKE header",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Error generating random number",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Failed to generate certificate request",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Error in nonce data generation",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Could not generate keying material",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Deallocate the dynamically allocated data in the payloads if it
     * has been allocated during the processing above.
     */
    if(ke_i.ike2_ke_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(ke_i.ike2_ke_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate KE payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        ke_i.ike2_ke_data = NU_NULL;
    }
    if(nonce_i.ike2_nonce_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(nonce_i.ike2_nonce_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Nonce data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        nonce_i.ike2_nonce_data = NU_NULL;
    }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(cert_req_i.ike2_ca != NU_NULL)
    {
        if(NU_Deallocate_Memory(cert_req_i.ike2_ca) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate CERTREQ data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        cert_req_i.ike2_ca = NU_NULL;
    }
#endif

    return (status);

}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_AUTH_I
*
* DESCRIPTION
*
*       This function receives the IKE_SA_INIT reply and sends the
*       IKE_AUTH message when we are initiators. They payloads processed
*       can be represented as:
*
*       *Initiator                               Responder
*       -----------                             -----------
*
*                                       <-- HDR, SAr1, KEr, Nr, [CERTREQ]
*       HDR, SK {IDi, [CERT,] [CERTREQ,]
*           [IDr,] AUTH, SAi2, TSi, TSr} -->
*
*
* INPUTS
*
*       *handle                 Exchange information for this exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*       IKE2_UNEXPECTED_PAYLOAD The payload is invalid.
*
*************************************************************************/
STATIC STATUS IKE2_AUTH_I(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status;
    IKE2_MESSAGE            *in_msg;
    IKE2_MESSAGE            *out_msg;
    IKE2_STATE_PARAMS       *params;

    IKE2_SA_PAYLOAD         sa_r;
    IKE2_KE_PAYLOAD         ke_r;
    IKE2_NONCE_PAYLOAD      nonce_r;
    IKE2_VENDOR_ID_PAYLOAD  vid_r;
    IKE2_ATTRIB             *in_attirbs = NU_NULL;

    IKE2_HDR                hdr_i;
    IKE2_ID_PAYLOAD         id_i;
    IKE2_AUTH_PAYLOAD       auth_i;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE2_CERT_REQ_PAYLOAD   cert_req_i; /* Certificate request from peer */
    IKE2_CERT_REQ_PAYLOAD   cert_req_r; /* Certificate request from us */
    IKE2_CERT_PAYLOAD       cert_r;     /* Certificate from our side */
#endif

    IKE2_SA_PAYLOAD         sa2;
    IKE2_TS_PAYLOAD         ts_i;
    IKE2_TS_PAYLOAD         ts_r;
    IKE2_TS                 ts_data_i;
    IKE2_TS                 ts_data_r;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_params == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Set local pointers. */
    params = handle->ike2_params;
    in_msg = &params->ike2_in;
    out_msg = &params->ike2_out;

    ts_i.ike2_ts = &ts_data_i;
    ts_r.ike2_ts = &ts_data_r;

    /* Do sanity checks here on input. */
    if(in_msg->ike2_hdr->ike2_next_payload != IKE2_SA_PAYLOAD_ID)
    {
        /* Decode all payloads in the packet. */
        status = IKE2_Decode_Message(params->ike2_packet->ike_data, &params->ike2_in);
    
        /* Check if any information payloads have been received. */
        if((status == NU_SUCCESS) && (in_msg->ike2_notify != NU_NULL))
        {
            /* Process the notification. */
            status = IKE2_Process_Notify(handle);

            if(status == NU_SUCCESS)
            {
                /* If the response is indicating that we should retransmit the init with
                 * a cookie. */
                if(handle->ike2_flags & IKE2_USE_INIT_COOKIE)
                {
                    /* Send the INIT SA packet again but this time append the N(COOKIE)
                     * payload to it. */
                    status = IKE2_SA_INIT_I(handle);

                    /* Clear the IKE2_USE_INIT_COOKIE flag now. This should be done regardless
                     * of the status returned above. */
                    handle->ike2_flags &= ~IKE2_USE_INIT_COOKIE;

                    /* No need to process ahead and we also do not increment handle-ike2_state
                     * for this control path so that we remain at the IKE2_AUTH_I state. */
                    return (status);
                }
            }

            /* An error occurred when processing the notify. */
            else
            {
                status = IKE2_UNEXPECTED_PAYLOAD;
                NLOG_Error_Log("Failed to process notify", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
                return (status);
            }
        }

        else
        {
            status = IKE2_UNEXPECTED_PAYLOAD;
            NLOG_Error_Log("SA payload expected but not found",
                NERR_RECOVERABLE, __FILE__, __LINE__);
            return (status);
        }
    }

    /* The following members are dynamically allocated so initialize
     * them to a known value here and we will need to deallocate them
     * when we're done processing.
     */
    ke_r.ike2_ke_data = NU_NULL;
    nonce_r.ike2_nonce_data = NU_NULL;
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    cert_req_r.ike2_ca = NU_NULL;
#endif

    /* Read the reply to IKE_SA_INIT */
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_sa, &sa_r);
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_ke, &ke_r);
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_nonce, &nonce_r);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_vid, &vid_r);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_cert_req, &cert_req_i);
#endif

    /* Decode the payloads out of the incoming message */
    status = IKE2_Decode_Message(params->ike2_packet->ike_data, in_msg);

    if(status == NU_SUCCESS)
    {
        /* We have received reply for our SA_INIT request. First unset
         * any message reply timeout events we have set for this request.
         */
        status = IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                 (UNSIGNED)handle->ike2_sa,
                                 (UNSIGNED)handle);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to unset message reply timeout event",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Packet has successfully been decoded. */

        /* Check if any information payloads have been received. */
        if((status == NU_SUCCESS) && (in_msg->ike2_notify != NU_NULL))
        {
            status = IKE2_Process_Notify(handle);

            if(status != NU_NULL)
            {
                IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                (UNSIGNED)handle->ike2_sa,
                                (UNSIGNED)handle);

                NLOG_Error_Log("Failed to process Notify payload",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        if(status == NU_SUCCESS)
        {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
            /* See if a CERT_REQ was sent. If yes, we need to send back a
             * certificate (if our policy allows it).
             */
            if(IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_cert_req))
            {
                /* Set the flag in exchange handle to send the certificate
                 * while replying to this message.
                 */
                handle->ike2_flags |= IKE_CERTREQ_RECEIVED;
            }
#endif
            /* See if any of the proposals is acceptable to us. The proposal
             * in this case must be one of the proposals we sent. It must
             * match one of our own proposals.
             */
            status = IKE2_Select_Proposal(handle, &in_attirbs);

            if(status == NU_SUCCESS)
            {
                /* Extract the values from the proposal structure and fill the
                 * SA structure now since we now agree on the parameters.
                 */
                status = IKE2_Extract_IKE_SA_Attribs(in_attirbs, handle->ike2_sa);

                if(status == NU_SUCCESS)
                {
                    /* Allocate buffer for DH public values. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                        (VOID**)&handle->ike2_remote_dh,
                        in_msg->ike2_ke->ike2_ke_data_len, NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        handle->ike2_remote_dh = TLS_Normalize_Ptr(
                            handle->ike2_remote_dh);
                        NU_BLOCK_COPY(handle->ike2_remote_dh,
                            in_msg->ike2_ke->ike2_ke_data,
                            in_msg->ike2_ke->ike2_ke_data_len);

                        handle->ike2_remote_dh_len =
                            in_msg->ike2_ke->ike2_ke_data_len;

                        /* Allocate buffer for nonce. */
                        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                            (VOID**)&handle->ike2_peer_nonce,
                            in_msg->ike2_nonce->ike2_nonce_data_len, NU_NO_SUSPEND);

                        if(status == NU_SUCCESS)
                        {
                            handle->ike2_peer_nonce = TLS_Normalize_Ptr(
                                handle->ike2_peer_nonce);
                            NU_BLOCK_COPY(handle->ike2_peer_nonce,
                                in_msg->ike2_nonce->ike2_nonce_data,
                                in_msg->ike2_nonce->ike2_nonce_data_len);

                            handle->ike2_peer_nonce_len =
                                (UINT8)(in_msg->ike2_nonce->ike2_nonce_data_len);

                            /* Copy the initiator's SPI. We need to send
                             * it back to the peer in responses.
                             */
                            NU_BLOCK_COPY(handle->ike2_sa->ike2_remote_spi,
                                          in_msg->ike2_hdr->ike2_sa_spi_r,
                                          IKE2_SPI_LENGTH);

                            handle->ike2_sa->ike2_peer_auth_len =
                                handle->ike2_params->ike2_packet->ike_data_len;
                            status = IKE2_Save_Auth_Data(
                                &handle->ike2_sa->ike2_peer_auth_data,
                                handle->ike2_params->ike2_packet->ike_data,
                                handle->ike2_params->ike2_packet->ike_data_len);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Authentication data could not be saved",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                        else
                        {
                            NLOG_Error_Log("Memory allocation failed for peer's nonce",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
                    else
                    {
                        NLOG_Error_Log("Memory allocation failed for remote key",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Received attributed are not valid",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Returned proposal invalid", NERR_RECOVERABLE,
                    __FILE__, __LINE__);
            }
        }
    }
    else
    {
        NLOG_Error_Log("Packet could not be decoded.", NERR_RECOVERABLE,
            __FILE__, __LINE__);
    }

    if(status == NU_SUCCESS)
    {
        /* If packet was received and decoded successfully and all went
         * well so far, generate keying material from the information
         * exchanged so far.
         */
        status = IKE2_Generate_KEYMAT(handle);

        /* Flush both incoming and outgoing SA payloads. */
        IKE2_FLUSH_SA_PAYLOAD(in_msg);
        IKE2_FLUSH_SA_PAYLOAD(out_msg);

    }
    /* The "if" above does not have an else and does not need one
     * because if any error occurred has already been logged above.
     * If the statement insides this "if" fails, the error will be
     * logged in the "else" part of next "if" statement.
     */

    /* Incoming packet has been decoded and keys generated. Now construct
     * and send AUTH packet.
     */
    if(status == NU_SUCCESS)
    {
        IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr_i);

        IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr,
                        IKE2_NONE_PAYLOAD_ID);

        IKE2_SET_OUTBOUND(out_msg->ike2_id_i, &id_i);
        IKE2_SET_OUTBOUND(out_msg->ike2_auth, &auth_i);

        /* Generate ID and append the data to the response packet. */
        status = IKE2_Generate_ID_Data(handle, &id_i);

        if(status == NU_SUCCESS)
        {
            /* Generate authentication data. */
            status = IKE2_Generate_AUTH_Data(handle, &auth_i);

            if(status == NU_SUCCESS)
            {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                /* If we received a CERT_REQ or our policy mandates sending
                 * a certificate, send one now and also send a CERT_REQ so
                 * that the peer also sends one if it won't send pro-actively.
                 */
                if(((handle->ike2_params->ike2_policy->ike_flags &
                    IKE_INBAND_CERT_XCHG) !=0) ||
                    ((handle->ike2_flags & IKE_CERTREQ_RECEIVED) != 0))
                {
                    /* Encode and add the certificate data to the outgoing
                     * message. If this node is using rsa for authentication.
                     */
                    if((handle->ike2_sa->ike_attributes.ike_auth_method ==
                        IKE2_AUTH_METHOD_RSA_DS))
                    {
                        status = IKE2_Generate_Cert_Data(handle, &cert_r);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to generate CERT data",
                                          NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                    }

                    if(status == NU_SUCCESS)
                    {
                        /* Add CERT_REQ. */
                        status = IKE2_Generate_CertReq_Data(handle, &cert_req_r);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to generate CERT_REQ data",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                }

                if(status == NU_SUCCESS)
                {
#endif
                    IKE2_SET_OUTBOUND(out_msg->ike2_sa, &sa2);
                    IKE2_SET_OUTBOUND(out_msg->ike2_ts_i, &ts_i);
                    IKE2_SET_OUTBOUND(out_msg->ike2_ts_r, &ts_r);

                    if((handle->ike2_flags & IKE2_USE_TRANSPORT_MODE) != 0)
                    {
                        status = IKE2_Generate_Notify(
                                    handle, IKE2_PROTO_ID_RESERVED,
                                    IKE2_NOTIFY_USE_TRANSPORT_MODE,
                                    NU_NULL,  /* No notify data */
                                    0,        /* No notify data length. */
                                    0,        /* No SPI, zero size */
                                    NU_NULL   /* No SPI */);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to add Notify payload for \
                                           transport mode", NERR_RECOVERABLE,
                                           __FILE__, __LINE__);
                        }
                    }

                /* The first Child SA will be created as part of this AUTH
                 * exchange. Add the IPsec specific payloads to the packet
                 * as well. We have all the information necessary for that
                 * at this point.
                 */
                status = IKE2_Create_IPsec_SA_Payloads(handle);

                if(status == NU_SUCCESS)
                {
                    /* Generate the IV for encryption. */
                    if (NU_TRUE != RAND_bytes(handle->ike2_sa->ike_encryption_iv, IKE_MAX_ENCRYPT_BLOCK_LEN))
                    {
                        NLOG_Error_Log("Error generating random data.",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }

                    IKE2_END_CHAIN(out_msg->ike2_last);
                    /* The packet is now ready to be sent. Add the IKEv2
                     * header now. Set the flags for current exchange and
                     * whether we are initiator or responder.
                     */
                    status = IKE2_Construct_Header(handle, IKE2_AUTH,
                                                   IKE2_HDR_INITIATOR_FLAG);

                    if(status == NU_SUCCESS)
                    {
                        /* We will need to encrypt the packet before
                         * sending. Set the key to appropriate value.
                         */
                        if((handle->ike2_flags & IKE2_INITIATOR) == 0)
                        {
                            /* We are the responder. */
                            handle->ike2_sa->ike_encryption_key =
                                handle->ike2_sa->ike2_sk_er;
                        }

                        else
                        {
                            /* We are the initiator. */
                            handle->ike2_sa->ike_encryption_key =
                                handle->ike2_sa->ike2_sk_ei;
                        }

                        /* Send the packet to the peer. */
                        status = IKE2_Send_Packet(handle);

                        if(status == NU_SUCCESS)
                        {
                            /* Packet was sent successfully. Update the
                             * state for this exchange.
                             */
                            handle->ike2_state += IKE2_NEXT_STATE_INC;

                            /* Packet was sent successfully!
                             * Set the message reply timeout timer.
                             */
                            status = IKE_Set_Timer(IKE2_Message_Reply_Event,
                                                   (UNSIGNED)handle->ike2_sa,
                                                   (UNSIGNED)handle,
                                                   IKE2_MESSAGE_REPLY_TIMEOUT);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Message retransmission timer \
                                               could not be added",
                                               NERR_RECOVERABLE, __FILE__,
                                               __LINE__);
                            }
                        }
                    }
                }
                else
                {
                    NLOG_Error_Log("IPsec SA payloads could not be added",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                }
#endif
            }
            else
            {
                NLOG_Error_Log("Failed to generate AUTH data",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Failed to generate ID data", NERR_RECOVERABLE,
                __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Keying material could not be generated", NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    }

    /* Deallocate the dynamically allocated payload data if it
     * has been allocated during the processing above.
     */
    if(ke_r.ike2_ke_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(ke_r.ike2_ke_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate KE payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        ke_r.ike2_ke_data = NU_NULL;
    }
    if(nonce_r.ike2_nonce_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(nonce_r.ike2_nonce_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Nonce payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        nonce_r.ike2_nonce_data = NU_NULL;
    }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(cert_req_r.ike2_ca != NU_NULL)
    {
        if(NU_Deallocate_Memory(cert_req_r.ike2_ca) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate CERTREQ payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        cert_req_r.ike2_ca = NU_NULL;
    }
#endif

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_AUTH_R
*
* DESCRIPTION
*
*       This function receives the IKE_AUTH request and sends the
*       IKE_AUTH reply. After this call, the first child SA will also
*       get established as the first child SA exchange is piggy-backed
*       on the IKE_AUTH exchange. The payloads exchanged can be
*       represented as:
*
*       Initiator                               *Responder
*       -----------                             -----------
*       HDR, SK {IDi, [CERT,] [CERTREQ,]
*           [IDr,] AUTH, SAi2, TSi, TSr} -->
*                                        <-- HDR, SK {IDr, [CERT,] AUTH,
*                                               SAr2, TSi, TSr}
*
* INPUTS
*
*       *handle                 Exchange handle for this exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATIC STATUS IKE2_AUTH_R(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status;

    IKE2_MESSAGE        *out_msg;

    IKE2_ID_PAYLOAD     my_id;
    IKE2_AUTH_PAYLOAD   my_auth;
    IKE2_HDR            hdr_r;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE2_CERT_PAYLOAD       my_cert;
#endif

    IKE2_SA_PAYLOAD         sa2;
    IKE2_TS_PAYLOAD         ts_i;
    IKE2_TS_PAYLOAD         ts_r;
    IKE2_TS                 ts_data_i[IKE2_MAX_SELECTORS_IN_TS];
    IKE2_TS                 ts_data_r[IKE2_MAX_SELECTORS_IN_TS];

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    out_msg = &handle->ike2_params->ike2_out;
    ts_i.ike2_ts = &(ts_data_i[0]);
    ts_r.ike2_ts = &(ts_data_r[0]);

    /* HDR, SK {IDr, [CERT,] AUTH, SAr2, TSi, TSr} */
    IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr_r);
    IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr, IKE2_NONE_PAYLOAD_ID);

    IKE2_SET_OUTBOUND(out_msg->ike2_id_r, &my_id);
    IKE2_SET_OUTBOUND(out_msg->ike2_auth, &my_auth);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(((handle->ike2_params->ike2_policy->ike_flags &
        IKE_INBAND_CERT_XCHG) !=0) ||
        ((handle->ike2_flags & IKE_CERTREQ_RECEIVED) != 0))
    {
        IKE2_SET_OUTBOUND(out_msg->ike2_cert, &my_cert);
    }
#endif

    IKE2_SET_OUTBOUND(out_msg->ike2_sa, &sa2);
    IKE2_SET_OUTBOUND(out_msg->ike2_ts_i, &ts_i);
    IKE2_SET_OUTBOUND(out_msg->ike2_ts_r, &ts_r);

    /* Encode our ID and add to the outgoing message. */
    status = IKE2_Generate_ID_Data(handle, &my_id);

    if(status == NU_SUCCESS)
    {
        /* Set Responder's payload data in Selector. */
        status = IKE2_IPS_ID_To_Selector(&my_id,
                    &(handle->ike2_params->ike2_packet->ike_local_addr),
                    &(handle->ike2_ips_selector), IKE_REMOTE);
    }

    if(status == NU_SUCCESS)
    {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        if((handle->ike2_sa->ike_attributes.ike_auth_method ==
            IKE2_AUTH_METHOD_RSA_DS) &&
              (((handle->ike2_params->ike2_policy->ike_flags &
                IKE_INBAND_CERT_XCHG) !=0) ||
               ((handle->ike2_flags & IKE_CERTREQ_RECEIVED) != 0)))
        {
            /* Encode and add our certificate. */
            status = IKE2_Generate_Cert_Data(handle, &my_cert);
        }

        if(status == NU_SUCCESS)
        {
#endif
        /* Generate the AUTH data. */
        status = IKE2_Generate_AUTH_Data(handle, &my_auth);

        if(status == NU_SUCCESS)
        {
            status = IKE2_AUTH_Receive(handle);

            /*
             * Incoming packet processed. Now send a response.
             */

            if(status == NU_SUCCESS)
            {
                /* The AUTH response is ready. Add IPsec payloads for the
                 * first child SA.
                 */
                status = IKE2_Create_IPsec_SA_Payloads(handle);

                if(status == NU_SUCCESS)
                {
                    IKE2_END_CHAIN(out_msg->ike2_last);
                    /* All done. Now add the IKEv2 header, exchange identifier
                     * and response flag.
                     */
                    status = IKE2_Construct_Header(handle, IKE2_AUTH,
                                                   IKE2_HDR_RESPONSE_FLAG);

                    if(status == NU_SUCCESS)
                    {
                        /* We will need to encrypt the packet before
                         * sending. Set the key to appropriate value.
                         */
                        if((handle->ike2_flags & IKE2_INITIATOR) == 0)
                        {
                            /* We are the responder. */
                            handle->ike2_sa->ike_encryption_key =
                                handle->ike2_sa->ike2_sk_er;
                        }

                        else
                        {
                            /* We are the initiator. */
                            handle->ike2_sa->ike_encryption_key =
                                handle->ike2_sa->ike2_sk_ei;
                        }

                        /* Send the message to the peer. */
                        status = IKE2_Send_Packet(handle);

                        if(status == NU_SUCCESS)
                        {
                            /* Message has been sent, now update the state
                             * of this exchange for further messages on this
                             * exchange.
                             */
                            handle->ike2_state += IKE2_NEXT_STATE_INC;

                        }

                        else
                        {
                            NLOG_Error_Log("Failed to send packet!",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to construct IKE header",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("IPsec SA payloads could not be added",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to decode incoming packet.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to generate Authentication data",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        }
        else
        {
            NLOG_Error_Log("Failed to add CERT payload", NERR_RECOVERABLE,
                __FILE__, __LINE__);
        }
#endif
    }

    else
    {
        NLOG_Error_Log("Failed to generate ID data", NERR_RECOVERABLE,
            __FILE__, __LINE__);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_AUTH_I_Response
*
* DESCRIPTION
*
*       This function receives the response to the IKE_AUTH message when
*       when we are the initiators. The payloads exchanged can be
*       represented as below:
*
*       *Initiator                       Responder
*       -----------                     -----------
*                            <-- HDR, SK {IDr, [CERT,] AUTH,SAr2, TSi, TSr}
*
* INPUTS
*
*       *handle                 Exchange handle for this exchange
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Invalid parameters.
*
*************************************************************************/
STATIC STATUS IKE2_AUTH_I_Response(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS          status;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Receive the actual response from the peer. */
    status = IKE2_AUTH_Receive(handle);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("AUTH packet could not be received",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_AUTH_Receive
*
* DESCRIPTION
*
*       This function is used to process the incoming IKE_AUTH payloads
*       (incoming IKE_AUTH request or response to an IKE_AUTH request we
*       sent). It is called by the main state machine functions in both
*       cases of being initiator and responder.
*
* INPUTS
*
*       *handle                 Exchange handle for this exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATIC STATUS IKE2_AUTH_Receive(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_MESSAGE            *in_msg;
    IKE2_STATE_PARAMS       *params;
    IKE2_ID_PAYLOAD         peer_id;
    IKE2_ID_PAYLOAD         my_id;
    IKE2_AUTH_PAYLOAD       peer_auth;
    INT                     narrow_proto;
    UINT8                   tsi_selec_index;
    UINT8                   tsr_selec_index;

    IKE2_SA_PAYLOAD         sa2;
    IKE2_TS_PAYLOAD         *ts_i;
    IKE2_TS_PAYLOAD         *ts_r;

    IKE2_TS_PAYLOAD         ts_i_init;
    IKE2_TS_PAYLOAD         ts_r_init;
    IKE2_TS                 ts_data_i;
    IKE2_TS                 ts_data_r;
    IKE2_CONFIG_PAYLOAD     rmt_cfg;
    IKE2_VENDOR_ID_PAYLOAD  vid_peer;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE2_CERT_PAYLOAD       peer_cert;
    IKE2_CERT_REQ_PAYLOAD   peer_cert_req;
    UINT8                   *remote_key = NU_NULL;
    UINT32                  remote_key_len = 0;
    UINT8                   *peer_cert_local;
    UINT16                  peer_cert_len;
#endif

    params = handle->ike2_params;
    in_msg = &params->ike2_in;

    /* The following member is dynamically allocated so initialize
     * it to a known value here and we will need to deallocate it
     * when we're done processing.
     */
    peer_auth.ike2_auth_data = NU_NULL;

    if((handle->ike2_flags & IKE2_INITIATOR) != 0)
    {
        UTL_Zero(&ts_i_init, sizeof(IKE2_TS_PAYLOAD));
        UTL_Zero(&ts_r_init, sizeof(IKE2_TS_PAYLOAD));
        /* We are initiators and this packet is the last one in exchange.
         * Set the TS payload pointers to local structures.
         */
        ts_i = &ts_i_init;
        ts_r = &ts_r_init;
        ts_i->ike2_ts = &ts_data_i;
        ts_r->ike2_ts = &ts_data_r;
    }

    else
    {
        /* We are responder, we still have to send the packet so set
         * the TS payload pointers to outgoing payloads.
         */
        ts_i = handle->ike2_params->ike2_out.ike2_ts_i;
        ts_r = handle->ike2_params->ike2_out.ike2_ts_r;
    }

    if(in_msg->ike2_hdr->ike2_next_payload != IKE2_ENCRYPT_PAYLOAD_ID)
    {
        status = IKE2_UNEXPECTED_PAYLOAD;
        NLOG_Error_Log("Encrypted payload expected but not found",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Set which key to use to decrypt the packet. */
        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            /* We are the initiator. Use responder key to decrypt. */
            handle->ike2_sa->ike_encryption_key =
                                            handle->ike2_sa->ike2_sk_er;
        }

        else
        {
            /* We are the responder. Use initiator key to decrypt. */
            handle->ike2_sa->ike_encryption_key =
                                            handle->ike2_sa->ike2_sk_ei;
        }
        /* Process incoming packet */
        status = IKE2_Decode_IKE_Encrypted_Payload(
                    params->ike2_packet->ike_data,
                    params->ike2_packet->ike_data_len,
                    handle->ike2_sa);

        if(status == NU_SUCCESS)
        {
            /* The total length and next payload ID have been updated
             * in the buffer, update them in the IKEv2 header as well.
             * (This header was already decoded and needs to be updated
             * separately)
             */
            in_msg->ike2_hdr->ike2_length = GET32(
                                            params->ike2_packet->ike_data,
                                            IKE2_HDR_LEN_OFST);
            in_msg->ike2_hdr->ike2_next_payload = GET8(
                                            params->ike2_packet->ike_data,
                                            IKE2_HDR_NXT_PYLD_OFST);

            if((handle->ike2_flags & IKE2_INITIATOR) != 0)
            {
                IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_id_r, &peer_id);
                IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_id_i, &my_id);
            }

            else
            {
                IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_id_i, &peer_id);
                IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_id_r, &my_id);
            }

            IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_auth, &peer_auth);

            IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_sa,   &sa2);

            IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ts_i, ts_i);
            IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ts_r, ts_r);
            IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_cfg, &rmt_cfg);
            IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_vid, &vid_peer);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
            IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_cert, &peer_cert);
            IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_cert_req, &peer_cert_req);
#endif

            /* Now decode the message. */
            status = IKE2_Decode_Message(params->ike2_packet->ike_data,
                                         in_msg);

            /* Check if any information payloads have been received. */
            if((status == NU_SUCCESS) && (in_msg->ike2_notify != NU_NULL))
            {
                status = IKE2_Process_Notify(handle);

                if(status != NU_NULL)
                {
                    IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                    (UNSIGNED)handle->ike2_sa,
                                    (UNSIGNED)handle);

                    NLOG_Error_Log("Failed to process Notify payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            if((status == NU_SUCCESS) && (in_msg->ike2_del != NU_NULL))
            {
                status = IKE2_Process_Delete(handle);

                if(status != NU_NULL)
                {
                    NLOG_Error_Log("Failed to process Delete payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            if((status == NU_SUCCESS) && (in_msg->ike2_id_i != NU_NULL))
            {
                /* Set Initiator's payload data in Selector. */
                status = IKE2_IPS_ID_To_Selector(in_msg->ike2_id_i,
                                &(handle->ike2_sa->ike_node_addr),
                                &(handle->ike2_ips_selector), IKE_LOCAL);
            }

            if(status == NU_SUCCESS)
            {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                if(IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_cert_req))
                {
                    handle->ike2_flags |= IKE_CERTREQ_RECEIVED;
                }

                if(IKE2_IS_RSA_METHOD(
                    handle->ike2_params->ike2_in.ike2_auth->ike2_auth_method))
                {
                    if((handle->ike2_params->ike2_policy->ike_flags &
                        IKE_INBAND_CERT_XCHG) != 0)
                    {
                        if(IKE2_PAYLOAD_IS_PRESENT(
                            in_msg->ike2_cert) == NU_TRUE)
                        {
                            status = IKE_Cert_Get_PKCS1_Public_Key(
                                in_msg->ike2_cert->ike2_cert_data,
                                in_msg->ike2_cert->ike2_cert_data_len,
                                &remote_key, &remote_key_len);
                        }
                    }

                    else
                    {
                        /* In-band certificate exchange is not desired */
                        if(handle->ike2_sa->ike_attributes.ike_peer_cert_file != NU_NULL)
                        {
                            status = IKE_Cert_Get(
                                handle->ike2_sa->ike_attributes.ike_peer_cert_file,
                                &peer_cert_local, &peer_cert_len,
                                handle->ike2_sa->ike_attributes.ike_cert_encoding);
                            if(status == NU_SUCCESS)
                            {
                                status = IKE_Cert_Get_PKCS1_Public_Key(peer_cert_local,
                                    peer_cert_len, &remote_key, &remote_key_len);
                            }

                            if((peer_cert_local != NU_NULL) &&
                                (NU_Deallocate_Memory(peer_cert_local)
                                    != NU_SUCCESS))
                            {
                                NLOG_Error_Log("Failed to deallocate memory for \
                                           peer's certificate", NERR_RECOVERABLE,
                                           __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            status = IKE_NOT_FOUND;
                            NLOG_Error_Log("Peer's certificate is not specified",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                    }

                    if(status == NU_SUCCESS)
                    {
                        /* Ours and peer's public keys have been read
                         * successfully.
                         */
                        handle->ike2_sa->ike_attributes.ike_remote_key =
                                                                remote_key;
                        handle->ike2_sa->ike_attributes.ike_remote_key_len =
                                                    (UINT16)remote_key_len;
                    }
                }
#endif
                /* Verify the authentication data. */
                status = IKE2_Verify_Auth(handle);

                if(status == NU_SUCCESS)
                {
                    /* We have received reply for our SA_INIT request.
                     * First unset any message reply timeout events we
                     * have set for this request.
                     */
                    status = IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                             (UNSIGNED)handle->ike2_sa,
                                             (UNSIGNED)handle);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to unset message reply \
                                       timeout event", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }

                    /* IKE SA is good. Now proceed to see if we have
                     * received an IPsec SA request as well.
                     */
                    if(in_msg->ike2_sa != NU_NULL)
                    {

                        status = IPSEC_NOT_FOUND;

                        /* Create a selector out of proposed traffic selectors
                         * that match any of our IPSec policies.
                         */
                        for(tsi_selec_index = 0;
                                    (tsi_selec_index < in_msg->ike2_ts_i->
                                    ike2_ts_count) && (status != NU_SUCCESS);
                                    tsi_selec_index++)
                        {
                            for(tsr_selec_index = 0;
                                    (tsr_selec_index < in_msg->ike2_ts_r->
                                    ike2_ts_count) && (status != NU_SUCCESS);
                                    tsr_selec_index++)
                            {

                                /* Nucleus IPSec only support same protocol,
                                 * for incoming and outgoing traffic, per policy.
                                 */
                                if((in_msg->ike2_ts_i->ike2_ts[tsi_selec_index].
                                    ike2_ip_protocol_id == in_msg->ike2_ts_r->
                                    ike2_ts[tsr_selec_index].ike2_ip_protocol_id))
                                {
                                    /* Extract TS sent by peer. */
                                    IKE2_Extract_TS_Info(handle, tsi_selec_index, tsr_selec_index);
                                }
                                else
                                    continue;

                                /* Get the IPsec Policy Index matching TS sent by peer. */
                                if((handle->ike2_flags & IKE2_INITIATOR) != 0)
                                {
                                    /* If we are the initiator, we'll be sending out packet so we
                                     * need to lookup outbound policy.
                                     */
                                    status  = IPSEC_Get_Policy_Index_Narrow(handle->ike2_ips_group,
                                                                     &handle->ike2_ips_selector,
                                                                     IPSEC_OUTBOUND | IPSEC_APPLY,
                                                                     &(handle->ike2_ips_policy_index),
                                                                     NU_NULL);
                                }

                                else
                                {
                                    /* If we are the responder, we'll be sending out packet so we
                                     * need to lookup inbound policy.
                                     */
                                    status  = IPSEC_Get_Policy_Index_Narrow(handle->ike2_ips_group,
                                                                     &handle->ike2_ips_selector,
                                                                     IPSEC_INBOUND | IPSEC_APPLY,
                                                                     &(handle->ike2_ips_policy_index),
                                                                     &narrow_proto);

                                    if((status == NU_SUCCESS) && (narrow_proto != -1))
                                    {
                                        /* Update the narrowed protocol in the outgoing Traffic
                                         * Selectors accordingly.
                                         */
                                        if((handle->ike2_params->ike2_out.ike2_ts_i != NU_NULL) &&
                                         (handle->ike2_params->ike2_out.ike2_ts_i->ike2_ts != NU_NULL))
                                        {
                                            handle->ike2_params->ike2_out.ike2_ts_i->ike2_ts[tsi_selec_index].
                                                ike2_ip_protocol_id = (UINT8)narrow_proto;
                                        }

                                        if((handle->ike2_params->ike2_out.ike2_ts_r != NU_NULL) &&
                                         (handle->ike2_params->ike2_out.ike2_ts_r->ike2_ts != NU_NULL))
                                        {
                                            handle->ike2_params->ike2_out.ike2_ts_r->ike2_ts[tsr_selec_index].
                                                ike2_ip_protocol_id = (UINT8)narrow_proto;
                                        }
                                    }
#if IKE2_MAX_SELECTORS_IN_TS > 1
                                    if(status == NU_SUCCESS)
                                    {
                                        /* Move the chosen traffic sectors on top as the
                                         * same TS would be sent back. */
                                        if(tsi_selec_index > 0)
                                        {
                                            NU_BLOCK_COPY(&params->ike2_in.ike2_ts_i->ike2_ts[0],
                                                    &params->ike2_in.ike2_ts_i->ike2_ts[tsi_selec_index],
                                                    sizeof(IKE2_TS));
                                        }
                                        if(tsr_selec_index > 0)
                                        {
                                            NU_BLOCK_COPY(&params->ike2_in.ike2_ts_r->ike2_ts[0],
                                                    &params->ike2_in.ike2_ts_r->ike2_ts[tsr_selec_index],
                                                    sizeof(IKE2_TS));
                                        }
                                    }
#endif
                                }
                            }/* for tsr_selec_index */
                        }/* for tsi_selec_index */

                        if(status == NU_SUCCESS)
                        {
                            status = IKE2_Process_IPsec_SA_Payloads(handle);

                            if(status == NU_SUCCESS)
                            {
                                /* IPsec SA established successfully. Remove
                                 * any events on this exchange handle.
                                 */
                                 IKE2_Unset_Matching_Timers(
                                     (UNSIGNED)handle, 0,
                                     TQ_CLEAR_ALL_EXTRA);

                                 IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                                 (UNSIGNED)handle->ike2_sa,
                                                 (UNSIGNED)handle);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("TS don't match our policy",
                                           NERR_RECOVERABLE, __FILE__,
                                           __LINE__);
                        }
                    }
                }
                else
                {
                    NLOG_Error_Log("Authentication failed!", NERR_RECOVERABLE,
                        __FILE__, __LINE__);

                    /* Authentication failed. The SA added is not to be
                     * used so delete that SA.
                     */
                    status = IKE2_Delete_IKE_SA(
                                        &params->ike2_policy->ike_sa_list,
                                        handle->ike2_sa);
                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to delete unauthenticated SA",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Could not decode payloads", NERR_RECOVERABLE,
                    __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Could not decrypt received packet!",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Deallocate the dynamically allocated payload data if it
     * has been allocated during the processing above.
     */
    if(peer_auth.ike2_auth_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(peer_auth.ike2_auth_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate AUTH payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        peer_auth.ike2_auth_data = NU_NULL;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_CREATE_CHILD_SA_I
*
* DESCRIPTION
*
*       This function sends a CREATE_CHILD_SA request when we are the
*       initiator of the exchange. The payloads sent by this function can
*       be represented as:
*
*       *Initiator                                   Responder
*       -----------                                 -----------
*       HDR, SK {[N], SA, Ni, [KEi],[TSi, TSr]} -->
*
* INPUTS
*
*       handle                  Exchange handle representing current
*                               exchange under process
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_CREATE_CHILD_SA_I(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_MESSAGE        *out_msg;
    IKE2_STATE_PARAMS   *params;

    IKE2_HDR            hdr_i;
    IKE2_SA_PAYLOAD     sa;
    IKE2_NONCE_PAYLOAD  nonce_i;
    IKE2_KE_PAYLOAD     ke_i;

    IKE2_TS_PAYLOAD     ts_i;
    IKE2_TS_PAYLOAD     ts_r;
    IKE2_TS             ts_data_i;
    IKE2_TS             ts_data_r;
    IKE_SA2             *ike_sa2;
    IPSEC_OUTBOUND_SA   *sa_ptr_ob = NU_NULL;
    IPSEC_POLICY_GROUP  *ips_group;

#if(IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Set local pointers for use in this function. */
    params = handle->ike2_params;
    out_msg = &params->ike2_out;

    ts_i.ike2_ts = &ts_data_i;
    ts_r.ike2_ts = &ts_data_r;

    UTL_Zero(&hdr_i, sizeof(IKE2_HDR));
    UTL_Zero(&sa, sizeof(IKE2_SA_PAYLOAD));
    UTL_Zero(&nonce_i, sizeof(IKE2_NONCE_PAYLOAD));
    UTL_Zero(&ke_i, sizeof(IKE2_KE_PAYLOAD));

    /* Set the payload pointer to receive from incoming message. */
    IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr_i);

    IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr,
                    IKE2_NONE_PAYLOAD_ID);

    IKE2_SET_OUTBOUND(out_msg->ike2_sa, &sa);
    IKE2_SET_OUTBOUND(out_msg->ike2_nonce, &nonce_i);

    if(status == NU_SUCCESS)
    {
        if((handle->ike2_flags & IKE2_SA_REKEY) != 0)
        {
            /* It is an IKE SA rekey request. */

            IKE2_SET_OUTBOUND(out_msg->ike2_ke, &ke_i);

            /* If there is no SA for this exchange, allocate a new one. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID **)&handle->ike2_new_sa,
                                        sizeof(IKE2_SA), NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                UTL_Zero(handle->ike2_new_sa, sizeof(IKE2_SA));

                /* If this child SA exchange switches the initial role of
                 * Initiator and Responder, then keep track of this.
                 */
                if((handle->ike2_flags & IKE2_INITIATOR) == 0)
                {
                    handle->ike2_new_sa->ike2_switch_role = NU_TRUE;
                }

                /* If this child exchange is meant to re-key an IKE SA,
                 * construct IKE proposals.
                 */
                status = IKE2_Extract_IKE_SA_Attribs(
                            handle->ike2_params->ike2_policy->ike_xchg1_attribs,
                            handle->ike2_sa);

                if(status == NU_SUCCESS)
                {
                    status = IKE2_Construct_Proposal(handle);

                    if(status == NU_SUCCESS)
                    {
                        /* Generate they keying material for this exchange. */
                        status = IKE2_Generate_KE_Data(handle, &ke_i);

                        if(status == NU_SUCCESS)
                        {
                            /* Generate nonce data. */
                            status = IKE2_Generate_Nonce_Data(handle, &nonce_i);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to generate nonce data",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to generate KE data",
                                NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate IKE proposal for re-keying",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }

        else
        {
            /* It is an IPsec SA rekey request. */

            IPSEC_Get_Group_Entry(handle->ike2_ips_group, &ips_group);

            /* Add REKEY_SA notification for each SA pair. */
            for(ike_sa2 = handle->ike2_sa2_db.ike_flink;
                    (ike_sa2 != NU_NULL) && (status == NU_SUCCESS);
                                        ike_sa2 = ike_sa2->ike_flink)
            {
                /* Find the SAs that have matching selector and security. */
                if(IPSEC_Get_Outbound_SA(ips_group, &handle->ike2_ips_selector,
                                &ike_sa2->ike_ips_security, &sa_ptr_ob) != NU_SUCCESS)
                    break;

                status = IKE2_Generate_Notify(handle,
                                              IKE_Protocol_ID_IPS_To_IKE(sa_ptr_ob->
                                                    ipsec_security.ipsec_protocol),
                                              IKE2_NOTIFY_REKEY_SA, NU_NULL, 0,
                                              sizeof(sa_ptr_ob->ipsec_remote_spi),
                                              (UINT8 *)&sa_ptr_ob->ipsec_remote_spi);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to add Notify payload for \
                                   SA REKEY mode", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);
                }
            }

            /* If using Transport Mode. Add USE_TRANSPORT_MODE notification. */
            if((handle->ike2_flags & IKE2_USE_TRANSPORT_MODE) != 0)
            {
                status = IKE2_Generate_Notify(handle, IKE2_PROTO_ID_RESERVED,
                                              IKE2_NOTIFY_USE_TRANSPORT_MODE,
                                              NU_NULL,  /* No notify data */
                                              0,        /* No notify data length */
                                              0,        /* No SPI, zero size */
                                              NU_NULL   /* No SPI */);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to add Notify payload for \
                                   transport mode", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);
                }
            }

            /* This exchange is to create a child SA, therefore, create IPsec
             * proposals.
             */
            status = IKE2_Construct_IPsec_Proposal(handle);

            if(status == NU_SUCCESS)
            {
                /* Generate NONCE for this child exchange. */
                status = IKE2_Generate_Nonce_Data(handle, &nonce_i);

                if(status == NU_SUCCESS)
                {
                    IKE2_SET_OUTBOUND(out_msg->ike2_ts_i, &ts_i);
                    IKE2_SET_OUTBOUND(out_msg->ike2_ts_r, &ts_r);

                    /* Generate TS payloads for this child SA. */
                    status = IKE2_Generate_TS(handle);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to generate TS payloads",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Nonce data could not be generated",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Proposal could not be generated",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }


        if(status == NU_SUCCESS)
        {
            IKE2_END_CHAIN(out_msg->ike2_last);

            /* The packet is now ready to be sent. Add the IKEv2
             * header now. Set the flags for current exchange and
             * whether we are initiator or responder.
             */
            status = IKE2_Construct_Header(handle, IKE2_CREATE_CHILD_SA,
                        ((handle->ike2_flags & IKE2_INITIATOR) != 0) ?
                        IKE2_HDR_INITIATOR_FLAG : 0);

            if(status == NU_SUCCESS)
            {
                /* We will need to encrypt the packet before
                 * sending. Set the key to appropriate value.
                 */
                if((handle->ike2_flags & IKE2_INITIATOR) == 0)
                {
                    /* We are the responder. */
                    handle->ike2_sa->ike_encryption_key =
                        handle->ike2_sa->ike2_sk_er;
                }

                else
                {
                    /* We are the initiator. */
                    handle->ike2_sa->ike_encryption_key =
                        handle->ike2_sa->ike2_sk_ei;
                }

                /* All the payloads have been constructed successfully,
                 * now send the packet to the peer.
                 */
                status = IKE2_Send_Packet(handle);

                if(status == NU_SUCCESS)
                {
                    handle->ike2_state += IKE2_NEXT_STATE_INC;

                    /* Packet was sent successfully!
                     * Set the message reply timeout timer.
                     */
                    status = IKE_Set_Timer(IKE2_Message_Reply_Event,
                                           (UNSIGNED)handle->ike2_sa,
                                           (UNSIGNED)handle,
                                           IKE2_MESSAGE_REPLY_TIMEOUT);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Message retransmission timer could not be added",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to send packet", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to construct IKEv2 header",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_CREATE_CHILD_SA_I_Response
*
* DESCRIPTION
*
*       This function is called to receive and process the response sent
*       by the peer to our CREATE_CHILD_SA request. It also takes
*       appropriate actions i.e. adding newly negotiated SA's and replacing
*       and deleting old ones.
*
*       *Initiator                                  Responder
*       -----------                                 -----------
*                               <--  HDR, SK {SA, Nr, [KEr],[TSi, TSr]}
*
* INPUTS
*
*       *handle                 Exchange information for this exchange
*
* OUTPUTS
*
*       NU_SUCCESS              On successful execution.
*       IKE2_INVALID_PARAMS     Parameters provided are not valid.
*
*************************************************************************/
STATIC STATUS IKE2_CREATE_CHILD_SA_I_Response(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_ATTRIB             *attribs = NU_NULL;

    IKE2_MESSAGE            *in_msg;

    IKE2_KE_PAYLOAD         ke_r;
    IKE2_NONCE_PAYLOAD      nonce;
    IKE2_SA_PAYLOAD         sa2;
    IKE2_TS_PAYLOAD         ts_i;
    IKE2_TS_PAYLOAD         ts_r;
    IKE2_TS                 ts_data_i;
    IKE2_TS                 ts_data_r;
    IKE2_VENDOR_ID_PAYLOAD  vid_r;

    IPSEC_SECURITY_PROTOCOL *ips_security = NU_NULL;
    UINT8                   security_size;
    UINT8                   revrt_flags = 0;

    IKE_SA2                 *ike2_sa2;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    in_msg  = &handle->ike2_params->ike2_in;

    ts_i.ike2_ts = &ts_data_i;
    ts_r.ike2_ts = &ts_data_r;

    UTL_Zero(&nonce, sizeof(IKE2_NONCE_PAYLOAD));
    UTL_Zero(&sa2, sizeof(IKE2_SA_PAYLOAD));

    /* The following members are dynamically allocated so initialize
     * them to a known value here and we will need to deallocate them
     * when we're done processing.
     */
    ke_r.ike2_ke_data = NU_NULL;
    nonce.ike2_nonce_data = NU_NULL;

    /* Set incoming payloads to receive. */
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_sa, &sa2);
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_nonce, &nonce);

    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ke, &ke_r);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ts_i, &ts_i);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ts_r, &ts_r);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_vid, &vid_r);

    status = IKE2_CREATE_CHILD_SA_Receive(handle);

    if(status == NU_SUCCESS)
    {
        /* We have received reply for our SA_INIT request. First unset
         * any message reply timeout events we have set for this request.
         */
        status = IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                 (UNSIGNED)handle->ike2_sa,
                                 (UNSIGNED)handle);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to unset message reply timeout event",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        if(((handle->ike2_flags & IKE2_SA_REKEY) != 0) &&
            IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_ts_i) != NU_TRUE)
        {
            /* This is an SA being re-keyed. */

            if(status == NU_SUCCESS)
            {
                if((handle->ike2_flags & IKE2_RESPONDER) != 0)
                {
                    /* This is an IKE SA re-keying exchange. */
                    handle->ike2_flags &= ~IKE2_RESPONDER;

                    status = IKE2_Select_Proposal(handle, &attribs);

                    handle->ike2_flags |= IKE2_RESPONDER;
                }

                else
                {
                    status = IKE2_Select_Proposal(handle, &attribs);
                }

                if(status == NU_SUCCESS)
                {
                    /* Proposal has been selected. Now convert attributes
                     * to SA values.
                     */
                    status = IKE2_Extract_IKE_SA_Attribs(attribs,
                                handle->ike2_new_sa);

                    if(status == NU_SUCCESS)
                    {
                        /* Allocate buffer for DH public values. */
                        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&handle->ike2_remote_dh,
                                    in_msg->ike2_ke->ike2_ke_data_len,
                                    NU_NO_SUSPEND);

                        if(status == NU_SUCCESS)
                        {
                            handle->ike2_remote_dh = TLS_Normalize_Ptr(
                                handle->ike2_remote_dh);

                            if(in_msg->ike2_ke->ike2_ke_data != NU_NULL)
                            {
                                NU_BLOCK_COPY(handle->ike2_remote_dh,
                                    in_msg->ike2_ke->ike2_ke_data,
                                    in_msg->ike2_ke->ike2_ke_data_len);
                            }

                            if(in_msg->ike2_nonce->ike2_nonce_data
                                    != NU_NULL)
                            {
                                NU_BLOCK_COPY(handle->ike2_peer_nonce,
                                    in_msg->ike2_nonce->ike2_nonce_data,
                                    in_msg->ike2_nonce->ike2_nonce_data_len);
                            }

                            status = IKE2_Generate_KEYMAT(handle);

                            if(status == NU_SUCCESS)
                            {
                                handle->ike2_flags &= ~IKE2_SA_REKEY;

                                status = IKE2_Send_Info_Delete(
                                            handle, IKE2_PROTO_ID_IKE,
                                            0, 0, NU_NULL);

                                if(status != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Failed to send informational",
                                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                                }

                                else
                                {
                                    /* Informational sent, now wait for
                                     * reply and then delete the old SA.
                                     */
                                    handle->ike2_sa->ike2_flags |=
                                        IKE2_WAIT_DELETE;
                                }
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Could not convert attributes",
                                    NERR_RECOVERABLE, __FILE__,
                                    __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to allocate memory for DH keys",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate keying material for rekeyed SA",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to decode the attributes",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for SA",
                    NERR_RECOVERABLE, __FILE__, __LINE__);

            }

        }

        else
        {
            if(IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_ke) == NU_TRUE)
            {
                /* We have received a KE payload. This is an IPsec
                 * SA re-keying request.
                 */
                if((handle->ike2_remote_dh == NU_NULL) ||
                    (handle->ike2_remote_dh_len !=
                    in_msg->ike2_ke->ike2_ke_data_len))
                {
                    if((handle->ike2_remote_dh != NU_NULL) &&
                        (NU_Deallocate_Memory(handle->ike2_remote_dh)
                        != NU_SUCCESS))
                    {
                        NLOG_Error_Log("Failed to deallocate memory for \
                                       old DH value", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }

                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                (VOID**)&handle->ike2_remote_dh,
                                in_msg->ike2_ke->ike2_ke_data_len,
                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        handle->ike2_remote_dh = TLS_Normalize_Ptr(
                            handle->ike2_remote_dh);

                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for \
                                       new DH shared value", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }

                }

                /* Copy DH public values. */

                if(status == NU_SUCCESS)
                {
                    if(in_msg->ike2_ke->ike2_ke_data != NU_NULL)
                    {
                        NU_BLOCK_COPY(handle->ike2_remote_dh,
                                      in_msg->ike2_ke->ike2_ke_data,
                                      in_msg->ike2_ke->ike2_ke_data_len);
                    }
                }
            }

            status = IKE2_Extract_TS_Info(handle, 0, 0);

            if(status == NU_SUCCESS)
            {
                status = IPSEC_Get_Policy_Index_Narrow(handle->ike2_ips_group,
                                                &handle->ike2_ips_selector,
                                                IPSEC_OUTBOUND,
                                                &(handle->ike2_ips_policy_index),
                                                NU_NULL);

                if (status == NU_SUCCESS)
                {
                    status = IKE2_Get_IPsec_Policy_Security(handle,
                                &ips_security, &security_size);

                    if(status == NU_SUCCESS)
                    {
                        /* If we were not the original initiaor but now we are
                         * the initiator of CHILD_SA exchange so we need to
                         * switch the flags */
                        if(handle->ike2_flags & IKE2_RESPONDER)
                        {
                            revrt_flags = 1;
                            handle->ike2_flags &= ~IKE2_RESPONDER;
                            handle->ike2_flags |= IKE2_INITIATOR;
                        }
                        status = IKE2_Select_IPsec_Proposal(handle,
                                    ips_security,security_size);

                        if(status == NU_SUCCESS)
                        {
                            status = IKE2_Generate_CHILD_SA_KEYMAT(handle);

                            if(status == NU_SUCCESS)
                            {
                                status = IKE2_IPS_Generate_SA_Pair(handle);

                                if(status == NU_SUCCESS)
                                {
                                    /* When we are initiator, we allocate memory for
                                     * child SA keying material to ike_remote_keymat
                                     * pointer. However, the flush SA call deletes
                                     * through ike_local_keymat pointer which in this
                                     * case will be pointing elsewhere. Set this pointer
                                     * back to where memory was allocated so that it
                                     * can be deleted.
                                     */
                                    if((handle->ike2_flags & IKE2_RESPONDER) == 0 &&
                                        (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
                                        IKE2_HDR_RESPONSE_FLAG) != 0)
                                    {
                                        ike2_sa2 = handle->ike2_sa2_db.ike_flink;

                                        while(ike2_sa2 != NU_NULL)
                                        {
                                            ike2_sa2->ike_local_keymat =
                                                ike2_sa2->ike_remote_keymat;

                                            ike2_sa2 = ike2_sa2->ike_flink;
                                        }
                                    }

                                    /* Purge SA2 database. */
                                    IKE_Flush_SA2(&handle->ike2_sa2_db);

                                    handle->ike2_state = IKE2_COMPLETE_STATE;
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                        "Failed to add SA pair",
                                        NERR_RECOVERABLE, __FILE__,
                                        __LINE__);

                                }
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to generate keying material for Child SA",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("No suitable proposal selected",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                        /* If flags need to be reverted back */
                        if(revrt_flags)
                        {
                            handle->ike2_flags &= ~IKE2_INITIATOR;
                            handle->ike2_flags |= IKE2_RESPONDER;
                        }

                        /* Deallocate the 'security' memory blcok as it
                         * is dynamically allocated. */
                        if(ips_security != NU_NULL)
                        {
                            if(NU_Deallocate_Memory(ips_security)
                                != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to deallocate Security memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                            }
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to get Policy security parameters",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("No matching IPsec policy found",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            }

            else
            {
                NLOG_Error_Log("Failed to parse TS payloads",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to receive incoming CREATE CHILD request",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Deallocate the dynamically allocated payload data if it
     * has been allocated during the processing above.
     */
    if(ke_r.ike2_ke_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(ke_r.ike2_ke_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate KE payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        ke_r.ike2_ke_data = NU_NULL;
    }
    if(nonce.ike2_nonce_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(nonce.ike2_nonce_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Nonce payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        nonce.ike2_nonce_data = NU_NULL;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_CREATE_CHILD_SA_R
*
* DESCRIPTION
*
*       Initiator                                 *Responder
*       -----------                               -----------
*       HDR, SK {[N], SA, Ni,
*               [KEi],[TSi, TSr]} -->
*                                 <--    HDR, SK {SA, Nr, [KEr],[TSi, TSr]}
*
* INPUTS
*
*       *handle                 Exchange information for current exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful execution.
*       IKE2_INVALID_PARAMS     Parameters provided are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_CREATE_CHILD_SA_R(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_TRUE;
    IKE2_ATTRIB             *attribs = NU_NULL;

    IKE2_MESSAGE            *out_msg;
    IKE2_MESSAGE            *in_msg;

    IKE2_NONCE_PAYLOAD      nonce;
    IKE2_SA_PAYLOAD         sa2;
    IKE2_TS_PAYLOAD         ts_i;
    IKE2_TS_PAYLOAD         ts_r;
    IKE2_TS                 ts_data_i[IKE2_MAX_SELECTORS_IN_TS];
    IKE2_TS                 ts_data_r[IKE2_MAX_SELECTORS_IN_TS];
    IKE2_KE_PAYLOAD         ke_r;
    IKE2_KE_PAYLOAD         ke_i;
    IKE2_VENDOR_ID_PAYLOAD  vid_i;
    IKE2_HDR                hdr_r;
    INT                     narrow_proto;
    UINT8                   tsi_selec_index;
    UINT8                   tsr_selec_index;

    IPSEC_SECURITY_PROTOCOL *ips_security = NU_NULL;
    UINT8                   security_size;
    UINT8                   revrt_flags = 0;

    /* Payloads that came from peer. */
    IKE2_SA_PAYLOAD         peer_sa;
    IKE2_NONCE_PAYLOAD      peer_nonce;

    IKE2_TS_PAYLOAD         *ts_i_peer;
    IKE2_TS_PAYLOAD         *ts_r_peer;

    IKE_SA2                 *ike2_sa2;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    out_msg = &handle->ike2_params->ike2_out;
    in_msg  = &handle->ike2_params->ike2_in;

    ts_i.ike2_ts = &(ts_data_i[0]);
    ts_r.ike2_ts = &(ts_data_r[0]);

    UTL_Zero(&nonce, sizeof(IKE2_NONCE_PAYLOAD));
    UTL_Zero(&sa2, sizeof(IKE2_SA_PAYLOAD));
    UTL_Zero(&peer_sa, sizeof(IKE2_SA_PAYLOAD));

    /* Initialize hdr_r to remove benign KW warning. */
    memset(&hdr_r, 0, sizeof(IKE2_HDR));
    IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr_r);

    IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr, IKE2_NONE_PAYLOAD_ID);

    IKE2_SET_OUTBOUND(out_msg->ike2_nonce, &nonce);
    IKE2_SET_OUTBOUND(out_msg->ike2_sa, &sa2);
    IKE2_SET_OUTBOUND(out_msg->ike2_ts_i, &ts_i);
    IKE2_SET_OUTBOUND(out_msg->ike2_ts_r, &ts_r);

    ts_i_peer = handle->ike2_params->ike2_out.ike2_ts_i;
    ts_r_peer = handle->ike2_params->ike2_out.ike2_ts_r;

    /* The following members are dynamically allocated so initialize
     * them to a known value here and we will need to deallocate them
     * when we're done processing.
     */
    ke_r.ike2_ke_data = NU_NULL;
    peer_nonce.ike2_nonce_data = NU_NULL;

    /* Set incoming payloads to receive. */
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_sa, &peer_sa);
    IKE2_SET_INBOUND_REQUIRED(in_msg->ike2_nonce, &peer_nonce);

    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ke, &ke_r);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ts_i, ts_i_peer);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_ts_r, ts_r_peer);
    IKE2_SET_INBOUND_OPTIONAL(in_msg->ike2_vid, &vid_i);

    status = IKE2_CREATE_CHILD_SA_Receive(handle);

    if(status == NU_SUCCESS)
    {
        if(((handle->ike2_flags & IKE2_SA_REKEY) != 0) &&
            IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_ts_i) != NU_TRUE)
        {
            /* This is an SA being re-keyed. */

            /* If there is no SA for this exchange, allocate a new one. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID **)&handle->ike2_new_sa,
                                        sizeof(IKE2_SA), NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                UTL_Zero(handle->ike2_new_sa, sizeof(IKE2_SA));

                if((handle->ike2_flags & IKE2_RESPONDER) != 0)
                {
                    status = IKE2_Select_Proposal(handle, &attribs);
                }

                else
                {
                    /* Mark flag that the roles of Initiator and
                     * Responder have been switched in this child SA
                     * exchange.
                     */
                    handle->ike2_new_sa->ike2_switch_role = NU_TRUE;

                    /* This is an IKE SA re-keying exchange. */
                    handle->ike2_flags |= IKE2_RESPONDER;

                    status = IKE2_Select_Proposal(handle, &attribs);

                    handle->ike2_flags &= ~(IKE2_RESPONDER);
                }

                if(status == NU_SUCCESS)
                {
                    /* Proposal has been selected. Now convert attributes to
                     * SA values.
                     */
                    status = IKE2_Extract_IKE_SA_Attribs(attribs,
                                                         handle->ike2_new_sa);

                    if(status == NU_SUCCESS)
                    {
                        /* Allocate buffer for DH public values. */
                        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                            (VOID**)&handle->ike2_remote_dh,
                            in_msg->ike2_ke->ike2_ke_data_len, NU_NO_SUSPEND);

                        if(status == NU_SUCCESS)
                        {
                            handle->ike2_remote_dh = TLS_Normalize_Ptr(
                                handle->ike2_remote_dh);

                            if(in_msg->ike2_ke->ike2_ke_data != NU_NULL)
                            {
                                NU_BLOCK_COPY(handle->ike2_remote_dh,
                                    in_msg->ike2_ke->ike2_ke_data,
                                    in_msg->ike2_ke->ike2_ke_data_len);
                            }

                            if(in_msg->ike2_nonce->ike2_nonce_data != NU_NULL)
                            {
                                NU_BLOCK_COPY(handle->ike2_peer_nonce,
                                    in_msg->ike2_nonce->ike2_nonce_data,
                                    in_msg->ike2_nonce->ike2_nonce_data_len);
                            }

                            status = IKE2_Generate_Nonce_Data(handle, &nonce);

                            if(status == NU_SUCCESS)
                            {
                                status = IKE2_Generate_KE_Data(handle, &ke_i);

                                if(status == NU_SUCCESS)
                                {
                                    status = IKE2_Generate_KEYMAT(handle);

                                    if(status != NU_SUCCESS)
                                    {
                                        NLOG_Error_Log("Could not convert attributes",
                                            NERR_RECOVERABLE, __FILE__, __LINE__);
                                    }
                                }

                                else
                                {
                                    NLOG_Error_Log("Failed to generate KE data",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log("Failed to generate nonce data",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to allocate memory for DH keys",
                                           NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate keying material for rekeyed SA",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to decode the attributes",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for SA",
                    NERR_RECOVERABLE, __FILE__, __LINE__);

            }

        }

        else
        {
            status = IKE2_Generate_Nonce_Data(handle, &nonce);

            if(status == NU_SUCCESS)
            {
                if(IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_ke) == NU_TRUE)
                {
                    /* We have received a KE payload. This is an IPsec
                     * SA re-keying request. Copy new keying material to
                     * the handle and generate new keying material for
                     * outgoing message.
                     */
                    IKE2_SET_OUTBOUND(out_msg->ike2_ke, &ke_r);

                    if((handle->ike2_remote_dh == NU_NULL) ||
                        (handle->ike2_remote_dh_len !=
                        in_msg->ike2_ke->ike2_ke_data_len))
                    {
                        if((handle->ike2_remote_dh != NU_NULL) &&
                            (NU_Deallocate_Memory(handle->ike2_remote_dh)
                            != NU_SUCCESS))
                        {
                            NLOG_Error_Log("Failed to deallocate memory for \
                                            old DH value", NERR_RECOVERABLE,
                                            __FILE__, __LINE__);
                        }

                        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&handle->ike2_remote_dh,
                                    in_msg->ike2_ke->ike2_ke_data_len,
                                    NU_NO_SUSPEND);

                        if(status == NU_SUCCESS)
                        {
                            handle->ike2_remote_dh = TLS_Normalize_Ptr(
                                handle->ike2_remote_dh);

                        }

                        else
                        {
                            NLOG_Error_Log("Failed to allocate memory for \
                                           new DH shared value", NERR_RECOVERABLE,
                                           __FILE__, __LINE__);
                        }

                    }
                    /* Allocate buffer for DH public values. */

                    if(status == NU_SUCCESS)
                    {
                        if(in_msg->ike2_ke->ike2_ke_data != NU_NULL)
                        {
                            NU_BLOCK_COPY(handle->ike2_remote_dh,
                                in_msg->ike2_ke->ike2_ke_data,
                                in_msg->ike2_ke->ike2_ke_data_len);
                        }
                    }
                }
                /* Create a selector out of proposed traffic selectors
                 * that match any of our IPSec policies.
                 */
                for(tsi_selec_index = 0;
                            (tsi_selec_index < in_msg->ike2_ts_i->
                            ike2_ts_count) && (status != NU_SUCCESS);
                            tsi_selec_index++)
                {
                    for(tsr_selec_index = 0;
                            (tsr_selec_index < in_msg->ike2_ts_r->
                            ike2_ts_count) && (status != NU_SUCCESS);
                            tsr_selec_index++)
                    {

                        /* Nucleus IPSec only support same protocol,
                         * for incoming and outgoing traffic, per policy.
                         */
                        if((in_msg->ike2_ts_i->ike2_ts[tsi_selec_index].
                            ike2_ip_protocol_id == in_msg->ike2_ts_r->
                            ike2_ts[tsr_selec_index].ike2_ip_protocol_id))
                        {
                            /* Extract TS sent by peer. */
                            IKE2_Extract_TS_Info(handle, tsi_selec_index, tsr_selec_index);
                        }
                        else
                            continue;

                        /* Get the IPsec Policy Index matching TS sent by peer. */
                        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
                        {
                            /* If we are the initiator, we'll be sending out packet so we
                             * need to lookup outbound policy.
                             */
                            status  = IPSEC_Get_Policy_Index_Narrow(handle->ike2_ips_group,
                                                             &handle->ike2_ips_selector,
                                                             IPSEC_OUTBOUND | IPSEC_APPLY,
                                                             &(handle->ike2_ips_policy_index),
                                                             NU_NULL);
                        }

                        else
                        {
                            /* If we are the responder, we'll be sending out packet so we
                             * need to lookup inbound policy.
                             */
                            status  = IPSEC_Get_Policy_Index_Narrow(handle->ike2_ips_group,
                                                             &handle->ike2_ips_selector,
                                                             IPSEC_INBOUND | IPSEC_APPLY,
                                                             &(handle->ike2_ips_policy_index),
                                                             &narrow_proto);

                            if((status == NU_SUCCESS) && (narrow_proto != -1))
                            {
                                /* Update the narrowed protocol in the outgoing Traffic
                                 * Selectors accordingly.
                                 */
                                if((handle->ike2_params->ike2_out.ike2_ts_i != NU_NULL) &&
                                 (handle->ike2_params->ike2_out.ike2_ts_i->ike2_ts != NU_NULL))
                                {
                                    handle->ike2_params->ike2_out.ike2_ts_i->ike2_ts[tsi_selec_index].
                                        ike2_ip_protocol_id = (UINT8)narrow_proto;
                                }

                                if((handle->ike2_params->ike2_out.ike2_ts_r != NU_NULL) &&
                                 (handle->ike2_params->ike2_out.ike2_ts_r->ike2_ts != NU_NULL))
                                {
                                    handle->ike2_params->ike2_out.ike2_ts_r->ike2_ts[tsr_selec_index].
                                        ike2_ip_protocol_id = (UINT8)narrow_proto;
                                }
                            }
    #if IKE2_MAX_SELECTORS_IN_TS > 1
                            /* Move the chosen traffic sectors on top as the
                             * same TS would be sent back. */
                            if(status == NU_SUCCESS)
                            {
                                if(tsi_selec_index > 0)
                                {
                                    NU_BLOCK_COPY(&handle->ike2_params->ike2_in.ike2_ts_i->ike2_ts[0],
                                            &handle->ike2_params->ike2_in.ike2_ts_i->ike2_ts[tsi_selec_index],
                                            sizeof(IKE2_TS));
                                }
                                if(tsr_selec_index > 0)
                                {
                                    NU_BLOCK_COPY(&handle->ike2_params->ike2_in.ike2_ts_r->ike2_ts[0],
                                            &handle->ike2_params->ike2_in.ike2_ts_r->ike2_ts[tsr_selec_index],
                                            sizeof(IKE2_TS));
                                }
                            }
    #endif
                        }
                    }/* for tsr_selec_index */
                }/* for tsi_selec_index */

                if(status == NU_SUCCESS)
                {

                    if (status == NU_SUCCESS)
                    {
                        status = IKE2_Get_IPsec_Policy_Security(
                                    handle, &ips_security, &security_size);

                        if(status == NU_SUCCESS)
                        {
                            /* If we were originaly initiaor, but now we are
                             * the responder of CHILD_SA exchange so we need to
                             * switch the flags */
                            if(handle->ike2_flags & IKE2_INITIATOR)
                            {
                                revrt_flags = 1;
                                handle->ike2_flags &= ~IKE2_INITIATOR;
                                handle->ike2_flags |= IKE2_RESPONDER;
                            }

                            status = IKE2_Select_IPsec_Proposal(
                                        handle, ips_security,security_size);

                            if(status == NU_SUCCESS)
                            {
                                status = IKE2_Generate_CHILD_SA_KEYMAT(handle);

                                if(status == NU_SUCCESS)
                                {
                                    status = IKE2_IPS_Generate_SA_Pair(handle);

                                    if(status == NU_SUCCESS)
                                    {
                                        status =
                                            IKE2_Create_IPsec_SA_Payloads(handle);

                                        if(status == NU_SUCCESS)
                                        {
                                            /* When we are initiator, we allocate memory for
                                             * child SA keying material to ike_remote_keymat
                                             * pointer. However, the flush SA call deletes
                                             * through ike_local_keymat pointer which in this
                                             * case will be pointing elsewhere. Set this pointer
                                             * back to where memory was allocated so that it
                                             * can be deleted.
                                             */
                                            if((handle->ike2_flags & IKE2_RESPONDER) == 0 &&
                                                (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
                                                 IKE2_HDR_RESPONSE_FLAG) != 0)
                                            {
                                                ike2_sa2 = handle->ike2_sa2_db.ike_flink;

                                                while(ike2_sa2 != NU_NULL)
                                                {
                                                    ike2_sa2->ike_local_keymat =
                                                        ike2_sa2->ike_remote_keymat;

                                                    ike2_sa2 = ike2_sa2->ike_flink;
                                                }
                                            }
                                            /* Purge SA2 database. */
                                            IKE_Flush_SA2(&handle->ike2_sa2_db);
                                        }

                                        else
                                        {
                                            NLOG_Error_Log(
                                                "Failed to create IPsec payloads",
                                                NERR_RECOVERABLE, __FILE__, __LINE__);
                                        }
                                    }

                                    else
                                    {
                                        NLOG_Error_Log(
                                            "Failed to add SA pair",
                                            NERR_RECOVERABLE, __FILE__,
                                            __LINE__);

                                    }
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                        "Failed to generate keying material for Child SA",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log("No suitable proposal selected",
                                               NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                            /* If flags need to be reverted back */
                            if(revrt_flags)
                            {
                                handle->ike2_flags &= ~IKE2_RESPONDER;
                                handle->ike2_flags |= IKE2_INITIATOR;
                            }

                            /* Deallocate the 'security' memory blcok as
                             * it is dynamically allocated. */
                            if(ips_security != NU_NULL)
                            {
                                if(NU_Deallocate_Memory(ips_security)
                                    != NU_SUCCESS)
                                {
                                    NLOG_Error_Log(
                                        "Failed to deallocate Security",
                                        NERR_SEVERE, __FILE__, __LINE__);
                                }
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to get Policy security parameters",
                                           NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("No matching IPsec policy found",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }

                }

                else
                {
                    NLOG_Error_Log("Failed to parse TS payloads",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            }

            else
            {
                NLOG_Error_Log("Failed to generate nonce data",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to receive incoming CREATE CHILD request",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(status == NU_SUCCESS)
    {
        IKE2_END_CHAIN(out_msg->ike2_last);

        status = IKE2_Construct_Header(handle, IKE2_CREATE_CHILD_SA,
                                       ((handle->ike2_flags & IKE2_INITIATOR)
                                       == 0)?IKE2_HDR_RESPONSE_FLAG:
                                       (IKE2_HDR_INITIATOR_FLAG |
                                       IKE2_HDR_RESPONSE_FLAG));
        if(status == NU_SUCCESS)
        {
            if((hdr_r.ike2_flags & (IKE2_HDR_INITIATOR_FLAG |
                IKE2_HDR_RESPONSE_FLAG)) != 0)
            {
                /* We are initiator but we are sending a response to a
                 * request from responder. Use the same msg ID as peer
                 * did.
                 */
                hdr_r.ike2_msg_id = in_msg->ike2_hdr->ike2_msg_id;
            }

            /* We will need to encrypt the packet before
             * sending. Set the key to appropriate value.
             */
            if((handle->ike2_flags & IKE2_INITIATOR) == 0)
            {
                /* We are the responder. */
                handle->ike2_sa->ike_encryption_key =
                    handle->ike2_sa->ike2_sk_er;
            }

            else
            {
                /* We are the initiator. */
                handle->ike2_sa->ike_encryption_key =
                    handle->ike2_sa->ike2_sk_ei;
            }

            /* Send the message to the peer. */
            status = IKE2_Send_Packet(handle);

            if(status == NU_SUCCESS)
            {
                /* Message has been sent, now update the state
                 * of this exchange for further messages on this
                 * exchange.
                 */
                handle->ike2_state += IKE2_NEXT_STATE_INC;

                if((handle->ike2_flags & IKE2_SA_REKEY) != 0)
                {
                    handle->ike2_flags &= ~IKE2_SA_REKEY;
                }
            }

            else
            {
                NLOG_Error_Log("Failed to send packet!",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to construct IKE header",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Deallocate the dynamically allocated payload data if it
     * has been allocated during the processing above.
     */
    if(ke_r.ike2_ke_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(ke_r.ike2_ke_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate KE payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        ke_r.ike2_ke_data = NU_NULL;
    }
    if(peer_nonce.ike2_nonce_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(peer_nonce.ike2_nonce_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Nonce payload data",
                       NERR_SEVERE, __FILE__, __LINE__);
        }

        peer_nonce.ike2_nonce_data = NU_NULL;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_CREATE_CHILD_SA_Receive
*
* DESCRIPTION
*
*       This function is responsible for receiving incoming CREATE_CHILD_SA
*       messages. It is called as both initiator and responder. It is not
*       a state machine handler, it is only a helper function.
*
* INPUTS
*
*       *handle                 Exchange information for this exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are not correct.
*
*************************************************************************/
STATIC STATUS IKE2_CREATE_CHILD_SA_Receive(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_MESSAGE        *in_msg;
    IKE2_STATE_PARAMS   *params;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Set local pointers. */
    params = handle->ike2_params;
    in_msg = &params->ike2_in;

    if(in_msg->ike2_hdr->ike2_next_payload != IKE2_ENCRYPT_PAYLOAD_ID)
    {
        status = IKE2_UNEXPECTED_PAYLOAD;
        NLOG_Error_Log("Encrypted packet expected but not found.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* Set which key to use to decrypt the packet. */
        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            /* We are the initiator. Use responder key to decrypt. */
            handle->ike2_sa->ike_encryption_key =
                handle->ike2_sa->ike2_sk_er;
        }

        else
        {
            /* We are the responder. Use initiator key to decrypt. */
            handle->ike2_sa->ike_encryption_key =
                handle->ike2_sa->ike2_sk_ei;
        }

        /* We have encountered encrypted payload. Decrypt the message. */
        status = IKE2_Decode_IKE_Encrypted_Payload(
                                        params->ike2_packet->ike_data,
                                        params->ike2_packet->ike_data_len,
                                        handle->ike2_sa);

        if(status == NU_SUCCESS)
        {
            in_msg->ike2_hdr->ike2_length =
                GET32(params->ike2_packet->ike_data, IKE2_HDR_LEN_OFST);

            in_msg->ike2_hdr->ike2_next_payload =
                GET8(params->ike2_packet->ike_data, IKE2_HDR_NXT_PYLD_OFST);


            /* Decode the incoming message. Extract all payloads. */
            status = IKE2_Decode_Message(params->ike2_packet->ike_data, in_msg);

            if(status == NU_SUCCESS)
            {
                /* CREATE_CHILD_SA exchange can create new child SA or
                 * re-key an existing IKE SA or re-key an existing IPsec
                 * SA. TS payloads are omitted only in case of IKE SA
                 * re-keying. If we did not get TS payloads, set the
                 * IKE2_SA_REKEY flag.
                 */
                if(IKE2_PAYLOAD_IS_PRESENT(in_msg->ike2_ts_i) != NU_TRUE)
                {
                    handle->ike2_flags |= IKE2_SA_REKEY;
                }

                else
                {
                    /* We received TS payloads so this is either new IPsec
                     * SA exchange or we are re-keying an existing IPsec
                     * SA. In later case, we must also have got a notify
                     * payload and the re-keying flag is set when we
                     * process the notify payload.
                     */
                    handle->ike2_flags &= ~IKE2_SA_REKEY;
                }

                /* Check if any information payloads have been received. */
                if((status == NU_SUCCESS) && (in_msg->ike2_notify != NU_NULL))
                {
                    status = IKE2_Process_Notify(handle);

                    if(status != NU_NULL)
                    {
                        IKE_Unset_Timer(IKE2_Message_Reply_Event,
                                        (UNSIGNED)handle->ike2_sa,
                                        (UNSIGNED)handle);

                        NLOG_Error_Log("Failed to process Notify payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                /* Incoming message has been decoded. Copy out the data for
                 * incoming payloads.
                 */

                if(status == NU_SUCCESS)
                {
                    /* Allocate memory for incoming nonce payload data. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                        (VOID**)&handle->ike2_peer_nonce,
                        in_msg->ike2_nonce->ike2_nonce_data_len, NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        handle->ike2_peer_nonce_len =
                            (UINT8)in_msg->ike2_nonce->ike2_nonce_data_len;

                        NU_BLOCK_COPY(handle->ike2_peer_nonce,
                            in_msg->ike2_nonce->ike2_nonce_data,
                            handle->ike2_peer_nonce_len);

                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for nonce",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Failed to decode incoming packet",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

        }

        else
        {
            NLOG_Error_Log("Failed to decrypt incoming message.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Construct_Header
*
* DESCRIPTION
*
*       This function constructs a new header for outgoing IKEv2 message.
*
* INPUTS
*
*       *handle                 Exchange information for this exchange
*       xchg                    Type of exchange
*       flags                   Additional information for exchange e.g. if
*                               are responder or initiator.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Construct_Header(IKE2_EXCHANGE_HANDLE *handle, UINT8 xchg,
                             UINT8 flags)
{
    STATUS      status = NU_SUCCESS;
    IKE2_HDR    *hdr;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    hdr = handle->ike2_params->ike2_out.ike2_hdr;
    hdr->ike2_exchange_type = xchg;
    hdr->ike2_major_version = IKE2_MAJOR_VERSION;
    hdr->ike2_minor_version = IKE2_MINOR_VERSION;
    hdr->ike2_flags = flags;

    /* See if we are initiator or responder. This decides which SPI goes
     * where.
     */
    if((flags & IKE2_HDR_INITIATOR_FLAG) != 0)
    {
        /* We are initiator. */
        NU_BLOCK_COPY(hdr->ike2_sa_spi_i, handle->ike2_sa->ike2_local_spi,
            IKE2_SPI_LENGTH);
        NU_BLOCK_COPY(hdr->ike2_sa_spi_r, handle->ike2_sa->ike2_remote_spi,
            IKE2_SPI_LENGTH);
    }
    else
    {
        /* We are responder. */
        NU_BLOCK_COPY(hdr->ike2_sa_spi_r, handle->ike2_sa->ike2_local_spi,
            IKE2_SPI_LENGTH);
        NU_BLOCK_COPY(hdr->ike2_sa_spi_i, handle->ike2_sa->ike2_remote_spi,
            IKE2_SPI_LENGTH);

        /* When we are responder, we just need to send back the ID
         * received in request packet.
         */
    }

    /* Echo the initiator's message ID if we are the responder, or if we are the
     * initiator, and the responder has requested us to retransmit with a cookie.
     */
    if(((flags & IKE2_HDR_RESPONSE_FLAG) != 0) || (handle->ike2_flags & IKE2_USE_INIT_COOKIE))
    {
        /* When we are responder, we just need to send back the ID
         * received in request packet.
         */
        hdr->ike2_msg_id = handle->ike2_next_id_peer;
    }
    else
    {
        hdr->ike2_msg_id = handle->ike2_next_id_nu;

        /* When we are initiator, we have to increment the message ID
         * with each message.
         */
        handle->ike2_next_id_nu = handle->ike2_next_id_nu + 1;

    }
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Process_IPsec_SA_Payloads
*
* DESCRIPTION
*
*       SA2, TSi, TSr,
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful execution.
*       NU_INVALID_PARAMS       Parameters provided are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_Process_IPsec_SA_Payloads(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_SUCCESS;

    IPSEC_SECURITY_PROTOCOL *ips_security = NU_NULL;
    UINT8                   security_size;
    IKE_SA2                 *ike2_sa2;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    status = IKE2_Get_IPsec_Policy_Security(handle, &ips_security,
                                            &security_size);

    if(status == NU_SUCCESS)
    {
        status = IKE2_Select_IPsec_Proposal(handle, ips_security,
                                            security_size);

        if(status == NU_SUCCESS)
        {
            status = IKE2_Generate_CHILD_SA_KEYMAT(handle);

            if(status == NU_SUCCESS)
            {
                status = IKE2_IPS_Generate_SA_Pair(handle);

                if(status == NU_SUCCESS)
                {
                    /* When we are initiator, we allocate memory for
                     * child SA keying material to ike_remote_keymat
                     * pointer. However, the flush SA call deletes
                     * through ike_local_keymat pointer which in this
                     * case will be pointing elsewhere. Set this pointer
                     * back to where memory was allocated so that it
                     * can be deleted.
                     */
                    if((handle->ike2_flags & IKE2_INITIATOR) != 0)
                    {
                        ike2_sa2 = handle->ike2_sa2_db.ike_flink;

                        while(ike2_sa2 != NU_NULL)
                        {
                            ike2_sa2->ike_local_keymat =
                                ike2_sa2->ike_remote_keymat;

                            ike2_sa2 = ike2_sa2->ike_flink;
                        }
                    }

                    /* Purge SA2 database. */
                    IKE_Flush_SA2(&handle->ike2_sa2_db);
                }

                else
                {
                    NLOG_Error_Log("Failed to add SA pair",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log(
                    "Failed to generate keying material for Child SA",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("No suitable proposal selected",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }


        /* Deallocate the 'security' memory blcok as it is dynamically
         * allocated.
         */
        if(ips_security != NU_NULL)
        {
            if(NU_Deallocate_Memory(ips_security) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate Security memory",
                    NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get Policy security parameters",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }



    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Create_IPsec_SA_Payloads
*
* DESCRIPTION
*
*       Generates payloads needed to create an IPsec SA and adds them
*       to the outgoing message.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_Create_IPsec_SA_Payloads(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_SUCCESS;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* If we are responder, we already have added the selected proposal
     * to the outgoing message. If, however, we are initiator, we need
     * to send all possible proposals that we can accept for ourselves.
     */
    if((handle->ike2_flags & IKE2_INITIATOR) != 0)
    {
        status = IKE2_Construct_IPsec_Proposal(handle);

        if(status == NU_SUCCESS)
        {
            status = IKE2_Generate_TS(handle);
        }

        else
        {
            NLOG_Error_Log("Failed to construct IPsec proposal",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        /* Proposals have already been added. Verify the traffic selectors
         * received.
         */
        status = IKE2_Verify_TS(handle);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("TS does not match", NERR_RECOVERABLE,
                            __FILE__, __LINE__);
        }

    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Save_Auth_Data
*
* DESCRIPTION
*
*       Saves the first pair of messages for later use in calculation of
*       authentication data and verification of authentication data sent
*       by peer.
*
* INPUTS
*
*       *first_msg              Pointer to location where message will be
*                               saved.
*       *message                Actual message to be saved.
*       message_len             Length of the message.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATIC STATUS IKE2_Save_Auth_Data(UINT8 **first_msg, UINT8 *message,
                                  UINT32 message_len)
{
    STATUS status = NU_SUCCESS;

#if (IKE2_DEBUG == NU_TRUE)
    if((first_msg == NU_NULL) || (message == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    if(*first_msg != NU_NULL)
    {
        if(NU_Deallocate_Memory(*first_msg) != NU_SUCCESS)
        {
            NLOG_Error_Log("Could not deallocate previous memory for first \
                           message", NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID **)first_msg,
                                message_len, NU_NO_SUSPEND);
    if(status == NU_SUCCESS)
    {
        *first_msg = TLS_Normalize_Ptr(*first_msg);
        NU_BLOCK_COPY(*first_msg, message, message_len);
    }
    else
    {
        NLOG_Error_Log("Could not allocate memory to store authentication \
                       data", NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Process_SA_INIT
*
* DESCRIPTION
*
*       Entry point for state machine to negotiate an IKE SA.
*
* INPUTS
*
*       *handle                 Information for exchange under way.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*       IKE_SA_NOT_FOUND        No SA has yet been allocated.
*
*************************************************************************/
STATUS IKE2_Process_SA_INIT(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_STATE_PARAMS   *params;
    UINT8               state;
#if(IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_params == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_sa == NU_NULL)
    {
        return (IKE_SA_NOT_FOUND);
    }
    else if(handle->ike2_params->ike2_policy == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif
    IKE2_DEBUG_LOG("IKE SA Exchange");

    params = handle->ike2_params;
    if (params->ike2_in.ike2_hdr != NU_NULL)
    {
        /* Check if Minor version matches. */
        if(params->ike2_in.ike2_hdr->ike2_minor_version != IKE2_MINOR_VERSION)
        {
            NLOG_Error_Log("Minor version does not match",
                NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* If this packet is not one of the first pair of packet in
         * exchange, it must have non-zero message ID.
         */
        if((params->ike2_in.ike2_hdr->ike2_exchange_type != IKE2_SA_INIT)
                && (params->ike2_in.ike2_hdr->ike2_msg_id == 0))
        {
            NLOG_Error_Log("Invalid message ID, should be non-zero",
                NERR_RECOVERABLE, __FILE__, __LINE__);
            status = IKE2_INVALID_MSGID;
        }

        /* If this packet is a response to our request, check that it has
         * correct message ID as in request.
         */
        else if(((params->ike2_in.ike2_hdr->ike2_flags & IKE2_HDR_RESPONSE_FLAG)
                    != 0) && (params->ike2_in.ike2_hdr->ike2_msg_id
                    != handle->ike2_next_id_nu -1))
        {
            NLOG_Error_Log("Reply does not match with the last request",
                NERR_RECOVERABLE, __FILE__, __LINE__);
            status = IKE2_INVALID_MSGID;
        }

    }

    if(status == NU_SUCCESS)
    {
        state = handle->ike2_state;

        if(state != IKE2_COMPLETE_STATE)
        {
            status = IKE2_State_Handler[(INT)state](handle);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("IKEv2 Exchange failed!", NERR_RECOVERABLE,
                    __FILE__, __LINE__);
            }

            if(state == IKE2_SA_ESTABLISHED)
            {
                IKE2_DEBUG_LOG("IKE Exchange completed successfully");
            }
        }

        else
        {
            NLOG_Error_Log("SA already established", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Process_CREATE_CHILD_SA
*
* DESCRIPTION
*
*       Entry point for state machine to negotiate an Create Child SA
*       exchange.
*
* INPUTS
*
*       *handle                 Information for exchange under way.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful execution.
*
*************************************************************************/
STATUS IKE2_Process_CREATE_CHILD_SA(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_STATE_PARAMS   *params;
    UINT8               state;

#if(IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_params == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_sa == NU_NULL)
    {
        return (IKE_SA_NOT_FOUND);
    }
    else if(handle->ike2_params->ike2_policy == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("IKE SA Exchange");

    params = handle->ike2_params;

    if (params->ike2_in.ike2_hdr != NU_NULL)
    {
        /* Check if Minor version matches. */
        if(params->ike2_in.ike2_hdr->ike2_minor_version != IKE2_MINOR_VERSION)
        {
            NLOG_Error_Log("Minor version not supported",
                NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* If this packet is a response to our request, check that it has
         * correct message ID as in request.
         */
        if(((params->ike2_in.ike2_hdr->ike2_flags & IKE2_HDR_RESPONSE_FLAG)
                 == NU_TRUE) && (params->ike2_in.ike2_hdr->ike2_msg_id
                 != handle->ike2_next_id_nu -1))
        {
            NLOG_Error_Log("Reply does not match with the last request",
                NERR_RECOVERABLE, __FILE__, __LINE__);
            status = IKE2_INVALID_MSGID;
        }

        else
        {
            if((params->ike2_in.ike2_hdr->ike2_flags &
                IKE2_HDR_RESPONSE_FLAG) == 0)
            {
                handle->ike2_state = IKE2_STATE_CREATE_CHILD_SA_R;
            }
        }

    }

    if(status == NU_SUCCESS)
    {
        state = handle->ike2_state;

        if(state != IKE2_COMPLETE_STATE)
        {
            status = IKE2_State_Handler[(INT)state](handle);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("IKEv2 Exchange failed!", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("SA already established", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE2_Process_CREATE_CHILD_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Replace_SA
*
* DESCRIPTION
*
*       This function replaces the old IKEv2 SA with the newly re-keyed SA.
*
* INPUTS
*
*       *handle                 The handle to the exchange under progress
*       *new_sa                 Pointer to the new SA to be added
*
* OUTPUTS
*
*       NU_SUCCESS              On successfully replacing the old SA
*
*************************************************************************/
STATUS IKE2_Replace_SA(IKE2_EXCHANGE_HANDLE *handle, IKE2_SA *new_sa)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_SA                 *temp_sa;
    IKE2_SA                 *temp_sa_next;
    IKE2_IPS_SA_INDEX       *ipsec_sa;
    IKE2_EXCHANGE_HANDLE    *temp_handle;

    /* Copy all the child SA's from old IKE SA to the new IKE SA. */

    ipsec_sa = handle->ike2_sa->ips_sa_index.flink;

    /* Check if at least one SA index is present. */
    while(ipsec_sa != NU_NULL)
    {
        SLL_Enqueue(&new_sa->ips_sa_index, ipsec_sa);
        ipsec_sa = ipsec_sa->flink;
    }

    /* Set the flink and last pointer in this DB to NULL so that the
     * delete SA call does not deallocate these indices.
     */
    handle->ike2_sa->ips_sa_index.flink = NU_NULL;
    handle->ike2_sa->ips_sa_index.last = NU_NULL;

    temp_handle = handle->ike2_sa->xchg_db.ike2_flink;

    while(temp_handle != NU_NULL)
    {
        IKE2_Add_Exchange_Handle(&handle->ike2_new_sa->xchg_db,
                                 temp_handle);

        /* If the SA referenced by the current handle is the same
         * as the SA currently being replaced, then replace its
         * reference with the new SA. */
        if(temp_handle->ike2_sa == handle->ike2_sa)
        {
            temp_handle->ike2_sa = handle->ike2_new_sa;
        }

        temp_handle = temp_handle->ike2_flink;
    }

    /* Set the flink and last pointer in this DB to NULL so that the
     * "clean-up exchange" call does not deallocate these items.
     */
    handle->ike2_sa->xchg_db.ike2_flink = NU_NULL;
    handle->ike2_sa->xchg_db.ike2_last_link = NU_NULL;

    temp_sa = handle->ike2_params->ike2_policy->ike_sa_list.ike_flink;

    while(temp_sa != NU_NULL)
    {
        temp_sa_next = temp_sa->ike_flink;

        if((temp_sa == handle->ike2_sa) && (memcmp(temp_sa->ike2_local_spi,
            handle->ike2_sa->ike2_local_spi, IKE2_SPI_LENGTH) == 0) &&
           (memcmp(temp_sa->ike2_remote_spi,
           handle->ike2_sa->ike2_remote_spi, IKE2_SPI_LENGTH) == 0))
        {
            NU_BLOCK_COPY(&handle->ike2_new_sa->ike_node_addr,
                          &handle->ike2_sa->ike_node_addr,
                          sizeof(struct addr_struct));

            handle->ike2_new_sa->ike2_current_handle = handle;
            handle->ike2_new_sa->ike2_flags = handle->ike2_sa->ike2_flags;

            status = IKE2_Delete_IKE_SA(&handle->ike2_params->ike2_policy->ike_sa_list,
                                        temp_sa);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to delete IKE SA", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }
        }

        temp_sa = temp_sa_next;
    }

    /* If Initiator and Responder roles were switched for this exchange. */
    if(handle->ike2_new_sa->ike2_switch_role == NU_TRUE)
    {
        handle->ike2_new_sa->ike2_switch_role = NU_FALSE;

        handle->ike2_flags ^= IKE2_INITIATOR;
        handle->ike2_flags ^= IKE2_RESPONDER;
    }

    /* Set state of the new SA to established. */
    handle->ike2_new_sa->ike_state = IKE2_SA_ESTABLISHED;

    status = IKE2_Add_IKE_SA(&handle->ike2_params->ike2_policy->ike_sa_list,
                             new_sa);

    if(status == NU_SUCCESS)
    {
        handle->ike2_sa = new_sa;
        handle->ike2_new_sa = NU_NULL;

        /* SA has been re-keyed. Reset the message IDs to zero. */
        handle->ike2_next_id_nu = 0;
        handle->ike2_next_id_peer = 0;
    }

    else
    {
        NLOG_Error_Log("IKE SA could not be replaced", NERR_RECOVERABLE,
                        __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Replace_SA */

#endif

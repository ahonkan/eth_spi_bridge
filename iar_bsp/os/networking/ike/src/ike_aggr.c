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
*       ike_aggr.c
*
* COMPONENT
*
*       IKE - Aggressive Mode
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE
*       Aggressive Mode Exchange.
*
* DATA STRUCTURES
*
*       IKE_Aggr_Mode_Handlers  Aggressive mode state handlers.
*
* FUNCTIONS
*
*       IKE_Aggr_State_1
*       IKE_Aggr_State_2
*       IKE_Aggr_State_2_Recv
*       IKE_Aggr_State_2_Send
*       IKE_Aggr_State_3
*       IKE_Aggr_State_3_Recv
*       IKE_Aggr_State_3_Send
*       IKE_Aggr_State_4
*       IKE_Process_Aggr_Mode
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_api.h
*       ike_pkt.h
*       ike_enc.h
*       ike_auth.h
*       ike_oak.h
*       ike_evt.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_pkt.h"
#include "networking/ike_enc.h"
#include "networking/ike_auth.h"
#include "networking/ike_oak.h"
#include "networking/ike_evt.h"

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)

/* Local function prototypes. */
STATIC STATUS IKE_Aggr_State_1(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_2(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_2_Recv(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_2_Send(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_3(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_3_Recv(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_3_Send(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Aggr_State_4(IKE_PHASE1_HANDLE *ph1);

/* Aggressive Mode state handlers. */
static
IKE_AGGR_MODE_FUNC IKE_Aggr_Mode_Handlers[IKE_TOTAL_AGGR_MODE_STATES] =
{
    IKE_Aggr_State_1,
    IKE_Aggr_State_2,
    IKE_Aggr_State_3,
    IKE_Aggr_State_4
};

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_1
*
* DESCRIPTION
*
*       This function implements Aggressive Mode state 1 of the
*       IKE state machine. It initiates an IKE Phase 1
*       exchange on behalf of the Initiator. An un-negotiated
*       and incomplete IKE SA must be added to the SADB
*       before this function is called. This SA and the IKE
*       Policy that applies to it must be passed as parameters.
*       The function generates an SA proposal (using the
*       Policy), Key exchange data, Nonce data and Identification
*       data and sends it to the Responder. The Responder
*       address is obtained from the IKE SA.
*
*       All fields of the IKE SA must be filled, except for
*       those which are undetermined and are to be negotiated.
*
*       This function is called in response to a local request
*       for SA establishment. It must not result in response
*       to an incoming message.
*
*       Following are payloads sent in this state:
*
*       PSK Auth:
*
*       Initiator*                      Responder
*       ---------                       ---------
*       HDR, SA, KE, Ni, IDii   -->
*
*       SIG Auth:
*
*       Initiator*                              Responder
*       ---------                               ---------
*       HDR, SA, KE, Ni, IDii[, CREQ]   -->
*
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message longer than output buffer.
*       IKE_UNEXPECTED_MESSAGE  This state must not result in
*                               response to any incoming message.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_1(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    IKE_ENC_MESSAGE         *out;
    IKE_STATE_PARAMS        *params;
    IKE_ENC_HDR             enc_hdr;
    IKE_KEYXCHG_ENC_PAYLOAD enc_key;
    IKE_NONCE_ENC_PAYLOAD   enc_nonce;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERTREQ_ENC_PAYLOAD enc_certreq;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Aggressive mode: state 1");

    /* Set local pointers to commonly used data in the Handle. */
    out    = &ph1->ike_params->ike_out;
    params = ph1->ike_params;

    /* Make sure packet pointer is NULL. */
    if(params->ike_packet != NU_NULL)
    {
        /* Set error code. */
        status = IKE_UNEXPECTED_MESSAGE;
    }

    else
    {
        /* Set outbound payloads used in this state. */
        IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(out->ike_sa, &IKE_Large_Data.ike_enc_sa);
        IKE_SET_OUTBOUND(out->ike_key, &enc_key);
        IKE_SET_OUTBOUND(out->ike_nonce, &enc_nonce);
        IKE_SET_OUTBOUND(out->ike_id_i, &IKE_Large_Data.ike_enc_idi);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        if((ph1->ike_params->ike_policy->ike_flags &
            IKE_INBAND_CERT_XCHG) != 0)
        {
            IKE_SET_OUTBOUND(out->ike_certreq, &enc_certreq);
        }
#endif

        /* Initialize chain of outbound payloads. */
        IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

        /* Generate SA payload proposal using the Policy. */
        status = IKE_Construct_Proposal(ph1);

        if(status == NU_SUCCESS)
        {
            /* Set the Diffie-Hellman group description in the SA,
             * since this parameter can not be negotiated in
             * Aggressive mode and will be the same in all
             * transforms of the proposal.
             */
            ph1->ike_sa->ike_attributes.ike_group_desc =
                params->ike_policy->ike_xchg1_attribs->ike_group_desc;

            /* Generate Key Exchange data. This would also generate
             * the Diffie-Hellman key pair and store it in the Handle.
             */
            status = IKE_Generate_KeyXchg_Data(ph1);

            if(status == NU_SUCCESS)
            {
                /* Generate Nonce data and add its payload to the
                 * outgoing chain. This also stores the data in
                 * a dynamically allocated buffer of the Handle.
                 */
                status = IKE_Generate_Nonce_Data(ph1);

                if(status == NU_SUCCESS)
                {
                    /* Add Identification payload to the outgoing chain. */
                    status = IKE_Generate_ID_Data(ph1);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

                    if(status == NU_SUCCESS)
                    {
                        if((ph1->ike_params->ike_policy->ike_flags &
                            IKE_INBAND_CERT_XCHG) != 0)
                        {
                            status = IKE_Generate_CertReq_Data(ph1);
                        }

                        if(status == NU_SUCCESS)
                        {
#endif /*(IKE_INCLUDE_SIG_AUTH == NU_TRUE)*/
                            /* Terminate chain of payloads. */
                            IKE_END_CHAIN(out->ike_last);

                            /* Initialize ISAKMP header. */
                            IKE_Set_Header(&enc_hdr, 0,
                                           ph1->ike_sa->ike_cookies,
                                           IKE_XCHG_AGGR, 0);

                            /* Send the message. */
                            status = IKE_Send_Packet(ph1);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to send IKE packet",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to generate CREQ payload",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
#endif /*(IKE_INCLUDE_SIG_AUTH == NU_TRUE)*/
                    else
                    {
                        NLOG_Error_Log("Failed to generate ID payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to generate Nonce payload",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to generate Key Exchange payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to construct IKE proposal",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* If no error occurred, carry out further processing after
     * packet has been sent.
     */
    if(status == NU_SUCCESS)
    {
        /* Save the SAi_b in the Handle. */
        status = IKE_Extract_Raw_Payload(ph1->ike_last_message,
                                         &ph1->ike_sa_b,
                                         &ph1->ike_sa_b_len,
                                         IKE_SA_PAYLOAD_ID);

        if(status == NU_SUCCESS)
        {
            /* Save IDii_b in the Handle. This is used in state 3. */
            status = IKE_Extract_Raw_Payload(ph1->ike_last_message,
                                             &ph1->ike_id_b,
                                             &ph1->ike_id_b_len,
                                             IKE_ID_PAYLOAD_ID);

            if(status == NU_SUCCESS)
            {
                /* Add phase 1 timeout event. */
                status = IKE_Set_Timer(IKE_Phase1_Timeout_Event,
                                       (UNSIGNED)ph1->ike_sa,
                                       (UNSIGNED)&params->ike_policy->
                                           ike_sa_list,
                                       IKE_PHASE1_TIMEOUT);

                if(status == NU_SUCCESS)
                {
                    /* Increment the machine state. */
                    ph1->ike_xchg_state += IKE_NEXT_STATE_INC;
                }

                else
                {
                    NLOG_Error_Log("Failed to set exchange timeout timer",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to extract raw ID payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to extract raw SA payload",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_2
*
* DESCRIPTION
*
*       This function implements Aggressive Mode state 2 of
*       the IKE state machine. It handles the first message
*       of this exchange, received by the Responder. The
*       message contains an SA payload containing the
*       Initiator's proposal, Key Exchange data, Nonce data
*       and Identification data.
*
*       As this is the first message to the Responder, the
*       IKE SA and Handle passed to this function are not
*       part of the database. This function adds the
*       incomplete IKE SA and the Handle to the SADB.
*
*       Since this state involves a large number of incoming
*       and outgoing payloads, it has been divided into two
*       utility functions. One for receiving and another
*       for sending payloads.
*
*       Following are payloads received/sent in this state:
*
*       PSK Auth:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR, SA, KE, Ni, IDii   -->
*                               <--     HDR, SA, KE, Nr, IDir, HASH_R
*
*       SIG Auth:
*
*       Initiator                               Responder*
*       ---------                               ---------
*       HDR, SA, KE, Ni, IDii[, CREQ]   -->
*                                       <--     HDR, SA, KE, Nr, IDir, [CREQ,] [CERT,] SIG_R
*
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_COOKIE      Cookie is invalid.
*       IKE_INVALID_PAYLOAD     SA Payload is invalid.
*       IKE_INVALID_PROTOCOL    Protocol in SA is not ISAKMP.
*       IKE_INVALID_TRANSFORM   Invalid transform in SA.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_KEYLEN      Negotiated encryption key
*                               length is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in packet.
*       IKE_TOO_MANY_PROPOSALS  The SA should only contain a
*                               single proposal payload.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed limit.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*       IKE_NOT_NEGOTIABLE      Specified SA no negotiable
*                               under the current policy.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_MISSING_ATTRIB      One or more of the required
*                               attributes was not negotiated.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_UNSUPPORTED_ALGO    Algorithm used in attribute is
*                               not supported by IKE.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_NOT_FOUND           Pre-shared key not found.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_ID_MISMATCH         Unexpected identity of remote node.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_2(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status;
    IKE_DEC_MESSAGE             *in;
    IKE_ENC_MESSAGE             *out;
    IKE_NONCE_DEC_PAYLOAD       dec_nonce;
    IKE_ENC_HDR                 enc_hdr;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Aggressive mode: state 2");

    /* Set local pointers to commonly used data in the Handle. */
    in  = &ph1->ike_params->ike_in;
    out = &ph1->ike_params->ike_out;

    /* Make sure next payload is an SA. */
    if(in->ike_hdr->ike_first_payload != IKE_SA_PAYLOAD_ID)
    {
        /* Report unexpected payload. */
        status = IKE_MISSING_PAYLOAD;
    }

    /* This message must not be encrypted. */
    else if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) != 0)
    {
        /* Report invalid flags. */
        status = IKE_INVALID_FLAGS;
    }

    /* Make sure the Responder cookie is not set. */
    else if(IKE_COOKIE_IS_SET(in->ike_hdr->ike_rcookie))
    {
        /* Report error because cookie is set. */
        status = IKE_INVALID_COOKIE;
    }

    else
    {
        /* Partially set inbound payloads used in this state. */
        IKE_SET_INBOUND_REQUIRED(in->ike_sa, &IKE_Large_Data.ike_dec_sa);
        IKE_SET_INBOUND_REQUIRED(in->ike_nonce, &dec_nonce);

        /* Partially set outbound payloads required during
         * decoding of the received packet.
         */
        IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(out->ike_sa, &IKE_Large_Data.ike_enc_sa);

        /* Specify ISAKMP header as first item in payload chain. */
        IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

        /* Receive all the payloads. */
        status = IKE_Aggr_State_2_Recv(ph1);

        if(status == NU_SUCCESS)
        {
            /* Update the Handle pointer. */
            ph1 = ph1->ike_sa->ike_phase1;

            /* Send a message in response to the received payloads. */
            status = IKE_Aggr_State_2_Send(ph1);

            if(status == NU_SUCCESS)
            {
                /* Update hash of last message received. */
                IKE_UPDATE_RESEND(ph1);

                /* Add phase 1 timeout event. */
                status = IKE_Set_Timer(IKE_Phase1_Timeout_Event,
                                       (UNSIGNED)ph1->ike_sa,
                                       (UNSIGNED)&ph1->ike_params->
                                           ike_policy->ike_sa_list,
                                       IKE_PHASE1_TIMEOUT);

                if(status == NU_SUCCESS)
                {
                    /* Increment the machine state. */
                    ph1->ike_xchg_state += IKE_NEXT_STATE_INC;
                }

                else
                {
                    NLOG_Error_Log("Failed to set exchange timeout timer",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to send message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to receive payloads",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_2_Recv
*
* DESCRIPTION
*
*       This is a utility function which decodes the chain
*       of payloads expected in Aggressive mode state 2. It
*       processes these payloads accordingly.
*
*       Note that the ike_sa_b and ike_id_b members of the
*       Handle are dynamically allocated by this function.
*       The caller is responsible for freeing them after
*       use.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     A payload is invalid.
*       IKE_INVALID_PROTOCOL    Protocol is not ISAKMP.
*       IKE_INVALID_TRANSFORM   Only IKE_KEY transform is allowed.
*       IKE_INVALID_KEYLEN      Negotiated encryption key
*                               length is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_NOT_NEGOTIABLE      Proposal not negotiable using
*                               the specified policy.
*       IKE_MISSING_ATTRIB      One or more of the required
*                               attributes are missing in the
*                               negotiation.
*       IKE_UNSUPPORTED_ALGO    Algorithm used in attribute is
*                               not supported by IKE.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_NOT_FOUND           Pre-shared key not found.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_ID_MISMATCH         Unexpected identity of remote node.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_2_Recv(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_STATE_PARAMS        *params;
    IKE_KEYXCHG_DEC_PAYLOAD dec_key;
    IKE_ID_DEC_PAYLOAD      dec_id;
    IKE_ATTRIB              *policy_attrib;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERTREQ_DEC_PAYLOAD dec_certreq;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Receiving Aggressive mode payloads");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph1->ike_params->ike_in;
    params = ph1->ike_params;

    /* Partially set the rest of the inbound payloads. */
    IKE_SET_INBOUND_REQUIRED(in->ike_key, &dec_key);
    IKE_SET_INBOUND_REQUIRED(in->ike_id_i, &dec_id);
    IKE_SET_INBOUND_OPTIONAL(in->ike_certreq, &dec_certreq);

    /* Decode all payloads in the packet. */
    status = IKE_Decode_Message(params->ike_packet->ike_data, in);

    if(IKE_PAYLOAD_IS_PRESENT(in->ike_certreq) == NU_TRUE)
    {
        ph1->ike_flags |= IKE_CERTREQ_RECEIVED;
    }

    if(status == NU_SUCCESS)
    {
        /* Make selections from the given proposal. */
        status = IKE_Select_Proposal(ph1, &policy_attrib);

        if(status == NU_SUCCESS)
        {
            /* Set the negotiated attributes of the SA payload
             * in the IKE SA. It is safe to ignore the return value.
             */
            status = IKE_Convert_Attributes(
                         IKE_Large_Data.ike_enc_sa.ike_proposals[0].
                             ike_transforms[0].ike_sa_attributes,
                         IKE_Large_Data.ike_enc_sa.ike_proposals[0].
                             ike_transforms[0].ike_num_attributes,
                         &ph1->ike_sa->ike_attributes);

            if(status == NU_SUCCESS)
            {
                /* Also set the Authentication method specific
                 * attributes in the SA.
                 */
                status = IKE_Auth_Parameters(policy_attrib,
                                             ph1->ike_sa, &dec_id);

                if(status == NU_SUCCESS)
                {
                    /* Verify the remote node's identity. */
                    status = IKE_Verify_Auth_Data(ph1);

                    if(status == NU_SUCCESS)
                    {
                        /* Add the SA to the policy's SADB. */
                        status = IKE_Add_SA(
                                     &params->ike_policy->ike_sa_list,
                                     ph1->ike_sa, &ph1->ike_sa);

                        if(status == NU_SUCCESS)
                        {
                            /* Update the Handle pointer. */
                            ph1 = ph1->ike_sa->ike_phase1;

                            /* Allocate buffer for remote Diffie-Hellman
                             * public key.
                             */
                            status = NU_Allocate_Memory(
                                        IKE_Data.ike_memory,
                                        (VOID**)&ph1->ike_dh_remote_key,
                                        in->ike_key->ike_keyxchg_data_len,
                                        NU_NO_SUSPEND);

                            if(status == NU_SUCCESS)
                            {
                                /* Normalize the pointer. */
                                ph1->ike_dh_remote_key =
                                    TLS_Normalize_Ptr(
                                        ph1->ike_dh_remote_key);

                                /* Copy Diffie-Hellman key to Handle. */
                                NU_BLOCK_COPY(ph1->ike_dh_remote_key,
                                    in->ike_key->ike_keyxchg_data,
                                    in->ike_key->ike_keyxchg_data_len);

                                /* Also set public key length in Handle. */
                                ph1->ike_dh_remote_key_len =
                                    in->ike_key->ike_keyxchg_data_len;

                                /* Save the SAi_b in the Handle. */
                                status = IKE_Extract_Raw_Payload(
                                    params->ike_packet->ike_data,
                                    &ph1->ike_sa_b, &ph1->ike_sa_b_len,
                                    IKE_SA_PAYLOAD_ID);

                                if(status == NU_SUCCESS)
                                {
                                    /* Also save IDii_b in the Handle. */
                                    status = IKE_Extract_Raw_Payload(
                                        params->ike_packet->ike_data,
                                        &ph1->ike_id_b, &ph1->ike_id_b_len,
                                        IKE_ID_PAYLOAD_ID);
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                        "Failed to extract raw ID payload",
                                        NERR_RECOVERABLE, __FILE__,
                                        __LINE__);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to allocate memory for DH key",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Unable to add SA to IKE SADB",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Authentication failed",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log(
                        "Unable to obtain authentication parameters",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to convert proposal attributes",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("No proposal selected",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to decode incoming message",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_2_Recv */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_2_Send
*
* DESCRIPTION
*
*       This is a utility function which encodes a chain
*       of payloads in response to the incoming payloads.
*       This function is called from state 2 of the
*       Aggressive mode state machine.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier not supported.
*       IKE_LENGTH_IS_SHORT     Message buffer not large enough.
*       IKE_UNSUPPORTED_METHOD  Authentication method is not
*                               supported.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     SA payload is invalid.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_2_Send(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status;
    IKE_ENC_MESSAGE             *out;
    IKE_KEYXCHG_ENC_PAYLOAD     enc_key;
    IKE_NONCE_ENC_PAYLOAD       enc_nonce;
    IKE_HASH_ENC_PAYLOAD        enc_hash;
    IKE_SIGNATURE_ENC_PAYLOAD   enc_sig;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERT_ENC_PAYLOAD        enc_cert;
    IKE_CERTREQ_ENC_PAYLOAD     enc_certreq;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Sending Aggressive mode payloads");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;

    /* Partially set the remaining outbound payloads
     * used in this state.
     */
    IKE_SET_OUTBOUND(out->ike_key, &enc_key);
    IKE_SET_OUTBOUND(out->ike_nonce, &enc_nonce);
    IKE_SET_OUTBOUND(out->ike_id_i, &IKE_Large_Data.ike_enc_idi);
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if((ph1->ike_params->ike_policy->ike_flags & IKE_INBAND_CERT_XCHG)
        != 0)
    {
        IKE_SET_OUTBOUND(out->ike_certreq, &enc_certreq);
    }
    if(((ph1->ike_params->ike_policy->ike_flags &
        IKE_SEND_CERT_PROACTIVELY) != 0) ||
        ((ph1->ike_flags & IKE_CERTREQ_RECEIVED) != 0))
    {
        IKE_SET_OUTBOUND(out->ike_cert, &enc_cert);
    }
#endif
    IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);
    IKE_SET_OUTBOUND(out->ike_sig, &enc_sig);

    /* Generate the Key Exchange data for the Responder and
     * add it following the ISAKMP header. This would also
     * generate the Diffie-Hellman key pair and store it in
     * the SA.
     */
    status = IKE_Generate_KeyXchg_Data(ph1);

    if(status == NU_SUCCESS)
    {
        /* Generate Nonce data and add its payload to the
         * outgoing chain. This also stores the data in
         * a dynamically allocated buffer of the Handle.
         */
        status = IKE_Generate_Nonce_Data(ph1);

        if(status == NU_SUCCESS)
        {
            /* Generate and add the Identification payload
             * to the payloads chain.
             */
            status = IKE_Generate_ID_Data(ph1);

            if(status == NU_SUCCESS)
            {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                if((ph1->ike_params->ike_policy->ike_flags &
                    IKE_INBAND_CERT_XCHG) != 0)
                {
                    status = IKE_Generate_CertReq_Data(ph1);
                }

                if(status == NU_SUCCESS)
                {
                    if(((ph1->ike_params->ike_policy->ike_flags &
                        IKE_SEND_CERT_PROACTIVELY) != 0) ||
                        ((ph1->ike_flags & IKE_CERTREQ_RECEIVED) != 0))
                    {
                        status = IKE_Generate_Cert_Data(ph1);
                    }

                    if(status == NU_SUCCESS)
                    {
#endif
                        /* Temporarily terminate chain of payloads to
                         * encode the ID payload only.
                         */
                        IKE_END_CHAIN(out->ike_last);

                        /* Encode the Identification payload as the
                         * encoded payload is used to calculate HASH_I
                         * and HASH_R.
                         */
                        status = IKE_Encode_ID_Payload(
                            IKE_Large_Data.ike_id.ike_raw_id, (UINT16)
                            sizeof(IKE_Large_Data.ike_id.ike_raw_id),
                            out->ike_id_i);

                        if(status == NU_SUCCESS)
                        {
                            /* Initialize the Signature data to NULL. */
                            enc_sig.ike_signature_data = NU_NULL;

                            /* Compute the key material. This is a VERY
                             * CPU intensive operation.
                             */
                            status = IKE_Phase1_Key_Material(ph1);

                            if(status == NU_SUCCESS)
                            {
                                /* Add the Signature or Hash payload to
                                 * chain.
                                 */
                                status = IKE_Generate_Hash_Data(ph1,
                                    IKE_Large_Data.ike_id.ike_raw_id
                                    + IKE_GEN_HDR_LEN,
                                    out->ike_id_i->ike_hdr.ike_payload_len
                                    - IKE_GEN_HDR_LEN);

                                if(status == NU_SUCCESS)
                                {
                                    /* Terminate chain of payloads. */
                                    IKE_END_CHAIN(out->ike_last);

                                    /* Initialize ISAKMP header. */
                                    IKE_Set_Header(out->ike_hdr, 0,
                                        ph1->ike_sa->ike_cookies,
                                        IKE_XCHG_AGGR,
                                        ph1->ike_params->ike_in.ike_hdr->
                                        ike_flags);

                                    /* Send the message. */
                                    status = IKE_Send_Packet(ph1);

                                    if(status != NU_SUCCESS)
                                    {
                                        NLOG_Error_Log(
                                            "Failed to send IKE packet",
                                            NERR_RECOVERABLE, __FILE__,
                                            __LINE__);
                                    }

                                    /* If the Signature data is allocated. */
                                    if(enc_sig.ike_signature_data != NU_NULL)
                                    {
                                        /* Free the Signature data. */
                                        if(NU_Deallocate_Memory(enc_sig.
                                                                ike_signature_data)
                                            != NU_SUCCESS)
                                        {
                                            /* Log the message. */
                                            NLOG_Error_Log(
                                            "Cannot deallocate Signature",
                                            NERR_SEVERE, __FILE__,
                                            __LINE__);
                                        }
                                    }
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                    "Failed to generate hash payload",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to compute Phase 1 keys",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                        else
                        {
                            NLOG_Error_Log("Failed to encode ID payload",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate Cert payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Failed to generate Cert-Req payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
#endif
            }

            else
            {
                NLOG_Error_Log("Failed to generate ID payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to generate Nonce payload",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to generate Key Exchange payload",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_2_Send */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_3
*
* DESCRIPTION
*
*       This function implements Aggressive Mode state 3 of the
*       IKE state machine. It receives all the payloads sent
*       by the Responder and processes them. The response to
*       the proposal is verified, a shared secret is calculated
*       based on the Key Exchange data, SKEYIDs are then
*       computed and the exchange is authenticated using the
*       Responder's Identification and Hash data.
*
*       Finally a response is sent to the Responder, containing
*       the Initiator's Hash (or Signature) data. The negotiated
*       SA is finalized for the Initiator if this function
*       completes successfully.
*
*       Since this state involves a large number of incoming
*       and outgoing payloads, it has been divided into two
*       utility functions. One for receiving and another
*       for sending payloads.
*
*       Following are payloads received/sent in this state:
*
*       PSK Auth:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR, SA, KE, Nr, IDir, HASH_R
*       HDR, HASH_I             -->
*
*       SIG Auth:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR, SA, KE, Nr, IDir[, CREQ][, CERT], SIG_R
*       HDR[,CERT], SIG_I       -->
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     SA payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in message.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_3(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status;
    IKE_DEC_MESSAGE             *in;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Aggressive mode: state 3");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

    /* Make sure first payload is an SA. */
    if(in->ike_hdr->ike_first_payload != IKE_SA_PAYLOAD_ID)
    {
        /* Report missing payload. */
        status = IKE_MISSING_PAYLOAD;
    }

    /* This message must not be encrypted. */
    else if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) != 0)
    {
        /* Report invalid flags. */
        status = IKE_INVALID_FLAGS;
    }

    /* Make sure the Responder cookie is set. */
    else if(!IKE_COOKIE_IS_SET(in->ike_hdr->ike_rcookie))
    {
        /* Report error because cookie not set. */
        status = IKE_INVALID_COOKIE;
    }

    else
    {
        /* Receive all the payloads. */
        status = IKE_Aggr_State_3_Recv(ph1);

        if(status == NU_SUCCESS)
        {
            status = IKE_Aggr_State_3_Send(ph1);

            if(status == NU_SUCCESS)
            {
                /* Update hash of last message received. */
                IKE_UPDATE_RESEND(ph1);

                /* SA established, so synchronize the
                 * encryption IV.
                 */
                IKE_SYNC_IV(ph1->ike_sa->ike_encryption_iv,
                            ph1->ike_sa->ike_decryption_iv,
                            ph1->ike_sa);

                /* Phase 1 completed. */
                ph1->ike_xchg_state = IKE_COMPLETE_STATE;
            }

            else
            {
                NLOG_Error_Log("Failed to send message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to receive payloads",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_3 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_3_Recv
*
* DESCRIPTION
*
*       This is a utility function which processes the
*       payloads received by the Initiator in Aggressive
*       mode, state 3 of the IKE State Machine.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     A payload is invalid.
*       IKE_INVALID_PROTOCOL    Protocol in proposal is not ISAKMP.
*       IKE_INVALID_TRANSFORM   Transform ID is not valid.
*       IKE_LENGTH_IS_SHORT     Not enough data in message.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_TOO_MANY_PROPOSALS  SA contains more than one proposal.
*       IKE_TOO_MANY_TRANSFORMS SA contains more than one transform.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_UNSUPPORTED_ATTRIB  An unrecognized attribute
*                               was encountered.
*       IKE_UNSUPPORTED_ALGO    Algorithm used in attribute is
*                               not supported by IKE.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_ID_MISMATCH         Unexpected identity of remote
*                               node.
*       IKE_NOT_FOUND           Pre-shared key not found.
*       IKE_MISSING_ATTRIB      One or more of the required
*                               attributes are missing in the
*                               negotiation.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_3_Recv(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status;
    IKE_DEC_MESSAGE             *in;
    IKE_STATE_PARAMS            *params;
    IKE_ATTRIB                  *policy_attrib;
    UINT8                       *id_b;
    UINT16                      id_b_len;
    IKE_KEYXCHG_DEC_PAYLOAD     dec_key;
    IKE_NONCE_DEC_PAYLOAD       dec_nonce;
    IKE_ID_DEC_PAYLOAD          dec_id;
    IKE_AUTH_DEC_PAYLOADS       auth;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERTREQ_DEC_PAYLOAD     dec_certreq;
    IKE_CERT_DEC_PAYLOAD        dec_cert;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Receiving Aggressive mode payloads");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph1->ike_params->ike_in;
    params = ph1->ike_params;

    /* Partially set inbound payloads used in this state. */
    IKE_SET_INBOUND_REQUIRED(in->ike_sa, &IKE_Large_Data.ike_dec_sa);
    IKE_SET_INBOUND_REQUIRED(in->ike_key, &dec_key);
    IKE_SET_INBOUND_REQUIRED(in->ike_nonce, &dec_nonce);
    IKE_SET_INBOUND_REQUIRED(in->ike_id_i, &dec_id);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if((ph1->ike_params->ike_policy->ike_flags & IKE_INBAND_CERT_XCHG)
        != 0)
    {
        IKE_SET_INBOUND_OPTIONAL(in->ike_certreq, &dec_certreq);
        IKE_SET_INBOUND_OPTIONAL(in->ike_cert, &dec_cert);
    }
#endif

    IKE_SET_INBOUND_REQUIRED(in->ike_hash, &auth.dec_hash);
    IKE_SET_INBOUND_REQUIRED(in->ike_sig, &auth.dec_sig);

    /* Decode all payloads in the packet. */
    status = IKE_Decode_Message(params->ike_packet->ike_data, in);

    if(IKE_PAYLOAD_IS_PRESENT(in->ike_certreq) == NU_TRUE)
    {
        ph1->ike_flags |= IKE_CERTREQ_RECEIVED;
    }

    if(status == NU_SUCCESS)
    {
        /* Make sure proposal in SA payload is valid. */
        status = IKE_Verify_Selection_SA(&IKE_Large_Data.ike_dec_sa);

        if(status == NU_SUCCESS)
        {
            /* Convert the attributes from payload format to the
             * internal representation. Also fill in the default
             * values of those attributes which were not negotiated.
             */
            status = IKE_Convert_Attributes(
                         IKE_Large_Data.ike_dec_sa.ike_proposals[0].
                             ike_transforms[0].ike_sa_attributes,
                         IKE_Large_Data.ike_dec_sa.ike_proposals[0].
                             ike_transforms[0].ike_num_attributes,
                         &ph1->ike_sa->ike_attributes);

            if(status == NU_SUCCESS)
            {
                /* Make sure the attributes negotiated are
                 * those which were sent initially.
                 */
                status = IKE_Verify_Attributes(
                             &ph1->ike_sa->ike_attributes,
                             params->ike_policy->ike_xchg1_attribs,
                             params->ike_policy->ike_xchg1_attribs_no,
                             &policy_attrib);

                if(status == NU_SUCCESS)
                {
                    /* Copy the Responder's cookie to the SA. */
                    NU_BLOCK_COPY(
                        &ph1->ike_sa->ike_cookies[IKE_COOKIE_LEN],
                        in->ike_hdr->ike_rcookie, IKE_COOKIE_LEN);

                    /* Also set the Authentication method specific
                     * attributes in the SA.
                     */
                    status = IKE_Auth_Parameters(policy_attrib,
                                                 ph1->ike_sa, &dec_id);

                    if(status == NU_SUCCESS)
                    {
                        /* Allocate buffer for remote
                         * Diffie-Hellman public key.
                         */
                        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                     (VOID**)&ph1->ike_dh_remote_key,
                                     dec_key.ike_keyxchg_data_len,
                                     NU_NO_SUSPEND);

                        if(status == NU_SUCCESS)
                        {
                            /* Normalize the pointer. */
                            ph1->ike_dh_remote_key =
                                TLS_Normalize_Ptr(ph1->ike_dh_remote_key);

                            /* Store remote Diffie-Hellman public key. */
                            NU_BLOCK_COPY(ph1->ike_dh_remote_key,
                                          dec_key.ike_keyxchg_data,
                                          dec_key.ike_keyxchg_data_len);

                            /* Also store the key length. */
                            ph1->ike_dh_remote_key_len =
                                dec_key.ike_keyxchg_data_len;

                            /* Compute the Phase 1 key material. This is
                             * a VERY CPU intensive operation.
                             */
                            status = IKE_Phase1_Key_Material(ph1);

                            if(status == NU_SUCCESS)
                            {
                                /* Temporarily save the local ID body
                                 * pointer. The pointer currently stored
                                 * is Aggressive mode specific so it must
                                 * be explicitly saved to not get in the
                                 * way of the generic ID processing being
                                 * invoked below.
                                 */
                                id_b = ph1->ike_id_b;
                                id_b_len = ph1->ike_id_b_len;

                                /* Extract remote ID payload body. */
                                status = IKE_Extract_Raw_Payload(
                                    params->ike_packet->ike_data,
                                    &ph1->ike_id_b, &ph1->ike_id_b_len,
                                    IKE_ID_PAYLOAD_ID);

                                if(status == NU_SUCCESS)
                                {
                                    /* Verify the Authentication data. */
                                    status = IKE_Verify_Auth_Data(ph1);

                                    /* Deallocated ID allocation. */
                                    if(NU_Deallocate_Memory(ph1->ike_id_b)
                                       != NU_SUCCESS)
                                    {
                                        NLOG_Error_Log(
                                            "Failed to deallocate memory",
                                            NERR_SEVERE,
                                            __FILE__, __LINE__);
                                    }

                                    /* Set ID pointer to NU_NULL. */
                                    ph1->ike_id_b = NU_NULL;
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                        "Failed to extract raw ID payload",
                                        NERR_RECOVERABLE, __FILE__,
                                        __LINE__);
                                }

                                /* Restore local ID payload pointer. */
                                ph1->ike_id_b = id_b;
                                ph1->ike_id_b_len = id_b_len;
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to calculate Phase 1 keys",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log(
                                "Failed to allocate memory for DH key",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log(
                            "Unable to obtain authentication parameters",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Attribute verification failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                /* Conversion should have succeeded if the selection
                 * was correct. Failure implies that the proposal
                 * was not the same as the one sent earlier.
                 */
                status = IKE_PROPOSAL_TAMPERED;

                NLOG_Error_Log("Proposal tampered by Responder",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Verification of SA selection failed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to decode incoming message",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_3_Recv */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_3_Send
*
* DESCRIPTION
*
*       This is a utility function which sends a reply
*       message in response to the message received in
*       state 3 of the Aggressive Mode State Machine.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_UNSUPPORTED_METHOD  Authentication method is not
*                               supported.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_LENGTH_IS_SHORT     Message buffer not large enough.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_3_Send(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status = NU_SUCCESS;
    IKE_ENC_MESSAGE             *out;
    IKE_ENC_HDR                 enc_hdr;
    IKE_HASH_ENC_PAYLOAD        enc_hash;
    IKE_SIGNATURE_ENC_PAYLOAD   enc_sig;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERT_ENC_PAYLOAD        enc_cert;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;

    /* Set outbound payloads used in this state. */
    IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
    IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);
    IKE_SET_OUTBOUND(out->ike_sig, &enc_sig);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(((ph1->ike_params->ike_policy->ike_flags &
        IKE_SEND_CERT_PROACTIVELY) != 0) ||
        ((ph1->ike_flags & IKE_CERTREQ_RECEIVED) != 0))
    {
        IKE_SET_OUTBOUND(out->ike_cert, &enc_cert);
    }
#endif

    /* Specify ISAKMP header as first item in chain. */
    IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

    /* Initialize Signature data to NULL. This would
     * later be checked to free the dynamically
     * allocated buffer in which the signature is stored.
     */
    enc_sig.ike_signature_data = NU_NULL;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

    if(((ph1->ike_params->ike_policy->ike_flags &
        IKE_SEND_CERT_PROACTIVELY) != 0) ||
        ((ph1->ike_flags & IKE_CERTREQ_RECEIVED) != 0))
    {
        status = IKE_Generate_Cert_Data(ph1);
    }

    if(status == NU_SUCCESS)
    {
#endif
        /* Add the Signature or Hash payload to the chain. */
        status = IKE_Generate_Hash_Data(ph1, ph1->ike_id_b,
                                        ph1->ike_id_b_len);

        if(status == NU_SUCCESS)
        {
            /* Terminate chain of payloads. */
            IKE_END_CHAIN(out->ike_last);

            /* Initialize ISAKMP header. */
            IKE_Set_Header(&enc_hdr, 0, ph1->ike_sa->ike_cookies,
                           IKE_XCHG_AGGR,
                           (ph1->ike_params->ike_in.ike_hdr->ike_flags |
                            IKE_HDR_ENC_MASK));

            /* Send the message. */
            status = IKE_Send_Packet(ph1);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to send IKE packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* If the Signature data is allocated. */
            if(enc_sig.ike_signature_data != NU_NULL)
            {
                /* Free the Signature data. */
                if(NU_Deallocate_Memory(enc_sig.ike_signature_data) != NU_SUCCESS)
                {
                    /* Log the message. */
                    NLOG_Error_Log("Unable to deallocate Signature",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to generate Hash payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    }

    else
    {
        NLOG_Error_Log("Failed to generate Cert payload",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }
#endif

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_3_Send */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_State_4
*
* DESCRIPTION
*
*       This function implements Aggressive Mode state 4 of the
*       IKE state machine. State 4 is executed by the Responder
*       and it authenticates the exchange using either a
*       Pre-shared Key or Signatures. It is responsible for
*       receiving the Hash/Signature data sent by the Initiator
*       and then verifying this data. The message received by
*       this function may or may not be encrypted.
*
*       Following are payloads received in this state:
*
*       PSK Auth:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR, HASH_I             -->
*
*       SIG Auth:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR[, CERT], SIG_I      -->
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Message buffer not large enough.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_AUTH_FAILED         Authentication failed.
*
*************************************************************************/
STATIC STATUS IKE_Aggr_State_4(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status = NU_SUCCESS;
    IKE_DEC_MESSAGE     *in;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Aggressive mode: state 4");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

    /* Make sure the Responder cookie is set. */
    if(!IKE_COOKIE_IS_SET(in->ike_hdr->ike_rcookie))
    {
        /* Report error because cookie not set. */
        status = IKE_INVALID_COOKIE;
    }

    else
    {
        /* If the message is encrypted. */
        if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) != 0)
        {
            /* Decrypt the message. */
            status = IKE_Encrypt(ph1->ike_sa,
                ph1->ike_params->ike_packet->ike_data + IKE_HDR_LEN,
                ph1->ike_params->ike_packet->ike_data_len - IKE_HDR_LEN,
                NU_NULL,
                ph1->ike_sa->ike_decryption_iv,
                ph1->ike_sa->ike_encryption_iv, IKE_ENCRYPT);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Receive and verify Authentication data from
             * the Initiator.
             */
            status = IKE_Aggr_4_Main_6_7_Recv(ph1);

            if(status == NU_SUCCESS)
            {
                /* If the message is encrypted. */
                if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) != 0)
                {
                    /* Since the message is authenticated, the
                     * decryption IV can now be synchronized.
                     */
                    IKE_SYNC_IV(ph1->ike_sa->ike_decryption_iv,
                                ph1->ike_sa->ike_encryption_iv,
                                ph1->ike_sa);
                }

                /* Phase 1 completed. */
                ph1->ike_xchg_state = IKE_COMPLETE_STATE;
            }

            else
            {
                NLOG_Error_Log("Unable to decode/verify incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to decrypt incoming message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Aggr_State_4 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Aggr_Mode
*
* DESCRIPTION
*
*       This function handles the IKE Aggressive Mode negotiation
*       in Phase 1 exchange.
*
*       First state of Initiator: packet, isakmp header should be NULL.
*       First state of Responder: SA should be locally allocated.
*
* INPUTS
*
*       *ph1                    Pointer to Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_SA_NOT_FOUND        SA was expected but not passed.
*       IKE_IS_RESEND           Message is a retransmission.
*       Exchange Status         If the state handler was called,
*                               status of the current state
*                               processing is returned.
*
*************************************************************************/
STATUS IKE_Process_Aggr_Mode(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status = NU_SUCCESS;
    IKE_STATE_PARAMS    *params;
    UINT8               state;

#if (IKE_DEBUG == NU_TRUE)
    /* Check that the Handle pointer is not NULL. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be passed to this function. */
    else if(ph1->ike_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the policy pointer is valid. */
    else if(ph1->ike_params->ike_policy == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the SA pointer is valid. */
    else if(ph1->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        return (IKE_SA_NOT_FOUND);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("AGGRESSIVE MODE");

    /* Set local pointer to commonly used data in the Handle. */
    params = ph1->ike_params;

    /* If the header is not NULL. */
    if(params->ike_in.ike_hdr != NU_NULL)
    {
        /* Check if authentication bit set in ISAKMP header. */
        if((params->ike_in.ike_hdr->ike_flags & IKE_HDR_AUTH_MASK) != 0)
        {
            NLOG_Error_Log("Invalid flags in ISAKMP header",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Authentication only is not allowed in Aggressive mode. */
            status = IKE_INVALID_FLAGS;
        }

        /* Message ID must be zero in Phase 1. */
        else if(params->ike_in.ike_hdr->ike_msg_id != 0)
        {
            NLOG_Error_Log("Non-zero message ID in phase 1",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Report invalid message ID. */
            status = IKE_INVALID_MSGID;
        }

        else
        {
            /* Make sure message is not a retransmission. */
            status = IKE_Check_Resend(ph1);
        }
    }

    /* Make sure no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Get the current state from the Handle. */
        state = ph1->ike_xchg_state;

        /* Make sure state is not already complete. */
        if(state == IKE_COMPLETE_STATE)
        {
            NLOG_Error_Log(
                "Unexpected message - IKE SA is already established",
                NERR_RECOVERABLE, __FILE__, __LINE__);

            /* SA is established. Unexpected message received. */
            status = IKE_UNEXPECTED_MESSAGE;
        }

        else
        {
            /* Call the current state handler. */
            status = IKE_Aggr_Mode_Handlers[(INT)state - 1](ph1);

            /* If the exchange has failed. */
            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Aggressive Mode failed - aborting",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                /* Update status of all phase 2 exchanges waiting
                 * for the current phase 1 exchange to complete.
                 */
                if(IKE_Update_Phase1_Status(ph1->ike_sa, status)
                   == NU_SUCCESS)
                {
                    IKE_DEBUG_LOG("Updated Phase 1 failure status");
                }
#endif

                /* Make sure this is not the local copy of the
                 * SA variable passed to State 2 of the state machine.
                 */
                if(ph1->ike_sa != &IKE_Large_Data.ike_sa)
                {
                    /* Make sure Handle pointer is valid. */
                    if(ph1->ike_sa->ike_phase1 != NU_NULL)
                    {
                        /* Mark the Handle as deleted. */
                        ph1->ike_sa->ike_phase1->ike_flags |=
                            IKE_DELETE_FLAG;
                    }

                    /* Enqueue event to remove the IKE SA. The SA is
                     * removed through the event queue to keep all
                     * events synchronized.
                     *
                     * Note that this event will not be processed
                     * until the current packet is being handled
                     * because the IKE semaphore is obtained. Therefore
                     * the Phase 2 Handle could be accessed by the
                     * caller when this function returns.
                     */
                    IKE_SYNC_REMOVE_SA(&params->ike_policy->ike_sa_list,
                                       ph1->ike_sa);
                }

                /* Otherwise this is the local copy of the SA. */
                else
                {
                    /* Deallocate all dynamically allocated memory
                     * because the state machine is responsible for
                     * the IKE SA as long as it is not added to the
                     * SADB.
                     */
                    if(IKE_Free_Local_SA(ph1->ike_sa) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to deallocate local SA",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_Aggr_Mode */

#endif /* (IKE_INCLUDE_AGGR_MODE == NU_TRUE) */

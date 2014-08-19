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
*       ike_main.c
*
* COMPONENT
*
*       IKE - Main Mode
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE
*       Main Mode Exchange.
*
* DATA STRUCTURES
*
*       IKE_Main_Mode_Handlers  Main mode state handlers.
*
* FUNCTIONS
*
*       IKE_Main_State_1
*       IKE_Main_State_2
*       IKE_Main_State_3
*       IKE_Main_State_4
*       IKE_Main_State_5
*       IKE_Main_State_5_6_Send
*       IKE_Main_State_6
*       IKE_Main_State_7
*       IKE_Process_Main_Mode
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
*       ike_crypto_wrappers.h
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
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)

/* Local function prototypes. */
STATIC STATUS IKE_Main_State_1(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_2(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_3(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_4(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_5(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_5_6_Send(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_6(IKE_PHASE1_HANDLE *ph1);
STATIC STATUS IKE_Main_State_7(IKE_PHASE1_HANDLE *ph1);

/* Main Mode state handlers. */
static
IKE_MAIN_MODE_FUNC IKE_Main_Mode_Handlers[IKE_TOTAL_MAIN_MODE_STATES] =
{
    IKE_Main_State_1,
    IKE_Main_State_2,
    IKE_Main_State_3,
    IKE_Main_State_4,
    IKE_Main_State_5,
    IKE_Main_State_6,
    IKE_Main_State_7
};

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_1
*
* DESCRIPTION
*
*       This function implements Main Mode state 1 of the
*       IKE state machine. It initiates an IKE Phase 1
*       Exchange on behalf of the Initiator. An un-negotiated
*       and incomplete IKE SA must be added to the SADB
*       before this function is called. This SA and the IKE
*       Policy that applies to it must be passed as parameters.
*       The function generates an SA proposal using the
*       Policy and sends it to the Responder. The Responder
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
*       Initiator*                      Responder
*       ---------                       ---------
*       HDR, SA                 -->
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
STATIC STATUS IKE_Main_State_1(IKE_PHASE1_HANDLE *ph1)
{
    STATUS          status;
    IKE_ENC_MESSAGE *out;
    IKE_ENC_HDR     enc_hdr;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Main mode: state 1");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;

    /* Make sure packet pointer is NULL. */
    if(ph1->ike_params->ike_packet != NU_NULL)
    {
        /* Set error code. */
        status = IKE_UNEXPECTED_MESSAGE;
    }

    else
    {
        /* Set outbound payloads used in this state. */
        IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(out->ike_sa, &IKE_Large_Data.ike_enc_sa);

        /* Initialize chain of outbound payloads. */
        IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

        /* Generate SA payload proposal using the Policy. */
        status = IKE_Construct_Proposal(ph1);

        if(status == NU_SUCCESS)
        {
            /* Terminate chain of payloads. */
            IKE_END_CHAIN(out->ike_last);

            /* Initialize ISAKMP header. */
            IKE_Set_Header(&enc_hdr, 0, ph1->ike_sa->ike_cookies,
                           IKE_XCHG_MAIN, 0);

            /* Send the message. */
            status = IKE_Send_Packet(ph1);

            if(status == NU_SUCCESS)
            {
                /* Save the SAi_b in the Handle. */
                status = IKE_Extract_Raw_Payload(ph1->ike_last_message,
                                                 &ph1->ike_sa_b,
                                                 &ph1->ike_sa_b_len,
                                                 IKE_SA_PAYLOAD_ID);

                if(status == NU_SUCCESS)
                {
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
                        NLOG_Error_Log(
                            "Failed to set exchange timeout timer",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to extract raw SA payload",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to send IKE packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to construct IKE proposal",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Main_State_1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_2
*
* DESCRIPTION
*
*       This function implements Main Mode state 2 of the
*       IKE state machine. It handles the first Main mode
*       message received by the Responder. The message
*       contains an SA payload containing the Initiator's
*       proposal. A transform from this proposal is
*       selected, based on the Policy, and a reply is sent
*       to the initiator, containing the selection.
*
*       As this is the first message to the Responder, the
*       IKE SA and Handle passed to this function are not
*       part of the database. This function adds the
*       incomplete IKE SA and the Handle to the SADB.
*
*       Following are payloads received/sent in this state:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR, SA                 -->
*                               <--     HDR, SA
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
*       IKE_INVALID_KEYLEN      Negotiated encryption key
*                               length is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in packet.
*       IKE_TOO_MANY_PROPOSALS  The SA should only contain a
*                               single proposal payload.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed limit.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_NOT_NEGOTIABLE      Specified SA no negotiable
*                               under the current policy.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_MISSING_ATTRIB      One or more of the required
*                               attributes was not negotiated.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_UNSUPPORTED_ALGO    Algorithm used in attribute is
*                               not supported by IKE.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_NOT_FOUND           Pre-shared key not found.
*       IKE_DUPLICATE_PAYLOAD   Payload decoded already.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_2(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status;
    IKE_DEC_MESSAGE     *in;
    IKE_ENC_MESSAGE     *out;
    IKE_STATE_PARAMS    *params;
    IKE_ENC_HDR         enc_hdr;
    IKE_ATTRIB          *policy_attrib;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Main mode: state 2");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph1->ike_params->ike_in;
    out    = &ph1->ike_params->ike_out;
    params = ph1->ike_params;

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
        /* Set inbound payloads used in this state. */
        IKE_SET_INBOUND_REQUIRED(in->ike_sa, &IKE_Large_Data.ike_dec_sa);

        /* Set outbound payloads used in this state. */
        IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(out->ike_sa, &IKE_Large_Data.ike_enc_sa);

        /* Initialize chain of outbound payloads. */
        IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

        /* Decode all payloads in the packet. */
        status = IKE_Decode_Message(params->ike_packet->ike_data, in);

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
                                                 ph1->ike_sa, NU_NULL);

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

                            /* Terminate chain of payloads. */
                            IKE_END_CHAIN(out->ike_last);

                            /* Initialize ISAKMP header. */
                            IKE_Set_Header(&enc_hdr, 0,
                                           ph1->ike_sa->ike_cookies,
                                           IKE_XCHG_MAIN,
                                           in->ike_hdr->ike_flags);

                            /* Send the message. */
                            status = IKE_Send_Packet(ph1);

                            if(status == NU_SUCCESS)
                            {
                                /* Update hash of last message received. */
                                IKE_UPDATE_RESEND(ph1);

                                /* Save the SAi_b in the Handle. */
                                status = IKE_Extract_Raw_Payload(
                                             params->ike_packet->ike_data,
                                             &ph1->ike_sa_b,
                                             &ph1->ike_sa_b_len,
                                             IKE_SA_PAYLOAD_ID);

                                if(status == NU_SUCCESS)
                                {
                                    /* Add phase 1 timeout event. */
                                    status = IKE_Set_Timer(
                                        IKE_Phase1_Timeout_Event,
                                        (UNSIGNED)ph1->ike_sa,
                                        (UNSIGNED)&params->ike_policy->
                                            ike_sa_list,
                                        IKE_PHASE1_TIMEOUT);

                                    if(status == NU_SUCCESS)
                                    {
                                        /* Increment the machine state. */
                                        ph1->ike_xchg_state +=
                                            IKE_NEXT_STATE_INC;
                                    }

                                    else
                                    {
                                        NLOG_Error_Log(
                                            "Failed to set exchange timer",
                                            NERR_RECOVERABLE, __FILE__,
                                            __LINE__);
                                    }
                                }

                                else
                                {
                                    NLOG_Error_Log(
                                        "Unable to extract raw SA payload",
                                        NERR_RECOVERABLE, __FILE__,
                                        __LINE__);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log("Failed to send IKE packet",
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
    }

    /* Return the status. */
    return (status);

} /* IKE_Main_State_2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_3
*
* DESCRIPTION
*
*       This function implements Main Mode state 3 of the
*       IKE state machine. It is executed by the Initiator.
*       A response to the SA proposal sent in State 1 is
*       received and verified against the Policy to make
*       sure the proposal has not been tampered. Public key
*       data for the IKE key exchange is then sent to the
*       Responder.
*
*       Following are payloads received/sent in this state:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR, SA
*       HDR, KE, Ni             -->
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
*       IKE_SA_NOT_FOUND        SA was expected but not passed.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_INVALID_PROTOCOL    Protocol in proposal is not ISAKMP.
*       IKE_INVALID_TRANSFORM   Transform ID is not valid.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed limit.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_INVALID_PAYLOAD     SA payload is invalid.
*       IKE_MISSING_ATTRIB      One or more of the required
*                               attributes are missing in the
*                               negotiation.
*       IKE_UNSUPPORTED_ATTRIB  An unrecognized attribute
*                               was encountered.
*       IKE_UNSUPPORTED_ALGO    Algorithm used in attribute is
*                               not supported by IKE.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_NOT_FOUND           Pre-shared key not found.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_3(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_ENC_MESSAGE         *out;
    IKE_STATE_PARAMS        *params;
    IKE_ENC_HDR             enc_hdr;
    IKE_KEYXCHG_ENC_PAYLOAD enc_key;
    IKE_NONCE_ENC_PAYLOAD   enc_nonce;
    IKE_ATTRIB              *policy_attrib;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Main mode: state 3");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph1->ike_params->ike_in;
    out    = &ph1->ike_params->ike_out;
    params = ph1->ike_params;

    /* Make sure the SA pointer is valid. */
    if(ph1->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        status = IKE_SA_NOT_FOUND;
    }

    /* Make sure first payload is an SA. */
    else if(in->ike_hdr->ike_first_payload != IKE_SA_PAYLOAD_ID)
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
        /* Set inbound payloads used in this state. */
        IKE_SET_INBOUND_REQUIRED(in->ike_sa, &IKE_Large_Data.ike_dec_sa);

        /* Decode all payloads in the packet. */
        status = IKE_Decode_Message(params->ike_packet->ike_data, in);

        if(status == NU_SUCCESS)
        {
            /* Make sure proposal in SA payload is valid. */
            status = IKE_Verify_Selection_SA(&IKE_Large_Data.ike_dec_sa);

            if(status == NU_SUCCESS)
            {
                /* Convert the attributes from payload format
                 * to the internal representation. Also fill in
                 * the default values of those attributes which
                 * were not negotiated.
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
                        /* Also set the Authentication method
                         * specific attributes in the SA.
                         */
                        status = IKE_Auth_Parameters(policy_attrib,
                                                     ph1->ike_sa, NU_NULL);

                        if(status != NU_SUCCESS)
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
                    /* Conversion should have succeeded if the
                     * selection was correct. Failure implies
                     * that the proposal was not the same as
                     * the one sent earlier.
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
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Set outbound payloads used in this state. */
        IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(out->ike_key, &enc_key);
        IKE_SET_OUTBOUND(out->ike_nonce, &enc_nonce);

        /* Initialize chain of outbound payloads. */
        IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

        /* Copy the Responder's cookie to the SA. */
        NU_BLOCK_COPY(&ph1->ike_sa->ike_cookies[IKE_COOKIE_LEN],
                      in->ike_hdr->ike_rcookie, IKE_COOKIE_LEN);

        /* Generate the Key Exchange data for the Responder and
         * add its payload to the outgoing chain. This would also
         * generate the Diffie-Hellman key pair and store it in
         * the Handle.
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
                /* Terminate chain of payloads. */
                IKE_END_CHAIN(out->ike_last);

                /* Initialize ISAKMP header. */
                IKE_Set_Header(&enc_hdr, 0, ph1->ike_sa->ike_cookies,
                               IKE_XCHG_MAIN, in->ike_hdr->ike_flags);

                /* Send the message. */
                status = IKE_Send_Packet(ph1);

                if(status == NU_SUCCESS)
                {
                    /* Update hash of last message received. */
                    IKE_UPDATE_RESEND(ph1);

                    /* Increment the machine state. */
                    ph1->ike_xchg_state += IKE_NEXT_STATE_INC;
                }

                else
                {
                    NLOG_Error_Log("Failed to send IKE packet",
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

    /* Return the status. */
    return (status);

} /* IKE_Main_State_3 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_4
*
* DESCRIPTION
*
*       This function implements Main Mode state 4 of the
*       IKE state machine. State 4 is executed by the Responder.
*       It is responsible for receiving the Key Exchange data
*       sent by the Initiator, sending the Responder's Key
*       Exchange data and generating the Diffie-Hellman shared
*       secret based on the Key Exchange data.
*
*       Following are payloads received/sent in this state:
*
*       PSK Auth:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR, KE, Ni             -->
*                               <--     HDR, KE, Nr
*
*       SIG Auth:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR, KE, Ni             -->
*                               <--     HDR, KE, Nr [,CREQ]
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_SA_NOT_FOUND        SA was expected but not passed.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_4(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_ENC_MESSAGE         *out;
    IKE_KEYXCHG_DEC_PAYLOAD dec_key;
    IKE_NONCE_DEC_PAYLOAD   dec_nonce;
    IKE_ENC_HDR             enc_hdr;
    IKE_KEYXCHG_ENC_PAYLOAD enc_key;
    IKE_NONCE_ENC_PAYLOAD   enc_nonce;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERTREQ_ENC_PAYLOAD cert_req;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Main mode: state 4");

    /* Set local pointers to commonly used data in the Handle. */
    in  = &ph1->ike_params->ike_in;
    out = &ph1->ike_params->ike_out;

    /* Make sure the SA pointer is valid. */
    if(ph1->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        status = IKE_SA_NOT_FOUND;
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
        /* Set inbound payloads used in this state. */
        IKE_SET_INBOUND_REQUIRED(in->ike_key, &dec_key);
        IKE_SET_INBOUND_REQUIRED(in->ike_nonce, &dec_nonce);

        /* Decode all payloads in the packet. */
        status = IKE_Decode_Message(ph1->ike_params->ike_packet->ike_data,
                                    in);

        if(status == NU_SUCCESS)
        {
            /* Allocate buffer for remote Diffie-Hellman public key. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&ph1->ike_dh_remote_key,
                                        in->ike_key->ike_keyxchg_data_len,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Normalize the pointer. */
                ph1->ike_dh_remote_key =
                    TLS_Normalize_Ptr(ph1->ike_dh_remote_key);

                /* Copy remote Diffie-Hellman public key to Handle. */
                NU_BLOCK_COPY(ph1->ike_dh_remote_key,
                              in->ike_key->ike_keyxchg_data,
                              in->ike_key->ike_keyxchg_data_len);

                /* Also set public key length in Handle. */
                ph1->ike_dh_remote_key_len =
                    in->ike_key->ike_keyxchg_data_len;
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for DH key",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to decode incoming message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Set outbound payloads used in this state. */
        IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(out->ike_key, &enc_key);
        IKE_SET_OUTBOUND(out->ike_nonce, &enc_nonce);
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        /* If policy specifies to send CERT-REQ. */
        if((ph1->ike_params->ike_policy->ike_flags & IKE_INBAND_CERT_XCHG)
            != 0)
        {
            IKE_SET_OUTBOUND(out->ike_certreq, &cert_req);
        }
#endif

        /* Initialize chain of outbound payloads. */
        IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

        /* Generate the Key Exchange data for the Responder and
         * add its payload to the outgoing chain. This would also
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
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

                /* If In-Band certificate exchange is enabled and
                 * authentication method is digital signatures. */
                if(((ph1->ike_params->ike_policy->ike_flags &
                    IKE_INBAND_CERT_XCHG) != 0) &&
                    (ph1->ike_sa->ike_attributes.ike_auth_method
                     == IKE_RSA))
                {
                    /* Generate the CERTREQ payload data. */
                    status = IKE_Generate_CertReq_Data(ph1);
                }

                if(status == NU_SUCCESS)
                {
#endif /*(IKE_INCLUDE_SIG_AUTH == NU_TRUE)*/
                    /* Terminate chain of payloads. */
                    IKE_END_CHAIN(out->ike_last);

                    /* Initialize ISAKMP header. */
                    IKE_Set_Header(&enc_hdr, 0, ph1->ike_sa->ike_cookies,
                        IKE_XCHG_MAIN, in->ike_hdr->ike_flags);

                    /* Send the message. */
                    status = IKE_Send_Packet(ph1);

                    if(status == NU_SUCCESS)
                    {
                        /* Compute the Phase 1 key material. This is
                         * a VERY CPU intensive operation.
                         */
                        status = IKE_Phase1_Key_Material(ph1);

                        if(status == NU_SUCCESS)
                        {
                            /* Update hash of last message received. */
                            IKE_UPDATE_RESEND(ph1);

                            /* Increment the machine state. */
                            ph1->ike_xchg_state += IKE_NEXT_STATE_INC;
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
                        NLOG_Error_Log("Failed to send IKE packet",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                }
                else
                {
                    NLOG_Error_Log("Failed to generate Cert-Req payload",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                }
#endif /*(IKE_INCLUDE_SIG_AUTH == NU_TRUE)*/
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

    /* Return the status. */
    return (status);

} /* IKE_Main_State_4 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_5
*
* DESCRIPTION
*
*       This function implements Main Mode state 5 of the
*       IKE state machine. State 5 is executed by the Initiator.
*       It is responsible for receiving the Key Exchange data
*       sent by the Responder and generating the Diffie-Hellman
*       shared secret based on it. The Identification and Hash
*       or Signature payloads are then sent to the Responder,
*       after encryption.
*
*       Following are payloads received/sent in this state:
*
*       PSK Auth:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR, KE, Nr
*       HDR*, IDii, HASH_I      -->
*
*       SIG Auth:
*
*       Initiator*                                  Responder
*       ---------                                   ---------
*                                           <--     HDR, KE, Nr [,CREQ]
*       HDR*, IDii [,CREQ] [,CERT], SIG_I   -->
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
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_SA_NOT_FOUND        SA was expected but not passed.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_UNSUPPORTED_IDTYPE  Identifier not supported.
*       IKE_UNSUPPORTED_METHOD  Authentication method is not
*                               supported.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_5(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    UINT8                   msg_hash[IKE_MD5_DIGEST_LEN];
    IKE_DEC_MESSAGE         *in;
    IKE_KEYXCHG_DEC_PAYLOAD dec_key;
    IKE_NONCE_DEC_PAYLOAD   dec_nonce;

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
    IKE_DEBUG_LOG("Initiator processing Main mode: state 5");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

    /* Make sure the SA pointer is valid. */
    if(ph1->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        status = IKE_SA_NOT_FOUND;
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
        /* Set inbound payloads used in this state. */
        IKE_SET_INBOUND_REQUIRED(in->ike_key, &dec_key);
        IKE_SET_INBOUND_REQUIRED(in->ike_nonce, &dec_nonce);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        if((ph1->ike_params->ike_policy->ike_flags
             & IKE_INBAND_CERT_XCHG) != 0)
        {
            IKE_SET_INBOUND_REQUIRED(in->ike_certreq, &dec_certreq);
        }
#endif

        /* Decode all payloads in the packet. */
        status = IKE_Decode_Message(ph1->ike_params->ike_packet->ike_data,
                                    in);
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        if(IKE_PAYLOAD_IS_PRESENT(in->ike_certreq) == NU_TRUE)
        {
            switch(in->ike_certreq->ike_cert_type)
            {

            case IKE_CERT_ENCODING_X509_SIG:
                ph1->ike_flags |= IKE_CERTREQ_RECEIVED;
                break;

            default:
                NLOG_Error_Log(
                    "Requested CERT type not valid or not supported",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
                break;

            }

        }
#endif

        if(status == NU_SUCCESS)
        {
            /* Allocate buffer for remote Diffie-Hellman public key. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&ph1->ike_dh_remote_key,
                                        dec_key.ike_keyxchg_data_len,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Normalize the pointer. */
                ph1->ike_dh_remote_key =
                    TLS_Normalize_Ptr(ph1->ike_dh_remote_key);

                /* Copy remote Diffie-Hellman public key to SA. */
                NU_BLOCK_COPY(ph1->ike_dh_remote_key,
                              dec_key.ike_keyxchg_data,
                              dec_key.ike_keyxchg_data_len);

                /* Also set public key length in SA. */
                ph1->ike_dh_remote_key_len = dec_key.ike_keyxchg_data_len;

                /* Compute the Phase 1 key material. This is
                 * a VERY CPU intensive operation.
                 */
                status = IKE_Phase1_Key_Material(ph1);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to calculate Phase 1 keys",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for DH key",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to decode incoming message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Calculate hash of the encrypted message. */
        IKE_HASH_ENC_RESEND(ph1, msg_hash);

        /* Send Authentication data to Responder. */
        status = IKE_Main_State_5_6_Send(ph1);

        if(status == NU_SUCCESS)
        {
            /* Update hash of last message received. */
            IKE_UPDATE_ENC_RESEND(ph1, msg_hash);

            /* Increment the machine state. */
            ph1->ike_xchg_state += IKE_NEXT_STATE_INC;
        }

        else
        {
            NLOG_Error_Log("Failed to generate IKE message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Main_State_5 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_5_6_Send
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to send Authentication data to the other party. It is
*       called only in Phase 1 Main mode. The Identification and
*       Hash (or Signature) payloads are encrypted before
*       being sent.
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
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_LENGTH_IS_SHORT     Message buffer not large enough.
*       IKE_INVALID_PAYLOAD     SA payload is invalid.
*       IKE_INVALID_DOMAIN      Domain name in Policy is invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier not supported.
*       IKE_UNSUPPORTED_METHOD  Authentication method is not
*                               supported.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_5_6_Send(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status;
    IKE_ENC_MESSAGE             *out;
    IKE_ENC_HDR                 enc_hdr;
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
    IKE_DEBUG_LOG("Sending authentication data");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;

    /* Initialize signature data pointer. */
    enc_sig.ike_signature_data = NU_NULL;

    /* Set outbound payloads used in this state. */
    IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
    IKE_SET_OUTBOUND(out->ike_id_i, &IKE_Large_Data.ike_enc_idi);
    IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);
    IKE_SET_OUTBOUND(out->ike_sig, &enc_sig);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(((ph1->ike_params->ike_policy->ike_flags & IKE_INBAND_CERT_XCHG)
        != 0) && (ph1->ike_xchg_state == 5) &&
        (ph1->ike_sa->ike_attributes.ike_auth_method == IKE_RSA))
    {
        IKE_SET_OUTBOUND(out->ike_certreq, &enc_certreq);
    }
    if((((ph1->ike_params->ike_policy->ike_flags &
        IKE_SEND_CERT_PROACTIVELY) != 0)||
        ((ph1->ike_flags & IKE_CERTREQ_RECEIVED) != 0))
        && (ph1->ike_sa->ike_attributes.ike_auth_method == IKE_RSA))
    {
        IKE_SET_OUTBOUND(out->ike_cert, &enc_cert);
    }

#endif

    /* Initialize chain of outbound payloads. */
    IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

    /* Add Identification payload to the chain. */
    status = IKE_Generate_ID_Data(ph1);

    if(status == NU_SUCCESS)
    {
        /* Temporarily terminate chain of payloads to encode
         * the ID payload only.
         */
        IKE_END_CHAIN(out->ike_last);

        /* Encode the Identification payload as the encoded
         * payload is used to calculate HASH_I and HASH_R.
         */
        status = IKE_Encode_ID_Payload(IKE_Large_Data.ike_id.ike_raw_id,
                     (UINT16)sizeof(IKE_Large_Data.ike_id.ike_raw_id),
                     out->ike_id_i);

        if(status == NU_SUCCESS)
        {
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
            if(((ph1->ike_params->ike_policy->ike_flags &
                IKE_INBAND_CERT_XCHG) != 0) && (ph1->ike_xchg_state == 5)
                && (ph1->ike_sa->ike_attributes.ike_auth_method
                    == IKE_RSA))
            {
                /* Generate Cert-Req payload to be sent. */
                status = IKE_Generate_CertReq_Data(ph1);
            }
            if(status == NU_SUCCESS)
            {
                if((((ph1->ike_params->ike_policy->ike_flags &
                    IKE_SEND_CERT_PROACTIVELY) != 0) ||
                    ((ph1->ike_flags & IKE_CERTREQ_RECEIVED) != 0)) &&
                    (ph1->ike_sa->ike_attributes.ike_auth_method ==
                    IKE_RSA))
                {
                    status = IKE_Generate_Cert_Data(ph1);
                }
                if(status == NU_SUCCESS)
                {
#endif
                    /* Add the Signature or Hash payload to the chain. */
                    status = IKE_Generate_Hash_Data(ph1,
                        IKE_Large_Data.ike_id.ike_raw_id +
                        IKE_GEN_HDR_LEN,
                        out->ike_id_i->ike_hdr.ike_payload_len -
                        IKE_GEN_HDR_LEN);

                    if(status == NU_SUCCESS)
                    {
                        /* Terminate chain of payloads. */
                        IKE_END_CHAIN(out->ike_last);

                        /* Initialize ISAKMP header. */
                        IKE_Set_Header(&enc_hdr, 0,
                            ph1->ike_sa->ike_cookies, IKE_XCHG_MAIN,
                            (ph1->ike_params->ike_in.ike_hdr->ike_flags
                            | IKE_HDR_ENC_MASK));

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
                            if(NU_Deallocate_Memory(enc_sig.ike_signature_data)
                               != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Unable to deallocate Signature",
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
                    NLOG_Error_Log("Failed to encode Cert payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to encode Cert-Req payload",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
#endif
        }

        else
        {
            NLOG_Error_Log("Failed to encode ID payload for hashing",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to generate ID payload",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Main_State_5_6_Send */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_6
*
* DESCRIPTION
*
*       This function implements Main Mode state 6 of the
*       IKE state machine. State 6 is executed by the Responder
*       and it authenticates the exchange using either a
*       Pre-shared Key or Signatures. It is responsible for
*       receiving the Authentication data sent by the Initiator,
*       verifying this data and then sending encrypted
*       Authentication data of the Responder, in response.
*       The message received by this function must be encrypted.
*
*       Following are payloads received/sent in this state:
*
*       PSK Auth:
*
*       Initiator                   Responder*
*       ---------                   ---------
*       HDR*, IDii, HASH_I  -->
*                           <--     HDR*, IDir, HASH_R
*
*       SIG Auth:
*
*       Initiator                                Responder*
*       ---------                                ---------
*       HDR*, IDii [,CREQ] [,CERT], SIG_I   -->
*                                           <--  HDR*, IDir [,CERT], SIG_R
*
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
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_SA_NOT_FOUND        SA was expected but not passed.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_DOMAIN      Domain name in Policy is invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier not supported.
*       IKE_UNSUPPORTED_METHOD  Authentication method is not
*                               supported.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_6(IKE_PHASE1_HANDLE *ph1)
{
    STATUS          status;
    UINT8           msg_hash[IKE_MD5_DIGEST_LEN];
    IKE_DEC_MESSAGE *in;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Main mode: state 6");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

    /* Make sure the SA pointer is valid. */
    if(ph1->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        status = IKE_SA_NOT_FOUND;
    }

    /* This message must be encrypted. */
    else if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) == 0)
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
        /* Calculate hash of the encrypted message. */
        IKE_HASH_ENC_RESEND(ph1, msg_hash);

        /* Decrypt the message. */
        status = IKE_Encrypt(ph1->ike_sa,
            ph1->ike_params->ike_packet->ike_data + IKE_HDR_LEN,
            ph1->ike_params->ike_packet->ike_data_len - IKE_HDR_LEN,
            NU_NULL, ph1->ike_sa->ike_decryption_iv,
            ph1->ike_sa->ike_encryption_iv, IKE_DECRYPT);

        if(status == NU_SUCCESS)
        {
            /* Receive and verify Authentication data from
             * the Initiator.
             */
            status = IKE_Aggr_4_Main_6_7_Recv(ph1);

            if(status == NU_SUCCESS)
            {
                /* Since the message is authenticated, the
                 * decryption IV can now be synchronized.
                 */
                IKE_SYNC_IV(ph1->ike_sa->ike_decryption_iv,
                            ph1->ike_sa->ike_encryption_iv, ph1->ike_sa);

                /* Send own Authentication data in response to the
                 * Initiator's Authentication data.
                 */
                status = IKE_Main_State_5_6_Send(ph1);

                if(status == NU_SUCCESS)
                {
                    /* SA established, so synchronize the
                     * encryption IV.
                     */
                    IKE_SYNC_IV(ph1->ike_sa->ike_encryption_iv,
                                ph1->ike_sa->ike_decryption_iv,
                                ph1->ike_sa);

                    /* Update hash of last message received. */
                    IKE_UPDATE_ENC_RESEND(ph1, msg_hash);

                    /* Phase 1 completed. */
                    ph1->ike_xchg_state = IKE_COMPLETE_STATE;
                }

                else
                {
                    NLOG_Error_Log("Unable to send IKE message",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to decode/verify incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to decrypt IKE message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Main_State_6 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Main_State_7
*
* DESCRIPTION
*
*       This function implements Main Mode state 7 of the
*       IKE state machine. State 7 is executed by the Initiator
*       and is specific to Authentication based on pre-shared key.
*       It is responsible for receiving the Authentication data
*       sent by the Responder and then verifying this data. The
*       message received by this function must be encrypted.
*
*       Following are payloads received in this state:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR*, IDir [,CERT], SIG_R
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
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_SA_NOT_FOUND        SA was expected but not passed.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      Responder cookie not set.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Main_State_7(IKE_PHASE1_HANDLE *ph1)
{
    STATUS          status;
    IKE_DEC_MESSAGE *in;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Main mode: state 7");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

    /* Make sure the SA pointer is valid. */
    if(ph1->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        status = IKE_SA_NOT_FOUND;
    }

    /* This message must be encrypted. */
    else if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) == 0)
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
        /* Decrypt the message. */
        status = IKE_Encrypt(ph1->ike_sa,
            ph1->ike_params->ike_packet->ike_data + IKE_HDR_LEN,
            ph1->ike_params->ike_packet->ike_data_len - IKE_HDR_LEN,
            NU_NULL, ph1->ike_sa->ike_decryption_iv,
            ph1->ike_sa->ike_encryption_iv, IKE_DECRYPT);

        if(status == NU_SUCCESS)
        {
            /* Receive and verify Authentication data from
             * the Initiator.
             */
            status = IKE_Aggr_4_Main_6_7_Recv(ph1);

            if(status == NU_SUCCESS)
            {
                /* Since the message is authenticated, the
                 * decryption IV can now be synchronized.
                 */
                IKE_SYNC_IV(ph1->ike_sa->ike_decryption_iv,
                            ph1->ike_sa->ike_encryption_iv, ph1->ike_sa);

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
            NLOG_Error_Log("Failed to decrypt IKE message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Main_State_7 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Main_Mode
*
* DESCRIPTION
*
*       This function handles the IKE Main Mode negotiation
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
STATUS IKE_Process_Main_Mode(IKE_PHASE1_HANDLE *ph1)
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

    /* Valid state must be passed to this function. */
    else if(ph1->ike_xchg_state == NU_NULL ||
            (ph1->ike_xchg_state > IKE_TOTAL_MAIN_MODE_STATES &&
             ph1->ike_xchg_state != IKE_COMPLETE_STATE))
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
    IKE_DEBUG_LOG("MAIN MODE");

    /* Set local pointers to commonly used data in the Handle. */
    params = ph1->ike_params;

    /* If the header is not NULL. */
    if(params->ike_in.ike_hdr != NU_NULL)
    {
        /* Check if authentication bit set in ISAKMP header. */
        if((params->ike_in.ike_hdr->ike_flags & IKE_HDR_AUTH_MASK) != 0)
        {
            NLOG_Error_Log("Invalid flags in ISAKMP header",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Authentication only is not allowed in Main mode. */
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
            status = IKE_Main_Mode_Handlers[(INT)state - 1](ph1);

            /* If the exchange has failed. */
            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Main Mode failed - aborting",
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

} /* IKE_Process_Main_Mode */

#endif /* (IKE_INCLUDE_MAIN_MODE == NU_TRUE) */

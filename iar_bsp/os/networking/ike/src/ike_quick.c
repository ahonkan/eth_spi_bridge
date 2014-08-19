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
*       ike_quick.c
*
* COMPONENT
*
*       IKE - Quick Mode
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE
*       Quick Mode Exchange.
*
* DATA STRUCTURES
*
*       IKE_Quick_Mode_Handlers     Quick mode state handlers.
*
* FUNCTIONS
*
*       IKE_Construct_Proposal2
*       IKE_Match_Transform2
*       IKE_Proposal_Span2
*       IKE_Select_Proposal2
*       IKE_Verify_Proposal2
*       IKE_Convert_Transform2
*       IKE_Generate_Nonce_Data2
*       IKE_Generate_KeyXchg_Data2
*       IKE_Generate_ID_Data2
*       IKE_Process_RSelection
*       IKE_Process_ISelection
*       IKE_Quick_State_1
*       IKE_Quick_State_2
*       IKE_Quick_State_2_3_Recv
*       IKE_Quick_State_2_Process
*       IKE_Quick_State_2_Send
*       IKE_Quick_State_3
*       IKE_Quick_State_3_Process
*       IKE_Quick_State_3_Send
*       IKE_Quick_State_4
*       IKE_Quick_State_5
*       IKE_Process_Quick_Mode
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*       ike_api.h
*       ike_pkt.h
*       ike_enc.h
*       ike_auth.h
*       ike_oak.h
*       ike_ips.h
*       ike_evt.h
*       ike_crypto_wrappers.h
*       rand.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_pkt.h"
#include "networking/ike_enc.h"
#include "networking/ike_auth.h"
#include "networking/ike_oak.h"
#include "networking/ike_ips.h"
#include "networking/ike_evt.h"
#include "networking/ike_crypto_wrappers.h"
#include "openssl/rand.h"

/* Local function prototypes. */
STATIC STATUS IKE_Construct_Proposal2(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Match_Transform2(IKE_PHASE2_HANDLE *ph2, UINT8 proto,
                                   IPSEC_SECURITY_PROTOCOL *security,
                                   IKE_TRANSFORM_PAYLOAD *transform);
UINT8 IKE_Proposal_Span2(UINT8 prop_index,
                                IPSEC_SECURITY_PROTOCOL *security,
                                UINT8 security_size);
STATIC STATUS IKE_Select_Proposal2(IKE_PHASE2_HANDLE *ph2,
                                   IPSEC_SECURITY_PROTOCOL *security,
                                   UINT8 security_size);
STATIC STATUS IKE_Verify_Proposal2(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Convert_Transform2(UINT8 proto,
                                     IKE_TRANSFORM_PAYLOAD *transform,
                                     IPSEC_SECURITY_PROTOCOL *security,
                                     IPSEC_SA_LIFETIME *sa_life);
STATIC STATUS IKE_Generate_Nonce_Data2(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Generate_KeyXchg_Data2(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Generate_ID_Data2(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Process_RSelection(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Process_ISelection(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_1(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_2(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_2_3_Recv(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_2_Process(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_2_Send(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_3(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_3_Process(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_3_Send(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_4(IKE_PHASE2_HANDLE *ph2);
STATIC STATUS IKE_Quick_State_5(IKE_PHASE2_HANDLE *ph2);

/* Quick Mode state handlers. */
static
IKE_QUICK_MODE_FUNC IKE_Quick_Mode_Handlers[IKE_TOTAL_QUICK_MODE_STATES] =
{
    IKE_Quick_State_1,
    IKE_Quick_State_2,
    IKE_Quick_State_3,
    IKE_Quick_State_4,
    IKE_Quick_State_5
};

/*************************************************************************
*
* FUNCTION
*
*       IKE_Construct_Proposal2
*
* DESCRIPTION
*
*       This is a utility function used by Phase 2 Initiator. It
*       takes a list of IPsec SAs being negotiated (SA2)
*       and constructs an SA payload from it. The SA payload is
*       suitable to initiate a negotiation. The Handle must contain
*       an SA2 item corresponding to each two-way IPsec SA
*       negotiation.
*
*       Following payloads are appended to the chain:
*       - IKE_SA_ENC_PAYLOAD
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   Invalid algorithm ID in security
*                               protocol.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed limit.
*       IKE_SA2_NOT_FOUND       SA2 Item not found in Handle.
*       IKE_INVALID_PROTOCOL    Protocol ID in security is not valid.
*
*************************************************************************/
STATIC STATUS IKE_Construct_Proposal2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status = IKE_SA2_NOT_FOUND;
    INT                     i;
    UINT8                   attrib_num;
    IKE_TRANSFORM_PAYLOAD   *out_transform;
    IKE_SA_ENC_PAYLOAD      *out_sa;
    IKE_SA2                 *sa2;
    IPSEC_SECURITY_PROTOCOL *security;
    UINT16                  key_len;
    UINT8                   algo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be present. */
    else if(ph2->ike_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Constructing Initiator's proposal");

    /* Set SA payload pointer. */
    out_sa = ph2->ike_params->ike_out.ike_sa;

    /* Set the SA2 pointer to first item in DB. */
    sa2 = ph2->ike_sa2_db.ike_flink;

    /* Loop for each SA2 item in the DB. An IKE
     * proposal payload would be generated for each
     * SA2 item. However, all proposal payloads
     * would be part of a single proposal (same proposal
     * ID of all proposal payloads) because the Nucleus
     * IPsec Policy does not currently handle multiple
     * proposals.
     */
    for(i = 0; sa2 != NU_NULL; i++)
    {
        /* Make sure there is room in the proposal. Note that
         * a similar check on transforms is not needed since
         * a minimum limit is specified in the header file.
         */
        if(i >= IKE_MAX_PROPOSALS)
        {
            NLOG_Error_Log("Number of proposals exceed IKE_MAX_PROPOSALS",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Set status to error and break out of loop. */
            status = IKE_TOO_MANY_PROPOSALS;
            break;
        }

        /* Set pointer to the IPsec security protocol. */
        security = &sa2->ike_ips_security;

        /* Initialize all fields of the current proposal. */
        out_sa->ike_proposals[i].ike_proposal_no =
            IKE_PHASE2_PROPOSAL_NUMBER;
        out_sa->ike_proposals[i].ike_num_transforms =
            IKE_PHASE2_NUM_TRANSFORMS;
        out_sa->ike_proposals[i].ike_protocol_id =
            IKE_Protocol_ID_IPS_To_IKE(security->ipsec_protocol);

        /* Copy the local SPI to the proposal. */
        out_sa->ike_proposals[i].ike_spi_len = IKE_IPS_SPI_LEN;
        PUT32(out_sa->ike_proposals[i].ike_spi, 0, sa2->ike_local_spi);

        /* Set pointer to the first (and only) transform payload. */
        out_transform = out_sa->ike_proposals[i].ike_transforms;

        /* Initialize attribute count. */
        attrib_num = 0;

        /* Check whether protocol is AH. */
        if(out_sa->ike_proposals[i].ike_protocol_id == IKE_PROTO_AH)
        {
            /* Set the AH transform ID. */
            out_transform->ike_transform_id =
                IKE_AH_Trans_ID_IPS_To_IKE(security->ipsec_auth_algo);
        }

        else if(out_sa->ike_proposals[i].ike_protocol_id == IKE_PROTO_ESP)
        {
            /* Set the ESP transform ID. */
            out_transform->ike_transform_id =
                IKE_ESP_Trans_ID_IPS_To_IKE(security->
                                            ipsec_encryption_algo);

            /* Get IPsec algorithm ID from security protocol. */
            algo = security->ipsec_encryption_algo;

            /* Grab the IPsec semaphore. */
            status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

            if(status == NU_SUCCESS)
            {
                /* Convert the ID to algorithm array index. */
                status = IPSEC_Get_Encrypt_Algo_Index(&algo);

                if(status == NU_SUCCESS)
                {
                    /* Get encryption algorithm key length from IPsec.
                     * This would be verified below if the (optional)
                     * key length attribute is negotiated.
                     */
                    key_len =
                        IPSEC_Encryption_Algos[(INT)algo].ipsec_key_len;

                    /* Make sure key length is not zero. */
                    if(key_len != 0)
                    {
                        /* Convert key length from bytes to bits. */
                        key_len = (UINT16)IKE_BYTES_TO_BITS(key_len);
                    }

                    /* Add encryption key length attribute. */
                    IKE_Add_Attribute(IKE_IPS_ATTRIB_KEY_LEN,
                                      out_transform->ike_sa_attributes,
                                      &attrib_num, key_len);
                }

                else
                {
                    NLOG_Error_Log(
                        "Unable to get encryption algorithm index",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Release the IPsec semaphore. */
                if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release the semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to obtain IPsec semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            /* Unrecognized protocol ID. */
            status = IKE_INVALID_PROTOCOL;
            break;
        }

        /* Add encapsulation mode attribute. */
        IKE_Add_Attribute(IKE_IPS_ATTRIB_ENCAP_MODE,
                          out_transform->ike_sa_attributes,
                          &attrib_num, security->ipsec_security_mode);

        /* If the group description is specified. */
        if(ph2->ike_group_desc != IKE_GROUP_NONE)
        {
            /* Add group description attribute. */
            IKE_Add_Attribute(IKE_IPS_ATTRIB_GROUP_DESC,
                              out_transform->ike_sa_attributes,
                              &attrib_num, ph2->ike_group_desc);
        }

#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
        /* If algorithm is not NULL authentication. */
        if(security->ipsec_auth_algo != IPSEC_NULL_AUTH)
        {
#endif
            /* Add authentication algorithm attribute. */
            IKE_Add_Attribute(IKE_IPS_ATTRIB_AUTH_ALGO,
                out_transform->ike_sa_attributes, &attrib_num,
                IKE_Auth_Algo_ID_IPS_To_IKE(security->ipsec_auth_algo));

#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
        }
#endif

        /* If lifetime specified in seconds. */
        if(ph2->ike_ips_lifetime.ipsec_no_of_secs != IPSEC_WILDCARD)
        {
            /* Add lifetime type attribute. */
            IKE_Add_Attribute(IKE_IPS_ATTRIB_LIFE_TYPE,
                              out_transform->ike_sa_attributes,
                              &attrib_num, IKE_IPS_VAL_SECS);

            /* Add lifetime duration attribute. */
            IKE_Add_Variable_Attribute(IKE_IPS_ATTRIB_LIFE_DURATION,
                out_transform->ike_sa_attributes, &attrib_num,
                ph2->ike_ips_lifetime.ipsec_no_of_secs,
                (UINT8*)&sa2->ike_attrib_secs_buffer);
        }

        /* Initialize remaining fields of transform payload. */
        out_transform->ike_num_attributes = attrib_num;
        out_transform->ike_transform_no   = IKE_PHASE2_TRANSFORM_NUMBER;

        /* Move to the next SA2 item. */
        sa2 = sa2->ike_flink;

        /* Set status to success since at least one proposal
         * payload has been added to the SA payload.
         */
        status = NU_SUCCESS;
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Initialize fields of the SA payload. */
        out_sa->ike_num_proposals = (UINT8)i;
        out_sa->ike_doi           = IKE_DOI_IPSEC;
        out_sa->ike_situation_len = IKE_IPS_SITUATION_LEN;

        /* Set situation to ID only. */
        PUT32(out_sa->ike_situation, 0, IKE_IPS_SIT_ID_ONLY);

        /* Add SA payload to the payloads chain. */
        IKE_ADD_TO_CHAIN(ph2->ike_params->ike_out.ike_last,
                         out_sa, IKE_SA_PAYLOAD_ID);
    }

    /* Return the status. */
    return (status);

} /* IKE_Construct_Proposal2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Match_Transform2
*
* DESCRIPTION
*
*       This is a utility function which checks whether the
*       specified Transform matches with the IPsec security
*       protocol.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       proto                   The IKE security protocol.
*       *security               Pointer to IPsec Security.
*       *transform              Transform being matched.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   Invalid algorithm ID in security
*                               protocol.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_TRANSFORM   Transform not supported.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_TRANSFORM_MISMATCH  Transform does not match Policy.
*
*************************************************************************/
STATIC STATUS IKE_Match_Transform2(IKE_PHASE2_HANDLE *ph2, UINT8 proto,
                                   IPSEC_SECURITY_PROTOCOL *security,
                                   IKE_TRANSFORM_PAYLOAD *transform)
{
    STATUS          status = NU_SUCCESS;
    IKE_DATA_ATTRIB *attrib;
    UINT32          attrib_val;
    INT             i;
    UINT16          key_len = IKE_WILDCARD;
    UINT8           ignore_lifetime = NU_FALSE;
    UINT8           algo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((ph2       == NU_NULL) || (security == NU_NULL) ||
       (transform == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Matching proposal's transform...");

    /* Make sure the transform ID is valid. */
    if(transform->ike_transform_id == 0)
    {
        /* The transform ID is not valid. */
        status = IKE_INVALID_TRANSFORM;
    }

    /* Check if protocol is AH. */
    else if(proto == IKE_PROTO_AH)
    {
        /* Make sure the specified transform ID matches. */
        if(transform->ike_transform_id !=
           IKE_AH_Trans_ID_IPS_To_IKE(security->ipsec_auth_algo))
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("Authentication algorithm mismatch (AH)");

            /* Transform ID mismatch. */
            status = IKE_TRANSFORM_MISMATCH;
        }
    }

    /* Otherwise would be ESP. Already checked by the caller. */
    else
    {
        /* Make sure the specified transform ID matches. */
        if(transform->ike_transform_id !=
           IKE_ESP_Trans_ID_IPS_To_IKE(security->ipsec_encryption_algo))
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("Encryption algorithm mismatch (ESP)");

            /* Transform ID mismatch. */
            status = IKE_TRANSFORM_MISMATCH;
        }

        else
        {
            /* Get IPsec algorithm ID from security protocol. */
            algo = security->ipsec_encryption_algo;

            /* Grab the IPsec semaphore. */
            status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

            if(status == NU_SUCCESS)
            {
                /* Convert the ID to algorithm array index. */
                status = IPSEC_Get_Encrypt_Algo_Index(&algo);

                if(status == NU_SUCCESS)
                {
                    /* Get encryption algorithm key length from IPsec.
                     * This would be verified below if the (optional)
                     * key length attribute is negotiated.
                     */
                    key_len =
                        IPSEC_Encryption_Algos[(INT)algo].ipsec_key_len;

                    /* Make sure key length is not zero. */
                    if(key_len != 0)
                    {
                        /* Convert key length from bytes to bits. */
                        key_len = (UINT16)IKE_BYTES_TO_BITS(key_len);
                    }
                }

                else
                {
                    NLOG_Error_Log(
                        "Failed to get encryption algorithm index",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Release the IPsec semaphore. */
                if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release the semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to obtain IPsec semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Loop for each attribute of this transform. */
        for(i = 0; i < transform->ike_num_attributes; i++)
        {
            /* Get pointer to current attribute. */
            attrib = &transform->ike_sa_attributes[i];

            /* Determine the attribute type. */
            switch(attrib->ike_attrib_type)
            {
            case IKE_IPS_ATTRIB_LIFE_TYPE:
                /* Make sure lifetime type is seconds. */
                if(attrib->ike_attrib_lenval == IKE_IPS_VAL_SECS)
                {
                    /* Do not ignore lifetime duration attribute. */
                    ignore_lifetime = NU_FALSE;
                }

                else
                {
                    /* Ignore lifetime duration attribute.
                     *
                     * NOTE: The IKE implementation of Nucleus IPsec
                     * does not support SA lifetime based on KB.
                     * However, if this attribute is specified in a
                     * proposal, the value is simply ignored to maintain
                     * interoperability with other implementations.
                     */
                    ignore_lifetime = NU_TRUE;
                }

                break;

            case IKE_IPS_ATTRIB_GROUP_DESC:
                /* Check if this group is supported. */
                if((ph2->ike_group_desc == IKE_GROUP_NONE) ||
                   (ph2->ike_group_desc != attrib->ike_attrib_lenval))
                {
                    /* Log debug message. */
                    IKE_DEBUG_LOG("PFS group mismatch");

                    /* Transform does not match. */
                    status = IKE_TRANSFORM_MISMATCH;
                }
                break;

            case IKE_IPS_ATTRIB_ENCAP_MODE:
                /* Check if the encapsulation mode matches. */
                if(security->ipsec_security_mode !=
                   attrib->ike_attrib_lenval)
                {
                    /* Log debug message. */
                    IKE_DEBUG_LOG(
                        "Security mode (transform/tunnel) mismatch");

                    /* Transform does not match. */
                    status = IKE_TRANSFORM_MISMATCH;
                }
                break;

            case IKE_IPS_ATTRIB_AUTH_ALGO:
                /* Check if policy allows this algorithm. */
                if(IKE_Auth_Algo_ID_IPS_To_IKE(security->ipsec_auth_algo)
                   != attrib->ike_attrib_lenval)
                {
                    /* Log debug message. */
                    IKE_DEBUG_LOG("Hash algorithm mismatch");

                    /* Transform does not match. */
                    status = IKE_TRANSFORM_MISMATCH;
                }
                break;

            case IKE_IPS_ATTRIB_KEY_LEN:
                /* Make sure key length is equal to the
                 * length supported by IPsec.
                 */
                if(key_len != attrib->ike_attrib_lenval)
                {
                    /* Log debug message. */
                    IKE_DEBUG_LOG("Encryption key length mismatch");

                    /* Transform does not match. */
                    status = IKE_TRANSFORM_MISMATCH;
                }
                break;

            default:
                /* The life duration attribute may have been
                 * encoded as either variable or a basic type.
                 */
                if((attrib->ike_attrib_type & IKE_ATTRIB_TYPE_MASK) ==
                   IKE_IPS_ATTRIB_LIFE_DURATION)
                {
                    /* If lifetime type is supported. */
                    if(ignore_lifetime == NU_FALSE)
                    {
                        /* Make sure value length is manageable. */
                        if(IKE_Decode_Attribute_Value(&attrib_val, attrib)
                           != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Lifetime mismatch (too large)",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

                            /* Value too large. */
                            status = IKE_TRANSFORM_MISMATCH;
                            break;
                        }

                        /* If lifetime specified in seconds. */
                        if(ph2->ike_ips_lifetime.ipsec_no_of_secs !=
                           IPSEC_WILDCARD)
                        {
                            /* Check if lifetime is greater than the
                             * maximum possible allowed by policy.
                             */
                            if(ph2->ike_ips_lifetime.ipsec_no_of_secs
                               < attrib_val)
                            {
                                /* Log debug message. */
                                IKE_DEBUG_LOG("Lifetime mismatch");

                                /* Lifetime value mismatch. */
                                status = IKE_TRANSFORM_MISMATCH;
                            }
                        }
                    }
                }

                else
                {
                    /* Report error. */
                    status = IKE_UNSUPPORTED_ATTRIB;
                }

                break;
            }

            /* Break out of loop if error occurred or if
             * an unacceptable proposal was encountered.
             */
            if(status != NU_SUCCESS)
            {
                break;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Match_Transform2 */

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Proposal_Span2
*
* DESCRIPTION
*
*       This is a utility function which returns the span of
*       the specified proposal. It is called by the Responder
*       to verify a selected proposal.
*
* INPUTS
*
*       prop_index              Index of one of the selected
*                               security protocols.
*       *security               An array of security protocols
*                               specified by the IPsec policy.
*       security_size           Number of securities in the array.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_NOT_NEGOTIABLE      Specified proposal not negotiable
*                               because it is incomplete.
*
*************************************************************************/
UINT8 IKE_Proposal_Span2(UINT8 prop_index,
                                IPSEC_SECURITY_PROTOCOL *security,
                                UINT8 security_size)
{
    UINT8           prop_span = 0;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the security pointer is valid. */
    if(security == NU_NULL)
    {
        return (prop_span);
    }
#endif

    /* Look for all security protocols in the array. */
    while(security_size > 0)
    {
        /* Decrement the security size. */
        security_size--;

        /* Increment current proposal span. */
        prop_span++;

        /* If the current security protocol is for tunnel mode. */
        if(security[security_size].ipsec_security_mode ==
           IPSEC_TUNNEL_MODE)
        {
            /* If current proposal is the selected one. */
            if(security_size <= (UINT8)prop_index)
            {
                /* Complete proposal traversed. Look no further. */
                break;
            }

            /* Otherwise, a new security suite is being
             * traversed now.
             */
            else
            {
                /* Reset proposal span. */
                prop_span = 0;
            }
        }
    }

    /* Return the proposal span. */
    return (prop_span);

} /* IKE_Proposal_Span2 */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_Select_Proposal2
*
* DESCRIPTION
*
*       This is a utility function used by Phase 2 Responder. It
*       takes an SA payload with multiple proposals and matches
*       those against the security protocol in the IPsec Policy.
*       If a match is found, an IKE SA payload suitable for a
*       reply to the negotiation is returned. SA2 items are also
*       added to the Handle in the order specified in the incoming
*       and outgoing proposal payloads. These items are initially
*       set to the values specified in the IPsec policy. They are
*       later overwritten with values from the actual proposal.
*
*       Note that variable length attributes of the outbound SA
*       payload share the same buffer space as those of the
*       inbound SA payload.
*
*       Following payloads are appended to the chain:
*       - IKE_SA_ENC_PAYLOAD
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       *security               Pointer to the IPsec Securities
*                               from the IPsec Policy.
*       security_size           Number of IPsec securities in
*                               the array.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   Invalid algorithm ID in security
*                               protocol.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PROTOCOL    Protocol is not supported.
*       IKE_INVALID_TRANSFORM   Transform not supported.
*       IKE_NOT_NEGOTIABLE      Proposal not negotiable using
*                               the specified policy.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_TRANSFORM_MISMATCH  Transform does not match Policy.
*
*************************************************************************/
STATIC STATUS IKE_Select_Proposal2(IKE_PHASE2_HANDLE *ph2,
                                   IPSEC_SECURITY_PROTOCOL *security,
                                   UINT8 security_size)
{
    STATUS                  status = NU_SUCCESS;
    UINT8                   i;
    UINT8                   j = 0;
    UINT8                   k;
    IKE_TRANSFORM_PAYLOAD   *in_transform = NU_NULL;
    IKE_TRANSFORM_PAYLOAD   *out_transform;
    IKE_DATA_ATTRIB         *in_attrib;
    IKE_DATA_ATTRIB         *out_attrib;
    IKE_SA_DEC_PAYLOAD      *in_sa;
    IKE_SA_ENC_PAYLOAD      *out_sa;
    UINT8                   is_match;
    UINT8                   prop_no;
    UINT8                   prop_span = 0;
    IKE_SA2                 sa2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((ph2 == NU_NULL) || (security == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure number of proposals is not zero. This is
     * verified in the payload decode function so is within
     * the IKE_DEBUG macro here.
     */
    else if(ph2->ike_params->ike_in.ike_sa->ike_num_proposals == 0)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Making selection from Initiator's proposal");

    /* Set local pointers to commonly used data in the Handle. */
    in_sa  = ph2->ike_params->ike_in.ike_sa;
    out_sa = ph2->ike_params->ike_out.ike_sa;

    /* Make sure number of securities is not zero. */
    if(security_size == 0)
    {
        /* No securities to match against. */
        status = IKE_NOT_NEGOTIABLE;
    }

    else
    {
        /* Set the proposal number to that of the first proposal. */
        prop_no = in_sa->ike_proposals->ike_proposal_no;

        /* Loop for each proposal in the input SA payload. */
        for(i = 0; i < in_sa->ike_num_proposals; i++)
        {
            /* If next proposal is encountered. */
            if(in_sa->ike_proposals[i].ike_proposal_no != prop_no)
            {
                /* Update current proposal number. */
                prop_no = in_sa->ike_proposals[i].ike_proposal_no;

                /* If previous proposal matched. */
                if(status == NU_SUCCESS)
                {
                    /* Check if matching proposal is complete. */
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
                    if(IKE_Proposal_Span2(j, security, security_size) ==
                       prop_span)
#else
                    if(security_size == prop_span)
#endif
                    {
                        /* Valid proposal found. End search. */
                        break;
                    }
                }

                /* Otherwise, proposal did not match.
                 * Set proposal payload span to zero.
                 */
                prop_span = 0;

                /* Remove previous SA2 items from database. */
                IKE_Flush_SA2(&ph2->ike_sa2_db);

                /* Update the status. */
                status = NU_SUCCESS;
            }

            /* If current proposal mismatched, then skip all payloads
             * belonging to the current proposal until the next
             * proposal, if any, is found.
             */
            else if(status != NU_SUCCESS)
            {
                continue;
            }

            /* Make sure protocol ID is valid. */
            if(in_sa->ike_proposals[i].ike_protocol_id == 0)
            {
                NLOG_Error_Log("Protocol ID set to RESERVED value",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* RESERVED protocol ID. */
                status = IKE_INVALID_PROTOCOL;
                break;
            }

            /* Set the match flag to false. */
            is_match = NU_FALSE;

            /* Loop for each security and match it against the
             * current proposal payload.
             *
             * NOTE: Order of protocols within the proposal is
             * not specified by the IKE standard. Therefore
             * match the proposal to the securities regardless
             * of order, using two nested loops.
             */
            for(j = 0; j < security_size; j++)
            {
                /* Check if protocol matches. */
                if(in_sa->ike_proposals[i].ike_protocol_id !=
                   IKE_Protocol_ID_IPS_To_IKE(security[j].ipsec_protocol))
                {
                    /* Log debug message. */
                    IKE_DEBUG_LOG("Protocol mismatch (AH/ESP)");

                    /* Protocol mismatch. Try next security. */
                    status = IKE_INVALID_PROTOCOL;
                    continue;
                }

                /* Loop for each transform in the input SA payload. */
                for(k = 0;
                    ((k < in_sa->ike_proposals[i].ike_num_transforms) &&
                     (is_match == NU_FALSE));
                    k++)
                {
                    /* Log debug message. */
                    IKE_DEBUG_LOG(
                        "Protocol matches - Now matching transform");

                    /* Get pointer to current transform. */
                    in_transform =
                        &in_sa->ike_proposals[i].ike_transforms[k];

                    /* Check if the current transform matches. */
                    if(IKE_Match_Transform2(ph2,
                           in_sa->ike_proposals[i].ike_protocol_id,
                           &security[j], in_transform) == NU_SUCCESS)
                    {
                        /* Set match flag to TRUE. */
                        is_match = NU_TRUE;

                        status = NU_SUCCESS;
                    }
                }

                /* If a matching transform was found. */
                if(is_match == NU_TRUE)
                {
                    break;
                }
            }

            /* If a valid Transform not found, try next proposal. */
            if(is_match == NU_FALSE)
            {
                /* Log debug message. */
                IKE_DEBUG_LOG(
                    "Current proposal not suitable - trying next");

                /* If status has not been updated already. */
                if(status == NU_SUCCESS)
                {
                    /* Transform mismatch. Proposal match not possible
                     * so update status and don't look any further.
                     */
                    status = IKE_NOT_NEGOTIABLE;
                }
            }

            else
            {
                /* Add an SA2 item corresponding to the
                 * current security protocol.
                 */

                /* Zero out the SA2 structure. */
                UTL_Zero(&sa2, sizeof(sa2));

                /* Set the IPsec security protocol in SA2. */
                NU_BLOCK_COPY(&sa2.ike_ips_security,
                              &security[j],
                              sizeof(IPSEC_SECURITY_PROTOCOL));

                /* Add the SA2 item to the Handle. */
                status = IKE_Add_SA2(&ph2->ike_sa2_db, &sa2);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to add SA2 item to database",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }

                /* Copy the current proposal. */
                out_sa->ike_proposals[prop_span].ike_num_transforms =
                    IKE_PHASE2_NUM_TRANSFORMS;
                out_sa->ike_proposals[prop_span].ike_proposal_no    =
                    in_sa->ike_proposals[i].ike_proposal_no;
                out_sa->ike_proposals[prop_span].ike_protocol_id    =
                    in_sa->ike_proposals[i].ike_protocol_id;

                /* Temporarily set the remote SPI in the SA payload.
                 * This would be overwritten later, after the previous
                 * value is saved in the SA2 structure.
                 */
                NU_BLOCK_COPY(out_sa->ike_proposals[prop_span].ike_spi,
                              in_sa->ike_proposals[i].ike_spi,
                              IKE_IPS_SPI_LEN);

                out_sa->ike_proposals[prop_span].ike_spi_len =
                    IKE_IPS_SPI_LEN;

                /* Copy matching transform of the current proposal. */
                out_transform =
                    &out_sa->ike_proposals[prop_span].ike_transforms[0];
                out_transform->ike_num_attributes =
                    in_transform->ike_num_attributes;
                out_transform->ike_transform_no =
                    in_transform->ike_transform_no;
                out_transform->ike_transform_id =
                    in_transform->ike_transform_id;

                /* Loop for each attribute. */
                for(k = 0; k < in_transform->ike_num_attributes; k++)
                {
                    /* Set pointers to source and destination. */
                    in_attrib  = &in_transform->ike_sa_attributes[k];
                    out_attrib = &out_transform->ike_sa_attributes[k];

                    /* Convert the attribute. */
                    out_attrib->ike_attrib_type =
                        in_attrib->ike_attrib_type;
                    out_attrib->ike_attrib_lenval =
                        in_attrib->ike_attrib_lenval;

                    /* If the value field is present. */
                    if((in_attrib->ike_attrib_type &
                        IKE_ATTRIB_AF_MASK) == IKE_ATTRIB_AF_TLV)
                    {
                        /* Copy the attribute value. */
                        out_attrib->ike_attrib_val =
                            in_attrib->ike_attrib_val;
                    }
                }

                /* Increment proposal payloads span. */
                prop_span++;

                /* If this is the last proposal of the list. */
                if(i == in_sa->ike_num_proposals - 1)
                {
                    /* Check if matching proposal is incomplete. */
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
                    if(IKE_Proposal_Span2(j, security, security_size) !=
                       prop_span)
#else
                    if(security_size != prop_span)
#endif
                    {
                        /* Valid proposal not found. Update status. */
                        status = IKE_NOT_NEGOTIABLE;
                    }
                }
            }
        }

        /* If a matching proposal was found. */
        if(status == NU_SUCCESS)
        {
#if (IKE_DECODE_PARTIAL_SA == NU_TRUE)
            /* Make sure that if the SA was decoded partially, then
             * the selected proposal is not the last proposal in the
             * list, because the last proposal could be incomplete.
             */
            if((in_sa->ike_partial_proposals == NU_TRUE) &&
               (out_sa->ike_proposals->ike_proposal_no == prop_no))
            {
                NLOG_Error_Log(
                    "Last proposal cannot be chosen from partial SA",
                    NERR_INFORMATIONAL, __FILE__, __LINE__);

                status = IKE_NOT_NEGOTIABLE;
            }

            else
#endif
            {
                /* Log debug message. */
                IKE_DEBUG_LOG("Suitable proposal selected");

                /* Set fields of the SA payload for reply. */
                out_sa->ike_num_proposals = prop_span;
                out_sa->ike_doi           = in_sa->ike_doi;
                out_sa->ike_situation_len = in_sa->ike_situation_len;

                /* Also copy the situation field. */
                NU_BLOCK_COPY(out_sa->ike_situation,
                              in_sa->ike_situation,
                              in_sa->ike_situation_len);

                /* Add SA payload to the payloads chain. */
                IKE_ADD_TO_CHAIN(ph2->ike_params->ike_out.ike_last,
                                 out_sa, IKE_SA_PAYLOAD_ID);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Select_Proposal2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Verify_Proposal2
*
* DESCRIPTION
*
*       This is a utility function used by the Initiator
*       to verify that none of the proposals sent in the
*       initial SA have been tampered by the Responder.
*       It ensures that the negotiated attributes are one
*       of those which were sent in the proposal.
*
*       The Phase 2 Handle specified must contain an SA2
*       corresponding to each two-way IPsec SA being
*       negotiated.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              If verification successful.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_INVALID_SPI         SPI in proposal is invalid.
*       IKE_SA2_NOT_FOUND       SA2 item not found in Handle.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed limit.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*
*************************************************************************/
STATIC STATUS IKE_Verify_Proposal2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status = NU_SUCCESS;
    IKE_SA_DEC_PAYLOAD  *dec_sa;
    IKE_SA2             *sa2;
    INT                 i;
    UINT8               prop_no;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Verifying proposal selection");

    /* Set local pointer to commonly used data in the Handle. */
    dec_sa = ph2->ike_params->ike_in.ike_sa;

    /* Make sure the number of proposal payloads
     * received is not equal to zero.
     */
    if(dec_sa->ike_num_proposals == 0)
    {
        /* The negotiated SA match is not possible. */
        status = IKE_PROPOSAL_TAMPERED;
    }

    else
    {
        /* Set the proposal number to that of the first proposal.
         * All proposal payloads in the SA MUST have the same
         * proposal number since they all form a single proposal
         * selection.
         */
        prop_no = dec_sa->ike_proposals->ike_proposal_no;

        /* Set the SA2 item to first item in the database. */
        sa2 = ph2->ike_sa2_db.ike_flink;

        /* Loop for each proposal payload in the SA. Each
         * proposal payload in the SA should have a
         * corresponding SA2 item in the SA2DB, both
         * occurring in the same order.
         */
        for(i = 0;
            (status == NU_SUCCESS) && (i < dec_sa->ike_num_proposals);
            i++)
        {
            /* Make sure a valid SA2 item exists. */
            if(sa2 == NU_NULL)
            {
                NLOG_Error_Log("No SA2 item found in Handle",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* SA2 not passed in Handle. */
                status = IKE_SA2_NOT_FOUND;
            }

            /* Make sure proposal number of all proposal payloads
             * is the same as they make up a single proposal.
             */
            else if(dec_sa->ike_proposals[i].ike_proposal_no != prop_no)
            {
                NLOG_Error_Log("Too many proposals specified in selection",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Report error. All proposals must have the
                 * same proposal number.
                 */
                status = IKE_TOO_MANY_PROPOSALS;
            }

            /* Make sure only a single transform payload is
             * present in the current proposal. Multiple
             * transforms must NOT exist in the reply to the
             * proposal.
             */
            else if(dec_sa->ike_proposals[i].ike_num_transforms !=
                    IKE_NUM_NEGOTIATED_TRANSFORMS)
            {
                NLOG_Error_Log("Multiple Transform payloads in selection",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Multiple transforms found. */
                status = IKE_PROPOSAL_TAMPERED;
            }

            else
            {
                /* Make sure the SPI is not reserved. */
                if((dec_sa->ike_proposals[i].ike_spi[0] == 0) &&
                   (dec_sa->ike_proposals[i].ike_spi[1] == 0) &&
                   (dec_sa->ike_proposals[i].ike_spi[2] == 0))
                {
                    NLOG_Error_Log("SPI set to reserved value",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Has a reserved value. */
                    status = IKE_INVALID_SPI;
                }

                /* Make sure the first (and only) transform matches
                 * the current SA proposal.
                 */
                else if(IKE_Match_Transform2(ph2,
                            dec_sa->ike_proposals[i].ike_protocol_id,
                            &sa2->ike_ips_security,
                            dec_sa->ike_proposals[i].ike_transforms)
                            != NU_SUCCESS)
                {
                    NLOG_Error_Log("Proposal tampered by Responder",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Proposal does not match IPsec security protocol. */
                    status = IKE_PROPOSAL_TAMPERED;
                }

                else
                {
                    /* Move to the next SA2 item. */
                    sa2 = sa2->ike_flink;
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Verify_Proposal2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Convert_Transform2
*
* DESCRIPTION
*
*       This function converts the specified transform payload
*       to an equivalent SA2 structure. The optional fields
*       are set to default values if they have not been specified
*       in the transform.
*
* INPUTS
*
*       proto                   The IKE protocol ID of proposal.
*       *transform              Transform payload to be converted.
*       *security               On return, this contains an
*                               equivalent of the specified
*                               transform payload.
*       *sa_life                On return, contains the IPsec SA
*                               negotiated lifetime.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_MISSING_ATTRIB      A required attribute was missing.
*
*************************************************************************/
STATIC STATUS IKE_Convert_Transform2(UINT8 proto,
                                     IKE_TRANSFORM_PAYLOAD *transform,
                                     IPSEC_SECURITY_PROTOCOL *security,
                                     IPSEC_SA_LIFETIME *sa_life)
{
    STATUS          status = NU_SUCCESS;
    INT             i;
    UINT32          attrib_val;
    IKE_DATA_ATTRIB *attrib;
    UINT8           ignore_lifetime = NU_FALSE;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((transform == NU_NULL) || (security == NU_NULL) ||
       (sa_life   == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Converting transforms");

    /* Convert the protocol ID. */
    security->ipsec_protocol = IKE_Protocol_ID_IKE_To_IPS(proto);

    /* Check whether protocol is AH. */
    if(proto == IKE_PROTO_AH)
    {
        /* Convert the AH transform ID to IPsec algorithm ID. */
        security->ipsec_auth_algo =
            IKE_AH_Trans_ID_IKE_To_IPS(transform->ike_transform_id);
    }

    /* Otherwise the protocol would be ESP. */
    else
    {
        /* Convert the ESP transform ID to IPsec algorithm ID. */
        security->ipsec_encryption_algo =
            IKE_ESP_Trans_ID_IKE_To_IPS(transform->ike_transform_id);
    }

    /* Loop for each attribute of the transform. */
    for(i = 0; i < transform->ike_num_attributes; i++)
    {
        /* Set pointer to the current attribute. */
        attrib = &transform->ike_sa_attributes[i];

        /* Determine the attribute type. Validation of attribute
         * type is not performed because it is performed in the
         * proposal selection function. The following attribute
         * types are ignored, as they are not currently supported
         * and would have been negotiated with the default
         * values, assumed by IPsec.
         *
         *    - IKE_IPS_ATTRIB_KEY_LEN
         *    - IKE_IPS_ATTRIB_KEY_RND
         *
         * Following items are not set because they are not
         * negotiable and have been validated before this
         * function is called:
         *
         *    - IKE_IPS_ATTRIB_LIFE_TYPE
         *    - IKE_IPS_ATTRIB_GROUP_DESC
         */
        switch(attrib->ike_attrib_type)
        {
        case IKE_IPS_ATTRIB_ENCAP_MODE:
            /* Set the encapsulation mode. */
            security->ipsec_security_mode = attrib->ike_attrib_lenval;
            break;

        case IKE_IPS_ATTRIB_AUTH_ALGO:
            /* Set the authentication algorithm. */
            security->ipsec_auth_algo =
                IKE_Auth_Algo_ID_IKE_To_IPS(attrib->ike_attrib_lenval);
            break;

        case IKE_IPS_ATTRIB_LIFE_TYPE:
            /* Make sure lifetime type is seconds. */
            if(attrib->ike_attrib_lenval == IKE_IPS_VAL_SECS)
            {
                /* Do not ignore lifetime duration attribute. */
                ignore_lifetime = NU_FALSE;
            }

            else
            {
                /* Ignore lifetime duration attribute. */
                ignore_lifetime = NU_TRUE;
            }

            break;

        default:
            /* The life duration attribute may have been
             * encoded as either variable or a basic type.
             */
            if((attrib->ike_attrib_type & IKE_ATTRIB_TYPE_MASK) ==
                IKE_IPS_ATTRIB_LIFE_DURATION)
            {
                /* If lifetime type is supported. */
                if(ignore_lifetime == NU_FALSE)
                {
                    /* Decode the attribute value. This value has
                     * already been checked in the proposal selection
                     * function so this call must not fail.
                     */
                    if(IKE_Decode_Attribute_Value(&attrib_val, attrib)
                       != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to decode attribute value",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* Set lifetime duration in seconds. */
                    sa_life->ipsec_no_of_secs = attrib_val;
                }
            }

            /* Simply break on unrecognized attributes. These have
             * been checked in the proposal selection function and
             * would contain default values assumed by IPsec.
             */
            break;
        }
    }

    /* Make sure the lifetime was specified. */
    if(sa_life->ipsec_no_of_secs == IPSEC_WILDCARD)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Lifetime not negotiated - using default");

        /* Otherwise, use the default lifetime defined by the DOI. */
        sa_life->ipsec_no_of_secs = IKE_IPS_DEFAULT_SA_LIFETIME;
    }

    /* If the security mode not specified, use default value. */
    if(security->ipsec_security_mode == IPSEC_WILDCARD)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Security mode not negotiated - using default");

        /* Transport mode is assumed if not specified. */
        security->ipsec_security_mode = IKE_IPS_DEFAULT_ENCAP_MODE;
    }

    /* Return the status. */
    return (status);

} /* IKE_Convert_Transform2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_Nonce_Data2
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to add Nonce data to the chain of payloads. The data is
*       stored in the Handle within a dynamically allocated
*       buffer. The caller is responsible for freeing this buffer
*       when it is no longer needed.
*
*       Following payloads are appended to the chain:
*       - IKE_NONCE_ENC_PAYLOAD
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATIC STATUS IKE_Generate_Nonce_Data2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS          status;
    IKE_ENC_MESSAGE *out;
    UINT8           *nonce_buffer;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating Nonce data");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph2->ike_params->ike_out;

    /* Allocate memory for the Nonce buffer. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                (VOID**)&nonce_buffer,
                                IKE_OUTBOUND_NONCE_DATA_LEN,
                                NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        nonce_buffer = TLS_Normalize_Ptr(nonce_buffer);

        /* Determine whether caller is Initiator. */
        if((ph2->ike_flags & IKE_INITIATOR) != 0)
        {
            /* Set the Initiator's Nonce data and length. */
            ph2->ike_nonce_i     = nonce_buffer;
            ph2->ike_nonce_i_len = IKE_OUTBOUND_NONCE_DATA_LEN;
        }

        /* Otherwise, would be Responder. */
        else
        {
            /* Set the Responder's Nonce data and length. */
            ph2->ike_nonce_r     = nonce_buffer;
            ph2->ike_nonce_r_len = IKE_OUTBOUND_NONCE_DATA_LEN;
        }

        /* Generate the random Nonce data. */

        if(RAND_bytes(nonce_buffer, IKE_OUTBOUND_NONCE_DATA_LEN))
        {
            /* Set Nonce data in the payload. */
            out->ike_nonce->ike_nonce_data     = nonce_buffer;
            out->ike_nonce->ike_nonce_data_len =
                IKE_OUTBOUND_NONCE_DATA_LEN;

            /* Link payload to the chain. */
            IKE_ADD_TO_CHAIN(out->ike_last, out->ike_nonce,
                             IKE_NONCE_PAYLOAD_ID);
        }

        else
        {
            NLOG_Error_Log("Failed to generate random data",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_Nonce_Data2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_KeyXchg_Data2
*
* DESCRIPTION
*
*       This function generates the Diffie-Hellman key pair and
*       stores it in the Phase 2 Handle. The public key is also
*       set in the Key Exchange payload.
*
*       The Diffie-Hellman key pair is stored in the Handle
*       within a dynamically allocated buffer. The caller is
*       responsible for freeing this buffer when it is no longer
*       needed.
*
*       Following payloads are appended to the chain:
*       - IKE_KEYXCHG_ENC_PAYLOAD
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC STATUS IKE_Generate_KeyXchg_Data2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status;
    IKE_ENC_MESSAGE     *out;
    IKE_KEY_PAIR        dh_keys;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating Key Exchange data");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph2->ike_params->ike_out;

    /* Make the request for Key Pair generation. */
    status = IKE_DH_Generate_Key(IKE_Oakley_Group_Prime(ph2->ike_group_desc),
                                 IKE_Oakley_Group_Length(ph2->ike_group_desc),
                                 IKE_OAKLEY_GROUP_GEN(ph2->ike_group_desc),
                                 &dh_keys, IKE_DH_PRIVATE_KEY_SIZE,
                                 NU_NULL, 0);

    if(status == NU_SUCCESS)
    {
        /* Store the Diffie-Hellman Kay Pair. This must
         * be deallocated later.
         */
        ph2->ike_dh_key.ike_public_key      = dh_keys.ike_public_key;
        ph2->ike_dh_key.ike_public_key_len  = dh_keys.ike_public_key_len;
        ph2->ike_dh_key.ike_private_key     = dh_keys.ike_private_key;
        ph2->ike_dh_key.ike_private_key_len = dh_keys.ike_private_key_len;

        /* Set Key Exchange data in the payload. */
        out->ike_key->ike_keyxchg_data      = dh_keys.ike_public_key;
        out->ike_key->ike_keyxchg_data_len  = dh_keys.ike_public_key_len;

        /* Link payload to the chain. */
        IKE_ADD_TO_CHAIN(out->ike_last, out->ike_key,
                         IKE_KEYXCHG_PAYLOAD_ID);
    }

    else
    {
        NLOG_Error_Log("Failed to generate Diffie-Hellman key pair",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_KeyXchg_Data2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_ID_Data2
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to generate the Identification payloads based on an
*       IPsec selector.
*
*       Following payloads are appended to the chain:
*       - IKE_ID_ENC_PAYLOAD
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_SELECTOR    Address type in selector is
*                               invalid.
*
*************************************************************************/
STATIC STATUS IKE_Generate_ID_Data2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status;
    IKE_ID_ENC_PAYLOAD  *enc_id;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating ID data");

    /* Set pointer to Initiator's ID payload. */
    enc_id = ph2->ike_params->ike_out.ike_id_i;

    /* Generate the Initiator's ID payload. */
    status = IKE_IPS_Selector_To_ID(&ph2->ike_ips_select, enc_id,
                                    IKE_LOCAL);

    if(status == NU_SUCCESS)
    {
        /* Add IDci payload to the chain. */
        IKE_ADD_TO_CHAIN(ph2->ike_params->ike_out.ike_last,
                         enc_id, IKE_ID_PAYLOAD_ID);

        /* Set pointer to Responder's ID payload. */
        enc_id = ph2->ike_params->ike_out.ike_id_r;

        /* Generate the Responder's ID payload. */
        status = IKE_IPS_Selector_To_ID(&ph2->ike_ips_select, enc_id,
                                        IKE_REMOTE);

        if(status == NU_SUCCESS)
        {
            /* Add IDcr payload to the chain. */
            IKE_ADD_TO_CHAIN(ph2->ike_params->ike_out.ike_last,
                             enc_id, IKE_ID_PAYLOAD_ID);
        }

        else
        {
            NLOG_Error_Log(
                "Unable to convert IPsec selector to ID payload",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Unable to convert IPsec selector to ID payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_ID_Data2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_RSelection
*
* DESCRIPTION
*
*       This function processes the selection made locally
*       from the Initiator's proposal. It is executed by the
*       Responder. IPsec SA parameters are set in the SA2
*       items. The only data missing after this step are the
*       encryption and authentication keys. These would be
*       generated in the next state of the Responder.
*
*       Note that this function does not test the specified
*       selection for validity. Values of the selected
*       proposal are checked during selection.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PROPOSAL    Selection contains no proposal.
*       IKE_MISSING_ATTRIB      Required attribute is missing.
*
*************************************************************************/
STATIC STATUS IKE_Process_RSelection(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status = IKE_INVALID_PROPOSAL;
    INT                 i;
    IKE_SA_ENC_PAYLOAD  *enc_sa;
    IKE_SA2             *sa2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Processing local proposal selection");

    /* Set local pointer to commonly used data in the Handle. */
    enc_sa = ph2->ike_params->ike_out.ike_sa;

    /* Set SA2 item to first item in the database. */
    sa2 = ph2->ike_sa2_db.ike_flink;

    /* Loop for each proposal payload in the SA payload. */
    for(i = 0; i < enc_sa->ike_num_proposals; i++)
    {
        /* Convert the transform to IPsec security protocol. */
        status = IKE_Convert_Transform2(
            enc_sa->ike_proposals[i].ike_protocol_id,
            &enc_sa->ike_proposals[i].ike_transforms[0],
            &sa2->ike_ips_security, &ph2->ike_ips_lifetime);

        if(status == NU_SUCCESS)
        {
            /* Generate the local SPI and store it in the SA2. */
            sa2->ike_local_spi = IKE_Data.ike_spi_index;

            /* Increment the SPI counter for next use. */
            IKE_Data.ike_spi_index++;

            /* If the SPI variable has overflowed. */
            if(IKE_Data.ike_spi_index == 0)
            {
                NLOG_Error_Log("SPI variable wrapped to initial value",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Reset to start of the SPI range assigned to IKE. */
                IKE_Data.ike_spi_index = IPSEC_SPI_END + 1;
            }

            /* Also store the remote SPI in SA2. */
            sa2->ike_remote_spi =
                GET32(enc_sa->ike_proposals[i].ike_spi, 0);

            /* Overwrite the remote SPI by the local
             * SPI, in the proposal.
             */
            PUT32(enc_sa->ike_proposals[i].ike_spi, 0,
                  sa2->ike_local_spi);
        }

        else
        {
            NLOG_Error_Log("Failed to convert transforms",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Stop processing if error occurred. */
            break;
        }

        /* Move to the next SA2 item. */
        sa2 = sa2->ike_flink;
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_RSelection */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_ISelection
*
* DESCRIPTION
*
*       This function processes the selection made by the
*       remote node from the Initiator's proposal. It is
*       executed by the Initiator. The IPsec SA parameters
*       present in SA2 items are updated according to the
*       selection. The only thing missing from the parameters
*       after processing by this function would be the key
*       material.
*
*       Note that this function does not test the specified
*       selection for validity. Values of the selected
*       proposal should be verified before calling this
*       function.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PROPOSAL    Selection contains no proposal.
*       IKE_MISSING_ATTRIB      Required attribute is missing.
*
*************************************************************************/
STATIC STATUS IKE_Process_ISelection(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status = IKE_INVALID_PROPOSAL;
    INT                 i;
    IKE_SA_DEC_PAYLOAD  *dec_sa;
    IKE_SA2             *sa2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Processing proposal selection made by Responder");

    /* Set local pointer to commonly used data in the Handle. */
    dec_sa = ph2->ike_params->ike_in.ike_sa;

    /* Set SA2 item to first item in the database. */
    sa2 = ph2->ike_sa2_db.ike_flink;

    /* Loop for each proposal payload in the SA payload. */
    for(i = 0; i < dec_sa->ike_num_proposals; i++)
    {
        /* Convert the transform to IPsec security protocol. */
        status = IKE_Convert_Transform2(
                     dec_sa->ike_proposals[i].ike_protocol_id,
                     &dec_sa->ike_proposals[i].ike_transforms[0],
                     &sa2->ike_ips_security, &ph2->ike_ips_lifetime);

        if(status == NU_SUCCESS)
        {
            /* Copy the Responder generated remote SPI. Note that the
             * local SPI has been generated earlier when this exchange
             * was initiated.
             */
            sa2->ike_remote_spi =
                GET32(dec_sa->ike_proposals[i].ike_spi, 0);
        }

        else
        {
            NLOG_Error_Log("Failed to convert transforms",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Stop processing if error occurred. */
            break;
        }

        /* Move to the next SA2 item. */
        sa2 = sa2->ike_flink;
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_ISelection */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_1
*
* DESCRIPTION
*
*       This function implements Quick Mode state 1 of the
*       IKE state machine. It is executed by the Initiator.
*       A Phase 2 proposal is created using the IPsec SA
*       requests stored in the Handle. Nonce data is also
*       generated and added to the message. The Key Exchange
*       payload is added to the message if the IPsec policy
*       specifies Perfect Forward Secrecy (PFS) to be used.
*       The optional identification payloads are always sent
*       in the message. Encryption is also performed on the
*       outbound message.
*
*       Following are payloads sent in this state:
*
*       Initiator*                      Responder
*       ---------                       ---------
*       HDR*, HASH(1), SA, Ni   -->
*         [, KE] [, IDci, IDcr]
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_INVALID_ALGO_ID   Invalid algorithm ID in security
*                               protocol.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed limit.
*       IKE_SA2_NOT_FOUND       SA2 Item not found in Handle.
*       IKE_INVALID_PROTOCOL    Protocol ID in security is not valid.
*       IKE_INVALID_SELECTOR    Address type in selector is
*                               invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_1(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_ENC_MESSAGE         *out;
    IKE_STATE_PARAMS        *params;
    IKE_ENC_HDR             enc_hdr;
    IKE_HASH_ENC_PAYLOAD    enc_hash;
    IKE_NONCE_ENC_PAYLOAD   enc_nonce;
    IKE_KEYXCHG_ENC_PAYLOAD enc_key;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Quick mode: state 1");

    /* Set local pointers to commonly used data in the Handle. */
    out    = &ph2->ike_params->ike_out;
    params = ph2->ike_params;

    /* Make sure packet pointer is NULL. */
    if(params->ike_packet != NU_NULL)
    {
        /* Packet MUST be NULL in this state as a
         * new exchange is being initiated.
         */
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Generate the IV for this new Phase 2 Exchange. */
        status = IKE_Phase2_IV(ph2);

        if(status == NU_SUCCESS)
        {
            /* Set outbound payloads used in this state. */
            IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
            IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);
            IKE_SET_OUTBOUND(out->ike_sa, &IKE_Large_Data.ike_enc_sa);
            IKE_SET_OUTBOUND(out->ike_nonce, &enc_nonce);
            IKE_SET_OUTBOUND(out->ike_key, &enc_key);
            IKE_SET_OUTBOUND(out->ike_id_i, &IKE_Large_Data.ike_enc_idi);
            IKE_SET_OUTBOUND(out->ike_id_r,
                             &IKE_Large_Data.ike_id.ike_enc_idr);

            /* Initialize chain of outbound payloads. */
            IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

            /* Get hash digest length. */

            status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[ph2->ike_sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                           &(enc_hash.ike_hash_data_len));

            if(status == NU_SUCCESS)
            {
                /* Add a blank Hash payload to the chain. This would
                 * be filled when the packet is being sent.
                 */
                IKE_ADD_TO_CHAIN(out->ike_last, &enc_hash,
                                IKE_HASH_PAYLOAD_ID);

                /* Construct an SA proposal. */
                status = IKE_Construct_Proposal2(ph2);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Unable to construct IKE proposal",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to get hash digest length",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to generate Phase 2 IV",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Generate Nonce data and add its payload to the chain. */
            status = IKE_Generate_Nonce_Data2(ph2);

            if(status == NU_SUCCESS)
            {
                /* If PFS is being used. */
                if(ph2->ike_group_desc != IKE_GROUP_NONE)
                {
                    /* Generate key exchange data. */
                    status = IKE_Generate_KeyXchg_Data2(ph2);
                }

                /* If no error occurred. */
                if(status == NU_SUCCESS)
                {
                    /* Add ID payloads to the response message. */
                    status = IKE_Generate_ID_Data2(ph2);

                    if(status == NU_SUCCESS)
                    {
                        /* Terminate chain of payloads. */
                        IKE_END_CHAIN(out->ike_last);

                        /* Initialize ISAKMP header. */
                        IKE_Set_Header(&enc_hdr, ph2->ike_msg_id,
                                       ph2->ike_sa->ike_cookies,
                                       IKE_XCHG_QUICK, IKE_HDR_ENC_MASK);

                        /* Send the message. */
                        status = IKE_Send_Phase2_Packet(ph2);

                        if(status == NU_SUCCESS)
                        {
                            /* Message sent, so synchronize the
                             * encryption IV.
                             */
                            IKE_SYNC_IV(ph2->ike_encryption_iv,
                                        ph2->ike_decryption_iv,
                                        ph2->ike_sa);

                            /* Add phase 2 timeout event. */
                            status = IKE_Set_Timer(
                                         IKE_Phase2_Timeout_Event,
                                         (UNSIGNED)ph2->ike_sa,
                                         (UNSIGNED)ph2,
                                         IKE_PHASE2_TIMEOUT);

                            if(status == NU_SUCCESS)
                            {
                                /* Move to next state. */
                                ph2->ike_xchg_state += IKE_NEXT_STATE_INC;
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
                            NLOG_Error_Log("Failed to send IKE packet",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate ID payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log(
                        "Failed to generate Key Exchange payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to generate Nonce payload",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_2
*
* DESCRIPTION
*
*       This function implements Quick Mode state 2 of the
*       IKE state machine. It is executed on behalf of the
*       Responder. The Phase 2 proposal sent by the
*       Initiator is Received and compared with the IPsec
*       Policy. If the proposal is approved, the selection
*       is sent to the Initiator. If PFS is requested, the
*       Key Exchange payload is also sent in the message.
*       Identification payloads are also sent if tunnel mode
*       is being used.
*
*       As this is the first Phase 2 message to the Responder,
*       the Phase 2 Handle passed to this function is not
*       part of the database. This function adds the Handle
*       to the database maintained in the IKE SA.
*
*       Since this state involves a large number of incoming
*       and outgoing payloads, it has been divided into two
*       utility functions. One for receiving and another
*       for sending payloads.
*
*       Following are payloads received/sent in this state:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR*, HASH(1), SA, Ni   -->
*         [, KE] [, IDci, IDcr]
*                               <--     HDR*, HASH(2), SA, Nr
*                                         [, KE] [, IDci, IDcr]
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         In case policy is not found.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_INVALID_PROPOSAL    Proposal is not valid.
*       IKE_NOT_NEGOTIABLE      Proposal is not negotiable.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*       IKE_VERIFY_FAILED       Hash verification failed.
*       IKE_UNSUPPORTED_IDTYPE  Identification type not
*                               supported by IKE.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*       IKE_UNALLOWED_XCHG      If exchange not allowed.
*       IPSEC_INVALID_ALGO_ID   Invalid algorithm ID in security
*                               protocol.
*       IKE_INVALID_PROTOCOL    Protocol is not supported.
*       IKE_INVALID_TRANSFORM   Transform not supported.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_TRANSFORM_MISMATCH  Transform does not match Policy.
*       IKE_MISSING_ATTRIB      Required attribute is missing.
*       IKE_INVALID_SELECTOR    Address type in selector is
*                               invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_ENC_MESSAGE         *out;
    IKE_STATE_PARAMS        *params;
    IKE_ID_DEC_PAYLOAD      dec_idi;
    IKE_ID_DEC_PAYLOAD      dec_idr;
    IKE_ENC_HDR             enc_hdr;
    IKE_HASH_ENC_PAYLOAD    enc_hash;
    UINT8                   msg_hash[IKE_MD5_DIGEST_LEN];

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Quick mode: state 2");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    out    = &ph2->ike_params->ike_out;
    params = ph2->ike_params;

    /* Make sure packet pointer is not NULL. */
    if(params->ike_packet == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Calculate hash of the encrypted message. */
        IKE_HASH_ENC_RESEND(ph2, msg_hash);

        /* Generate the IV for this new Phase 2 Exchange.
         * It is safe to ignore the return value.
         */
        status = IKE_Phase2_IV(ph2);

        if(status == NU_SUCCESS)
        {
            /* Decrypt the message. */
            status = IKE_Encrypt(ph2->ike_sa,
                         params->ike_packet->ike_data + IKE_HDR_LEN,
                         params->ike_packet->ike_data_len - IKE_HDR_LEN,
                         NU_NULL, ph2->ike_decryption_iv,
                         ph2->ike_encryption_iv, IKE_DECRYPT);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decrypt incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to generate Phase 2 IV",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Partially set inbound payloads used in this state. */
            IKE_SET_INBOUND_REQUIRED(in->ike_sa,
                                     &IKE_Large_Data.ike_dec_sa);
            IKE_SET_INBOUND_OPTIONAL(in->ike_id_i, &dec_idi);
            IKE_SET_INBOUND_OPTIONAL(in->ike_id_r, &dec_idr);

            /* Partially set outbound payloads required during
             * decoding of the received packet.
             */
            IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
            IKE_SET_OUTBOUND(out->ike_sa, &IKE_Large_Data.ike_enc_sa);
            IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);

            /* Specify ISAKMP header as first item in payload chain. */
            IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

            /* Add a blank Hash payload to the chain. This would
             * be filled when the reply packet is being sent.
             */
            IKE_ADD_TO_CHAIN(out->ike_last, &enc_hash,
                             IKE_HASH_PAYLOAD_ID);

            /* Receive and verify the payloads. The Handle is
             * passed by reference because it would be added to
             * to the database and be replaced by a pointer in
             * the database.
             */
            status = IKE_Quick_State_2_3_Recv(ph2);

            if(status == NU_SUCCESS)
            {
                /* Since the message is authenticated, the
                 * decryption IV can now be synchronized.
                 */
                IKE_SYNC_IV(ph2->ike_decryption_iv,
                            ph2->ike_encryption_iv, ph2->ike_sa);

                /* Now its safe to add the Handle to the database. */
                status = IKE_Add_Phase2(&ph2->ike_sa->ike_phase2_db,
                                        ph2, &ph2->ike_flink);

                if(status == NU_SUCCESS)
                {
                    /* Update the Handle pointer. */
                    ph2 = ph2->ike_flink;

                    /* Process all received payloads. */
                    status = IKE_Quick_State_2_Process(ph2);

                    if(status == NU_SUCCESS)
                    {
                        /* Send the reply message. */
                        status = IKE_Quick_State_2_Send(ph2);

                        if(status == NU_SUCCESS)
                        {
                            /* Update hash of last message received. */
                            IKE_UPDATE_ENC_RESEND(ph2, msg_hash);

                            /* Message sent, so synchronize the
                             * encryption IV.
                             */
                            IKE_SYNC_IV(ph2->ike_encryption_iv,
                                        ph2->ike_decryption_iv,
                                        ph2->ike_sa);

                            /* Add Phase 2 timeout event. */
                            status = IKE_Set_Timer(
                                         IKE_Phase2_Timeout_Event,
                                         (UNSIGNED)ph2->ike_sa,
                                         (UNSIGNED)ph2,
                                         IKE_PHASE2_TIMEOUT);

                            if(status == NU_SUCCESS)
                            {
                                /* Move to next state. */
                                ph2->ike_xchg_state += IKE_NEXT_STATE_INC;
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
                            NLOG_Error_Log("Failed to send message",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log(
                            "Failed to process incoming message",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to add Handle to database",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to decode/verify incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_2_3_Recv
*
* DESCRIPTION
*
*       This function receives Quick Mode payloads for
*       state 2 and 3 and performs basic checks on the
*       received payloads. It verifies the Hash value in
*       the message. The Nonce and Key Exchange payload's
*       data is returned to the caller in dynamically
*       allocated buffers. The caller is responsible for
*       freeing this memory.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*       IKE_VERIFY_FAILED       Hash verification failed.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_2_3_Recv(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_STATE_PARAMS        *params;
    IKE_HASH_DEC_PAYLOAD    dec_hash;
    IKE_NONCE_DEC_PAYLOAD   dec_nonce;
    IKE_KEYXCHG_DEC_PAYLOAD dec_key;
    IKE_NOTIFY_DEC_PAYLOAD  dec_notify;
    UINT8                   **nonce_buffer;
    UINT16                  data_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Receiving Quick mode payloads");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    params = ph2->ike_params;

    /* Partially set the rest of the inbound payloads. */
    IKE_SET_INBOUND_REQUIRED(in->ike_hash, &dec_hash);
    IKE_SET_INBOUND_REQUIRED(in->ike_nonce, &dec_nonce);
    IKE_SET_INBOUND_OPTIONAL(in->ike_key, &dec_key);
    IKE_SET_INBOUND_OPTIONAL(in->ike_notify, &dec_notify);

    /* Decode all payloads in the packet. */
    status = IKE_Decode_Message(params->ike_packet->ike_data, in);

    if(status == NU_SUCCESS)
    {
        /* If caller is the Initiator. */
        if((ph2->ike_flags & IKE_INITIATOR) != 0)
        {
            /* Set Responder's Nonce buffer and length. */
            nonce_buffer = &ph2->ike_nonce_r;
            ph2->ike_nonce_r_len = dec_nonce.ike_nonce_data_len;
        }

        else
        {
            /* Set Initiator's Nonce buffer and length. */
            nonce_buffer = &ph2->ike_nonce_i;
            ph2->ike_nonce_i_len = dec_nonce.ike_nonce_data_len;
        }

        /* Allocate the Nonce data buffer. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)nonce_buffer,
                                    dec_nonce.ike_nonce_data_len,
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Normalize the pointer. */
            *nonce_buffer = TLS_Normalize_Ptr(*nonce_buffer);

            /* Copy the Nonce data. */
            NU_BLOCK_COPY(*nonce_buffer, dec_nonce.ike_nonce_data,
                          dec_nonce.ike_nonce_data_len);

            /* If the Key Exchange payload was received. */
            if(IKE_PAYLOAD_IS_PRESENT(&dec_key) == NU_TRUE)
            {
                /* Allocate memory for the Key Exchange data. */
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                             (VOID**)&ph2->ike_dh_remote_key,
                             dec_key.ike_keyxchg_data_len,
                             NU_NO_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    /* Normalize the pointer. */
                    ph2->ike_dh_remote_key =
                        TLS_Normalize_Ptr(ph2->ike_dh_remote_key);

                    /* Copy the Key Exchange data. */
                    NU_BLOCK_COPY(ph2->ike_dh_remote_key,
                                  dec_key.ike_keyxchg_data,
                                  dec_key.ike_keyxchg_data_len);

                    /* Also set length of the Key Exchange data. */
                    ph2->ike_dh_remote_key_len =
                        dec_key.ike_keyxchg_data_len;
                }

                else
                {
                    NLOG_Error_Log("Failed to allocate memory for DH key",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* If no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Get length of the message data without padding. */
                status = IKE_Get_Message_Length(
                             params->ike_packet->ike_data,
                             params->ike_packet->ike_data_len,
                             &data_len);

                if(status == NU_SUCCESS)
                {
                    /* Verify the Hash of the message. */
                    status = IKE_Verify_Hash_x(ph2,
                                 params->ike_packet->ike_data, data_len,
                                 dec_hash.ike_hash_data,
                                 dec_hash.ike_hash_data_len);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("HASH(x) verification failed",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Unable to calculate message length",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to allocate memory for Nonce data",
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

} /* IKE_Quick_State_2_3_Recv */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_2_Process
*
* DESCRIPTION
*
*       This function processes the payloads received
*       in Quick Mode state 2. It looks up the IPsec
*       Policy using either the ID payloads or creating
*       a selector from the source and destination
*       IP addresses. Once the Policy is found, presence
*       checks are performed on payloads. Finally, a
*       selection is made from the received SA payload.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IPSEC_NOT_FOUND         In case policy is not found.
*       IKE_UNSUPPORTED_IDTYPE  Identification type not
*                               supported by IKE.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*       IKE_UNALLOWED_XCHG      If exchange not allowed.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_NOT_NEGOTIABLE      Proposal is not negotiable.
*       IPSEC_INVALID_ALGO_ID   Invalid algorithm ID in security
*                               protocol.
*       IKE_INVALID_PROTOCOL    Protocol is not supported.
*       IKE_INVALID_TRANSFORM   Transform not supported.
*       IKE_TOO_MANY_PROPOSALS  The SA has more than one proposals.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_TRANSFORM_MISMATCH  Transform does not match Policy.
*       IKE_INVALID_PROPOSAL    Selection contains no proposal.
*       IKE_MISSING_ATTRIB      Required attribute is missing.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_2_Process(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_STATE_PARAMS        *params;
    IPSEC_SECURITY_PROTOCOL *security;
    UINT8                   security_size;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Processing Quick mode payloads");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    params = ph2->ike_params;

    /* Get the IPsec Policy Index using the ID
     * payloads, if they were sent, or using the
     * IP address of both parties, if not sent.
     */
    status = IKE_IPS_Policy_By_ID(ph2, in->ike_id_i, in->ike_id_r);

    if(status == NU_SUCCESS)
    {
        /* Make sure the IKE policy also allows
         * this phase 2 exchange.
         */
        status = IKE_IPS_Phase2_Allowed(params->ike_policy,
                                        &ph2->ike_ips_select);

        if(status == NU_SUCCESS)
        {
            /* Get the IPsec Security Protocol array in
             * a dynamically allocated buffer.
             */
            status = IKE_IPS_Get_Policy_Parameters(ph2, &security,
                                                   &security_size);

            if(status == NU_SUCCESS)
            {
                /* If IPsec policy specifies PFS. */
                if(ph2->ike_group_desc != IKE_GROUP_NONE)
                {
                    /* Make sure the Key Exchange payload
                     * was received.
                     */
                    if(ph2->ike_dh_remote_key == NU_NULL)
                    {
                        NLOG_Error_Log("Missing Key Exchange payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* A required payload was missing. */
                        status = IKE_MISSING_PAYLOAD;
                    }
                }

                /* Otherwise PFS is not to be used. */
                else
                {
                    /* The Key Exchange payload should not
                     * have been received.
                     */
                    if(ph2->ike_dh_remote_key != NU_NULL)
                    {
                        NLOG_Error_Log("Unexpected Key Exchange payload",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* PFS not allowed by policy. */
                        status = IKE_NOT_NEGOTIABLE;
                    }
                }

                /* If no error occurred. */
                if(status == NU_SUCCESS)
                {
                    /* Make selections from proposal. */
                    status = IKE_Select_Proposal2(ph2, security,
                                                  security_size);

                    if(status == NU_SUCCESS)
                    {
                        /* Set IPsec SA parameters in SA2 items
                         * using the selection from the proposal.
                         */
                        status = IKE_Process_RSelection(ph2);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to process selection",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        IKE_DEBUG_LOG("No proposal selected");
                    }
                }

                /* Security protocols are no longer needed
                 * so free dynamically allocated buffer.
                 */
                if(NU_Deallocate_Memory(security) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to get IPsec policy parameters",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Phase 2 not allowed by IKE policy",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("No matching IPsec policy found",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_2_Process */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_2_Send
*
* DESCRIPTION
*
*       This function sends a message in response to the
*       message received in Quick Mode state 2.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_SELECTOR    Address type in selector is
*                               invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_2_Send(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_ENC_MESSAGE         *out;
    IKE_DEC_MESSAGE         *in;
    IKE_NONCE_ENC_PAYLOAD   enc_nonce;
    IKE_KEYXCHG_ENC_PAYLOAD enc_key;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Sending Quick mode reply");

    /* Set local pointers to commonly used data in the Handle. */
    in  = &ph2->ike_params->ike_in;
    out = &ph2->ike_params->ike_out;

    /* Partially set the remaining outbound payloads
     * used in this state.
     */
    IKE_SET_OUTBOUND(out->ike_nonce, &enc_nonce);
    IKE_SET_OUTBOUND(out->ike_key, &enc_key);
    IKE_SET_OUTBOUND(out->ike_id_i, &IKE_Large_Data.ike_enc_idi);
    IKE_SET_OUTBOUND(out->ike_id_r, &IKE_Large_Data.ike_id.ike_enc_idr);

    /* Generate Nonce data and add its payload to the chain. */
    status = IKE_Generate_Nonce_Data2(ph2);

    if(status == NU_SUCCESS)
    {
        /* If PFS is being used. */
        if(ph2->ike_group_desc != IKE_GROUP_NONE)
        {
            /* Generate key exchange data. */
            status = IKE_Generate_KeyXchg_Data2(ph2);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Check if ID payloads were received. */
            if(IKE_PAYLOAD_IS_PRESENT(in->ike_id_i) == NU_TRUE)
            {
                /* Add ID payloads to the response message. */
                status = IKE_Generate_ID_Data2(ph2);
            }

            /* If no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Terminate chain of payloads. */
                IKE_END_CHAIN(out->ike_last);

                /* Initialize ISAKMP header. */
                IKE_Set_Header(out->ike_hdr, ph2->ike_msg_id,
                               ph2->ike_sa->ike_cookies,
                               IKE_XCHG_QUICK, in->ike_hdr->ike_flags);

                /* Get hash digest length. */

                status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[ph2->ike_sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                               &(out->ike_hash->ike_hash_data_len));

                if(status == NU_SUCCESS)
                {
                    /* Send the message. */
                    status = IKE_Send_Phase2_Packet(ph2);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to send IKE packet",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to get hash digest length",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to generate ID payloads",
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
        NLOG_Error_Log("Failed to generate Nonce payload",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_2_Send */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_3
*
* DESCRIPTION
*
*       This function implements Quick Mode state 3 of the
*       IKE state machine. It is executed by the Initiator.
*       Response to the proposal is received and verified.
*       If verification is successful, the key material is
*       generated for the selection and IPsec SAs are
*       established. IPsec SA establishment is delayed until
*       the next (optional) state if the commit bit was set
*       during the phase 2 exchange. Finally, the HASH(3)
*       data is sent to the Responder.
*
*       Following are payloads received/sent in this state:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR*, HASH(2), SA, Nr
*                                         [, KE] [, IDci, IDcr]
*       HDR*, HASH(3)           -->
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     Payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed the
*                               defined limit.
*       IKE_TOO_MANY_TRANSFORMS Number of transforms exceed the
*                               defined limit.
*       IKE_VERIFY_FAILED       Hash verification failed.
*       IKE_INVALID_SPI         SPI in proposal is invalid.
*       IKE_SA2_NOT_FOUND       SA2 item not found in Handle.
*       IKE_INVALID_PROPOSAL    Selection contains no proposal.
*       IKE_MISSING_ATTRIB      Required attribute is missing.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_3(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status;
    IKE_DEC_MESSAGE     *in;
    IKE_STATE_PARAMS    *params;
    IKE_ID_DEC_PAYLOAD  dec_idi;
    IKE_ID_DEC_PAYLOAD  dec_idr;
    UINT8               msg_hash[IKE_MD5_DIGEST_LEN];

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Quick mode: state 3");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    params = ph2->ike_params;

    /* Make sure packet pointer is not NULL. */
    if(params->ike_packet == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Calculate hash of the encrypted message. */
        status = IKE_HASH_ENC_RESEND(ph2, msg_hash);

        if(status == NU_SUCCESS)
        {
            /* Decrypt the message. */
            status = IKE_Encrypt(ph2->ike_sa,
                         params->ike_packet->ike_data + IKE_HDR_LEN,
                         params->ike_packet->ike_data_len - IKE_HDR_LEN,
                         NU_NULL, ph2->ike_decryption_iv,
                         ph2->ike_encryption_iv, IKE_DECRYPT);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decrypt incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to calculate IKE message hash",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Partially set inbound payloads used in this state. */
            IKE_SET_INBOUND_REQUIRED(in->ike_sa,
                                     &IKE_Large_Data.ike_dec_sa);
            IKE_SET_INBOUND_OPTIONAL(in->ike_id_i, &dec_idi);
            IKE_SET_INBOUND_OPTIONAL(in->ike_id_r, &dec_idr);

            /* Receive and process the payloads. */
            status = IKE_Quick_State_2_3_Recv(ph2);

            if(status == NU_SUCCESS)
            {
                /* Since the message has been authenticated,
                 * the decryption IV can now be synchronized.
                 */
                IKE_SYNC_IV(ph2->ike_decryption_iv,
                            ph2->ike_encryption_iv, ph2->ike_sa);

                /* Process received payloads. */
                status = IKE_Quick_State_3_Process(ph2);

                if(status == NU_SUCCESS)
                {
                    status = IKE_Quick_State_3_Send(ph2);

                    if(status == NU_SUCCESS)
                    {
                        /* Update hash of last message received. */
                        IKE_UPDATE_ENC_RESEND(ph2, msg_hash);

                        /* Now its safe to synchronize encryption IV. */
                        IKE_SYNC_IV(ph2->ike_encryption_iv,
                                    ph2->ike_decryption_iv,
                                    ph2->ike_sa);

                        /* If the commit bit is not set. */
                        if((in->ike_hdr->ike_flags & IKE_HDR_COMMIT_MASK)
                           == 0)
                        {
                            /* Set state as completed. */
                            ph2->ike_xchg_state = IKE_COMPLETE_STATE;
                        }

                        else
                        {
                            /* Move to next state. */
                            ph2->ike_xchg_state += IKE_NEXT_STATE_INC;
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to send IKE message",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to process incoming message",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to decode/verify incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_3 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_3_Process
*
* DESCRIPTION
*
*       This function processes the payloads received
*       in Quick Mode state 2. It looks up the IPsec
*       Policy using either the ID payloads or creating
*       a selector from the source and destination
*       IP addresses. Once the Policy is found, presence
*       checks are performed on payloads. Finally, a
*       selection is made from the received SA payload.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_INVALID_SPI         SPI in proposal is invalid.
*       IKE_SA2_NOT_FOUND       SA2 item not found in Handle.
*       IKE_TOO_MANY_PROPOSALS  Number of proposals exceed limit.
*       IKE_INVALID_PROPOSAL    Selection contains no proposal.
*       IKE_MISSING_ATTRIB      Required attribute is missing.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_3_Process(IKE_PHASE2_HANDLE *ph2)
{
    STATUS          status = NU_SUCCESS;
    IKE_DEC_MESSAGE *in;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Processing Quick mode payloads");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph2->ike_params->ike_in;

    /* If IPsec policy specifies PFS. */
    if(ph2->ike_group_desc != IKE_GROUP_NONE)
    {
        /* Make sure the Key Exchange payload was received. */
        if(ph2->ike_dh_remote_key == NU_NULL)
        {
            /* Key Exchange payload not received. */
            status = IKE_MISSING_PAYLOAD;
        }
    }

    /* Otherwise PFS is not to be used. */
    else
    {
        /* The Key Exchange payload should not
         * have been received.
         */
        if(ph2->ike_dh_remote_key != NU_NULL)
        {
            /* Unexpected Key Exchange payload received. */
            status = IKE_UNEXPECTED_PAYLOAD;
        }
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Make sure the response to the proposal is
         * a valid selection.
         */
        status = IKE_Verify_Proposal2(ph2);

        if(status == NU_SUCCESS)
        {
            /* Update the undetermined parameters of the IPsec
             * outbound SAs using the proposal's selection.
             * The only thing missing after this step, in the
             * IPsec outbound SAs, is the key material.
             */
            status = IKE_Process_ISelection(ph2);

            if(status == NU_SUCCESS)
            {
                /* Generate the key material. */
                status = IKE_Phase2_Key_Material(ph2);

                /* If key material generated and if the
                 * commit bit is not set.
                 */
                if(status == NU_SUCCESS)
                {
                    if((in->ike_hdr->ike_flags & IKE_HDR_COMMIT_MASK) == 0)
                    {
                        /* Generate IPsec SAs and add to SADB. */
                        status = IKE_IPS_Generate_SA_Pairs(ph2);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to generate key material",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to process proposal selection",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Proposal verification failed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_3_Process */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_3_Send
*
* DESCRIPTION
*
*       This is a utility function which sends a reply
*       message in response to the message received in
*       state 3 of the Quick Mode State Machine.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_3_Send(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_ENC_MESSAGE         *out;
    IKE_DEC_MESSAGE         *in;
    IKE_ENC_HDR             enc_hdr;
    IKE_HASH_ENC_PAYLOAD    enc_hash;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set local pointers to commonly used data in the Handle. */
    in  = &ph2->ike_params->ike_in;
    out = &ph2->ike_params->ike_out;

    /* Set outbound payloads used in this state. */
    IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
    IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);

    /* Initialize chain of outbound payloads. */
    IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

    /* Get hash digest length. */

    status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[ph2->ike_sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                   &(enc_hash.ike_hash_data_len));

    if(status == NU_SUCCESS)
    {
        /* Add a blank Hash payload to the chain. This would
         * be filled when the packet is being sent.
         */
        IKE_ADD_TO_CHAIN(out->ike_last, &enc_hash, IKE_HASH_PAYLOAD_ID);

        /* Terminate chain of payloads. */
        IKE_END_CHAIN(out->ike_last);

        /* Initialize ISAKMP header. */
        IKE_Set_Header(&enc_hdr, ph2->ike_msg_id, ph2->ike_sa->ike_cookies,
                       IKE_XCHG_QUICK, in->ike_hdr->ike_flags);

        /* Send the message. */
        status = IKE_Send_Phase2_Packet(ph2);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to send IKE packet",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get hash digest length",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_3_Send */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_4
*
* DESCRIPTION
*
*       This function implements Quick Mode state 4 of the
*       IKE state machine. It is executed by the Responder.
*       The Initiator's HASH(3) is received and verified.
*       If the verification is successful, key material is
*       generated and the negotiated IPsec inbound SA is
*       added to IPsec's SADB. If the commit bit was set
*       during the phase 2 exchange, a final message
*       containing a CONNECTED notification is sent to the
*       Initiator.
*
*       Following are payloads received/sent in this state:
*
*       Initiator                       Responder*
*       ---------                       ---------
*       HDR*, HASH(3)           -->
*                               <--     HDR*, HASH(4), N
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     A payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough space in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_VERIFY_FAILED       Hash verification failed.
*       IKE_SA2_NOT_FOUND       No SA2 items found in Handle.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_4(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_ENC_MESSAGE         *out;
    IKE_STATE_PARAMS        *params;
    IKE_HASH_DEC_PAYLOAD    dec_hash;
    IKE_NOTIFY_DEC_PAYLOAD  dec_notify;
    IKE_ENC_HDR             enc_hdr;
    IKE_HASH_ENC_PAYLOAD    enc_hash;
    IKE_NOTIFY_ENC_PAYLOAD  enc_notify;
    UINT8                   msg_hash[IKE_MD5_DIGEST_LEN];

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Responder processing Quick mode: state 4");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    out    = &ph2->ike_params->ike_out;
    params = ph2->ike_params;

    /* Make sure packet pointer is not NULL. */
    if(params->ike_packet == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Calculate hash of the encrypted message. */
        status = IKE_HASH_ENC_RESEND(ph2, msg_hash);

        if(status == NU_SUCCESS)
        {
            /* Decrypt the message. */
            status = IKE_Encrypt(ph2->ike_sa,
                        params->ike_packet->ike_data + IKE_HDR_LEN,
                        params->ike_packet->ike_data_len - IKE_HDR_LEN,
                        NU_NULL, ph2->ike_decryption_iv,
                        ph2->ike_encryption_iv, IKE_DECRYPT);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to decrypt incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to calculate IKE message hash",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Set inbound payloads used in this state. */
            IKE_SET_INBOUND_REQUIRED(in->ike_hash, &dec_hash);
            IKE_SET_INBOUND_OPTIONAL(in->ike_notify, &dec_notify);

            /* Decode all payloads in the packet. */
            status = IKE_Decode_Message(params->ike_packet->ike_data, in);

            if(status == NU_SUCCESS)
            {
                /* Verify the Hash of the message. Message need not
                 * be sent as not required in HASH(3) calculation.
                 */
                status = IKE_Verify_Hash_x(ph2, NU_NULL, 0,
                                           dec_hash.ike_hash_data,
                                           dec_hash.ike_hash_data_len);

                if(status == NU_SUCCESS)
                {
                    /* Since the message is verified, the
                     * decryption IV can now be synchronized.
                     */
                    IKE_SYNC_IV(ph2->ike_decryption_iv,
                                ph2->ike_encryption_iv, ph2->ike_sa);

                    /* Generate the key material. */
                    status = IKE_Phase2_Key_Material(ph2);

                    if(status == NU_SUCCESS)
                    {
                        /* Generate IPsec SAs and add to SADB. */
                        status = IKE_IPS_Generate_SA_Pairs(ph2);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to generate IPsec SAs",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate key material",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("HASH(x) verification failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to decode incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    /* If IPsec SAs established successfully. */
    if(status == NU_SUCCESS)
    {
        /* If the commit bit is set. */
        if((in->ike_hdr->ike_flags & IKE_HDR_COMMIT_MASK) != 0)
        {
            /* Set outbound payloads used in this state. */
            IKE_SET_OUTBOUND(out->ike_hdr, &enc_hdr);
            IKE_SET_OUTBOUND(out->ike_hash, &enc_hash);
            IKE_SET_OUTBOUND(out->ike_notify, &enc_notify);

            /* Initialize chain of outbound payloads. */
            IKE_INIT_CHAIN(out->ike_last, &enc_hdr, IKE_NONE_PAYLOAD_ID);

            /*
             * Add Hash payload.
             */

            /* Get hash digest length. */

            status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[ph2->ike_sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                           &(enc_hash.ike_hash_data_len));

            if(status == NU_SUCCESS)
            {
                /* Add Hash payload to the chain. This would
                 * be filled when the packet is being sent.
                 */
                IKE_ADD_TO_CHAIN(out->ike_last, &enc_hash,
                                 IKE_HASH_PAYLOAD_ID);

                /*
                 * Add Notification payload.
                 */

                /* Initialize all fields of the notification payload. */
                enc_notify.ike_doi             = IKE_DOI_IPSEC;
                enc_notify.ike_notify_type     = IKE_NOTIFY_STAT_CONNECTED;
                enc_notify.ike_notify_data     = NU_NULL;
                enc_notify.ike_notify_data_len = 0;
                enc_notify.ike_spi_len         = IKE_IPS_SPI_LEN;
                enc_notify.ike_protocol_id     =
                    IKE_Protocol_ID_IPS_To_IKE(ph2->ike_sa2_db.ike_flink->
                        ike_ips_security.ipsec_protocol);

                /* Copy SPI to the notification payload. */
                PUT32(enc_notify.ike_spi, 0,
                      ph2->ike_sa2_db.ike_flink->ike_remote_spi);

                /* Add Notification payload to the chain. */
                IKE_ADD_TO_CHAIN(out->ike_last, &enc_notify,
                                 IKE_NOTIFY_PAYLOAD_ID);

                /* Terminate chain of payloads. */
                IKE_END_CHAIN(out->ike_last);

                /* Initialize ISAKMP header. */
                IKE_Set_Header(&enc_hdr, ph2->ike_msg_id,
                               ph2->ike_sa->ike_cookies,
                               IKE_XCHG_QUICK, in->ike_hdr->ike_flags);

                /* Send the message. */
                status = IKE_Send_Phase2_Packet(ph2);

                if(status == NU_SUCCESS)
                {
                    /* Message sent, so synchronize the encryption IV. */
                    IKE_SYNC_IV(ph2->ike_encryption_iv,
                                ph2->ike_decryption_iv, ph2->ike_sa);
                }

                else
                {
                    NLOG_Error_Log("Failed to send IKE packet",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to get hash digest length",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* If no error occurred while processing this state. */
        if(status == NU_SUCCESS)
        {
            /* Update hash of last message received. */
            IKE_UPDATE_ENC_RESEND(ph2, msg_hash);

            /* Set state as completed. */
            ph2->ike_xchg_state = IKE_COMPLETE_STATE;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Quick_State_4 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Quick_State_5
*
* DESCRIPTION
*
*       This function implements Quick Mode state 5 of the
*       IKE state machine. This additional mode is executed
*       only if the commit bit was set during phase 2. A
*       notification payload is received by the Initiator
*       containing the CONNECTED message. This ensures that
*       the last message of the exchange was successfully
*       processed and the IPsec SA was established.
*
*       Following are payloads received in this state:
*
*       Initiator*                      Responder
*       ---------                       ---------
*                               <--     HDR*, HASH(4), N
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PAYLOAD     A payload is invalid.
*       IKE_LENGTH_IS_SHORT     Not enough data in buffer.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  An unexpected payload encountered.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_VERIFY_FAILED       Hash verification failed.
*       IKE_SA2_NOT_FOUND       No SA2 items found in Handle.
*
*************************************************************************/
STATIC STATUS IKE_Quick_State_5(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_DEC_MESSAGE         *in;
    IKE_STATE_PARAMS        *params;
    UINT16                  real_len;
    IKE_HASH_DEC_PAYLOAD    dec_hash;
    IKE_NOTIFY_DEC_PAYLOAD  dec_notify;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Initiator processing Quick mode: state 5");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    params = ph2->ike_params;

    /* Make sure packet pointer is not NULL. */
    if(params->ike_packet == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Decrypt the message. */
        status = IKE_Encrypt(ph2->ike_sa,
                     params->ike_packet->ike_data + IKE_HDR_LEN,
                     params->ike_packet->ike_data_len - IKE_HDR_LEN,
                     NU_NULL, ph2->ike_decryption_iv,
                     ph2->ike_encryption_iv, IKE_DECRYPT);

        if(status == NU_SUCCESS)
        {
            /* Set inbound payloads used in this state. */
            IKE_SET_INBOUND_REQUIRED(in->ike_hash, &dec_hash);
            IKE_SET_INBOUND_REQUIRED(in->ike_notify, &dec_notify);

            /* Decode all payloads in the packet. */
            status = IKE_Decode_Message(params->ike_packet->ike_data, in);

            if(status == NU_SUCCESS)
            {
                /* Get length of the message data without padding. */
                status = IKE_Get_Message_Length(
                             params->ike_packet->ike_data,
                             params->ike_packet->ike_data_len,
                             &real_len);

                if(status == NU_SUCCESS)
                {
                    /* Verify the Hash of the message. */
                    status = IKE_Verify_Hash_x(ph2,
                                 params->ike_packet->ike_data, real_len,
                                 dec_hash.ike_hash_data,
                                 dec_hash.ike_hash_data_len);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("HASH(x) verification failed",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Unable to calculate message length",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* If Hash payload verified successfully. */
            if(status == NU_SUCCESS)
            {
                /* Since the message is verified, the
                 * decryption IV can now be synchronized.
                 */
                IKE_SYNC_IV(ph2->ike_decryption_iv,
                            ph2->ike_encryption_iv, ph2->ike_sa);

                /* Make sure notification type is CONNECTED. */
                if(dec_notify.ike_notify_type != IKE_NOTIFY_STAT_CONNECTED)
                {
                    status = IKE_UNEXPECTED_PAYLOAD;
                }

                else
                {
                    /* Generate IPsec SAs and add to SADB. */
                    status = IKE_IPS_Generate_SA_Pairs(ph2);

                    if(status == NU_SUCCESS)
                    {
                        /* Set state as completed. */
                        ph2->ike_xchg_state = IKE_COMPLETE_STATE;
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to generate IPsec SAs",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
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

} /* IKE_Quick_State_5 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Quick_Mode
*
* DESCRIPTION
*
*       This function handles the IKE Quick Mode negotiation
*       in a Phase 2 exchange.
*
* INPUTS
*
*       *ph2                    Pointer to Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_SA_NOT_FOUND        SA not passed to this function.
*       IKE_INVALID_MSGID       Received a Message ID of zero.
*       IKE_INVALID_STATE       IKE SA state is not complete.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_COOKIE      One of the cookies is invalid.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_IS_RESEND           Message is a retransmission.
*       IKE_UNEXPECTED_MESSAGE  Exchange is already complete.
*       Exchange Status         If the state handler was called,
*                               status of the current state
*                               processing is returned.
*
*************************************************************************/
STATUS IKE_Process_Quick_Mode(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status = NU_SUCCESS;
    UINT8               state;
    IKE_STATE_PARAMS    *params;
    IKE_DEC_HDR         *hdr;

#if (IKE_DEBUG == NU_TRUE)
    /* Check that the Handle pointer is not NULL. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be passed to this function. */
    else if(ph2->ike_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Valid state must be passed to this function. */
    else if(ph2->ike_xchg_state == NU_NULL ||
                (ph2->ike_xchg_state > IKE_TOTAL_QUICK_MODE_STATES &&
                        ph2->ike_xchg_state != IKE_COMPLETE_STATE))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the policy pointer is valid. */
    else if(ph2->ike_params->ike_policy == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the SA pointer is valid. */
    else if(ph2->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        return (IKE_SA_NOT_FOUND);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("QUICK MODE");

    /* Set local pointers to commonly used data in the Handle. */
    params = ph2->ike_params;
    hdr    = ph2->ike_params->ike_in.ike_hdr;

    /* Make sure IKE SA is in the completed state. */
    if(ph2->ike_sa->ike_state != IKE_SA_ESTABLISHED)
    {
        NLOG_Error_Log("IKE SA has invalid state",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_INVALID_STATE;
    }

    else
    {
        /* If ISAKMP header is not NULL. */
        if(hdr != NU_NULL)
        {
            /* Make sure message ID is non-zero. */
            if(hdr->ike_msg_id == 0)
            {
                NLOG_Error_Log("Message ID zero in Quick Mode",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_INVALID_MSGID;
            }

            /* All Quick mode messages must be encrypted. */
            else if((hdr->ike_flags & IKE_HDR_ENC_MASK) == 0)
            {
                NLOG_Error_Log("Discarding un-encrypted Quick mode packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_INVALID_FLAGS;
            }

            /* Check if authentication bit set in ISAKMP header. */
            else if((hdr->ike_flags & IKE_HDR_AUTH_MASK) != 0)
            {
                NLOG_Error_Log("Invalid flags in ISAKMP header",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_INVALID_FLAGS;
            }

            /* Responder cookie must be set in Phase 2. */
            else if(!IKE_COOKIE_IS_SET(hdr->ike_rcookie))
            {
                NLOG_Error_Log("Cookie in ISAKMP header is invalid",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_INVALID_COOKIE;
            }

            /* Make sure first payload is a Hash payload. */
            else if(hdr->ike_first_payload != IKE_HASH_PAYLOAD_ID)
            {
                NLOG_Error_Log("HASH(x) missing in Phase 2 message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_MISSING_PAYLOAD;
            }
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Check that the packet is not NULL. SA2 has
             * been searched above and would be valid if
             * the following statement is true.
             */
            if(params->ike_packet != NU_NULL)
            {
                /* Make sure message is not a retransmission. Cast
                 * the Phase 2 Handle to a Phase 1 Handle because
                 * both use a common interface for message
                 * retransmission.
                 */
                status = IKE_Check_Resend((IKE_PHASE1_HANDLE*)ph2);
            }
        }
    }

    /* If no error occurred till here. */
    if((status == NU_SUCCESS) || (status == IKE_NOT_FOUND))
    {
        /* Get current exchange state from the Handle. */
        state = ph2->ike_xchg_state;

        /* Make sure state is not already complete. */
        if(state == IKE_COMPLETE_STATE)
        {
            NLOG_Error_Log(
                "Unexpected message - Phase 2 is already complete",
                NERR_RECOVERABLE, __FILE__, __LINE__);

            /* No more messages expected on this handle. */
            status = IKE_UNEXPECTED_MESSAGE;
        }

        else
        {
            /* Call the state handler. */
            status = IKE_Quick_Mode_Handlers[(INT)state - 1](ph2);

            /* If the exchange failed. */
            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Quick Mode failed - aborting",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                /* Update status of the current phase 2 exchange. */
                if(IKE_Update_Phase2_Status(ph2, status) == NU_SUCCESS)
                {
                    IKE_DEBUG_LOG("Updated Phase 2 failure status");
                }
#endif

                /* Make sure this is not the local copy of the
                 * Phase 2 Handle which is passed to State 2 of
                 * the Quick Mode state machine.
                 */
                if(ph2 != &IKE_Large_Data.ike_phase2)
                {
                    /* Mark the Handle as deleted. */
                    ph2->ike_flags |= IKE_DELETE_FLAG;

                    /* Enqueue event to remove the Handle because the
                     * exchange failed. The Handle is removed through
                     * the event queue to keep all events synchronized.
                     *
                     * Note that this event will not be processed
                     * until the current packet is being handled
                     * because the IKE semaphore is obtained. Therefore
                     * the Phase 2 Handle could be accessed by the
                     * caller when this function returns.
                     */
                    IKE_SYNC_REMOVE_PHASE2(ph2->ike_sa, ph2);
                }

                /* Otherwise if the Handle was added to the database
                 * during State 2 processing.
                 */
                else if(ph2->ike_flink != NU_NULL)
                {
                    /* Mark the Handle as deleted. */
                    ph2->ike_flink->ike_flags |= IKE_DELETE_FLAG;

                    /* Same as above, except that the actual Handle
                     * pointer is stored in the front link.
                     */
                    IKE_SYNC_REMOVE_PHASE2(ph2->ike_flink->ike_sa,
                                           ph2->ike_flink);
                }

                /* Otherwise this is the local copy of the Handle. */
                else
                {
                    /* Deallocate all dynamically allocated memory
                     * because the state machine is responsible for the
                     * Handle as long as it is not added to the
                     * database.
                     */
                    if(IKE_Free_Local_Phase2(ph2) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to deallocate local Handle",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_Quick_Mode */

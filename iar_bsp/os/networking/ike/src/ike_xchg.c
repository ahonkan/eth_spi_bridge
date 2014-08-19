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
*       ike_xchg.c
*
* COMPONENT
*
*       IKE - IKE Exchange
*
* DESCRIPTION
*
*       This file contains shared code between the different
*       IKE Exchange Modes.
*
* DATA STRUCTURES
*
*       IKE_Large_Data          Large data structures declared
*                               globally to reduce stack usage.
*
* FUNCTIONS
*
*       IKE_Construct_Proposal
*       IKE_Select_Proposal
*       IKE_Convert_Attributes
*       IKE_Verify_Attributes
*       IKE_Verify_Selection_SA
*       IKE_Auth_Parameters
*       IKE_Generate_KeyXchg_Data
*       IKE_Generate_Nonce_Data
*       IKE_Generate_Cert_Data
*       IKE_Generate_CertReq_Data
*       IKE_Get_Local_ID
*       IKE_Generate_ID_Data
*       IKE_Generate_Hash_Data
*       IKE_Verify_Auth_Data
*       IKE_Aggr_4_Main_6_7_Recv
*       IKE_Payload_To_Identifier
*       IKE_Lookup_Preshared_Key
*       IKE_Free_Phase1_Memory
*       IKE_Free_Phase2_Memory
*       IKE_Finalize_Phase1
*       IKE_Finalize_Phase2
*       IKE_Resume_Waiting_Process
*       IKE_Resume_Waiting_Processes
*       IKE_Update_Phase2_Status
*       IKE_Update_Phase1_Status
*       IKE_Check_Resend
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_api.h
*       ike_pkt.h
*       ike_enc.h
*       ike_auth.h
*       ike_evt.h
*       ike_ips.h
*       ike_cert.h
*       ike_crypto_wrappers.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_pkt.h"
#include "networking/ike_enc.h"
#include "networking/ike_auth.h"
#include "networking/ike_evt.h"
#include "networking/ike_ips.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
#include "networking/ike_cert.h"
#endif

/* Make sure the default phase 1 exchange is valid. */
#if ((IKE_PHASE1_DEFAULT_XCHG != IKE_XCHG_MAIN) && \
     (IKE_PHASE1_DEFAULT_XCHG != IKE_XCHG_AGGR))
#error Default phase 1 exchange must be set to Main or Aggressive mode.
#endif

/* Make sure the default phase 1 exchange is valid. */
#if (((IKE_INCLUDE_MAIN_MODE == NU_FALSE) && \
      (IKE_PHASE1_DEFAULT_XCHG == IKE_XCHG_MAIN)) || \
     ((IKE_INCLUDE_AGGR_MODE == NU_FALSE) && \
      (IKE_PHASE1_DEFAULT_XCHG == IKE_XCHG_AGGR)))
#error Default phase 1 exchange mode is disabled.
#endif

/* Large data structures used by IKE Exchanges. */
IKE_LARGE_DATA_STRUCTS  IKE_Large_Data;

/*************************************************************************
*
* FUNCTION
*
*       IKE_Construct_Proposal
*
* DESCRIPTION
*
*       This is a utility function used by Phase 1 Initiator. It
*       takes an IKE policy and constructs an SA payload from it.
*       The SA payload is suitable to initiate a negotiation.
*
*       Following payloads are appended to the chain:
*       - IKE_SA_ENC_PAYLOAD
*
* INPUTS
*
*       *ph1                    Phase 1 Handle containing the
*                               IKE SA and policy.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Construct_Proposal(IKE_PHASE1_HANDLE *ph1)
{
    INT                     i;
    UINT8                   attrib_num;
    IKE_POLICY              *policy;
    IKE_ATTRIB              *policy_attrib;
    IKE_TRANSFORM_PAYLOAD   *out_transform;
    IKE_SA_ENC_PAYLOAD      *out_sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be present. */
    else if(ph1->ike_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Constructing Initiator's proposal");

    /* Set local pointer to commonly used data in the Handle. */
    policy = ph1->ike_params->ike_policy;
    out_sa = ph1->ike_params->ike_out.ike_sa;

    /* Loop for each attribute group in the policy. */
    for(i = 0; i < policy->ike_xchg1_attribs_no; i++)
    {
        /* Set pointer to the current policy attribute. */
        policy_attrib = &policy->ike_xchg1_attribs[i];

        /* Set pointer to current transform payload. */
        out_transform = &out_sa->ike_proposals[0].ike_transforms[i];

        /* Initialize attribute count. */
        attrib_num = 0;

        /* Add encryption algorithm attribute. */
        IKE_Add_Attribute(IKE_ATTRIB_ENC_ALGO,
                          out_transform->ike_sa_attributes, &attrib_num,
                          policy_attrib->ike_encryption_algo);

        /* Add hash algorithm attribute. */
        IKE_Add_Attribute(IKE_ATTRIB_HASH_ALGO,
                          out_transform->ike_sa_attributes, &attrib_num,
                          policy_attrib->ike_hash_algo);

        /* Add authentication method attribute. */
        IKE_Add_Attribute(IKE_ATTRIB_AUTH_METHOD,
                          out_transform->ike_sa_attributes, &attrib_num,
                          policy_attrib->ike_auth_method);

        /* Add group description attribute. */
        IKE_Add_Attribute(IKE_ATTRIB_GRP_DESC,
                          out_transform->ike_sa_attributes, &attrib_num,
                          policy_attrib->ike_group_desc);

        /* If encryption algorithm key length specified. */
        if(policy_attrib->ike_key_len != IKE_WILDCARD)
        {
            /* Add key length attribute. */
            IKE_Add_Attribute(IKE_ATTRIB_KEY_LEN,
                out_transform->ike_sa_attributes, &attrib_num,
                (UINT16)IKE_BYTES_TO_BITS(policy_attrib->ike_key_len));
        }

        /* If Lifetime specified in number of seconds. */
        if(policy_attrib->ike_sa_lifetime.ike_no_of_secs != IKE_WILDCARD)
        {
            /* Add Lifetime type attribute. */
            IKE_Add_Attribute(IKE_ATTRIB_LIFE_TYPE,
                              out_transform->ike_sa_attributes,
                              &attrib_num, IKE_IPS_VAL_SECS);

            /* Add Lifetime duration (seconds) attribute. */
            IKE_Add_Variable_Attribute(IKE_ATTRIB_LIFE_DURATION,
                out_transform->ike_sa_attributes, &attrib_num,
                policy_attrib->ike_sa_lifetime.ike_no_of_secs,
                (UINT8 *)&(policy_attrib->ike_sa_lifetime.
                             ike_attrib_secs_buffer));
        }

        /* Initialize current Transform payload. */
        out_transform->ike_num_attributes = attrib_num;
        out_transform->ike_transform_no   = (UINT8)(i + 1);
        out_transform->ike_transform_id   = IKE_IPS_TRANS_IKE_KE;
    }

    /* Initialize fields of the SA payload. */
    out_sa->ike_num_proposals        = IKE_MAX_PHASE1_PROPOSALS;
    out_sa->ike_doi                  = IKE_DOI_IPSEC;
    out_sa->ike_situation_len        = IKE_IPS_SITUATION_LEN;

    /* Set situation to ID only. */
    PUT32(out_sa->ike_situation, 0, IKE_IPS_SIT_ID_ONLY);

    /* Initialize fields of the first (and only) proposal. */
    out_sa->ike_proposals[0].ike_num_transforms = (UINT8)i;
    out_sa->ike_proposals[0].ike_protocol_id    = IKE_PROTO_ISAKMP;
    out_sa->ike_proposals[0].ike_spi_len        = IKE_PHASE1_SPI_LEN;
    out_sa->ike_proposals[0].ike_proposal_no    =
        IKE_PHASE1_PROPOSAL_NUMBER;

    /* Add SA payload to the payloads chain. */
    IKE_ADD_TO_CHAIN(ph1->ike_params->ike_out.ike_last,
                     out_sa, IKE_SA_PAYLOAD_ID);

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Construct_Proposal */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Select_Proposal
*
* DESCRIPTION
*
*       This is a utility function used by Phase 1 Responder. It
*       takes an SA payload with multiple transforms and matches
*       those against an IKE Policy. If a match is found, an
*       IKE SA payload suitable for a reply to the negotiation
*       is returned. Note that variable length attributes of
*       outbound SA payload share the same buffer space as the
*       inbound SA payload.
*
*       Following payloads are appended to the chain:
*       - IKE_SA_ENC_PAYLOAD
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*                               On return, this contains an SA
*                               payload suitable for reply to
*                               the decoded one.
*       **ret_policy_attrib     Pointer to the group of Policy
*                               attributes matching the proposal.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_PROTOCOL    Protocol is not ISAKMP.
*       IKE_INVALID_TRANSFORM   Only IKE_KEY transform is allowed.
*       IKE_TOO_MANY_PROPOSALS  The SA has more than one proposals.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_NOT_NEGOTIABLE      Proposal not negotiable using
*                               the specified policy.
*
*************************************************************************/
STATUS IKE_Select_Proposal(IKE_PHASE1_HANDLE *ph1,
                           IKE_ATTRIB **ret_policy_attrib)
{
    STATUS                  status = NU_SUCCESS;
    INT                     i;
    INT                     j = 0;
    INT                     k;
    IKE_SA_DEC_PAYLOAD      *in_sa;
    IKE_SA_ENC_PAYLOAD      *out_sa;
    IKE_TRANSFORM_PAYLOAD   *in_transform = NU_NULL;
    IKE_TRANSFORM_PAYLOAD   *out_transform;
    IKE_DATA_ATTRIB         *in_attrib;
    IKE_DATA_ATTRIB         *out_attrib;
    IKE_ATTRIB              *policy_attrib;
    UINT8                   is_match = NU_FALSE;
    UINT8                   attrib_flags;
    UINT32                  attrib_val;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((ph1 == NU_NULL) || (ret_policy_attrib == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Making selection from Initiator's proposal");

    /* Set local pointers to commonly used data in the Handle. */
    in_sa  = ph1->ike_params->ike_in.ike_sa;
    out_sa = ph1->ike_params->ike_out.ike_sa;

    /* Make sure only a single proposal is present in input SA. */
    if((in_sa->ike_num_proposals != IKE_MAX_PHASE1_PROPOSALS)
#if ((IKE_DECODE_PARTIAL_SA == NU_TRUE) && (IKE_MAX_PROPOSALS == 1))
       || (in_sa->ike_partial_proposals == NU_TRUE)
#endif
       )
    {
        NLOG_Error_Log("Multiple proposals in Phase 1 not allowed",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_TOO_MANY_PROPOSALS;
    }

    /* Protocol being negotiated must be ISAKMP. */
    else if(in_sa->ike_proposals[0].ike_protocol_id != IKE_PROTO_ISAKMP)
    {
        NLOG_Error_Log("Protocol must be ISAKMP in Phase 1 proposal",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_INVALID_PROTOCOL;
    }

    else
    {
        /* Loop for each transform in the input SA payload. */
        for(i = 0;
            (i < in_sa->ike_proposals[0].ike_num_transforms) &&
            (i < IKE_MAX_TRANSFORMS);
            i++)
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("Matching proposal's transform...");

            /* Get pointer to current transform. */
            in_transform = &in_sa->ike_proposals[0].ike_transforms[i];

            /* If this is not the first transform iteration. */
            if(i != 0)
            {
                /* Make sure transform numbers are monotonically
                 * increasing.
                 */
                if(in_sa->ike_proposals[0].ike_transforms[i - 1].
                       ike_transform_no + 1 !=
                   in_sa->ike_proposals[0].ike_transforms[i].
                       ike_transform_no)
                {
                    status = IKE_INVALID_TRANSFORM;
                    break;
                }
            }

            /* Make sure transform ID is valid. */
            if(in_transform->ike_transform_id != IKE_IPS_TRANS_IKE_KE)
            {
                status = IKE_INVALID_TRANSFORM;
                break;
            }

            /* Loop for each attribute entry in the policy. */
            for(j = 0;
                j < ph1->ike_params->ike_policy->ike_xchg1_attribs_no;
                j++)
            {
                /* Set match flag to TRUE. */
                is_match = NU_TRUE;

                /* Set attribute presence flag to zero. This is
                 * used to check for conflicting attribute types.
                 */
                attrib_flags = 0;

                /* Get pointer to policy attribute. */
                policy_attrib =
                    &ph1->ike_params->ike_policy->ike_xchg1_attribs[j];

                /* Loop for each attribute of this transform. */
                for(k = 0; k < in_transform->ike_num_attributes; k++)
                {
                    /* Get pointer to current attribute. */
                    in_attrib = &in_transform->ike_sa_attributes[k];

                    /* Determine the attribute type. */
                    switch(in_attrib->ike_attrib_type)
                    {
                    case IKE_ATTRIB_ENC_ALGO:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_ENC_ALGO_FLAG) != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Check if policy allows this algorithm. */
                        else if(policy_attrib->ike_encryption_algo !=
                                in_attrib->ike_attrib_lenval)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Encryption algorithm mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_ENC_ALGO_FLAG;
                        break;

                    case IKE_ATTRIB_HASH_ALGO:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_HASH_ALGO_FLAG) != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Check if policy allows this algorithm. */
                        else if(policy_attrib->ike_hash_algo !=
                                in_attrib->ike_attrib_lenval)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Hash algorithm mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_HASH_ALGO_FLAG;
                        break;

                    case IKE_ATTRIB_AUTH_METHOD:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_AUTH_METHOD_FLAG)
                           != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Check if policy allows this method. */
                        else if(policy_attrib->ike_auth_method !=
                                in_attrib->ike_attrib_lenval)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Authenticate method mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_AUTH_METHOD_FLAG;
                        break;

                    case IKE_ATTRIB_LIFE_TYPE:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_LIFE_TYPE_FLAG) != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Lifetime must be in seconds. */
                        else if(in_attrib->ike_attrib_lenval !=
                                IKE_IPS_VAL_SECS)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Lifetime type mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_LIFE_TYPE_FLAG;
                        break;

                    case IKE_ATTRIB_KEY_LEN:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_KEY_LEN_FLAG) != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Check if key length is less than the
                         * minimum possible key length allowed in
                         * the policy.
                         */
                        else if(
                           (policy_attrib->ike_key_len != IKE_WILDCARD) &&
                           (IKE_BYTES_TO_BITS(policy_attrib->ike_key_len) >
                            in_attrib->ike_attrib_lenval))
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Encrypt key length mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_KEY_LEN_FLAG;
                        break;

                    case IKE_ATTRIB_GRP_DESC:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_GRP_DESC_FLAG) != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Check if this group is supported. */
                        else if(policy_attrib->ike_group_desc !=
                                in_attrib->ike_attrib_lenval)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Group description mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_GRP_DESC_FLAG;
                        break;

                    case IKE_ATTRIB_GRP_TYPE:
                        /* Make sure this attribute occurs just once. */
                        if((attrib_flags & IKE_ATTRIB_GRP_TYPE_FLAG) != 0)
                        {
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        /* Check if the group type is supported. */
                        else if(in_attrib->ike_attrib_lenval !=
                                IKE_VAL_MODP)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Group type mismatch");

                            is_match = NU_FALSE;
                        }

                        /* Mark the attribute presence flag. */
                        attrib_flags |= IKE_ATTRIB_GRP_TYPE_FLAG;
                        break;

                    default:
                        /* The life duration attribute may have been
                         * encoded as either variable or a basic type.
                         */
                        if((in_attrib->ike_attrib_type &
                            IKE_ATTRIB_TYPE_MASK) ==
                           IKE_ATTRIB_LIFE_DURATION)
                        {
                            /* Make sure this attribute occurs once. */
                            if((attrib_flags & IKE_ATTRIB_LIFE_DUR_FLAG)
                               != 0)
                            {
                                status = IKE_UNSUPPORTED_ATTRIB;
                                break;
                            }

                            /* Make sure value length is manageable. */
                            else if(IKE_Decode_Attribute_Value(&attrib_val,
                                        in_attrib) != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Lifetime mismatch (too large)",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                                /* Value too large. */
                                is_match = NU_FALSE;
                                break;
                            }

                            /* Check if lifetime is greater than the
                             * maximum possible allowed by policy.
                             */
                            if((policy_attrib->ike_sa_lifetime.
                                    ike_no_of_secs != IKE_WILDCARD) &&
                               (policy_attrib->ike_sa_lifetime.
                                    ike_no_of_secs < attrib_val))
                            {
                                /* Log debug message. */
                                IKE_DEBUG_LOG("Lifetime mismatch");

                                is_match = NU_FALSE;
                            }

                            /* Mark the attribute presence flag. */
                            attrib_flags |= IKE_ATTRIB_LIFE_DUR_FLAG;
                        }

                        else
                        {
                            /* Report error. */
                            status = IKE_UNSUPPORTED_ATTRIB;
                        }

                        break;
                    }

                    /* Break out of loop if error occurred or if
                     * an unacceptable proposal was encountered, so
                     * that the next proposal can be considered.
                     */
                    if((status != NU_SUCCESS) || (is_match == NU_FALSE))
                    {
                        break;
                    }
                }

                /* Break out of loop if error occurred or if
                 * an acceptable proposal was encountered.
                 */
                if((status != NU_SUCCESS) || (is_match == NU_TRUE))
                {
                    break;
                }
            }

            /* Break out of loop if error occurred or
             * if an acceptable proposal was found.
             */
            if((status != NU_SUCCESS) || (is_match == NU_TRUE))
            {
                break;
            }
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /* Check if the proposal is not acceptable. */
            if(is_match == NU_FALSE)
            {
                /* Log debug message. */
                IKE_DEBUG_LOG("Unable to find suitable proposal");

                /* Report negotiation failure. */
                status = IKE_NOT_NEGOTIABLE;
            }

            else
            {
                /* Log debug message. */
                IKE_DEBUG_LOG("Suitable proposal selected");

                /* Set pointer to the Policy attributes which
                 * matched the proposal.
                 */
                *ret_policy_attrib =
                    &ph1->ike_params->ike_policy->ike_xchg1_attribs[j];

                /* Set fields of the SA payload for reply. */
                out_sa->ike_num_proposals = IKE_MAX_PHASE1_PROPOSALS;
                out_sa->ike_doi           = in_sa->ike_doi;
                out_sa->ike_situation_len = in_sa->ike_situation_len;

                /* Copy the situation from the input SA. */
                NU_BLOCK_COPY(out_sa->ike_situation,
                    in_sa->ike_situation,
                    in_sa->ike_situation_len);

                /* Copy the first proposal. */
                out_sa->ike_proposals[0].ike_num_transforms =
                    IKE_SELECTED_PHASE1_TRANSFORMS;
                out_sa->ike_proposals[0].ike_proposal_no    =
                    in_sa->ike_proposals[0].ike_proposal_no;
                out_sa->ike_proposals[0].ike_protocol_id    =
                    in_sa->ike_proposals[0].ike_protocol_id;
                out_sa->ike_proposals[0].ike_spi_len        =
                    in_sa->ike_proposals[0].ike_spi_len;

                /* Copy SPI from the input SA. */
                NU_BLOCK_COPY(out_sa->ike_proposals[0].ike_spi,
                    in_sa->ike_proposals[0].ike_spi,
                    in_sa->ike_proposals[0].ike_spi_len);

                /* Copy the selected transform of the proposal. */
                out_transform                       =
                    &out_sa->ike_proposals[0].ike_transforms[0];
                out_transform->ike_num_attributes   =
                    in_transform->ike_num_attributes;
                out_transform->ike_transform_no     =
                    in_transform->ike_transform_no;
                out_transform->ike_transform_id     =
                    in_transform->ike_transform_id;

                /* Loop for each attribute. */
                for(i = 0; i < in_transform->ike_num_attributes; i++)
                {
                    /* Set pointers to source and destination. */
                    in_attrib  = &in_transform->ike_sa_attributes[i];
                    out_attrib = &out_transform->ike_sa_attributes[i];

                    /* Convert the attribute. */
                    out_attrib->ike_attrib_type   =
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

                /* Add SA payload to the payloads chain. */
                IKE_ADD_TO_CHAIN(ph1->ike_params->ike_out.ike_last,
                                 out_sa, IKE_SA_PAYLOAD_ID);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Select_Proposal */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Convert_Attributes
*
* DESCRIPTION
*
*       This is a utility function which converts the
*       attributes used in the Transform payload to
*       equivalent attributes used in the IKE SA.
*
*       Note that ALL fields of the output attribute
*       are initialized to zero before they are set,
*       so the caller should not store any data in
*       'sa_attrib' before calling this function.
*
* INPUTS
*
*       *attribs                An array of attributes which
*                               are to be converted.
*       num_attribs             Number of attributes in the
*                               attribute array.
*       *sa_attrib              On return, this contains the
*                               equivalent of SA payload's
*                               negotiated attributes.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_KEYLEN      Negotiated encryption key
*                               length is invalid.
*       IKE_MISSING_ATTRIB      One or more of the required
*                               attributes are missing in the
*                               negotiation.
*       IKE_UNSUPPORTED_ATTRIB  An unrecognized attribute
*                               was encountered.
*       IKE_UNSUPPORTED_ALGO    Algorithm used in attribute is
*                               not supported by IKE.
*       IKE_ATTRIB_TOO_LONG     Attribute length is too long.
*
*************************************************************************/
STATUS IKE_Convert_Attributes(IKE_DATA_ATTRIB *attribs,
                              UINT8 num_attribs,
                              IKE_ATTRIB *sa_attrib)
{
    STATUS              status = NU_SUCCESS;
    INT                 i;
    const IKE_ENCRYPTION_ALGO *enc_algo;
    UINT32              life_duration;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((attribs == NU_NULL) || (sa_attrib == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Converting attributes");

    /* Initialize all attributes to zero. This would set
     * the value of all attributes to IKE_WILDCARD.
     */
    UTL_Zero(sa_attrib, sizeof(IKE_ATTRIB));

    /* Loop for each attribute in the array. */
    for(i = 0; (i<num_attribs) && (status == NU_SUCCESS); i++)
    {
        switch(attribs[i].ike_attrib_type)
        {
        case IKE_ATTRIB_ENC_ALGO:
            /* Set the Encryption algorithm. */
            status =
                IKE_ENCRYPTION_ALGO_INDEX(attribs[i].ike_attrib_lenval,
                                          sa_attrib->ike_encryption_algo);
            break;

        case IKE_ATTRIB_HASH_ALGO:
            /* Set the Hash algorithm. */
            status = IKE_HASH_ALGO_INDEX(attribs[i].ike_attrib_lenval,
                                         sa_attrib->ike_hash_algo);
            break;

        case IKE_ATTRIB_AUTH_METHOD:
            /* Set the Authentication method. */
            sa_attrib->ike_auth_method = attribs[i].ike_attrib_lenval;
            break;

        case IKE_ATTRIB_LIFE_TYPE:
            /* Make sure lifetime is in seconds. */
            if(attribs[i].ike_attrib_lenval != IKE_IPS_VAL_SECS)
            {
                /* Any other lifetime type is not supported. */
                status = IKE_UNSUPPORTED_ATTRIB;
            }
            break;

        case IKE_ATTRIB_KEY_LEN:
            /* Set the key length. */
            sa_attrib->ike_key_len =
                (UINT16)IKE_BITS_TO_BYTES(attribs[i].ike_attrib_lenval);
            break;

        case IKE_ATTRIB_GRP_DESC:
            /* Set the Diffie-Hellman group description. */
            sa_attrib->ike_group_desc = attribs[i].ike_attrib_lenval;
            break;

        default:
            /* The life duration attribute may have been
             * encoded as either variable or a basic type.
             */
            if((attribs[i].ike_attrib_type & IKE_ATTRIB_TYPE_MASK) ==
               IKE_ATTRIB_LIFE_DURATION)
            {
                /* Decode the attribute. */
                status = IKE_Decode_Attribute_Value(&life_duration,
                                                    &attribs[i]);

                /* Set the seconds lifetime duration. */
                sa_attrib->ike_sa_lifetime.ike_no_of_secs = life_duration;
            }

            else
            {
                /* Unrecognized attribute. */
                status = IKE_UNSUPPORTED_ATTRIB;
            }

            break;
        }
    }

    /* If no error occurred till now. */
    if(status == NU_SUCCESS)
    {
        /* Make sure all the required attributes
         * have been negotiated.
         */
        if((sa_attrib->ike_auth_method == IKE_WILDCARD) ||
           (sa_attrib->ike_group_desc  == IKE_WILDCARD))
        {
            NLOG_Error_Log("Required attributes not negotiated",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Set status to error. */
            status = IKE_MISSING_ATTRIB;
        }

        else
        {
            /* If the lifetime has not been negotiated. */
            if(sa_attrib->ike_sa_lifetime.ike_no_of_secs == IKE_WILDCARD)
            {
                /* Log debug message. */
                IKE_DEBUG_LOG("Lifetime not negotiated - using default");

                /* Set lifetime to the default value specified in
                 * the IPsec DOI.
                 */
                sa_attrib->ike_sa_lifetime.ike_no_of_secs =
                    IKE_IPS_DEFAULT_SA_LIFETIME;
            }

            /* Get the Encryption algorithm structure. */
            enc_algo =
                &IKE_Encryption_Algos[sa_attrib->ike_encryption_algo];

            /* If the key length was not negotiated. */
            if(sa_attrib->ike_key_len == IKE_WILDCARD)
            {
                /* Set the key length to zero to request the
                 * default key length.
                 */
                sa_attrib->ike_key_len = 0;

                /* Get default key length. */
                status = IKE_Crypto_Enc_Key_Len(enc_algo->crypto_algo_id,
                                                &(sa_attrib->ike_key_len));
            }

            else
            {
                /* Verify key length. */
                status = IKE_Crypto_Enc_Key_Len(enc_algo->crypto_algo_id,
                                                (&sa_attrib->ike_key_len));

                if(status == IKE_INVALID_KEYLEN)
                {
                    NLOG_Error_Log("Unsupported key length in proposal",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Convert_Attributes */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Verify_Attributes
*
* DESCRIPTION
*
*       This is a utility function used by the Initiator
*       to verify that none of the attributes sent in the
*       initial proposal have been tampered by the
*       Responder. It ensures that the negotiated attributes
*       are one of those which were sent in the proposal.
*
* INPUTS
*
*       *attrib                 The negotiated attributes.
*       *attrib_list            An array of attributes which
*                               the negotiated attributes must
*                               belong to.
*       num_attribs             Number of attributes in the
*                               array.
*       **out_attrib            On return, contains a pointer to
*                               the group of attributes
*                               which match the negotiation.
*
* OUTPUTS
*
*       NU_SUCCESS              If verification successful.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_PROPOSAL_TAMPERED   Proposal has been tampered.
*
*************************************************************************/
STATUS IKE_Verify_Attributes(IKE_ATTRIB *attrib,
                             IKE_ATTRIB *attrib_list,
                             UINT8 num_attribs,
                             IKE_ATTRIB **out_attrib)
{
    STATUS          status = IKE_PROPOSAL_TAMPERED;
    INT             i;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((attrib     == NU_NULL) || (attrib_list == NU_NULL) ||
       (out_attrib == NU_NULL) || (num_attribs == 0))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Verifying proposal selection");

    /* Loop for each attribute in the array. */
    for(i = 0; i < num_attribs; i++)
    {
        /* Compare Encryption algorithm. */
        if(attrib_list[i].ike_encryption_algo != (UINT16)
           IKE_Encryption_Algos[attrib->ike_encryption_algo].
               ike_algo_identifier)
        {
            /* Mismatch. Start over again. */
            continue;
        }

        /* Compare Hash algorithm. */
        else if(attrib_list[i].ike_hash_algo != (UINT16)
                IKE_Hash_Algos[attrib->ike_hash_algo].ike_algo_identifier)
        {
            /* Mismatch. Start over again. */
            continue;
        }

        /* Compare Authentication method. */
        else if(attrib_list[i].ike_auth_method !=
                attrib->ike_auth_method)
        {
            /* Mismatch. Start over again. */
            continue;
        }

        /* Compare Diffie-Hellman group description. */
        else if(attrib_list[i].ike_group_desc !=
                attrib->ike_group_desc)
        {
            /* Mismatch. Start over again. */
            continue;
        }

        /* Compare Encryption key length, if specified. */
        else if((attrib_list[i].ike_key_len != IKE_WILDCARD) &&
                (attrib_list[i].ike_key_len != attrib->ike_key_len))
        {
            /* Mismatch. Start over again. */
            continue;
        }

        /* Check if SA Lifetime type is specified. */
        else if(attrib_list[i].ike_sa_lifetime.ike_no_of_secs !=
                IKE_WILDCARD)
        {
            /* Make sure lifetime is not a wildcard. */
            if(attrib->ike_sa_lifetime.ike_no_of_secs == IKE_WILDCARD)
            {
                /* Mismatch. Start over again. */
                continue;
            }

            /* Make sure life duration is within allowed limit. */
            if(attrib_list[i].ike_sa_lifetime.ike_no_of_secs <
               attrib->ike_sa_lifetime.ike_no_of_secs)
            {
                /* Mismatch. Start over again. */
                continue;
            }
        }

        /* If control reaches here, then a valid match
         * has been found in the attribute list.
         */
        status = NU_SUCCESS;

        /* Return the Policy attributes which matched. */
        *out_attrib = &attrib_list[i];

        /* Look no further. */
        break;
    }

    /* Return the status. */
    return (status);

} /* IKE_Verify_Attributes */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Verify_Selection_SA
*
* DESCRIPTION
*
*       This is a utility function used to verify that the
*       proposal selection SA received from the Responder
*       is valid. It only checks the policy/attributes
*       independent fields.
*
* INPUTS
*
*       *dec_sa                 Pointer to decoded SA payload.
*
* OUTPUTS
*
*       NU_SUCCESS              If verification is successful.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_TOO_MANY_PROPOSALS  SA contains more than one proposal.
*       IKE_TOO_MANY_TRANSFORMS SA contains more than one transform.
*       IKE_INVALID_PROTOCOL    Protocol in proposal is not ISAKMP.
*       IKE_INVALID_TRANSFORM   Transform ID is not valid.
*
*************************************************************************/
STATUS IKE_Verify_Selection_SA(IKE_SA_DEC_PAYLOAD *dec_sa)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(dec_sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Verifying received selection SA");

    /* Make sure there is only a single transform in the SA. */
    if(dec_sa->ike_num_proposals != IKE_MAX_PHASE1_PROPOSALS)
    {
        NLOG_Error_Log("Multiple proposals in Phase 1 not allowed",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_TOO_MANY_PROPOSALS;
    }

    else if(dec_sa->ike_proposals[0].ike_num_transforms !=
            IKE_SELECTED_PHASE1_TRANSFORMS)
    {
        NLOG_Error_Log("Selection must contain only 1 transform.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_TOO_MANY_TRANSFORMS;
    }

    else if(dec_sa->ike_proposals[0].ike_protocol_id !=
            IKE_PROTO_ISAKMP)
    {
        NLOG_Error_Log("Protocol must be ISAKMP in Phase 1 proposal",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_INVALID_PROTOCOL;
    }

    else if(dec_sa->ike_proposals[0].ike_transforms[0].
                ike_transform_id != IKE_IPS_TRANS_IKE_KE)
    {
        NLOG_Error_Log("Transform ID must be IKE_KEY in Phase 1",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        status = IKE_INVALID_TRANSFORM;
    }

    /* Return the status. */
    return (status);

} /* IKE_Verify_Selection_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Auth_Parameters
*
* DESCRIPTION
*
*       This is a utility function which sets the parameters
*       required for the negotiated Authentication method.
*       Following operations are performed depending on the
*       Authentication method:
*
*       - PSK                   Pre-shared key is looked-up
*                               using the identification payload,
*                               if specified, or using the remote
*                               node's IP address.
*       - Signatures            Sets the Public key pair
*                               specified in the Policy.
*
*       This function should only be called after the SA
*       attributes have been negotiated.
*
* INPUTS
*
*       *policy_attrib          Attributes of the Policy which
*                               matched the proposal.
*       *sa                     The IKE SA, On return, this
*                               contains the Authentication method
*                               specific attributes.
*       *dec_id                 Received identification payload
*                               used to look-up the pre-shared
*                               key. This should be NULL for
*                               Main Mode exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_NOT_FOUND           Pre-shared key not found.
*
*************************************************************************/
STATUS IKE_Auth_Parameters(IKE_ATTRIB *policy_attrib, IKE_SA *sa,
                           IKE_ID_DEC_PAYLOAD *dec_id)
{
    STATUS          status;

#if (IKE_INCLUDE_PSK_AUTH == NU_FALSE)
    UNUSED_PARAMETER(dec_id);
#endif
#if (IKE_INCLUDE_SIG_AUTH == NU_FALSE)
    UNUSED_PARAMETER(policy_attrib);
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((policy_attrib == NU_NULL) || (sa == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
    /* If authentication method based on pre-shared keys. */
    if(IKE_IS_PSK_METHOD(sa->ike_attributes.ike_auth_method))
    {
        /* Look-up the pre-shared key using the identification
         * payload. Use the remote node's IP address for the
         * look-up if the identification payload is NULL.
         */
        status = IKE_Lookup_Preshared_Key(sa, dec_id);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Phase 1 pre-shared key not found",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
#endif

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    /* Otherwise, if authentication method based on signatures. */
    if(IKE_IS_SIGN_METHOD(sa->ike_attributes.ike_auth_method))
    {
        /* Copy the local certificate files' paths */
        if((policy_attrib->ike_local_cert_file != NU_NULL) &&
            (policy_attrib->ike_local_key_file != NU_NULL) )
        {
            sa->ike_attributes.ike_local_cert_file =
                policy_attrib->ike_local_cert_file;
            sa->ike_attributes.ike_local_key_file =
                policy_attrib->ike_local_key_file;
            sa->ike_attributes.ike_cert_encoding =
                policy_attrib->ike_cert_encoding;
        }

        if(policy_attrib->ike_ca_cert_file != NU_NULL)
        {
            sa->ike_attributes.ike_ca_cert_file =
                policy_attrib->ike_ca_cert_file;
        }

        if(policy_attrib->ike_peer_cert_file != NU_NULL)
        {
            sa->ike_attributes.ike_peer_cert_file =
                policy_attrib->ike_peer_cert_file;
        }

        if(policy_attrib->ike_crl_file != NU_NULL)
        {
            sa->ike_attributes.ike_crl_file =
                policy_attrib->ike_crl_file;
        }

#if(IKE_INCLUDE_PEM == NU_TRUE)
        if(policy_attrib->ike_pem_callback != NU_NULL)
        {
            sa->ike_attributes.ike_pem_callback =
                policy_attrib->ike_pem_callback;
        }
#endif /* (IKE_INCLUDE_PEM == NU_TRUE) */

        /* Set status to success. */
        status = NU_SUCCESS;
    }

    else
#endif /* (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */
    {
        /* Authentication method is unsupported. */
        status = IKE_UNSUPPORTED_METHOD;
    }

    /* Return the status. */
    return (status);

} /* IKE_Auth_Parameters */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_KeyXchg_Data
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to add Key Exchange data to the chain of payloads. It
*       first generates a Diffie-Hellman key pair and then
*       adds the Public key to a Key Exchange payload.
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
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Generate_KeyXchg_Data(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status;
    IKE_ENC_MESSAGE     *out;
    IKE_KEY_PAIR        dh_keys;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating Key Exchange data");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;

    /* Make the request for Key Pair generation. */
    status = IKE_DH_Generate_Key(IKE_Oakley_Group_Prime(ph1->ike_sa->ike_attributes.ike_group_desc),
                                 IKE_Oakley_Group_Length(ph1->ike_sa->ike_attributes.ike_group_desc),
                                 IKE_OAKLEY_GROUP_GEN(ph1->ike_sa->ike_attributes.ike_group_desc),
                                 &dh_keys, IKE_DH_PRIVATE_KEY_SIZE,
                                 NU_NULL, 0);

    if(status == NU_SUCCESS)
    {
        /* Store the Diffie-Hellman Kay Pair. This must
         * be deallocated later.
         */
        ph1->ike_dh_key.ike_public_key      = dh_keys.ike_public_key;
        ph1->ike_dh_key.ike_public_key_len  = dh_keys.ike_public_key_len;
        ph1->ike_dh_key.ike_private_key     = dh_keys.ike_private_key;
        ph1->ike_dh_key.ike_private_key_len = dh_keys.ike_private_key_len;

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

} /* IKE_Generate_KeyXchg_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_Nonce_Data
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
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Generate_Nonce_Data(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status;
    IKE_ENC_MESSAGE     *out;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating Nonce data");

    /* Set local pointer to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;

    /* Allocate memory for storing the Nonce data. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                (VOID**)&ph1->ike_nonce_data,
                                IKE_OUTBOUND_NONCE_DATA_LEN,
                                NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        ph1->ike_nonce_data = TLS_Normalize_Ptr(ph1->ike_nonce_data);

        /* Generate the random Nonce data. */

        if(RAND_bytes(ph1->ike_nonce_data, IKE_OUTBOUND_NONCE_DATA_LEN))
        {
            /* Set Key Exchange data in the payload. */
            out->ike_nonce->ike_nonce_data = ph1->ike_nonce_data;
            out->ike_nonce->ike_nonce_data_len =
                IKE_OUTBOUND_NONCE_DATA_LEN;

            /* Link payload to the chain. */
            IKE_ADD_TO_CHAIN(out->ike_last, out->ike_nonce,
                             IKE_NONCE_PAYLOAD_ID);
        }

        else
        {
            status = IKE_INTERNAL_ERROR;
            NLOG_Error_Log("Failed to generate random data",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for Nonce data",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_Nonce_Data */

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_Cert_Data
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to add Certificate data to the chain of payloads. The data is
*       stored in the Handle within a dynamically allocated
*       buffer. The caller is responsible for freeing this buffer
*       when it is no longer needed.
*
*       Following payloads are appended to the chain:
*       - IKE_CERT_ENC_PAYLOAD
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
*
*************************************************************************/
STATUS IKE_Generate_Cert_Data(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    IKE_CERT_ENC_PAYLOAD    *cert_payload;
    UINT8                   *cert_data;
    UINT16                  cert_len;

#if (IKE_DEBUG == NU_TRUE)
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message */
    IKE_DEBUG_LOG("Generating Certificate data.");

    cert_payload = ph1->ike_params->ike_out.ike_cert;

    /* Read the certificate data from file. */
    status = IKE_Cert_Get(ph1->ike_sa->ike_attributes.ike_local_cert_file,
                          &cert_data, &cert_len,
                          ph1->ike_sa->ike_attributes.ike_cert_encoding);

    /* If the certificate was read successfully. */
    if(status == NU_SUCCESS)
    {
        /* Populate the Cert encoding payload structure. */
        cert_payload->ike_cert_data = cert_data;
        cert_payload->ike_cert_data_len = cert_len;
        cert_payload->ike_cert_encoding = IKE_CERT_X509_SIG;

        /* Add the payload to the chain of outgoing payloads. */
        IKE_ADD_TO_CHAIN(ph1->ike_params->ike_out.ike_last, cert_payload,
                         IKE_CERT_PAYLOAD_ID);
    }

    /* Certificate could not be read. */
    else
    {
        NLOG_Error_Log("Failed to generate CERT payload data",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_CertReq_Data
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to add Certificate Request data to the chain of payloads. The
*       data is stored in the Handle within a dynamically allocated
*       buffer. The caller is responsible for freeing this buffer
*       when it is no longer needed.
*
*       Following payloads are appended to the chain:
*       - IKE_CERTREQ_ENC_PAYLOAD
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
**************************************************************************/
STATUS IKE_Generate_CertReq_Data(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                      status = NU_SUCCESS;
    IKE_CERTREQ_ENC_PAYLOAD     *cert_req;
    IKE_POLICY                  *policy;
    UINT8                       *cert_data;
    UINT16                      cert_len;
    UINT8                       *subject;
    UINT32                      sub_len;

#if (IKE_DEBUG == NU_TRUE)
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message */
    IKE_DEBUG_LOG("Generating Certificate Request data");

    /* Set local pointers to data in the Handle */
    cert_req = ph1->ike_params->ike_out.ike_certreq;
    policy   = ph1->ike_params->ike_policy;

    /* Set the desired encoding of the certificate that we want to accept.
     * Currently only one encoding is supported.
     */
    cert_req->ike_cert_type = IKE_CERT_ENCODING_X509_SIG;

    /* See if CA's DN needs to be specified in CERT_REQ. */
    if((policy->ike_flags & IKE_CA_IN_CERTREQ) != 0)
    {
        /* Read the certificate data from file. */
        status = IKE_Cert_Get(
                        ph1->ike_sa->ike_attributes.ike_ca_cert_file,
                        &cert_data, &cert_len,
                        ph1->ike_sa->ike_attributes.ike_cert_encoding);
        /* Certificate was read successfully. */
        if(status == NU_SUCCESS)
        {
            /* Get the SubjectName from the certificate. */
            status = IKE_Cert_Get_ASN1_SubjectName(cert_data, cert_len,
                                                   &subject, &sub_len);
            /* SubjectName read successfully. */
            if(status == NU_SUCCESS)
            {
                /* Copy the CA's DN in the CERTREQ structure. */
                NU_BLOCK_COPY((VOID*)cert_req->ike_ca_dn, (VOID*)subject,
                              sub_len);
                /* Set the length of CA's DN just copied above. */
                cert_req->ike_ca_dn_length = (UINT16)sub_len;
                if((NU_Deallocate_Memory((VOID*)cert_data) != NU_SUCCESS)
                   || (NU_Deallocate_Memory((VOID*)subject) != NU_SUCCESS))
                {
                    NLOG_Error_Log(
                       "Failed to free certificate or SubjectName memory.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* SubjectName could not be read, free the certificate data. */
            else
            {
                if(NU_Deallocate_Memory(cert_data) != NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Failed to deallocate memory for certificate.",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
    }

    /* If CA's DN is not to be sent in request, set the length of this
     * field to zero.
     */
    else
    {
        cert_req->ike_ca_dn_length = 0;
    }

    /* If all went good so far, add the payload to the chain of outgoing
     * payloads.
     */
    if(status == NU_SUCCESS)
    {
        IKE_ADD_TO_CHAIN(ph1->ike_params->ike_out.ike_last, cert_req,
                         IKE_CERTREQ_PAYLOAD_ID);
    }
    else
    {
        NLOG_Error_Log("Failed to generate CERTREQ payload data",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
}

#endif /* #if (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Get_Local_ID
*
* DESCRIPTION
*
*       This is a utility function used to fill in the
*       Identification payload with the local IP address. The
*       result is stored in the Identification payload pointer
*       of the state parameters.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Get_Local_ID(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status = NU_SUCCESS;
    IKE_ID_ENC_PAYLOAD  *enc_id;
#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
    IKE_PHASE2_HANDLE   *ph2;
#endif

    /* Set local pointer to commonly used data in the Handle. */
    enc_id = ph1->ike_params->ike_out.ike_id_i;

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
    /* If we are an Aggressive Mode Initiator, then this is
     * the first packet and the source IP could not be obtained
     * from the incoming packet pointer.
     */
    if((ph1->ike_xchg_mode == IKE_XCHG_AGGR) &&
       ((ph1->ike_flags & IKE_INITIATOR) != 0))
    {
        ph2 = ph1->ike_sa->ike_phase2_db.ike_flink;

        /* Make sure the phase 2 exchange is queued. If would not
         * be queued if the phase 2 exchange timed out before
         * the first message of the exchange was sent. This
         * improbable condition is checked to be on the safe side.
         */
        if(ph2 == NU_NULL)
        {
            status = IKE_INVALID_PARAMS;
        }

        /* If the phase 2 exchange is for tunnel mode. */
        else if(ph2->ike_sa2_db.ike_flink->ike_ips_security.
                    ipsec_security_mode == IPSEC_TUNNEL_MODE)
        {
            /* Set the identifier type. */
            enc_id->ike_id_type = IKE_IPS_FLAGS_TO_IKE(
                ph2->ike_sa2_db.ike_flink->ike_ips_security.ipsec_flags);

            /* Set data length by ID type. */
            enc_id->ike_id_data_len =
                IKE_IP_LEN_BY_FLAGS(enc_id->ike_id_type);

            /* Copy the tunnel source IP. */
            NU_BLOCK_COPY(enc_id->ike_id_data,
                ph2->ike_sa2_db.ike_flink->ike_ips_security.
                    ipsec_tunnel_source,
                enc_id->ike_id_data_len);
        }

        /* Otherwise, get the local IP from the IPsec selector. */
        else
        {
            /* Set the identifier type. */
            enc_id->ike_id_type = IKE_IPS_FLAGS_TO_IKE(
                ph2->ike_ips_select.ipsec_source_type);

            /* Set data length by ID type. */
            enc_id->ike_id_data_len =
                IKE_IP_LEN_BY_FLAGS(enc_id->ike_id_type);

            /* Copy the selector source IP. */
            NU_BLOCK_COPY(enc_id->ike_id_data,
                          ph2->ike_ips_select.ipsec_source_ip.ipsec_addr,
                          enc_id->ike_id_data_len);
        }
    }

    /* Otherwise obtain local IP from incoming packet pointer. */
    else
#endif
    {
        /* Set the identifier type to IP instead of IKE_WILDCARD. */
        enc_id->ike_id_type =
            IKE_FAMILY_TO_FLAGS(ph1->ike_params->ike_packet->
                                    ike_local_addr.family);

        /* Set data length by ID type. */
        enc_id->ike_id_data_len = IKE_IP_LEN_BY_FLAGS(enc_id->ike_id_type);

        /* Copy local IP address to identification data. */
        NU_BLOCK_COPY(enc_id->ike_id_data,
                      ph1->ike_params->ike_packet->ike_local_addr.id.
                          is_ip_addrs,
                      enc_id->ike_id_data_len);
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Local_ID */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_ID_Data
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to generate the Identification payload.
*
*       Following payloads are appended to the chain:
*       - IKE_ID_ENC_PAYLOAD
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_DOMAIN      Domain name in Policy is invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier not supported.
*
*************************************************************************/
STATUS IKE_Generate_ID_Data(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status = NU_SUCCESS;
    IKE_ID_ENC_PAYLOAD  *enc_id;
    IKE_POLICY          *policy;

#if(IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    UINT8               *cert_data;
    UINT8               *subject_name;
    UINT32              subject_len;
    UINT16              cert_len;
#endif /* #if(IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating ID data");

    /* Set local pointers to commonly used data in the Handle. */
    enc_id = ph1->ike_params->ike_out.ike_id_i;
    policy = ph1->ike_params->ike_policy;

    /*
     * Construct the Identification payload.
     */

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
    /* If this is a Main Mode exchange using PSK. */
    if((IKE_IS_PSK_METHOD(ph1->ike_sa->ike_attributes.ike_auth_method)
        == NU_TRUE)                             &&
#if (INCLUDE_IPV4 == NU_TRUE)
       (policy->ike_my_id.ike_type != IKE_IPV4) &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
       (policy->ike_my_id.ike_type != IKE_IPV6) &&
#endif
       (ph1->ike_xchg_mode == IKE_XCHG_MAIN))
    {
        /* Override the ID type specified in the policy because
         * only an IP address could be specified in the ID
         * payload when using Main Mode. The wildcard ID type
         * causes a look up of the local IP address.
         */
        enc_id->ike_id_type = IKE_WILDCARD;
    }

    else
#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */
    {
        /* Get ID type from the policy. */
        enc_id->ike_id_type = policy->ike_my_id.ike_type;
    }

    /* Set the unused fields to zero. */
    enc_id->ike_protocol_id = 0;
    enc_id->ike_port        = 0;

    /* Determine the local identifier type. */
    switch(enc_id->ike_id_type)
    {
    case IKE_WILDCARD:
        /* When Identifier type in the policy is IKE_WILDCARD, use
         * the local IP address as identification payload.
         */
        status = IKE_Get_Local_ID(ph1);
        break;

    case IKE_IPV4:
    case IKE_IPV6:
        /* Set length of identification data. */
        enc_id->ike_id_data_len =
            IKE_IP_LEN_BY_FLAGS(policy->ike_my_id.ike_type);

        /* Copy IP address to the identification data field. */
        NU_BLOCK_COPY(enc_id->ike_id_data,
                      policy->ike_my_id.ike_addr.ike_ip.ike_addr1,
                      enc_id->ike_id_data_len);
        break;

    case IKE_DOMAIN_NAME:
    case IKE_USER_DOMAIN_NAME:
        /* Make sure the domain name is specified. */
        if(policy->ike_my_id.ike_addr.ike_domain == NU_NULL)
        {
            status = IKE_INVALID_DOMAIN;
        }

        else
        {
            /* Set length of identification data. */
            enc_id->ike_id_data_len = (UINT16)
                strlen(policy->ike_my_id.ike_addr.ike_domain);

            /* Make sure domain name is not of zero length. */
            if(enc_id->ike_id_data_len == 0 ||
                    enc_id->ike_id_data_len > IKE_MAX_ID_DATA_LEN)
            {
                status = IKE_INVALID_DOMAIN;
            }

            else
            {
                /* Copy domain name to identification data field. */
                NU_BLOCK_COPY(enc_id->ike_id_data,
                              policy->ike_my_id.ike_addr.ike_domain,
                              enc_id->ike_id_data_len);
            }
        }
        break;

#if(IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    case IKE_DER_ASN1_DN:

        /* When this type of ID data is requested to be sent in ID payload,
         * we need to encode the SubjectName from the certificate in the
         * ID payload we send.
         */

        /* Read the certificate data from file. */
        status = IKE_Cert_Get(
            ph1->ike_sa->ike_attributes.ike_local_cert_file,
            &cert_data, &cert_len,
            ph1->ike_sa->ike_attributes.ike_cert_encoding);

        if(status == NU_SUCCESS)
        {
            /* Certificate read, now get the SubjectName. */
            status = IKE_Cert_Get_ASN1_SubjectName(cert_data, cert_len,
                                             &subject_name, &subject_len);

            if(status == NU_SUCCESS)
            {
                /* Set the length of ID data. */
                enc_id->ike_id_data_len = (UINT16)subject_len;

                if(enc_id->ike_id_data_len <= IKE_MAX_ID_DATA_LEN)
                {
                    /* Copy SubjectName to identification data field. */
                    NU_BLOCK_COPY(enc_id->ike_id_data, subject_name,
                                  enc_id->ike_id_data_len);
                }

                else
                {
                    status = IKE_LENGTH_IS_SHORT;
                }

                if((NU_Deallocate_Memory(cert_data) != NU_SUCCESS)
                    || (NU_Deallocate_Memory(subject_name) != NU_SUCCESS))
                {
                    NLOG_Error_Log(
                        "Could not free Cert or SubjectName memory",
                        NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* SubjectName could not be read, free the certificate
             * memory.
             */
            else
            {
                if(NU_Deallocate_Memory(cert_data) != NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Failed to deallocate certificate memory",
                        NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        break;

#endif /* #if(IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

    default:
        /* Identification type not supported. */
        status = IKE_UNSUPPORTED_IDTYPE;
        break;
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Link payload to the chain. */
        IKE_ADD_TO_CHAIN(ph1->ike_params->ike_out.ike_last,
                         enc_id, IKE_ID_PAYLOAD_ID);
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_ID_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_Hash_Data
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to generate the Hash payload. It calculates HASH_I/HASH_R
*       payload if the authentication method is PSK or SIG_I/SIG_R
*       if it is based on Signatures. The Hash or Signature
*       payload is then added to the chain.
*
*       Note that if a Signature is generated, the caller is
*       responsible for freeing the Signature data.
*
*       One of the following payloads is appended to the chain:
*       - IKE_HASH_ENC_PAYLOAD
*       - IKE_SIGNATURE_ENC_PAYLOAD
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*       *id_b                   Pointer to ID payload body.
*       id_b_len                Length of ID payload body.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_METHOD  Authentication method is not
*                               supported.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Generate_Hash_Data(IKE_PHASE1_HANDLE *ph1,
                              UINT8 *id_b, UINT16 id_b_len)
{
    STATUS              status;
    IKE_ENC_MESSAGE     *out;
    IKE_SA              *sa;
    VOID                *prf_ctx;
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    UINT8               *sign = NU_NULL;
    UINT                sign_len;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating HASH/SIG data");

    /* Set local pointers to commonly used data in the Handle. */
    out = &ph1->ike_params->ike_out;
    sa  = ph1->ike_sa;

    /* Calculate HASH_I or HASH_R depending on whether
     * we are the Initiator or Responder, respectively.
     */

    /* Initialize the PRF context. */

    status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                           &prf_ctx, sa->ike_skeyid, sa->ike_skeyid_len);

    if(status == NU_SUCCESS)
    {
        /* Add g^xi to PRF state, if caller is Initiator
         * Add g^xr to PRF state, if caller is Responder.
         */
        status = IKE_HMAC_Update(prf_ctx, ph1->ike_dh_key.ike_public_key,
                                 ph1->ike_dh_key.ike_public_key_len);

        if(status == NU_SUCCESS)
        {
            /* Add g^xr to PRF state, if caller is Initiator
             * Add g^xi to PRF state, if caller is Responder.
             */
            status = IKE_HMAC_Update(prf_ctx, ph1->ike_dh_remote_key,
                                     ph1->ike_dh_remote_key_len);

            if(status == NU_SUCCESS)
            {
                /* Determine whether caller is Initiator. */
                if((ph1->ike_flags & IKE_INITIATOR) != 0)
                {
                    /* Add CKY-I | CKY-R to PRF state. */
                    status = IKE_HMAC_Update(prf_ctx, sa->ike_cookies,
                                             IKE_COOKIE_LEN + IKE_COOKIE_LEN);
                }

                /* Otherwise the caller is the Responder. */
                else
                {
                    /* Add CKY-R to PRF state. */
                    status = IKE_HMAC_Update(prf_ctx, &sa->ike_cookies[IKE_COOKIE_LEN],
                                             IKE_COOKIE_LEN);

                    if(status == NU_SUCCESS)
                    {
                        /* Add CKY-I to PRF state. */
                        status = IKE_HMAC_Update(prf_ctx, sa->ike_cookies, IKE_COOKIE_LEN);
                    }
                }

                /* If no error occurred. */
                if(status == NU_SUCCESS)
                {
                    /* Add SAi_b to PRF state. */
                    status = IKE_HMAC_Update(prf_ctx, ph1->ike_sa_b, ph1->ike_sa_b_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Add IDii_b if caller is Initiator or
                         * add IDir_b if caller is Responder.
                         */
                        status = IKE_HMAC_Update(prf_ctx, id_b, id_b_len);

                        if(status == NU_SUCCESS)
                        {
                            /* Set the hash data length. */
                            out->ike_hash->ike_hash_data_len = 0;

                            /* Finalize the hash. */
                            status = IKE_HMAC_Final(prf_ctx,
                                                out->ike_hash->ike_hash_data,
                                                &out->ike_hash->ike_hash_data_len);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to finalize PRF context",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log(
                                "Failed to update PRF context",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to update PRF context",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to update PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to initialize PRF context",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
        /* If authentication method based on pre-shared keys. */
        if(IKE_IS_PSK_METHOD(sa->ike_attributes.ike_auth_method))
        {
            /* Add HASH_x to the payloads chain. */
            IKE_ADD_TO_CHAIN(out->ike_last, out->ike_hash,
                             IKE_HASH_PAYLOAD_ID);
        }

        else
#endif
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        /* Otherwise, if authentication method based on signatures. */
        if(IKE_IS_SIGN_METHOD(sa->ike_attributes.ike_auth_method))
        {
            /* Generate the Signature for HASH_x. */
            status = IKE_Sign(sa, out->ike_hash->ike_hash_data,
                              out->ike_hash->ike_hash_data_len,
                              &sign, &sign_len);

            if(status == NU_SUCCESS)
            {
                /* Set the data of the Signature payload. */
                out->ike_sig->ike_signature_data = sign;
                out->ike_sig->ike_signature_data_len = sign_len;

                /* Add SIG_x to the payloads chain. */
                IKE_ADD_TO_CHAIN(out->ike_last, out->ike_sig,
                                 IKE_SIGNATURE_PAYLOAD_ID);
            }

            else
            {
                NLOG_Error_Log("Failed to generate signature",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
#endif
        {
            /* Authentication method is unsupported. */
            status = IKE_UNSUPPORTED_METHOD;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_Hash_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Verify_Auth_Data
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to verify Authentication data of the other party. It
*       generates HASH_I if caller is the Responder or HASH_R if
*       caller is the Initiator. The generated hash digest value
*       is compared with the one sent by the other party.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_UNSUPPORTED_METHOD  Authentication method unsupported.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_ID_MISMATCH         Unexpected identity of remote
*                               node.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Verify_Auth_Data(IKE_PHASE1_HANDLE *ph1)
{
    STATUS          status;
    IKE_DEC_MESSAGE *in;
    IKE_IDENTIFIER  id;
    VOID            *prf_ctx;
    UINT8           dgst[IKE_MAX_HASH_DATA_LEN];
    UINT8           dgst_len = 0;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Verifying authentication data");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
    /* Initialize status to success. */
    status = NU_SUCCESS;

    /* Only the ID payload body (ph1->ike_id_b) will be present
     * if this function is called from the last state of
     * Aggressive mode. In that case, this payload would have been
     * checked in an earlier Aggressive mode state.
     */
    if(in->ike_id_i != NU_NULL)
#endif
    {
        /* Convert Identification payload to the IKE identifier. */
        status = IKE_Payload_To_Identifier(in->ike_id_i, &id);

        if(status == NU_SUCCESS)
        {
            /* Make sure protocol and port are either wildcards,
             * or set to the IKE UDP port.
             */
            if(!(((in->ike_id_i->ike_protocol_id == IKE_WILDCARD)  &&
                  (in->ike_id_i->ike_port        == IKE_WILDCARD)) ||
                 ((in->ike_id_i->ike_protocol_id == IP_UDP_PROT)   &&
                  (in->ike_id_i->ike_port        == IKE_RECV_UDP_PORT))))
            {
                status = IKE_INVALID_ID;
            }

            /* Check if the identifier matches the one
             * specified in the policy.
             */
            else if(IKE_MATCH_IDENTIFIERS(
                        &ph1->ike_params->ike_policy->ike_peers_id, &id)
                        == NU_FALSE)
            {
                /* Log the identification mismatch. */
                NLOG_Error_Log(
                    "Received payload has Identification type mismatch",
                    NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* Check if identification verification is
                 * enabled in the Policy.
                 */
                if((ph1->ike_params->ike_policy->ike_flags & IKE_VERIFY_ID)
                   != 0)
                {
                    /* Report error. */
                    status = IKE_ID_MISMATCH;
                }
            }

            /* If ID is a single IP address. */
            else if(IKE_IS_SINGLE_IP(id.ike_type) == NU_TRUE)
            {
                /* Make sure address family of remote node, with which
                 * this exchange is taking place, also belongs to the
                 * same IP family.
                 */
                if(IKE_FAMILY_TO_FLAGS(ph1->ike_sa->ike_node_addr.family)
                   != id.ike_type)
                {
                    status = IKE_ID_MISMATCH;
                }

                /* Make sure remote IP address is equal to the one
                 * specified in the ID payload.
                 */
                else if(memcmp(ph1->ike_sa->ike_node_addr.id.is_ip_addrs,
                               id.ike_addr.ike_ip.ike_addr1,
                               IKE_IP_LEN_BY_FLAGS(id.ike_type)))
                {
                    status = IKE_ID_MISMATCH;
                }
            }

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
            /* If this is a Main Mode exchange using PSK and ID
             * payload does not contain a single IP.
             */
            if((status == NU_SUCCESS) &&
               (IKE_IS_PSK_METHOD(ph1->ike_sa->ike_attributes.
                                  ike_auth_method) == NU_TRUE) &&
               (ph1->ike_xchg_mode == IKE_XCHG_MAIN) &&
               (IKE_IS_SINGLE_IP(id.ike_type) == NU_FALSE))
            {
                /* Single IP based identification must be used
                 * in Main Mode using PSK.
                 */
                status = IKE_ID_MISMATCH;
            }
#endif

            /* If the IKE identifier is a domain name. */
            if((id.ike_type == IKE_DOMAIN_NAME) ||
               (id.ike_type == IKE_USER_DOMAIN_NAME))
            {
                /* Deallocate the domain name memory allocated above. */
                if(NU_Deallocate_Memory(id.ike_addr.ike_domain)
                   != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate IKE memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to convert ID payload to identifier",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* If no errors occurred and the remote node's
     * Identification payload has been received.
     */
    if((status == NU_SUCCESS)
#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
       && (ph1->ike_id_b != NU_NULL)
#endif
       )
    {
        /* Calculate HASH_I or HASH_R depending on whether
         * the caller is Responder or Initiator, respectively.
         */

        /* Initialize the PRF context. */

        status = IKE_HMAC_Init(IKE_Hash_Algos[ph1->ike_sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                               &prf_ctx, ph1->ike_sa->ike_skeyid, ph1->ike_sa->ike_skeyid_len);

        if(status == NU_SUCCESS)
        {
            /* Add g^xi | g^xr to PRF state, if caller is Responder
             * Add g^xr | g^xi to PRF state, if caller is Initiator.
             */
            status = IKE_HMAC_Update(prf_ctx, ph1->ike_dh_remote_key,
                                 ph1->ike_dh_remote_key_len);

            if(status == NU_SUCCESS)
            {
                status = IKE_HMAC_Update(prf_ctx,
                                     ph1->ike_dh_key.ike_public_key,
                                     ph1->ike_dh_key.ike_public_key_len);

                if(status == NU_SUCCESS)
                {
                    /* Determine whether caller is Initiator. */
                    if((ph1->ike_flags & IKE_INITIATOR) != 0)
                    {
                        /* Add CKY-R to PRF state. */
                        status = IKE_HMAC_Update(prf_ctx,
                                             &ph1->ike_sa->ike_cookies[IKE_COOKIE_LEN],
                                             IKE_COOKIE_LEN);

                        if(status == NU_SUCCESS)
                        {
                            /* Add CKY-I to PRF state. */
                            status = IKE_HMAC_Update(prf_ctx,
                                                 ph1->ike_sa->ike_cookies,
                                                 IKE_COOKIE_LEN);
                        }
                    }

                    /* Otherwise the caller is the Responder. */
                    else
                    {
                        /* Add CKY-I | CKY-R to PRF state. */
                        status = IKE_HMAC_Update(prf_ctx,
                                             ph1->ike_sa->ike_cookies,
                                             IKE_COOKIE_LEN + IKE_COOKIE_LEN);
                    }

                    /* If no error occurred. */
                    if(status == NU_SUCCESS)
                    {
                        /* Add SAi_b to PRF state. */
                        status = IKE_HMAC_Update(prf_ctx, ph1->ike_sa_b,
                                             ph1->ike_sa_b_len);

                        if(status == NU_SUCCESS)
                        {
                            /* Add IDii_b if caller is Responder or
                             * add IDir_b if caller is Initiator.
                             */
                            status = IKE_HMAC_Update(prf_ctx,
                                                 ph1->ike_id_b,
                                                 ph1->ike_id_b_len);

                            if(status == NU_SUCCESS)
                            {
                                /* Set the digest length. */
                                dgst_len = 0;

                                /* Finalize the hash. */
                                status = IKE_HMAC_Final(prf_ctx, dgst, &dgst_len);

                                if(status != NU_SUCCESS)
                                {
                                    NLOG_Error_Log(
                                      "Failed to finalize PRF context",
                                      NERR_RECOVERABLE,
                                      __FILE__, __LINE__);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to update PRF context",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log(
                                "Failed to update PRF context",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to update PRF context",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to initialize PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
            /* If authentication method based on pre-shared keys. */
            if(IKE_IS_PSK_METHOD(
                   ph1->ike_sa->ike_attributes.ike_auth_method))
            {
                /* Length of generated digest should be the same. */
                if(dgst_len != in->ike_hash->ike_hash_data_len)
                {
                    status = IKE_AUTH_FAILED;
                }

                /* Compare the digest values. */
                else if(memcmp(dgst, in->ike_hash->ike_hash_data,
                               dgst_len))
                {
                    status = IKE_AUTH_FAILED;
                }
            }

            else
#endif
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
            /* Otherwise, if authentication method based on signatures. */
            if(IKE_IS_SIGN_METHOD(
                   ph1->ike_sa->ike_attributes.ike_auth_method))
            {
                if((ph1->ike_params->ike_policy->ike_flags &
                     IKE_VERIFY_CERT) != 0)
                {
                    /* Verify that the ID payload and ID in certificate
                     * match.
                     */
                    status = IKE_Cert_Verify_ID(
                                    in->ike_cert->ike_cert_data,
                                    in->ike_cert->ike_cert_data_len, &id);

                    if(status == NU_SUCCESS)
                    {
                        /* Verify that the certificate is signed by trusted
                         * CA.
                         */
                        status = IKE_Cert_Verify(
                            in->ike_cert->ike_cert_data,
                            in->ike_cert->ike_cert_data_len,
                            ph1->ike_sa->ike_attributes.ike_ca_cert_file,
                            ph1->ike_sa->ike_attributes.ike_cert_encoding,
                            ph1->ike_sa->ike_attributes.ike_crl_file,
                            (ph1->ike_params->ike_policy->ike_flags
                            & IKE_VERIFY_AGAINST_CRL));
                    }
                }

                if(status == NU_SUCCESS)
                {
                    /* Generate the Signature for HASH_x. */
                    status = IKE_Sign_Verify(ph1->ike_sa, dgst, dgst_len,
                                 in->ike_sig->ike_signature_data,
                                 in->ike_sig->ike_signature_data_len);
                }

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Signature verification failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Report failure of Signature authentication. */
                    status = IKE_AUTH_FAILED;
                }
            }

            else
#endif
            {
                /* Authentication method is unsupported. */
                status = IKE_UNSUPPORTED_METHOD;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Verify_Auth_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Aggr_4_Main_6_7_Recv
*
* DESCRIPTION
*
*       This is a utility function used by the IKE state machine
*       to receive Authentication data from the other party. It
*       verifies the data and returns an error if authentication
*       fails. This function is used by both Initiator and
*       Responder of Phase 1 Main mode and also in the last state
*       of Aggressive mode.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_LENGTH_IS_SHORT     Message buffer not large enough.
*       IKE_INVALID_PAYLOAD     SA payload is invalid.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_MISSING_PAYLOAD     Required payload is missing.
*       IKE_AUTH_FAILED         Authentication failed.
*       IKE_NOT_FOUND           If authenticating by signatures and peer's
*                               certificate was to be used from local file,
*                               it was not specified.
*
*************************************************************************/
STATUS IKE_Aggr_4_Main_6_7_Recv(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status = NU_SUCCESS;
    IKE_DEC_MESSAGE         *in;
    IKE_NOTIFY_DEC_PAYLOAD  dec_notify;
    IKE_ID_DEC_PAYLOAD      dec_id;
    IKE_AUTH_DEC_PAYLOADS   auth;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    IKE_CERTREQ_DEC_PAYLOAD dec_certreq;
    IKE_CERT_DEC_PAYLOAD    dec_cert;
    UINT8                   *remote_pubkey = NU_NULL;
    UINT8                   *peer_cert;
    UINT32                  remote_keylen = 0;
    UINT16                  peer_cert_len;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Receiving authentication payloads");

    /* Set local pointer to commonly used data in the Handle. */
    in = &ph1->ike_params->ike_in;

    /* Set inbound payloads used in this state. */
    IKE_SET_INBOUND_OPTIONAL(in->ike_notify, &dec_notify);

    /* Check if this is Main mode. */
    if(ph1->ike_xchg_mode == IKE_XCHG_MAIN)
    {
        /* Set ID payload as required. */
        IKE_SET_INBOUND_REQUIRED(in->ike_id_i, &dec_id);
    }

    /* Set either Hash or Signature payload depending on
     * the Authentication method being used.
     */
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(IKE_IS_SIGN_METHOD(ph1->ike_sa->ike_attributes.ike_auth_method))
    {
        IKE_SET_INBOUND_REQUIRED(in->ike_sig, &auth.dec_sig);
    }

    else
#endif
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
    if(IKE_IS_PSK_METHOD(ph1->ike_sa->ike_attributes.ike_auth_method))
    {
        IKE_SET_INBOUND_REQUIRED(in->ike_hash, &auth.dec_hash);
    }

    else
#endif
    {
        NLOG_Error_Log("Unrecognized authentication method in IKE SA",
                       NERR_SEVERE, __FILE__, __LINE__);

        status = IKE_INVALID_PARAMS;
    }
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    if(IKE_IS_SIGN_METHOD(ph1->ike_sa->ike_attributes.ike_auth_method))
    {
        if(((ph1->ike_params->ike_policy->ike_flags) &
            IKE_INBAND_CERT_XCHG) != 0)
        {
            IKE_SET_INBOUND_OPTIONAL(in->ike_certreq, &dec_certreq);
            IKE_SET_INBOUND_OPTIONAL(in->ike_cert, &dec_cert);
        }

        else
        {
            IKE_SET_INBOUND_REQUIRED(in->ike_certreq, &dec_certreq);
            IKE_SET_INBOUND_REQUIRED(in->ike_cert, &dec_cert);
        }
    }
#endif

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Decode all payloads in the packet. */
        status = IKE_Decode_Message(ph1->ike_params->ike_packet->ike_data,
                                    in);

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        if(IKE_IS_SIGN_METHOD(
            ph1->ike_sa->ike_attributes.ike_auth_method))
        {
            if(((ph1->ike_params->ike_policy->ike_flags &
                IKE_INBAND_CERT_XCHG) != 0) &&
                (IKE_PAYLOAD_IS_PRESENT(in->ike_certreq) == NU_TRUE)
              )
            {
                switch(in->ike_certreq->ike_cert_type)
                {

                case IKE_CERT_ENCODING_X509_SIG:
                    /* If CERTREQ payload specifies an encoding that we
                     * support, set the flag for certificate payload to be
                     * sent.
                     */
                    ph1->ike_flags |= IKE_CERTREQ_RECEIVED;
                    break;

                default:
                    NLOG_Error_Log(
                        "Requested CERT type not valid or not supported",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }
            }

            /* If certificate has been received, and we are exchanging
             * certificates in-band, extract the public key from it.
             */
            if((ph1->ike_params->ike_policy->ike_flags
                 & IKE_INBAND_CERT_XCHG) != 0)
            {
                if(IKE_PAYLOAD_IS_PRESENT(in->ike_cert) == NU_TRUE)
                {
                    /* Read the public key from received certificate. */
                    status = IKE_Cert_Get_PKCS1_Public_Key(
                        in->ike_cert->ike_cert_data,
                        in->ike_cert->ike_cert_data_len,
                        &remote_pubkey, &remote_keylen);
                }
            }

            /* We do not want in-band certificate exchange, but are
             * authenticating with digital signatures, so see if a peer
             * certificate has been specified locally. If specified,
             * read public key from it.
             */
            else
            {
                if(ph1->ike_sa->ike_attributes.ike_peer_cert_file
                    != NU_NULL)
                {
                    /* Read the certificate from file. */
                    status = IKE_Cert_Get(
                        ph1->ike_sa->ike_attributes.ike_peer_cert_file,
                        &peer_cert, &peer_cert_len,
                        ph1->ike_sa->ike_attributes.ike_cert_encoding);
                    if(status == NU_SUCCESS)
                    {
                        /* Get the public key from certificate. */
                        status = IKE_Cert_Get_PKCS1_Public_Key(peer_cert,
                            peer_cert_len, &remote_pubkey, &remote_keylen);
                    }

                    /* If memory was allocated for peer's certificate,
                     * it is no longer needed, free it.
                     */
                    if((peer_cert != NU_NULL) &&
                        (NU_Deallocate_Memory(peer_cert) != NU_SUCCESS))
                    {
                        NLOG_Error_Log(
                            "Failed to free memory for certificate.",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                /* Peer's certificate was not specified. */
                else
                {
                    status = IKE_NOT_FOUND;
                }
            }

            if(status == NU_SUCCESS)
            {
                ph1->ike_sa->ike_attributes.ike_remote_key = remote_pubkey;
                ph1->ike_sa->ike_attributes.ike_remote_key_len =
                                                     (UINT16)remote_keylen;
            }
        }

#endif

        if(status == NU_SUCCESS)
        {
            /* Check if this is Main mode. */
            if(ph1->ike_xchg_mode == IKE_XCHG_MAIN)
            {
                /* Extract ID payload body from raw message. */
                status = IKE_Extract_Raw_Payload(
                             ph1->ike_params->ike_packet->ike_data,
                             &ph1->ike_id_b, &ph1->ike_id_b_len,
                             IKE_ID_PAYLOAD_ID);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to extract raw ID payload",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* If no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Verify the Authentication data. */
                status = IKE_Verify_Auth_Data(ph1);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("HASH_x verification failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
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

} /* IKE_Aggr_4_Main_6_7_Recv */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Payload_To_Identifier
*
* DESCRIPTION
*
*       This is a utility function used to convert an
*       identification payload to an equivalent IKE identifier.
*       Only the following identifier types are supported
*       in the Phase 1 identification payload:
*
*       - IKE_IPV4
*       - IKE_IPV6
*       - IKE_DOMAIN_NAME
*       - IKE_USER_DOMAIN_NAME
*
* INPUTS
*
*       *id_pload               Pointer to the identification
*                               payload.
*       *id                     On return, this contains the
*                               equivalent IKE identifier.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*
*************************************************************************/
STATUS IKE_Payload_To_Identifier(IKE_ID_DEC_PAYLOAD *id_pload,
                                 IKE_IDENTIFIER *id)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((id_pload == NU_NULL) || (id == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set identifier type. */
    id->ike_type = id_pload->ike_id_type;

    /* Determine the identifier type. */
    switch(id_pload->ike_id_type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IKE_IPV4:
        /* Make sure the IP length is valid. */
        if(id_pload->ike_id_data_len == IP_ADDR_LEN)
        {
            /* Copy IPv4 address to the identifier. */
            NU_BLOCK_COPY(id->ike_addr.ike_ip.ike_addr1,
                          id_pload->ike_id_data, IP_ADDR_LEN);
        }
        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    case IKE_IPV6:
        /* Make sure the IP length is valid. */
        if(id_pload->ike_id_data_len == IP6_ADDR_LEN)
        {
            /* Copy IPv6 address to the identifier. */
            NU_BLOCK_COPY(id->ike_addr.ike_ip.ike_addr1,
                          id_pload->ike_id_data, IP6_ADDR_LEN);
        }
        break;
#endif

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    case IKE_DER_ASN1_DN:
#endif
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    case IKE_DOMAIN_NAME:
    case IKE_USER_DOMAIN_NAME:
#endif
#if ((IKE_INCLUDE_SIG_AUTH == NU_TRUE) || \
     (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE))

        /* Allocate memory for the domain name. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                     (VOID**)&id->ike_addr.ike_domain,
                     (UNSIGNED)id_pload->ike_id_data_len + 1,
                     NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Normalize the pointer. */
            id->ike_addr.ike_domain =
                TLS_Normalize_Ptr(id->ike_addr.ike_domain);

            /* Copy the domain name to the identifier. */
            NU_BLOCK_COPY(id->ike_addr.ike_domain,
                          id_pload->ike_id_data,
                          id_pload->ike_id_data_len);

            /* Also set the NULL terminator at the end of string
             * because the terminator is not present in the
             * identification payload data.
             */
            id->ike_addr.ike_domain[(INT)id_pload->ike_id_data_len] = 0;
        }

        else
        {
            NLOG_Error_Log("Failed to allocate memory for ID data",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        break;
#endif

    default:
        /* Identification type not supported. */
        status = IKE_UNSUPPORTED_IDTYPE;
        break;
    }

    /* Return the status. */
    return (status);

} /* IKE_Payload_To_Identifier */

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Lookup_Preshared_Key
*
* DESCRIPTION
*
*       This is a utility function which looks up the pre-shared
*       key using the specified identification payload. If
*       the identification payload is not specified, the remote
*       node's IP address is used to look up the pre-shared key.
*       The key is returned in the remote key attribute of the SA.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA. On return
*                               this contains the pre-shared key.
*       *dec_id                 Pointer to the identification
*                               payload. If NULL, the default
*                               remote IP address in the IKE SA
*                               is used to look-up the PSK.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identifier type not supported.
*       IKE_NOT_FOUND           Pre-shared key not found.
*
*************************************************************************/
STATUS IKE_Lookup_Preshared_Key(IKE_SA *sa, IKE_ID_DEC_PAYLOAD *dec_id)
{
    STATUS              status = NU_SUCCESS;
    IKE_IDENTIFIER      id;
    IKE_PRESHARED_KEY   *psk_ptr;
    UINT8               *buffer;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Looking up Pre-Shared Key");

    /* If the identification payload has been specified. */
    if(dec_id != NU_NULL)
    {
        /* Convert Identification payload to the IKE identifier. */
        status = IKE_Payload_To_Identifier(dec_id, &id);
    }

    /* Otherwise, identification payload not specified. */
    else
    {
        /* Set type of IKE identifier. */
        id.ike_type = IKE_FAMILY_TO_FLAGS(sa->ike_node_addr.family);

        /* Copy the IP address of the remote node from the IKE SA. */
        NU_BLOCK_COPY(id.ike_addr.ike_ip.ike_addr1,
                      sa->ike_node_addr.id.is_ip_addrs,
                      IKE_IP_LEN(sa->ike_node_addr.family));
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Look-up the pre-shared key. */
        status = IKE_Get_Preshared_Key_By_ID(&id, &psk_ptr,
                                             IKE_MATCH_IDENTIFIERS);

        if(status == NU_SUCCESS)
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("Pre-shared Key found");

            /* Allocate buffer for the pre-shared key. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&buffer,
                                        IKE_OUTBOUND_NONCE_DATA_LEN,
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Copy key data to the allocated buffer. */
                NU_BLOCK_COPY(buffer, psk_ptr->ike_key,
                              psk_ptr->ike_key_len);

                /* Set pointer to the pre-shared key in the SA. */
                sa->ike_attributes.ike_remote_key     = buffer;
                sa->ike_attributes.ike_remote_key_len =
                    (UINT16)psk_ptr->ike_key_len;
            }

            else
            {
                NLOG_Error_Log(
                    "Failed to allocate memory for pre-shared key",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Phase 1 pre-shared Key not found",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If the IKE identifier is a domain name. */
        if((id.ike_type == IKE_DOMAIN_NAME) ||
           (id.ike_type == IKE_USER_DOMAIN_NAME))
        {
            /* Deallocate the domain name memory. */
            if(NU_Deallocate_Memory(id.ike_addr.ike_domain) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate IKE memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Lookup_Preshared_Key */
#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */

#if (IKE_RETAIN_LAST_MESSAGE == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_Phase1_Memory
*
* DESCRIPTION
*
*       This is a utility function used to free all dynamically
*       allocated fields of a Phase 1 Handle which are no longer
*       needed, after the IKE SA is established. Some fields
*       fields are not deallocated because they are used in
*       message retransmission.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Free_Phase1_Memory(IKE_PHASE1_HANDLE *ph1)
{
#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Freeing unused fields of Phase 1 Handle");

    /* If Nonce data is allocated. */
    if(ph1->ike_nonce_data != NU_NULL)
    {
        /* Deallocate the Nonce data. */
        if(NU_Deallocate_Memory(ph1->ike_nonce_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Nonce data field to NULL. */
        ph1->ike_nonce_data = NU_NULL;
    }

    /* If the Initiator's raw SA is allocated. */
    if(ph1->ike_sa_b != NU_NULL)
    {
        /* Deallocate the Initiator's raw SA. */
        if(NU_Deallocate_Memory(ph1->ike_sa_b) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Initiator's raw SA fields to NULL. */
        ph1->ike_sa_b = NU_NULL;
        ph1->ike_sa_b_len = 0;
    }

    /* If Initiator's ID payload is allocated. */
    if(ph1->ike_id_b != NU_NULL)
    {
        /* Deallocate Initiator's ID payload. */
        if(NU_Deallocate_Memory(ph1->ike_id_b) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Initiator's ID payload fields to NULL. */
        ph1->ike_id_b = NU_NULL;
        ph1->ike_id_b_len = 0;
    }

    /* If the remote Diffie-Hellman public key is allocated. */
    if(ph1->ike_dh_remote_key != NU_NULL)
    {
        /* Deallocate remote Diffie-Hellman public key. */
        if(NU_Deallocate_Memory(ph1->ike_dh_remote_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set remote Diffie-Hellman public key fields to NULL. */
        ph1->ike_dh_remote_key     = NU_NULL;
        ph1->ike_dh_remote_key_len = 0;
    }

    /* If the Diffie-Hellman key pair is allocated. */
    if(ph1->ike_dh_key.ike_public_key != NU_NULL)
    {
        /* Deallocate the Diffie-Hellman key pair. */
        if(NU_Deallocate_Memory(ph1->ike_dh_key.ike_public_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Diffie-Hellman key pair fields to NULL. */
        ph1->ike_dh_key.ike_public_key      = NU_NULL;
        ph1->ike_dh_key.ike_private_key     = NU_NULL;
        ph1->ike_dh_key.ike_public_key_len  = 0;
        ph1->ike_dh_key.ike_private_key_len = 0;
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_Phase1_Memory */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_Phase2_Memory
*
* DESCRIPTION
*
*       This is a utility function used to free all dynamically
*       allocated memory of a Phase 2 Handle which is no longer
*       needed, after a Phase 2 Exchange is complete. Note that
*       the Handle is still kept in the database because the
*       last message of the exchange might need retransmission.
*       Minimum fields required for retransmission are
*       maintained.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Free_Phase2_Memory(IKE_PHASE2_HANDLE *ph2)
{
    IKE_SA2         *cur_sa2;
    IKE_SA2         *prev_sa2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Freeing unused fields of Phase 2 Handle");

    /* If Initiator's Nonce data is allocated. */
    if(ph2->ike_nonce_i != NU_NULL)
    {
        /* Deallocate the Initiator's Nonce data. */
        if(NU_Deallocate_Memory(ph2->ike_nonce_i) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the Initiator's Nonce data field to NULL. */
        ph2->ike_nonce_i = NU_NULL;
    }

    /* If Responder's Nonce data is allocated. */
    if(ph2->ike_nonce_r != NU_NULL)
    {
        /* Deallocate the Responder's Nonce data. */
        if(NU_Deallocate_Memory(ph2->ike_nonce_r) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the Responder's Nonce data field to NULL. */
        ph2->ike_nonce_r = NU_NULL;
    }

    /* If the Diffie-Hellman remote public key is allocated. */
    if(ph2->ike_dh_remote_key != NU_NULL)
    {
        /* Deallocate the remote public key. */
        if(NU_Deallocate_Memory(ph2->ike_dh_remote_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the remote public key field to NULL. */
        ph2->ike_dh_remote_key = NU_NULL;
    }

    /* If the Diffie-Hellman key pair is allocated. */
    if(ph2->ike_dh_key.ike_public_key != NU_NULL)
    {
        /* Deallocate the key pair. */
        if(NU_Deallocate_Memory(ph2->ike_dh_key.ike_public_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the key pair fields to NULL. */
        ph2->ike_dh_key.ike_public_key  = NU_NULL;
        ph2->ike_dh_key.ike_private_key = NU_NULL;
    }

    /* Get the first SA2 item in the list. */
    cur_sa2 = ph2->ike_sa2_db.ike_flink;

    /* Traverse the whole SA2 list. */
    while(cur_sa2 != NU_NULL)
    {
        /* Keep reference of current SA2. */
        prev_sa2 = cur_sa2;

        /* Move to next item. */
        cur_sa2 = cur_sa2->ike_flink;

        /* If the key material is allocated. */
        if(prev_sa2->ike_local_keymat != NU_NULL)
        {
            /* Deallocate the both local and remote key
             * material. Both allocated in a single block
             * within the SA2.
             */
            if(NU_Deallocate_Memory(prev_sa2->ike_local_keymat) !=
               NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate the memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Set the key material fields to NULL. */
            prev_sa2->ike_local_keymat  = NU_NULL;
            prev_sa2->ike_remote_keymat = NU_NULL;
        }

        /* Deallocate memory of the previous SA2. */
        if(NU_Deallocate_Memory(prev_sa2) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Set pointers of the SA2DB to NULL. */
    ph2->ike_sa2_db.ike_flink = NU_NULL;
    ph2->ike_sa2_db.ike_last = NU_NULL;

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_Phase2_Memory */
#endif /* (IKE_RETAIN_LAST_MESSAGE == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Finalize_Phase1
*
* DESCRIPTION
*
*       This function finalizes an IKE SA after a Phase 1
*       Exchange is complete. It de-allocates all Phase 1
*       Handle fields which are not required anymore.
*       If the SA timeout is specified in seconds, this
*       function also starts a timer to remove the SA after
*       the specified number of seconds have elapsed.
*
*       Note that if retaining the last message is enabled,
*       the Phase 1 Handle is not removed from the database
*       because the last message might have been lost. The
*       Phase 1 exchange timeout timer would remove this
*       Handle instead. However, if this feature is disabled,
*       then the Handle is also removed.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Finalize_Phase1(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status;
    IKE_SA              *sa;
    IKE_STATE_PARAMS    *params;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((ph1 == NU_NULL) || (ph1->ike_sa == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Finalizing Phase 1 Handle");

    /* Set local pointers to commonly used data in the Handle. */
    sa     = ph1->ike_sa;
    params = ph1->ike_params;

#if (IKE_RETAIN_LAST_MESSAGE == NU_TRUE)
    /* Free members of the Handle which are no longer required. */
    status = IKE_Free_Phase1_Memory(ph1);
#else
    /* Remove the Handle since it is no longer required. */
    status = IKE_Remove_Phase1(sa);
#endif

    if(status == NU_SUCCESS)
    {
        /* Remove any message re-send timers of this SA. */
        status = IKE_Unset_Timer(IKE_Message_Reply_Event,
                                 (UNSIGNED)sa, 0);

#if (IKE_RETAIN_LAST_MESSAGE == NU_FALSE)
        if(status == NU_SUCCESS)
        {
            /* Also remove Phase 1 timeout event. */
            status = IKE_Unset_Timer(IKE_Phase1_Timeout_Event,
                         (UNSIGNED)sa,
                         (UNSIGNED)&params->ike_policy->ike_sa_list);
        }
#endif

        if(status == NU_SUCCESS)
        {
            /* Start a timer to remove the SA after
             * the SA lifetime has expired.
             */
            status = IKE_Set_Timer(IKE_SA_Timeout_Event, (UNSIGNED)sa,
                         (UNSIGNED)&params->ike_policy->ike_sa_list,
                         (UNSIGNED)sa->ike_attributes.
                         ike_sa_lifetime.ike_no_of_secs);

            if(status == NU_SUCCESS)
            {
                /* Set SA state to established. */
                sa->ike_state = IKE_SA_ESTABLISHED;
            }

            else
            {
                NLOG_Error_Log("Failed to set timer for IKE SA lifetime",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to unset IKE event timers",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to finalize/remove Phase 1 Handle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Finalize_Phase1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Finalize_Phase2
*
* DESCRIPTION
*
*       This function finalizes a Phase 2 Handle after a
*       Phase 2 Exchange is complete. It de-allocates all
*       fields which are not required anymore. The message
*       reply timer is also removed because there will be
*       no further message exchanges. The Phase 2 Handle
*       is not removed from the database because the last
*       message sent might need retransmission. It is
*       removed from the database by the exchange time-out
*       event.
*
* INPUTS
*
*       *ph2                    Pointer to the Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Finalize_Phase2(IKE_PHASE2_HANDLE *ph2)
{
    STATUS          status;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((ph2 == NU_NULL) || (ph2->ike_sa == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Finalizing Phase 2 Handle");

#if (IKE_RETAIN_LAST_MESSAGE == NU_TRUE)
    /* Remove any message resend timers of this Handle. */
    status = IKE_Unset_Timer(IKE_Message_Reply_Event,
                             (UNSIGNED)ph2->ike_sa, (UNSIGNED)ph2);

    if(status == NU_SUCCESS)
    {
        /* Free the Phase 2 specific memory of the SA. */
        status = IKE_Free_Phase2_Memory(ph2);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to finalize/remove Phase 2 Handle",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to unset message resend timers",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
#else
    /* Mark the Handle as deleted. */
    ph2->ike_flags |= IKE_DELETE_FLAG;

    /* Enqueue event to remove the Handle. It is removed
     * through the event queue to keep all events synchronized.
     * All timer events running on this Handle are also
     * un-registered by the event handler.
     */
    IKE_SYNC_REMOVE_PHASE2(ph2->ike_sa, ph2);

    /* Set status to success. */
    status = NU_SUCCESS;
#endif

    /* Return the status. */
    return (status);

} /* IKE_Finalize_Phase2 */

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Resume_Waiting_Process
*
* DESCRIPTION
*
*       This function resumes a process waiting for a
*       Phase 2 Exchange with the specified exchange status.
*       The Phase 2 Exchange is identified by the Handle.
*       A special status of IKE_NO_UPDATE signifies that the
*       exchange status should not be updated. Use the value
*       which was last updated.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       xchg_status             Status of the exchange to be
*                               passed to the process being
*                               resumed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INDEX_NOT_FOUND     Specified process not waiting.
*
*************************************************************************/
STATUS IKE_Resume_Waiting_Process(IKE_PHASE2_HANDLE *ph2,
                                  STATUS xchg_status)
{
    STATUS          status;
    INT             xchg_index;
    UNSIGNED        event_flag;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If a process is waiting for this exchange. */
    status = IKE_Get_Exchange_Index(ph2->ike_msg_id, &xchg_index);

    if(status == NU_SUCCESS)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Resuming process waiting on Phase 2");

        /* If status is to be updated. */
        if(xchg_status != IKE_NO_UPDATE)
        {
            /* Set the exchange status. */
            IKE_Data.ike_status[xchg_index] = xchg_status;
        }

        /* Set the event flag. */
        event_flag = (UNSIGNED)(1 << xchg_index);

        /* Set event to resume the waiting process. */
        status = NU_Set_Events(&IKE_Data.ike_event_group,
                               event_flag, NU_OR);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to set event to resume waiting process",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Resume_Waiting_Process */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Resume_Waiting_Processes
*
* DESCRIPTION
*
*       This function resumes processes waiting on all Phase 2
*       Exchanges which fall under the specified Phase 1
*       Exchange, using the specified exchange status. The
*       Phase 1 Exchange is identified by the IKE SA.
*       A special status of IKE_NO_UPDATE signifies that the
*       exchange status should not be updated. Use the value
*       which was last updated.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       xchg_status             Status of the exchange to be
*                               specified to the processes
*                               being resumed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Resume_Waiting_Processes(IKE_SA *sa, STATUS xchg_status)
{
    IKE_PHASE2_HANDLE   *ph2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set pointer to the first Phase 2 Handle in the database. */
    ph2 = sa->ike_phase2_db.ike_flink;

    /* Loop for all Handles in the database. */
    while(ph2 != NU_NULL)
    {
        /* Resume the process of the current Handle, if any. */
        if(IKE_Resume_Waiting_Process(ph2, xchg_status) != NU_SUCCESS)
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("No process waiting on current exchange");
        }

        /* Move to the next Handle. */
        ph2 = ph2->ike_flink;
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Resume_Waiting_Processes */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Update_Phase2_Status
*
* DESCRIPTION
*
*       This function updates the status of the specified
*       Phase 2 Exchange. The Phase 2 Exchange is identified
*       by the Handle.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       xchg_status             New status of the exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INDEX_NOT_FOUND     Specified process not waiting.
*
*************************************************************************/
STATUS IKE_Update_Phase2_Status(IKE_PHASE2_HANDLE *ph2,
                                STATUS xchg_status)
{
    STATUS          status;
    INT             xchg_index;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If a process is waiting for this exchange. */
    status = IKE_Get_Exchange_Index(ph2->ike_msg_id, &xchg_index);

    if(status == NU_SUCCESS)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Updating Phase 2 Exchange status");

        /* Set the exchange status. */
        IKE_Data.ike_status[xchg_index] = xchg_status;
    }

    /* Return the status. */
    return (status);

} /* IKE_Update_Phase2_Status */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Update_Phase1_Status
*
* DESCRIPTION
*
*       This function updates the status of all Phase 2
*       Exchanges which are waiting on the specified Phase 1
*       Exchange. The Phase 1 Exchange is identified by the
*       IKE SA.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       xchg_status             New status of the exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Update_Phase1_Status(IKE_SA *sa, STATUS xchg_status)
{
    IKE_PHASE2_HANDLE   *ph2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set pointer to the first Phase 2 Handle in the database. */
    ph2 = sa->ike_phase2_db.ike_flink;

    /* Loop for all Handles in the database. */
    while(ph2 != NU_NULL)
    {
        /* Update status of the current Handle, if any. */
        if(IKE_Update_Phase2_Status(ph2, xchg_status) != NU_SUCCESS)
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("No process waiting on current exchange");
        }

        /* Move to the next Handle. */
        ph2 = ph2->ike_flink;
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Update_Phase1_Status */
#endif /* (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Check_Resend
*
* DESCRIPTION
*
*       This function checks whether the received message
*       has been retransmitted, and if so, it re-sends the
*       last response. The check is performed by comparing
*       the received message's hash with the hash of the
*       previous message received for an exchange. The hash
*       for an exchange is updated on a successful message
*       transmission.
*
* INPUTS
*
*       *ph1                    Pointer to a Phase 1 or Phase 2
*                               Handle. Both Handles share a
*                               common interface for message
*                               retransmission.
*
* OUTPUTS
*
*       NU_SUCCESS              Received message is not a
*                               retransmission.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_IS_RESEND           Received message is a retransmission.
*
*************************************************************************/
STATUS IKE_Check_Resend(IKE_PHASE1_HANDLE *ph1)
{
    STATUS          status;
    UINT8           dgst[IKE_MD5_DIGEST_LEN];

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Calculate MD5 hash of the message. */
    status = IKE_MD5_Hash(ph1->ike_params->ike_packet->ike_data,
                          ph1->ike_params->ike_packet->ike_data_len, dgst);

    /* Check if digest matches last received message's digest. */
    if((status == NU_SUCCESS) &&
       (memcmp(dgst, ph1->ike_last_message_hash, IKE_MD5_DIGEST_LEN) == 0))
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Received message is a retransmission");

        /* Retransmit the response. */
        if(IKE_Resend_Packet(ph1) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to retransmit IKE response message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Indicate that the incoming message is a retransmission. */
        status = IKE_IS_RESEND;
    }

    /* Return the status. */
    return (status);

} /* IKE_Check_Resend */

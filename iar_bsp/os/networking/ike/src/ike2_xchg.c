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
*       ike2_xchg.c
*
* COMPONENT
*
*       IKEv2 - Exchange
*
* DESCRIPTION
*       This file contains all functions related to proposal construction,
*       selection, generation of Key exchange, nonces and related data.
*
* FUNCTIONS
*
*       IKE2_Construct_Proposal
*       IKE2_Construct_IPsec_Proposal
*       IKE2_Select_Proposal
*       IKE2_Select_IPsec_Proposal
*       IKE2_Cleanup_Exchange
*       IKE2_Generate_KE_Data
*       IKE2_Generate_Nonce_Data
*       IKE2_Extract_IKE_SA_Attribs
*       IKE2_Verify_Auth
*       IKE2_Compute_Encryption_KeyLen
*       IKE2_Add_Transform
*       IKE2_Add_Transform_Attrib
*
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ike_api.h
*       ike_ips.h
*       ike_buf.h
*       ike_crypto_wrappers.h
*       rand.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ike_api.h"
#include "networking/ike_ips.h"
#include "networking/ike_buf.h"
#include "networking/ike_crypto_wrappers.h"
#include "openssl/rand.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#if(IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
extern UINT8 IKE_Proposal_Span2(UINT8 prop_index,
                                IPSEC_SECURITY_PROTOCOL *security,
                                UINT8 security_size);
#endif
STATIC VOID IKE2_Add_Transform(IKE2_PROPOSAL_PAYLOAD *proposal, UINT8 type,
                               UINT16 id, UINT8 is_last);
STATIC VOID IKE2_Add_Transform_Attrib(IKE2_TRANSFORMS_PAYLOAD *transform,
                                      UINT16 type, UINT16 lenval,
                                      UINT8 *value);

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Add_Transform
*
* DESCRIPTION
*
*       This is a utility function that adds an IKEv2 Transform to a proposal
*       and is called from other API functions.
*
* INPUTS
*
*       *proposal               The source proposal payload in which
*                               transform is to be added.
*       type                    Type of transform like Encryption, integrity etc.
*       id                      Specific algorithm of the above type.
*       is_last                 Flag value to indicate whether this is the
*                               last transform or not.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC
VOID IKE2_Add_Transform(IKE2_PROPOSAL_PAYLOAD *proposal, UINT8 type,
                        UINT16 id, UINT8 is_last)
{
    INT                     index;
    IKE2_TRANSFORMS_PAYLOAD *new_transform;

    /* Calculate the new index from transform count in proposal */
    index = proposal->ike2_no_of_transforms;

    /* Set pointer to new transform payload. */
    new_transform = &proposal->ike2_transforms[index];

    new_transform->ike2_more_transforms = is_last;

    new_transform->ike2_transform_type = type;
    new_transform->ike2_transform_id = id;

    /* Initialize num of attribs to none. It will be incremented when an
     * attrib is added.
     */
    new_transform->ike2_transform_attrib_num = 0;

    /* Transform length with no attributes */
    new_transform->ike2_transform_length = IKE2_MIN_TRANS_PAYLOAD_LEN;

    /* Update the proposal's total length. */
    proposal->ike2_proposal_len = proposal->ike2_proposal_len +
        new_transform->ike2_transform_length;

    /* Increment the transform count in proposal. */
    (proposal->ike2_no_of_transforms) += 1;

    return;
} /* IKE2_Add_Transform */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Add_Transform_Attrib
*
* DESCRIPTION
*
*       This is a utility function that adds an IKEv2 Transform attribute
*       to a transform and is called from other API functions.
*
* INPUTS
*
*       *transform              The source transform payload in which
*                               attribute is to be added.
*       type                    Type of attribute like key length etc.
*       lenval                  value of transform attribute if it is fixed
*                               length. Otherwise, it's length only. At
*                               Present only one fixed length attribute is
*                               used.
*       *value                  Pointer to variable length data if it exists
*                               and length is equal to the above field lenval.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID IKE2_Add_Transform_Attrib(IKE2_TRANSFORMS_PAYLOAD *transform,
                                      UINT16 type, UINT16 lenval,
                                      UINT8 *value)
{
    /* Set pointer to the first and only transform attribute */
    IKE2_TRANS_ATTRIB_PAYLOAD *trans_attrib =
        &transform->ike2_transform_attrib[0];

    trans_attrib->ike2_attrib_type = type;
    trans_attrib->ike2_attrib_lenval = lenval;

    /* Add minimum length of a trans attribute to transform length */
    transform->ike2_transform_length += IKE2_MIN_TRANS_ATTRIB_LEN;

    /* If the attribute is variable, add it value as well as length */
    if (value != NU_NULL)
    {
        NU_BLOCK_COPY(trans_attrib->ike2_attrib_value, value, lenval);

        /* When attrib is variable then lenval field is treated as length */
        transform->ike2_transform_length = transform->ike2_transform_length
            + lenval;
        trans_attrib->ike2_attrib_type |= IKE2_TRANS_ATTRIB_AF_TLV;
    }

    else
    {
        trans_attrib->ike2_attrib_type |= IKE2_TRANS_ATTRIB_AF_TV;
    }

    /* Increment the transform attribute number. */
    transform->ike2_transform_attrib_num++;

    return;
} /* IKE2_Add_Transform_Attrib */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Compute_Encryption_KeyLen
*
* DESCRIPTION
*
*       This is a utility function that computes key length through
*       calculating index into encryption algorithms array and is called
*       from other API functions.
*
* INPUTS
*
*       *security               Source security protocol to identify
*                               encryption algorithm used.
*       *key_len                Upon return this field contains the
*                               resultant key length computed by this
*                               function as an out parameter.
*
* OUTPUTS
*
*       NU_SUCCESS              If no error occurred and key calculated.
*
*************************************************************************/
STATUS IKE2_Compute_Encryption_KeyLen(IPSEC_SECURITY_PROTOCOL *security,
                                      UINT16 *key_len)
{
    UINT8           algo;
    STATUS          status = NU_SUCCESS;

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
            *key_len =
                IPSEC_Encryption_Algos[(INT)algo].ipsec_key_len;

            /* Make sure key length is not zero. */
            if(*key_len != 0)
            {
                /* Convert key length from bytes to bits. */
                *key_len = (UINT16)IKE_BYTES_TO_BITS(*key_len);
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

    return (status);
} /* IKE2_Compute_Encryption_KeyLen */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Construct_Proposal
*
* DESCRIPTION
*
*       This is a utility function used by the IKE_SA_INIT exchange
*       initiator. It takes an IKEv2 policy and constructs an SA payload
*       from it. The SA payload is suitable to initiate a negotiation.
*
* INPUTS
*
*       *handle                 Pointer to exchange handle containing the
*                               IKEv2 SA and policy.
*
* OUTPUTS
*
*       NU_SUCCESS              If no error occurred and proposal constructed.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Construct_Proposal(IKE2_EXCHANGE_HANDLE *handle)
{
    INT                     i;
    INT                     j;
    IKE2_POLICY             *policy;
    IKE2_SA_PAYLOAD         *out_sa;
    IKE_ATTRIB              *policy_attrib;

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if (handle == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be present. */
    else if(handle->ike2_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Constructing Initiator's proposal");

    /* Set local pointer to commonly used data in the Handle. */
    policy = handle->ike2_params->ike2_policy;
    out_sa = handle->ike2_params->ike2_out.ike2_sa;

    /* Loop for each index in the policy attrib array and add one proposal
     * per entry. Then add a transform for each field in the index to the
     * current proposal
     */
    for (i=0; i<policy->ike_xchg1_attribs_no ; i++)
    {
        /* Set pointer to the current policy attribute. */
        policy_attrib = &policy->ike_xchg1_attribs[i];

        /* Indicate that the current proposal array would contain more
         * proposals. The last index would be overwritten when we are sure
         * of this upon loop termination. This corresponds to the first
         * field in an IKEv2 Proposal payload RFC 4306 section-3.3.1.
         */
        out_sa->ike2_proposals[i].ike2_more_proposals = IKE2_PRPSL_MORE_VALUE;

        /* Assign Proposal number which is always in sequential order. */
        out_sa->ike2_proposals[i].ike2_proposal_num = (UINT8) (i + 1);

        /* Protocol is IKE (ISAKMP in v1) */
        out_sa->ike2_proposals[i].ike2_protocol_id = IKE2_PROTO_ID_IKE;

        /* Each proposal initially has no transforms. */
        out_sa->ike2_proposals[i].ike2_no_of_transforms = 0;

        /* Set the length needed for the proposal header. */
        out_sa->ike2_proposals[i].ike2_proposal_len = IKE2_MIN_PRPSL_PAYLOAD_LEN;

        /* For an initial IKE_SA negotiation, this field MUST be zero; the
         * SPI is obtained from the outer header. During subsequent
         * negotiations, it is equal to the size, in octets, of the SPI of
         * the corresponding protocol (8 for IKE, 4 for ESP and AH).
         */
        if((handle->ike2_flags & IKE2_SA_REKEY) == 0)
        {
            out_sa->ike2_proposals[i].ike2_spi_size = 0;
        }

        else
        {
            /* Set the SPI's length. */
            out_sa->ike2_proposals[i].ike2_spi_size = IKE2_SPI_LENGTH;

            /* Generate new SPI. */
            RAND_bytes(out_sa->ike2_proposals[i].ike2_spi, IKE2_SPI_LENGTH);

            NU_BLOCK_COPY(handle->ike2_new_sa->ike2_local_spi,
                          out_sa->ike2_proposals[i].ike2_spi,
                          IKE2_SPI_LENGTH);

            /* Update the proposal length. */
            out_sa->ike2_proposals[i].ike2_proposal_len =
                out_sa->ike2_proposals[i].ike2_proposal_len + IKE2_SPI_LENGTH;
        }

        /* Add transforms one by one. Presently implementation supports only
         * four transforms in IKE with one only with a single transform
         * attribute i.e. Encryption algo with Key length attribute.
         */
        IKE2_Add_Transform(&out_sa->ike2_proposals[i],IKE2_TRANS_TYPE_ENCR,
            policy_attrib->ike_encryption_algo, IKE2_TRANS_MORE_VALUE);

        /* If encryption algorithm key length has been specified, then add
         * a corresponding transform attribute.
         */
        if(policy_attrib->ike_key_len != IKE_WILDCARD)
        {
            IKE2_Add_Transform_Attrib(&(out_sa->ike2_proposals[i].
                ike2_transforms[0]),IKE2_KEY_LEN_ID,
                (UINT16)IKE_BYTES_TO_BITS(policy_attrib->ike_key_len), NU_NULL);

            /* After adding the attribute we have to update the
             * proposal length as well by adding only the new
             * attribute's length to proposal.The Key Length
             * is a AF-TV(Attribute Format Type/Value) so
             * does not have variable length.
             */
            out_sa->ike2_proposals[i].ike2_proposal_len +=
                                    IKE2_MIN_TRANS_ATTRIB_LEN;
        }

        IKE2_Add_Transform(&out_sa->ike2_proposals[i], IKE2_TRANS_TYPE_INTEG,
            policy_attrib->ike_hash_algo, IKE2_TRANS_MORE_VALUE);

        IKE2_Add_Transform(&out_sa->ike2_proposals[i], IKE2_TRANS_TYPE_PRF,
            policy_attrib->ike2_prf_algo, IKE2_TRANS_MORE_VALUE);

        IKE2_Add_Transform(&out_sa->ike2_proposals[i], IKE2_TRANS_TYPE_DH,
            policy_attrib->ike_group_desc, IKE2_TRANS_LAST_VALUE);


    } /* End loop filling each proposal */

    if ( (i > 0) && (i <= IKE2_NUM_OF_PROPOSALS) )
    {
        /* Ensure that the last proposal has it's indication. Loop counter
         * has incremented beyond the last proposal number so subtract one
         * from the index.
         */
        out_sa->ike2_proposals[i-1].ike2_more_proposals = IKE2_PRPSL_LAST_VALUE;
    }

    /* Indicate that number of proposals in the out sa filled from the
     * local policy attrib array. This field is not an RFC requirement
     * but it simplifies (and is used while) encoding the outgoing proposal
     * list.
     */
    out_sa->ike2_proposals_num =  policy->ike_xchg1_attribs_no;

    /* Calculate the SA payload length i.e. sum of all proposals length in
     * payload plus the generic payload header length
     */
    out_sa->ike2_gen.ike2_payload_length = IKE2_MIN_SA_PAYLOAD_LEN;

    for (j=0; j<out_sa->ike2_proposals_num; j++)
    {
        out_sa->ike2_gen.ike2_payload_length = out_sa->
            ike2_gen.ike2_payload_length + out_sa->ike2_proposals[j].
            ike2_proposal_len;
    }

    out_sa->ike2_gen.ike2_payload_type = IKE2_SA_PAYLOAD_ID;
    out_sa->ike2_gen.ike2_critical = 0;

    /* Add SA payload to the payloads chain. */
    IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
        out_sa, IKE2_SA_PAYLOAD_ID);

    return NU_SUCCESS;

} /* IKE2_Construct_Proposal */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Construct_IPsec_Proposal
*
* DESCRIPTION
*
*       This is a utility function used by IKE to construct an IPsec
*       proposal from the initiator. It takes an IPsec policy and
*       constructs an SA payload from it.
*
* INPUTS
*
*       *handle                 Pointer to exchange handle containing the
*                               IKEv2 SA and policy.
*
* OUTPUTS
*
*       NU_SUCCESS              If no error occurred and proposal constructed.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Construct_IPsec_Proposal(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_SUCCESS;
    INT                     i;
    IKE2_SA_PAYLOAD         *out_sa;
    IKE2_IPS_SA             *sa2;
    IPSEC_SECURITY_PROTOCOL *security;
    IKE2_TRANSFORMS_PAYLOAD *out_transform;
    UINT16                  key_len;
    UINT8                   auth_algo;

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if (handle == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be present. */
    else if(handle->ike2_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Constructing Initiator's proposal");

    /* Set SA payload pointer. */
    out_sa = handle->ike2_params->ike2_out.ike2_sa;

    /* Set the SA2 pointer to first item in DB. */
    sa2 = handle->ike2_sa2_db.ike_flink;

    /* Initialize header fields of the SA payload. */
    out_sa->ike2_gen.ike2_payload_length = IKE2_MIN_SA_PAYLOAD_LEN;
    out_sa->ike2_gen.ike2_next_payload = NU_NULL;
    out_sa->ike2_gen.ike2_payload_type = IKE2_SA_PAYLOAD_ID;
    out_sa->ike2_gen.ike2_critical = 0;

    /* For each entry in the local IPsec SADB i.e. 'sa2', construct an
     * entry in the outgoing proposal array using 'i' as index.
     */
    for(i = 0; sa2 != NU_NULL; i++)
    {
        /* Set pointer to the IPsec security protocol. */
        security = &sa2->ike_ips_security;

        /* Initialize all fields of the current proposal. The number of
         * first proposal should always be 1. Since our implementation
         * of IPsec doesn't support an OR combination of proposals, hence
         * every element in the proposal array has a proposal number 1.
         */
        out_sa->ike2_proposals[i].ike2_proposal_num = 1;

        /* Initialize the number of transforms field. This will be
         * incremented automatically with addition of each transform.
         */
        out_sa->ike2_proposals[i].ike2_no_of_transforms = 0;

        /* Copy the local SPI to the proposal. */
        out_sa->ike2_proposals[i].ike2_spi_size = IKE_IPS_SPI_LEN;
        PUT32(out_sa->ike2_proposals[i].ike2_spi, 0, sa2->ike_local_spi);

        /* Initialize the proposal length. This does not include the length
         * of transforms which will be updated later.
         */
        out_sa->ike2_proposals[i].ike2_proposal_len =
            IKE2_MIN_PRPSL_PAYLOAD_LEN + IKE_IPS_SPI_LEN;

        out_sa->ike2_proposals[i].ike2_protocol_id =
            IKE_Protocol_ID_IPS_To_IKE(security->ipsec_protocol);

        /* If protocol is AH.then Integrity Algorithm transform is
         * to be added.
         */
        if(out_sa->ike2_proposals[i].ike2_protocol_id == IKE2_PROTO_ID_AH)
        {
            /* Add the AH transform. IPsec Algorithm id should be mapped to
             * IKE transform ID.
             */
            IKE2_Add_Transform(&out_sa->ike2_proposals[i],
                IKE2_TRANS_TYPE_INTEG,
                IKE2_AH_Trans_ID_IPS_To_IKE(security->ipsec_auth_algo),
                IKE2_TRANS_MORE_VALUE);
        }

        /* But if protocol is ESP then Encryption Algorithm transform and
         * it's key length attribute(optional) is to be added. Moreover, we
         * must check if Integrity Algorithm transform is to be added as
         * well or not.
         */
        else if(out_sa->ike2_proposals[i].ike2_protocol_id == IKE2_PROTO_ID_ESP)
        {
            IKE2_Add_Transform(&out_sa->ike2_proposals[i],
                IKE2_TRANS_TYPE_ENCR,
                IKE2_ESP_Trans_ID_IPS_To_IKE(security->ipsec_encryption_algo),
                IKE2_TRANS_MORE_VALUE);

            /* If the encryption algo is AES, Only then we will add an
             * attribute to this transform specifying key length to be
             * negotiated with the other side as per RFC 4306.
             */
            if(IKE2_ESP_Trans_ID_IPS_To_IKE(security->ipsec_encryption_algo)
                == IKE2_ENCR_AES_CBC)
            {
                /* Now calculate key length. */
                if( IKE2_Compute_Encryption_KeyLen(security, &key_len) ==
                    NU_SUCCESS )
                {
                    /* If key length found then set pointer to the current
                     * transform payload
                     */
                    out_transform = &out_sa->ike2_proposals[i].
                        ike2_transforms[0];

                    /* Add encryption key length attribute. */
                    IKE2_Add_Transform_Attrib(out_transform, IKE2_KEY_LEN_ID,
                        key_len, NU_NULL);

                    /* After adding the attribute we have to update the
                     * proposal length as well by adding only the new
                     * attribute's length to proposal.The Key Length
                     * is a AF-TV(Attribute Format Type/Value) so
                     * does not have variable length.
                     */
                    out_sa->ike2_proposals[i].ike2_proposal_len +=
                                            IKE2_MIN_TRANS_ATTRIB_LEN;
                }
            }

            /* Verify if a valid Integrity Algorithm is present */
            auth_algo = IKE2_AH_Trans_ID_IPS_To_IKE(security->ipsec_auth_algo);

            if(auth_algo != 0)
            {
                /* Then add the optional Integrity transform as well. */
                IKE2_Add_Transform(&out_sa->ike2_proposals[i],
                    IKE2_TRANS_TYPE_INTEG, auth_algo, IKE2_TRANS_MORE_VALUE);
            }
        }

        else
        {
            /* Unrecognized protocol ID. */
            status = IKE_INVALID_PROTOCOL;
            break;
        }

        /* If ESN is enabled/disabled add its respective transform to the
         * Proposal.
         */
        if((security->ipsec_flags & IPSEC_ENABLE_ESN))
        {
            IKE2_Add_Transform(&out_sa->ike2_proposals[i],
                               IKE2_TRANS_TYPE_ESN, IKE2_ENABLED_ESN_VALUE,
                               IKE2_TRANS_LAST_VALUE);

            /* Set the corresponding flag in the exchange handle. */
            handle->ike2_sa->ike2_flags |= IKE2_USE_IPS_ESN;
        }

        else
        {
            IKE2_Add_Transform(&out_sa->ike2_proposals[i],
                               IKE2_TRANS_TYPE_ESN, IKE2_NO_ESN_VALUE,
                               IKE2_TRANS_LAST_VALUE);
        }

        /* Now update the payload length in generic header. */
        out_sa->ike2_gen.ike2_payload_length =
            out_sa->ike2_gen.ike2_payload_length +
            out_sa->ike2_proposals[i].ike2_proposal_len;

        /* Move to the next SA2 item. */
        sa2 = sa2->ike_flink;
    }

    if(i != 0)
    {
        out_sa->ike2_proposals[i-1].ike2_more_proposals = IKE2_PRPSL_LAST_VALUE;
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Number of proposals added to the current chain. This will always
         * be one since our implementation of IPsec supports only one
         * set of proposals that are ANDed together.
         */
        out_sa->ike2_proposals_num = 1;

        /* Add SA payload to the payloads chain. */
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                          out_sa, IKE2_SA_PAYLOAD_ID);
    }

    return (status);

} /* IKE2_Construct_IPsec_Proposal */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Select_Proposal
*
* DESCRIPTION
*
*       This is a utility function used by Responder. It
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
*       *handle                 Pointer to the Exchange Handle.
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
*                               Only IKE_KEY transform is allowed.
*       IKE_TOO_MANY_PROPOSALS  The SA has more than one proposals.
*       IKE_UNSUPPORTED_ATTRIB  Attribute in SA is not supported.
*       IKE_NOT_NEGOTIABLE      Proposal not negotiable using
*                               the specified policy.
*       IKE2_TOO_FEW_TRANSFORMS Incomplete transform set.
*       IKE2_NOT_FOUND          Matching proposal not found.
*
*************************************************************************/
STATUS IKE2_Select_Proposal(IKE2_EXCHANGE_HANDLE *handle,
                            IKE_ATTRIB **ret_policy_attrib)
{
    STATUS                     status = IKE2_NOT_MATCHED;
    INT                        i; /* for each proposal */
    INT                        j; /* for each policy attribute */
    INT                        k; /* for each transform */
    IKE2_SA_PAYLOAD            *in_sa;
    IKE2_SA_PAYLOAD            *out_sa;
    IKE2_KE_PAYLOAD            *in_ke;
    IKE2_TRANSFORMS_PAYLOAD    *in_transform = NU_NULL;
    UINT8                      num_of_transforms;
    UINT8                      is_match;
    UINT8                      dh_notify[2];

    /* Policy against which we shall compare the in coming SA. */
    IKE_ATTRIB                 *policy_attrib;

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((handle == NU_NULL) || (ret_policy_attrib == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Selecting from Initiator's proposal");

    /* Set local pointers to commonly used data in the Handle. */
    in_sa  = handle->ike2_params->ike2_in.ike2_sa;
    out_sa  = handle->ike2_params->ike2_out.ike2_sa;
    in_ke  = handle->ike2_params->ike2_in.ike2_ke;

    /* Loop for each proposal. Each index in the attrib array in policy
     * on receiving side is considered a single proposal and is compared
     * to the list of proposals in the input sa. The below proposals_num
     * field indicating the number of incoming proposals was filled by the
     * packet decoding function.
     */
    for (i = 0; i < in_sa->ike2_proposals_num; i++)
    {
        /* Protocol being negotiated must be IKE. */
        if(in_sa->ike2_proposals[i].ike2_protocol_id != IKE2_PROTO_ID_IKE)
        {
            NLOG_Error_Log("Protocol must be IKE in Initial Exchange proposal",
                NERR_RECOVERABLE, __FILE__, __LINE__);

            status = IKE_INVALID_PROTOCOL;
            break;
        }

        else /* Proceed with the rest of proposals */
        {
            num_of_transforms = in_sa->ike2_proposals[i].ike2_no_of_transforms;

            /* If the current incoming proposal contains fewer transforms
             * than the minimum for IKE i.e. 4, then reject it and proceed
             * to the next proposal.
             */
            if(num_of_transforms < IKE2_MIN_TRANSFORMS)
            {
                status = IKE2_TOO_FEW_TRANSFORMS;
            }

            else
            {
                /* Loop for each element in the IKE attrib array. */
                for (j = 0;
                     j < handle->ike2_params->ike2_policy->ike_xchg1_attribs_no;
                     j++)
                {
                    /* Initialize all transform flags to false (not found) */
                    is_match = IKE2_NOT_FOUND;

                    /* Get pointer to current policy attribute. */
                    policy_attrib = &handle->ike2_params->ike2_policy->
                                                    ike_xchg1_attribs[j];

                    /* Loop for each transform in the current proposal and
                     * compare with corresponding attrib element.
                     */
                    for (k = 0; k < num_of_transforms; k++)
                    {
                        /* Log debug message. */
                        IKE2_DEBUG_LOG("Matching proposal's transform. ");

                        /* Get pointer to current transform. */
                        in_transform = &in_sa->ike2_proposals[i].
                            ike2_transforms[k];

                        /* Check each type of transform and see if policy
                         * attribs contain a matching algorithm.
                         */
                        switch(in_transform->ike2_transform_type)
                        {
                            case IKE2_TRANS_TYPE_ENCR:
                                /* If algo already matched then skip it. */
                                if((is_match & IKE2_ENCR_FLAG) == IKE2_NOT_FOUND)
                                {
                                    /* If the incoming encryption algo
                                     * matches the one in our local policy.
                                     */
                                    if(in_transform->ike2_transform_id ==
                                    policy_attrib->ike_encryption_algo)
                                    {
                                        /* Check if any transform attribute
                                         * exist and compare it. This will
                                         * happen only when AES is used and
                                         * both sides have specified key length.
                                         */
                                        if(in_transform->
                                        ike2_transform_attrib_num > 0)
                                        {
                                            /* Key length for Encryption algo
                                             * is the only possible attribute
                                             * so we directly check for this
                                             * field only (RFC 4306)Sec 3.3.5
                                             * and RFC (4718) Sec. 7.11.
                                             */
                                            if(IKE_BYTES_TO_BITS(policy_attrib->ike_key_len) ==
                                            in_transform->ike2_transform_attrib[0].
                                            ike2_attrib_lenval)
                                            {
                                                is_match |= IKE2_ENCR_FLAG;
                                            }
                                        }

                                        else
                                        {
                                            /* If encryption algo matched
                                             * and no attribute specified
                                             */
                                            is_match |= IKE2_ENCR_FLAG;
                                        }
                                    }
                                }
                                break;

                            case IKE2_TRANS_TYPE_PRF:
                                /* If algo already matched then skip it. */
                                if((is_match & IKE2_PRF_FLAG)== IKE2_NOT_FOUND)
                                {
                                    /* If the incoming prf algo matches the
                                     * one in our local policy.
                                     */
                                    if(in_transform->ike2_transform_id ==
                                    policy_attrib->ike2_prf_algo)
                                    {
                                        is_match |= IKE2_PRF_FLAG;
                                    }
                                }
                                break;

                            case IKE2_TRANS_TYPE_INTEG:
                                /* If algo already matched then skip it. */
                                if((is_match & IKE2_INTEG_FLAG)== IKE2_NOT_FOUND)
                                {
                                    /* If the incoming integrity algo
                                     * matches the one in our local policy.
                                     */
                                    if(in_transform->ike2_transform_id ==
                                    policy_attrib->ike_hash_algo)
                                    {
                                        is_match |= IKE2_INTEG_FLAG;
                                    }
                                }
                                break;

                            case IKE2_TRANS_TYPE_DH:
                                /* If algo already matched then skip it. */
                              if((is_match & IKE2_DIFFIE_HELLMAN_FLAG)==
                                  IKE2_NOT_FOUND)
                              {
                                  /* If the incoming Diffie Hellman group
                                   * matches the one in our local policy.
                                   */
                                    if(in_transform->ike2_transform_id ==
                                    policy_attrib->ike_group_desc)
                                    {
                                        /* If the KE payload has been sent,
                                         * make sure it matches the
                                         * Diffie-Hellman group which we
                                         * are expecting.
                                         */
                                        if((in_ke != NU_NULL) &&
                                           (in_ke->ike2_dh_group_no !=
                                            policy_attrib->ike_group_desc))
                                        {
                                            /* Copy the payload type as
                                             * notification data. */
                                            PUT16(dh_notify, 0,
                                                policy_attrib->
                                                ike_group_desc);

                                            /* Set the Remote SPI in the
                                             * SA since it is needed in
                                             * the response. */
                                            NU_BLOCK_COPY(
                                                handle->ike2_sa->ike2_remote_spi,
                                                handle->ike2_params->ike2_in.
                                                    ike2_hdr->ike2_sa_spi_i,
                                                IKE2_SPI_LENGTH);

                                            status = IKE2_INVALID_KE_PAYLOAD;
                                        }
                                        else
                                        {
                                            is_match |=
                                                IKE2_DIFFIE_HELLMAN_FLAG;
                                        }
                                    }
                                }
                                break;

                            default:
                                /* Incoming transform does not exist in the
                                 * current security index. However, proposal
                                 * can still match.
                                 */
                                break;
                        } /* End transform type switch */
                    } /* End transforms loop */

                    /* If all the algorithms matched from the current
                     * proposal then fill the out sa and break.
                     * Otherwise look into the next proposal.
                     */
                    if( (is_match & IKE2_ENCR_FLAG) &&
                        (is_match & IKE2_PRF_FLAG) &&
                        (is_match & IKE2_INTEG_FLAG) &&
                        (is_match & IKE2_DIFFIE_HELLMAN_FLAG))
                    {
                        /* Log debug message. */
                        IKE2_DEBUG_LOG("Suitable proposal selected");

                        /* Set pointer to the Policy attributes which
                         * matched the proposal.
                         */

                        *ret_policy_attrib = &handle->ike2_params->
                                             ike2_policy->ike_xchg1_attribs[j];

                        /* Change the status value to indicate a
                         * matching proposal has been found.
                         */
                        status = NU_SUCCESS;

                        /* If there is an SPI present in the proposal, as
                         * may be the case when re-keying IKE SA, copy
                         * that to exchange handle.
                         */
                        if((in_sa->ike2_proposals[i].ike2_spi_size > 0) &&
                            (handle->ike2_new_sa != NU_NULL))
                        {
                            NU_BLOCK_COPY(
                                handle->ike2_new_sa->ike2_remote_spi,
                                in_sa->ike2_proposals[i].ike2_spi,
                                IKE2_SPI_LENGTH);
                        }
                    }

                    /* Set values in out SA only if we are responder,
                     * otherwise it is just a verification of peer's
                     * selection.
                     */
                    if(((handle->ike2_flags & IKE2_RESPONDER) != 0) &&
                        (status == NU_SUCCESS))
                    {
                        /* Since we select only one matching proposal */
                        out_sa->ike2_proposals_num = 1;

                        /* We always fill only the first proposal */
                        out_sa->ike2_proposals[0].ike2_more_proposals =
                            IKE2_PRPSL_LAST_VALUE;

                        /* Fill the proposal number that is matched */
                        out_sa->ike2_proposals[0].ike2_proposal_num =
                            in_sa->ike2_proposals[i].ike2_proposal_num;

                        /* Fill protocol ID. */
                        out_sa->ike2_proposals[0].ike2_protocol_id =
                            IKE2_PROTO_ID_IKE;

                        /* Fill SPI and it's size */
                        out_sa->ike2_proposals[0].ike2_spi_size =
                            in_sa->ike2_proposals[i].ike2_spi_size;
                        if(out_sa->ike2_proposals[0].ike2_spi_size > 0)
                        {
                            NU_BLOCK_COPY(out_sa->ike2_proposals[0].ike2_spi,
                                in_sa->ike2_proposals[i].ike2_spi,
                                out_sa->ike2_proposals[0].ike2_spi_size);
                        }
                        break;
                    }

                } /* End IKE attributes loop */

                /* We haven't found a matching proposal yet, continue looking
                 * through the attribs array
                 */
            }
        }
    }  /* end loop for each proposal */

    /* If a matching proposal and a valid index into the local policy
     * structure containing that proposal has been found and we are not the
     * initiator of the current IKEv2 exchange, then fill the out_sa.
     */
    if((status == NU_SUCCESS) && (*ret_policy_attrib != NU_NULL) &&
        ((handle->ike2_flags & IKE2_RESPONDER) != 0))
    {
        out_sa->ike2_gen.ike2_critical = 0;
        out_sa->ike2_gen.ike2_payload_length = IKE2_GEN_HDR_TOTAL_LEN;
        out_sa->ike2_proposals->ike2_proposal_len = IKE2_MIN_PRPSL_PAYLOAD_LEN;

        /* Fill out all the matching transforms */

        IKE2_Add_Transform(&out_sa->ike2_proposals[0],IKE2_TRANS_TYPE_ENCR,
            (*ret_policy_attrib)->ike_encryption_algo, IKE2_TRANS_MORE_VALUE);
        if((*ret_policy_attrib)->ike_key_len != IKE_WILDCARD)
        {
            IKE2_Add_Transform_Attrib(&(out_sa->ike2_proposals[0].
                ike2_transforms[0]), IKE2_KEY_LEN_ID,
               (UINT16)IKE_BYTES_TO_BITS((*ret_policy_attrib)->ike_key_len), NU_NULL);

            /* Since the above attribute is TV form, we need only add the
             * minimum attribute length to the proposal's total length.
             */
            out_sa->ike2_proposals[0].ike2_proposal_len +=
                IKE2_MIN_TRANS_ATTRIB_LEN;
        }

        IKE2_Add_Transform(&out_sa->ike2_proposals[0], IKE2_TRANS_TYPE_INTEG,
            (*ret_policy_attrib)->ike_hash_algo, IKE2_TRANS_MORE_VALUE);

        IKE2_Add_Transform(&out_sa->ike2_proposals[0],IKE2_TRANS_TYPE_PRF,
            (*ret_policy_attrib)->ike2_prf_algo, IKE2_TRANS_MORE_VALUE);

        IKE2_Add_Transform(&out_sa->ike2_proposals[0], IKE2_TRANS_TYPE_DH,
            (*ret_policy_attrib)->ike_group_desc, IKE2_TRANS_LAST_VALUE);

        out_sa->ike2_gen.ike2_payload_length = out_sa->ike2_gen.
        ike2_payload_length + out_sa->ike2_proposals[0].ike2_proposal_len;

        if( (i > 0) && (i <= IKE2_NUM_OF_PROPOSALS) &&
            (in_sa->ike2_proposals[i-1].ike2_spi_size > 0) &&
            (handle->ike2_new_sa != NU_NULL) )
        {
            /* This is a IKE SA re-key exchange. Generate new SPI for
             * new SA.
             */
            RAND_bytes(handle->ike2_new_sa->ike2_local_spi,
                       IKE2_SPI_LENGTH);

            /* Copy the SPI into the outgoing proposal. */
            NU_BLOCK_COPY(out_sa->ike2_proposals[i-1].ike2_spi,
                          handle->ike2_new_sa->ike2_local_spi,
                          IKE2_SPI_LENGTH);

            /* Set the length of SPI in the proposal. */
            out_sa->ike2_proposals[i-1].ike2_spi_size = IKE2_SPI_LENGTH;

            /* Now update the proposal length to reflect the size of SPI
             * added to the proposal.
             */
            out_sa->ike2_proposals[i-1].ike2_proposal_len += IKE2_SPI_LENGTH;

            out_sa->ike2_gen.ike2_payload_length =
                out_sa->ike2_gen.ike2_payload_length + IKE2_SPI_LENGTH;
        }

        /* Add SA payload to the payloads chain. */
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last, out_sa,
            IKE2_SA_PAYLOAD_ID);
    }

    else if((status != NU_SUCCESS) || (*ret_policy_attrib == NU_NULL))
    {
        NLOG_Error_Log("Could not select an appropriate proposal.",
            NERR_RECOVERABLE, __FILE__, __LINE__);

        if(status == IKE2_INVALID_KE_PAYLOAD)
        {
            NLOG_Error_Log("Incorrect DH guess by initiator. Notifying.",
                NERR_RECOVERABLE, __FILE__, __LINE__);

            if(IKE2_Send_Info_Notification(handle, IKE2_PROTO_ID_RESERVED,
                                           0,       /* No SPI, zero size */
                                           IKE2_NOTIFY_INVALID_KE_PAYLOAD,
                                           NU_NULL, /* No SPI */
                                           dh_notify, sizeof(dh_notify),
                                           IKE2_SA_INIT) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to send Invalid KE notification",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        /* We are the initiator and we don't need to fill the out sa. Just
         * log debug message.
         */
        IKE2_DEBUG_LOG("Accepted proposal has been verified.");
    }

    return (status);

} /* IKE2_Select_Proposal */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Extract_IKE_SA_Attribs
*
* DESCRIPTION
*
*       This function copies the selected attributes from proposals to
*       IKEv2 SA.
*
* INPUTS
*
*       *attribs                Selected attributes.
*       *sa                     SA to be populated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters to this function are invalid.
*
*************************************************************************/
STATUS IKE2_Extract_IKE_SA_Attribs(IKE2_ATTRIB *attribs, IKE2_SA *sa)
{
    STATUS      status = NU_SUCCESS;

#if (IKE2_DEBUG == NU_TRUE)
    if((attribs == NU_NULL) || (sa == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Copy the negotiated values in current SA. */
    if(attribs->ike_sa_lifetime.ike_no_of_secs == IKE_WILDCARD)
        sa->ike_attributes.ike_sa_lifetime.ike_no_of_secs = IKE2_SA_TIMEOUT;
    else
        sa->ike_attributes.ike_sa_lifetime.ike_no_of_secs = attribs->
                                                 ike_sa_lifetime.ike_no_of_secs;

    /* Compute the indexes of all algorithms and get their corresponding ID
     * in cipher suite. This ID is placed in SA.
     */
    status =  IKE2_ENCRYPTION_ALGO_INDEX(attribs->ike_encryption_algo,
                               sa->ike_attributes.ike_encryption_algo);

   if(status == NU_SUCCESS)
   {
       status = IKE2_HASH_ALGO_INDEX(attribs->ike_hash_algo,
           sa->ike_attributes.ike_hash_algo);

       if(status == NU_SUCCESS)
       {
           status = IKE2_PRF_ALGO_INDEX(attribs->ike2_prf_algo,
                        sa->ike_attributes.ike2_prf_algo);

           if(status == NU_SUCCESS)
           {
               sa->ike_attributes.ike_auth_method = attribs->ike_auth_method;
               sa->ike_attributes.ike_group_desc = attribs->ike_group_desc;
               sa->ike_attributes.ike_key_len = attribs->ike_key_len;

               /* If key length was not specified. */
               if(sa->ike_attributes.ike_key_len == IKE_WILDCARD)
               {
                   status = IKE_Crypto_Enc_Key_Len(
                       IKE_Encryption_Algos[sa->ike_attributes.ike_encryption_algo].crypto_algo_id,
                       &sa->ike_attributes.ike_key_len);
               }

               else
               {
                   /* Key length already set, verify that it is correct. */
                   status = IKE_Crypto_Enc_Key_Len(
                       IKE_Encryption_Algos[sa->ike_attributes.ike_encryption_algo].crypto_algo_id,
                       &sa->ike_attributes.ike_key_len);
                   if(status != NU_SUCCESS)
                   {
                       NLOG_Error_Log("Invalid or unsupported key length",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
                       status = IKE_INVALID_KEYLEN;
                   }
               }

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
               if(IKE2_IS_PSK_METHOD(attribs->ike_auth_method))
               {
                   sa->ike_attributes.ike2_psk_local =
                       attribs->ike2_psk_local;
                   sa->ike_attributes.ike2_psk_local_len =
                       attribs->ike2_psk_local_len;
               }
#endif

               /* The remote PSK keys must be set unconditionally because
                * they may even be used when the local host uses RSA. */
               sa->ike_attributes.ike2_psk_remote =
                   attribs->ike2_psk_remote;
               sa->ike_attributes.ike2_psk_remote_len =
                   attribs->ike2_psk_remote_len;

               sa->ike_attributes.ike_remote_key =
                   attribs->ike2_psk_remote;
               sa->ike_attributes.ike_remote_key_len =
                   attribs->ike2_psk_remote_len;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
               if(IKE2_IS_RSA_METHOD(attribs->ike_auth_method))
               {
                   /* Copy the local certificate files' paths */
                   if((attribs->ike_local_cert_file != NU_NULL) &&
                       (attribs->ike_local_key_file != NU_NULL) )
                   {
                       sa->ike_attributes.ike_local_cert_file =
                           attribs->ike_local_cert_file;
                       sa->ike_attributes.ike_local_key_file =
                           attribs->ike_local_key_file;
                   }
               }

               sa->ike_attributes.ike_cert_encoding =
                   attribs->ike_cert_encoding;

               if(attribs->ike_ca_cert_file != NU_NULL)
               {
                   sa->ike_attributes.ike_ca_cert_file =
                       attribs->ike_ca_cert_file;
               }

               if(attribs->ike_peer_cert_file != NU_NULL)
               {
                   sa->ike_attributes.ike_peer_cert_file =
                       attribs->ike_peer_cert_file;
               }

               if(attribs->ike_crl_file != NU_NULL)
               {
                   sa->ike_attributes.ike_crl_file =
                       attribs->ike_crl_file;
               }

#if(IKE_INCLUDE_PEM == NU_TRUE)
               if(attribs->ike_pem_callback != NU_NULL)
               {
                   sa->ike_attributes.ike_pem_callback =
                       attribs->ike_pem_callback;
               }
#endif /* (IKE_INCLUDE_PEM == NU_TRUE) */

               /* Set status to success. */
               status = NU_SUCCESS;
#endif
           }

           else
           {
               NLOG_Error_Log("Failed to get prf algo index",
                   NERR_RECOVERABLE, __FILE__, __LINE__);
           }
       }

       else
       {
           NLOG_Error_Log("Failed to get hash algo index",
               NERR_RECOVERABLE, __FILE__, __LINE__);
       }
   }

   else
   {
       NLOG_Error_Log("Failed to get encryption algo index",
           NERR_RECOVERABLE, __FILE__, __LINE__);
   }

   return (status);

} /* IKE2_Extract_IKE_SA_Attribs */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Select_IPsec_Proposal
*
* DESCRIPTION
*
*       This is a utility function used by Responder. It
*       takes an SA payload with multiple proposals and matches
*       those against the security protocol in the IPsec Policy.
*       If a match is found, an IKEv2 SA payload suitable for a
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
*       - IKE2_SA_PAYLOAD
*
* INPUTS
*
*       *handle                 Pointer to the Exchange Handle.
*       *security               Pointer to the IPsec Securities
*                               from the IPsec Policy.
*       security_size           Number of IPsec securities in
*                               the array.
*
* OUTPUTS
*
*       NU_SUCCESS                On success.
*       IPSEC_INVALID_ALGO_ID     Invalid algorithm ID in security
*                                 protocol.
*       IKE_INVALID_PARAMS        If parameters are invalid.
*       IKE_INVALID_PROTOCOL      Protocol is not supported.
*       IKE_INVALID_TRANSFORM     Transform not supported.
*       IKE_NOT_NEGOTIABLE        Proposal not negotiable using
*                                 the specified policy.
*       IKE_UNSUPPORTED_ATTRIB    Attribute in SA is not supported.
*       IKE_TRANSFORM_MISMATCH    Transform does not match Policy.
*       IKE2_INVALID_PROPOSAL_NUM The incoming SA does not list proposal
*                                 numbers according to RFC 4306
*
*************************************************************************/
STATUS IKE2_Select_IPsec_Proposal(IKE2_EXCHANGE_HANDLE *handle,
                                  IPSEC_SECURITY_PROTOCOL *security,
                                  UINT8 security_size)
{
    STATUS                  status = NU_SUCCESS;
    UINT8                   i;        /* each index in proposal array */
    UINT8                   j=0;      /* each index in security array */
    UINT8                   k;        /* each index in transform array */
    UINT16                  key_len = 0;
    IKE2_TRANSFORMS_PAYLOAD *in_transform = NU_NULL;
    IKE2_SA_PAYLOAD         *in_sa;
    IKE2_SA_PAYLOAD         *out_sa = NU_NULL;
    UINT8                   is_match = NU_FALSE; /* if the whole proposal matched */
    UINT16                   algo_match_flag;
    UINT8                   prop_no;
    UINT8                   prop_span = 0;
    IKE2_IPS_SA             sa2;
    UINT8                   auth_algo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((handle == NU_NULL) || (security == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure number of proposals is not zero. This is
     * verified in the payload decode function so is within
     * the IKE_DEBUG macro here.
     */
    else if(handle->ike2_params->ike2_in.ike2_sa->ike2_proposals_num == 0)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure number of securities in local policy is not zero. */
    if(security_size == 0)
    {
        /* No securities to match against. */
        status = IKE_NOT_NEGOTIABLE;
    }

    else
    {
        /* Log debug message. */
        IKE2_DEBUG_LOG("Making selection from Initiator's proposal");

        /* Set local pointers to commonly used data in the Handle. */
        in_sa  = handle->ike2_params->ike2_in.ike2_sa;

        /* Set outgoing SA fields only if we are responder, otherwise it is
         * only a verification of peer's selection.
         */
        if(((handle->ike2_flags & IKE2_RESPONDER) != 0) ||
            ((handle->ike2_params->ike2_in.ike2_hdr->ike2_flags
              & IKE2_HDR_RESPONSE_FLAG) == 0))
        {
            out_sa = handle->ike2_params->ike2_out.ike2_sa;

            /* Initialize header fields of the SA payload. */
            out_sa->ike2_gen.ike2_payload_length = IKE2_MIN_SA_PAYLOAD_LEN;
            out_sa->ike2_gen.ike2_next_payload = NU_NULL;
            out_sa->ike2_gen.ike2_payload_type = IKE2_SA_PAYLOAD_ID;
            out_sa->ike2_gen.ike2_critical = 0;
        }
        /* The first proposal's number should always be one as per RFC. */
        if(in_sa->ike2_proposals[0].ike2_proposal_num != 1)
        {
            status = IKE2_INVALID_PROPOSAL_NUM;
        }

        else
        {
            /* Set the prop number to that of the first in the input SA. */
            prop_no = in_sa->ike2_proposals[0].ike2_proposal_num;

            /* Loop for each proposal in the input SA payload. Please note
             * that one proposal may span multiple indexes in the proposals
             * array. Hence loop terminates when the actual proposal count
             * exceeds the same in the input SA or when the index reaches
             * the maximum possible size of the proposals array.
             */
            for(i = 0; (prop_no <= in_sa->ike2_proposals_num) &&
                (i < IKE2_NUM_OF_PROPOSALS); i++)
            {
                /* If next proposal is encountered. This will be skipped
                 * in the first iteration as well as whenever the new
                 * index is still part of the currently examined proposal.
                 * Example of such a case would be an AND of ESP followed
                 * by AH. Here, ike2_proposal_num would still have the same
                 * value for two indexes of proposal array.
                 */
                if(in_sa->ike2_proposals[i].ike2_proposal_num != prop_no)
                {
                    /* Update current proposal number by incrementing one
                     * since each new proposal index should be one greater
                     * than the previous.
                     */
                    prop_no++;

                    /* If previous proposal matched. */
                    if((status == NU_SUCCESS) && (is_match == NU_TRUE))
                    {
                        /* Check if matching proposal is complete. */
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
                        if(IKE_Proposal_Span2(j, security, security_size)
                            == prop_span)
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
                    IKE_Flush_SA2(&handle->ike2_sa2_db);

                    /* Update the status. */
                    status = NU_SUCCESS;
                }

                /* The below step is skipped in the first iteration. */

                /* If current proposal mismatched, then skip all payloads
                 * belonging to the current proposal until the next
                 * proposal, if any, is found.
                 */
                else if(status != NU_SUCCESS)
                {
                    continue;
                }

                /* Make sure protocol ID is valid. */
                if(in_sa->ike2_proposals[i].ike2_protocol_id == 0)
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
                 * NOTE: Order of protocols (ESP/AH) within the proposal is
                 * not specified by the IKE standard. Therefore
                 * match the proposal to the securities regardless
                 * of order, using two nested loops. This practically
                 * accepts proposals spanning multiple indexes irrespective
                 * of their order.
                 */
                for(j = 0; j < security_size; j++)
                {
                    /* Initialize all transform flags to false (not found)
                     * since they are to be matched later.
                     */
                    algo_match_flag = IKE2_NOT_FOUND;

                    /* Check if protocol matches. */
                    if(in_sa->ike2_proposals[i].ike2_protocol_id !=
                        IKE2_Protocol_ID_IPS_To_IKE(security[j].ipsec_protocol))
                    {
                        /* Log debug message. */
                        IKE2_DEBUG_LOG("Protocol mismatch (AH/ESP)");

                        /* Protocol mismatch. Try next security. */
                        status = IKE_INVALID_PROTOCOL;
                        continue;
                    }

                    /* Log debug message. */
                    IKE2_DEBUG_LOG(
                        "Protocol matches - Now matching transforms");

                    /* Loop for each transform in the current proposal of
                     * input SA payload.
                     */
                    for(k = 0;
                        (k < in_sa->ike2_proposals[i].ike2_no_of_transforms);
                        k++)
                    {
                        /* Get pointer to current transform. */
                        in_transform =
                            &in_sa->ike2_proposals[i].ike2_transforms[k];

                        /* Matching the transform type and then it's id */
                        switch(in_transform->ike2_transform_type)
                        {
                            /* Protocol is ESP */
                        case IKE2_TRANS_TYPE_ENCR:
                            /* If algo already matched then skip it. */
                            if((algo_match_flag & IKE2_ENCR_FLAG) ==
                                IKE2_NOT_FOUND)
                            {
                                if(in_transform->ike2_transform_id ==
                                    IKE2_ESP_Trans_ID_IPS_To_IKE(
                                    security[j].ipsec_encryption_algo))
                                {
                                    /* Check if any transform attribute exist and
                                     * compare it. This will happen only when AES is
                                     * used and both sides have specified key length.
                                     */
                                    if(in_transform->ike2_transform_attrib_num > 0)
                                    {
                                        /* Key length for Encryption algo is the only
                                         * possible attribute so we directly check for
                                         * this field only (RFC 4306)Sec 3.3.5 and
                                         * RFC (4718) Sec. 7.11.
                                         */

                                        /* Compute key length first. */
                                        status = IKE2_Compute_Encryption_KeyLen(
                                            security, &key_len);

                                        if(status == NU_SUCCESS)
                                        {
                                            if(key_len == in_transform->
                                                ike2_transform_attrib[0].
                                                ike2_attrib_lenval)
                                            {
                                                algo_match_flag |= IKE2_ENCR_FLAG;
                                            }
                                        }
                                    }

                                    else
                                    {
                                        /* If encryption algo matched and no
                                         * attribute specified.
                                         */
                                        algo_match_flag |= IKE2_ENCR_FLAG;
                                    }
                                }
                            }
                            break;

                            /* Protocol is AH or this is an optional ESP transform. */
                        case IKE2_TRANS_TYPE_INTEG:

                            /* This flag indicates that an optional transform
                             * was specified in the incoming proposal when
                             * protocol was ESP.
                             */
                            algo_match_flag |= IKE2_OPTIONAL_INTEG_FLAG;

                            /* If algo already matched then skip it. */
                            if((algo_match_flag & IKE2_INTEG_FLAG) ==
                                IKE2_NOT_FOUND)
                            {
                                if(in_transform->ike2_transform_id ==
                                    IKE2_AH_Trans_ID_IPS_To_IKE(
                                    security[j].ipsec_auth_algo))
                                {
                                    algo_match_flag |= IKE2_INTEG_FLAG;
                                }
                            }
                            break;

                            /* Present in both ESP and AH. */
                        case IKE2_TRANS_TYPE_ESN:
                            /* If ESN already matched then skip it. */
                            if((algo_match_flag & IKE2_ESN_FLAG) ==
                                IKE2_NOT_FOUND)
                            {
                                /* If it is enabled in local policy as well
                                 * as in incoming proposal, then matched.
                                 */
                                if(security[j].ipsec_flags & IPSEC_ENABLE_ESN)
                                {
                                    if(in_transform->ike2_transform_id ==
                                    IKE2_ENABLED_ESN_VALUE)
                                    {
                                        algo_match_flag |= IKE2_ESN_FLAG;
                                    }
                                }

                                /* Otherwise, if disabled in both incoming
                                 * proposal and local policy then, it
                                 * should be matched.
                                 */
                                else
                                {
                                    if(in_transform->ike2_transform_id ==
                                    IKE2_NO_ESN_VALUE)
                                    {
                                        algo_match_flag |= IKE2_ESN_FLAG;
                                    }
                                }
                            }
                            break;

                        default:
                            /* Incoming transform does not exist in the
                             * current security index. However, proposal
                             * can still match.
                             */
                            break;
                        }
                    } /* End each transform loop */

                    /* If all the algorithms from security index matched
                     * with the current proposal index, then our proposal
                     * has matched as yet. However, whether proposal ends
                     * or not is yet to be checked.
                     */
                    if((algo_match_flag & IKE2_ESN_FLAG) != IKE2_NOT_FOUND)
                    {
                        if(IKE2_Protocol_ID_IPS_To_IKE(
                            security[j].ipsec_protocol) ==
                            IKE2_PROTO_ID_AH)
                        {
                            if((algo_match_flag & IKE2_INTEG_FLAG) !=
                                IKE2_NOT_FOUND)
                                is_match = NU_TRUE;
                        }

                        else
                        {
                            /* Protocol is ESP so we have to check
                             * encryption algo and the optional Integrity
                             * transform as well.
                             */
                            if((algo_match_flag & IKE2_ENCR_FLAG) !=
                                IKE2_NOT_FOUND)
                            {
                                /* If an optional integrity algorithm was
                                 * not specified then our proposal matched.
                                 */
                                if((algo_match_flag & IKE2_OPTIONAL_INTEG_FLAG)
                                    == IKE2_NOT_FOUND)
                                    is_match = NU_TRUE;

                                /* Optional integrity algorithm was
                                 * specified, then it must match.
                                 */
                                else if((algo_match_flag & IKE2_INTEG_FLAG)
                                    != IKE2_NOT_FOUND)
                                        is_match = NU_TRUE;
                            }
                        }
                    }

                    /* If match found then break, otherwise continue looking */
                    if(is_match == NU_TRUE)
                        break;

                } /* End loop for each security */

                if(is_match == NU_FALSE)
                {
                    /* Log debug message. */
                    IKE2_DEBUG_LOG(
                        "Current proposal not suitable - trying next");
                }

                else
                {
                    if(((handle->ike2_flags & IKE2_RESPONDER) != 0) ||
                        ((handle->ike2_params->ike2_in.ike2_hdr->ike2_flags
                          & IKE2_HDR_RESPONSE_FLAG) == 0))
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

                        /* Generate the SPI for remote node and store it in
                         * the SA2.
                         */
                        sa2.ike_remote_spi =
                            GET32(in_sa->ike2_proposals[i].ike2_spi, 0);

                        /* Also store the SPI for local node in SA2 after
                         * fetching it from the incoming SA's selected proposal.
                         * SPI will always be four bytes or IPsec (ESP/AH).
                         */

                        sa2.ike_local_spi = sa2.ike_remote_spi;

                        /* Add the SA2 item to the Handle. */
                        status = IKE_Add_SA2(&handle->ike2_sa2_db, &sa2);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to add SA2 item to database",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                            break;
                        }

                        /* Copy the current proposal array index which matched.
                         * This may or may not be a complete proposal. We are
                         * using the prop_span which handles the current index
                         * of proposal in the out_sa.
                         */
                        if(out_sa != NU_NULL)
                        {
                            /* Because only one proposal is selected after negotiation */
                            out_sa->ike2_proposals_num = 1;

                            /* The matched proposal is on the current index i.e. i,
                             * so copy it's proposal number.
                             */
                            if (prop_span < IKE2_NUM_OF_PROPOSALS)
                            {
                                out_sa->ike2_proposals[prop_span].ike2_proposal_num =
                                in_sa->ike2_proposals[i].ike2_proposal_num;

                                out_sa->ike2_proposals[prop_span].ike2_more_proposals =
                                IKE2_PRPSL_LAST_VALUE;

                                /* If the current index in proposal array is not the
                                 * only one, then the last index should indicate this.
                                 */
                                if(prop_span > 0)
                                {
                                    out_sa->ike2_proposals[prop_span-1].
                                        ike2_more_proposals = IKE2_PRPSL_MORE_VALUE;
                                }

                                /* Since transforms will be added later. */
                                out_sa->ike2_proposals[prop_span].ike2_no_of_transforms = 0;

                                out_sa->ike2_proposals[prop_span].ike2_protocol_id =
                                    in_sa->ike2_proposals[i].ike2_protocol_id;

                                out_sa->ike2_proposals[prop_span].ike2_spi_size =
                                    in_sa->ike2_proposals[i].ike2_spi_size;

                                NU_BLOCK_COPY(out_sa->ike2_proposals[prop_span].ike2_spi,
                                    in_sa->ike2_proposals[i].ike2_spi,
                                    out_sa->ike2_proposals[prop_span].ike2_spi_size);

                                out_sa->ike2_proposals[prop_span].ike2_proposal_len =
                                    IKE2_MIN_PRPSL_PAYLOAD_LEN +
                                    out_sa->ike2_proposals[prop_span].ike2_spi_size;

                                /* Copy transforms now. */
                                if(out_sa->ike2_proposals[prop_span].ike2_protocol_id ==
                                    IKE2_PROTO_ID_AH)
                                {
                                    /* Protocol is AH. */
                                    IKE2_Add_Transform(&out_sa->ike2_proposals[prop_span],
                                        IKE2_TRANS_TYPE_INTEG,
                                        IKE2_AH_Trans_ID_IPS_To_IKE(security[j].
                                        ipsec_auth_algo), IKE2_TRANS_MORE_VALUE);
                                }

                                else  /* Protocol is ESP. */
                                {
                                    IKE2_Add_Transform(&out_sa->
                                        ike2_proposals[prop_span], IKE2_TRANS_TYPE_ENCR,
                                        IKE2_ESP_Trans_ID_IPS_To_IKE(security[j].
                                        ipsec_encryption_algo), IKE2_TRANS_MORE_VALUE);

                                    /* If a valid key length attribute was matched
                                     * then it's value must be present here.
                                     */
                                    if(key_len != 0)
                                    {
                                        IKE2_Add_Transform_Attrib(&(out_sa->
                                            ike2_proposals[prop_span].ike2_transforms[0]),
                                            IKE2_KEY_LEN_ID, key_len, NU_NULL);

                                        /* After adding the attribute we have to update the
                                         * proposal length as well by adding only the new
                                         * attribute's length to proposal.The Key Length
                                         * is a AF-TV(Attribute Format Type/Value) so
                                         * does not have variable length
                                         */
                                        out_sa->ike2_proposals[prop_span].
                                        ike2_proposal_len += IKE2_MIN_TRANS_ATTRIB_LEN;
                                    }

                                    /* Verify if a valid Integrity Algorithm is present */
                                    auth_algo = IKE2_AH_Trans_ID_IPS_To_IKE
                                        (security[j].ipsec_auth_algo);

                                    if(auth_algo != 0)
                                    {
                                        /* Then add the optional Integrity transform as well. */
                                        IKE2_Add_Transform(&out_sa->ike2_proposals[prop_span],
                                            IKE2_TRANS_TYPE_INTEG, auth_algo, IKE2_TRANS_MORE_VALUE);
                                    }
                                }

                                /* If ESN is enabled/disabled add its respective
                                 * transform to the Proposal.
                                 */
                                if(security[j].ipsec_flags & IPSEC_ENABLE_ESN)
                                {
                                    IKE2_Add_Transform(&out_sa->ike2_proposals[prop_span],
                                        IKE2_TRANS_TYPE_ESN,
                                        IKE2_ENABLED_ESN_VALUE, IKE2_TRANS_LAST_VALUE);

                                    /* Set the corresponding flag in the exchange handle. */
                                    handle->ike2_sa->ike2_flags |= IKE2_USE_IPS_ESN;
                                }

                                else
                                {
                                    IKE2_Add_Transform(&out_sa->ike2_proposals[prop_span],
                                        IKE2_TRANS_TYPE_ESN,
                                        IKE2_NO_ESN_VALUE, IKE2_TRANS_LAST_VALUE);
                                }

                                /* Now update the payload length in generic header. */
                                out_sa->ike2_gen.ike2_payload_length =
                                    out_sa->ike2_gen.ike2_payload_length +
                                    out_sa->ike2_proposals[prop_span].ike2_proposal_len;

                            }
                            else
                            {
                                /* It is highly unlikely that this would happen */
                                NLOG_Error_Log("Proposal index out of range",
                                                NERR_SEVERE, __FILE__, __LINE__);
                                break;
                            }

                        }
                        else
                        {
                            status = IKE_INVALID_PARAMS;
                            break;
                        }

                    }

                    else
                    {
                        /* Generate the SPI for remote node and store it in
                         * the SA2.
                         */
                        handle->ike2_sa2_db.ike_flink->ike_remote_spi =
                            GET32(in_sa->ike2_proposals[i].ike2_spi, 0);
                    }

                    /* Increment proposal payloads span. */
                    prop_span++;

                    /* If this is the last proposal of the list. */
                    if(i == in_sa->ike2_proposals_num - 1)
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

                /* Continue traversing through the proposal list whether
                 * we have found a match yet or not, because even if we
                 * matched we are not sure if the proposal is completed
                 */
            } /* End loop for each proposal */
        }

        /* If a matching proposal was found. */
        if((status == NU_SUCCESS) && (is_match == NU_TRUE) &&
            ((handle->ike2_flags & IKE2_RESPONDER) != 0))
        {
            /* Log debug message. */
            IKE2_DEBUG_LOG("Suitable proposal selected");


            /* Add SA payload to the payloads chain. */
            IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                out_sa, IKE2_SA_PAYLOAD_ID);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE2_Select_IPsec_Proposal */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Cleanup_Exchange
*
* DESCRIPTION
*
*       This function is responsible for cleaning up any resources being
*       held by an exchange handle before the deletion of the handle. It
*       also deletes the handle itself.
*
* INPUTS
*
*       *handle                 Exchange handle to be cleaned up.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Handle passed was not valid.
*       IKE_GEN_ERROR           Failed to properly deallocated memory for
*                               at least one field of handle.
*
*************************************************************************/
STATUS IKE2_Cleanup_Exchange(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS status = NU_SUCCESS;

    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }

    /* First remove handle references any SA databases. */
    if(SLL_Remove(&handle->ike2_sa->xchg_db, handle) == NU_NULL)
    {
        status = IKE_GEN_ERROR;
        NLOG_Error_Log("Exchange handle not in the SA's database",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(handle->ike2_decrypt_iv != NU_NULL)
    {
        if(NU_Deallocate_Memory(handle->ike2_decrypt_iv) != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate decryption IV memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_decrypt_iv = NU_NULL;
    }

    if(handle->ike2_encrypt_iv != NU_NULL)
    {
        if(NU_Deallocate_Memory(handle->ike2_encrypt_iv) != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate encryption IV memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_encrypt_iv = NU_NULL;
    }

    if(handle->ike2_last_message != NU_NULL)
    {
        if(IKE_Deallocate_Buffer(handle->ike2_last_message) != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate memory for last message",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_last_message = NU_NULL;
    }

    if(handle->ike2_nonce != NU_NULL)
    {
        if(NU_Deallocate_Memory(handle->ike2_nonce) != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate memory for nonce",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_nonce = NU_NULL;
    }

    if(handle->ike2_peer_nonce != NU_NULL)
    {
        if(NU_Deallocate_Memory(handle->ike2_peer_nonce) != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate memory for peer's nonce",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_peer_nonce = NU_NULL;
    }

    if(handle->ike2_remote_dh != NU_NULL)
    {
        if(NU_Deallocate_Memory(handle->ike2_remote_dh) != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate memory for DH public value",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_remote_dh = NU_NULL;
    }

    if(handle->ike2_dh_keys.ike_public_key != NU_NULL)
    {
        if(NU_Deallocate_Memory(handle->ike2_dh_keys.ike_public_key)
                != NU_SUCCESS)
        {
            status = IKE_GEN_ERROR;
            NLOG_Error_Log("Failed to deallocate memory for DH keys",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        handle->ike2_dh_keys.ike_public_key = NU_NULL;
    }

    IKE_Flush_SA2(&handle->ike2_sa2_db);

    if(NU_Deallocate_Memory(handle) != NU_SUCCESS)
    {
        status = IKE_GEN_ERROR;
        NLOG_Error_Log("Failed to deallocate handle", NERR_RECOVERABLE,
            __FILE__, __LINE__);
    }

    return (status);
} /* IKE2_Cleanup_Exchange */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_KE_Data
*
* DESCRIPTION
*
*       Generates the DH public value for current exchange.
*
* INPUTS
*
*       *handle                 Exchange information.
*       *ke                     Key exchange data.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            If no memory available for allocation.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE2_Generate_KE_Data(IKE2_EXCHANGE_HANDLE *handle,
                             IKE2_KE_PAYLOAD *ke)
{
    STATUS              status;
    IKE_KEY_PAIR        dh_keys;
    UINT8               *g_ext;
    INT                 g_ext_len;
    IKE_OAKLEY_GROUP_INFO *oakinfo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(handle == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Generating Key Exchange data");

    /* Get the Oakley group being used. */
    oakinfo = IKE_Get_Oakley_Group(
                    handle->ike2_sa->ike_attributes.ike_group_desc);

    if(oakinfo == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }

    if(oakinfo->ike_generator != IKE_DH_GENERATOR_POS)
    {
        g_ext = NU_NULL;
        g_ext_len = 0;
    }
    else
    {
        g_ext = oakinfo->ike_generator_ext;
        g_ext_len = oakinfo->ike_generator_ext_len;
    }

    /* Make the request for Key Pair generation. */
    status = IKE_DH_Generate_Key(oakinfo->ike_prime,
                                 oakinfo->ike_prime_len,
                                 oakinfo->ike_generator,
                                 &dh_keys, IKE_DH_PRIVATE_KEY_SIZE,
                                 g_ext, g_ext_len);

    if(status == NU_SUCCESS)
    {
        /* Store the Diffie-Hellman Kay Pair. This must
         * be deallocated later.
         */
        handle->ike2_dh_keys.ike_public_key      = dh_keys.ike_public_key;
        handle->ike2_dh_keys.ike_public_key_len  = dh_keys.ike_public_key_len;
        handle->ike2_dh_keys.ike_private_key     = dh_keys.ike_private_key;
        handle->ike2_dh_keys.ike_private_key_len = dh_keys.ike_private_key_len;

        /* Set Key Exchange data in the payload. */
        ke->ike2_ke_data     = dh_keys.ike_public_key;
        ke->ike2_ke_data_len = dh_keys.ike_public_key_len;
        ke->ike2_dh_group_no = handle->ike2_sa->ike_attributes.ike_group_desc;

        /* Set the length of payload. */
        ke->ike2_gen.ike2_payload_type = IKE2_KE_PAYLOAD_ID;
        ke->ike2_gen.ike2_payload_length = IKE2_MIN_KE_PAYLOAD_LEN +
                                           ke->ike2_ke_data_len;
        ke->ike2_gen.ike2_critical = 0;

        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                          ke, IKE2_KE_PAYLOAD_ID);
    }

    else
    {
        NLOG_Error_Log("Failed to generate Diffie-Hellman key pair",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Generate_KE_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_Nonce_Data
*
* DESCRIPTION
*
*       Generates the nonce data.
*
* INPUTS
*
*       *handle                 Exchange information.
*       *nonce                  Nonce payload to be populated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE2_Generate_Nonce_Data(IKE2_EXCHANGE_HANDLE *handle,
                                IKE2_NONCE_PAYLOAD *nonce)
{
    STATUS              status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((handle == NU_NULL) || (nonce == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Generating Nonce data");

    if(nonce->ike2_nonce_data_len == 0)
    {
        nonce->ike2_nonce_data_len = IKE2_NONCE_MIN_LEN;
    }

    if(handle->ike2_nonce == NU_NULL)
    {
        /* Allocate memory for storing the Nonce data. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&handle->ike2_nonce,
                                    nonce->ike2_nonce_data_len,
                                    NU_NO_SUSPEND);
    }

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        handle->ike2_nonce = TLS_Normalize_Ptr(handle->ike2_nonce);

        /* Generate the random Nonce data. */
        status = RAND_bytes(handle->ike2_nonce, nonce->ike2_nonce_data_len)
                 ? NU_SUCCESS : IKE_CRYPTO_ERROR;

        if(status == NU_SUCCESS)
        {
            /* Set Key Exchange data in the payload. */

            nonce->ike2_nonce_data = handle->ike2_nonce;
            handle->ike2_nonce_len = nonce->ike2_nonce_data_len;

            /* Update the length and payload type fields in Gen Header. */
            nonce->ike2_gen.ike2_payload_type = IKE2_NONCE_PAYLOAD_ID;
            nonce->ike2_gen.ike2_payload_length = IKE2_MIN_NONCE_PAYLOAD_LEN +
                                                  nonce->ike2_nonce_data_len;
            nonce->ike2_gen.ike2_critical = 0;

            IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                              nonce, IKE2_NONCE_PAYLOAD_ID);
        }

        else
        {
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

} /* IKE2_Generate_Nonce_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_ID_Data
*
* DESCRIPTION
*
*       Generates identification data for outgoing message during exchange.
*
* INPUTS
*
*       *handle                 Exchange information.
*       *id                     ID payload to be filled.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters supplied are invalid.
*       IKE_UNSUPPORTED_IDTYPE  ID type specified is not valid.
*       IKE_INVALID_DOMAIN      A valid domain has not been specified.
*
*************************************************************************/
STATUS IKE2_Generate_ID_Data(IKE2_EXCHANGE_HANDLE *handle,
                             IKE2_ID_PAYLOAD *id)
{
    STATUS              status = NU_SUCCESS;
    IKE2_POLICY         *policy;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    UINT8               *cert_data;
    UINT8               *subject_name;
    UINT32              subject_len;
    UINT16              cert_len;
#endif

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((handle == NU_NULL) || (id == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Generating ID data");

    /* Set local pointers. */
    policy = handle->ike2_params->ike2_policy;

    /* Set the ID type. */
    id->ike2_id_type = policy->ike_my_id.ike_type;

    switch(id->ike2_id_type)
    {
    case IKE_WILDCARD:
        /* The wildcard case is no longer supported by Nucleus IKEv2
         * so this code can later be removed. */
        id->ike2_id_type = IKE_FAMILY_TO_FLAGS(handle->ike2_params->
                            ike2_packet->ike_local_addr.family);

        /* Set data length by ID type. */
        id->ike2_id_data_len = IKE_IP_LEN_BY_FLAGS(id->ike2_id_type);

        /* Copy local IP address to identification data. */
        NU_BLOCK_COPY(id->ike2_id_data, handle->ike2_params->
                        ike2_packet->ike_local_addr.id.is_ip_addrs,
                        id->ike2_id_data_len);
        break;

    case IKE2_ID_TYPE_IPV4_ADDR:
        /* Set length of ID data. */
        id->ike2_id_data_len = IP_ADDR_LEN;
        /* Copy IP address to the identification data field. */
        NU_BLOCK_COPY(id->ike2_id_data,
            policy->ike_my_id.ike_addr.ike_ip.ike_addr1,
            id->ike2_id_data_len);
        break;

    case IKE2_ID_TYPE_IPV6_ADDR:
        /* Set length of ID data. */
        /* Not using IPv6 address length macro as it may be disabled
         * at compile-time. */
        id->ike2_id_data_len = 16 /*IP6_ADDR_LEN*/;
        /* Copy IP address to the identification data field. */
        NU_BLOCK_COPY(id->ike2_id_data,
            policy->ike_my_id.ike_addr.ike_ip.ike_addr1,
            id->ike2_id_data_len);
        break;

    case IKE2_ID_TYPE_FQDN:
    case IKE2_ID_TYPE_RFC822_ADDR:
        /* Make sure the domain name is specified. */
        if(policy->ike_my_id.ike_addr.ike_domain == NU_NULL)
        {
            status = IKE_INVALID_DOMAIN;
        }

        else
        {
            /* Set length of identification data. */
            id->ike2_id_data_len = (UINT16)
                strlen(policy->ike_my_id.ike_addr.ike_domain);

            /* Make sure domain name is not of zero length. */
            if(id->ike2_id_data_len == 0 ||
                    id->ike2_id_data_len > IKE_MAX_ID_DATA_LEN)
            {
                status = IKE_INVALID_DOMAIN;
            }

            else
            {
                /* Copy domain name to identification data field. */
                NU_BLOCK_COPY(id->ike2_id_data,
                              policy->ike_my_id.ike_addr.ike_domain,
                              id->ike2_id_data_len);
            }
        }
        break;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    case IKE2_ID_TYPE_DER_ASN1_DN:

        /* When this type of ID data is requested to be sent in ID payload,
         * we need to encode the SubjectName from the certificate in the
         * ID payload we send.
         */

        /* Read the certificate data from file. */
        status = IKE_Cert_Get(
                    handle->ike2_sa->ike_attributes.ike_local_cert_file,
                    &cert_data, &cert_len,
                    handle->ike2_sa->ike_attributes.ike_cert_encoding);

        if(status == NU_SUCCESS)
        {
            /* Certificate read, now get the SubjectName. */
            status = IKE_Cert_Get_ASN1_SubjectName(cert_data, cert_len,
                                                   &subject_name,
                                                   &subject_len);

            if(status == NU_SUCCESS)
            {
                /* Set the length of ID data. */
                id->ike2_id_data_len = (UINT16)subject_len;

                if(id->ike2_id_data_len <= IKE_MAX_ID_DATA_LEN)
                {
                    /* Copy SubjectName to identification data field. */
                    NU_BLOCK_COPY(id->ike2_id_data, subject_name,
                                  id->ike2_id_data_len);
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
#endif

    default:
        /* Identification type not supported. */
        status = IKE_UNSUPPORTED_IDTYPE;
        break;
    }

    if(status == NU_SUCCESS)
    {
        id->ike2_gen.ike2_critical = 0;
        id->ike2_gen.ike2_payload_length = IKE2_MIN_ID_PAYLOAD_LEN +
                                            id->ike2_id_data_len;

        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            id->ike2_gen.ike2_payload_type = IKE2_ID_I_PAYLOAD_ID;
        }

        else
        {
            id->ike2_gen.ike2_payload_type = IKE2_ID_R_PAYLOAD_ID;
        }

        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
            id, id->ike2_gen.ike2_payload_type);

    }

    return (status);

} /* IKE2_Generate_ID_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_AUTH_Data
*
* DESCRIPTION
*
*       Generates authentication data for outgoing message during exchange.
*
* INPUTS
*
*       *handle                 Handle to the current exchange.
*       *auth                   Authentication payload to be sent.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_INVALID_PARAMS       Parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE2_Generate_AUTH_Data(IKE2_EXCHANGE_HANDLE *handle,
                               IKE2_AUTH_PAYLOAD *auth)
{
    STATUS              status = NU_SUCCESS;
    UINT8               algo_id;
    UINT8               *digest;
    UINT8               digest_len;
    UINT8               *text;
    INT                 text_len;
    UINT8               *key;
    INT                 key_len;
    IKE2_ID_PAYLOAD     *id;
    UINT8               *auth_buffer = NU_NULL;
    UINT16              auth_buffer_len;
    UINT8               *id_prf = NU_NULL;
    UINT16              id_prf_len;
    UINT8               *auth_key = NU_NULL;
    UINT8               auth_key_len = 0;
    UINT8               *temp_ptr = NU_NULL;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    EVP_MD_CTX          hash_ctx;
    UINT8               hash_len = 0;
    UINT16              sign_algo;
    UINT8               *sign = NU_NULL;
    UINT                sign_len = 0;
#endif

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    else if(((handle->ike2_flags & IKE2_INITIATOR) == NU_TRUE) &&
            (handle->ike2_params->ike2_in.ike2_id_r == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    else if(((handle->ike2_flags & IKE2_RESPONDER) == NU_TRUE) &&
        (handle->ike2_params->ike2_in.ike2_id_i == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Select which ID payload we need to use. We have to use our own ID.
     * Check if we are initiator or the responder.
     */
    if((handle->ike2_flags & IKE2_INITIATOR) != 0)
    {
        id = handle->ike2_params->ike2_out.ike2_id_i;
    }
    else
    {
        id = handle->ike2_params->ike2_out.ike2_id_r;
    }

    /* Authentication data will be calculated with PSK as:
     * AUTH = prf(prf(PSK, PSK_PAD), (Message|Nonce|prf(SK_p,ID)))
     *
     * or with digital signatures as:
     * AUTH = SHA1(PrivateKey, (Message|Nonce|prf(SK_p,ID)))
     */

    id_prf_len = IKE2_PRF_HMAC_Algos[handle->ike2_sa->ike_attributes.
                    ike2_prf_algo].ike2_output_len;

    /* Length of the buffer to hold messages octets appended with nonce
     * PRFed ID payload.
     */
    auth_buffer_len = handle->ike2_sa->ike2_local_auth_len +
                      handle->ike2_peer_nonce_len + id_prf_len;

    auth->ike2_auth_method = (UINT8)handle->ike2_sa->ike_attributes.
                                ike_auth_method;

    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&id_prf,
                                id_prf_len + id->ike2_id_data_len,
                                NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        memset(id_prf, 0, id_prf_len);

        /* Encode ID data into allocated buffer. */
        PUT8(id_prf, 0, id->ike2_id_type);
        NU_BLOCK_COPY(id_prf + 4, id->ike2_id_data, id->ike2_id_data_len);

        /* Calculate PRF(SK_p, ID) */
        digest = id_prf;
        digest_len = IKE2_PRF_HMAC_Algos[handle->ike2_sa->
                                    ike_attributes.ike2_prf_algo].ike2_output_len;
        algo_id = IKE2_PRF_Algos[handle->ike2_sa->ike_attributes.
                                    ike2_prf_algo].crypto_algo_id;

        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            key = handle->ike2_sa->ike2_sk_pi;
        }

        else
        {
            key = handle->ike2_sa->ike2_sk_pr;
        }

        key_len = IKE2_PRF_HMAC_Algos[handle->ike2_sa->
                                ike_attributes.ike2_prf_algo].ike2_key_len;
        text = id_prf;

        /* Data over which this hash is being calculated is the ID value
         * pre-pended with a 4 octet value (ID type).
         */
        text_len = id->ike2_id_data_len + 4;

        status = IKE_HMAC(algo_id, key, key_len,
                          text, text_len, digest, &digest_len);

        if(status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&auth_buffer,
                                        auth_buffer_len, NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                temp_ptr = auth_buffer;

                /* Copy in the saved message over which to calculate PRF */
                NU_BLOCK_COPY(temp_ptr,
                              handle->ike2_sa->ike2_local_auth_data,
                              handle->ike2_sa->ike2_local_auth_len);

                temp_ptr = temp_ptr + handle->ike2_sa->ike2_local_auth_len;

                /* Copy in peer's nonce value. */
                NU_BLOCK_COPY(temp_ptr, handle->ike2_peer_nonce,
                              handle->ike2_peer_nonce_len);

                temp_ptr = temp_ptr + handle->ike2_peer_nonce_len;

                /* Copy in the PRF(SK_p, ID) */
                NU_BLOCK_COPY(temp_ptr, id_prf, digest_len);
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for \
                               authentication data buffer", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to calculate PRF(SK_p, ID)",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for prf(ID) buffer",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Processing common to both authentication methods has been completed.
     * Now calculate authentication data according to method selected.
     */
    if(status == NU_SUCCESS)
    {
        switch(auth->ike2_auth_method)
        {
        case IKE2_AUTH_METHOD_SKEY_MIC:
            /* No need to explicitly lookup pre-shared key as it is
             * part of the SA now. */

            if(status == NU_SUCCESS)
            {
                /* Calculate the key length of the PRF algorithm. */

                if(IKE2_PRF_Algos[handle->ike2_sa->ike_attributes.
                    ike2_prf_algo].ike2_algo_identifier ==
                    IKE2_PRF_AES128_XCBC)
                {
                    auth_key_len = IKE2_PRF_HMAC_Algos[
                        handle->ike2_sa->ike_attributes.ike2_prf_algo].
                        ike2_output_len;
                }

                else
                {
                    IKE_Crypto_Digest_Len(IKE_Hash_Algos[handle->ike2_sa->ike_attributes.ike2_prf_algo].crypto_algo_id,
                                          &auth_key_len);
                }
            }

            if(status == NU_SUCCESS)
            {
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                            (VOID**)&auth_key, auth_key_len,
                                            NU_NO_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    /* Calculate the key for calculating the AUTH data. */
                    digest = auth_key;
                    digest_len = 0;
                    algo_id = IKE2_PRF_Algos[handle->ike2_sa->ike_attributes.
                                                ike2_prf_algo].crypto_algo_id;
                    key = handle->ike2_sa->ike_attributes.ike2_psk_local;
                    key_len = handle->ike2_sa->ike_attributes.ike2_psk_local_len;
                    text = (UINT8*)IKE2_PSK_PAD_STRING;
                    text_len = IKE2_PSK_PAD_LENGTH;

                    /* Calculate PRF(PSK, "Key Pad for IKEv2") */
                    status = IKE_HMAC(algo_id, key, key_len,
                                      text, text_len, digest, &digest_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Calculate AUTH data. */
                        digest = auth_buffer;
                        digest_len = 0;
                        algo_id = IKE2_PRF_Algos[handle->ike2_sa->ike_attributes.
                                                    ike2_prf_algo].crypto_algo_id;
                        key = auth_key;
                        key_len = auth_key_len;
                        text = auth_buffer;
                        text_len = auth_buffer_len;

                        status = IKE_HMAC(algo_id, key, key_len,
                                          text, text_len, digest, &digest_len);

                        if(status == NU_SUCCESS)
                        {
                            auth_buffer_len = digest_len;
                        }
                        else
                        {
                            NLOG_Error_Log("Failed to calculate AUTH data",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to calculate auth key",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to allocate memory for AUTH key",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Pre-shared key not found for this peer",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            break;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        case IKE2_AUTH_METHOD_RSA_DS:
            status = IKE_Hash_Init(IKE_OPENSSL_SHA1, &hash_ctx);

            if(status == NU_SUCCESS)
            {
                status = IKE_Hash_Update(&hash_ctx, auth_buffer, auth_buffer_len);

                if(status == NU_SUCCESS)
                {
                    status = IKE_Hash_Final(&hash_ctx, auth_buffer, &hash_len);

                    if(status == NU_SUCCESS)
                    {
                        status = IKE2_SIGN_ALGO_INDEX(
                                        handle->ike2_sa->ike_attributes.
                                        ike_auth_method, sign_algo);

                        if(status == NU_SUCCESS)
                        {
                            status = IKE_Compute_Signature(IKE_Sign_Algos[sign_algo].crypto_algo_id,
                                                           (&handle->ike2_sa->ike_attributes),
                                                           auth_buffer, hash_len, NID_sha1,
                                                           &sign, &sign_len);

                            if(status == NU_SUCCESS)
                            {
                                auth_buffer = sign;
                                auth_buffer_len = sign_len;
                            }

                            else
                            {
                                NLOG_Error_Log("Failed to compute digital signature",
                                               NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("No signature algorithm found",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to calculate hash",
                                       NERR_RECOVERABLE, __FILE__,
                                       __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update hash context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to initialize hash context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            break;
#endif
        default:
            /* Selected method is not supported. */
            status = IKE_UNSUPPORTED_METHOD;
            break;
        }
    }

    /* AUTH data calculated. Now copy it in the payload and add the
     * payload to the chain.
     */
    if(status == NU_SUCCESS)
    {
        auth->ike2_auth_data = auth_buffer;
        auth->ike2_auth_data_len = auth_buffer_len;

        auth->ike2_gen.ike2_critical = 0;
        auth->ike2_gen.ike2_payload_type = IKE2_AUTH_PAYLOAD_ID;
        auth->ike2_gen.ike2_payload_length = IKE2_MIN_AUTH_PAYLOAD_LEN +
                                             auth_buffer_len;

        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                          auth, IKE2_AUTH_PAYLOAD_ID);
    }

    /* All done. Now deallocate any memory that was allocated in the
     * process
     */
    if((id_prf != NU_NULL) && (NU_Deallocate_Memory(id_prf) != NU_SUCCESS))
    {
        NLOG_Error_Log("Failed to deallocate memory for id_prf buffer",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if((auth_key != NU_NULL) && (NU_Deallocate_Memory(auth_key) != NU_SUCCESS))
    {
        NLOG_Error_Log("Failed to deallocate memory for auth_key buffer",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Generate_AUTH_Data */

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_Cert_Data
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
*       *handle                 Pointer to the Exchange Handle.
*       *cert_payload           Pointer to input Certification payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Generate_Cert_Data(IKE2_EXCHANGE_HANDLE *handle,
                               IKE2_CERT_PAYLOAD *cert_payload)
{
    STATUS                  status;
    UINT8                   *cert_data;
    UINT16                  cert_len;

#if (IKE2_DEBUG == NU_TRUE)
    if((handle == NU_NULL) || (cert_payload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message */
    IKE2_DEBUG_LOG("Generating Certificate data.");

    /* Read the certificate data from file. */
    status = IKE_Cert_Get(handle->ike2_sa->ike_attributes.ike_local_cert_file,
                          &cert_data, &cert_len,
                          handle->ike2_sa->ike_attributes.ike_cert_encoding);

    /* If the certificate was read successfully. */
    if(status == NU_SUCCESS)
    {
        /* Populate the Cert encoding payload structure. */
        cert_payload->ike2_cert_data = cert_data;
        cert_payload->ike2_cert_data_len = cert_len;
        cert_payload->ike2_cert_encoding = IKE2_CERT_X509_SIGNATURE;

        /* Update the length and payload type fields in Gen Header. */
        cert_payload->ike2_gen.ike2_payload_type = IKE2_NONCE_PAYLOAD_ID;
        cert_payload->ike2_gen.ike2_payload_length =
            IKE2_MIN_CERT_PAYLOAD_LEN + cert_payload->ike2_cert_data_len;
        cert_payload->ike2_gen.ike2_critical = 0;

        /* Add the payload to the chain of outgoing payloads. */
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last, cert_payload,
                          IKE2_CERT_PAYLOAD_ID);
    }

    /* Certificate could not be read. */
    else
    {
        NLOG_Error_Log("Failed to generate CERT payload data",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Generate_Cert_Data */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_CertReq_Data
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
*       - IKE2_CERTREQ_ENC_PAYLOAD
*
* INPUTS
*
*       *handle                 Pointer to the Exchange Handle.
*       *cert_req               Pointer to the Certificate Request payload
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
**************************************************************************/
STATUS IKE2_Generate_CertReq_Data(IKE2_EXCHANGE_HANDLE *handle,
                                  IKE2_CERT_REQ_PAYLOAD *cert_req)
{
    STATUS                      status = NU_SUCCESS;
    IKE_POLICY                  *policy;
    UINT8                       *cert_data;
    UINT16                      cert_len;
    UINT8                       *spki;
    UINT32                      spki_len;
    UINT8                       digest[SHA_DIGEST_LENGTH];
    UINT8                       digest_len = SHA_DIGEST_LENGTH;

#if (IKE2_DEBUG == NU_TRUE)
    if((handle == NU_NULL) || (cert_req == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Log debug message */
    IKE2_DEBUG_LOG("Generating Certificate Request data");

    /* Set local pointers to data in the Handle */
    policy   = handle->ike2_params->ike2_policy;

    /* Set the desired encoding of the certificate that we want to accept.
    * Currently only one encoding is supported.
    */
    cert_req->ike2_cert_encoding = IKE2_CERT_X509_SIGNATURE;

    /* See if CA's DN needs to be specified in CERT_REQ. */
    if((policy->ike_flags & IKE_CA_IN_CERTREQ) != 0)
    {
        /* Read the certificate data from file. */
        status = IKE_Cert_Get(
            handle->ike2_sa->ike_attributes.ike_ca_cert_file,
            &cert_data, &cert_len,
            handle->ike2_sa->ike_attributes.ike_cert_encoding);

        /* Certificate was read successfully. */
        if(status == NU_SUCCESS)
        {
            /* Calculate the Certification Authority value from certificate. */
            /* RFC4306 - 3.7. Certificate Request Payload:
             * The Certification Authority value is a concatenated list of SHA-1 hashes
             * of the public keys of trusted Certification Authorities (CAs).  Each
             * is encoded as the SHA-1 hash of the Subject Public Key Info element
             * (see section 4.1.2.7 of [RFC3280]) from each Trust Anchor
             * certificate.
             */
            status = IKE_Cert_Get_PKCS1_SPKI(cert_data, cert_len,
                                                   &spki, &spki_len);

            if(status == NU_SUCCESS)
            {
                /* Calculate SHA-1 hash of SPKI */
                if (NU_NULL == SHA1(spki, spki_len, digest))
                {
                    status = IKE2_INVALID_PARAMS;
                }
            }

            /* SPKI read successfully. */
            if(status == NU_SUCCESS)
            {
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                            (VOID**)&cert_req->ike2_ca,
                                            digest_len, NU_NO_SUSPEND);
                if(status == NU_SUCCESS)
                {
                    /* Copy the hash in the CERTREQ structure. */
                    NU_BLOCK_COPY((VOID*)cert_req->ike2_ca, (VOID*)digest,
                            digest_len);

                    /* Set the length of hash copied above. */
                    cert_req->ike2_ca_len = (UINT16)digest_len;

                    if((NU_Deallocate_Memory((VOID*)cert_data) != NU_SUCCESS
                        || (NU_Deallocate_Memory((VOID*)spki) != NU_SUCCESS))
                        )
                    {
                        NLOG_Error_Log(
                            "Failed to free certificate or SPKI memory.",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to allocate memory for CA's KeyId",
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

    /* If CA's KeyId is not to be sent in request, set the length of this
     * field to zero.
     */
    else
    {
        cert_req->ike2_ca_len = 0;
    }

    /* If all went good so far, add the payload to the chain of outgoing
     * payloads.
     */
    if(status == NU_SUCCESS)
    {
        cert_req->ike2_gen.ike2_critical = 0;
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last, cert_req,
                          IKE2_CERTREQ_PAYLOAD_ID);
    }
    else
    {
        NLOG_Error_Log("Failed to generate CERTREQ payload data",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
} /* IKE2_Generate_CertReq_Data */

#endif /* #if (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_TS
*
* DESCRIPTION
*
*       Generates the TS data for out going packets. This function is
*       used when we are the initiator.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters provided are invalid.
*
*************************************************************************/
STATUS IKE2_Generate_TS(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_TS             *ts_i;
    IKE2_TS             *ts_r;
    IPSEC_SELECTOR      *selector;
    UINT8               addr_len = 0;
    IKE2_IPS_SA         *sa2;
    UINT8               mask;
    INT                 i;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Set local pointers. */
    ts_i = handle->ike2_params->ike2_out.ike2_ts_i->ike2_ts;
    ts_r = handle->ike2_params->ike2_out.ike2_ts_r->ike2_ts;
    selector = &handle->ike2_ips_pol_selector;

    sa2 = handle->ike2_sa2_db.ike_flink;

    /* A TS payload can either represent a pair of IP addresses or a
     * range of IP addresses.
     */

    /*
     * First populate the TSi payload.
     */
    ts_i->ike2_selector_length = IKE2_TS_SELECTOR_HDR_LEN;

#if (INCLUDE_IPV4 == NU_TRUE)
    if((selector->ipsec_source_type & IPSEC_IPV4) != 0)
    {
        ts_i->ike2_ts_type = IKE2_TS_IPV4_ADDR_RANGE;
        ts_i->ike2_selector_length = ts_i->ike2_selector_length + 2 *
                                     IP_ADDR_LEN;
        addr_len = IP_ADDR_LEN;
    }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
    {
        ts_i->ike2_ts_type = IKE2_TS_IPV6_ADDR_RANGE;
        ts_i->ike2_selector_length = ts_i->ike2_selector_length + 2 *
                                     IP6_ADDR_LEN;
        addr_len = IP6_ADDR_LEN;
    }
#endif

    ts_i->ike2_ip_protocol_id = selector->ipsec_transport_protocol;

    switch(ts_i->ike2_ip_protocol_id)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IPPROTO_ICMP:
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    case IPPROTO_ICMPV6:
#endif
        if(selector->ipsec_icmp_msg == IPSEC_WILDCARD)
        {
            ts_i->ike2_start_port = 0x0000;
            ts_i->ike2_end_port = 0xFFFF;
        }
        else
        {
            ts_i->ike2_start_port = (UINT16)selector->ipsec_icmp_msg << 8 & 0xFF00;
            ts_i->ike2_end_port = (UINT16)selector->ipsec_icmp_msg << 8 | 0x00FF;
        }

        if(selector->ipsec_icmp_code != IPSEC_WILDCARD)
        {
            ts_i->ike2_start_port = (UINT16)selector->ipsec_icmp_code |
                                                ts_i->ike2_start_port;
            ts_i->ike2_end_port = ((UINT16)selector->ipsec_icmp_code & 0xFFFF) |
                                                ts_i->ike2_end_port;
        }
        /* We do not currently support ts_i and ts_r
         * to be different. */
        ts_r->ike2_start_port = ts_i->ike2_start_port;
        ts_r->ike2_end_port = ts_i->ike2_end_port;

        break;

    case 135: /* Case for IP Mobility header. No define in NET found. */
        ts_i->ike2_start_port = selector->ipsec_src_tid.ipsec_mobility_hdr;
        ts_i->ike2_end_port = 0;

        ts_r->ike2_start_port = selector->ipsec_dst_tid.ipsec_mobility_hdr;
        ts_r->ike2_end_port = 0;

        break;

    default:

        if(selector->ipsec_src_tid.ipsec_src_port == IPSEC_WILDCARD)
        {
            ts_i->ike2_start_port = 0;
            ts_i->ike2_end_port = 65535;
        }

        else
        {
            ts_i->ike2_start_port = selector->ipsec_src_tid.ipsec_src_port;
            ts_i->ike2_end_port = selector->ipsec_src_tid.ipsec_src_port;
        }

        if(selector->ipsec_dst_tid.ipsec_dst_port == IPSEC_WILDCARD)
        {
            ts_r->ike2_start_port = 0;
            ts_r->ike2_end_port = 65535;
        }

        else
        {
            ts_r->ike2_start_port = selector->ipsec_dst_tid.ipsec_dst_port;
            ts_r->ike2_end_port = selector->ipsec_dst_tid.ipsec_dst_port;
        }

        break;
    }

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    if((sa2->ike_ips_security.ipsec_security_mode == IPSEC_TUNNEL_MODE) &&
        ((handle->ike2_flags & IKE2_INITIATOR) != 0) &&
        ((handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
        IKE2_HDR_RESPONSE_FLAG) == 0))
    {
        NU_BLOCK_COPY(ts_i->ike2_start_addr.is_ip_addrs,
                      selector->ipsec_dest_ip.ipsec_addr, addr_len);

        if((selector->ipsec_source_type & IPSEC_SINGLE_IP) != 0)
        {
            NU_BLOCK_COPY(ts_i->ike2_end_addr.is_ip_addrs,
                selector->ipsec_dest_ip.ipsec_addr, addr_len);
        }

        else
        {
            NU_BLOCK_COPY(ts_i->ike2_end_addr.is_ip_addrs,
                selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                addr_len);
        }
    }

    else
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
    if((selector->ipsec_source_type & IPSEC_SUBNET_IP) == 0)
    {
        NU_BLOCK_COPY(ts_i->ike2_start_addr.is_ip_addrs,
            selector->ipsec_source_ip.ipsec_addr, addr_len);

        if((selector->ipsec_source_type & IPSEC_SINGLE_IP) != 0)
        {
            NU_BLOCK_COPY(ts_i->ike2_end_addr.is_ip_addrs,
                selector->ipsec_source_ip.ipsec_addr, addr_len);
        }

        else
        {
            NU_BLOCK_COPY(ts_i->ike2_end_addr.is_ip_addrs,
                selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2,
                addr_len);
        }
    }
    else
    {
        /* Processing for IPv4 and IPv6 subnets. */

        for(i = 0; i < addr_len; i++)
        {
            ts_i->ike2_start_addr.is_ip_addrs[i] =
                (selector->ipsec_source_ip.ipsec_addr[i] &
                 selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2[i]);

            mask =
                ~(selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2[i]);

            ts_i->ike2_end_addr.is_ip_addrs[i] =
                (selector->ipsec_source_ip.ipsec_addr[i] | mask);
        }
    }

    /*
     * Now populate TSr payload.
     */
    ts_r->ike2_selector_length = IKE2_TS_SELECTOR_HDR_LEN;

#if (INCLUDE_IPV4 == NU_TRUE)
    if((selector->ipsec_dest_type & IPSEC_IPV4) != 0)
    {
        ts_r->ike2_ts_type = IKE2_TS_IPV4_ADDR_RANGE;
        ts_r->ike2_selector_length = ts_r->ike2_selector_length + 2 *
                                     IP_ADDR_LEN;
        addr_len = IP_ADDR_LEN;
    }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
    {
        ts_r->ike2_ts_type = IKE2_TS_IPV6_ADDR_RANGE;
        ts_r->ike2_selector_length = ts_r->ike2_selector_length + 2 *
                                     IP6_ADDR_LEN;
        addr_len = IP6_ADDR_LEN;
    }
#endif

    ts_r->ike2_ip_protocol_id = selector->ipsec_transport_protocol;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    if((sa2->ike_ips_security.ipsec_security_mode == IPSEC_TUNNEL_MODE) &&
        ((handle->ike2_flags & IKE2_INITIATOR) != 0) &&
        ((handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
        IKE2_HDR_RESPONSE_FLAG) == 0))
    {
        NU_BLOCK_COPY(ts_r->ike2_start_addr.is_ip_addrs,
            selector->ipsec_source_ip.ipsec_addr, addr_len);

        if((selector->ipsec_source_type & IPSEC_SINGLE_IP) != 0)
        {
            NU_BLOCK_COPY(ts_r->ike2_end_addr.is_ip_addrs,
                selector->ipsec_source_ip.ipsec_addr, addr_len);
        }

        else
        {
            NU_BLOCK_COPY(ts_r->ike2_end_addr.is_ip_addrs,
                selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2,
                addr_len);
        }
    }

    else
#endif /* (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) */
    if((selector->ipsec_dest_type & IPSEC_SUBNET_IP) == 0)
    {
        NU_BLOCK_COPY(ts_r->ike2_start_addr.is_ip_addrs,
            selector->ipsec_dest_ip.ipsec_addr, addr_len);

        if((selector->ipsec_dest_type & IPSEC_SINGLE_IP) != 0)
        {
            NU_BLOCK_COPY(ts_r->ike2_end_addr.is_ip_addrs,
                selector->ipsec_dest_ip.ipsec_addr, addr_len);
        }

        else
        {
            NU_BLOCK_COPY(ts_r->ike2_end_addr.is_ip_addrs,
                selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                addr_len);
        }
    }
    else
    {
        /* Processing for IPv4 and IPv6 subnets. */

        for(i = 0; i < addr_len; i++)
        {
            ts_r->ike2_start_addr.is_ip_addrs[i] =
                (selector->ipsec_dest_ip.ipsec_addr[i] &
                 selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2[i]);

            mask =
                ~(selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2[i]);

            ts_r->ike2_end_addr.is_ip_addrs[i] =
                (selector->ipsec_dest_ip.ipsec_addr[i] | mask);
        }
    }

    /* Enqueue TS payload to the chain of outgoing payloads. */
    handle->ike2_params->ike2_out.ike2_ts_i->ike2_ts_count = 1;
    handle->ike2_params->ike2_out.ike2_ts_i->ike2_gen.ike2_critical = 0;
    handle->ike2_params->ike2_out.ike2_ts_i->ike2_gen.ike2_payload_type =
        IKE2_TS_I_PAYLOAD_ID;
    handle->ike2_params->ike2_out.ike2_ts_i->ike2_gen.ike2_payload_length
        = IKE2_MIN_TS_HEAD_PAYLOAD_LEN + IKE2_TS_SELECTOR_HDR_LEN +
          (addr_len * 2);

    handle->ike2_params->ike2_out.ike2_ts_r->ike2_ts_count = 1;
    handle->ike2_params->ike2_out.ike2_ts_r->ike2_gen.ike2_critical = 0;
    handle->ike2_params->ike2_out.ike2_ts_r->ike2_gen.ike2_payload_type =
        IKE2_TS_R_PAYLOAD_ID;
    handle->ike2_params->ike2_out.ike2_ts_r->ike2_gen.ike2_payload_length
        = IKE2_MIN_TS_HEAD_PAYLOAD_LEN + IKE2_TS_SELECTOR_HDR_LEN +
          (addr_len * 2);

    IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                      handle->ike2_params->ike2_out.ike2_ts_i,
                      IKE2_TS_I_PAYLOAD_ID);

    IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                      handle->ike2_params->ike2_out.ike2_ts_r,
                      IKE2_TS_R_PAYLOAD_ID);

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Verify_TS
*
* DESCRIPTION
*
*       When we are the responder, this function is used to verify that
*       the traffic selectors that we have received are acceptable for
*       us.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              Traffic selectors are acceptable.
*
*************************************************************************/
STATUS IKE2_Verify_TS(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_TS_PAYLOAD     *ts_i;
    IKE2_TS_PAYLOAD     *ts_r;
    UINT8               addr_len = 0;
    IPSEC_SELECTOR      *selector;
    UINT8               tsi_msg_start, tsi_msg_end, tsr_msg_start,
                        tsr_msg_end, tsi_code_start, tsi_code_end,
                        tsr_code_start, tsr_code_end;

    ts_i = handle->ike2_params->ike2_in.ike2_ts_i;
    ts_r = handle->ike2_params->ike2_in.ike2_ts_r;
    selector = &handle->ike2_ips_pol_selector;

#if (INCLUDE_IPV4 == NU_TRUE)
    if((selector->ipsec_source_type & IPSEC_IPV4) != 0)
    {
        addr_len = IP_ADDR_LEN;
        if(ts_i->ike2_ts->ike2_ts_type != IKE2_TS_IPV4_ADDR_RANGE)
        {
            status = IKE2_TS_MISMATCH;
        }
    }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
    {
        addr_len = IP6_ADDR_LEN;
        if(ts_i->ike2_ts->ike2_ts_type != IKE2_TS_IPV6_ADDR_RANGE)
        {
            status = IKE2_TS_MISMATCH;
        }
    }
#endif

    switch(ts_i->ike2_ts->ike2_ip_protocol_id)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IPPROTO_ICMP:
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    case IPPROTO_ICMPV6:
#endif
        tsi_msg_start  = (UINT8)(ts_i->ike2_ts->ike2_start_port >> 8);
        tsi_msg_end    = (UINT8)(ts_i->ike2_ts->ike2_end_port >> 8);
        tsr_msg_start  = (UINT8)(ts_r->ike2_ts->ike2_start_port >> 8);
        tsr_msg_end    = (UINT8)(ts_r->ike2_ts->ike2_end_port >> 8);
        tsi_code_start = (UINT8)(ts_i->ike2_ts->ike2_start_port & 0xff);
        tsi_code_end   = (UINT8)(ts_i->ike2_ts->ike2_end_port & 0xff);
        tsr_code_start = (UINT8)(ts_r->ike2_ts->ike2_start_port & 0xff);
        tsr_code_end   = (UINT8)(ts_r->ike2_ts->ike2_end_port & 0xff);

        /* Verify icmp/icmpv6 code. */
        if(selector->ipsec_icmp_code != IPSEC_WILDCARD)
        {
            if(selector->ipsec_icmp_code >= tsi_code_start &&
               selector->ipsec_icmp_code <= tsi_code_end &&
               selector->ipsec_icmp_code >= tsr_code_start &&
               selector->ipsec_icmp_code <= tsr_code_end)
            {
                tsi_code_start = selector->ipsec_icmp_code;
                tsi_code_end   = selector->ipsec_icmp_code;
                tsr_code_start = selector->ipsec_icmp_code;
                tsr_code_end   = selector->ipsec_icmp_code;
            }
            else
            {
                status = IKE2_TS_MISMATCH;
            }
        }

        /* Verify icmp/icmpv6 type. */
        if(selector->ipsec_icmp_msg != IPSEC_WILDCARD && status != IKE2_TS_MISMATCH)
        {
            if(selector->ipsec_icmp_msg >= tsi_msg_start &&
               selector->ipsec_icmp_msg <= tsi_msg_end &&
               selector->ipsec_icmp_msg >= tsr_msg_start &&
               selector->ipsec_icmp_msg <= tsr_msg_end)
            {
                tsi_msg_start = selector->ipsec_icmp_msg;
                tsi_msg_end   = selector->ipsec_icmp_msg;
                tsr_msg_start = selector->ipsec_icmp_msg;
                tsr_msg_end   = selector->ipsec_icmp_msg;
            }
            else
            {
                status = IKE2_TS_MISMATCH;
            }
        }

        if(status == NU_SUCCESS)
        {
            /* The responder is allowed to narrow the choices by selecting
             * a subset of the traffic, for instance by eliminating or
             * narrowing the range of one or more members of the set of
             * traffic selectors, provided the set does not become the
             * NULL set. (RFC4306 section 2.9)
             */
            ts_i->ike2_ts->ike2_start_port = (tsi_msg_start << 8) |
                tsi_code_start;
            ts_i->ike2_ts->ike2_end_port = (tsi_msg_end << 8) |
                tsi_code_end;
            ts_r->ike2_ts->ike2_start_port = (tsr_msg_start << 8) |
                tsr_code_start;
            ts_r->ike2_ts->ike2_end_port = (tsr_msg_end << 8) |
                tsr_code_end;
        }
        break;

    case 135: /* Case for IP Mobility header. No define in NET found. */
        break;

    default:
        if(selector->ipsec_src_tid.ipsec_src_port == IPSEC_WILDCARD)
        {
            if((ts_i->ike2_ts->ike2_start_port != 0) ||
               (ts_i->ike2_ts->ike2_end_port != 65535))
            {
                status = IKE2_TS_MISMATCH;
            }
        }
        else
        {
            /* If the policy's ports are a subset of the Traffic Selector
             * ports. */
            if((ts_i->ike2_ts->ike2_start_port <=
                selector->ipsec_src_tid.ipsec_src_port) &&
                (ts_i->ike2_ts->ike2_end_port >=
                 selector->ipsec_src_tid.ipsec_intern_src_port_end[1]))
            {
                ts_i->ike2_ts->ike2_start_port =
                    selector->ipsec_src_tid.ipsec_src_port;
                ts_i->ike2_ts->ike2_end_port =
                    selector->ipsec_src_tid.ipsec_intern_src_port_end[1];
                ts_r->ike2_ts->ike2_start_port =
                    selector->ipsec_dst_tid.ipsec_dst_port;
                ts_r->ike2_ts->ike2_end_port =
                    selector->ipsec_dst_tid.ipsec_intern_dst_port_end[1];
            }
            else
            {
                status = IKE2_TS_MISMATCH;
            }
        }

        break;
    }

    if(status == NU_SUCCESS)
    {
        /* We have selected a selector so update the selector
         * count and payload length.
         */
        ts_i->ike2_ts_count = 1;
        ts_r->ike2_ts_count = 1;

        handle->ike2_params->ike2_out.ike2_ts_i->ike2_gen.ike2_payload_length =
                IKE2_MIN_TS_HEAD_PAYLOAD_LEN + IKE2_TS_SELECTOR_HDR_LEN +
                      (addr_len * 2);

        handle->ike2_params->ike2_out.ike2_ts_r->ike2_gen.ike2_payload_length =
                IKE2_MIN_TS_HEAD_PAYLOAD_LEN + IKE2_TS_SELECTOR_HDR_LEN +
                       (addr_len * 2);

        /* Selectors have matched. Send them back to initiator. */
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                          ts_i, IKE2_TS_I_PAYLOAD_ID);

        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                          ts_r, IKE2_TS_R_PAYLOAD_ID);
    }

    return (status);

} /* IKE2_Verify_TS */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Extract_TS_Info
*
* DESCRIPTION
*
*       Extracts the information from the TS payload sent by peer and
*       fills the selector in the handle to help find related policy.
*
* INPUTS
*
*       handle                  Exchange handle.
*       index_i                 index of ts in ts_i
*       index_r                 index of ts in ts_r
*
* OUTPUTS
*
*       NU_SUCCESS              On successful execution.
*
*************************************************************************/
STATUS IKE2_Extract_TS_Info(IKE2_EXCHANGE_HANDLE *handle, UINT16 index_i, UINT16 index_r)
{
    IKE2_TS             *ts_i;
    IKE2_TS             *ts_r;
    IPSEC_SELECTOR      *selector;

    ts_i = &(handle->ike2_params->ike2_in.ike2_ts_i->ike2_ts[index_i]);
    ts_r = &(handle->ike2_params->ike2_in.ike2_ts_r->ike2_ts[index_r]);

    selector = &handle->ike2_ips_selector;

    /* Do not modify addresses in transport mode. */
    selector->ipsec_source_type = ts_i->ike2_ts_type;
    selector->ipsec_dest_type   = ts_r->ike2_ts_type;

    selector->ipsec_transport_protocol = ts_i->ike2_ip_protocol_id;

    switch(ts_i->ike2_ip_protocol_id)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IPPROTO_ICMP:
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    case IPPROTO_ICMPV6:
#endif

        selector->ipsec_icmp_code = (UINT8)ts_i->ike2_start_port;
        selector->ipsec_icmp_msg = (UINT8)(ts_i->ike2_start_port >> 8);
        selector->ipsec_icmp_code_high = (UINT8)ts_r->ike2_start_port;
        selector->ipsec_icmp_msg_high = (UINT8)(ts_r->ike2_start_port >> 8);

        break;

    case 135: /* Case for IP Mobility header. No define in NET found. */
        break;

    default:

        selector->ipsec_src_tid.ipsec_src_port = ts_i->ike2_start_port;
        selector->ipsec_src_tid.ipsec_intern_src_port_end[1] =
                ts_i->ike2_end_port;
        selector->ipsec_dst_tid.ipsec_dst_port = ts_r->ike2_start_port;
        selector->ipsec_dst_tid.ipsec_intern_dst_port_end[1] =
                ts_r->ike2_end_port;

        break;
    }

    switch(selector->ipsec_source_type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IKE2_TS_IPV4_ADDR_RANGE:
        NU_BLOCK_COPY(selector->ipsec_source_ip.ipsec_addr,
                        &ts_i->ike2_start_addr, IP_ADDR_LEN);

        NU_BLOCK_COPY(selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2,
                        &ts_i->ike2_end_addr, IP_ADDR_LEN);

        NU_BLOCK_COPY(selector->ipsec_dest_ip.ipsec_addr,
                        &ts_r->ike2_start_addr, IP_ADDR_LEN);

        NU_BLOCK_COPY(selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                        &ts_r->ike2_end_addr, IP_ADDR_LEN);

        if(memcmp(selector->ipsec_source_ip.ipsec_addr,
                  selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2,
                  IP_ADDR_LEN) == 0)
        {
            selector->ipsec_source_type = IPSEC_SINGLE_IP | IPSEC_IPV4;
        }

        else
        {
            selector->ipsec_source_type = IPSEC_RANGE_IP | IPSEC_IPV4;
        }

        if(memcmp(selector->ipsec_dest_ip.ipsec_addr,
                  selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                  IP_ADDR_LEN) == 0)
        {
            selector->ipsec_dest_type = IPSEC_SINGLE_IP | IPSEC_IPV4;
        }

        else
        {
            selector->ipsec_dest_type = IPSEC_RANGE_IP | IPSEC_IPV4;
        }

        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    case IKE2_TS_IPV6_ADDR_RANGE:
        NU_BLOCK_COPY(selector->ipsec_source_ip.ipsec_addr,
                        &ts_i->ike2_start_addr, IP6_ADDR_LEN);

        NU_BLOCK_COPY(selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2,
                        &ts_i->ike2_end_addr, IP6_ADDR_LEN);

        NU_BLOCK_COPY(selector->ipsec_dest_ip.ipsec_addr,
                        &ts_r->ike2_start_addr, IP6_ADDR_LEN);

        NU_BLOCK_COPY(selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                        &ts_r->ike2_end_addr, IP6_ADDR_LEN);

        if(memcmp(selector->ipsec_source_ip.ipsec_addr,
                  selector->ipsec_source_ip.ipsec_ext_addr.ipsec_addr2,
                  IP6_ADDR_LEN) == 0)
        {
            selector->ipsec_source_type = IPSEC_SINGLE_IP | IPSEC_IPV6;
        }

        else
        {
            selector->ipsec_source_type = IPSEC_RANGE_IP | IPSEC_IPV6;
        }

        if(memcmp(selector->ipsec_dest_ip.ipsec_addr,
                  selector->ipsec_dest_ip.ipsec_ext_addr.ipsec_addr2,
                  IP6_ADDR_LEN) == 0)
        {
            selector->ipsec_dest_type = IPSEC_SINGLE_IP | IPSEC_IPV6;
        }

        else
        {
            selector->ipsec_dest_type = IPSEC_RANGE_IP | IPSEC_IPV6;
        }

        break;
#endif
    default:
        break;
    }

    return (NU_SUCCESS);

} /* IKE2_Extract_TS_Info */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Verify_Auth
*
* DESCRIPTION
*
*       Verifies the Authentication data sent by peer.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Verify_Auth(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    UINT8               algo_id;
    UINT8               *digest;
    UINT8               digest_len;
    UINT8               *text;
    INT                 text_len;
    UINT8               *key;
    INT                 key_len;
    IKE2_ID_PAYLOAD     *peer_id;
    IKE2_AUTH_PAYLOAD   *peer_auth;
    UINT8               *auth_buffer = NU_NULL;
    UINT16              auth_buffer_len;
    UINT8               *id_prf = NU_NULL;
    UINT16              id_prf_len;
    UINT8               *auth_key = NU_NULL;
    UINT8               auth_key_len = 0;
    UINT8               *temp_ptr = NU_NULL;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    EVP_MD_CTX          hash_ctx;
    UINT8               hash_len = 0;
    UINT16              sign_algo;
#endif

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    else if(((handle->ike2_flags & IKE2_INITIATOR) == NU_TRUE) &&
        (handle->ike2_params->ike2_in.ike2_id_r == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    else if(((handle->ike2_flags & IKE2_RESPONDER) == NU_TRUE) &&
        (handle->ike2_params->ike2_in.ike2_id_i == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    peer_auth = handle->ike2_params->ike2_in.ike2_auth;

    /* Verify the Authentication data sent by peer. */
    if((handle->ike2_flags & IKE2_INITIATOR) != 0)
    {
        peer_id = handle->ike2_params->ike2_in.ike2_id_r;
    }
    else
    {
        peer_id = handle->ike2_params->ike2_in.ike2_id_i;
    }

    /* The initiator signs the first message, starting with the
     * first octet of the first SPI in the header and ending with the last
     * octet of the last payload.  Appended to this (For purposes of
     * computing the signature) are the responder's nonce Nr, and the value
     * prf(SK_pi,IDi').  In the above calculation, IDi' and IDr' are the
     * entire ID payloads excluding the fixed header.  It is critical to
     * security of the exchange that each side sign the other side's nonce.
     *
     * First MSG | Nonce(R) | prf(SK_pi, IDi')
     */

    /* Authentication data will be calculated with PSK as:
     * AUTH = prf(prf(PSK, PSK_PAD), (Message|Nonce|prf(SK_p,ID)))
     *
     * or with digital signatures as:
     * AUTH = SHA1(PrivateKey, (Message|Nonce|prf(SK_p,ID)))
     */

    id_prf_len = IKE2_PRF_HMAC_Algos[handle->ike2_sa->ike_attributes.
                    ike2_prf_algo].ike2_output_len;

    /* Length of the buffer to hold messages octets appended with nonce
     * PRFed ID payload.
     */
    auth_buffer_len = handle->ike2_sa->ike2_peer_auth_len +
                        handle->ike2_nonce_len + id_prf_len;

    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&id_prf,
                                id_prf_len + peer_id->ike2_id_data_len,
                                NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        memset(id_prf, 0, id_prf_len);

        /* Encode ID and reserved data into allocated buffer. */
        PUT32(id_prf, 0, peer_id->ike2_rsvd);
        NU_BLOCK_COPY(id_prf + 4, peer_id->ike2_id_data,
                      peer_id->ike2_id_data_len);

        /* Calculate PRF(SK_p, ID) */
        digest = id_prf;
        digest_len = (UINT8)id_prf_len;
        algo_id = IKE2_PRF_Algos[handle->ike2_sa->ike_attributes.ike2_prf_algo].
                                    crypto_algo_id;

        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            key = handle->ike2_sa->ike2_sk_pr;
        }

        else
        {
            key = handle->ike2_sa->ike2_sk_pi;
        }

        key_len = IKE2_PRF_HMAC_Algos[handle->ike2_sa->
                                    ike_attributes.ike2_prf_algo].
                                    ike2_key_len;
        text = id_prf;

        /* Data over which this hash is being calculated is the ID value
         * pre-pended with a 4 octet value (ID type).
         */
        text_len = peer_id->ike2_id_data_len + 4;

        status = IKE_HMAC(algo_id, key, key_len,
                          text, text_len, digest, &digest_len);

        if(status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&auth_buffer,
                                        auth_buffer_len, NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                temp_ptr = auth_buffer;

                /* Copy in the saved message over which to calculate PRF */
                NU_BLOCK_COPY(temp_ptr,
                              handle->ike2_sa->ike2_peer_auth_data,
                              handle->ike2_sa->ike2_peer_auth_len);
                temp_ptr = temp_ptr + handle->ike2_sa->ike2_peer_auth_len;

                /* Copy in the nonce value. */
                NU_BLOCK_COPY(temp_ptr, handle->ike2_nonce,
                              handle->ike2_nonce_len);
                temp_ptr = temp_ptr + handle->ike2_nonce_len;

                /* Copy in the PRF(SK_p, ID) */
                NU_BLOCK_COPY(temp_ptr, id_prf, digest_len);

            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for \
                               authentication data buffer",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to calculate PRF(SK_p, ID)",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for prf(ID) buffer",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(status == NU_SUCCESS)
    {
        /* Check to see which authentication method was used. It can either
         * be pre-shared key or RSA authentication.
         */
        switch(peer_auth->ike2_auth_method)
        {
        case IKE2_AUTH_METHOD_SKEY_MIC:
            /* No need to explicitly lookup pre-shared key as it is
             * part of the SA now. */

            if(status == NU_SUCCESS)
            {
                /* PRF(SK_pr,IDr') calculated. Now calculate
                 * PRF(Shared key, PSK_PAD).
                 */
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                            (VOID**)&auth_key,
                                            id_prf_len, NU_NO_SUSPEND);
                if(status == NU_SUCCESS)
                {
                    /* Calculate the key for calculating the AUTH data. */
                    digest = auth_key;
                    digest_len = 0;
                    algo_id = IKE2_PRF_Algos[handle->
                                                ike2_sa->ike_attributes.
                                                ike2_prf_algo].crypto_algo_id;
                    key = handle->ike2_sa->ike_attributes.ike2_psk_remote;
                    key_len = handle->ike2_sa->
                                       ike_attributes.ike2_psk_remote_len;
                    text = (UINT8*)IKE2_PSK_PAD_STRING;
                    text_len = IKE2_PSK_PAD_LENGTH;

                    status = IKE_HMAC(algo_id, key, key_len,
                                      text, text_len, digest, &digest_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Save the size of the key length calculated in
                         * previous operation.
                         */
                        auth_key_len = digest_len;

                        /* Fill the HMAC request function. */
                        digest = auth_buffer;
                        digest_len = IKE2_PRF_HMAC_Algos[handle->
                                                    ike2_sa->ike_attributes.
                                                    ike2_prf_algo].ike2_output_len;
                        algo_id = IKE2_PRF_Algos[handle->
                                                    ike2_sa->ike_attributes.
                                                    ike2_prf_algo].crypto_algo_id;
                        key = auth_key;
                        key_len = auth_key_len;
                        text = auth_buffer;
                        text_len = auth_buffer_len;

                        status = IKE_HMAC(algo_id, key, key_len,
                                          text, text_len, digest, &digest_len);

                        if(status == NU_SUCCESS)
                        {
                            if(digest_len != peer_auth->ike2_auth_data_len)
                            {
                                status = IKE_AUTH_FAILED;
                                NLOG_Error_Log("Invalid length of AUTH data",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("AUTH data could not be calculated",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                    }

                    else
                    {
                        NLOG_Error_Log("Failed to calculate PRF over \
                                       PSK pad", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to allocate memory for auth \
                                   key buffer", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);
                }

            }

            else
            {
                NLOG_Error_Log("Pre-shared key not found", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }

            /* AUTH data calculated, now compare it with data
             * received from peer.
             */
            if(status == NU_SUCCESS)
            {
                if(memcmp(auth_buffer, peer_auth->ike2_auth_data,
                    peer_auth->ike2_auth_data_len) == NU_SUCCESS)
                {
                    /* Authentication data has matched. The IKE SA is now
                     * established.
                     */
                    handle->ike2_sa->ike_state = IKE2_SA_ESTABLISHED;

                    /* See what we need to do when the SA expires. Set the
                     * action.
                     */
                    if((handle->ike2_params->ike2_policy->ike2_flags &
                        IKE2_SA_REKEY) != 0)
                    {
                        handle->ike2_sa->ike2_flags |= IKE2_SA_REKEY;
                    }

                    else if((handle->ike2_params->ike2_policy->ike2_flags &
                            IKE2_SA_DELETE) != 0)
                    {
                        handle->ike2_sa->ike2_flags |= IKE2_SA_DELETE;
                    }

                }

                else
                {
                    status = IKE_AUTH_FAILED;
                    NLOG_Error_Log("Authentication failed!", NERR_RECOVERABLE,
                        __FILE__, __LINE__);
                }
            }

            break;

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
        case IKE2_AUTH_METHOD_RSA_DS:

            /* Verify the signature. The hash calculation for generation and
             * verification of signature is always done with SHA1. RFC4718,
             * section 3.2.
             */

            /* Initialize the hash context. */
            status = IKE_Hash_Init(IKE_OPENSSL_SHA1, &hash_ctx);

            if(status == NU_SUCCESS)
            {
                /* Update hash context with data over which hash is to be
                 * calculated.
                 */
                status = IKE_Hash_Update(&hash_ctx, auth_buffer, auth_buffer_len);

                if(status == NU_SUCCESS)
                {
                    /* Finalize the hash calculation and copy the calculated
                     * hash in out buffer.
                     */
                    status = IKE_Hash_Final(&hash_ctx, auth_buffer, &hash_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Get the signature algorithm index. */
                        status = IKE2_SIGN_ALGO_INDEX(peer_auth->ike2_auth_method, sign_algo);

                        if(status == NU_SUCCESS)
                        {
                            /* Verify the signature. */
                            status = IKE_Verify_Signature(IKE_Sign_Algos[sign_algo].crypto_algo_id,
                                                          &(handle->ike2_sa->ike_attributes),
                                                          auth_buffer, hash_len, NID_sha1,
                                                          peer_auth->ike2_auth_data,
                                                          peer_auth->ike2_auth_data_len);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Signature verification failed",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                                /* Set status to IKE error status so that
                                 * informational messages can be sent.
                                 */
                                status = IKE_VERIFY_FAILED;
                            }

                            else
                            {
                                /* Log debug message. */
                                IKE2_DEBUG_LOG("Signature verification successful");
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Unknown signature algorithm specified",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to finalize hash context",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update hash context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to initialize the hash context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            break;
#endif

        default:
            status = IKE_UNSUPPORTED_METHOD;
            break;
        }
    }

    /* Deallocate buffers allocated in this function. */

    if(auth_buffer != NU_NULL)
    {
        if(NU_Deallocate_Memory(auth_buffer) != NU_SUCCESS)
        {
            NLOG_Error_Log(
                "Could not deallocate memory for authentication data buffer",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    if(id_prf != NU_NULL)
    {
        if(NU_Deallocate_Memory(id_prf) != NU_SUCCESS)
        {
            NLOG_Error_Log("Could not deallocate memory for ID buffer",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    if(auth_key != NU_NULL)
    {
        if(NU_Deallocate_Memory(auth_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Could not deallocate memory for authentication key",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE2_Verify_Auth */

#endif

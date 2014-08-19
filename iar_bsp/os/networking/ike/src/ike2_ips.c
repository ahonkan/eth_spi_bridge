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
*       ike2_ips.c
*
* COMPONENT
*
*       IKEv2 - IPsec Specific
*
* DESCRIPTION
*
*       This file contains the IPsec specific functions used
*       by IKEv2.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE2_IPS_Tunnel_Override
*       IKE2_AH_Trans_ID_IPS_To_IKE
*       IKE2_Protocol_ID_IPS_To_IKE
*       IKE2_ESP_Trans_ID_IPS_To_IKE
*       IKE2_Get_IPsec_Policy_Security
*       IKE2_IPS_Tunnel_Override
*       IKE2_IPS_Generate_SA_Pair
*       IKE2_IPS_ID_To_Selector
*       IKE2_IPS_Policy_By_ID
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike_ips.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_ips.h"
#include "networking/ips_api.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/* IPsec semaphore to protect access to IPsec data structures. */
extern NU_SEMAPHORE         IPSEC_Resource;

/*************************************************************************
*
* FUNCTION
*
*       IKE2_AH_Trans_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       This function maps the IPsec AH authentication algorithm
*       ID to the IKEv2 AH transform ID. It returns zero if no
*       matching ID is found. No valid transform ID could be zero
*       because it is RESERVED.
*
* INPUTS
*
*       ips_auth_algo           The IPsec authentication algorithm ID.
*
* OUTPUTS
*
*       The IKEv2 transform ID equivalent to the specified
*       authentication algorithm ID, or 0 (RESERVED) on
*       failure.
*
*************************************************************************/
UINT8 IKE2_AH_Trans_ID_IPS_To_IKE(UINT8 ips_auth_algo)
{
    UINT8           transform_id;

    /* Determine the IPsec algorithm. */
    switch(ips_auth_algo)
    {
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
    case IPSEC_HMAC_MD5_96:
        /* Set the IKEv2 transform ID. */
        transform_id = IKE2_AUTH_HMAC_MD5_96;
        break;
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
    case IPSEC_HMAC_SHA_1_96:
        /* Set the IKEv2 transform ID. */
        transform_id = IKE2_AUTH_HMAC_SHA1_96;
        break;
#endif

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
    case IPSEC_XCBC_MAC_96:
        /* Set the IKEv2 transform ID. */
        transform_id = IKE2_AUTH_AES_XCBC_96;
        break;
#endif

    default:
        /* Unrecognized IPsec algorithm ID. */
        transform_id = 0;
        break;
    }

    /* Return the transform ID. */
    return (transform_id);

} /* IKE2_AH_Trans_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Protocol_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       Converts the protocol identifiers from IPsec specific
*       values to IKEv2 specific values.
*
* INPUTS
*
*       ips_proto               IPsec protocol ID to be converted.
*
* OUTPUTS
*
*       UINT8                   IKEv2 specific protocol ID. Zero on
*                               failure.
*
*************************************************************************/
UINT8 IKE2_Protocol_ID_IPS_To_IKE(UINT8 ips_proto)
{
    UINT8       ret_id;

    switch(ips_proto)
    {
#if (IPSEC_INCLUDE_AH == NU_TRUE)
    case IPSEC_AH:
        ret_id = IKE2_PROTO_ID_AH;
        break;
#endif

#if (IPSEC_INCLUDE_ESP == NU_TRUE)
    case IPSEC_ESP:
        ret_id = IKE2_PROTO_ID_ESP;
        break;
#endif

    default:
        ret_id = 0;
        break;
    }

    return (ret_id);

} /* IKE2_Protocol_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_ESP_Trans_ID_IPS_To_IKE
*
* DESCRIPTION
*
*       This function maps the IPsec ESP encryption algorithm
*       ID to the IKEv2 transform ID. It returns zero if no matching
*       ID is found. No valid transform ID could be zero because
*       it is RESERVED.
*
* INPUTS
*
*       ips_encrypt_algo        The IPsec encryption algorithm ID.
*
* OUTPUTS
*
*       The IKEv2 transform ID equivalent to the specified
*       encryption algorithm ID, or 0 (RESERVED) on
*       failure.
*
*************************************************************************/
UINT8 IKE2_ESP_Trans_ID_IPS_To_IKE(UINT8 ips_encrypt_algo)
{
    UINT8           transform_id;

    /* Determine the IPsec encryption algorithm ID. */
    switch(ips_encrypt_algo)
    {
#if (IPSEC_INCLUDE_DES == NU_TRUE)
    case IPSEC_DES_CBC:
        /* Set the IKEv2 transform ID. */
        transform_id = IKE2_ENCR_DES;
        break;
#endif

#if (IPSEC_INCLUDE_3DES == NU_TRUE)
    case IPSEC_3DES_CBC:
        /* Set the IKEv2 transform ID. */
        transform_id = IKE2_ENCR_3DES;
        break;
#endif

#if (IPSEC_INCLUDE_CAST128 == NU_TRUE)
    case IPSEC_CAST_128:
        /* Set the IKEv2 transform ID. */
        transform_id = IKE2_ENCR_CAST;
        break;
#endif

#if (IPSEC_INCLUDE_BLOWFISH == NU_TRUE)
    case IPSEC_BLOWFISH_CBC:
        /* Set the IKE transform ID. */
        transform_id = IKE2_ENCR_BLOWFISH;
        break;
#endif

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    case IPSEC_NULL_CIPHER:
        /* Set the IKE transform ID. */
        transform_id = IKE2_ENCR_NULL;
        break;
#endif

#if (IPSEC_INCLUDE_AES == NU_TRUE)
    case IPSEC_AES_CBC:
        /* Set the IKE transform ID. */
        transform_id = IKE2_ENCR_AES_CBC;
        break;
#endif

    default:
        /* Unrecognized encryption algorithm ID. */
        transform_id = 0;
        break;
    }

    /* Return the transform ID. */
    return (transform_id);

} /* IKE2_ESP_Trans_ID_IPS_To_IKE */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Get_IPsec_Policy_Security
*
* DESCRIPTION
*
*       Get the security parameters for IPsec policy to be used.
*
* INPUTS
*
*       *handle                 Exchange information.
*       **security              Pointer to structure containing the
*                               security parameters.
*       *security_size          Number of security parameter structures
*                               in queue.
*
* OUTPUTS
*
*       NU_SUCCESS              On successfully finding a matching
*                               security suite.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Get_IPsec_Policy_Security(IKE2_EXCHANGE_HANDLE *handle,
                                      IPSEC_SECURITY_PROTOCOL **security,
                                      UINT8 *security_size)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group;
    IPSEC_POLICY        *policy;

#if (IKE2_DEBUG == NU_TRUE)
    /* Check for parameter validity. */
    if((handle == NU_NULL) || (security == NU_NULL) ||
       (security_size == NU_NULL))
    {
        return IKE2_INVALID_PARAMS;
    }
#endif

    IKE2_DEBUG_LOG("Getting IPsec policy security parameters");

    /* Obtain semaphore to protect access to IPsec policy data structures. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Get the group entry for the name we have in exchange handle. */
        status = IPSEC_Get_Group_Entry(handle->ike2_ips_group, &group);

        if(status == NU_SUCCESS)
        {
            /* Now we have the group to which the policy belongs. Now we
             * can find the policy structure using the group and policy
             * index.
             */
            status = IPSEC_Get_Policy_Entry(group,
                                            handle->ike2_ips_policy_index,
                                            &policy);

            if(status == NU_SUCCESS)
            {
                /* Check if policy action is set to apply. For a bypass
                 * policy, IKE should not be negotiating any security.
                 */
                if((policy->ipsec_flags & IPSEC_APPLY) == 0)
                {
                    /* Policy action is bypass. */
                    NLOG_Error_Log("IPsec policy action is bypass, no need\
                                   for exchange.", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);

                    status = IKE2_INVALID_PARAMS;
                }

                else
                {
                    /* Get selector from the IPsec policy. */
                    NU_BLOCK_COPY(&handle->ike2_ips_pol_selector,
                                  &policy->ipsec_select,
                                  sizeof(IPSEC_SELECTOR));

                    /* Get lifetime from the IPsec policy. */
                    NU_BLOCK_COPY(&handle->ike2_ips_lifetime,
                                  &policy->ipsec_sa_max_lifetime,
                                  sizeof(IPSEC_SA_LIFETIME));

                    handle->ike2_sa2_db.ike_ips_flags = policy->ipsec_flags;

                    /* Policy action is apply. */

                    /* Allocate memory for buffer to hold the security
                     * parameters.
                     */
                    status = NU_Allocate_Memory(
                                IKE_Data.ike_memory,
                                (VOID**)security,
                                (policy->ipsec_security_size *
                                sizeof(IPSEC_SECURITY_PROTOCOL)),
                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        *security = TLS_Normalize_Ptr(*security);

                        /* Set the number of security protocols. */
                        *security_size = policy->ipsec_security_size;

                        /* Copy security protocols from IPsec policy. */
                        NU_BLOCK_COPY(*security, policy->ipsec_security,
                                      (policy->ipsec_security_size *
                                      sizeof(IPSEC_SECURITY_PROTOCOL)));
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for IPsec security",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("IPsec policy not found", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to get IPsec Group entry",
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
        NLOG_Error_Log("Failed to obtain IPsec semaphore.", NERR_RECOVERABLE,
                       __FILE__, __LINE__);
    }

    return (status);
} /* IKE2_Get_IPsec_Policy_Security */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_IPS_Generate_SA_Pair
*
* DESCRIPTION
*
*       When negotiation with IKE is complete, this function converts the
*       IKE values to IPsec SA values and calls the IPsec API to add a
*       pair of SA (one inbound and one outbound SA).
*
* INPUTS
*
*       *handle                 Exchange under which the SA information
*                               was negotiated.
*
* OUTPUTS
*
*       NU_SUCCESS              IPsec SA pair successfully added.
*       IKE2_INVALID_PARAMS     Parameters provided were invalid.
*
*************************************************************************/
STATUS IKE2_IPS_Generate_SA_Pair(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    IKE2_IPS_SA         *sa2;
    UINT16              enc_key_len;
    UINT16              auth_key_len;
    IPSEC_INBOUND_SA    ips_in;
    IPSEC_OUTBOUND_SA   ips_out;
    UINT32              ret_spi;

    IKE2_IPS_SA_INDEX   *ips_index;

#if (IKE2_DEBUG == NU_TRUE)
    /* Parameter sanity check. */
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    sa2 = handle->ike2_sa2_db.ike_flink;

    /* Loop for all SA2 items and create an IPsec SA pair corresponding
     * to each of them.
     */
    while(sa2 != NU_NULL)
    {
        /* Get the key length of encryption and authentication algorithms */
        status = IKE_IPS_Get_Key_Length(&sa2->ike_ips_security, &enc_key_len,
                                        &auth_key_len);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to calculate IPsec SA key lengths",
                NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Abort if error occurred. */
            break;
        }

        /* If encryption algorithm is being applied. */
        if(enc_key_len != 0)
        {
            /* Set the encryption key in both SAs. */
            ips_in.ipsec_encryption_key  = sa2->ike_local_keymat;
            ips_out.ipsec_encryption_key = sa2->ike_remote_keymat;
        }

        else
        {
            /* Set encryption key pointers to NULL. */
            ips_in.ipsec_encryption_key  = NU_NULL;
            ips_out.ipsec_encryption_key = NU_NULL;
        }

        /* If authentication algorithm is being applied. */
        if(auth_key_len != 0)
        {
            /* Set the authentication key in both SAs. */
            ips_in.ipsec_auth_key  = sa2->ike_local_keymat +
                (INT)enc_key_len;
            ips_out.ipsec_auth_key = sa2->ike_remote_keymat +
                (INT)enc_key_len;
        }

        else
        {
            /* Set the authentication key pointers to NULL. */
            ips_in.ipsec_auth_key  = NU_NULL;
            ips_out.ipsec_auth_key = NU_NULL;
        }

        /*
         * Generate the IPsec inbound SA.
         */

        /* Log debug message. */
        IKE2_DEBUG_LOG("Establishing IPsec inbound SA");

        /* Set the SPI. */
        ips_in.ipsec_spi = sa2->ike_local_spi;

        /* Copy the SA selector. */
        NU_BLOCK_COPY(&(ips_in.ipsec_select), &(handle->ike2_ips_selector),
                      sizeof(IPSEC_SELECTOR));

        /* If we are the Initiator. */
        if((handle->ike2_flags & IKE2_INITIATOR) != 0 &&
            (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
            IKE2_HDR_RESPONSE_FLAG) != 0)
        {
            /* Copy selector while switching source and destination. */
            IKE_IPS_Switch_Selector(&ips_in.ipsec_select,
                                    &handle->ike2_ips_selector);
        }

        /* Copy the security protocol. */
        NU_BLOCK_COPY(&ips_in.ipsec_security, &sa2->ike_ips_security,
                      sizeof(IPSEC_SECURITY_PROTOCOL));

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If policy direction is outbound or dual asynchronous. */
        if((IPSEC_POLICY_FLOW(handle->ike2_sa2_db.ike_ips_flags)
            & IPSEC_OUTBOUND) != 0)
        {
            /* Switch tunnel source and destination end points. */
            IKE_IPS_Switch_Security(&ips_in.ipsec_security,
                &sa2->ike_ips_security);
        }

        if( (sa2->ike_ips_security.ipsec_security_mode ==
             IPSEC_TUNNEL_MODE) &&
            (handle->ike2_flags & IKE2_INITIATOR) != 0 &&
            (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
             IKE2_HDR_RESPONSE_FLAG) == 0)
        {
            IKE_IPS_Switch_Selector(&ips_in.ipsec_select,
                &handle->ike2_ips_selector);
        }
#endif

        /* Set hard lifetime in SA. */
        NU_BLOCK_COPY(&ips_in.ipsec_hard_lifetime, &handle->ike2_ips_lifetime,
            sizeof(IPSEC_SA_LIFETIME));

        /* Make sure the soft lifetime is only set for the
         * first SA pair, in case multiple pairs are being
         * established. Expiry of the first pair would also
         * re-negotiate all the following pairs.
         */
        if(sa2 == handle->ike2_sa2_db.ike_flink)
        {
            /* If the soft lifetime is applicable. */
            if(handle->ike2_ips_lifetime.ipsec_expiry_action != 0)
            {
                /* Set soft lifetime in SA. */
                NU_BLOCK_COPY(&ips_in.ipsec_soft_lifetime,
                    &handle->ike2_ips_lifetime, sizeof(IPSEC_SA_LIFETIME));

                /* Decrement soft lifetime offset. */
                ips_in.ipsec_soft_lifetime.ipsec_no_of_secs -=
                    IKE_SOFT_LIFETIME_OFFSET;
            }
        }

        else
        {
            /* Zero out the soft lifetime. */
            UTL_Zero(&ips_in.ipsec_soft_lifetime, sizeof(IPSEC_SA_LIFETIME));
        }

        /*
         * Generate the IPsec outbound SA.
         */

        /* Log debug message. */
        IKE2_DEBUG_LOG("Establishing IPsec outbound SA");

        /* Set the SPI. */
        ips_out.ipsec_remote_spi = sa2->ike_remote_spi;

        /* Copy the SA selector. */
        NU_BLOCK_COPY(&ips_out.ipsec_select, &handle->ike2_ips_selector,
            sizeof(IPSEC_SELECTOR));

        /* If we are the Responder. */
        if((handle->ike2_flags & IKE2_RESPONDER) != 0 ||
            (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
            IKE2_HDR_RESPONSE_FLAG) == 0)
        {
            /* Switch the selector source and destination. */
            IKE_IPS_Switch_Selector(&ips_out.ipsec_select,
                &handle->ike2_ips_selector);
        }

        /* Copy the security protocol. */
        NU_BLOCK_COPY(&ips_out.ipsec_security, &sa2->ike_ips_security,
            sizeof(IPSEC_SECURITY_PROTOCOL));

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If policy direction is simply inbound (and not outbound
         * or dual asynchronous).
         */
        if((IPSEC_POLICY_FLOW(handle->ike2_sa2_db.ike_ips_flags) &
            IPSEC_OUTBOUND) == 0)
        {
            /* Switch tunnel source and destination end points. */
            IKE_IPS_Switch_Security(&ips_out.ipsec_security,
                &sa2->ike_ips_security);
        }

        if( (sa2->ike_ips_security.ipsec_security_mode ==
             IPSEC_TUNNEL_MODE) &&
            (handle->ike2_flags & IKE2_INITIATOR) != 0 &&
            (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
             IKE2_HDR_RESPONSE_FLAG) == 0)
        {
            IKE_IPS_Switch_Selector(&ips_out.ipsec_select,
                                    &ips_in.ipsec_select);
        }
#endif

        /* Rehash existing outbound SAs which match the new
         * outbound SA being established.
         */
        if(IPSEC_Rehash_Outbound_SAs(handle->ike2_ips_group,
            &ips_out.ipsec_select,
            &ips_out.ipsec_security) == NU_SUCCESS)
        {
            IKE2_DEBUG_LOG("At least one IPsec SA was re-hashed");
        }

        /*
         * Add both IPsec SAs to the SADB. Check to see if ESN is to be
         * used by IPsec. If negotiated, set IPsec SAs with ESN enabled.
         */

        if((handle->ike2_sa->ike2_flags & IKE2_USE_IPS_ESN) == 0)
        {
            /* ESN use not required. */
            status = IPSEC_Add_SA_Pair(
                        handle->ike2_params->ike2_packet->ike_if_index,
                        &ips_out, &ips_in);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to add SA pair to IPsec SADB",
                    NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Abort on failure. */
                break;
            }

        }

        else
        {
            /* ESN to be used. */
            status = IPSEC_Add_Inbound_ESN_SA(handle->ike2_ips_group,
                                              &ips_in, &ret_spi);

            if(status == NU_SUCCESS)
            {
                status = IPSEC_Add_Outbound_ESN_SA(handle->ike2_ips_group,
                                                   &ips_out, &ret_spi);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Unable to add IPsec outbound SA with ESN",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to add IPsec inbound SA with ESN",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

        }

        /* We need to keep track of which IPsec SAs are negotiated under
         * this IKE SA. We need to delete all child SAs when we delete an
         * IKE SA or move them to new IKE SA in case it is re-keyed.
         */
        if(status == NU_SUCCESS)
        {
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&ips_index,
                                        sizeof(IKE2_IPS_SA_INDEX),
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Zero out the newly allocated structure. */
                UTL_Zero(ips_index, sizeof(IKE2_IPS_SA_INDEX));

                /* Set the SPI and IPsec protocol for the SA that we
                 * just added. We will use it to delete IPsec SAs when
                 * we delete IKE SA.
                 */
                ips_index->ips_spi = ips_out.ipsec_remote_spi;
                ips_index->protocol = sa2->ike_ips_security.ipsec_protocol;

                /* Add the SPI for this SA pair into the indices DB with
                 * the current IKEv2 SA.
                 */
                SLL_Enqueue(&handle->ike2_sa->ips_sa_index, ips_index);
            }
        }

        /* Move to the next SA2 item. */
        sa2 = sa2->ike_flink;
    }

    return (status);

} /* IKE2_IPS_Generate_SA_Pair */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_IPS_ID_To_Selector
*
* DESCRIPTION
*
*       This is a utility function which converts an IKEv2
*       Identification payload to an equivalent IPsec
*       Selector. If the Identification payload's type
*       is IKE_WILDCARD, then an absolute IP address is used
*       instead. IKE_DOMAIN_NAME and IKE_USER_DOMAIN_NAME
*       address types are not allowed in the Identification
*       payload passed to this function.
*
* INPUTS
*
*       *id                     Identification payload.
*       *abs_addr               Pointer to an IP address.
*                               Used if ID type is IKE_WILDCARD.
*       *select                 On return, this contains the
*                               equivalent IPsec selector.
*       side                    If IKE_LOCAL, then the source
*                               parameters of the Selector
*                               are filled in and if IKE_REMOTE,
*                               then the destination
*                               parameters are filled in.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identification type not
*                               supported by IKE.
*
*************************************************************************/
STATUS IKE2_IPS_ID_To_Selector(IKE2_ID_PAYLOAD *id,
                               struct addr_struct *abs_addr,
                               IPSEC_SELECTOR *selector, UINT8 side)
{
    STATUS          status = NU_SUCCESS;
    UINT8           *type;
    UINT8           *ip_addr1;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((id == NU_NULL) || (abs_addr == NU_NULL) || (selector == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the 'side' parameter is valid. */
    else if((side != IKE_LOCAL) && (side != IKE_REMOTE))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If the source side is to be filled. */
    if(side == IKE_LOCAL)
    {
        /* Point all variables to the source side. */
        type     = &selector->ipsec_source_type;
        ip_addr1 = selector->ipsec_source_ip.ipsec_addr;
    }

    /* Otherwise the destination is to be filled. */
    else
    {
        /* Point all variables to the destination side. */
        type     = &selector->ipsec_dest_type;
        ip_addr1 = selector->ipsec_dest_ip.ipsec_addr;
    }

    /* Determine Identification payload type. */
    switch(id->ike2_id_type)
    {
    case IKE2_ID_TYPE_IPV4_ADDR:
        /* Set the address type and copy the IP address. */
        *type = IPSEC_IPV4 | IPSEC_SINGLE_IP;
        NU_BLOCK_COPY(ip_addr1, id->ike2_id_data, IP_ADDR_LEN);
        break;

    case IKE2_ID_TYPE_IPV6_ADDR:
        /* Set the address type and copy the IP address. */
        *type = IPSEC_IPV6 | IPSEC_SINGLE_IP;

        /* Address length macro not used because it might be disabled
         * if IPv6 is disabled. */
        NU_BLOCK_COPY(ip_addr1, id->ike2_id_data, 16 /*IP6_ADDR_LEN*/);
        break;

    default:
        /* Set type of IP address. */
        *type = (UINT8)(IPSEC_SINGLE_IP |
            IKE_IPS_FAMILY_TO_FLAGS(abs_addr->family));

        /* Copy the IP address. */
        NU_BLOCK_COPY(ip_addr1, abs_addr->id.is_ip_addrs,
            IKE_IP_LEN(abs_addr->family));
        break;

    }

    /* Return the status. */
    return (status);

} /* IKE2_IPS_ID_To_Selector */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_IPS_Policy_By_ID
*
* DESCRIPTION
*
*       This is a utility function used to look up an
*       IPsec policy using the Identification payloads
*       received by the Responder. This function must
*       only be called by the Responder.
*
* INPUTS
*
*       *handle                 Pointer to the Exchange Handle.
*       *id_i                   Initiator's ID payload.
*       *id_r                   Responder's ID payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         In case policy is not found.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_IDTYPE  Identification type not
*                               supported by IKE.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*
*************************************************************************/
STATUS IKE2_IPS_Policy_By_ID(IKE2_EXCHANGE_HANDLE *handle,
                             IKE2_ID_PAYLOAD *id_i,
                             IKE2_ID_PAYLOAD *id_r)
{
    STATUS          status;
    UINT32          buffer_len;
    IKE2_PACKET      *pkt;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((handle == NU_NULL) || (id_i == NU_NULL) || (id_r == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Looking up IPsec policy by selector");

    /* Set local pointer to commonly used data in the Handle. */
    pkt = handle->ike2_params->ike2_packet;

    /* Set the Initiator's (source) payload data in Selector. */
    status = IKE2_IPS_ID_To_Selector(id_i, &(handle->ike2_sa->ike_node_addr),
        &(handle->ike2_ips_selector), IKE_LOCAL);

    if(status == NU_SUCCESS)
    {
        /* Set the Responder's (destination) payload data in Selector. */
        status = IKE2_IPS_ID_To_Selector(id_r, &pkt->ike_local_addr,
            &handle->ike2_ips_selector, IKE_REMOTE);

        if(status == NU_SUCCESS)
        {
            buffer_len = sizeof(handle->ike2_ips_group);

            /* Get the IPsec group name. */
            status = IKE_IPS_Group_Name_By_Packet(pkt,
                handle->ike2_ips_group, &buffer_len);

            if(status == NU_SUCCESS)
            {
                /* Get the IPsec Policy using the IPsec Selector. */
                status = IPSEC_Get_Policy_Index(handle->ike2_ips_group,
                    &(handle->ike2_ips_selector), IPSEC_INBOUND,
                    &(handle->ike2_ips_policy_index));
            }

            else
            {
                NLOG_Error_Log("Unable to get IPsec group name by packet",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to set destination side in selector",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Unable to set source side in selector",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE2_IPS_Policy_By_ID */

#endif

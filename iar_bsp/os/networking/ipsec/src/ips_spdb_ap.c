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
*       ips_spdb_ap.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Add_Policy along with other utility functions.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Add_Policy
*       IPSEC_Convert_Subnet6
*       IPSEC_Normalize_Security
*       IPSEC_Validate_Addr_Type
*       IPSEC_Validate_Selector
*       IPSEC_Validate_Auth_Algo
*       IPSEC_Validate_Encrypt_Algo
*       IPSEC_Validate_Sec_Prot
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*       ike_api.h
*       ike_evt.h
*
*************************************************************************/
/* Including the required header files. */
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
#include "networking/ike_api.h"
#include "networking/ike_evt.h"
#endif

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_Policy
*
* DESCRIPTION
*
*       This function adds a new policy to the list of policies.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       *policy                 Pointer to the policy to be added.
*       *return_index           Pointer to the callers index. On a
*                               successful request, the uniquely assigned
*                               index is returned.
*
* OUTPUTS
*
*       NU_SUCCESS              Policy was successfully added.
*       NU_TIMEOUT              The operation timed out
*       NU_NO_MEMORY            Memory not available.
*       NU_INVALID_PARM         If policy already exists.
*       IPSEC_NOT_FOUND         The group was not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Policy(CHAR *group_name, IPSEC_POLICY *policy,
                        UINT32 *return_index)
{
    STATUS              status = NU_SUCCESS;
    UINT16              mem_size;
    IPSEC_POLICY_GROUP  *group;
    IPSEC_POLICY        *policy_ptr;
    UINT8               policy_action;
    UINT8               policy_flow;
    UINT8               i;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();


    /* Validate input parameters. */
    if((group_name == NU_NULL) || (policy == NU_NULL) ||
        (return_index == NU_NULL))
    {

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (IPSEC_INVALID_PARAMS);
    }

    /* Get the policy action. */
    policy_action = IPSEC_POLICY_ACTION(policy->ipsec_flags);

    /* Get the policy flow. */
    policy_flow = IPSEC_POLICY_FLOW(policy->ipsec_flags);

    /* Verify the flags. */
    if(policy->ipsec_flags == NU_NULL)
        status = IPSEC_INVALID_PARAMS;

    /* Make sure that the flags are correct. */
    else if((policy_action != IPSEC_DISCARD) &&
            (policy_action != IPSEC_BY_PASS) &&
            (policy_action != IPSEC_APPLY))

        status = IPSEC_INVALID_PARAMS;

    /* Make sure that the flags are correct. */
    else if((policy_flow != IPSEC_INBOUND) &&
            (policy_flow != IPSEC_OUTBOUND)&&
            (policy_flow != IPSEC_DUAL_ASYNCHRONOUS))

        status = IPSEC_INVALID_PARAMS;

    /* Passed security size should must be less than or equal to
       'IPSEC_MAX_SA_BUNDLE_SIZE' value. */
    else if(policy->ipsec_security_size > IPSEC_MAX_SA_BUNDLE_SIZE)

        status = IPSEC_INVALID_PARAMS;

#if (INCLUDE_IKE == NU_TRUE)
    /* Lifetime associated with bundle list must not be zero. */
    else if((policy->ipsec_security_size != 0) &&
            (policy->ipsec_bundles.ipsec_lifetime == 0))

        status = IPSEC_INVALID_PARAMS;
#endif

    /* Check if policy action specifies 'BY PASS' action but there is
       still some securities present. */
    else if((policy_action == IPSEC_BY_PASS) &&
                                (policy->ipsec_security_size != 0))

        status = IPSEC_INVALID_PARAMS;

#if (INCLUDE_IPV6 == NU_TRUE)

    /* Verify and convert IPv6 prefix length to the subnet mask. */
    else if(policy->ipsec_select.ipsec_source_type ==
                                        (IPSEC_SUBNET_IP | IPSEC_IPV6))
    {
        if(IPSEC_Convert_Subnet6(policy->ipsec_select.ipsec_source_ip.
                                 ipsec_ext_addr.ipsec_addr2) != NU_SUCCESS)
            status = IPSEC_INVALID_PARAMS;
    }

    /* Verify and convert IPv6 prefix length to the subnet mask. */
    if((policy->ipsec_select.ipsec_dest_type ==
                   (IPSEC_SUBNET_IP | IPSEC_IPV6)) &&
                   (status != IPSEC_INVALID_PARAMS))
    {
        if(IPSEC_Convert_Subnet6(policy->ipsec_select.ipsec_dest_ip.
                                 ipsec_ext_addr.ipsec_addr2) != NU_SUCCESS)

            status = IPSEC_INVALID_PARAMS;
    }

#endif

    if(status == NU_SUCCESS)
    {
        if(policy->ipsec_security_size != 0)
        {
            /* Validate the security protocol. */
            status = IPSEC_Validate_Sec_Prot(policy->ipsec_security);

            if(status == NU_SUCCESS)
            {
                /* Validate the selector protocol. */
                status = IPSEC_Validate_Selector(&policy->ipsec_select);

#if (INCLUDE_IKE == NU_TRUE)

                if(status == NU_SUCCESS)
                {
                    /* Validate the PFS group. */
                    if((policy->ipsec_pfs_group_desc != IKE_GROUP_NONE) &&
                       (IKE_Oakley_Group_Prime(
                             policy->ipsec_pfs_group_desc) == NU_NULL))
                    {
                        status = IPSEC_INVALID_PARAMS;
                    }
                    else
                    {
                        /* Validate the lifetime. */
                        if((policy->ipsec_sa_max_lifetime.
                            ipsec_expiry_action != IPSEC_REFRESH_SA) &&
                            (policy->ipsec_sa_max_lifetime.
                                ipsec_expiry_action != IPSEC_LOG_WARNING) &&
                            (policy->ipsec_sa_max_lifetime.ipsec_no_of_secs > 0 &&
                            policy->ipsec_sa_max_lifetime.ipsec_no_of_secs < 2*IKE_SOFT_LIFETIME_OFFSET))
                        {
                            status = IPSEC_INVALID_PARAMS;
                        }
                    }
                }
#endif

            }
        }
    }

    /* Check the status first. */
    if(status == NU_SUCCESS)
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status value. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* First get the group pointer. */
            status = IPSEC_Get_Group_Entry(group_name, &group);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* Calculate the total memory size. */
                mem_size = sizeof(IPSEC_POLICY) +
                          (sizeof(IPSEC_SECURITY_PROTOCOL) *
                           policy->ipsec_security_size);

                /* Allocate the memory now. */
                status = NU_Allocate_Memory(IPSEC_Memory_Pool,
                        (VOID **)&policy_ptr, mem_size, NU_NO_SUSPEND);

                /* Check the status value. */
                if(status == NU_SUCCESS)
                {
                    /* First normalize the pointer. */
                    policy_ptr = TLS_Normalize_Ptr(policy_ptr);

                    /* Now copy the policy passed. */
                    NU_BLOCK_COPY(policy_ptr, policy,
                                  sizeof(IPSEC_POLICY));
                    /* Initialize internal policy parameters. */
                    policy_ptr->ipsec_flink = NU_NULL;
                    policy_ptr->ipsec_index = NU_NULL;
                    policy_ptr->ipsec_bundles.ipsec_head = NU_NULL;
                    policy_ptr->ipsec_bundles.ipsec_tail = NU_NULL;

#if (INCLUDE_IKE == NU_TRUE)

                    policy_ptr->ipsec_sa_max_lifetime.ipsec_in_sa =
                                                                NU_NULL;
                    policy_ptr->ipsec_sa_max_lifetime.ipsec_out_sa =
                                                                NU_NULL;
                    policy_ptr->ipsec_sa_max_lifetime.ipsec_flags = 0;

#endif
                    /* Increment the next policy index and assign the
                       unique identifier to the policy index. */
                    policy_ptr->ipsec_index =
                                ++(group->ipsec_policy_list.
                                ipsec_next_policy_index);

                    /* Assigning ipsec security structure memory. */
                    policy_ptr->ipsec_security =
                                            (IPSEC_SECURITY_PROTOCOL*)
                        (((UINT8 *)policy_ptr) + sizeof(IPSEC_POLICY));

                    /* Copy the passed ipsec security structure to the
                       just created policy structure's security member. */
                    NU_BLOCK_COPY(policy_ptr->ipsec_security,
                                      policy->ipsec_security,
                                     (policy->ipsec_security_size *
                                      sizeof(IPSEC_SECURITY_PROTOCOL)));

                    /* Loop for all security policies. */
                    for(i = 0; i < policy_ptr->ipsec_security_size; i++)
                    {
                        /* Normalize this security protocol. */
                        IPSEC_Normalize_Security(&(policy_ptr->
                                                    ipsec_security[i]));
                    }

                    /* Update policy selector's fields if incoming user request
                     * is for ICMP protocol requiring Wildcard support. Update
                     * the fields to include all possible ICMP message types
                     * and codes.
                     *
                     * If Wildcard value is given than the ICMP message
                     * type value must be zero as Wildcard is defined as zero.
                     */
                    if(((policy_ptr->ipsec_select.ipsec_transport_protocol == IP_ICMP_PROT)
#if (INCLUDE_IPV6 == NU_TRUE)
                     || (policy_ptr->ipsec_select.ipsec_transport_protocol == IP_ICMPV6_PROT)
#endif
                            ) &&
                       ((policy_ptr->ipsec_select.ipsec_icmp_msg == IPSEC_WILDCARD) &&
                        (policy_ptr->ipsec_select.ipsec_icmp_msg_high == IPSEC_WILDCARD) &&
                        (policy_ptr->ipsec_select.ipsec_icmp_code == IPSEC_WILDCARD) &&
                        (policy_ptr->ipsec_select.ipsec_icmp_code_high == IPSEC_WILDCARD))
                        )
                    {
                        /* Update the ICMP message type and code values to include all possible
                         * values i.e. From 0 to 255. This will enable all types of
                         * ICMP messages to pass through IPSec policy check.
                         */
                        policy_ptr->ipsec_select.ipsec_icmp_msg = 0;
                        policy_ptr->ipsec_select.ipsec_icmp_msg_high = 255;

                        policy_ptr->ipsec_select.ipsec_icmp_code = 0;
                        policy_ptr->ipsec_select.ipsec_icmp_code_high = 255;
                    }

                    /* Add this newly created policy to the list of
                     * policies of the passed group.
                     */
#if ( IPSEC_ENABLE_PRIORITY == NU_FALSE )
                    SLL_Enqueue(&(group->ipsec_policy_list), policy_ptr);
#else
                    IPSEC_Priority_Insert ( &(group->ipsec_policy_list),
                                            policy_ptr );
#endif

                    /* Return the policy index. */
                    *return_index =  policy_ptr->ipsec_index;
                }
            }

            /* Now everything is done, release the semaphore too. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release IPsec semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Add_Policy */

#if (INCLUDE_IPV6 == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IPSEC_Convert_Subnet6
*
* DESCRIPTION
*
*       This function converts an IPv6 prefix length to a subnet
*       mask. The prefix must be from 0 to 128 bits.
*
* INPUTS
*
*       *subnet6                Pointer to a buffer where the IPv6 subnet
*                               is to be stored. The first byte of this
*                               buffer contains the prefix length.
*
* OUTPUTS
*
*       NU_SUCCESS              Subnet is a valid prefix.
*       IPSEC_INVALID_PARAMS    Subnet is invalid.
*
*************************************************************************/
STATUS IPSEC_Convert_Subnet6(UINT8 *subnet6)
{
    STATUS              status;
    UINT8               prefix_len;
    INT                 i;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(subnet6 == NU_NULL)
        return (IPSEC_INVALID_PARAMS);

#endif

    /* Get the prefix length from the subnet buffer. */
    prefix_len = *subnet6;

    /* Make sure the prefix length is valid. */
    if((prefix_len == 0) ||
       (prefix_len > (IP6_ADDR_LEN * IPSEC_BITS_PER_BYTE)))
    {
        status = IPSEC_INVALID_PARAMS;
    }
    else
    {
        /* Set status to success. */
        status = NU_SUCCESS;

        /* Loop for all bytes of the subnet. */
        for(i = 0; i < IP6_ADDR_LEN; i++)
        {
            /* If all bits are set in the current byte. */
            if(prefix_len >= IPSEC_BITS_PER_BYTE)
            {
                /* Set all bits in the current byte. */
                subnet6[i] = 0xff;
            }
            else
            {
                /* Set remaining number of bits in the current byte. */
                subnet6[i] =
                    (UINT8)(0xff << (IPSEC_BITS_PER_BYTE - prefix_len));

                /* Complete prefix set so break out of loop. */
                break;
            }

            /* Update remaining number of bits. */
            prefix_len = prefix_len - IPSEC_BITS_PER_BYTE;
        }

        /* Initialize the remaining bytes to zero. */
        for(; i < IP6_ADDR_LEN; i++)
        {
            subnet6[i] = 0;
        }
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Convert_Subnet6 */
#endif

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Normalize_Security
*
* DESCRIPTION
*
*       The IPsec security protocol structure needs to be compared
*       using memcmp(). Such comparisons require all unused structure
*       fields to be set to normalized values. This function normalizes
*       all unused fields of the security protocol.
*
* INPUTS
*
*       *security               Pointer to the security protocol.
*
* OUTPUTS
*
*       NU_SUCCESS              Structure normalized successfully.
*       IPSEC_INVALID_PARAMS    Security protocol is invalid.
*
************************************************************************/
STATUS IPSEC_Normalize_Security(IPSEC_SECURITY_PROTOCOL *security)
{
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    UINT8           addr_len;
#endif

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(security == NU_NULL)
        return (IPSEC_INVALID_PARAMS);

#endif

    /* Zero out structure padding. */
    UTL_Zero(security->ipsec_pad1, sizeof(security->ipsec_pad1));
    UTL_Zero(security->ipsec_pad2, sizeof(security->ipsec_pad2));

#if (IPSEC_INCLUDE_TRANSPORT_MODE == NU_TRUE)
    /* If this is a transport mode security. */
    if(security->ipsec_security_mode == IPSEC_TRANSPORT_MODE)
    {
        /* Zero out the tunnel source and destination. */
        UTL_Zero(security->ipsec_tunnel_destination, MAX_ADDRESS_SIZE);
        UTL_Zero(security->ipsec_tunnel_source, MAX_ADDRESS_SIZE);

    }
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    else
#endif
#endif

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
    {
        /* Determine the tunnel IP address length. */
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
        if((security->ipsec_flags & IPSEC_IPV4) != 0)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        {
            addr_len = IP_ADDR_LEN;
        }
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
        else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        {
            addr_len = IP6_ADDR_LEN;
        }
#endif

        /* Zero out unused bytes of tunnel source and destination. */
        UTL_Zero(security->ipsec_tunnel_destination + addr_len,
                 MAX_ADDRESS_SIZE - addr_len);
        UTL_Zero(security->ipsec_tunnel_source + addr_len,
                 MAX_ADDRESS_SIZE - addr_len);

        /* Set unused bits of the flags to zero. */
        security->ipsec_flags &= (IPSEC_IPV4 | IPSEC_IPV6);
    }
#endif

    /* Return the status. */
    return (NU_SUCCESS);

} /* IPSEC_Normalize_Security */

/*************************************************************************
*
* FUNCTION
*
*       IPSEC_Validate_Addr_Type
*
* DESCRIPTION
*
*       This function is used to validate the type of IP addresses
*       specified in the selector source and destination type field.
*
* INPUTS
*
*       type                    IPsec selector source and destination
*                               address type.
*
* OUTPUTS
*
*       NU_SUCCESS              Type is valid.
*       IPSEC_INVALID_PARAMS    Type is invalid.
*
*************************************************************************/
STATIC STATUS IPSEC_Validate_Addr_Type(UINT8 type)
{
    STATUS              status = NU_SUCCESS;

    /* Validating IP version and category. */
    if((type & IPSEC_IPV6) && (type & IPSEC_IPV4))
    {
        status = IPSEC_INVALID_PARAMS;
    }

    else if((type & IPSEC_SINGLE_IP) && (type & IPSEC_RANGE_IP))
    {
        status = IPSEC_INVALID_PARAMS;
    }

    else if((type & IPSEC_SINGLE_IP) && (type & IPSEC_SUBNET_IP))
    {
        status = IPSEC_INVALID_PARAMS;
    }

    else if((type & IPSEC_SUBNET_IP) && (type & IPSEC_RANGE_IP))
    {
        status = IPSEC_INVALID_PARAMS;
    }

    return (status);

} /* IPSEC_Validate_Addr_Type */

/*************************************************************************
*
* FUNCTION
*
*       IPSEC_Validate_Selector
*
* DESCRIPTION
*
*       This function is used to validate the selector specified in the
*       IPsec policy, Outbound SA, and Inbound SA.
*
* INPUTS
*
*       selector                IPsec selector source and destination
*                               address type.
*
* OUTPUTS
*
*       NU_SUCCESS              Selector is valid.
*       IPSEC_INVALID_PARAMS    Selector is invalid.
*
*************************************************************************/
STATUS IPSEC_Validate_Selector(IPSEC_SELECTOR *selector)
{
    STATUS              status;

    /* Validate source address type. */
    status = IPSEC_Validate_Addr_Type(selector->ipsec_source_type);

    if(status == NU_SUCCESS)
    {
        /* Validate destination address type. */
        status = IPSEC_Validate_Addr_Type(selector->ipsec_dest_type);

        if(status == NU_SUCCESS)
        {
            /* Validate transport protocol. */
            if((selector->ipsec_transport_protocol != IP_ICMP_PROT) &&
                (selector->ipsec_transport_protocol != IP_TCP_PROT) &&
                (selector->ipsec_transport_protocol != IP_UDP_PROT) &&
                (selector->ipsec_transport_protocol != IP_IGMP_PROT) &&
                (selector->ipsec_transport_protocol != IPSEC_WILDCARD)

#if (INCLUDE_IPV6 == NU_TRUE)

                && (selector->ipsec_transport_protocol != IP_ICMPV6_PROT)

#endif

                )
            {
                status = IPSEC_INVALID_PARAMS;
            }
        }
    }

    return (status);

} /* IPSEC_Validate_Selector */

/*************************************************************************
*
* FUNCTION
*
*       IPSEC_Validate_Auth_Algo
*
* DESCRIPTION
*
*       This function is used to validate the Authentication algorithm
*       specified in the IPsec security protocol.
*
* INPUTS
*
*       auth_algo               Authentication algorithm index.
*
* OUTPUTS
*
*       NU_SUCCESS              Index is valid.
*       IPSEC_INVALID_PARAMS    Index is invalid.
*
*************************************************************************/
STATIC STATUS IPSEC_Validate_Auth_Algo(UINT8 auth_algo)
{
    STATUS              status = IPSEC_INVALID_PARAMS;
    UINT8               i;

    /* Validate authentication algorithms. */
    for(i=0; i < IPSEC_TOTAL_AUTH_ALGO; i++)
    {
        if(auth_algo ==
                    IPSEC_Authentication_Algos[i].ipsec_algo_identifier)
        {
            status = NU_SUCCESS;
            break;
        }
    }


    return (status);

} /* IPSEC_Validate_Auth_Algo */

/*************************************************************************
*
* FUNCTION
*
*       IPSEC_Validate_Encrypt_Algo
*
* DESCRIPTION
*
*       This function is used to validate the Encryption algorithm
*       specified in the IPsec security protocol.
*
* INPUTS
*
*       enc_algo                Encryption algorithm index.
*
* OUTPUTS
*
*       NU_SUCCESS              Index is valid.
*       IPSEC_INVALID_PARAMS    Index is invalid.
*
*************************************************************************/
STATIC STATUS IPSEC_Validate_Encrypt_Algo(UINT8 enc_algo)
{

    STATUS              status = IPSEC_INVALID_PARAMS;
    UINT8               i;

    /* Validate encryption algorithm. */
    for(i = 0; i < IPSEC_TOTAL_ENCRYPT_ALGO; i++)
    {
        if(enc_algo == IPSEC_Encryption_Algos[i].ipsec_algo_identifier)
        {
            status = NU_SUCCESS;
            break;
        }
    }

    return (status);

} /* IPSEC_Validate_Encrypt_Algo */

/*************************************************************************
*
* FUNCTION
*
*       IPSEC_Validate_Sec_Prot
*
* DESCRIPTION
*
*       This function is used to validate the IPsec security protocol
*       specified in the IPsec policy, Outbound SA, and Inbound SA.
*
* INPUTS
*
*       *sec_prot               IPsec Security Protocol.
*
* OUTPUTS
*
*       NU_SUCCESS              Security Protocol is valid.
*       IPSEC_INVALID_PARAMS    Security Protocol is invalid.
*
*************************************************************************/
STATUS IPSEC_Validate_Sec_Prot(IPSEC_SECURITY_PROTOCOL *sec_prot)
{
    STATUS              status;

    /* Validate input parameters. */
    if(sec_prot == NU_NULL)
        status = IPSEC_INVALID_PARAMS;
    else
    {
        /* Validate IPsec protocol. */
        if((sec_prot->ipsec_protocol != IPSEC_AH) &&
                                (sec_prot->ipsec_protocol != IPSEC_ESP))
            status = IPSEC_INVALID_PARAMS;
        else
        {
            /* Validate authentication algorithm. */
            status = IPSEC_Validate_Auth_Algo(sec_prot->ipsec_auth_algo);

            if(status == NU_SUCCESS)
            {
                /* Validate encryption algorithm. */
                if(sec_prot->ipsec_protocol == IPSEC_ESP)
                {
                    status = IPSEC_Validate_Encrypt_Algo(
                        sec_prot->ipsec_encryption_algo);
                }

#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
                else
                {
                    /* If IPsec protocol AH specified then authentication
                     * algorithm cant be IPSEC_NULL_AUTH. */
                    if(sec_prot->ipsec_auth_algo == IPSEC_NULL_AUTH)
                        status = IPSEC_INVALID_PARAMS;
                }
#endif
            }

            if(status == NU_SUCCESS)
            {
                /* Validate the security mode. */
                if((sec_prot->ipsec_security_mode !=
                                                IPSEC_TRANSPORT_MODE) &&
                    (sec_prot->ipsec_security_mode != IPSEC_TUNNEL_MODE))
                {
                    status = IPSEC_INVALID_PARAMS;
                }
            }
        }
    }

    return (status);

} /* IPSEC_Validate_Sec_Prot */


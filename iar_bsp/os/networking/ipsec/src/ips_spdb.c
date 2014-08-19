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
*       ips_spdb.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       Implementation of IPsec security policy database.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Policy_Entry
*       IPSEC_Get_Policy_By_Selector
*       IPSEC_Match_Sel_Addr_IPs
*       IPSEC_Remove_Policy_Real
*       IPSEC_Get_Bundle_By_Selector
*       IPSEC_Add_Outbound_Bundle
*       IPSEC_Match_Selectors
*       IPSEC_Match_Policy_In
*       IPSEC_Match_Policy_Out
*       IPSEC_Get_Bundle_SA_Entries
*       IPSEC_IKE_SA_Request
*       IPSEC_Int_Ptr_Sort
*       IPSEC_Verify_Policy
*       IPSEC_Get_Policy_Index_Narrow
*       IPSEC_Get_Policy_By_Selector_Narrow
*       IPSEC_Match_Selectors_Narrow
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
#include "networking/ike_ips.h"
#endif

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Get_Policy_Entry
*
* DESCRIPTION
*
*       This function returns the pointer to the required policy.
*
* INPUTS
*
*       *group_ptr              Pointer to IPsec policy group.
*       policy_index            Index of the policy whose pointer is
*                               needed.
*       **policy_ptr            Pointer to the policy returned after the
*                               successful request.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       IPSEC_NOT_FOUND         If policy is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Policy_Entry(IPSEC_POLICY_GROUP *group_ptr,
                              UINT32 policy_index,
                              IPSEC_POLICY **policy_ptr)
{
    STATUS              status = IPSEC_NOT_FOUND;
    IPSEC_POLICY        *policy_list;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_ptr == NU_NULL) || (policy_ptr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Find the policy corresponding to the passed policy index by
       traversing the IPsec SPDB. */
    for(policy_list = group_ptr->ipsec_policy_list.ipsec_head;
        policy_list != NU_NULL;
        policy_list = policy_list->ipsec_flink)
    {
        /* Compare the two indices. */
        if(policy_list->ipsec_index == policy_index)
        {
            /* Return the pointer of the policy to the user. */
            *policy_ptr = policy_list;

            /* Policy has been found. */
            status = NU_SUCCESS;

            break;
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Policy_Entry */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Policy_By_Selector
*
* DESCRIPTION
*
*       Get the first policy matched by the given the selector.
*
* INPUTS
*
*       *group                  Pointer to the group.
*       *policy_ptr             Starting searching point.
*       *pkt_selector           Packet selector passed.
*       policy_type             Direction of policy. Valid values are
*                               IPSEC_OUTBOUND and IPSEC_INBOUND.
*       **ret_policy_ptr        Policy to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       IPSEC_NOT_FOUND         In case policy is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Policy_By_Selector(IPSEC_POLICY_GROUP *group,
                                    IPSEC_POLICY *policy_ptr,
                                    IPSEC_SELECTOR *pkt_selector,
                                    UINT8 policy_type,
                                    IPSEC_POLICY **ret_policy_ptr)
{
    STATUS              status = IPSEC_NOT_FOUND;
    IPSEC_POLICY        *policy_start;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group        == NU_NULL) || (pkt_selector   == NU_NULL) ||
                                    (ret_policy_ptr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* If policy pointer is null then we have to search from the start
       of the policy list. */
    if(policy_ptr == NU_NULL)
    {
        /* Search from first policy in the group. */
        policy_start = group->ipsec_policy_list.ipsec_head;
    }
    else
    {
        /* Starting point for policy searching is given. */
        policy_start = policy_ptr;
    }

    /* Find the required policy. */
    for(;policy_start != NU_NULL;
        policy_start = policy_start->ipsec_flink)
    {
        /* If the policy supports the desired direction. */
        if((IPSEC_POLICY_FLOW(policy_start->ipsec_flags) & policy_type)
                                                                    != 0)
        {
            /* Compare the two selectors. */
            if(IPSEC_Match_Selectors(&(policy_start->ipsec_select),
                                    pkt_selector,
                        ((IPSEC_POLICY_FLOW(policy_start->ipsec_flags) ==
                          IPSEC_DUAL_ASYNCHRONOUS) &&
                         (policy_type & IPSEC_INBOUND) ?
                          NU_TRUE : NU_FALSE)) == NU_TRUE)
            {
                /* Now return policy whose selector has been matched. */
                *ret_policy_ptr = policy_start;

                /* Successful operation. */
                status = NU_SUCCESS;
                break;
            }
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Policy_By_Selector */

/************************************************************************
* FUNCTION
*
*       IPSEC_Match_Sel_Addr_IPs
*
* DESCRIPTION
*
*       This function takes in as input two pairs of type and address
*       and matches them.
*
* INPUTS
*
*       policy_sel_type         Policy selector source type.
*       *policy_sel_src         Pointer to policy selector source.
*       pkt_sel_type            Packet selector source type.
*       *pkt_sel_src            Pointer to packet selector source.
*
* OUTPUTS
*
*       NU_TRUE                 If policy_sel_src != pkt_sel_src
*       NU_FALSE                If policy_sel_src == pkt_sel_src
*
*************************************************************************/
UINT8 IPSEC_Match_Sel_Addr_IPs(UINT8 policy_sel_type,
                               VOID *policy_sel_src,
                               UINT8 pkt_sel_type, VOID *pkt_sel_src)
{
    UINT8               result = NU_FALSE;
    UINT8               *policy_ip;
    UINT8               *policy_ip_range;
    UINT8               *pkt_ip;

#if ((INCLUDE_IKE == NU_TRUE) && (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE))
    UINT8               *pkt_ip_range;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32              policy_subnet;
    UINT32              pkt_subnet;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8               i;
    UINT8               policy_subnet6[IP6_ADDR_LEN];
    UINT8               pkt_subnet6[IP6_ADDR_LEN];
#endif

#if ((INCLUDE_IKE == NU_FALSE) || (IPSEC_INCLUDE_TUNNEL_MODE == NU_FALSE))
    UNUSED_PARAMETER(pkt_sel_type);
#endif

    /* Set pointers to IP addresses and ranges. */
    policy_ip       = ((IPSEC_IP_ADDR*)policy_sel_src)->ipsec_addr;
    policy_ip_range = ((IPSEC_IP_ADDR*)policy_sel_src)->
                                        ipsec_ext_addr.ipsec_addr2;

    /* Set pointer to the packet IP address. */
    pkt_ip          = ((IPSEC_IP_ADDR*)pkt_sel_src)->ipsec_addr;

#if ((INCLUDE_IKE == NU_TRUE) && (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE))
    pkt_ip_range    = ((IPSEC_IP_ADDR*)pkt_sel_src)->
                                        ipsec_ext_addr.ipsec_addr2;
#endif

     /* If tunnel mode is disabled or IKE is not being used then
     * the packet selector will always be a single IP address.
     */
#if ((IPSEC_INCLUDE_TUNNEL_MODE == NU_FALSE) || (INCLUDE_IKE == NU_FALSE))
    /* First decode the policy selector's type. */
    switch((INT)policy_sel_type)
    {

#if (INCLUDE_IPV4 == NU_TRUE)

    /* If it is single IP. */
    case(IPSEC_SINGLE_IP | IPSEC_IPV4):
    {
        /* Now compare the two IP addresses. */
        if(IP_ADDR(policy_ip) == IP_ADDR(pkt_ip))
        {
            result = NU_TRUE;
        }
        break;
    }

    /* Compare range type. */
    case(IPSEC_RANGE_IP | IPSEC_IPV4):
    {
        /* Check if packet IP lies within policy range. */
        if((IP_ADDR(pkt_ip) >= IP_ADDR(policy_ip)) &&
           (IP_ADDR(pkt_ip) <= IP_ADDR(policy_ip_range)))
        {
            result = NU_TRUE;
        }
        break;
    }

    /* Compare subnet type. */
    case(IPSEC_SUBNET_IP | IPSEC_IPV4):
    {
        /* Calculate the subnets of the packet and the policy. */
        policy_subnet = IP_ADDR(policy_ip) & IP_ADDR(policy_ip_range);
        pkt_subnet    = IP_ADDR(pkt_ip)    & IP_ADDR(policy_ip_range);

        /* Compare the two subnets. */
        if(policy_subnet == pkt_subnet)
        {
            result = NU_TRUE;
        }
        break;
    }

#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If it is single IP. */
    case(IPSEC_SINGLE_IP | IPSEC_IPV6):
    {
        /* Match the two IP addresses. */
        if(memcmp(pkt_ip, policy_ip, IP6_ADDR_LEN) == 0)
        {
            result = NU_TRUE;
        }
        break;
    }

    /* Check if the type in the policy is of some range. */
    case(IPSEC_RANGE_IP | IPSEC_IPV6):
    {
        /* Policy is a range and packet is a single IP. */
        for(i = 0; i < IP6_ADDR_LEN; i++)
        {
            /* If packet IP completely lies within policy range. */
            if((pkt_ip[i] > policy_ip[i]) &&
               (pkt_ip[i] < policy_ip_range[i]))
            {
                result = NU_TRUE;
                break;
            }
            /* Otherwise if packet IP lies outside policy range. */
            else if((pkt_ip[i] < policy_ip[i]) ||
                    (pkt_ip[i] > policy_ip_range[i]))
            {
                /* Result has been initialized to false above. */
                break;
            }
        }

        /* If above loop didn't break, then it lies within range. */
        if(i == IP6_ADDR_LEN)
        {
            result = NU_TRUE;
        }
        break;
    }

    /* Check for the subnet type. */
    case(IPSEC_SUBNET_IP | IPSEC_IPV6):
    {
        /* Policy is a subnet and packet is a single IP. */
        for(i = 0; i < IP6_ADDR_LEN; i++)
        {
            /* Calculate the subnets of policy and packet. */
            policy_subnet6[i] = (UINT8)(policy_ip[i] & policy_ip_range[i]);
            pkt_subnet6[i]    = (UINT8)(pkt_ip[i] & policy_ip_range[i]);
        }

        /* Compare the two subnets. */
        if(memcmp(policy_subnet6, pkt_subnet6, IP6_ADDR_LEN) == 0)
        {
            result = NU_TRUE;
        }
        break;
    }

#endif  /* #if (INCLUDE_IPV6 == NU_TRUE) */

    case IPSEC_WILDCARD:
        /* Wildcards cannot be compared so return exact match. */
        result = NU_TRUE;
        break;

    } /* End of 'switch' statement. */

     /* If both tunnel mode and IKE are enabled, then IKE
     * may specify a packet selector possibly containing a
     * subnet or a range. Therefore, several cases need to
     * be handled.
     */
#else /* #if ((IPSEC_INCLUDE_TUNNEL_MODE == NU_FALSE) ||
              (INCLUDE_IKE == NU_FALSE)) */

    /* First decode the policy selector's type. */
    switch((INT)policy_sel_type)
    {

#if (INCLUDE_IPV4 == NU_TRUE)

    /* If it is single IP. */
    case(IPSEC_SINGLE_IP | IPSEC_IPV4):
    {
        /* Decode the packet selector's type. */
        switch((INT)pkt_sel_type)
        {
        case(IPSEC_SINGLE_IP | IPSEC_IPV4):
            /* Compare the two IP addresses. */
            if(IP_ADDR(policy_ip) == IP_ADDR(pkt_ip))
            {
                result = NU_TRUE;
            }
            break;

        case IPSEC_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
        break;
    }

    /* Compare range type. */
    case(IPSEC_RANGE_IP | IPSEC_IPV4):
    {
        /* Decode the packet selector's type. */
        switch((INT)pkt_sel_type)
        {
        case(IPSEC_SINGLE_IP | IPSEC_IPV4):
            /* Check if packet IP lies within policy range. */
            if((IP_ADDR(pkt_ip) >= IP_ADDR(policy_ip)) &&
               (IP_ADDR(pkt_ip) <= IP_ADDR(policy_ip_range)))
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_RANGE_IP | IPSEC_IPV4):
            /* Make sure packet range lies within policy range. */
            if((IP_ADDR(pkt_ip)       >= IP_ADDR(policy_ip)) &&
               (IP_ADDR(pkt_ip_range) <= IP_ADDR(policy_ip_range)))
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_SUBNET_IP | IPSEC_IPV4):
            /* Calculate subnets such that policy_subnet contains
             * the smallest IP and pkt_subnet contains the largest
             * under subnet of the packet.
             */
            policy_subnet = IP_ADDR(pkt_ip) & IP_ADDR(pkt_ip_range);
            pkt_subnet    = policy_subnet | ~(IP_ADDR(pkt_ip_range));

            /* Make sure packet subnet's range is a subset of
             * the range of the policy.
             */
            if((policy_subnet >= IP_ADDR(policy_ip)) &&
               (pkt_subnet    <= IP_ADDR(policy_ip_range)))
            {
                result = NU_TRUE;
            }
            break;

        case IPSEC_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
        break;
    }

    /* Compare subnet type. */
    case(IPSEC_SUBNET_IP | IPSEC_IPV4):
    {
        /* Decode the packet selector's type. */
        switch((INT)pkt_sel_type)
        {
        case(IPSEC_SINGLE_IP | IPSEC_IPV4):
            /* Calculate the subnets of the packet and the policy. */
            policy_subnet = IP_ADDR(policy_ip) & IP_ADDR(policy_ip_range);
            pkt_subnet    = IP_ADDR(pkt_ip)    & IP_ADDR(policy_ip_range);

            /* Compare the two subnets. */
            if(policy_subnet == pkt_subnet)
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_RANGE_IP | IPSEC_IPV4):
            /* Calculate subnets such that policy_subnet contains
             * the smallest IP and pkt_subnet contains the largest
             * under subnet of the policy.
             */
            policy_subnet = IP_ADDR(policy_ip) & IP_ADDR(policy_ip_range);
            pkt_subnet    = policy_subnet | ~(IP_ADDR(policy_ip_range));

            /* Make sure policy subnet's range is a superset of
             * the packet selector range.
             */
            if((policy_subnet <= IP_ADDR(pkt_ip)) &&
               (pkt_subnet    >= IP_ADDR(pkt_ip_range)))
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_SUBNET_IP | IPSEC_IPV4):
            /* Calculate the subnets of the policy and the packet. */
            policy_subnet = IP_ADDR(policy_ip) & IP_ADDR(policy_ip_range);
            pkt_subnet    = IP_ADDR(pkt_ip)    & IP_ADDR(pkt_ip_range);

            /* Bitwise AND packet subnet with policy's mask to make
             * sure packet subnet lies within policy subnet.
             */
            pkt_subnet &= IP_ADDR(policy_ip_range);

            /* Compare the two subnets. */
            if(policy_subnet == pkt_subnet)
            {
                result = NU_TRUE;
            }
            break;

        case IPSEC_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
        break;
    }

#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If it is single IP. */
    case(IPSEC_SINGLE_IP | IPSEC_IPV6):
    {
        /* Decode the packet selector's type. */
        switch((INT)pkt_sel_type)
        {
        case(IPSEC_SINGLE_IP | IPSEC_IPV6):
            /* Match the two IP addresses. */
            if(memcmp(pkt_ip, policy_ip, IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;

        case IPSEC_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
        break;
    }

    /* Check if the type in the policy is of some range. */
    case(IPSEC_RANGE_IP | IPSEC_IPV6):
    {
        /* Decode the packet selector's type. */
        switch((INT)pkt_sel_type)
        {
        case(IPSEC_SINGLE_IP | IPSEC_IPV6):
            /* 'policy_ip' is a range and 'packet_ip' is a single IP. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* If IP 'packet' completely lies within range 'policy'. */
                if((pkt_ip[i] > policy_ip[i]) &&
                   (pkt_ip[i] < policy_ip_range[i]))
                {
                    result = NU_TRUE;
                    break;
                }
                /* Otherwise if IP 'packet' lies outside range 'policy'. */
                else if((pkt_ip[i] < policy_ip[i]) ||
                        (pkt_ip[i] > policy_ip_range[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then it lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_RANGE_IP | IPSEC_IPV6):
            /* Compare both ranges. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* If packet range lies completely within policy range. */
                if((pkt_ip[i]       > policy_ip[i]) &&
                   (pkt_ip_range[i] < policy_ip_range[i]))
                {
                    result = NU_TRUE;
                    break;
                }
                /* Otherwise if packet range lies outside policy range. */
                else if((pkt_ip[i]       < policy_ip[i]) ||
                        (pkt_ip_range[i] > policy_ip_range[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then it lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_SUBNET_IP | IPSEC_IPV6):
            /* Policy is a range and packet is a subnet. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate subnets such that policy_subnet6 contains
                 * the smallest IP and pkt_subnet6 contains the largest
                 * under packet subnet.
                 */
                policy_subnet6[i] = (UINT8)(pkt_ip[i] & pkt_ip_range[i]);
                pkt_subnet6[i]    = (UINT8)(policy_subnet6[i] |
                                          ~(pkt_ip_range[i]));

                /* If packet subnet lies completely within policy range. */
                if((policy_subnet6[i] > policy_ip[i]) &&
                   (pkt_subnet6[i]    < policy_ip_range[i]))
                {
                    result = NU_TRUE;
                    break;
                }
                /* Otherwise if packet subnet lies outside policy range. */
                else if((policy_subnet6[i] < policy_ip[i]) ||
                        (pkt_subnet6[i]    > policy_ip_range[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then it lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case IPSEC_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
        break;
    }

    /* Check for the subnet type. */
    case(IPSEC_SUBNET_IP | IPSEC_IPV6):
    {
        /* Decode the packet selector's type. */
        switch((INT)pkt_sel_type)
        {
        case(IPSEC_SINGLE_IP | IPSEC_IPV6):
            /* Policy is a subnet and packet is a single IP. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate the subnets of policy and packet. */
                policy_subnet6[i] = (UINT8)
                                    (policy_ip[i] & policy_ip_range[i]);
                pkt_subnet6[i]    = (UINT8)
                                    (pkt_ip[i] & policy_ip_range[i]);
            }

            /* Compare the two subnets. */
            if(memcmp(policy_subnet6, pkt_subnet6, IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_RANGE_IP | IPSEC_IPV6):
            /* Policy is a subnet and packet is a range. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate subnets such that policy_subnet6 contains
                 * the smallest IP and packet_subnet6 contains the largest
                 * under policy subnet.
                 */
                policy_subnet6[i] = (UINT8)
                                    (policy_ip[i] & policy_ip_range[i]);
                pkt_subnet6[i]    = (UINT8)(policy_subnet6[i] |
                                          ~(policy_ip_range[i]));

                /* If packet range completely lies within policy subnet. */
                if((pkt_ip[i]       > policy_subnet6[i]) &&
                   (pkt_ip_range[i] < pkt_subnet6[i]))
                {
                    result = NU_TRUE;
                    break;
                }
                /* Otherwise if packet range lies outside policy subnet. */
                else if((pkt_ip[i]       < policy_subnet6[i]) ||
                        (pkt_ip_range[i] > pkt_subnet6[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then it lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case(IPSEC_SUBNET_IP | IPSEC_IPV6):
            /* Policy and packet are both subnets. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate both subnets. */
                policy_subnet6[i] = (UINT8)
                                    (policy_ip[i] & policy_ip_range[i]);
                pkt_subnet6[i]    = (UINT8)(pkt_ip[i] & pkt_ip_range[i]);

                /* Bitwise AND packet subnet with policy's mask to make
                 * sure packet subnet lies within policy subnet.
                 */
                pkt_subnet6[i] = (UINT8)
                                 (pkt_subnet6[i] & policy_ip_range[i]);
            }

            /* Compare the two subnets. */
            if(memcmp(policy_subnet6, pkt_subnet6, IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;

        case IPSEC_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
        break;
    }

#endif  /* #if (INCLUDE_IPV6 == NU_TRUE) */

    case IPSEC_WILDCARD:
        /* Wildcards cannot be compared so return exact match. */
        result = NU_TRUE;
        break;

    } /* End of 'switch' statement. */

#endif /* #if ((IPSEC_INCLUDE_TUNNEL_MODE == NU_FALSE) ||
               (INCLUDE_IKE == NU_FALSE)) */

    /* Now return the result. */
    return (result);

} /* IPSEC_Match_Sel_Addr_IPs */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Remove_Policy_Real
*
* DESCRIPTION
*
*       This function actually removes a policy and all its dependencies
*       from a group.
*
* INPUTS
*
*       *group_ptr              Pointer to the group passed.
*       *policy_ptr             Pointer to the policy to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_Policy_Real(IPSEC_POLICY_GROUP *group_ptr,
                                IPSEC_POLICY *policy_ptr)
{
    UINT16                  i;
    IPSEC_OUTBOUND_BUNDLE   *out_bundle;
    IPSEC_OUTBOUND_BUNDLE   *next_bundle;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_ptr == NU_NULL) || (policy_ptr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First we have to make sure that no TCP or UDP
     * port is accessing this policy. If so then we
     * first need to clear the policy pointers in
     * their respective instances to avoid having
     * dangling policy pointers.
     */
    for(i = 0; i < TCP_MAX_PORTS; i++)
    {
        /* Check if this port is using same policy
           which is going to be removed. */

        if((TCP_Ports[i] != NU_NULL) &&
          ((TCP_Ports[i]->tp_ips_in_policy  == policy_ptr) ||
           (TCP_Ports[i]->tp_ips_out_policy == policy_ptr)))
        {
            /* Set the policy pointers to NULL in TCP
               port instance. */
            TCP_Ports[i]->tp_ips_in_policy  = NU_NULL;
            TCP_Ports[i]->tp_ips_out_policy = NU_NULL;

            /* Set the bundle pointer to NULL. */
            TCP_Ports[i]->tp_ips_out_bundle = NU_NULL;
        }
    }

    /* Now clear the UDP ports as well. */
    for(i = 0; i < UDP_MAX_PORTS; i++)
    {
        /* Check if this port is using same policy
           which is going to be removed. */
        if((UDP_Ports[i] != NU_NULL) &&
          ((UDP_Ports[i]->up_ips_in_policy  == policy_ptr) ||
           (UDP_Ports[i]->up_ips_out_policy == policy_ptr)))
        {
            /* Set the incoming policy pointer to NULL in UDP port. */
            UDP_Ports[i]->up_ips_in_policy  = NU_NULL;

            /* Set the bundle pointer to NULL. */
            UDP_Ports[i]->up_ips_out_bundle = NU_NULL;

            /* Set the outgoing policy pointer to NULL in UDP port. */
            UDP_Ports[i]->up_ips_out_policy = NU_NULL;

            /* Clear the incoming selector. */
            UTL_Zero(&(UDP_Ports[i]->up_ips_in_select),
                     sizeof(IPSEC_SELECTOR));

            /* Clear the outgoing selector. */
            UTL_Zero(&(UDP_Ports[i]->up_ips_out_select),
                     sizeof(IPSEC_SELECTOR));
        }
    }/* End of 'for' loop. */

    next_bundle = policy_ptr->ipsec_bundles.ipsec_head;

    /* Remove all timer events associated with attached bundles with this
     * policy and then deallocate the respective memory as well.
     */
    for(out_bundle = next_bundle; (next_bundle != NU_NULL);
        out_bundle = next_bundle)
    {
        /* Store the next pointer. */
        next_bundle = out_bundle->ipsec_flink;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
        /* Remove all timer events associated with attached bundles with
           this policy. */
        TQ_Timerunset(IPSEC_Out_Bundle_Lifetime_Event, TQ_CLEAR_ALL_EXTRA,
                      (UNSIGNED)policy_ptr, (UNSIGNED)out_bundle);
#endif

        /* Deallocate the outbound SA memory. */
        if(NU_Deallocate_Memory(out_bundle) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the outbound bundle memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Remove the policy from the group SPDB. */
    SLL_Remove(&(group_ptr->ipsec_policy_list), policy_ptr);

    /* Now deallocate the policy memory too. */
    if(NU_Deallocate_Memory(policy_ptr) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate the memory",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return success value to the caller. */
    return (NU_SUCCESS);

} /* IPSEC_Remove_Policy_Real */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Get_Bundle_By_Selector
*
* DESCRIPTION
*
*       Returns a pointer to the first bundle that matches the passed
*       selector.
*
* INPUTS
*
*       *start_bundle           Pointer to the starting bundle.
*       *selector               Pointer to the selector.
*       **ret_bundle            Bundle pointer to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               bundle corresponding to the passed
*                               selector.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Bundle_By_Selector(IPSEC_OUTBOUND_BUNDLE *start_bundle,
                                    IPSEC_SELECTOR *selector,
                                    IPSEC_OUTBOUND_BUNDLE **ret_bundle)
{
    STATUS                  status = IPSEC_NOT_FOUND;
    IPSEC_OUTBOUND_BUNDLE   *bundle_ptr;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((start_bundle == NU_NULL) || (selector   == NU_NULL) ||
                                    (ret_bundle == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Make sure that the starting point is present and then find the
       required bundle through out the list. */
    for(bundle_ptr = start_bundle;
        bundle_ptr != NU_NULL;
        bundle_ptr = bundle_ptr->ipsec_flink)
    {
        /* Now match the two selectors. */
        if(IPSEC_Match_Selectors(selector, &(bundle_ptr->ipsec_selector),
                                 NU_FALSE) == NU_TRUE)
        {
            /* Return the bundle pointer to the caller. */
            *ret_bundle = bundle_ptr;

            /* Also make the status success. */
            status = NU_SUCCESS;
            break;
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Bundle_By_Selector */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_Outbound_Bundle
*
* DESCRIPTION
*
*       This function adds an outbound bundle to the passed policy.
*       Initially there is no SA present in the bundle.
*
* INPUTS
*
*       *policy_ptr             Identifier for the policy in which the
*                               bundle is to be added.
*       *pkt_selector           Pointer to packet selector.
*       **out_bundle            Pointer to the selector.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               bundle corresponding to the passed
*                               selector.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Outbound_Bundle(IPSEC_POLICY *policy_ptr,
                                 IPSEC_SELECTOR *pkt_selector,
                                 IPSEC_OUTBOUND_BUNDLE **out_bundle)
{
    STATUS              status;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((policy_ptr == NU_NULL) || (pkt_selector == NU_NULL) ||
                                  (out_bundle   == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Allocate the memory for the outbound bundle. */
    status = NU_Allocate_Memory(IPSEC_Memory_Pool, (VOID **)out_bundle,
                                sizeof(IPSEC_OUTBOUND_BUNDLE),
                                NU_NO_SUSPEND);

    /* Check the status value. */
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate the memory for outbound bundle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* First normalize the pointer. */
        *out_bundle = TLS_Normalize_Ptr(*out_bundle);

        /* Zero out the bundle first. */
        UTL_Zero(*out_bundle, sizeof(IPSEC_OUTBOUND_BUNDLE));

        /* Copy the passed packet selector to the bundle structure. */
        NU_BLOCK_COPY(&((*out_bundle)->ipsec_selector), pkt_selector,
                      sizeof(IPSEC_SELECTOR));

#if (INCLUDE_IKE == NU_TRUE)
        /* Now before inserting into the bundles list of the policy set
           the lifetime of this bundle. */
        status = TQ_Timerset(IPSEC_Out_Bundle_Lifetime_Event,
                            (UNSIGNED)*out_bundle,
                            (policy_ptr->ipsec_bundles.ipsec_lifetime *
                            TICKS_PER_SECOND),
                            (UNSIGNED)policy_ptr);

        /* Check the status value. */
        if(status == NU_SUCCESS)
#endif
        {
            /* Insert this bundle into the bundle list of the policy. */
            SLL_Insert(&(policy_ptr->ipsec_bundles), *out_bundle,
                         policy_ptr->ipsec_bundles.ipsec_head);
        }
    }

    /* Return the status value to the caller. */
    return (status);

} /* IPSEC_Add_Outbound_Bundle */

/************************************************************************
* FUNCTION
*
*       IPSEC_Match_Selectors
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       selectors. It checks whether policy selector matches
*       packet selector or not.
*
* INPUTS
*
*       *policy_selector        Pointer to policy selector.
*       *pkt_selector           Pointer to packet selector.
*       swap_flag               Flag to specify the swapping of address
*                               comparison. If its NU_TRUE source address
*                               of policy selector is compared with
*                               destination address of packet selector
*                               and vice versa.
*
* OUTPUTS
*
*       NU_TRUE                 If both selectors matched.
*       NU_FALSE                If both selector does not matched.
*
************************************************************************/
UINT8 IPSEC_Match_Selectors(IPSEC_SELECTOR *policy_selector,
                            IPSEC_SELECTOR *pkt_selector,
                            UINT8 swap_flag)
{
    UINT8     result = NU_FALSE;

    /* Compare the two transport layer protocols. */
    if((policy_selector->ipsec_transport_protocol == IPSEC_WILDCARD) ||
       (policy_selector->ipsec_transport_protocol ==
           pkt_selector->ipsec_transport_protocol))
    {
        /* Compare ICMPv4 and ICMPv6 message types & codes. */
        if ( (policy_selector->ipsec_transport_protocol == IP_ICMP_PROT)
#if (INCLUDE_IPV6 == NU_TRUE)
          || (policy_selector->ipsec_transport_protocol == IP_ICMPV6_PROT)
#endif
          )
        {
            if( policy_selector->ipsec_icmp_msg == IPSEC_WILDCARD ||
                (policy_selector->ipsec_icmp_msg ==
                    pkt_selector->ipsec_icmp_msg))
            {
                if( policy_selector->ipsec_icmp_code == IPSEC_WILDCARD ||
                         (policy_selector->ipsec_icmp_code ==
                                 pkt_selector->ipsec_icmp_code))
                {
                     result = NU_TRUE;
                }
            }
        }

        else if ( swap_flag == NU_TRUE )
        {
            /* Match the two source ports. */
            if((policy_selector->ipsec_source_port == IPSEC_WILDCARD) ||
               (policy_selector->ipsec_source_port ==
                   pkt_selector->ipsec_destination_port))
            {
                /* Match the two destination ports. */
                if((policy_selector->ipsec_destination_port ==
                                                IPSEC_WILDCARD) ||
                   (policy_selector->ipsec_destination_port ==
                       pkt_selector->ipsec_source_port))
                {
                    result = NU_TRUE;
                }
            }
        }
        else
        {
            /* Match the two source ports. */
            if((policy_selector->ipsec_source_port == IPSEC_WILDCARD) ||
               (policy_selector->ipsec_source_port ==
                   pkt_selector->ipsec_source_port))
            {
                /* Match the two destination ports. */
                if((policy_selector->ipsec_destination_port ==
                                                    IPSEC_WILDCARD) ||
                   (policy_selector->ipsec_destination_port ==
                       pkt_selector->ipsec_destination_port))
                {
                    result = NU_TRUE;
                }
            }

        }

        /* Addresses need to be swapped if the passed
            selector is of inbound type. */
        if( ( swap_flag == NU_TRUE ) && ( result == NU_TRUE ) )
        {
            result = NU_FALSE;

            /* Compare the two source addresses. */
            if(IPSEC_Match_Sel_Addr_IPs(
                          policy_selector->ipsec_source_type,
                            &(policy_selector-> ipsec_source_ip),
                            pkt_selector->ipsec_dest_type,
                            &(pkt_selector->ipsec_dest_ip))
                                                    == NU_TRUE)
            {
                /* Compare the two destination addresses. */
                if(IPSEC_Match_Sel_Addr_IPs(
                           policy_selector->ipsec_dest_type,
                            &(policy_selector->ipsec_dest_ip),
                            pkt_selector->ipsec_source_type,
                            &(pkt_selector->ipsec_source_ip))
                                                   == NU_TRUE)
                {
                    /* Now return the true status. */
                    result = NU_TRUE;
                }
            }
        }

        /* If we do not need to swap the addresses. We still check the
         * result variable to ensure that we have not already failed, in
         * which case there is no point in continuing.
         */
        else if ( result == NU_TRUE )
        {
            result = NU_FALSE;

            /* Compare the two source addresses. */
            if(IPSEC_Match_Sel_Addr_IPs(
                          policy_selector->ipsec_source_type,
                           &(policy_selector->ipsec_source_ip),
                           pkt_selector->ipsec_source_type,
                           &(pkt_selector->ipsec_source_ip))
                                                  == NU_TRUE)
            {
                /* Compare the two destination addresses. */
                if(IPSEC_Match_Sel_Addr_IPs(
                          policy_selector->ipsec_dest_type,
                           &(policy_selector->ipsec_dest_ip),
                           pkt_selector->ipsec_dest_type,
                           &(pkt_selector->ipsec_dest_ip))
                                                 == NU_TRUE)
                {
                    /* Now return the true status. */
                    result = NU_TRUE;
                }
            }
        }
    }

    /* Now return the result. */
    return (result);

} /* IPSEC_Match_Selectors */

/************************************************************************
* FUNCTION
*
*       IPSEC_Match_Policy_In
*
* DESCRIPTION
*
*       This function checks whether the passed list of security
*       associations and packet selector have a matching policy
*       in the passed group.
*
* INPUTS
*
*       *group                  The group from which desired
*                               policy needs to be found.
*       *pkt_selector           Packet selector.
*       **in_bundle             Array of inbound SAs.
*       sa_count                No. of SAs that have been passed as an
*                               array.
*       **policy_ret_ptr        Policy to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful operation.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Match_Policy_In(IPSEC_POLICY_GROUP *group,
                             IPSEC_SELECTOR *pkt_selector,
                             IPSEC_INBOUND_SA **in_bundle, UINT8 sa_count,
                             IPSEC_POLICY **policy_ret_ptr)
{
    STATUS                  status = IPSEC_NOT_FOUND;
    UINT8                   index;
    IPSEC_POLICY            *policy_ptr;
    IPSEC_INBOUND_SA        **sa_list = in_bundle;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((pkt_selector == NU_NULL) || (in_bundle == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Make sure passed group is not NULL. */
    if(group != NU_NULL)
    {
        /* Make sure we got a non empty list of policies and then traverse
            the SPDB for the desired policy. */
        for(policy_ptr = group->ipsec_policy_list.ipsec_head;
            policy_ptr != NU_NULL;
            policy_ptr = policy_ptr->ipsec_flink)
        {
            /* First make sure that this policy is for incoming packets
                or not. If not then no need to proceed further. */
            if((IPSEC_POLICY_FLOW(policy_ptr->ipsec_flags) &
                                                    IPSEC_INBOUND) != 0)
            {
                /* Now match the two selectors. */
                if(IPSEC_Match_Selectors(&(policy_ptr->ipsec_select),
                                          pkt_selector,
                ((IPSEC_POLICY_FLOW(policy_ptr->ipsec_flags) ==
                    IPSEC_DUAL_ASYNCHRONOUS)? NU_TRUE : NU_FALSE)) ==
                                                                NU_TRUE)
                {
                    /* Compare the total no. of security protocols
                        with the total no. of SAs. */
                    if(policy_ptr->ipsec_security_size == sa_count)
                    {
                        status = NU_SUCCESS;

                        /* If no SAs specified by policy (in the case
                         * of discard or bypass policy action).
                         */
                        if(sa_count != 0)
                        {
                            /* Now we have to match the each SA in the
                             * given incoming SAs list with security
                             * protocols present in the current policy.
                             * Last security protocol will be matched
                             * with the first SA in the list and second
                             * last with the second SA in the list and so
                             * on for all the total security protocols.
                             */

                            /* Loop through all the security protocols and
                             * the given SAs.
                             */
                            for(index = sa_count; index > 0; index--)
                            {
                                /* Now check the kind and order of each
                                   given incoming SAs with the policy
                                   found. */
                                if(IPSEC_MATCH_SEC_PROT(
                                 &(policy_ptr->ipsec_security[index - 1]),
                                   &(sa_list[(sa_count - index)]->
                                            ipsec_security)) != NU_TRUE)
                                {
                                    /* Break this loop . */
                                    status = IPSEC_NOT_FOUND;
                                    break;
                                }
                            }
                        }

                        /* If all the SAs and the corresponding security
                         *  protocols have been matched.
                         */
                        if(status == NU_SUCCESS)
                        {
                            /* What action needs to be taken? */
                            switch(IPSEC_POLICY_ACTION(
                                                 policy_ptr->ipsec_flags))
                            {
                                case IPSEC_DISCARD:
                                {
                                    /* Packet needs to be discarded. */
                                    status = IPSEC_PKT_DISCARD;
                                    break;
                                }

                                default:
                                    break;
                            }

                            /* Now break the main for loop as well. */
                            break;
                        }
                    }
                    else if(policy_ptr->ipsec_flags & IPSEC_APPLY)
                    {
                        /* If policy matches and no SA found discard the packet */
                        status = IPSEC_PKT_DISCARD;
                        break;
                    }
                }
            }
        }/* End of for loop. */

        /* Check whether caller needs the policy to be returned. */
        if(policy_ret_ptr != NU_NULL)
        {
            /* Return the current policy only in case of success otherwise
               return NULL in the policy pointer. */
            if(status == NU_SUCCESS)
                *policy_ret_ptr = policy_ptr;
            else
                *policy_ret_ptr = NU_NULL;
        }
    }

    /* Now return the status. */
    return (status);

} /* IPSEC_Match_Policy_In */

/************************************************************************
* FUNCTION
*
*       IPSEC_Match_Policy_Out
*
* DESCRIPTION
*
*       This function checks whether the passed list of security
*       associations and packet selector have a matching policy
*       in the passed group.
*
* INPUTS
*
*       *group                  The group from which desired
*                               policy needs to be found.
*       *pkt_selector           Packet selector.
*       **ret_policy_ptr        Policy to be returned.
*       **ret_bundle_ptr        Bundle to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful operation.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Match_Policy_Out(IPSEC_POLICY_GROUP *group,
                              IPSEC_SELECTOR *pkt_selector,
                              IPSEC_POLICY **ret_policy_ptr,
                              IPSEC_OUTBOUND_BUNDLE **ret_bundle_ptr )
{
    STATUS                  status;
    IPSEC_SELECTOR          req_selector;
    IPSEC_POLICY            *policy_ptr;
    IPSEC_OUTBOUND_BUNDLE   *out_bundle = NU_NULL;
    UINT8                   sa_derive;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Make sure that the group pointer is not null. */
    if((group          == NU_NULL) || (pkt_selector   == NU_NULL) ||
       (ret_policy_ptr == NU_NULL) || (ret_bundle_ptr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First get the security policy. */
    status = IPSEC_Get_Policy_By_Selector(group, NU_NULL, pkt_selector,
                                          IPSEC_OUTBOUND, &policy_ptr);

    /* Check what policy action need to be taken. */
    if(status == NU_SUCCESS)
    {
        /* Also return policy pointer to the caller. */
        *ret_policy_ptr = policy_ptr;

        /* Which policy action needs to be taken. */
        switch(IPSEC_POLICY_ACTION(policy_ptr->ipsec_flags))
        {
            case IPSEC_BY_PASS:
            {
                /* No need to do anything just return the status as
                   NU_SUCCESS. */
                break;
            }

            case IPSEC_APPLY:
            {
                /* First make sure that there is some security protocol
                    present in the policy. */
                if(policy_ptr->ipsec_security_size != 0)
                {

                    /* Retrieve the SA derivation flags from the policy. We
                     * will be using this multiple times so store it in a
                     * local.
                     */
                    sa_derive = policy_ptr->ipsec_security[0].ipsec_sa_derivation;

                    /* If selector is to be created from the packet then
                     * just copy the packet selector.
                     */
                    if ( sa_derive == IPSEC_VALUE_FROM_PACKET )
                    {
                        memcpy ( &req_selector, pkt_selector, sizeof ( IPSEC_SELECTOR ) );
                    }

                    else
                    {
                        /* Otherwise, copy the selector from the policy. */
                        memcpy ( &req_selector, &(policy_ptr->ipsec_select),
                                  sizeof ( IPSEC_SELECTOR ) );

                        /* If the complete selector from policy was required,
                         * then we are done.
                         */
                        if ( sa_derive != IPSEC_VALUE_FROM_POLICY )
                        {
                            /* Otherwise, we need to check each flag and
                             * take the value from packet for those flags
                             * that are specifically set.
                             */
                            if ( sa_derive & IPSEC_LADDR_FROM_PACKET )
                            {
                                memcpy ( &( req_selector.ipsec_source_ip ),
                                         &( pkt_selector->ipsec_source_ip ),
                                         sizeof ( IPSEC_IP_ADDR ) );
                            }

                            if ( sa_derive & IPSEC_RADDR_FROM_PACKET )
                            {
                                memcpy ( &( req_selector.ipsec_dest_ip ),
                                         &( pkt_selector->ipsec_dest_ip ),
                                          sizeof ( IPSEC_IP_ADDR ) );
                            }

                            if ( sa_derive & IPSEC_NEXT_FROM_PACKET )
                            {
                                req_selector.ipsec_transport_protocol =
                                    pkt_selector->ipsec_transport_protocol;
                            }

                            if ( sa_derive & IPSEC_LPORT_FROM_PACKET )
                            {
                                req_selector.ipsec_source_port =
                                    pkt_selector->ipsec_source_port;
                            }

                            if ( sa_derive & IPSEC_RPORT_FROM_PACKET )
                            {
                                req_selector.ipsec_destination_port =
                                    pkt_selector->ipsec_destination_port;
                            }
                        }
                    }

                    /* Now get the bundle from the policy. */
                    status = IPSEC_Get_Bundle_By_Selector(policy_ptr->
                                               ipsec_bundles.ipsec_head,
                                               &req_selector, &out_bundle);

                    /* Check the status value and make sure we can create
                     * new SAs.
                     */
                    if( status != NU_SUCCESS )
                    {
                        /* We have to add a new bundle into the policy. */
                        status = IPSEC_Add_Outbound_Bundle(policy_ptr,
                                                           pkt_selector,
                                                           &out_bundle);
                    }
                }

                break;
            }

            case IPSEC_DISCARD:
            {
                /* Packet needs to be discarded. */
                status = IPSEC_PKT_DISCARD;
                break;
            }

            default:
                break;
        } /* End of switch statement. */
    }

    /* Return the outgoing bundle to be applied to the caller. If no
       bundle is found, NU_NULL will be returned. */
    *ret_bundle_ptr = out_bundle;

    /* Now return the status. */
    return (status);

} /* IPSEC_Match_Policy_Out */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Bundle_SA_Entries
*
* DESCRIPTION
*
*       This routine returns the outbound SA pointers from the SADB
*       corresponding to the passed indices. The passed indices are first
*       sorted to do all the processing in one traversal of the SADB.
*       If one is absent, return error status and optionally ask
*       IKE to negotiate the required SAs.
*
* INPUTS
*
*       dev_index               Device index on which Nucleus IPsec and
*                               IKE are operating.
*       *group_ptr              Security group being used.
*       *policy_ptr             Security policy being used.
*       *out_bundle             Pointer to out bundle.
*       *pkt_selector           Pointer to packet selector.
*       **sa_ptr_bundle         Out Bound SA's.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       IPSEC_NOT_FOUND         Required SA not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Bundle_SA_Entries(UINT32 dev_index,
                                   IPSEC_POLICY_GROUP *group_ptr,
                                   IPSEC_POLICY *policy_ptr,
                                   IPSEC_OUTBOUND_BUNDLE *out_bundle,
                                   IPSEC_SELECTOR *pkt_selector,
                                   IPSEC_OUTBOUND_SA **sa_ptr_bundle)
{
    UINT8               i;
    UINT8               bundle_index;
    STATUS              status;
    UINT32              total_sa_required;
    IPSEC_BUNDLE_SORT   bundle_sort[IPSEC_MAX_SA_BUNDLE_SIZE];
    IPSEC_OUTBOUND_SA   *outbound_sa;
    IPSEC_OUTBOUND_SA   *start_sa;

#if (INCLUDE_IKE == NU_FALSE)
    UNUSED_PARAMETER(dev_index);
#endif

#if (IPSEC_DEBUG == NU_TRUE)

    /* Make sure that the group pointer is not null. */
    if((group_ptr  == NU_NULL) || (policy_ptr    == NU_NULL) ||
       (out_bundle == NU_NULL) || (pkt_selector  == NU_NULL) ||
                                  (sa_ptr_bundle == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /*First get the number of SAs required from policy being used*/
    total_sa_required = policy_ptr->ipsec_security_size;

    /* Initialize the list SA index pointers which are to be sorted.
     */
    for(i = 0; i < policy_ptr->ipsec_security_size; i++)
    {
        /* Set the SA index pointer. */
        bundle_sort[i].ipsec_sa_index =
            &(out_bundle->ipsec_out_sa_indexes[i]);

        /* Set the bundle index. */
        bundle_sort[i].ipsec_bundle_index = i;
    }

    /* Sort only if security size is greater than '1'. */
    if(policy_ptr->ipsec_security_size > 1)
    {
        /* Sort the indices so that corresponding SA pointers can be
         * obtained in a single traversal of the SADB.
         */
        IPSEC_Int_Ptr_Sort(bundle_sort, policy_ptr->ipsec_security_size);
    }

    /* Start searching from the start of the SADB. */
    start_sa = group_ptr->ipsec_outbound_sa_list.ipsec_head;

    /* Loop for all Security Protocols in the policy. */
    for(i = 0;
        i < policy_ptr->ipsec_security_size;
        i++)
    {
        /* Get the bundle index from the sort array. */
        bundle_index = bundle_sort[i].ipsec_bundle_index;

        /* No need to search by index if it is already zero or
         * if no more SAs remain to be searched.
         */
        if(*(bundle_sort[i].ipsec_sa_index) != 0)
        {
            /* Get the SA pointer from the index. */
            if((start_sa != NULL) &&
               (IPSEC_Get_Outbound_SA_By_Index(
                            *(bundle_sort[i].ipsec_sa_index),
                            start_sa, &outbound_sa) == NU_SUCCESS))
            {
                /* Set the SA pointer at its proper location. */
                sa_ptr_bundle[bundle_index] = outbound_sa;

                /* At least one SA has been found. */
                total_sa_required--;
            }
            else
            {
                /* No SA found. Set SA index to zero. */
                *(bundle_sort[i].ipsec_sa_index) = 0;
            }
        }

        /* Check to see if SA was not found by index. */
        if(*(bundle_sort[i].ipsec_sa_index) == 0)
        {
            /* Initialize SA pointer to NU_NULL to start the search
             * from the first item of the database.
             */
            sa_ptr_bundle[bundle_index] = NU_NULL;

            /* Search for the SA again, but this time
             * use the packet selector for matching.
             */
            if(IPSEC_Get_Outbound_SA(group_ptr, pkt_selector,
                            &(policy_ptr->ipsec_security[bundle_index]),
                            &(sa_ptr_bundle[bundle_index]))
                            == NU_SUCCESS)
            {
                /* SA has been found, so update the bundle with its
                 * index.
                 */
                *(bundle_sort[i].ipsec_sa_index) =
                    sa_ptr_bundle[bundle_index]->ipsec_index;

                /* At least one SA has been found. */
                total_sa_required--;
            }
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_FALSE)
            else
            {
                /* Encountering just one missing SA, when tunnel
                 * mode is not being used, is enough to request
                 * IKE to negotiate all missing SAs. Therefore,
                 * skip all remaining SAs in the bundle.
                 */
                break;
            }
#endif
        }
    } /* End of for loop. */

    /* Check if all the SAs have been found. */
    if(total_sa_required == 0)
    {
        /* Mark the status as success. */
        status = NU_SUCCESS;
    }
    else
    {
        /* Some SA is not found. */
        status = IPSEC_NOT_FOUND;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
        /* Request IKE for negotiating the required SAs. */
        IPSEC_IKE_SA_Request(dev_index, policy_ptr, out_bundle,
                             pkt_selector);
#endif

    }

    /* Return the status value to the caller. */
    return (status);

} /* IPSEC_Get_Bundle_SA_Entries */

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
/************************************************************************
* FUNCTION
*
*       IPSEC_IKE_SA_Request
*
* DESCRIPTION
*
*       This routine is responsible for requesting all the required SAs
*       in the passed bundle. The routine searches for missing SAs in
*       the bundle and then requests IKE for negotiating them.
*
* INPUTS
*
*       dev_index               Device index on which Nucleus
*                               IPsec and IKE are operating.
*       *policy_ptr             Security policy being used
*       *out_bundle             Particular bundle in the policy of which
*                               SAs are required.
*       *pkt_selector           Pointer to packet selector.
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID IPSEC_IKE_SA_Request(UINT32 dev_index,
                          IPSEC_POLICY *policy_ptr,
                          IPSEC_OUTBOUND_BUNDLE *out_bundle,
                          IPSEC_SELECTOR *pkt_selector)
{
    UINT8               i;
    IKE_INITIATE_REQ    *ike_req;

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)

    UINT8               suite_requested = NU_FALSE;

#endif

    /* First check whether there is already a request pending for
     * this Bundle to IKE. If so and request timeout has not yet
     * expired then no need to request again. Also make sure that
     * the IKE daemon thread is up. If not then there is no point
     * to request for anything.
     */
    if((NU_Retrieve_Clock() >= out_bundle->ipsec_sa_req_timeout) &&
          (IKE_Daemon_State == IKE_DAEMON_RUNNING))
    {
        /* Loop for the total no. of SAs required. */
        for(i = 0; (i < policy_ptr->ipsec_security_size); i++)
        {
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
            /* If an IKE request has already been made. */
            if(suite_requested == NU_TRUE)
            {
                /* If the current security is for tunnel mode. */
                if(policy_ptr->ipsec_security[i].ipsec_security_mode ==
                   IPSEC_TUNNEL_MODE)
                {
                    /* The suite changes with tunnel mode so another
                     * IKE request would be required.
                     */
                    suite_requested = NU_FALSE;
                }
                else
                {
                    /* Otherwise, no need to look further. Check
                     * the next security protocol to see if it
                     * is for tunnel mode.
                     */
                    continue;
                }
            }
#endif

            /* Only if index is zero. */
            if(out_bundle->ipsec_out_sa_indexes[i] == 0)
            {
                /* First get memory for an IKE request structure.
                   This memory will be freed by IKE. */
                if(NU_Allocate_Memory(IPSEC_Memory_Pool,
                                    (VOID **)&ike_req,
                                    sizeof(IKE_INITIATE_REQ),
                                    NU_NO_SUSPEND) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to allocate the memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
                else
                {
                    /* Normalize the pointer first. */
                    ike_req = TLS_Normalize_Ptr(ike_req);

                    /* Set device index. */
                    ike_req->ike_dev_index = dev_index;

                    /* Copy the security protocol whose corresponding
                       SA is required. */
                    NU_BLOCK_COPY(&(ike_req->ike_ips_security),
                                &(policy_ptr->ipsec_security[i]),
                                    sizeof(IPSEC_SECURITY_PROTOCOL));

                    /* If policy direction is simply inbound (and not
                     * outbound or dual asynchronous).
                     */
                    if((IPSEC_POLICY_FLOW(policy_ptr->ipsec_flags) &
                        IPSEC_OUTBOUND) == 0)
                    {
                        /* Copy the selector by switching source and
                         * destination fields. */
                        IKE_IPS_Switch_Selector(&(ike_req->ike_ips_select),
                                              &(policy_ptr->ipsec_select));
                    }
                    else
                    {
                        /* Copy the selector. */
                        NU_BLOCK_COPY(&(ike_req->ike_ips_select),
                                      &(policy_ptr->ipsec_select),
                                      sizeof(IPSEC_SELECTOR));
                    }

                    /* Override fields from the policy selector. */
                    ike_req->ike_ips_select.ipsec_source_port =
                        policy_ptr->ipsec_select.ipsec_source_port;
                    ike_req->ike_ips_select.ipsec_destination_port =
                        policy_ptr->ipsec_select.ipsec_destination_port;
                    ike_req->ike_ips_select.ipsec_transport_protocol =
                        policy_ptr->ipsec_select.ipsec_transport_protocol;

                    /* This should always be a non blocking call. */
                    ike_req->ike_suspend = NU_NO_SUSPEND;

                    /* Now call the IKE routine for creating the SA. */
                    IKE_SYNC_INITIATE(ike_req);

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
                    /* Set flag to indicate that IKE has been
                     * requested.
                     */
                    suite_requested = NU_TRUE;
#else
                    /* Look no further, since no more requests need
                     * to be made.
                     */
                    break;
#endif
                }
            }
        } /* End of main 'for' loop. */

        /* Update the request timeout for this SA. */
        out_bundle->ipsec_sa_req_timeout =
            (NU_Retrieve_Clock() + (IPSEC_SA_REQ_TIMEOUT *
                                    TICKS_PER_SECOND));
    }

} /* IPSEC_IKE_SA_Request */
#endif

/************************************************************************
* FUNCTION
*
*       IPSEC_Int_Ptr_Sort
*
* DESCRIPTION
*
*       This routine is used to sort the integer pointers.
*
* INPUTS
*
*       *array                  Array of bundle indices which are to be
*                               sorted.
*       array_size              Number of items in array.
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID IPSEC_Int_Ptr_Sort(IPSEC_BUNDLE_SORT *array, UINT8 array_size)
{
    UINT8               i;
    UINT8               j;
    IPSEC_BUNDLE_SORT   index;

    /* Loop through out the passed array. */
    for(i = 1; i < array_size; i++)
    {
        /* Copy the first item of the array. */
        index.ipsec_bundle_index = array[i].ipsec_bundle_index;
        index.ipsec_sa_index     = array[i].ipsec_sa_index;

        /* Shift elements down until insertion point found. */
        for(j = i;
            (j > 0) && (*(array[j - 1].ipsec_sa_index) >
                        *(index.ipsec_sa_index));
            j--)
        {
            /* Shift items right. */
            array[j].ipsec_bundle_index = array[j - 1].ipsec_bundle_index;
            array[j].ipsec_sa_index     = array[j - 1].ipsec_sa_index;
        }

        /* If current item needs to be re-ordered. */
        if(j != i)
        {
            /* Insert the item into the proper position. */
            array[j].ipsec_bundle_index = index.ipsec_bundle_index;
            array[j].ipsec_sa_index     = index.ipsec_sa_index;
        }
    }

} /* IPSEC_Int_Ptr_Sort */

/************************************************************************
* FUNCTION
*
*       IPSEC_Verify_Policy
*
* DESCRIPTION
*
*       This routine is used to validate the policy at transport layer.
*
* INPUTS
*
*       *in_policy              Pointer to the policy to be validated.
*
* OUTPUTS
*
*       None
*
************************************************************************/
STATUS IPSEC_Verify_Policy(IPSEC_POLICY *in_policy)
{
    STATUS              status = NU_SUCCESS;
    UINT8               i;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if(in_policy == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    if(in_policy->ipsec_flags & IPSEC_DISCARD)
    {
        /* The packet is to be discarded. */
        status = IPSEC_PKT_DISCARD;
    }

    /* Verify that the SA count matches. */
    else if(IPSEC_SA_Count !=
                    in_policy->ipsec_security_size)
    {
        /* The packet is to be discarded. */
        status = IPSEC_PKT_DISCARD;
    }

    /* If we are here with status = NU_SUCCESS, it means that
     * either the policy states that we need to by-pass
     * security or IPsec needs to be applied.
     */

    /* If IPsec needs to be applied... */
    else if(in_policy->ipsec_flags & IPSEC_APPLY)
    {
        /* Loop through the security associations to make sure
         * that they are in accordance with what was required.
         */
        for(i = IPSEC_SA_Count; i > 0; i--)
        {
            /* We check conformance by doing a memory
             * comparison of the security protocols
             * array in the policy and the security
             * associations.
             */
            if(IPSEC_MATCH_SEC_PROT(
                       &(IPSEC_In_Bundle[(IPSEC_SA_Count - i)]->
                       ipsec_security), &(in_policy->ipsec_security[i-1]))
                       != NU_TRUE)
            {
                /* The SA does not match what is required for this
                 * policy so discard this packet.
                 */
                status = IPSEC_PKT_DISCARD;

                break;
            }
        }

    }

    return status;

} /* IPSEC_Verify_Policy */

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Match_Selectors_Narrow
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       selectors. It checks whether policy selector matches
*       packet selector or not while also handling a narrowing
*       subset of the selector. Such a policy matching is required
*       by IKEv2.
*
*       This function will only match successfully if the 'traffic
*       selector' is a super-set of the 'policy selector' or if they
*       both are completely equal.
*
* INPUTS
*
*       *policy_selector        Pointer to policy selector.
*       *traffic_selector       Pointer to traffic selector.
*       swap_flag               Flag to specify the swapping of address
*                               comparison. If its NU_TRUE source address
*                               of policy selector is compared with
*                               destination address of packet selector
*                               and vice versa.
*       *return_narrow_proto    New protocol if narrowed.
*
* OUTPUTS
*
*       NU_TRUE                 If both selectors matched.
*       NU_FALSE                If both selector does not matched.
*
************************************************************************/
UINT8 IPSEC_Match_Selectors_Narrow(IPSEC_SELECTOR *policy_selector,
                                   IPSEC_SELECTOR *traffic_selector,
                                   UINT8 swap_flag,
                                   INT *return_narrow_proto)
{
    UINT8     result = NU_FALSE;

    /* If the caller has requested back the narrowed protocol, then
     * initialize it to -1 first.
     */
    if(return_narrow_proto != NU_NULL)
    {
        *return_narrow_proto = -1;
    }

    /* Compare the two transport layer protocols. */
    if((traffic_selector->ipsec_transport_protocol == IPSEC_WILDCARD) ||
       (policy_selector->ipsec_transport_protocol ==
           traffic_selector->ipsec_transport_protocol))
    {
        if((policy_selector->ipsec_transport_protocol != IPSEC_WILDCARD) &&
           (traffic_selector->ipsec_transport_protocol == IPSEC_WILDCARD) &&
           (return_narrow_proto != NU_NULL))
        {
            *return_narrow_proto =
                (INT)policy_selector->ipsec_transport_protocol;
        }

        /* Compare ICMPv4 and ICMPv6 message types & codes. */
        if ((policy_selector->ipsec_transport_protocol ==
             traffic_selector->ipsec_transport_protocol) &&
           ((policy_selector->ipsec_transport_protocol == IP_ICMP_PROT)
#if (INCLUDE_IPV6 == NU_TRUE)
          || (policy_selector->ipsec_transport_protocol == IP_ICMPV6_PROT)
#endif
          ))
        {
            if( traffic_selector->ipsec_icmp_msg == IPSEC_WILDCARD ||
                (policy_selector->ipsec_icmp_msg ==
                    traffic_selector->ipsec_icmp_msg))
            {
                if( traffic_selector->ipsec_icmp_code == IPSEC_WILDCARD ||
                         (policy_selector->ipsec_icmp_code ==
                                 traffic_selector->ipsec_icmp_code))
                {
                     result = NU_TRUE;
                }
            }
        }

        else if ( swap_flag == NU_TRUE )
        {
            /* Match the two source ports (swapped). */
            if((traffic_selector->ipsec_source_port <=
                policy_selector->ipsec_destination_port) &&
               (traffic_selector->ipsec_src_tid.
                ipsec_intern_src_port_end[1] >=
                policy_selector->ipsec_destination_port))
            {
                /* Match the two destination ports (swapped). */
                if((traffic_selector->ipsec_destination_port <=
                    policy_selector->ipsec_source_port) &&
                   (traffic_selector->ipsec_dst_tid.
                    ipsec_intern_dst_port_end[1] >=
                    policy_selector->ipsec_source_port))
                {
                    result = NU_TRUE;
                }
            }
        }
        else
        {
            /* Match the two source ports. */
            if((traffic_selector->ipsec_source_port <=
                policy_selector->ipsec_source_port) &&
               (traffic_selector->ipsec_src_tid.
                ipsec_intern_src_port_end[1] >=
                policy_selector->ipsec_source_port))
            {
                /* Match the two destination ports. */
                if((traffic_selector->ipsec_destination_port <=
                    policy_selector->ipsec_destination_port) &&
                   (traffic_selector->ipsec_dst_tid.
                    ipsec_intern_dst_port_end[1] >=
                    policy_selector->ipsec_destination_port))
                {
                    result = NU_TRUE;
                }
            }
        }

        /* Addresses need to be swapped if the passed
            selector is of inbound type. */
        if( ( swap_flag == NU_TRUE ) && ( result == NU_TRUE ) )
        {
            result = NU_FALSE;

            /* Compare the two source addresses. */
            if(IPSEC_Match_Sel_Addr_IPs(
                            traffic_selector->ipsec_dest_type,
                            &(traffic_selector->ipsec_dest_ip),
                            policy_selector->ipsec_source_type,
                            &(policy_selector-> ipsec_source_ip))
                                                    == NU_TRUE)
            {
                /* Compare the two destination addresses. */
                if(IPSEC_Match_Sel_Addr_IPs(
                            traffic_selector->ipsec_source_type,
                            &(traffic_selector->ipsec_source_ip),
                            policy_selector->ipsec_dest_type,
                            &(policy_selector->ipsec_dest_ip))
                                                   == NU_TRUE)
                {
                    /* Now return the true status. */
                    result = NU_TRUE;
                }
            }
        }

        /* If we do not need to swap the addresses. We still check the
         * result variable to ensure that we have not already failed, in
         * which case there is no point in continuing.
         */
        else if ( result == NU_TRUE )
        {
            result = NU_FALSE;

            /* Compare the two source addresses. */
            if(IPSEC_Match_Sel_Addr_IPs(
                           traffic_selector->ipsec_source_type,
                           &(traffic_selector->ipsec_source_ip),
                           policy_selector->ipsec_source_type,
                           &(policy_selector->ipsec_source_ip))
                                                  == NU_TRUE)
            {
                /* Compare the two destination addresses. */
                if(IPSEC_Match_Sel_Addr_IPs(
                           traffic_selector->ipsec_dest_type,
                           &(traffic_selector->ipsec_dest_ip),
                           policy_selector->ipsec_dest_type,
                           &(policy_selector->ipsec_dest_ip))
                                                 == NU_TRUE)
                {
                    /* Now return the true status. */
                    result = NU_TRUE;
                }
            }
        }
    }

    /* Now return the result. */
    return (result);

} /* IPSEC_Match_Selectors_Narrow */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Policy_By_Selector_Narrow
*
* DESCRIPTION
*
*       Get the first policy matched by the given selector while
*       also matching a narrowing subset of the selector. This
*       policy matching approach is used by IKEv2 to search
*       policies.
*
* INPUTS
*
*       *group                  Pointer to the group.
*       *policy_ptr             Starting searching point.
*       *traffic_selector       Traffic selector passed.
*       policy_type             Direction of policy. Valid values are
*                               IPSEC_OUTBOUND and IPSEC_INBOUND.
*       **ret_policy_ptr        Policy to be returned.
*       *return_narrow_proto    New protocol if narrowed.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       IPSEC_NOT_FOUND         In case policy is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Policy_By_Selector_Narrow(IPSEC_POLICY_GROUP *group,
                                           IPSEC_POLICY *policy_ptr,
                                           IPSEC_SELECTOR *traffic_selector,
                                           UINT8 policy_type,
                                           IPSEC_POLICY **ret_policy_ptr,
                                           INT *return_narrow_proto)
{
    STATUS              status = IPSEC_NOT_FOUND;
    IPSEC_POLICY        *policy_start;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group        == NU_NULL) || (traffic_selector == NU_NULL) ||
                                    (ret_policy_ptr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* If policy pointer is null then we have to search from the start
       of the policy list. */
    if(policy_ptr == NU_NULL)
    {
        /* Search from first policy in the group. */
        policy_start = group->ipsec_policy_list.ipsec_head;
    }
    else
    {
        /* Starting point for policy searching is given. */
        policy_start = policy_ptr;
    }

    /* Find the required policy. */
    for(;policy_start != NU_NULL;
        policy_start = policy_start->ipsec_flink)
    {
        /* If the policy supports the desired direction and action. */
        if((policy_start->ipsec_flags & policy_type) == policy_type)
        {
            /* Compare the two selectors. */
            if(IPSEC_Match_Selectors_Narrow(&(policy_start->ipsec_select),
                                    traffic_selector,
                        ((IPSEC_POLICY_FLOW(policy_start->ipsec_flags) ==
                          IPSEC_DUAL_ASYNCHRONOUS) &&
                         (policy_type & IPSEC_INBOUND) ?
                          NU_TRUE : NU_FALSE),
                          return_narrow_proto) == NU_TRUE)
            {
                /* Now return policy whose selector has been matched. */
                *ret_policy_ptr = policy_start;

                /* Successful operation. */
                status = NU_SUCCESS;
                break;
            }
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Policy_By_Selector_Narrow */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Policy_Index_Narrow
*
* DESCRIPTION
*
*       This function returns a policy index matching the passed
*       selector in the given group. It is a variation of the
*       IPSEC_Get_Policy_Index() API and differs such that it
*       honors narrowing subsets of selectors during policy
*       matching. Such a method of policy look-up is internally
*       by IKEv2.
*
*       In other words, IPSEC_Get_Policy_Index() successfully
*       matches if the 'packet' selector is a subset of or equal to
*       the policy selector. Whereas this utility function
*       successfully matches if the 'traffic' selector is a super set
*       of or equal to the policy selector.
*
* INPUTS
*
*       *group_name             Name of the group.
*       *traffic_selector       Selector to be matched
*       selector_type           Type of selector IPSEC_INBOUND or
*                               IPSEC_OUTBOUND.
*       *return_index           Index to be found.
*       *return_narrow_proto    New protocol if narrowed.
*
* OUTPUTS
*
*       NU_SUCCESS             In case of success.
*       NU_TIMEOUT             Timeout for obtaining semaphore.
*       IPSEC_NOT_FOUND        If a policy is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Policy_Index_Narrow(CHAR *group_name,
                                     IPSEC_SELECTOR *traffic_selector,
                                     UINT8 selector_type,
                                     UINT32 *return_index,
                                     INT *return_narrow_proto)
{
    STATUS              status;
    IPSEC_POLICY        *ret_policy_ptr;
    IPSEC_POLICY_GROUP  *group;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed first. */
    if((group_name == NU_NULL) || (traffic_selector == NU_NULL) ||
                                  (return_index == NU_NULL))
        status = IPSEC_INVALID_PARAMS;

    else
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
            /* First get the group entry pointer. */
            status = IPSEC_Get_Group_Entry(group_name, &group);

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* Get the policy by selector. */
                status = IPSEC_Get_Policy_By_Selector_Narrow(group,
                                    NU_NULL, traffic_selector,
                                    selector_type,
                                    &ret_policy_ptr, return_narrow_proto);
                /* Check the status value. */
                if(status == NU_SUCCESS)
                {
                    /* Compare the policy found type and desired type. */
                    if((ret_policy_ptr->ipsec_flags & selector_type) ==
                        selector_type)
                    {
                        /* Now return the required index. */
                        *return_index = ret_policy_ptr->ipsec_index;
                    }
                    else
                    {
                        status = IPSEC_INVALID_PARAMS;
                    }
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

} /* IPSEC_Get_Policy_Index_Narrow */

#endif /* (IKE_INCLUDE_VERSION_2 == NU_TRUE) */

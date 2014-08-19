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
*       ike_spdb.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       Implementation of the IKE Security Policy database.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Match_Selectors
*       IKE_Match_Selectors_Abs
*       IKE_Add_Policy
*       IKE_Get_Policy_Entry
*       IKE_Get_Policy_By_Selector
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*       ike_doi.h
*       ike_dh.h
*       ike_cert.h
*       ike_crypto_wrappers.h
*
************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"
#include "networking/ike_doi.h"
#include "networking/ike_dh.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
#include "networking/ike_cert.h"
#endif

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#include "networking/ike2_api.h"
#include "networking/ike2.h"
#endif

/************************************************************************
*
* FUNCTION
*
*       IKE_Match_Selectors
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       selectors. It checks whether selector 'b' falls within
*       the range of selector 'a'. The address types of both
*       the selectors need not be equal, to be matched.
*
*       Note that IPv4 addresses cannot be compared to IPv6
*       addresses.
*
* INPUTS
*
*       *a                      Pointer to the first selector.
*       *b                      Pointer to the second selector.
*
* OUTPUTS
*
*       NU_TRUE                 If selector 'b' matches selector 'a'.
*       NU_FALSE                If selector 'b' does not match
*                               selector 'a'.
*
************************************************************************/
INT IKE_Match_Selectors(IKE_POLICY_SELECTOR *a,
                        IKE_POLICY_SELECTOR *b)
{
    INT                 result = NU_FALSE;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32              subnet4;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8               subnet6[IP6_ADDR_LEN];
    INT                 i;
#endif

    /* Determine the type of the first selector. */
    switch(a->ike_type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
    case IKE_IPV4:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_IPV4:
            /* Compare two single IP addresses. */
            if(IP_ADDR(a->ike_addr.ike_ip.ike_addr1) ==
               IP_ADDR(b->ike_addr.ike_ip.ike_addr1))
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4_SUBNET:
        case IKE_IPV4_RANGE:
        case IKE_IPV6:
        case IKE_IPV6_SUBNET:
        case IKE_IPV6_RANGE:
            /* Any other address type cannot be compared. */
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        } /* switch ... */
        break;

    case IKE_IPV4_SUBNET:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_IPV4:
            /* 'a' is a subnet and 'b' is a single IP. */

            /* Calculate the subnet of 'b'. */
            subnet4 = IP_ADDR(b->ike_addr.ike_ip.ike_addr1) &
                      IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2);

            /* Compare the two addresses. */
            if(IP_ADDR(a->ike_addr.ike_ip.ike_addr1) == subnet4)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4_SUBNET:
            /* 'a' and 'b' are both subnets. */

            /* Calculate the subnet of 'b' using the mask of 'a'. */
            subnet4 = IP_ADDR(b->ike_addr.ike_ip.ike_addr1) &
                      IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2);

            /* Compare the two subnets. */
            if(IP_ADDR(a->ike_addr.ike_ip.ike_addr1) == subnet4)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4_RANGE:
            /* 'a' is a subnet and 'b' is a range. */

            /* Calculate subnets such that subnet4 contains
             * the largest IP under subnet of 'a'.
             */
            subnet4 = IP_ADDR(a->ike_addr.ike_ip.ike_addr1) | ~(
                      IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2));

            /* Make sure subnet 'a's range is a superset of
             * the range of 'b'.
             */
            if((IP_ADDR(a->ike_addr.ike_ip.ike_addr1) <=
                IP_ADDR(b->ike_addr.ike_ip.ike_addr1)) &&
               (subnet4 >=
                IP_ADDR(b->ike_addr.ike_ip.ike_ext_addr.ike_addr2)))
            {
                /* 'b' lies within the subnet 'a'. */
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6:
        case IKE_IPV6_SUBNET:
        case IKE_IPV6_RANGE:
            /* IPv6 cannot be compared to IPv4 addresses. */
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        } /* switch ... */
        break;

    case IKE_IPV4_RANGE:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_IPV4:
            /* 'a' is a range and 'b' is a single IP. */

            /* Check if 'b' lies within 'a'. */
            if((IP_ADDR(b->ike_addr.ike_ip.ike_addr1) >=
                IP_ADDR(a->ike_addr.ike_ip.ike_addr1)) &&
               (IP_ADDR(b->ike_addr.ike_ip.ike_addr1) <=
                IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2)))
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4_SUBNET:
            /* 'a' is a range and 'b' is a subnet. */

            /* Calculate subnets such that subnet4 contains the
             * largest IP under subnet of 'b'.
             */
            subnet4 = IP_ADDR(b->ike_addr.ike_ip.ike_addr1) | ~(
                      IP_ADDR(b->ike_addr.ike_ip.ike_ext_addr.ike_addr2));

            /* Make sure subnet 'b's range is a subset of
             * the range of 'a'.
             */
            if((IP_ADDR(b->ike_addr.ike_ip.ike_addr1) >=
                IP_ADDR(a->ike_addr.ike_ip.ike_addr1)) &&
               (subnet4 <=
                IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2)))
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4_RANGE:
            /* 'a' and 'b' are both ranges. */

            /* Make sure range 'b' lies within range 'a'. */
            if((IP_ADDR(b->ike_addr.ike_ip.ike_addr1) >=
                IP_ADDR(a->ike_addr.ike_ip.ike_addr1)) &&
               (IP_ADDR(b->ike_addr.ike_ip.ike_ext_addr.ike_addr2) <=
                IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2)))
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6:
        case IKE_IPV6_SUBNET:
        case IKE_IPV6_RANGE:
            /* IPv6 cannot be compared to IPv4 addresses. */
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        } /* switch ... */
        break;
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
    case IKE_IPV6:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_IPV6:
            /* Compare two single IP addresses. */
            if(memcmp(a->ike_addr.ike_ip.ike_addr1,
                      b->ike_addr.ike_ip.ike_addr1, IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6_SUBNET:
        case IKE_IPV6_RANGE:
        case IKE_IPV4:
        case IKE_IPV4_SUBNET:
        case IKE_IPV4_RANGE:
            /* Any other address type cannot be compared. */
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        } /* switch ... */
        break;

    case IKE_IPV6_SUBNET:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_IPV6:
            /* 'a' is a subnet and 'b' is a single IP. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate the subnet of 'b'. */
                subnet6[i] = (UINT8)(b->ike_addr.ike_ip.ike_addr1[i] &
                                     a->ike_addr.ike_ip.ike_ext_addr.
                                         ike_addr2[i]);
            }

            /* Compare the two subnets. */
            if(memcmp(a->ike_addr.ike_ip.ike_addr1,
                      subnet6, IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6_SUBNET:
            /* 'a' and 'b' are both subnets. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Bitwise AND subnet of 'b' with 'a's mask to make
                 * sure subnet 'b' lies within subnet 'a'.
                 */
                subnet6[i] = (UINT8)(b->ike_addr.ike_ip.ike_addr1[i] &
                                     a->ike_addr.ike_ip.ike_ext_addr.
                                         ike_addr2[i]);
            }

            /* Compare the two subnets. */
            if(memcmp(a->ike_addr.ike_ip.ike_addr1,
                      subnet6, IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6_RANGE:
            /* 'a' is a subnet and 'b' is a range. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate subnets such that subnet6 contains
                 * the largest IP under subnet of 'a'.
                 */
                subnet6[i] = (UINT8)(a->ike_addr.ike_ip.ike_addr1[i] |
                                     ~(a->ike_addr.ike_ip.ike_ext_addr.
                                           ike_addr2[i]));

                /* If range 'b' completely lies within subnet 'a'. */
                if((b->ike_addr.ike_ip.ike_addr1[i] >
                    a->ike_addr.ike_ip.ike_addr1[i]) &&
                   (b->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i] <
                    subnet6[i]))
                {
                    result = NU_TRUE;
                    break;
                }

                /* Otherwise if range 'b' lies outside subnet 'a'. */
                else if((b->ike_addr.ike_ip.ike_addr1[i] <
                         a->ike_addr.ike_ip.ike_addr1[i]) ||
                        (b->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i] >
                         subnet6[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If the above loop didn't break, then 'b' lies within
             * the subnet 'a'.
             */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4:
        case IKE_IPV4_SUBNET:
        case IKE_IPV4_RANGE:
            /* IPv4 cannot be compared to IPv6 addresses. */
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        } /* switch ... */
        break;

    case IKE_IPV6_RANGE:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_IPV6:
            /* 'a' is a range and 'b' is a single IP. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* If IP 'b' completely lies within range 'a'. */
                if((b->ike_addr.ike_ip.ike_addr1[i] >
                    a->ike_addr.ike_ip.ike_addr1[i]) &&
                   (b->ike_addr.ike_ip.ike_addr1[i] <
                    a->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i]))
                {
                    result = NU_TRUE;
                    break;
                }

                /* Otherwise if IP 'b' lies outside range 'a'. */
                else if((b->ike_addr.ike_ip.ike_addr1[i] <
                         a->ike_addr.ike_ip.ike_addr1[i]) ||
                        (b->ike_addr.ike_ip.ike_addr1[i] >
                         a->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then 'b' lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6_SUBNET:
            /* 'a' is a range and 'b' is a subnet. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* Calculate subnet such that subnet6 contains
                 * the largest IP under subnet of 'b'.
                 */
                subnet6[i] = (UINT8)(b->ike_addr.ike_ip.ike_addr1[i] |
                                     ~(b->ike_addr.ike_ip.ike_ext_addr.
                                           ike_addr2[i]));

                /* If subnet 'b' lies completely within range 'a'. */
                if((b->ike_addr.ike_ip.ike_addr1[i] >
                    a->ike_addr.ike_ip.ike_addr1[i]) &&
                   (subnet6[i] <
                    a->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i]))
                {
                    result = NU_TRUE;
                    break;
                }

                /* Otherwise if subnet 'b' lies outside range 'a'. */
                else if((b->ike_addr.ike_ip.ike_addr1[i] <
                         a->ike_addr.ike_ip.ike_addr1[i]) ||
                        (subnet6[i] >
                         a->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then 'b' lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV6_RANGE:
            /* 'a' and 'b' are both ranges. */
            for(i = 0; i < IP6_ADDR_LEN; i++)
            {
                /* If range 'b' lies completely within range 'a'. */
                if((b->ike_addr.ike_ip.ike_addr1[i] >
                    a->ike_addr.ike_ip.ike_addr1[i]) &&
                   (b->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i] <
                    a->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i]))
                {
                    result = NU_TRUE;
                    break;
                }

                /* Otherwise if range 'b' lies outside range 'a'. */
                else if((b->ike_addr.ike_ip.ike_addr1[i] <
                         a->ike_addr.ike_ip.ike_addr1[i]) ||
                        (b->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i] >
                         a->ike_addr.ike_ip.ike_ext_addr.ike_addr2[i]))
                {
                    /* Result has been initialized to false above. */
                    break;
                }
            }

            /* If above loop didn't break, then 'b' lies within range. */
            if(i == IP6_ADDR_LEN)
            {
                result = NU_TRUE;
            }
            break;

        case IKE_IPV4:
        case IKE_IPV4_SUBNET:
        case IKE_IPV4_RANGE:
            /* IPv4 cannot be compared to IPv6 addresses. */
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        } /* switch ... */
        break;
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    case IKE_DOMAIN_NAME:
    case IKE_USER_DOMAIN_NAME:
        /* Determine the type of the second selector. */
        switch(b->ike_type)
        {
        case IKE_DOMAIN_NAME:
        case IKE_USER_DOMAIN_NAME:
            /* Compare the two user domain names. */
            if(strcmp(a->ike_addr.ike_domain,
                      b->ike_addr.ike_domain) == 0)
            {
                /* The domain name matches. */
                result = NU_TRUE;
            }
            break;

        case IKE_WILDCARD:
            /* No need to compare further. Wildcard matches all. */
            result = NU_TRUE;
            break;
        }
        break;
#endif

    case IKE_WILDCARD:
        /* No need to compare further. Wildcard matches all. */
        result = NU_TRUE;
        break;
    }

    /* Return the result. */
    return (result);

} /* IKE_Match_Selectors */

/************************************************************************
*
* FUNCTION
*
*       IKE_Match_Selectors_Abs
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       selectors. It matches selectors using their absolute
*       value, so that selectors with different address types
*       could never match.
*
*       No error checking is performed in this function because
*       it relies on the error checking of the functions which
*       call it.
*
* INPUTS
*
*       *a                      Pointer to the first selector.
*       *b                      Pointer to the second selector.
*
* OUTPUTS
*
*       NU_TRUE                 If the selectors are equal.
*       NU_FALSE                If selectors are not equal.
*
************************************************************************/
INT IKE_Match_Selectors_Abs(IKE_POLICY_SELECTOR *a,
                            IKE_POLICY_SELECTOR *b)
{
    INT             result = NU_FALSE;

    /* Only compare further if the selectors have
     * the same address types.
     */
    if(a->ike_type == b->ike_type)
    {
        switch(a->ike_type)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
        case IKE_IPV4_RANGE:
        case IKE_IPV4_SUBNET:
            if(IP_ADDR(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2) !=
               IP_ADDR(b->ike_addr.ike_ip.ike_ext_addr.ike_addr2))
            {
                break;
            }

        case IKE_IPV4:
            if(IP_ADDR(a->ike_addr.ike_ip.ike_addr1) ==
               IP_ADDR(b->ike_addr.ike_ip.ike_addr1))
            {
                result = NU_TRUE;
            }
            break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        case IKE_IPV6_RANGE:
        case IKE_IPV6_SUBNET:
            if(memcmp(a->ike_addr.ike_ip.ike_ext_addr.ike_addr2,
                      b->ike_addr.ike_ip.ike_ext_addr.ike_addr2,
                      IP6_ADDR_LEN) != 0)
            {
                break;
            }

        case IKE_IPV6:
            if(memcmp(a->ike_addr.ike_ip.ike_addr1,
                      b->ike_addr.ike_ip.ike_addr1,
                      IP6_ADDR_LEN) == 0)
            {
                result = NU_TRUE;
            }
            break;
#endif

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
        case IKE_DOMAIN_NAME:
        case IKE_USER_DOMAIN_NAME:
            /* Compare the domain name string. */
            if(strcmp(a->ike_addr.ike_domain,
                      b->ike_addr.ike_domain) == 0)
            {
                result = NU_TRUE;
            }
            break;
#endif

        case IKE_WILDCARD:
            /* Wildcard matches all addresses. */
            result = NU_TRUE;
            break;
        }
    }

    return (result);

} /* IKE_Match_Selectors_Abs */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_Policy
*
* DESCRIPTION
*
*       This function adds a new policy to the specified group.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       *policy                 Pointer to the policy to be added.
*       *index                  On a successful request, the
*                               uniquely assigned policy index is
*                               returned in this parameter.
*
* OUTPUTS
*
*       NU_SUCCESS              Policy was successfully added.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_KEYLEN      Encryption key length is invalid.
*       IKE_INVALID_PARAMS      Parameters passed are invalid.
*       IKE_ALREADY_EXISTS      Policy by this selector already exists.
*       IKE_NOT_FOUND           Group was not found.
*       IKE_UNSUPPORTED_ALGO    Algorithm specified in policy is
*                               not supported by IKE.
*
************************************************************************/
STATUS IKE_Add_Policy(CHAR *group_name, IKE_POLICY *policy,
                      UINT32 *index)
{
    STATUS              status = NU_SUCCESS;
    UNSIGNED            mem_size;
    IKE_POLICY_GROUP    *group;
    IKE_POLICY          *new_policy;
    IKE_POLICY          *find_policy;
    CHAR HUGE           *next_mem;
    INT                 i;
    UINT16              algo_index;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    if((policy != NU_NULL) && (policy->ike_version != IKE_VERSION_1) &&
    (policy->ike_version != IKE_VERSION_2))
    {
        /* When no version is specified, use version 1. This is specially
         * needed in the case when version 2 is included but not used by
         * application. In this case, rather than leaving this member
         * uninitialized, set it to version 1. For version 2, it has to be
         * explicitly set to IKE_VERSION_2.
         */
        policy->ike_version = IKE_VERSION_1;
    }
#endif

    /* Make sure all pointers are valid. */
    if((group_name == NU_NULL) || (policy == NU_NULL) ||
       (index      == NU_NULL))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Verify selector address type. */
    else if(
#if (INCLUDE_IPV4 == NU_TRUE)
            (policy->ike_select.ike_type != IKE_IPV4)             &&
            (policy->ike_select.ike_type != IKE_IPV4_RANGE)       &&
            (policy->ike_select.ike_type != IKE_IPV4_SUBNET)      &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (policy->ike_select.ike_type != IKE_IPV6)             &&
            (policy->ike_select.ike_type != IKE_IPV6_RANGE)       &&
            (policy->ike_select.ike_type != IKE_IPV6_SUBNET)      &&
#endif
            (policy->ike_select.ike_type != IKE_WILDCARD))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Verify local identification type (IKE version 1). */
    else if((policy->ike_version == IKE_VERSION_1) &&
            (
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
            (policy->ike_my_id.ike_type != IKE_DOMAIN_NAME)      &&
            (policy->ike_my_id.ike_type != IKE_USER_DOMAIN_NAME) &&
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            (policy->ike_my_id.ike_type != IKE_IPV4)             &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (policy->ike_my_id.ike_type != IKE_IPV6)             &&
#endif
            (policy->ike_my_id.ike_type != IKE_WILDCARD)
            ))
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    /* Verify local identification type if it is a domain name (v1). */
    else if((policy->ike_version == IKE_VERSION_1) &&
           (((policy->ike_my_id.ike_type == IKE_DOMAIN_NAME)       ||
             (policy->ike_my_id.ike_type == IKE_USER_DOMAIN_NAME)) &&
            ((policy->ike_my_id.ike_addr.ike_domain  == NU_NULL)   ||
             (*policy->ike_my_id.ike_addr.ike_domain == 0)         ||
             (strlen(policy->ike_my_id.ike_addr.ike_domain) >=
              IKE_MAX_DOMAIN_NAME_LEN))))
    {
        status = IKE_INVALID_PARAMS;
    }
#endif

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    /* Verify local identification type (IKE version 2). */
    else if((policy->ike_version == IKE_VERSION_2) &&
            (
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
            (policy->ike_my_id.ike_type != IKE2_ID_TYPE_FQDN)           &&
            (policy->ike_my_id.ike_type != IKE2_ID_TYPE_RFC822_ADDR)    &&
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            (policy->ike_my_id.ike_type != IKE2_ID_TYPE_IPV4_ADDR)      &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (policy->ike_my_id.ike_type != IKE2_ID_TYPE_IPV6_ADDR)      &&
#endif
            (policy->ike_my_id.ike_type != IKE2_ID_TYPE_DER_ASN1_DN)
            ))
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    /* Verify local identification type if it is a domain name (v2). */
    else if((policy->ike_version == IKE_VERSION_2) &&
           (((policy->ike_my_id.ike_type == IKE2_ID_TYPE_FQDN)          ||
             (policy->ike_my_id.ike_type == IKE2_ID_TYPE_RFC822_ADDR))  &&
            ((policy->ike_my_id.ike_addr.ike_domain  == NU_NULL)        ||
             (*policy->ike_my_id.ike_addr.ike_domain == 0)              ||
             (strlen(policy->ike_my_id.ike_addr.ike_domain) >=
              IKE_MAX_DOMAIN_NAME_LEN))))
    {
        status = IKE_INVALID_PARAMS;
    }
#endif
#endif /* (IKE_INCLUDE_VERSION_2 == NU_TRUE) */

    /* Verify remote identification type (IKE version 1). */
    else if((policy->ike_version == IKE_VERSION_1) &&
            (
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
            (policy->ike_peers_id.ike_type != IKE_DOMAIN_NAME)      &&
            (policy->ike_peers_id.ike_type != IKE_USER_DOMAIN_NAME) &&
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            (policy->ike_peers_id.ike_type != IKE_IPV4)             &&
            (policy->ike_peers_id.ike_type != IKE_IPV4_SUBNET)      &&
            (policy->ike_peers_id.ike_type != IKE_IPV4_RANGE)       &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (policy->ike_peers_id.ike_type != IKE_IPV6)             &&
            (policy->ike_peers_id.ike_type != IKE_IPV6_SUBNET)      &&
            (policy->ike_peers_id.ike_type != IKE_IPV6_RANGE)       &&
#endif
            (policy->ike_peers_id.ike_type != IKE_WILDCARD)
            ))
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    /* Verify remote identification type if it is a domain name (v1). */
    else if((policy->ike_version == IKE_VERSION_1) &&
           (((policy->ike_peers_id.ike_type == IKE_DOMAIN_NAME)       ||
             (policy->ike_peers_id.ike_type == IKE_USER_DOMAIN_NAME)) &&
            ((policy->ike_peers_id.ike_addr.ike_domain  == NU_NULL)   ||
             (*policy->ike_peers_id.ike_addr.ike_domain == 0)         ||
             (strlen(policy->ike_peers_id.ike_addr.ike_domain) >=
              IKE_MAX_DOMAIN_NAME_LEN))))
    {
        status = IKE_INVALID_PARAMS;
    }
#endif

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    /* Verify remote identification type (IKE version 2). */
    else if((policy->ike_version == IKE_VERSION_2) &&
            (
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
            (policy->ike_peers_id.ike_type != IKE2_ID_TYPE_FQDN)        &&
            (policy->ike_peers_id.ike_type != IKE2_ID_TYPE_RFC822_ADDR) &&
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            (policy->ike_peers_id.ike_type != IKE2_ID_TYPE_IPV4_ADDR)   &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (policy->ike_peers_id.ike_type != IKE2_ID_TYPE_IPV6_ADDR)   &&
#endif
            (policy->ike_peers_id.ike_type != IKE2_ID_TYPE_DER_ASN1_DN)
            ))
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    /* Verify remote identification type if it is a domain name (v2). */
    else if((policy->ike_version == IKE_VERSION_2) &&
           (((policy->ike_peers_id.ike_type == IKE2_ID_TYPE_FQDN)     ||
             (policy->ike_peers_id.ike_type == IKE2_ID_TYPE_RFC822_ADDR))&&
            ((policy->ike_peers_id.ike_addr.ike_domain  == NU_NULL)   ||
             (*policy->ike_peers_id.ike_addr.ike_domain == 0)         ||
             (strlen(policy->ike_peers_id.ike_addr.ike_domain) >=
              IKE_MAX_DOMAIN_NAME_LEN))))
    {
        status = IKE_INVALID_PARAMS;
    }
#endif
#endif /* (IKE_INCLUDE_VERSION_2 == NU_TRUE) */

    /* At least one phase 2 protocol that can be negotiated
     * must be specified in the policy.
     */
    else if((policy->ike_ids_no == 0) || (policy->ike_ids == NU_NULL))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* If it is an IKEv1 policy then at least one phase 1 IKE exchange
     * attribute must be specified.
     */
    else if(
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        (policy->ike_version == IKE_VERSION_1) &&
#endif
       ((policy->ike_xchg1_attribs_no == 0) ||
            (policy->ike_xchg1_attribs_no > IKE_MAX_TRANSFORMS) ||
            (policy->ike_xchg1_attribs == NU_NULL)))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* If it is an IKEv2 policy then number of exchange attribs represent
     * maximum number of proposals. So added a check for that.
     */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    else if(policy->ike_version == IKE_VERSION_2 &&
        policy->ike_xchg1_attribs_no > IKE2_NUM_OF_PROPOSALS)
    {
        status = IKE_INVALID_PARAMS;
    }
#endif

    /* Make sure required phase 1 exchange modes are specified. */
    else if(
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        (policy->ike_version == IKE_VERSION_1) &&
#endif
        ((
#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)
             (policy->ike_phase1_xchg & IKE_XCHG_MAIN_FLAG)
#endif
#if ((IKE_INCLUDE_AGGR_MODE == NU_TRUE) && \
     (IKE_INCLUDE_MAIN_MODE == NU_TRUE))
             |
#endif
#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
             (policy->ike_phase1_xchg & IKE_XCHG_AGGR_FLAG)
#endif
             ) == 0))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Make sure required phase 2 exchange mode is specified. */
    else if(
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        (policy->ike_version == IKE_VERSION_1) &&
#endif
        ((policy->ike_phase2_xchg & IKE_XCHG_QUICK_FLAG) == 0))
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Loop for each set of exchange attributes. */
        for(i = 0; i< policy->ike_xchg1_attribs_no; i++)
        {

            /* Make sure authentication method is valid. */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            if(policy->ike_version == IKE_VERSION_1)
            {
#endif
                if(
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                    (policy->ike_xchg1_attribs[i].ike_auth_method != IKE_RSA)
#endif
#if ((IKE_INCLUDE_SIG_AUTH == NU_TRUE) && \
    (IKE_INCLUDE_PSK_AUTH == NU_TRUE))
                    &&
#endif
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
                    (policy->ike_xchg1_attribs[i].ike_auth_method != IKE_PSK)
#endif
                    )
                {
                    status = IKE_INVALID_PARAMS;
                    break;
                }

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            }

            else if(policy->ike_version == IKE_VERSION_2)
            {
                if(
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
                    (policy->ike_xchg1_attribs[i].ike_auth_method !=
                     IKE2_AUTH_METHOD_RSA_DS)
#endif
#if ((IKE_INCLUDE_SIG_AUTH == NU_TRUE) && \
    (IKE_INCLUDE_PSK_AUTH == NU_TRUE))
                    &&
#endif
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
                    (policy->ike_xchg1_attribs[i].ike_auth_method !=
                     IKE2_AUTH_METHOD_SKEY_MIC)
#endif
                    )
                {
                    status = IKE_INVALID_PARAMS;
                    break;
                }
            }
#endif

            /* Make sure a valid Diffie-Hellman group is specified. */
            if(IKE_Oakley_Group_Prime(
                        policy->ike_xchg1_attribs[i].ike_group_desc)
                        == NU_NULL)
            {
                status = IKE_INVALID_PARAMS;
                break;
            }

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
            /* If authentication by signatures is being used on our end. */
            else if(IKE_IS_SIGN_METHOD(
                        policy->ike_xchg1_attribs[i].ike_auth_method))
            {
                /* If authentication method is via signature, local
                 * certificate and private key files should be specified.
                 * If peer's certificate is required to be signed by a
                 * particular CA, CA's certificate file should also be
                 * specified.
                 */
                if((policy->ike_xchg1_attribs[i].ike_local_cert_file
                    == NU_NULL) ||
                    (policy->ike_xchg1_attribs[i].ike_local_key_file
                    == NU_NULL))
                {
                    status = IKE_INVALID_PARAMS;
                    break;
                }
            }

            if(((policy->ike_flags & IKE_CA_IN_CERTREQ) != 0) &&
                (policy->ike_xchg1_attribs[i].ike_ca_cert_file
                == NU_NULL))
            {
                status = IKE_INVALID_PARAMS;
                break;
            }

            if(((policy->ike_xchg1_attribs[i].ike_cert_encoding)
                != IKE_X509_FILETYPE_ASN1)
#if (IKE_INCLUDE_PEM == NU_TRUE)
                && ((policy->ike_xchg1_attribs[i].ike_cert_encoding)
                != IKE_X509_FILETYPE_PEM)
#endif
                )
            {
                status = IKE_INVALID_PARAMS;
                break;
            }
#if (IKE_INCLUDE_PEM == NU_TRUE)
            if((policy->ike_xchg1_attribs[i].ike_cert_encoding
                == IKE_X509_FILETYPE_PEM) &&
                (policy->ike_xchg1_attribs[i].ike_pem_callback
                == NU_NULL))
            {
                status = IKE_INVALID_PARAMS;
                break;
            }
#endif

            if(((policy->ike_flags & IKE_VERIFY_AGAINST_CRL) != 0) &&
                (policy->ike_xchg1_attribs[i].ike_crl_file == NU_NULL))
            {
                status =  IKE_INVALID_PARAMS;
                break;
            }

            if(((policy->ike_flags & IKE_INBAND_CERT_XCHG) == 0) &&
                (policy->ike_xchg1_attribs[i].ike_peer_cert_file
                == NU_NULL))
            {
                status =  IKE_INVALID_PARAMS;
                break;
            }
#endif

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
            /* If Aggressive mode is enabled, all exchange attributes
             * must specify the same group.
             */
            if((i != 0) &&
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                (policy->ike_version == IKE_VERSION_1) &&
#endif
               ((policy->ike_phase1_xchg & IKE_XCHG_AGGR_FLAG) != 0))
            {
                if(policy->ike_xchg1_attribs[i].ike_group_desc !=
                   policy->ike_xchg1_attribs[0].ike_group_desc)
                {
                    status = IKE_INVALID_PARAMS;
                    break;
                }
            }
#endif

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            if (policy->ike_version == IKE_VERSION_1)
            {
#endif
                /* Verify that the hash algorithm is supported. */
                status = IKE_HASH_ALGO_INDEX(
                             policy->ike_xchg1_attribs[i].ike_hash_algo,
                             algo_index);

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            }

            else
            {
                /* Verify that the hash algorithm is supported in v2. */
                status = IKE2_HASH_ALGO_INDEX(
                             policy->ike_xchg1_attribs[i].ike_hash_algo,
                             algo_index);
            }
#endif
            if(status != NU_SUCCESS)
            {
                break;
            }

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            if (policy->ike_version == IKE_VERSION_1)
            {
#endif
                /* Verify that the encryption algorithm is supported. */
                status = IKE_ENCRYPTION_ALGO_INDEX(
                            policy->ike_xchg1_attribs[i].ike_encryption_algo,
                            algo_index);

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            }

            else
            {
                /* Verify that the encryption algorithm is supported in v2 */
                status = IKE2_ENCRYPTION_ALGO_INDEX(
                    policy->ike_xchg1_attribs[i].ike_encryption_algo,
                    algo_index);
            }
#endif

            if(status != NU_SUCCESS)
            {
                break;
            }

            /* If encryption key length is specified. */
            if(policy->ike_xchg1_attribs[i].ike_key_len != IKE_WILDCARD)
            {
                /* Make sure this key length is supported. */
                status = IKE_Crypto_Enc_Key_Len(IKE_Encryption_Algos[algo_index].crypto_algo_id,
                                                &(policy->ike_xchg1_attribs[i].ike_key_len));
                if(status != NU_SUCCESS)
                {
                    break;
                }
            }

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            if(policy->ike_version == IKE_VERSION_2)
            {
                /* Verify that the PRF algorithm is supported. */
                status = IKE2_PRF_ALGO_INDEX(
                             policy->ike_xchg1_attribs[i].ike2_prf_algo,
                             algo_index);
            }

            if(status != NU_SUCCESS)
            {
                break;
            }
#endif
        } /* End for loop each exchange attribute. */

        /* Make sure only allowed Phase 1 flags are set. */
        if(
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            (policy->ike_version == IKE_VERSION_1) &&
#endif
            (status == NU_SUCCESS))
        {
            i = 0;

#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)
            i |= IKE_XCHG_MAIN_FLAG;
#endif
#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
            i |= IKE_XCHG_AGGR_FLAG;
#endif
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
            i |= IKE_XCHG_INFO_FLAG;
#endif

            /* If any of the unsupported bits are set. */
            if(((~(UNSIGNED)i) & policy->ike_phase1_xchg) != 0)
            {
                status = IKE_INVALID_PARAMS;
            }
        }

        /* Make sure only allowed Phase 2 flags are set. */
        if((status == NU_SUCCESS) &&
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            (policy->ike_version == IKE_VERSION_1) &&
#endif
           ((~(IKE_XCHG_QUICK_FLAG
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
             | IKE_XCHG_INFO_FLAG
#endif
               ) & policy->ike_phase2_xchg) != 0))
        {
            status = IKE_INVALID_PARAMS;
        }

#if (INCLUDE_IPV6 == NU_TRUE)
        /* If an IPv6 subnet is specified. */
        if((status == NU_SUCCESS) &&
           (policy->ike_select.ike_type == IKE_IPV6_SUBNET))
        {
            /* Convert prefix length to the IPv6 subnet mask. */
            status = IPSEC_Convert_Subnet6(
                         policy->ike_select.ike_addr.ike_ip.ike_ext_addr.
                             ike_addr2);

        }
#endif

        /* Checks for valid flags for IKEv2. */
#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
        if(policy->ike_version == IKE_VERSION_2)
        {
            /* IKE2_SEND_TS_PAIR and IKE2_SEND_TS_RANGE cannot be both enabled
             * at the same time.
             */
            if((policy->ike2_flags & IKE2_SEND_TS_PAIR) &&
            (policy->ike2_flags & IKE2_SEND_TS_RANGE))
            {
                status = IKE_INVALID_PARAMS;
            }
            /* IKE2_SA_REKEY and IKE2_SA_DELETE cannot be both enabled
            * at the same time.
            */
            if((policy->ike2_flags & IKE2_SA_REKEY) &&
                (policy->ike2_flags & IKE2_SA_DELETE))
            {
                status = IKE_INVALID_PARAMS;
            }
        }
#endif
    }

    /* If all parameters are valid. */
    if(status == NU_SUCCESS)
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get the group pointer. */
            status = IKE_Get_Group_Entry(group_name, &group);

            if(status == NU_SUCCESS)
            {
                /* Make sure a policy with the same selector is not
                 * already present in the policy database.
                 */
                if(IKE_Get_Policy_By_Selector(group_name,
                                              &policy->ike_select,
                                              NU_NULL, &find_policy,
                                              IKE_MATCH_SELECTORS_ABS)
                                              == NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Policy with specified selector already exists",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_ALREADY_EXISTS;
                }

                else
                {
                    /* Calculate the total memory size. */
                    mem_size = (sizeof(IKE_POLICY)) +
                               (sizeof(IKE_IPS_ID) * policy->ike_ids_no) +
                               (sizeof(IKE_ATTRIB) *
                                policy->ike_xchg1_attribs_no);

                    /* Allocate the memory for the new policy. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                                (VOID**)&new_policy,
                                                mem_size, NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Normalize the pointer. */
                        new_policy = TLS_Normalize_Ptr(new_policy);

                        /* Copy the policy specified by the caller. */
                        NU_BLOCK_COPY(new_policy, policy,
                                      sizeof(IKE_POLICY));

                        /* Increment the index and then assign the
                         * unique identifier to the policy index.
                         */
                        group->ike_policy_list.ike_next_policy_index++;
                        new_policy->ike_index =
                            group->ike_policy_list.ike_next_policy_index;

                        /* Set all fields of SADB to zero. */
                        UTL_Zero(&new_policy->ike_sa_list,
                                 sizeof(IKE_SADB));

                        /* Set pointer for assigning memory
                         * to policy members.
                         */
                        next_mem = (CHAR*)new_policy + sizeof(IKE_POLICY);

                        /* Assigning memory to the ID list array. */
                        new_policy->ike_ids = (IKE_IPS_ID*)next_mem;

                        /* Copy contents into the memory. */
                        NU_BLOCK_COPY(new_policy->ike_ids, policy->ike_ids,
                            new_policy->ike_ids_no * sizeof(IKE_IPS_ID));

                        /* Increment next_mem for the next item. */
                        next_mem += (new_policy->ike_ids_no *
                                     sizeof(IKE_IPS_ID));

                        /* Assign memory to the attribute list array. */
                        new_policy->ike_xchg1_attribs =
                            (IKE_ATTRIB*)next_mem;

                        /* Copy contents into the memory. */
                        NU_BLOCK_COPY(new_policy->ike_xchg1_attribs,
                                      policy->ike_xchg1_attribs,
                                      new_policy->ike_xchg1_attribs_no *
                                      sizeof(IKE_ATTRIB));

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
                        /* Loop for each set of exchange attributes. */
                        for(i = 0; i < new_policy->ike_xchg1_attribs_no;
                            i++)
                        {
                            /* If authentication method is based on
                             * pre-shared keys.
                             */
                            if(IKE_IS_PSK_METHOD(new_policy->
                                   ike_xchg1_attribs[i].ike_auth_method))
                            {
                                /* Set remote key fields to zero. */
                                new_policy->ike_xchg1_attribs[i].
                                    ike_remote_key     = NU_NULL;
                                new_policy->ike_xchg1_attribs[i].
                                    ike_remote_key_len = 0;
                            }
                        }
#endif

                        /* Add this newly created policy to the list of
                         * policies.
                         */
                        SLL_Enqueue(&(group->ike_policy_list), new_policy);

                        /* Return the policy index. */
                        *index = new_policy->ike_index;
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Failed to get specified IKE group",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IKE semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status. */
    return (status);

} /* IKE_Add_Policy */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Policy_Entry
*
* DESCRIPTION
*
*       This is an internal utility function. It returns
*       the pointer to the required policy, given the group
*       name and the index of the policy being searched.
*
* INPUTS
*
*       *group_name             Name of the group.
*       **ret_policy            Pointer to the policy returned
*                               after the successful request.
*       index                   Index of the policy whose
*                               pointer is needed.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           If policy or group is not found.
*
************************************************************************/
STATUS IKE_Get_Policy_Entry(CHAR *group_name, IKE_POLICY **ret_policy,
                            UINT32 index)
{
    STATUS              status;
    IKE_POLICY_GROUP    *group;
    IKE_POLICY          *policy;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure that the group name given is correct. */
    if(group_name == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure that return policy pointer is not NULL. */
    if(ret_policy == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Get the specified IKE group. */
    status = IKE_Get_Group_Entry(group_name, &group);

    if(status == NU_SUCCESS)
    {
        /* Get the policy list of this group. */
        policy = group->ike_policy_list.ike_flink;

        /* Find the policy corresponding to the passed policy index. */
        while(policy != NU_NULL)
        {
            /* Compare the two indices. */
            if(policy->ike_index == index)
            {
                /* Return the pointer of the policy to the user. */
                *ret_policy = policy;

                /* Leave while loop. */
                break;
            }

            /* Move to the next policy in the list. */
            policy = policy->ike_flink;
        }

        /* If a matching policy was not found. */
        if(policy == NU_NULL)
        {
            status = IKE_NOT_FOUND;
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get specified IKE group",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Policy_Entry */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Policy_By_Selector
*
* DESCRIPTION
*
*       This function searches the policy database to find
*       a policy that matches the given selector. Two methods
*       can be used for matching policy selectors:
*
*       - IKE_MATCH_SELECTORS: The method used for matching
*         incoming IKE packets to the policy selectors.
*       - IKE_MATCH_SELECTORS_ABS: The selectors are
*         matched with absolute value. IP ranges are not
*         compared to single IPs and only the exact same
*         selectors match.
*
* INPUTS
*
*       *group_name             Name of the group to be searched.
*       *selector               Selector to be searched.
*       *start_policy           Searching start point. If this
*                               is NU_NULL, search starts from
*                               the first policy in the group.
*       **ret_policy            On return, this contains the
*                               pointer to the required policy.
*       match                   This is a pointer to the match
*                               function. Valid values are:
*                               IKE_MATCH_SELECTORS or
*                               IKE_MATCH_SELECTORS_ABS.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Policy not found.
*
************************************************************************/
STATUS IKE_Get_Policy_By_Selector(CHAR *group_name,
                                  IKE_POLICY_SELECTOR *selector,
                                  IKE_POLICY *start_policy,
                                  IKE_POLICY **ret_policy,
                                  IKE_SELECTOR_MATCH_FUNC match)
{
    STATUS              status = IKE_NOT_FOUND;
    IKE_POLICY_GROUP    *ret_group;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((group_name == NU_NULL) || (selector == NU_NULL) ||
       (ret_policy == NU_NULL) || (match    == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If start_policy pointer is NULL then start search
     * from the first item of the policy list.
     */
    if(start_policy == NU_NULL)
    {
        /* Get the group pointer. */
        if(IKE_Get_Group_Entry(group_name, &ret_group) == NU_SUCCESS)
        {
            /* Getting the first policy from the policy list. */
            start_policy = ret_group->ike_policy_list.ike_flink;
        }

        else
        {
            NLOG_Error_Log("Failed to get specified IKE group",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Loop for all policies. */
    while(start_policy != NU_NULL)
    {
        /* Compare the two selectors. */
        if(match(&start_policy->ike_select, selector) == NU_TRUE)
        {
            /* Match found. Assign value to the policy pointer. */
            *ret_policy = start_policy;

            /* Set the return status to success. */
            status = NU_SUCCESS;

            /* Break out of the search loop. */
            break;
        }

        /* Move to the next policy. */
        start_policy = start_policy->ike_flink;
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Policy_By_Selector */

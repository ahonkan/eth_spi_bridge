/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/
/* Portions of this program were written by: */
/*************************************************************************
*                                                                         
* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 and 2001 WIDE Project.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this 
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS 
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*                                                                         
*************************************************************************/

/*************************************************************************
*                                                                       
*   FILE NAME                                                              
*                                                                               
*       in6.c                                        
*                                                                       
*   DESCRIPTION                                                           
*              
*       This file contains the functions to find appropriately scoped
*       IPv6 addresses.
*
*   DATA STRUCTURES
*
*       None
*                                                                       
*   FUNCTIONS                                                             
*                
*       in6_ifawithifp
*       in6_compare_addrs
*       in6_matchlen
*       in6_addrscope
*                                                                       
*   DEPENDENCIES                                                          
*        
*       externs.h
*       in6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/in6.h"

extern UINT8 IP6_Loopback_Address[];

STATIC DEV6_IF_ADDRESS *in6_compare_addrs(DEV6_IF_ADDRESS *, 
                                          DEV6_IF_ADDRESS *, UINT8 *);

/***********************************************************************
*   
*   FUNCTION                                                                                                                                 
*                                                                       
*       in6_ifawithifp
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function searches the given interface for the address best
*       matching the scope of the passed in address per the Source
*       Address selection rules outlined by RFC 3484.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *dev_ptr                A pointer to the device data structure.
*       *address                A pointer to the address to match.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       A pointer to the address best matching the destination address 
*       or NU_NULL if no valid addresses exist on the interface.
*                                                                       
*************************************************************************/
UINT8 *in6_ifawithifp(const DV_DEVICE_ENTRY *dev_ptr, UINT8 *dest_addr)
{
    DEV6_IF_ADDRESS *current_addr, *next_addr, *best_addr = NU_NULL;

    /* Get a pointer to the first address in the Candidate Set. */
    current_addr = dev_ptr->dev6_addr_list.dv_head;

    /* If there are no addresses in the Candidate Set, return a null
     * pointer.
     */
    if (!current_addr)
    {
        return (NU_NULL);
    }

    /* If this address matches the destination address. */
    if (memcmp(current_addr->dev6_ip_addr, dest_addr, IP6_ADDR_LEN) == 0)
    {
        /* This is the best address. */
        best_addr = current_addr;
    }

    /* If there is only one address in the Candidate set. */
    else if (!current_addr->dev6_next)
    {
        /* Ensure the address is not detached, anycast, a duplicate or
         * tentative, and check that the scope of the source address 
         * is less than or equal to the scope of the destination 
         * address.
         */
        if ( (!(current_addr->dev6_addr_state & DV6_DETACHED)) &&
             (!(current_addr->dev6_addr_state & DV6_ANYCAST)) && 
             (!(current_addr->dev6_addr_state & DV6_DUPLICATED)) &&
             (!(current_addr->dev6_addr_state & DV6_TENTATIVE)) &&
             (in6_addrscope(current_addr->dev6_ip_addr) <= 
              in6_addrscope(dest_addr)) )
        {
            /* Set the best address as the only address on the 
             * interface. 
             */
            best_addr = current_addr;
        }
    }

    /* Compare each address to the next to find the best matching
     * source address for the destination.
     */
    else
    {
        /* Get a pointer to the next address in the list. */
        next_addr = current_addr->dev6_next;

        /* While there is a next address. */
        while (next_addr)
        {
            /* If the next address is not equal to the destination 
             * address.
             */
            if (memcmp(next_addr->dev6_ip_addr, dest_addr, 
                       IP6_ADDR_LEN) != 0)
            {
                /* Apply the source selection rules to the two addresses
                 * to determine which is a better match.
                 */
                best_addr = in6_compare_addrs(current_addr, next_addr, 
                                              dest_addr);

                /* If one of the two addresses is better than the other,
                 * set the current address to the best address.
                 */
                if (best_addr)
                {
                    current_addr = best_addr;
                }

                /* Otherwise, set the current address to the next address,
                 * and find the better of the next two addresses in the
                 * list.
                 */
                else
                {
                    current_addr = next_addr;
                }
            }

            /* Otherwise, this is the best address. */
            else
            {
                best_addr = next_addr;
                break;
            }

            /* Get a pointer to the next address. */
            next_addr = next_addr->dev6_next;
        }
    }
    
    /* If a best address was found, return that address. */
    if (best_addr)
        return (best_addr->dev6_ip_addr);

    /* Otherwise, return a NULL pointer. */
    else
        return (NU_NULL);

} /* in6_ifawithifp */

/***********************************************************************
*   
*   FUNCTION                                                                                                                                 
*                                                                       
*       in6_compare_addrs
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function performs the comparison rules for Source Address
*       Selection as outlined in RFC 3484 and returns a pointer to the
*       "better" of two addresses.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *sa                     A pointer to the first IPv6 address
*                               under consideration.
*       *sb                     A pointer to the second IPv6 address
*                               under consideration.
*       *dest_addr              A pointer to the destination address
*                               for which a source address is being
*                               searched.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       A pointer to the address best matching the destination address
*       per RFC 3484 processing rules or NU_NULL if neither address
*       is an appropriate match.
*                                                                       
*************************************************************************/
STATIC DEV6_IF_ADDRESS *in6_compare_addrs(DEV6_IF_ADDRESS *sa, 
                                          DEV6_IF_ADDRESS *sb, 
                                          UINT8 *dest_addr)
{   
    INT                     scope_sa, scope_sb, scope_d;
    INT                     sa_length, sb_length;
    DEV6_IF_ADDRESS         *best_addr = NU_NULL;
    IP6_POLICY_TABLE_ENTRY  *sa_policy, *sb_policy, *d_policy;

    /* Determine the scope of the source addresses. */
    scope_sa = in6_addrscope(sa->dev6_ip_addr);
    scope_sb = in6_addrscope(sb->dev6_ip_addr);

    /* If the scope of SA and SB are not the same. */
    if (scope_sa != scope_sb)
    {
        /* Determine the scope of the destination address. */
        scope_d = in6_addrscope(dest_addr);

        /* If the scope of SA is less than the scope of SB. */
        if (scope_sa < scope_sb)
        {
            /* If the scope of SA is less the scope of D. */
            if (scope_sa < scope_d)
            {
                /* Set SA as the best address */
                best_addr = sb;
            }

            /* Otherwise, set SA as the best address. */
            else
            {
                best_addr = sa;
            }
        }

        /* Otherwise, the scope of SB is less than the scope of SA. */
        else
        {   
            /* If the scope of SB is less than the scope of D. */
            if (scope_sb < scope_d)
            {
                /* Set SB as the best address. */
                best_addr = sa;
            }

            /* Otherwise, set SB as the best address. */
            else
            {
                best_addr = sb;   
            }
        }
    }

    /* If no best address has been found. */
    if (!best_addr)
    {
        /* If SA is deprecated. */
        if (sa->dev6_addr_state & DV6_DEPRECATED)
        {
            /* If SB is not deprecated. */
            if (!(sb->dev6_addr_state & DV6_DEPRECATED))
            {
                /* Set SB as the best address. */
                best_addr = sb;
            }
        }

        /* Else if SB is deprecated. */
        else if (sb->dev6_addr_state & DV6_DEPRECATED)
        {
            /* Set SA as the best address. */
            best_addr = sa;
        }
    }

    /* If no best address has been found. */
    if (!best_addr)
    {
        /* Get the policy for the destination. */
        d_policy = IP6_Find_Policy_For_Address(dest_addr);

        /* If there is a policy for the destination. */
        if (d_policy)
        {    
            /* Get the policies for SA and SB. */
            sa_policy = IP6_Find_Policy_For_Address(sa->dev6_ip_addr);
            sb_policy = IP6_Find_Policy_For_Address(sb->dev6_ip_addr);

            /* If the label of SA is equal to the label of D. */
            if ( (sa_policy) && 
                 (sa_policy->policy.label == d_policy->policy.label) )
            {
                /* If the label of SB is not equal to the label of D. */
                if ( (!sb_policy) || 
                     (sb_policy->policy.label != d_policy->policy.label) )
                {           
                    /* Set SA as the best address. */
                    best_addr = sa;
                }
            }

            /* If the label of SB is equal to the label of D. */
            else if ( (sb_policy) && 
                      (sb_policy->policy.label == d_policy->policy.label) )
            {
                /* Set SB as the best address. */
                best_addr = sb;
            }
        }
    }

    /* If no best address has been found. */
    if (!best_addr)
    {
        sa_length = in6_matchlen(sa->dev6_ip_addr, dest_addr);
        sb_length = in6_matchlen(sb->dev6_ip_addr, dest_addr);

        /* If the SA has a longer matching prefix than SB. */
        if (sa_length > sb_length)
        {
            /* Set SA as the best address. */
            best_addr = sa;
        }

        /* If SB matches more than 0 bits of the destination's
         * prefix. 
         */
        else if (sb_length > 0)
        {
            /* Set SB as the best address. */
            best_addr = sb;
        }
    }

    return (best_addr);

} /* in6_compare_addrs */

/***********************************************************************
*   
*   FUNCTION                                                                                                                                 
*                                                                       
*       in6_matchlen
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function determines the number of matching bits of the 
*       two provided addresses.   
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *src                    Address 1.
*       *dst                    Address 2.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The number of matching bits between address 1 and address 2.
*                                                                       
*************************************************************************/
int in6_matchlen(UINT8 *src, UINT8 *dst)
{
    int     match = 0;
    UINT8   *s = src, *d = dst;
    UINT8   *lim = s + 16, r;

    while (s < lim)
    {
        r = (UINT8)(*d++ ^ *s++);

        if (r != 0) 
        {
            while (r < 128) 
            {
                match++;
                r <<= 1;
            }
            break;
        } 
        else
            match += 8;
    }

    return (match);

} /* in6_matchlen */

/***********************************************************************
*   
*   FUNCTION                                                                                                                                 
*                                                                       
*       in6_addrscope
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function determines the scope of the address passed in.   
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *addr                   A pointer to the address.
*                                                                       
*   OUTPUTS                                                               
*                            
*       IPV6_ADDR_SCOPE_NODELOCAL                                           
*       IPV6_ADDR_SCOPE_LINKLOCAL
*       IPV6_ADDR_SCOPE_SITELOCAL
*       IPV6_ADDR_SCOPE_GLOBAL
*                                                                       
*************************************************************************/
int in6_addrscope(const UINT8 *addr)
{
    int scope;

    if (addr[0] == 0xfe) 
    {
        scope = addr[1] & 0xc0;

        switch (scope) 
        {
        case 0x80:
            return (IPV6_ADDR_SCOPE_LINKLOCAL);
        case 0xc0:
            return (IPV6_ADDR_SCOPE_SITELOCAL);
        default:
            return (IPV6_ADDR_SCOPE_GLOBAL); /* just in case */
        }
    }


    if (addr[0] == 0xff) 
    {
        scope = addr[1] & 0x0f;

        /*
         * due to other scope such as reserved,
         * return scope doesn't work.
         */
        switch (scope) 
        {
        case IPV6_ADDR_SCOPE_NODELOCAL:
            return (IPV6_ADDR_SCOPE_NODELOCAL);
        case IPV6_ADDR_SCOPE_LINKLOCAL:
            return (IPV6_ADDR_SCOPE_LINKLOCAL);
        case IPV6_ADDR_SCOPE_SITELOCAL:
            return (IPV6_ADDR_SCOPE_SITELOCAL);
        default:
            return (IPV6_ADDR_SCOPE_GLOBAL);
        }
    }

    if (memcmp(IP6_Loopback_Address, addr, IP6_ADDR_LEN - 1) == 0) 
    {
        if (addr[15] == 1) /* loopback */
            return (IPV6_ADDR_SCOPE_NODELOCAL);
        if (addr[15] == 0) /* unspecified */
            return (IPV6_ADDR_SCOPE_LINKLOCAL);
    }

    return (IPV6_ADDR_SCOPE_GLOBAL);

} /* in6_addrscope */

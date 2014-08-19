/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       sck_cpa.c
*
*   DESCRIPTION
*
*       This files contains functions to copy the foreign address from
*       an IP header into a socket structure and address structure.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Copy_Addresses
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       SCK_Copy_Addresses
*
*   DESCRIPTION
*
*       This function copies the foreign address from the IP header
*       of an incoming packet into the socket structure associated
*       with the communication and the address structure to be
*       returned to the application.
*
*   INPUTS
*
*       *sockptr                A pointer to the socket structure.
*       *buffer                 A pointer to the transport layer
*                               header.
*       *from                   A pointer to the address structure
*                               to fill in.
*       flags                   Flags indicating whether this is an
*                               IPv4 or IPv6 session.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SCK_Copy_Addresses(struct sock_struct *sockptr, UINT8 HUGE *buffer,
                        struct addr_struct *from, UINT16 flags)
{
    /* Get the foreign address. */
#if (INCLUDE_IPV6 == NU_TRUE)

    /* Copy the IPv6 address and set the family type */
    if (flags & NET_IP6)
    {
        NU_BLOCK_COPY(from->id.is_ip_addrs,
                      ((IP6LAYER *)(buffer))->ip6_src, IP6_ADDR_LEN);

        NU_BLOCK_COPY(sockptr->s_foreign_addr.ip_num.is_ip_addrs,
                      ((IP6LAYER *)(buffer))->ip6_src, IP6_ADDR_LEN);

        from->family = SK_FAM_IP6;
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif

#else
    UNUSED_PARAMETER(flags);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    {

#if (INCLUDE_IPV6 == NU_TRUE)

        /* If the application layer is using IPv6 to communicate
         * with an IPv4 node, map the IPv4 address to an IPv6 address.
         */
         if (sockptr->s_family == NU_FAMILY_IP6)
        {
            /* Map the IPv4 addresses to IPv6 IPv4-Mapped addresses */
            IP6_Create_IPv4_Mapped_Addr(from->id.is_ip_addrs,
                                        GET32((IPLAYER *)buffer, IP_SRC_OFFSET));

            IP6_Create_IPv4_Mapped_Addr(sockptr->s_foreign_addr.ip_num.is_ip_addrs,
                                        GET32((IPLAYER *)buffer, IP_SRC_OFFSET));

            /* Set the family */
            from->family = NU_FAMILY_IP6;
        }

        /* This is an IPv4 session.  Return the IPv4 address. */
        else
#endif
        {
            *(UINT32 *)from->id.is_ip_addrs =
                LONGSWAP(GET32((IPLAYER *)buffer, IP_SRC_OFFSET));

            *(UINT32 *)sockptr->s_foreign_addr.ip_num.is_ip_addrs =
                LONGSWAP(GET32((IPLAYER *)buffer, IP_SRC_OFFSET));

            from->family = SK_FAM_IP;
        }
    }
#endif

} /* SCK_Copy_Addresses */

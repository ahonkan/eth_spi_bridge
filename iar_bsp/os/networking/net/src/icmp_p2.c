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
*       icmp_p2.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Ping2.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ping2
*
*   DEPENDENCIES
*
*       nu_net.h
*       externs6.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ping2
*
*   DESCRIPTION
*
*       This function sends an ICMP Echo Request message to a node of
*       family type and waits timeout ticks for a reply.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address of the node
*                               to which to send the ICMP Echo Request.
*       timeout                 The number of ticks to wait for the reply.
*       family                  The family type of the destination node.
*                               Either NU_FAMILY_IP for an IPv4 node or
*                               NU_FAMILY_IP6 for an IPv6 node.
*
*   OUTPUTS
*
*      NU_SUCCESS               Success
*      NU_INVALID_PARM          One of the parameters is invalid
*      NU_NO_ACTION             Could not be sent
*      NU_MEM_ALLOC             Insufficient memory
*      NU_NO_BUFFERS            Insufficient buffers
*
*************************************************************************/
STATUS NU_Ping2(UINT8 *ip_addr, UINT32 timeout, INT16 family)
{
    STATUS  status;

    if (ip_addr != NU_NULL)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        /* Send an ICMP Echo Request to an IPv4 node */
        if (family == SK_FAM_IP)
            status = ICMP_Send_Echo_Request(ip_addr, timeout);

#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

        /* Send an ICMPv6 Echo Request to an IPv6 node */
        if (family == SK_FAM_IP6)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            /* If this is an IPv4-Mapped IPv6 address, do an IPv4 PING
             * on the IPv4 portion of the address.
             */
            if (IPV6_IS_ADDR_V4MAPPED(ip_addr))
                status = ICMP_Send_Echo_Request(&ip_addr[12], timeout);

            /* Otherwise, do an IPv6 PING on the IPv6 address */
            else
#endif
                status = ICMP6_Send_Echo_Request(ip_addr, timeout);
        }

#endif
        /* The family type is not recognized */
        else
            status = NU_INVALID_PARM;
    }
    else
        status = NU_INVALID_PARM;

    return (status);

} /* NU_Ping2 */



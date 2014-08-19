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
*   FILENAME
*
*       prt.c
*
*   COMPONENT
*
*       PRT -- TCP/UDP ports.
*
*   DESCRIPTION
*
*       Session interface routines
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PRT_Get_Unique_Port_Number
*       PRT_Is_Unique_Port_Number
*       PRT_Find_Matching_Port
*
*   DEPENDENCIES
*
*       nu_net.h
*       externs6.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

/*************************************************************************
*
*   FUNCTION
*
*       PRT_Get_Unique_Port_Number
*
*   DESCRIPTION
*
*       This function derives a new port number and then searches
*       both the TCP_Ports and the UDP_Ports to see if it has
*       already been used.
*
*   INPUTS
*
*       protocol                whether it is TCP or UDP Protocol
*       family                  the family - either SK_FAM_IP for IPv4
*                               or SK_FAM_IP6 for IPv6.
*
*   OUTPUTS
*
*       UINT16                  Port number.
*
*************************************************************************/
UINT16 PRT_Get_Unique_Port_Number(UINT16 protocol, INT16 family)
{
     UINT16  i=0;

     while (i==0)
     {
        i = (UINT16) UTL_Rand ();           /* get a unique number */
        i = (UINT16) (i | 2048);            /* make sure >= 0x800 */

        /*  search for an un-used port.  Search as long as we have
         *  not searched all ports and we have not found an existing port
         *  of the same number as the new one.
         */
        if (PRT_Is_Unique_Port_Number(protocol, i, family, NU_NULL,
                                      1) == NU_FALSE)
            i=0;
     }

     return(i);

} /* PRT_Get_Unique_Port_Number */

/*************************************************************************
*
*   FUNCTION
*
*       PRT_Is_Unique_Port_Number
*
*   DESCRIPTION
*
*       This function searches both the TCP_Ports and the UDP_Ports to
*       see if it has already been used.
*
*   INPUTS
*
*       protocol                whether it is TCP or UDP Protocol
*       port                    the port number to search for.
*       family                  the family - either SK_FAM_IP for IPv4
*                               or SK_FAM_IP6 for IPv6.
*       *local_addr             A pointer to the local address.
*       strict                  Strict or not strict.  If creating a new
*                               port, set.  If binding to a port, do
*                               not set.
*
*   OUTPUTS
*
*       INT                     NU_TRUE or NU_FALSE
*
*************************************************************************/
INT PRT_Is_Unique_Port_Number(UINT16 protocol, UINT16 port, INT16 family,
                              const UINT8 *local_addr, UINT8 strict)
{
    INT     j;

    if (strict)
    {
#if (INCLUDE_TCP == NU_TRUE)
        if (protocol == NU_PROTO_TCP)
        {
            /* If the port is not NULL, the local port matches the port
             * passed in, and the connection is not CLOSED, this port is
             * not unique.
             */
            for (j=0; j<TCP_MAX_PORTS; j++)
            {
                if ( (TCP_Ports[j] != NU_NULL) &&
                     (port == TCP_Ports[j]->in.port) &&
                     (TCP_Ports[j]->state != SCLOSED) )
                    return (NU_FALSE);
            }
        }
#endif

#if (INCLUDE_UDP == NU_TRUE)
        if (protocol == NU_PROTO_UDP)
        {
            /* If the port is not NULL and the local port matches the port
             * passed in, this port is not unique.
             */
            for (j=0; j < UDP_MAX_PORTS; j++)
            {
                if ( (UDP_Ports[j] != NU_NULL) && (port == UDP_Ports[j]->up_lport))
                    return (NU_FALSE);
            }
        }
#endif
    }

    else
    {
        /* Check that there are no active connections using this port */
        for (j = 0; j < NSOCKETS; j++)
        {
            /* If the socket is not NULL, the local port number matches
             * the port number passed in, the protocols match, the families
             * match and the local address passed in is the WILDCARD address
             * or the local address passed in matches the local address in
             * the socket structure, this is not a unique port number.
             */
            if ( (SCK_Sockets[j] != NU_NULL) &&
                 (SCK_Sockets[j]->s_local_addr.port_num == port) &&
                 (SCK_Sockets[j]->s_protocol == protocol) &&
                 (SCK_Sockets[j]->s_family == family) &&
                 ((local_addr == NU_NULL) ||

#if (INCLUDE_IPV4 == NU_TRUE)
                 ((family == NU_FAMILY_IP) &&
                  (IP_ADDR(SCK_Sockets[j]->s_local_addr.ip_num.is_ip_addrs) ==
                  (IP_ADDR(local_addr))))
#if (INCLUDE_IPV6 == NU_TRUE)
                 ||
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                 ((family == NU_FAMILY_IP6) &&
                  (memcmp(SCK_Sockets[j]->s_local_addr.ip_num.is_ip_addrs,
                          local_addr, IP6_ADDR_LEN) == 0))
#endif
               ) )
                return (NU_FALSE);
        }
    }

    return (NU_TRUE);

} /* PRT_Is_Unique_Port_Number */

/*************************************************************************
*
*   FUNCTION
*
*       PRT_Find_Matching_Port
*
*   DESCRIPTION
*
*       This function searches the specified protocol's port list for
*       an entry matching the parameters provided.
*
*   INPUTS
*
*       family                  The family - either SK_FAM_IP for IPv4
*                               or SK_FAM_IP6 for IPv6.
*       protocol                whether it is TCP or UDP Protocol
*       *source_ip              A pointer to the source IP address.
*       *dest_ip                A pointer to the destination IP address.
*       source_port             The source port.
*       dest_port               The destination port.
*
*   OUTPUTS
*
*       The port index corresponding to the target port.
*       -1 if no entry found.
*
*************************************************************************/
INT32 PRT_Find_Matching_Port(INT16 family, INT protocol,
                             const UINT8 *source_ip, const UINT8 *dest_ip,
                             UINT16 source_port, UINT16 dest_port)
{
    INT32  status;

    switch (family)
    {
#if (INCLUDE_IPV4 == NU_TRUE)

    case NU_FAMILY_IP:

        status = PRT4_Find_Matching_Port(protocol, IP_ADDR(source_ip),
                                         IP_ADDR(dest_ip), source_port,
                                         dest_port);

        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    case NU_FAMILY_IP6:

        status = PRT6_Find_Matching_Port(protocol, source_ip, dest_ip, source_port,
                                         dest_port);

        break;

#endif

    default:

        status = -1;
        break;
    }

    return (status);

} /* PRT_Find_Matching_Port */

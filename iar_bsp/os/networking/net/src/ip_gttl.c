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
*       ip_gttl.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the default Time To Live
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_TTL
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IP_RAW == NU_TRUE)
#include "networking/ipraw.h"
#endif

/*************************************************************************
*
*   FUNCTION
*
*       IP_Getsockopt_IP_TTL
*
*   DESCRIPTION
*
*       This function gets the default TTL.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the default TTL.
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Getsockopt_IP_TTL(INT socketd, UINT16 *optval, INT *optlen)
{
    switch (SCK_Sockets[socketd]->s_protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)

        case NU_PROTO_TCP:

            *optval = TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_ttl;
            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case NU_PROTO_UDP:

            *optval = UDP_Ports[SCK_Sockets[socketd]->s_port_index]->up_ttl;
            break;
#endif

        default:

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* If this is a RAW socket */
            if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
            {
                *optval = IPR_Ports[SCK_Sockets[socketd]->s_port_index]->ip_ttl;
            }
#endif

            break;
    }

    /* If no TTL has been configured for this socket, then the default
     * is being used.  Return the default value.
     */
    if (*optval == 0)
        *optval = IP_Time_To_Live;

    *optlen = sizeof(UINT16);

} /* IP_Getsockopt_IP_TTL */

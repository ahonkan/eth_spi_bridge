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
*       ip_sttl.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the default Time To Live
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_TTL
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
*       IP_Setsockopt_IP_TTL
*
*   DESCRIPTION
*
*       This function sets the default TTL.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       sck_ttl                 The new default Time To Live.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Setsockopt_IP_TTL(INT socketd, UINT16 sck_ttl)
{
#if ( (INCLUDE_TCP != NU_TRUE) && (INCLUDE_UDP != NU_TRUE) && \
      (INCLUDE_IP_RAW != NU_TRUE) )
    UNUSED_PARAMETER(sck_ttl);
#endif

    switch (SCK_Sockets[socketd]->s_protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)

        case NU_PROTO_TCP:

            TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_ttl = sck_ttl;
            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case NU_PROTO_UDP:

            UDP_Ports[SCK_Sockets[socketd]->s_port_index]->up_ttl = sck_ttl;
            break;
#endif

        default:

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* If this is a RAW socket */
            if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
            {
                IPR_Ports[SCK_Sockets[socketd]->s_port_index]->ip_ttl = sck_ttl;
            }
#endif

            break;
    }

} /* IP_Setsockopt_IP_TTL */

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
*       ip_gtos.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the Differentiated Service
*       (formally known as Type of Service) field for this socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_TOS
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
*       IP_Getsockopt_IP_TOS
*
*   DESCRIPTION
*
*       This function gets the default DSCP (formally TOS).
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the DSCP field.
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Getsockopt_IP_TOS(INT socketd, UINT8 *optval, INT *optlen)
{
#if ( (INCLUDE_TCP != NU_TRUE) && (INCLUDE_UDP != NU_TRUE) && \
      (INCLUDE_IP_RAW != NU_TRUE) )
    UNUSED_PARAMETER(optval);
#endif

    switch (SCK_Sockets[socketd]->s_protocol)
    {
#if (INCLUDE_TCP == NU_TRUE)

        case NU_PROTO_TCP:

            /* Check that the port structure is still valid */
            if (TCP_Ports[SCK_Sockets[socketd]->s_port_index])
            {
                *optval =
                    (UINT8)(TCP_Ports[SCK_Sockets[socketd]->
                                      s_port_index]->p_tos >> 2);

                *optlen =
                    sizeof(TCP_Ports[SCK_Sockets[socketd]->
                                     s_port_index]->p_tos);
            }

            else
                *optlen = 0;

            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case NU_PROTO_UDP:

            /* Check that the port structure is still valid */
            if (UDP_Ports[SCK_Sockets[socketd]->s_port_index])
            {
                *optval =
                    (UINT8)(UDP_Ports[SCK_Sockets[socketd]->
                                      s_port_index]->up_tos >> 2);

                *optlen =
                    sizeof(UDP_Ports[SCK_Sockets[socketd]->
                                     s_port_index]->up_tos);
            }

            else
                *optlen = 0;

            break;
#endif

        default:

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* If this is a RAW socket */
            if (IS_RAW_PROTOCOL(SCK_Sockets[socketd]->s_protocol))
            {
                /* Check that the port structure is still valid */
                if (IPR_Ports[SCK_Sockets[socketd]->s_port_index])
                {
                    *optval =
                        (UINT8)(IPR_Ports[SCK_Sockets[socketd]->
                                          s_port_index]->ip_tos >> 2);

                    *optlen =
                        sizeof(IPR_Ports[SCK_Sockets[socketd]->
                                         s_port_index]->ip_tos);
                }
            }

            else
#endif
                *optlen = 0;

            break;
    }

} /* IP_Getsockopt_IP_TOS */

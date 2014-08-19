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
*       ip_stos.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the Type of Service field
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_TOS
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
*       IP_Setsockopt_IP_TOS
*
*   DESCRIPTION
*
*       This function sets the Type of Service (TOS) field.
*
*       Note : Type of Service (TOS) has been replaced by Diff Serv Code
*       Point (DSCP) per RFC 2474.  This parameter is still called IP_TOS
*       but is used to set the DSCP to a value between 0 and 63.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       dscp                    The new TOS value.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was completed successfully.
*       NU_INVAL                The value of dscp is invalid.
*
*************************************************************************/
STATUS IP_Setsockopt_IP_TOS(INT socketd, UINT8 dscp)
{
    STATUS  status;

#if ( (INCLUDE_TCP != NU_TRUE) && (INCLUDE_UDP != NU_TRUE) && \
      (INCLUDE_IP_RAW != NU_TRUE) )
    UNUSED_PARAMETER(dscp);
#endif

    /* The DSCP field must be between 0 and 63, inclusive */
    if (dscp < 64)
    {
        status = NU_SUCCESS;

        switch (SCK_Sockets[socketd]->s_protocol)
        {
#if (INCLUDE_TCP == NU_TRUE)

            case NU_PROTO_TCP:

                /* Check that the port structure is still valid */
                if (TCP_Ports[SCK_Sockets[socketd]->s_port_index])
                {
                    /* Set the new TOS - Bits 7 and 6 are not used. */
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_tos =
                        (UINT8)(dscp << 2);
                }

                break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

            case NU_PROTO_UDP:

                /* Check that the port structure is still valid */
                if (UDP_Ports[SCK_Sockets[socketd]->s_port_index])
                {
                    /* Set the new TOS - Bits 7 and 6 are not used. */
                    UDP_Ports[SCK_Sockets[socketd]->s_port_index]->up_tos =
                        (UINT8)(dscp << 2);
                }

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
                        /* Set the new TOS - Bits 7 and 6 are not used. */
                        IPR_Ports[SCK_Sockets[socketd]->s_port_index]->ip_tos =
                            (UINT8)(dscp << 2);
                    }
                }
#endif

                break;
        }
    }

    else
        status = NU_INVAL;

    return (status);

} /* IP_Setsockopt_IP_TOS */

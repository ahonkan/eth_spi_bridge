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
*       tcp_skar2.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the maximum number of
*       unanswered keep-alive probes to transmit on a TCP socket at
*       run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_KEEPALIVE_R2
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Setsockopt_TCP_KEEPALIVE_R2
*
*   DESCRIPTION
*
*       Configure the maximum number of unanswered keep-alive probes to
*       transmit on a TCP socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       max_retrans             Maximum number of retransmissions of
*                               unanswered Keep-Alive packets for the
*                               specified socket before the connection is
*                               closed.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*       NU_INVAL                The value of the max_retrans parameter is
*                               not a positive integer.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_KEEPALIVE_R2(INT socketd, UINT8 max_retrans)
{
    INT     pindex;
    STATUS  status;

    /* Validate the max_retrans value. */
    if (max_retrans != 0)
    {
        /* Retrieve the port index. */
        pindex = SCK_Sockets[socketd]->s_port_index;

        /* If the TCP port is valid, configure the option. */
        if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
        {
            /* Set the value for the connection. */
            TCP_Ports[pindex]->p_ka_r2 = max_retrans;

            status = NU_SUCCESS;
        }

        else
        {
            status = NU_INVALID_SOCKET;
        }
    }

    /* The input is invalid. */
    else
    {
        status = NU_INVAL;
    }

    return (status);

} /* TCP_Setsockopt_TCP_KEEPALIVE_R2 */

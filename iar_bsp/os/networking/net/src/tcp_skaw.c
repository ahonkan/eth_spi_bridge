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
*       tcp_skaw.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the amount of time to
*       remain idle on a TCP socket before invoking the TCP keep-alive
*       mechanism at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_KEEPALIVE_WAIT
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
*       TCP_Setsockopt_TCP_KEEPALIVE_WAIT
*
*   DESCRIPTION
*
*       Configure the amount of time to remain idle on a TCP socket before
*       invoking the TCP keep-alive mechanism at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       delay                   Value in hardware ticks to allow a
*                               connection to remain idle before invoking
*                               the keep-alive protocol.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*       NU_INVAL                The delay parameter is set to zero.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 delay)
{
    INT     pindex;
    STATUS  status;

    /* Validate the delay value. */
    if (delay != 0)
    {
        /* Retrieve the port index. */
        pindex = SCK_Sockets[socketd]->s_port_index;

        /* If the TCP port is valid, configure the option. */
        if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
        {
            /* Set the value for the connection. */
            TCP_Ports[pindex]->p_ka_wait = delay;

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

} /* TCP_Setsockopt_TCP_KEEPALIVE_WAIT */

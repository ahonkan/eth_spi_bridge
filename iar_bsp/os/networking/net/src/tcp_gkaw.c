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
*       tcp_gkaw.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the time value to
*       remain idle on a TCP socket before invoking the TCP keep-alive
*       module at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_KEEPALIVE_WAIT
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
*       TCP_Getsockopt_TCP_KEEPALIVE_WAIT
*
*   DESCRIPTION
*
*       Retrieve the time value to remain idle on a TCP socket before
*       invoking the TCP keep-alive protocol at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to retrieve the value.
*       *delay                  The pointer to memory in which to store
*                               the value in hardware ticks to allow a
*                               connection to remain idle before invoking
*                               the keep-alive protocol.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 *delay)
{
    INT     pindex;
    STATUS  status;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, return the value of the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Store the value in the memory provided. */
        *delay = TCP_Ports[pindex]->p_ka_wait;

        status = NU_SUCCESS;
    }

    /* The socket is invalid. */
    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_KEEPALIVE_WAIT */

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
*       tcp_gfrto.c
*
*   DESCRIPTION
*
*       This file contains the routine to Retrieve the value of the first
*       retransmission timeout on a TCP socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_FIRST_RTO
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
*       TCP_Getsockopt_TCP_FIRST_RTO
*
*   DESCRIPTION
*
*       Retrieve the value of the first retransmission timeout on a TCP
*       socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to retrieve the value.
*       *timeout                Pointer to the memory in which to store
*                               the value in hardware ticks to delay
*                               before sending the first retransmission
*                               of an unACKed data packet for the
*                               specified socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_FIRST_RTO(INT socketd, INT32 *timeout)
{
    INT     pindex;
    STATUS  status;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, return the value of the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Store the value in the memory provided. */
        *timeout = TCP_Ports[pindex]->p_first_rto;

        status = NU_SUCCESS;
    }

    /* The socket is invalid. */
    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_FIRST_RTO */

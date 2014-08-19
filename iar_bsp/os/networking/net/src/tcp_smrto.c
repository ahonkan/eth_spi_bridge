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
*       tcp_smrto.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the value of the
*       maximum retransmission timeout on a TCP socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_MAX_RTO
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
*       TCP_Setsockopt_TCP_MAX_RTO
*
*   DESCRIPTION
*
*       Configure the value of the maximum retransmission timeout on a TCP
*       socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       timeout                 Maximum value in hardware ticks to delay
*                               between successive retransmissions of an
*                               unACKed data packet for the specified
*                               socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*       NU_INVAL                The value of the timeout parameter is not
*                               a positive integer.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_MAX_RTO(INT socketd, UINT32 timeout)
{
    INT     pindex;
    STATUS  status;

    /* Validate the timeout value. */
    if (timeout > 0)
    {
        /* Retrieve the port index. */
        pindex = SCK_Sockets[socketd]->s_port_index;

        /* If the TCP port is valid, configure the option. */
        if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
        {
            /* Set the value for the connection. */
            TCP_Ports[pindex]->p_max_rto = timeout;

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

} /* TCP_Setsockopt_TCP_MAX_RTO */

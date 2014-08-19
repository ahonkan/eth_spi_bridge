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
*       tcp_smsr2.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the number of times to
*       retransmit a SYN packet before closing the connection on a TCP
*       socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_MAX_SYN_R2
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
*       TCP_Setsockopt_TCP_MAX_SYN_R2
*
*   DESCRIPTION
*
*       Configure the number of times to retransmit a SYN packet before
*       closing the connection on a TCP socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       max_retrans             Maximum number of retransmissions of an
*                               unACKed SYN packet for the specified
*                               socket before the connection is closed.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*       NU_INVAL                A connection has already been established
*                               on this socket; therefore, this value
*                               cannot be set.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_MAX_SYN_R2(INT socketd, UINT8 max_retrans)
{
    INT     pindex;
    STATUS  status;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, configure the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Ensure a connection has not already been established on the
         * socket.
         */
        if (TCP_Ports[pindex]->state == SREADY)
        {
            /* Set the value for the connection. */
            TCP_Ports[pindex]->p_max_syn_r2 = max_retrans;

            status = NU_SUCCESS;
        }

        /* The socket is not in the READY state. */
        else
        {
            status = NU_INVAL;
        }
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Setsockopt_TCP_MAX_SYN_R2 */

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
*       tcp_gmr2.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the number of times to
*       retransmit a data segment before closing the connection on a TCP
*       socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_MAX_R2
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
*       TCP_Getsockopt_TCP_MAX_R2
*
*   DESCRIPTION
*
*       Retrieve the number of times to retransmit a data segment before
*       closing the connection on a TCP socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to retrieve the value.
*       *max_retrans            Pointer to the memory in which to store
*                               the maximum number of retransmissions of
*                               an unACKed data packet for the specified
*                               socket before the connection is closed.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_MAX_R2(INT socketd, UINT8 *max_retrans)
{
    INT     pindex;
    STATUS  status;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, return the value of the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Store the value in the memory provided. */
        *max_retrans = TCP_Ports[pindex]->p_max_r2;

        status = NU_SUCCESS;
    }

    /* The socket is invalid. */
    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_MAX_R2 */

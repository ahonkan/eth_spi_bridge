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
*       tcp_gcc.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine whether TCP
*       Congestion Control is set on a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_CONGESTION_CTRL
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
*       TCP_Getsockopt_TCP_CONGESTION_CTRL
*
*   DESCRIPTION
*
*       This function determines whether Congestion Control is set on
*       a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the Congestion Control
*                               algorithm.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The socket is invalid.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 *optval)
{
    INT     pindex;
    STATUS  status;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, return the value of the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Get the TCP Congestion Control flag */
        if (!(TCP_Ports[pindex]->portFlags & TCP_DIS_CONGESTION))
            *optval = 1;

        /* Congestion control is disabled. */
        else
            *optval = 0;

        status = NU_SUCCESS;
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_CONGESTION_CTRL */

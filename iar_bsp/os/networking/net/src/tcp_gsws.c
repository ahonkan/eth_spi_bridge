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
*       tcp_gsws.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the foreign window
*       size for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_SND_WINDOWSIZE
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
*       TCP_Getsockopt_TCP_SND_WINDOWSIZE
*
*   DESCRIPTION
*
*       This function retrieves the foreign window size for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The foreign window size for the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       Invalid port structure.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_SND_WINDOWSIZE(INT socketd, UINT32 *optval)
{
    STATUS  status = NU_SUCCESS;
    INT     pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Return the foreign window size. */
        *optval = TCP_Ports[pindex]->out.size;
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_SND_WINDOWSIZE */

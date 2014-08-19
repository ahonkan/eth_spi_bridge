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
*       tcp_gcsack.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine whether TCP
*       SACK support is set on a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_CFG_SACK
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
*       TCP_Getsockopt_TCP_CFG_SACK
*
*   DESCRIPTION
*
*       This function determines whether TCP SACK support is set on
*       a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of SACK support on the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       Invalid port structure.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_CFG_SACK(INT socketd, UINT8 *optval)
{
    STATUS  status;
    INT     pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, return the value of the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Get the SACK support flag */
        if (TCP_Ports[pindex]->portFlags & TCP_SACK)
            *optval = 1;

        /* Unset the SACK support flag */
        else
            *optval = 0;

        status = NU_SUCCESS;
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_CFG_SACK */

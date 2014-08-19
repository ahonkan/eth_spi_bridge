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
*       tcp_gws.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine whether TCP
*       Window Scale Option support is set on a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_WINDOWSCALE
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
*       TCP_Getsockopt_TCP_WINDOWSCALE
*
*   DESCRIPTION
*
*       This function determines whether TCP Window Scale Option support is
*       set on a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of Window Scale option support on
*                               the socket.  If disabled, this value
*                               will be negative.  If enabled, the value will
*                               contain the scale factor for the Window
*                               Scale option.  A value of zero indicates
*                               a scale factor of 1; ie, no scaling.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       Invalid port structure.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_WINDOWSCALE(INT socketd, INT *optval)
{
    STATUS  status;
    INT     pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Get the Window Scale option value. */
        if (TCP_Ports[pindex]->portFlags & TCP_REPORT_WINDOWSCALE)
            *optval = (INT)TCP_Ports[pindex]->in.p_win_shift;

        /* Window Scale is not enabled - return -1. */
        else
            *optval = -1;

        status = NU_SUCCESS;
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Getsockopt_TCP_WINDOWSCALE */

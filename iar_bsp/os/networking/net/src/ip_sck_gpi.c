/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       ip_sck_gpi.c
*
*   DESCRIPTION
*
*       This file contains the routines for getting the IPv4 sticky option
*       IP_PKTINFO on a socket.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_PKTINFO
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       IP_Getsockopt_IP_PKTINFO
*
*   DESCRIPTION
*
*       This function retrieves the value of the socket option to
*       return the destination address and interface index of the datagram
*       received via the NU_Recvmsg() call.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A value of zero disables the option on
*                               the socket.  A non-zero value enables the
*                               option on the socket.
*       *optlen                 The size of memory pointed to by optval
*                               on input, and the size of data copied into
*                               optval on return from the function.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_PARM         One of the input parameters is invalid or
*                               optlen is too small.
*
*************************************************************************/
STATUS IP_Getsockopt_IP_PKTINFO(INT socketd, INT *optval, INT *optlen)
{
    if ( (!optlen) || (!optval) || (*optlen < sizeof(INT)))
        return (NU_INVALID_PARM);

    *optlen = sizeof(INT);

    if (SCK_Sockets[socketd]->s_options & SO_IP_PKTINFO_OP)
        *optval = 1;
    else
        *optval = 0;

    return (NU_SUCCESS);

} /* IP_Getsockopt_IP_PKTINFO */

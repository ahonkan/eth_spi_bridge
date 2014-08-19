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
*       ip_sck_spi.c
*
*   DESCRIPTION
*
*       This file contains the routines for setting the IPv4 sticky option
*       IP_PKTINFO on a socket.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_PKTINFO
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
*       IP_Setsockopt_IP_PKTINFO
*
*   DESCRIPTION
*
*       This function sets the value of the socket option to
*       return the destination address and interface index of the datagram
*       received via the NU_Recvmsg() call.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  A value of zero disables the option on
*                               the socket.  A non-zero value enables the
*                               option on the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The socket is a TCP socket.
*
*************************************************************************/
STATUS IP_Setsockopt_IP_PKTINFO(INT socketd, INT optval)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    STATUS  status;

    if (sck_ptr->s_protocol != NU_PROTO_TCP)
    {
        /* Enable the option on the socket */
        if (optval)
            sck_ptr->s_options |= SO_IP_PKTINFO_OP;

        /* Disable the option on the socket */
        else
            sck_ptr->s_options &= ~SO_IP_PKTINFO_OP;

        status = NU_SUCCESS;
    }

    else
        status = NU_INVALID_SOCKET;

    return (status);

} /* IP_Setsockopt_IP_PKTINFO */

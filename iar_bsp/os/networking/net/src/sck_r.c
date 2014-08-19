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
* FILE NAME
*
*       sck_r.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Recv.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Recv
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Recv
*
*   DESCRIPTION
*
*       This function will handle receiving data across a network during a
*       connection oriented transfer.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the max number of bytes of data
*       flags                   This parameter is used for socket compatibility
*                               but we are currently not making any use of it
*
*   OUTPUTS
*
*       Number of bytes received.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_NOT_CONNECTED        The read side of the socket has been closed
*                               by the application, both the read and write
*                               side of the socket have been closed by the
*                               application or the connection has been reset
*                               by the other side.
*       NU_WOULD_BLOCK          No data is available, and the socket is
*                               non-blocking.
*       NU_NO_ROUTE_TO_HOST     This is an icmp_error if no route to host exist
*       NU_CONNECTION_REFUSED   This is an icmp_error if the connection is refused.
*       NU_MSG_TOO_LONG         This is an icmp_error if the message is too large.
*       NU_CONNECTION_TIMED_OUT TCP Keep-Alive packets found that the
*                               connection has timed out.
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for the socket:
*
*       NU_DEST_UNREACH_ADMIN
*       NU_DEST_UNREACH_ADDRESS
*       NU_DEST_UNREACH_PORT
*       NU_TIME_EXCEED_HOPLIMIT
*       NU_TIME_EXCEED_REASM
*       NU_PARM_PROB_HEADER
*       NU_PARM_PROB_NEXT_HDR
*       NU_PARM_PROB_OPTION
*       NU_DEST_UNREACH_NET
*       NU_DEST_UNREACH_HOST
*       NU_DEST_UNREACH_PROT
*       NU_DEST_UNREACH_FRAG
*       NU_DEST_UNREACH_SRCFAIL
*       NU_PARM_PROB
*       NU_SOURCE_QUENCH
*
*************************************************************************/
INT32 NU_Recv(INT socketd, CHAR *buff, UINT16 nbytes, INT16 flags)
{
    INT32               return_status;

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
#if ( (INCLUDE_TCP == NU_FALSE) && (INCLUDE_UDP == NU_FALSE) )
        UNUSED_PARAMETER(nbytes);
        UNUSED_PARAMETER(buff);
#endif

        /*  Clean up warnings.  This parameter is used for socket compatibility
            but we are currently not making any use of it.  */
        UNUSED_PARAMETER(flags);

#if (INCLUDE_TCP == NU_TRUE)

        /* Call the routine to receive the TCP data pending on the socket */
        if (SCK_Sockets[socketd]->s_protocol == NU_PROTO_TCP)
            return_status = TCPSS_Recv_Data(socketd, buff, nbytes);

        else

#endif

#if (INCLUDE_UDP == NU_TRUE)

        /* Call the routine to receive the UDP data pending on the socket */
        if ( (SCK_Sockets[socketd]->s_protocol == NU_PROTO_UDP) &&
             (SCK_Sockets[socketd]->s_state & SS_ISCONNECTED) )
            return_status = UDP_Recv_Data(socketd, buff, nbytes, NU_NULL);

        else

#endif
            /* This socket cannot receive data using NU_Recv */
            return_status = NU_INVALID_SOCKET;

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    /* return to caller */
    return (return_status);

} /* NU_Recv */

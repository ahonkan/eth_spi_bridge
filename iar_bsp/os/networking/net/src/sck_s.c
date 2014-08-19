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
*       sck_s.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Send.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Send
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
*       NU_Send
*
*   DESCRIPTION
*
*       This function is responsible for transmitting data across a
*       network during a connection-oriented transfer.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the number of bytes of data. Valid
*                               values for this parameter are in the range
*                               0-IP_MAX_DATA_SIZE(65,495).
*       flags                   This parameter is used for socket
*                               compatibility we are currently not making
*                               any use of it
*
*   OUTPUTS
*
*       > 0                     The number of bytes sent.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously allocated
*                               via the NU_Socket call.
*       NU_NOT_CONNECTED        The data transfer was not completed. This
*                               probably occurred because the connection
*                               was closed for some reason.
*       NU_NO_ROUTE_TO_HOST     This is an icmp_error if no route to host exist
*       NU_CONNECTION_REFUSED   This is an icmp_error if the connection is refused.
*       NU_MSG_TOO_LONG         This is an icmp_error if the message is too large.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource, but
*                               the socket is non-blocking.
*       NU_NO_PORT_NUMBER       No local port number was stored in the socket
*                               descriptor.
*       NU_CONNECTION_TIMED_OUT TCP Keep-Alive packets found that the
*                               connection has timed out.
*       NU_INVALID_PARM         The buffer of data to send is NULL, and
*                               the number of bytes is not zero.
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
INT32 NU_Send(INT socketd, CHAR *buff, UINT16 nbytes, INT16 flags)
{
    INT32                           return_status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the buffer is NULL and the number of bytes to send is not zero */
    if ( (buff == NU_NULL) && (nbytes != 0) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        /* Clean up warnings.  This parameter is used for socket compatibility
         * but we are currently not making any use of it.
         */
        UNUSED_PARAMETER(flags);

#if (INCLUDE_TCP == NU_TRUE)

        /* Send the TCP data */
        if (SCK_Sockets[socketd]->s_protocol == NU_PROTO_TCP)
            return_status = TCPSS_Send_Data(socketd, buff, nbytes);

        /* Send the UDP data */
        else

#endif

#if (INCLUDE_UDP == NU_TRUE)

        if ( (SCK_Sockets[socketd]->s_protocol == NU_PROTO_UDP) &&
             (SCK_Sockets[socketd]->s_state & SS_ISCONNECTED) )
            return_status = UDP_Send_Datagram(socketd, buff, nbytes);

        else
#endif

            /* This socket cannot transmit data using NU_Send */
            return_status = NU_INVALID_SOCKET;

        /* Release the semaphore */
        SCK_Release_Socket();
    }
    
    /* return to caller */
    return (return_status);

} /* NU_Send */

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
*       sck_rcvmsg.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Recvmsg.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Recvmsg
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Recvmsg
*
*   DESCRIPTION
*
*       This function is responsible for receiving data across a network
*       and creating and returning an ancillary data structure composed
*       from information contained in the most recently received packet
*       if ancillary data is requested from the application layer for a
*       non-TCP socket.  This function can be used for any type of socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *msg                    Data structure holding the data to
*                               transmit, the address structure of the
*                               other side of the connection and a buffer
*                               to fill in requested ancillary data.
*       flags                   This parameter is used for socket
*                               compatibility we are currently not making
*                               any use of it
*
*   OUTPUTS
*
*       Number of bytes received.
*       NU_INVALID_PARM         The addr_struct from is NU_NULL.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_NOT_CONNECTED        The connection is broken for some reason.
*                               Stop using the socket; it is best to close it.
*       NU_DEVICE_DOWN          The device that this socket was communicating
*                               over has gone down. If the device is a PPP
*                               device it is likely the physical connection
*                               has been broken. If the device is ethernet
*                               and DHCP is being used, the lease of the IP
*                               address may have expired.
*       NU_WOULD_BLOCK          No data is available, and the socket is
*                               non-blocking.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
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
INT32 NU_Recvmsg(INT socketd, msghdr *msg, INT16 flags)
{
    INT32               return_status;
#if ( (INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || \
    (INCLUDE_IP_RAW == NU_TRUE) )
    struct sock_struct  *sockptr;           /* pointer to current socket */
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the msghdr structure is NULL or the receive buffer is NULL */
    if ( (msg == NU_NULL) || (msg->msg_iov == NU_NULL) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

    UNUSED_PARAMETER(flags);

#if ( (INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )
    /* Pick up a pointer to the socket list entry. */
    sockptr = SCK_Sockets[socketd];
#endif

#if (INCLUDE_TCP == NU_TRUE)

    /* Receive the TCP datagram */
    if (sockptr->s_protocol == NU_PROTO_TCP)
    {
        return_status = TCPSS_Recv_Data(socketd, msg->msg_iov,
                                        msg->msg_iovlen);
    }

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) )
    else
#endif
#endif

#if (INCLUDE_UDP == NU_TRUE)

    /* Receive the UDP datagram */
    if (sockptr->s_protocol == NU_PROTO_UDP)
    {
        return_status = UDP_Recv_Data(socketd, msg->msg_iov,
                                      msg->msg_iovlen, msg->msg_name);
    }

#if (INCLUDE_IP_RAW == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

    /* Receive the IPRAW datagram */
    if ( (IS_RAW_PROTOCOL(sockptr->s_protocol)) ||
         (sockptr->s_protocol == 0) )
    {
        return_status = IPRAW_Recv_Data(socketd, msg->msg_iov, msg->msg_iovlen,
                                        msg->msg_name);
    }

#endif

#if ( (INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) )

    /* If the call to return data was successful and a socket option was
     * set to return ancillary data to the application layer, parse the
     * ancillary data from the IP header and return it to the application.
     */
    if ( (return_status >= 0) && (sockptr->s_protocol != NU_PROTO_TCP) &&
         (sockptr->s_rx_ancillary_data) &&
         (sockptr->s_options & (
#if (INCLUDE_IPV4 == NU_TRUE)
                 SO_IP_PKTINFO_OP
#if (INCLUDE_IPV6 == NU_TRUE)
                 |
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                 SO_IPV6_HOPLIMIT_OP | SO_IPV6_HOPOPTS |
                 SO_IPV6_PKTINFO_OP | SO_IPV6_RTHDR_OP |
                 SO_IPV6_TCLASS_OP | SO_IPV6_DESTOPTS
#endif
            )) )
    {
        /* Store the ancillary data from the most recently received buffer
         * into the buffer provided by the application.
         */
        SCK_Store_Ancillary_Data(sockptr, msg);
    }

#endif

    /* Release the semaphore */
    SCK_Release_Socket();

    return (return_status);

} /* NU_Recvmsg */

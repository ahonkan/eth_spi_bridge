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
*       sck_sndmsg.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Sendmsg.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Sendmsg
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
*       NU_Sendmsg
*
*   DESCRIPTION
*
*       This function is responsible for transmitting data across
*       a network, parsing incoming ancillary data and applying the
*       options to the transmission.  This function can be used for any
*       type of socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *msg                    Data structure holding the data to
*                               transmit, the address structure of the
*                               other side of the connection and any
*                               ancillary data to use for the
*                               transmission
*       flags                   This parameter is used for socket
*                               compatibility we are currently not making
*                               any use of it
*
*   OUTPUTS
*
*       > 0                     Number of bytes sent.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*       NU_NOT_CONNECTED        If the socket is not connected.
*       NU_INVALID_PARM         The addr_struct to or buffer is NU_NULL
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously allocated
*                               via the NU_Socket call.
*       NU_INVALID_ADDRESS      The address passed in was most likely
*                               incomplete (i.e., missing the IP or port number).
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       NU_DEVICE_DOWN          The device that this socket was communicating
*                               over has gone down. If the device is a PPP
*                               device it is likely the physical connection
*                               has been broken. If the device is ethernet
*                               and DHCP is being used, the lease of the IP
*                               address may have expired.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource, but
*                               the socket is non-blocking.
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
INT32 NU_Sendmsg(INT socketd, const msghdr *msg, INT16 flags)
{
    INT32               return_status;

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )
    struct sock_struct  *sockptr;           /* pointer to current socket */
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#if (INCLUDE_UDP == NU_TRUE)
    struct uport        *udp_port;
#endif
#if (INCLUDE_IP_RAW == NU_TRUE)
    struct iport        *raw_port;
#endif

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )
    tx_ancillary_data   ancillary_data;
#endif
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the msghdr structure is NULL or the buffer to transmit is NULL
     * and the number of bytes to send is not zero.
     */
    if ( (msg == NU_NULL) ||
         ((msg->msg_iov == NU_NULL) && (msg->msg_iovlen != 0)) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

    /* Clean up warnings.  This parameter is used for socket compatibility
     * but we are currently not making any use of it.
     */
    UNUSED_PARAMETER(flags);

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
      (INCLUDE_IP_RAW == NU_TRUE) )

    /* Pick up a pointer to the socket list entry. */
    sockptr = SCK_Sockets[socketd];

#if (INCLUDE_IPV6 == NU_TRUE)

#if (INCLUDE_TCP == NU_TRUE)
    /* If the socket type is not TCP, check for ancillary data */
    if (sockptr->s_protocol != NU_PROTO_TCP)
#endif
    {
        /* If ancillary data was provided by the application */
        if (msg->msg_control != NU_NULL)
        {
            /* Set the socket's ancillary data pointer to the locally declared
             * data structure and fill it in accordingly.
             */
#if (INCLUDE_UDP == NU_TRUE)
            if (sockptr->s_protocol == NU_PROTO_UDP)
            {
                udp_port = UDP_Ports[sockptr->s_port_index];
                udp_port->up_ancillary_data = &ancillary_data;
            }
#if (INCLUDE_IP_RAW == NU_TRUE)
            else
#endif
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
            {
                raw_port = IPR_Ports[sockptr->s_port_index];
                raw_port->ip_ancillary_data = &ancillary_data;
            }
#endif

            /* Initially, set all the pointers to NULL */
            ancillary_data.tx_flags = 0;
            ancillary_data.tx_source_address = NU_NULL;
            ancillary_data.tx_hop_limit = NU_NULL;
            ancillary_data.tx_next_hop = NU_NULL;
            ancillary_data.tx_interface_index = NU_NULL;
            ancillary_data.tx_route_header = NU_NULL;
            ancillary_data.tx_dest_opt = NU_NULL;
            ancillary_data.tx_hop_opt = NU_NULL;
            ancillary_data.tx_traffic_class = NU_NULL;
            ancillary_data.tx_rthrdest_opt = NU_NULL;

            /* Set the data portion of the ancillary data structure to the
             * ancillary data passed into the function.
             */
            ancillary_data.tx_buff = msg->msg_control;

            /* Parse the ancillary data from the msghdr structure into the
             * ancillary data structure for the socket.
             */
            return_status = SCK_Parse_Ancillary_Data(msg, &ancillary_data);
        }
    }

    /* If the Ancillary Data was parsed successfully */
    if (return_status == NU_SUCCESS)

#endif
#endif

    {
#if (INCLUDE_TCP == NU_TRUE)

        /* Transmit the TCP datagram */
        if (sockptr->s_protocol == NU_PROTO_TCP)
        {
            return_status = TCPSS_Send_Data(socketd, msg->msg_iov,
                                            msg->msg_iovlen);
        }

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) )
        else
#endif
#endif

#if (INCLUDE_UDP == NU_TRUE)

        /* Transmit the UDP datagram */
        if (sockptr->s_protocol == NU_PROTO_UDP)
        {
            /* If this socket is connected, use the faster transmission routine */
            if (sockptr->s_state & SS_ISCONNECTED)
                return_status = UDP_Send_Datagram(socketd, msg->msg_iov,
                                                  msg->msg_iovlen);

            else
                return_status = UDP_Send_Data(socketd, msg->msg_iov,
                                              msg->msg_iovlen, msg->msg_name);
        }

#if (INCLUDE_IP_RAW == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

        /* Transmit the IPRAW datagram */
        if ( (IS_RAW_PROTOCOL(sockptr->s_protocol)) ||
             (sockptr->s_protocol == 0) )
        {
            return_status = IPRAW_Send_Data(socketd, msg->msg_iov, msg->msg_iovlen,
                                            msg->msg_name);
        }

#endif
    }

#if (INCLUDE_IPV6 == NU_TRUE)

    /* Clear out the pointer to Ancillary data - this data is set and used
     * on a packet by packet basis; therefore, it should not be used again
     * unless passed in from the application again.
     */
    if (msg->msg_control != NU_NULL)
    {
#if (INCLUDE_UDP == NU_TRUE)
        if (sockptr->s_protocol == NU_PROTO_UDP)
            UDP_Ports[sockptr->s_port_index]->up_ancillary_data = NU_NULL;
#if (INCLUDE_IP_RAW == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
            IPR_Ports[sockptr->s_port_index]->ip_ancillary_data = NU_NULL;
#endif
    }
#endif

    /* Release the semaphore */
    SCK_Release_Socket();

    return (return_status);

} /* NU_Sendmsg */

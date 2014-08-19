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

/**************************************************************************
*
*   FILENAME
*
*       ipraw.c
*
*   DESCRIPTION
*
*       IPRAW Protocol routines
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IPRaw_Init
*       IPRAW_Recv_Data
*       IPRaw_Read
*       IPRaw_Send
*       IPRaw_Append
*       IPRaw_Interpret
*       IPRaw_Make_Port
*       IPRaw_Get_PCB
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*       ipraw4.h
*       ipraw6.h
*
****************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"
#include "networking/ipraw4.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/ipraw6.h"
#include "networking/in6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

STATIC STATUS  IPRaw_Append(INT port_index, NET_BUFFER *buf_ptr);

struct iport *IPR_Ports[IPR_MAX_PORTS];

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for raw ports */
struct iport NET_Raw_Memory[IPR_MAX_PORTS];

extern tx_ancillary_data    NET_Sticky_Options_Memory[];
extern UINT8                NET_Sticky_Options_Memory_Flags[];

#endif

/*************************************************************************
*
*   FUNCTION
*
*       IPRaw_Init
*
*   DESCRIPTION
*
*       Setup for IPRaw_Make_Port
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IPRaw_Init(VOID)
{
    /* Zero out the IPRaw port list. */
    UTL_Zero((CHAR *)IPR_Ports, sizeof(IPR_Ports));

} /* IPRaw_Init */

/*************************************************************************
*
*   FUNCTION
*
*       IPRAW_Recv_Data
*
*   DESCRIPTION
*
*       This function is responsible for receiving data across a network
*       during a connectionless IP Raw transfer.  This function is called
*       by NU_Recv_From_Raw and NU_Recvmsg to receive data over a IPRAW
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the maximum number of bytes
*                               of data the application can receive
*       *from                   Pointer to the source protocol-specific
*                               address structure
*
*   OUTPUTS
*
*       > 0                     The number of bytes received.
*       NU_WOULD_BLOCK          No data is available, and the socket is
*                               non-blocking.
*       NU_NO_PORT_NUMBER       No port number.
*       NU_DEVICE_DOWN          The device that this socket was
*                               communicating over has gone down. If the
*                               device is a PPP device it is likely the
*                               physical connection has been broken. If
*                               the device is ethernet and DHCP is being
*                               used, the lease of the IP address may
*                               have expired.
*       NU_SOCKET_CLOSED        The socket was closed while suspending.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*
*************************************************************************/
INT32 IPRAW_Recv_Data(INT socketd, CHAR *buff, UINT16 nbytes,
                      struct addr_struct *from)
{
    struct sock_struct  *sockptr;        /* pointer to current socket */
    struct SCK_TASK_ENT task_entry;      /* task entry for list operations */
    UINT32              socket_id;
    INT32               return_status = NU_SUCCESS;

    /*  Pick up a pointer to the socket list. */
    sockptr = SCK_Sockets[socketd];

    if (!(sockptr->s_state & SS_DEVICEDOWN))
    {
        /* The local port should be zero when dealing with raw IP sockets */
        if (sockptr->s_local_addr.port_num == 0)
        {
            if (!sockptr->s_recvpackets)
            {
                /* If the socket is Blocking, suspend the task and wait for data */
                if (sockptr->s_flags & SF_BLOCK)
                {
                    /* Initialize the list entry's task number */
                    task_entry.task = NU_Current_Task_Pointer();

                    /* Add it to the list of tasks pending on receive */
                    DLL_Enqueue(&sockptr->s_RXTask_List, &task_entry);

                    /* Get the socket ID to verify the socket after suspension */
                    socket_id = sockptr->s_struct_id;

                    SCK_Suspend_Task(task_entry.task);

                    /* If this is a different socket or the socket has been
                     * closed, handle it appropriately.
                     */
                    if ( (!SCK_Sockets[socketd]) ||
                         (SCK_Sockets[socketd]->s_struct_id != socket_id) ||
                         (SCK_Sockets[socketd]->s_state & SS_DEVICEDOWN) )
                        return_status = NU_SOCKET_CLOSED;
                }
                else
                    return_status = NU_WOULD_BLOCK;
            }

            /* Pick up the data. */
            if ( (return_status == NU_SUCCESS) && (sockptr->s_recvpackets != 0) )
                return_status = IPRaw_Read(buff, from, sockptr, nbytes);
        }
    }
    else
    {
        /* The device that this socket was communicating over has gone down. If
         * the device is a PPP device it is likely the physical connection has
         * been broken. If the device is ethernet and DHCP is being used, the
         * lease of the IP address may have expired.
         */
        return_status = NU_DEVICE_DOWN;
    }

    return (return_status);

} /* IPRAW_Recv_Data */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw_Read
*
*   DESCRIPTION
*
*       Get the data from the IP buffer and transfer it into your
*       buffer.  Returns the number of bytes transferred or 0 if none
*       available.
*
*   INPUTS
*
*       *buffer                 A pointer to the IP buffer
*       *from                   A pointer to the address structure
*                               information
*       *sockptr                A pointer to the socket structure
*                               information
*       nbytes                  The maximum number of bytes that can be
*                               copied into the user buffer.
*
*   OUTPUTS
*
*       > 0                     Number of bytes copied.
*       NU_NO_DATA_TRANSFER     Data transfer fails
*
*************************************************************************/
INT32 IPRaw_Read(CHAR *buffer, struct addr_struct *from,
                 struct sock_struct *sockptr, UINT16 nbytes)
{
    INT32   bytes_copied;
    VOID    *pkt;

#if (INCLUDE_IPV6 == NU_TRUE)
    UINT16  hlen;
#endif

    /* Check to see if there are any packets waiting. */
    if (sockptr->s_recvlist.head == NU_NULL)
        return (NU_NO_DATA_TRANSFER);

    /* Save a pointer to the IP header so the data pointer can be restored
     * to this point later.
     */
    pkt = sockptr->s_recvlist.head->data_ptr;

    /* Copy the foreign address into the socket structure and the address
     * structure to return to the application.
     */
    SCK_Copy_Addresses(sockptr, sockptr->s_recvlist.head->data_ptr, from,
                       sockptr->s_recvlist.head->mem_flags);

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (sockptr->s_recvlist.head->mem_flags & NET_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        hlen = (UINT16)(IP6_HEADER_LEN +
               (GET16(sockptr->s_recvlist.head->data_ptr, IP6_PAYLEN_OFFSET) -
               sockptr->s_recvlist.head->mem_total_data_len));

        sockptr->s_recvlist.head->data_ptr += hlen;
        sockptr->s_recvlist.head->data_len -= hlen;
    }
#endif

    /* Copy the data from the NET buffer into the user buffer */
    bytes_copied = MEM_Copy_Buffer(buffer, sockptr, (INT32)nbytes);

    from->port = 0;

    /* Check the IP_RECVIFADDR option */
    if (sockptr->s_options & IP_RECVIFADDR_OP)
    {
        sockptr->s_recv_if = sockptr->s_recvlist.head->mem_buf_device;
    }

    if ( (!(sockptr->s_flags & SF_ZC_MODE)) &&
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
        /* Restore the data pointer to the beginning of the IP header. */
        sockptr->s_recvlist.head->data_ptr = pkt;

        SCK_Set_RX_Anc_BufPtr(sockptr, sockptr->s_recvlist.head);
    }

    /* If this buffer is being saved off to extract ancillary data for
     * the application or this is a zero copy buffer, remove the buffer
     * from the socket's receive list.  Otherwise, move the buffer to
     * the free list.
     */
    if ( (sockptr->s_rx_ancillary_data != sockptr->s_recvlist.head) &&
         (!(sockptr->s_flags & SF_ZC_MODE)) )
        MEM_Buffer_Chain_Free(&sockptr->s_recvlist, &MEM_Buffer_Freelist);

    else
        MEM_Buffer_Dequeue(&sockptr->s_recvlist);

    /* Update the number of buffered datagrams. */
    sockptr->s_recvpackets--;

    sockptr->s_recvbytes -= bytes_copied;

    return (bytes_copied);

} /* IPRaw_Read */

/*************************************************************************
*
*   FUNCTION
*
*       IPRAW_Send_Data
*
*   DESCRIPTION
*
*       Transmit the data for IPRAW.  This function is used by
*       NU_Send_To_Raw and NU_Sendmsg to transmit data over a RAW
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the number of bytes of data
*       *to                     Pointer to the destination's
*                               protocol-specific address structure
*
*   OUTPUTS
*
*       > 0                     The number of bytes transmitted
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been
*                               previously allocated via the NU_Socket
*                               call.
*       NU_NOT_CONNECTED        The socket is not connected.
*       NU_INVALID_ADDRESS      The address passed in was most likely
*                               incomplete (i.e., missing the IP number).
*       NU_NO_PORT_NUMBER       The port number does not exist.
*       NU_DEVICE_DOWN          The device that this socket was
*                               communicating over has gone down. If the
*                               device is a PPP device it is likely the
*                               physical connection has been broken. If
*                               the device is Ethernet and DHCP is being
*                               used, the lease of the IP address may
*                               have expired.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       Nucleus Status Code
*
*************************************************************************/
INT32 IPRAW_Send_Data(INT socketd, CHAR *buff, UINT16 nbytes,
                      const struct addr_struct *to)
{
    UINT16              dest_port;
    IPR_PORT            *iprt;
    INT32               count;
    struct sock_struct  *sockptr;
    INT32               return_status;

    /* Pick up a pointer to the socket list. */
    sockptr = SCK_Sockets[socketd];

    if (!(sockptr->s_state & SS_DEVICEDOWN))
    {
        dest_port = to->port;

        /* Proceed only if the application specified a destination port. */
        if (dest_port == 0)
        {
            /* Make sure that he gave us the IP number. */
#if (INCLUDE_IPV6 == NU_TRUE)

            if ( ((to->family == SK_FAM_IP6) &&
                  (IPV6_IS_ADDR_UNSPECIFIED(to->id.is_ip_addrs)))

#if (INCLUDE_IPV4 == NU_TRUE)
                 ||
#endif

#else
            if (
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                 ((to->family == SK_FAM_IP) &&
                  (TLS_Comparen(to->id.is_ip_addrs, (VOID*)IP_Null, IP_ADDR_LEN))) )
#else
            )
#endif
            {
                return (NU_INVALID_ADDRESS);
            }

            /* Pick up the foreign IP address and port number. */
            sockptr->s_foreign_addr.port_num = dest_port;
            sockptr->s_local_addr.port_num = 0;
            memcpy(&(sockptr->s_foreign_addr.ip_num), &(to->id), MAX_ADDRESS_SIZE);

            iprt = IPR_Ports[sockptr->s_port_index];

#if (INCLUDE_IPV6 == NU_TRUE)

            if (to->family == SK_FAM_IP6)
            {
#if (INCLUDE_IPV4 == NU_TRUE)
                /* If the destination address is an IPv4-Mapped IPv6 address,
                 * extract the IPv4 address from the IPv6 address and set the
                 * family type to IPv4,
                 */
                if (IPV6_IS_ADDR_V4MAPPED(to->id.is_ip_addrs))
                {
                    /* If the application is attempting to transmit to an
                     * IPv4 address, but the socket option to transmit only
                     * IPv6 packets on this socket has been set, return an
                     * error to the application.
                     */
                    if (sockptr->s_options & SO_IPV6_V6ONLY)
                        return (NU_INVALID_ADDRESS);

                    IP6_EXTRACT_IPV4_ADDR(iprt->ip_faddr, to->id.is_ip_addrs);
                    iprt->ipraw_faddr_struct.ipraw_family = NU_FAMILY_IP;
                }

                else
#endif
                {
                    NU_BLOCK_COPY(iprt->ip_faddrv6, to->id.is_ip_addrs, IP6_ADDR_LEN);
                    iprt->ipraw_faddr_struct.ipraw_family = NU_FAMILY_IP6;
                }
            }

#if (INCLUDE_IPV4 == NU_TRUE)
            else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            {
#if (INCLUDE_IPV6 == NU_TRUE)
                /* If the application is attempting to connect to an IPv4 address,
                 * but the socket option to transmit only IPv6 packets on this socket
                 * has been set, return an error to the application.
                 */
                if (sockptr->s_options & SO_IPV6_V6ONLY)
                    return (NU_INVALID_ADDRESS);
#endif

                iprt->ip_faddr = IP_ADDR(to->id.is_ip_addrs);
                iprt->ipraw_faddr_struct.ipraw_family = NU_FAMILY_IP;
            }
#endif

            iprt->ip_protocol = sockptr->s_protocol;

            /* Send the data. */
            count = IPRaw_Send(iprt,(UINT8 *) buff, nbytes, sockptr->s_options);

            /* Let them know if it worked OK. */
            if (count < 0)
                /* return an error status */
                return_status = NU_NO_DATA_TRANSFER;
            else
                /* return number of bytes transferred */
                return_status = count;

        } /* port is 0 */

        else
            return_status = NU_INVALID_PARM;
    }
    else
    {
        /* The device that this socket was communicating over has gone down. If
         * the device is a PPP device it is likely the physical connection has
         * been broken. If the device is ethernet and DHCP is being used, the
         * lease of the IP address may have expired. */
        return_status = NU_DEVICE_DOWN;
    }

    return (return_status);

} /* IPRAW_Send_Data */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw_Send
*
*   DESCRIPTION
*
*       Send some data out in a IP packet.
*
*   INPUTS
*
*       *iptr                   A pointer to the IP port information
*       *buffer                 A pointer the IP buffer
*       nbytes                  Number of bytes to send
*       sock_options            Socket options
*
*   OUTPUTS
*
*       > 0                     Number of bytes sent
*       NU_NO_BUFFERS           All of the net buffers are used up
*       -1                      Failure
*
*************************************************************************/
INT32 IPRaw_Send(struct iport *iptr, UINT8 *buffer, UINT16 nbytes,
                 UINT16 sock_options)
{
    NET_BUFFER          *buf_ptr;
    STATUS              stat;
    struct sock_struct  *sock_ptr = SCK_Sockets[iptr->ip_socketd];

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS       ip6_options;
    INT32               flags = 0;
    UINT8               *src_addr;
#endif

    /* Remove compiler warning. */
    UNUSED_PARAMETER(sock_options);

    /* Extract the local IP address to use from the route. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    if (iptr->ipraw_faddr_struct.ipraw_family == SK_FAM_IP6)
    {
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Before we do anything else make sure a route to the host is up. */
        stat = IPRaw6_Cache_Route(iptr, iptr->ip_faddrv6);

        if (stat == NU_SUCCESS)
        {
            memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

            /* Set up the parameters to be used for the transmission */
            stat = IP6_Setup_Options(iptr->ip_ancillary_data,
                                     iptr->ip_sticky_options,
                                     &ip6_options, iptr->ip_faddrv6,
                                     &iptr->ipraw_routev6, (UINT8)iptr->ip_ttl);

            if (stat != NU_SUCCESS)
                return (stat);

            /* If an address was specified as Ancillary data, use that
             * address for this datagram.
             */
            if ( (iptr->ip_ancillary_data) &&
                 (iptr->ip_ancillary_data->tx_source_address) )
            {
                NU_BLOCK_COPY(iptr->ip_laddrv6,
                              iptr->ip_ancillary_data->tx_source_address,
                              IP6_ADDR_LEN);

                /* If this address does not exist on the specified interface, return
                 * an error.
                 */
                if (DEV6_Find_Target_Address(ip6_options.tx_route.rt_route->rt_device,
                                             iptr->ip_laddrv6) == NU_NULL)
                    return (-1);
            }

            /* Check if a source address has been specified using a Sticky
             * Option.
             */
            else if ( (iptr->ip_sticky_options) &&
                      (iptr->ip_sticky_options->tx_source_address) )
            {
                NU_BLOCK_COPY(iptr->ip_laddrv6,
                              iptr->ip_sticky_options->tx_source_address,
                              IP6_ADDR_LEN);

                /* If this address does not exist on the specified interface, return
                 * an error.
                 */
                if (DEV6_Find_Target_Address(ip6_options.tx_route.rt_route->rt_device,
                                             iptr->ip_laddrv6) == NU_NULL)
                    return (-1);
            }

            /* If a specific address was not bound to, select an address of the
             * same scope as the destination.
             */
            else if ( (!(sock_ptr->s_flags & SF_BIND)) ||
                      (IP_ADDR(sock_ptr->s_local_addr.ip_num.is_ip_addrs) == IP_ADDR_ANY) )
            {
                src_addr =
                    in6_ifawithifp(ip6_options.tx_route.rt_route->rt_device,
                                   iptr->ip_faddrv6);

                /* If an address of the same scope does not exist, return an error */
                if (src_addr)
                    NU_BLOCK_COPY(iptr->ip_laddrv6, src_addr, IP6_ADDR_LEN);

                else
                    return (-1);
            }

            /* Otherwise, use the bound-to address as the source address of
             * the packet.
             */
            else
            {
                NU_BLOCK_COPY(iptr->ip_laddrv6,
                              sock_ptr->s_local_addr.ip_num.is_ip_addrs,
                              IP6_ADDR_LEN);
            }

            ip6_options.tx_source_address = iptr->ip_laddrv6;
        }

        else
            return (stat);
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Before we do anything else make sure a route to the host is up. */
        stat = IPRaw4_Cache_Route(iptr, iptr->ip_faddr);

        if (stat == NU_SUCCESS)
        {
            /* If a specific address was not bound to, use the primary address of
             * the interface associated with the route as the source address of
             * the packet.
             */
            if ( (!(sock_ptr->s_flags & SF_BIND)) ||
                 (IP_ADDR(sock_ptr->s_local_addr.ip_num.is_ip_addrs) == IP_ADDR_ANY) )
                iptr->ip_laddr = iptr->ipraw_route.rt_route->rt_entry_parms.
                                 rt_parm_device->dev_addr.dev_addr_list.
                                 dv_head->dev_entry_ip_addr;

            /* Otherwise, use the bound-to address as the source address of
             * the packet.
             */
            else
                iptr->ip_laddr = IP_ADDR(sock_ptr->s_local_addr.ip_num.is_ip_addrs);
        }

        else
            return (stat);
    }

    /* Don't send more than we have concluded is our maximum. Our maximum
     * is different here depending on if the IP header is to be generated
     * by the stack or if it was created by the application. Note that if
     * this is an IPv6 packet, the application cannot generate the IP
     * header.  The stack will always generate the IP header of an IPv6
     * raw datagram.  If the user has included the IP header, fragmentation
     * is not supported on the datagram.  If it exceed the maximum length
     * of the network, reject the packet.
     */
    if ( (iptr->ipraw_faddr_struct.ipraw_family == SK_FAM_IP) &&
         (sock_ptr->s_flags & IP_RAWOUTPUT) )
    {
        /* Check for the max size. The macro IMAXLEN does not include the IP
         * header so it must be subtracted off the nbytes before the
         * comparison.
         */
        if ( (iptr->ipraw_faddr_struct.ipraw_family == SK_FAM_IP) &&
             ((nbytes -
             ((GET8(buffer, IP_VERSIONANDHDRLEN_OFFSET) & 0xf) << 2)) >
             (UINT16)(IMAXLEN +
             iptr->ipraw_route.rt_route->rt_entry_parms.rt_parm_device->dev_hdrlen)) )
            return (NU_MSGSIZE);
    }

#endif

    /* If this is a Zero Copy buffer, the data is already contained in
     * NET buffers.
     */
    if (sock_ptr->s_flags & SF_ZC_MODE)
        buf_ptr = (NET_BUFFER*)buffer;

    /* Otherwise, allocate a chain of NET buffers and copy the data */
    else
    {
        buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist, (INT32)nbytes);

        if (buf_ptr == NU_NULL)
        {
            MIB2_ipOutDiscards_Inc;

            return (NU_NO_BUFFERS);
        }
    }

    /* Set the data pointer to the correct location. */
    buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

    /* Copy the data into the chain of NET buffers. */
    nbytes = (UINT16)MEM_Copy_Data(buf_ptr, (CHAR*)buffer, (INT32)nbytes,
                                   sock_ptr->s_flags);

    /* Set the deallocation list pointer. */
    buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

    /* Send this packet. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    if (iptr->ipraw_faddr_struct.ipraw_family == SK_FAM_IP6)
    {
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        ip6_options.tx_dest_address = iptr->ip_faddrv6;

        /* If the IPV6_CHECKSUM socket option is enabled or this is an ICMPv6
         * RAW packet, set the checksum offset in the options structure.
         */
        if ( (sock_ptr->s_options & SO_IPV6_CHECKSUM) ||
             (iptr->ip_protocol == IPPROTO_ICMPV6) )
        {
#if (INCLUDE_IP_RAW == NU_TRUE)

            if (sock_ptr->s_options & SO_IPV6_CHECKSUM)
                ip6_options.tx_raw_chk_off = iptr->ip_chk_off;
            else
                ip6_options.tx_raw_chk_off = IP6_ICMP_CKSUM_OFFSET;
#endif

            flags |= IP6_CHECKSUM;
        }

        stat = IP6_Send(buf_ptr, &ip6_options, (UINT8)(iptr->ip_protocol),
                        &ip6_options.tx_route, flags, NU_NULL);

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* If the application created the header - socket option HDRINCL */
        if (sock_ptr->s_flags & IP_RAWOUTPUT)
        {
            stat = IP_Send((NET_BUFFER *)buf_ptr, &iptr->ipraw_route,
                          iptr->ipraw_route.rt_ip_dest.sck_addr,
                          iptr->ip_laddr, IP_RAWOUTPUT, iptr->ip_ttl,
                          (INT)iptr->ip_protocol, IP_TYPE_OF_SERVICE,
                          sock_ptr->s_moptions_v4);
        }

        else
        {
           stat = IP_Send((NET_BUFFER *)buf_ptr, &iptr->ipraw_route,
                          iptr->ipraw_route.rt_ip_dest.sck_addr,
                          iptr->ip_laddr, IP_ALLOWBROADCAST, iptr->ip_ttl,
                          (INT)iptr->ip_protocol, IP_TYPE_OF_SERVICE,
                          sock_ptr->s_moptions_v4);
        }
    }
#endif

    if (stat != NU_SUCCESS)
    {
        /* The packet was not sent.  Deallocate the buffer if this is not
         * a Zero Copy Buffer.  If the packet was transmitted it will be
         * deallocated later by the appropriate MAC layer TX routine.
         */
        if (!(sock_ptr->s_flags & SF_ZC_MODE))
            MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        return (stat);
    }

    /* If the send went ok, then return the number of bytes sent. */
    return (nbytes);

}  /* IPRaw_Send */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw_Interpret
*
*   DESCRIPTION
*
*       Process received IP Raw datagrams.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer
*       index                   Index into the IPRAW port list
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS IPRaw_Interpret(NET_BUFFER *buf_ptr, INT index)
{
    NET_BUFFER      *new_buf_ptr, *current_buf_ptr;
    INT32           total_data_len;

    /* Make sure there is room to add the received packet. */
    if (SCK_Sockets[IPR_Ports[index]->ip_socketd]->s_recvpackets
                                         < IMAX_DGRAMS)
    {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        /* Determine the length of the IPv6 header, extension headers
         * and payload.  The IPv6 header and extension headers were
         * stripped off the total data length, because those headers
         * do not get returned to the application layer.
         */
        if (buf_ptr->mem_flags & NET_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            total_data_len =
                (INT32)(buf_ptr->mem_total_data_len + IP6_HEADER_LEN +
                (GET16(buf_ptr->data_ptr, IP6_PAYLEN_OFFSET) -
                 buf_ptr->mem_total_data_len));
#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            total_data_len = (INT32)(buf_ptr->mem_total_data_len);
#endif

        /* Since this packet could be passed to  multiple sockets, make
         * a copy for each socket that might RX it. So, allocate buffers
         * for it and copy it.
         */
        new_buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                               total_data_len);

        /* Make sure we got the buffers. */
        if (new_buf_ptr)
        {
            /* Init the header fields for this new buffer. */
            new_buf_ptr->data_ptr = new_buf_ptr->mem_packet;
            new_buf_ptr->mem_buf_device = buf_ptr->mem_buf_device;

            /* Copy the data from the original packet. */
            MEM_Chain_Copy(new_buf_ptr, buf_ptr, 0, total_data_len);

            /* Set the total length for this new packet. */
            new_buf_ptr->mem_total_data_len = buf_ptr->mem_total_data_len;

            current_buf_ptr = new_buf_ptr;

            while (current_buf_ptr)
            {
#if (INCLUDE_IPV6 == NU_TRUE)
                if (buf_ptr->mem_flags & NET_IP6)
                    current_buf_ptr->mem_flags |= NET_IP6;
#if (INCLUDE_IPV4 == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                    current_buf_ptr->mem_flags |= NET_IP;
#endif

                current_buf_ptr = current_buf_ptr->next_buffer;
            }

            /* Append the new buffer to datagram queue */
            if (IPRaw_Append(index, new_buf_ptr) != NU_SUCCESS)
                NLOG_Error_Log("Failed to append IPRAW datagram to queue",
                               NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (NU_SUCCESS);

} /* IPRaw_Interpret */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw_Append
*
*   DESCRIPTION
*
*       Add a received packet to a sockets receive list.
*
*   INPUTS
*
*       port_index              The index into the IPRAW port list
*       *buf_ptr                Pointer to the net buffer
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATIC STATUS IPRaw_Append(INT port_index, NET_BUFFER *buf_ptr)
{
    struct iport        *iptr = IPR_Ports[port_index];
    struct sock_struct  *sockptr = SCK_Sockets[iptr->ip_socketd];

    /* Place the datagram onto this ports datagram list. */
    MEM_Buffer_Enqueue(&sockptr->s_recvlist, buf_ptr);

    sockptr->s_recvbytes += buf_ptr->mem_total_data_len;

    /* Update the number of buffered datagrams. */
    sockptr->s_recvpackets++;

    /* If there is a task pending data on the port, then set an event to
     * resume that task.
     */
    if (sockptr->s_RXTask_List.flink != NU_NULL)
    {
       if (EQ_Put_Event(IPDATA, (UNSIGNED)(iptr->ip_socketd), 0) != NU_SUCCESS)
           NLOG_Error_Log("Failed to set event to resume task suspended on IPRAW port",
                          NERR_SEVERE, __FILE__, __LINE__);
    }

    return (NU_SUCCESS);

} /* IPRaw_Append */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw_Make_Port
*
*   DESCRIPTION
*
*       This is the initialization for IP Raw based communication. When
*       an IP PCB needs to be created, this routine is called to do as
*       much pre-initialization as possible to save overhead during
*       operation.
*
*   INPUTS
*
*       socketd                 Socket descriptor
*
*   OUTPUTS
*
*       >= 0                    Port number
*       -1                      Failure to make a port
*
*************************************************************************/
INT32 IPRaw_Make_Port(INT socketd)
{
    INT16                   i, retval;
    UINT16                  *return_ptr;       /* pointer to memory block */
    struct  iport           *p;
    struct  sock_struct     *sockptr;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    STATUS                  status;
#else
#if (INCLUDE_IPV6 == NU_TRUE)
    INT                     j;
#endif
#endif

    sockptr = SCK_Sockets[socketd];

    for (i = 0; i < IPR_MAX_PORTS; i++)
    {
        /* If this entry is available. */
        if (IPR_Ports[i] == NU_NULL)
            break;
    } /* end for */

    /* If no available entries were found. */
    if (i == IPR_MAX_PORTS)
    {
        NLOG_Error_Log ("No room for IPRaw port", NERR_RECOVERABLE,
                            __FILE__, __LINE__);
        return (-1);               /* out of room for ports */
    } /* end if */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&return_ptr,
                                (UNSIGNED)sizeof(struct iport),
                                (UNSIGNED)NU_NO_SUSPEND);

    /* check status of memory allocation */
    if (status != NU_SUCCESS)
    {
        /* ERROR memory allocation error.\r\n */
        NLOG_Error_Log ("Unable to alloc memory for port structure", NERR_RECOVERABLE,
                            __FILE__, __LINE__);
        return (-1);               /* out of room for ports */
    }

    else

#else
    /* Assign memory to the the new port*/
    return_ptr = (UINT16 *) &NET_Raw_Memory[i];
#endif

    {
        return_ptr = (UINT16 *)TLS_Normalize_Ptr(return_ptr);
        p = (VOID *)return_ptr;
    }

#if ( (INCLUDE_STATIC_BUILD == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )

    /* Traverse the flag array to find the unused memory location*/
    for (j = 0; j != NSOCKETS; j++)
    {
        /* If this memory is available, use it */
        if (NET_Sticky_Options_Memory_Flags[j] != NU_TRUE)
        {
            /* Assign sticky options pointer to this memory */
            p->ip_sticky_options =
                (tx_ancillary_data *)&NET_Sticky_Options_Memory[j];

            /* Turn the memory flag on */
            NET_Sticky_Options_Memory_Flags[j] = NU_TRUE;

            break;
        }
    }

    /* If there are no available entries, set an error */
    if (j == NSOCKETS)
        return (-1);
#endif

    IPR_Ports[i] = p;
    retval = i;

    /* Clear the RawIP port structure. */
    UTL_Zero((CHAR *)p, sizeof(*p));

    sockptr->s_recvpackets = 0;
    sockptr->s_recvlist.head = NU_NULL;
    sockptr->s_recvlist.tail = NU_NULL;

    /* The socket with which this port is associated is unknown at this time. */
    p->ip_socketd = socketd;

    /* Set the protocol to the protocol associated with the socket */
    p->ip_protocol = sockptr->s_protocol;

    p->ipraw_route.rt_route = NU_NULL;

    return (retval);

} /* IPRaw_Make_Port */

/***********************************************************************
*
*   FUNCTION
*
*       IPRaw_Get_PCB
*
*   DESCRIPTION
*
*       This function is responsible for returning a IPR_Ports entry
*       number to a caller based on a match with the port information
*       sent in the socket descriptor.
*
*   INPUTS
*
*       socketd                 Socket descriptor
*       *socket_desc            A pointer to the socket information
*
*   OUTPUTS
*
*       >= 0                    Index into the IPRAW port list.
*       NU_IGNORE_VALUE         No match found.
*
*************************************************************************/
INT16 IPRaw_Get_PCB(INT socketd, const struct sock_struct *socket_desc)
{
    INT16   pnum = NU_IGNORE_VALUE;    /* IPR_Ports entry number initialized */
    INT16   index;
    struct  iport *iprt;

    for (index = 0; index < IPR_MAX_PORTS; index++)
    {
        iprt = IPR_Ports[index];

        if ( (iprt != NU_NULL) &&
             (iprt->ip_protocol == (UINT16)socket_desc->s_protocol) &&
             (iprt->ip_socketd == socketd) )
        {
            pnum = index;
            break;
        }
    }

    /* return a IPR_Ports entry number to the caller */
    return (pnum);

} /* IPRaw_Get_PCB */

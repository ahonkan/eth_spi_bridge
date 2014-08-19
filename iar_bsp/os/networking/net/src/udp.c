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
*   FILENAME
*
*       udp.c
*
*   DESCRIPTION
*
*       UDP Protocol routines
*
*   DATA STRUCTURES
*
*       UDP_Ports[]
*
*   FUNCTIONS
*
*       UDP_Init
*       UDP_Read
*       UDP_Send
*       UDP_Append
*       UDP_Recv_Data
*       UDP_Interpret
*       UDP_Make_Port
*       UDP_Port_Cleanup
*       UDP_Handle_Datagram_Error
*       UDP_Get_Pnum
*
*   DEPENDENCIES
*
*       nu_net.h
*       udp4.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/udp4.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#include "networking/udp6.h"
#include "networking/in6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/*
 * Define the UDP portlist table.  This is a critical structure in Nucleus NET
 * as it maintains information about all open UDP ports.
 */
struct uport *UDP_Ports[UDP_MAX_PORTS];

/* Local Prototypes */
STATIC  STATUS  UDP_Append(INT , NET_BUFFER *);
STATIC  INT32   UDP_Read(struct sock_struct *, CHAR *, struct addr_struct *,
                         UINT16);
STATIC  INT32   UDP_Send(UDP_PORT *, CHAR *, INT32);

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for UDP ports */
UDP_PORT NET_UDP_Ports_Memory[UDP_MAX_PORTS];

extern tx_ancillary_data    NET_Sticky_Options_Memory[];
extern UINT8                NET_Sticky_Options_Memory_Flags[];

#endif

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Init
*
*   DESCRIPTION
*
*       Initialize the UDP layer.
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
VOID UDP_Init(VOID)
{
    /* Zero out the UDP portlist. */
    UTL_Zero((CHAR *)UDP_Ports, sizeof(UDP_Ports));

} /* UDP_Init */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Interpret
*
*   DESCRIPTION
*
*       Take an incoming UDP packet and make it available to the user
*       level routines.  Currently keeps the last packet coming in to a
*       port.
*
*       Limitations :
*
*       Can only listen to one UDP port at a time, only saves the last
*       packet received on that port.  Port numbers should be assigned
*       like TCP ports.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer list
*       *uptr                   Pointer to the UDP_PORT
*       port_index              Port index number
*       make_copy               This flag indicates whether to make
*                               a copy of the buffer chain or pass
*                               the existing chain up the stack.  If
*                               not making a copy, this routine is
*                               responsible for freeing the passed
*                               buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       2
*       -1
*
*************************************************************************/
INT16 UDP_Interpret(NET_BUFFER *buf_ptr, UDP_PORT *uptr, INT port_index,
                    UINT8 make_copy)
{
    struct sock_struct *sockptr;
    INT32               total_data_len;
    NET_BUFFER          *new_buf_ptr, *current_buf_ptr;
    UINT16              hlen;

#if (INCLUDE_IPSEC == NU_TRUE)
    STATUS             status;
#endif

    sockptr = SCK_Sockets[uptr->up_socketd];

    /* Make sure there is room to add the received packet, then call UDP_Append
       to add this packet to the ports receive list. */
    if ( (sockptr) && (sockptr->s_recvpackets >= UMAX_DGRAMS) )
    {
        MIB2_udpInErrors_Inc;

        /* Drop the packet by placing it back on the buffer_freelist.  Since
         * we are not making a copy of this buffer chain, we are expected to
         * free the buffers.
         */
        if (!make_copy)
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("UDP packet discarded due to no room on the socket input list",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return(-1);
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    else
    {
        /* Check whether IPsec is enabled for the device which received
         * this packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Verify the IPsec policy for the received packet. It is
             * possible that the last processed packet was also from the
             * same source. The information about the last processed
             * packet is present in the UDP ports structure.  Check this
             * information.
             */
            status = IPSEC_UDP_Check_Policy_In(buf_ptr, uptr);

            /* If the packet was successfully processed so far. */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("IP packet discarded due to IPsec policy checks",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                /* An error was encountered - drop the packet. */
                if (!make_copy)
                {
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                          &MEM_Buffer_Freelist);
                }

                return ((INT16)status);
            }
        }
    }

#endif

    /* If this datagram is going to be delivered to another application
     * task, make a copy of it now.
     */
    if (make_copy)
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
        {
            hlen = (UINT16)(IP6_HEADER_LEN +
                (GET16(buf_ptr->data_ptr, IP6_PAYLEN_OFFSET) - buf_ptr->mem_total_data_len));

            total_data_len = (INT32)(buf_ptr->mem_total_data_len + hlen);

            /* Add the IPv6 header back on to the packet. */
            buf_ptr->data_len += hlen;
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            hlen = (UINT16)((GET8(buf_ptr->data_ptr, IP_VERSIONANDHDRLEN_OFFSET)
                & 0x0f) << 2);

            total_data_len = (INT32)(buf_ptr->mem_total_data_len + hlen);

            /* Add the IP header back on to the packet. */
            buf_ptr->data_len += hlen;
        }
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
            new_buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

            /* Copy the data from the original packet. */
            MEM_Chain_Copy(new_buf_ptr, buf_ptr, 0, total_data_len);

            /* Decrement hlen so this buffer is exactly like the incoming
             * buffer.  We needed to copy the bytes from the header, but
             * we don't want the length to reflect this.
             */
            new_buf_ptr->data_len -= hlen;

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
            if (UDP_Append(port_index, new_buf_ptr) != NU_SUCCESS)
            {
                MEM_One_Buffer_Chain_Free(new_buf_ptr, new_buf_ptr->mem_dlist);

                NLOG_Error_Log("Failed to append UDP datagram to queue",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Restore the buffer to its previous state */
            buf_ptr->data_len -= hlen;
        }
    }

    else
    {
        /* Pull the current buffer chain off of the receive list. It will be added
           to the port's receive list by UDP_Append. */
        MEM_Buffer_Dequeue(&MEM_Buffer_List);

        /* Add the received packet to the appropriate port's receive list. */
        if (UDP_Append(port_index, buf_ptr) != NU_SUCCESS)
        {
            /* Free the buffer chain */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            NLOG_Error_Log("Failed to append UDP datagram to queue",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Increment the number of UDP packets received. */
    MIB2_udpInDatagrams_Inc;

    return (NU_SUCCESS);

} /* UDP_Interpret */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Append
*
*   DESCRIPTION
*
*       Add a received packet to a sockets receive list.
*
*   INPUTS
*
*       port_index              The tcp port index number
*       *buf_ptr                Pointer to the net buffer list
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATIC STATUS UDP_Append(INT port_index, NET_BUFFER *buf_ptr)
{
    UDP_PORT            *uptr = UDP_Ports[port_index];
    struct sock_struct  *sockptr;
    STATUS              status = NU_SUCCESS;
    struct SCK_TASK_ENT *task_entry_ptr;

    sockptr = SCK_Sockets[uptr->up_socketd];

    if (sockptr != NU_NULL)
    {
        /* Strip off the UDP layer. */
        buf_ptr->data_len           -= UDP_HEADER_LEN;
        buf_ptr->mem_total_data_len -= UDP_HEADER_LEN;

        /* Place the datagram onto this ports datagram list. */
        MEM_Buffer_Enqueue(&sockptr->s_recvlist, buf_ptr);

        /* Update the number of buffered datagrams. */
        sockptr->s_recvpackets++;

        sockptr->s_recvbytes += buf_ptr->mem_total_data_len;
        
        /* Trace log */
        T_SOCK_ENQ(sockptr->s_recvpackets, sockptr->s_recvbytes, uptr->up_socketd);

        /* If there is a task pending data on the port, then set an event to
           resume that task. */
        if (sockptr->s_RXTask_List.flink)
        {
            /* Get the task to resume off the list */
            task_entry_ptr = DLL_Dequeue (&sockptr->s_RXTask_List);

            /* Resume the task pending on a receive */
            if (NU_Resume_Task(task_entry_ptr->task) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
           }
        }
    }
    else
    {
        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        status = -1;
    }

    return (status);

} /* UDP_Append */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Recv_Data
*
*   DESCRIPTION
*
*       This function is responsible for receiving data across a network
*       during a connectionless transfer.  This function is called by
*       NU_Recv_From and NU_Recvmsg to receive data over a UDP socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *buff                   Pointer to the data buffer
*       nbytes                  Specifies the number of bytes of data
*       *from                   Pointer to the source protocol-specific
*                               address structure
*   OUTPUTS
*
*       > 0                     Number of bytes received.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource,
*                               but the socket is non-blocking.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       NU_DEVICE_DOWN          The device that this socket was communicating
*                               over has gone down. If the device is a PPP
*                               device it is likely the physical connection
*                               has been broken. If the device is ethernet
*                               and DHCP is being used, the lease of the IP
*                               address may have expired.
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
INT32 UDP_Recv_Data(INT socketd, CHAR *buff, UINT16 nbytes,
                    struct addr_struct *from)
{
    INT32               return_status = NU_SUCCESS;
    struct sock_struct  *sockptr;           /* pointer to current socket */
    struct SCK_TASK_ENT task_entry;         /* task entry for list operations */
    UINT32              socket_id;

    /*  Pick up a pointer to the socket list. */
    sockptr = SCK_Sockets[socketd];

    if (!(sockptr->s_state & SS_DEVICEDOWN))
    {
        /* If there is no data immediately pending on the socket, wait
         * for data to arrive on the socket.
         */
        if (!sockptr->s_recvpackets)
        {
            /* If there is an error on the connected socket, return the
             * error.  Note that an error will be set only if the socket
             * is connected, so we do not need to check that the socket
             * is connected here.
             */
            if (sockptr->s_error != 0)
            {
                return_status = sockptr->s_error;

                /* Reset the socket error value */
                sockptr->s_error = 0;
            }

            /* If this is a blocking socket, wait for data */
            else if (sockptr->s_flags & SF_BLOCK)
            {
                /* Suspend until data has been received, an ICMP error has
                   been set on the socket or the socket is closed / aborted. */
                do
                {
                    /* Initialize the list entry's task number */
                    task_entry.task = NU_Current_Task_Pointer();

                    /* Add it to the list of tasks pending on receive */
                    DLL_Enqueue(&sockptr->s_RXTask_List, &task_entry);

                    /* Get the socket ID to verify the socket after suspension */
                    socket_id = sockptr->s_struct_id;

                    /* Suspend the task */
                    SCK_Suspend_Task(task_entry.task);

                    /* If this is a different socket or the socket has been
                     * closed, handle it appropriately.
                     */
                    if ( (!SCK_Sockets[socketd]) ||
                         (SCK_Sockets[socketd]->s_struct_id != socket_id) ||
                         (SCK_Sockets[socketd]->s_state & SS_DEVICEDOWN) )
                    {
                        return_status = NU_SOCKET_CLOSED;
                        break;
                    }

                    /* If an ICMP Error has been set on the connected socket,
                     * return the error.
                     */
                    if (sockptr->s_error != 0)
                    {
                        return_status = sockptr->s_error;

                        /* Reset the socket error value */
                        sockptr->s_error = 0;

                        break;
                    }

                } while (!sockptr->s_recvpackets);
            }

            /* Otherwise, there is no data to return, and the socket is
             * non-blocking.  Return an error indicating this.
             */
            else
                return_status = NU_WOULD_BLOCK;
        }

        /* If there is not an error on the socket */
        if (return_status == NU_SUCCESS)
        {
            /* Retrieve the data into the user buffer. */
            if (!(sockptr->s_state & SS_DEVICEDOWN))
                return_status = UDP_Read(sockptr, buff, from, nbytes);

            /* The interface has gone down.  Return an error. */
            else
                return_status = NU_DEVICE_DOWN;
        }
    }
    else
    {
        /* The device that this socket was communicating over has gone down. If
           the device is a PPP device it is likely the physical connection has
           been broken. If the device is ethernet and DHCP is being used, the
           lease of the IP address may have expired. */
        return_status = NU_DEVICE_DOWN;
    }

    /* Trace log */
    T_SOCK_RX(nbytes, socketd, return_status);
    
    return (return_status);

} /* UDP_Recv_Data */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Read
*
*   DESCRIPTION
*
*       Get the data from the UDP buffer and transfer it into your buffer.
*       Returns the number of bytes transferred or 0 if none available.
*
*   INPUTS
*
*       *sockptr                Pointer to the socket structure list
*       *buffer                 Pointer to the buffer to read
*       *from                   Pointer to the address structure you are
*                               getting udp information from
*       nbytes                  Number of bytes to read
*
*   OUTPUTS
*
*       NU_NO_DATA_TRANSFER     There are no packets to receive.
*       bytes_copied            The number of bytes received.  Note that
*                               this could be zero if the packet
*                               contained no data.
*
*************************************************************************/
STATIC INT32 UDP_Read(struct sock_struct *sockptr, char *buffer,
                      struct addr_struct *from, UINT16 nbytes)
{
    INT32           bytes_copied;
    UINT16          hlen;
    VOID            *pkt;

    /* Check to see if there are any packets waiting. */
    if (sockptr->s_recvlist.head == NU_NULL)
        return (NU_NO_DATA_TRANSFER);

    /* Save a pointer to the IP header so the data pointer can be restored
     * to this point later.
     */
    pkt = sockptr->s_recvlist.head->data_ptr;

    /* Increment past the IP header to the UDP header */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* The total data length has already been decremented to exclude the
     * IPv6 header, IPv6 extension headers and UDP header.  The payload stored
     * in the IPv6 header includes the IPv6 extension headers.  Subtract the
     * total data length stored in the buffer from the payload length stored
     * in the header to determine the length of the IPv6 extension headers.
     */
    if (sockptr->s_recvlist.head->mem_flags & NET_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        hlen = (UINT16)(IP6_HEADER_LEN +
               (GET16(sockptr->s_recvlist.head->data_ptr, IP6_PAYLEN_OFFSET) -
               sockptr->s_recvlist.head->mem_total_data_len) - UDP_HEADER_LEN);

#if (INCLUDE_IPV4 == NU_TRUE)
    /* Increment past the IPv4 header to the UDP header */
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        hlen = (UINT16)((GET8(sockptr->s_recvlist.head->data_ptr,
                              IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);
#endif

    /* If this socket is not connected, fill in the socket structure with
     * information from the UDP header.
     */
    if (!(sockptr->s_state & SS_ISCONNECTED))
    {
        /* Copy the foreign address into the socket structure and the address
         * structure to return to the application from the IP header.
         */
        SCK_Copy_Addresses(sockptr, sockptr->s_recvlist.head->data_ptr, from,
                           sockptr->s_recvlist.head->mem_flags);

        /* Increment past the IP header */
        sockptr->s_recvlist.head->data_ptr += hlen;

        /* Fill in the foreign side's name and port number */
        from->name = NU_NULL;

        from->port = GET16(sockptr->s_recvlist.head->data_ptr, UDP_SRC_OFFSET);

        sockptr->s_foreign_addr.port_num = from->port;

        /* Increment past the UDP header to the UDP data */
        sockptr->s_recvlist.head->data_ptr += UDP_HEADER_LEN;
    }

    /* Increment past all headers to the UDP data */
    else
        sockptr->s_recvlist.head->data_ptr += (UDP_HEADER_LEN + hlen);

    /* If there are more bytes in the packet than the application is able
     * to receive, log an informational error.  The data will be truncated.
     */
    if (sockptr->s_recvlist.head->mem_total_data_len > nbytes)
        NLOG_Error_Log("UDP_Read received more data than can be accepted by the application",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

    /* Copy the data into the user buffer */
    bytes_copied = MEM_Copy_Buffer(buffer, sockptr, (INT32)nbytes);

    sockptr->s_recv_if = sockptr->s_recvlist.head->mem_buf_device;

    /* If the application set the socket option to return ancillary data.
     * Ancillary data will only be returned with a call to NU_Recvmsg;
     * therefore, do not set the ancillary data pointer if this is a zero
     * copy socket.
     */
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

} /* UDP_Read */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Send_Data
*
*   DESCRIPTION
*
*       Transmit the data for UDP.  This function is used by NU_Send_To
*       and NU_Sendmsg to transmit data over a UDP socket.
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
*       NU_WOULD_BLOCK          The system needs to suspend on a resource,
*                               but the socket is non-blocking.
*
*************************************************************************/
INT32 UDP_Send_Data(INT socketd, CHAR *buff, UINT16 nbytes,
                    const struct addr_struct *to)
{
    struct uport                    *uprt;
    struct sock_struct              *sockptr;
    INT32                           return_status;

    /* Pick up a pointer to the socket list entry.  */
    sockptr = SCK_Sockets[socketd];

    /* Verify that a port number exists */
    if (to->port)
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        if ( ((to->family == SK_FAM_IP6) &&
              (IPV6_IS_ADDR_UNSPECIFIED(to->id.is_ip_addrs)) )

#if (INCLUDE_IPV4 == NU_TRUE)
              ||
#endif

#else
        if (
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

             ((to->family == NU_FAMILY_IP) &&
              (TLS_Comparen (to->id.is_ip_addrs, (VOID *)IP_Null, 4))) )
#else
    )
#endif
        {
            return (NU_INVALID_ADDRESS);
        }

        /* Pick up the foreign IP address and port number. */
        sockptr->s_foreign_addr.port_num = to->port;

#if (INCLUDE_IPV6 == NU_TRUE)

        if (to->family == SK_FAM_IP6)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            if (IPV6_IS_ADDR_V4MAPPED(to->id.is_ip_addrs))
            {
                /* If the application is attempting to transmit to
                 * an IPv4 address, but the socket option to transmit
                 * only IPv6 packets on this socket has been set,
                 * return an error to the application.
                 */
                if (sockptr->s_options & SO_IPV6_V6ONLY)
                    return (NU_INVALID_ADDRESS);

                sockptr->s_flags |= SF_V4_MAPPED;
            }
#endif

            NU_BLOCK_COPY(sockptr->s_foreign_addr.ip_num.is_ip_addrs,
                          to->id.is_ip_addrs, IP6_ADDR_LEN);
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
#if (INCLUDE_IPV6 == NU_TRUE)

            /* If the application is attempting to transmit to an
             * IPv4 address, but the socket option to transmit only
             * IPv6 packets on this socket has been set, return an
             * error to the application.
             */
            if (sockptr->s_options & SO_IPV6_V6ONLY)
                return (NU_INVALID_ADDRESS);
#endif

            *(UINT32 *)sockptr->s_foreign_addr.ip_num.is_ip_addrs =
                *(UINT32 *)to->id.is_ip_addrs;
        }
#endif

        /* Fill in the port structure */
        uprt = UDP_Ports[sockptr->s_port_index];

#if (INCLUDE_IPV6 == NU_TRUE)

        if (to->family == SK_FAM_IP6)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            /* If this is an IPv4-Mapped socket, set up the foreign address
             * and family type of the port according to IPv4.
             */
            if (sockptr->s_flags & SF_V4_MAPPED)
            {
                /* Extract the IPv4 address from the IPv6 IPv4-Mapped address */
                IP6_EXTRACT_IPV4_ADDR(uprt->up_faddr, to->id.is_ip_addrs);

                uprt->up_faddr_struct.up_family = SK_FAM_IP;

                /* Remove the V4-Mapped flag from the socket */
                sockptr->s_flags &= ~SF_V4_MAPPED;
            }

            else
#endif
            {
                NU_BLOCK_COPY(uprt->up_faddrv6, to->id.is_ip_addrs,
                              IP6_ADDR_LEN);
                uprt->up_faddr_struct.up_family = SK_FAM_IP6;
            }
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

        {
            uprt->up_faddr = IP_ADDR(to->id.is_ip_addrs);
            uprt->up_faddr_struct.up_family = SK_FAM_IP;
        }
#endif

        /* Get his port number. */
        uprt->up_fport = to->port;

        /* Send the datagram */
        return_status = UDP_Send_Datagram(socketd, buff, (INT32)nbytes);

    }  /*  He specified a destination port. */

    else
        return_status = NU_INVALID_PARM;

    return (return_status);

} /* UDP_Send_Data */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Send_Datagram
*
*   DESCRIPTION
*
*       Transmit the data for UDP.  This function is used by NU_Send_To
*       and NU_Sendmsg to transmit data over a UDP socket.
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
*       > 0                     Number of bytes sent.
*       NU_NO_DATA_TRANSFER     The data transfer was not completed.
*       NU_DEVICE_DOWN          The device that this socket was communicating
*                               over has gone down. If the device is a PPP
*                               device it is likely the physical connection
*                               has been broken. If the device is ethernet
*                               and DHCP is being used, the lease of the IP
*                               address may have expired.
*       NU_WOULD_BLOCK          The system needs to suspend on a resource,
*                               but the socket is non-blocking.
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for a connected socket:
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
INT32 UDP_Send_Datagram(INT socketd, CHAR *buff, INT32 nbytes)
{
    INT32                           count = 0;
    INT32                           return_status = NU_SUCCESS;
    NET_BUFFER_SUSPENSION_ELEMENT   waiting_for_buffer;
    struct SCK_TASK_ENT             task_entry;
    UINT32                          socket_id;
    struct uport                    *uprt;
    UINT16                          local_s_flags;
    struct sock_struct              *sockptr;

    /* Get a pointer to the socket structure */
    sockptr = SCK_Sockets[socketd];

    T_SOCK_TX_LAT_START(socketd);
    
    /* Save off the flags for this socket in case we have to suspend */
    local_s_flags = sockptr->s_flags;

    /* Get a pointer to the UDP port structure */
    uprt = UDP_Ports[sockptr->s_port_index];

    do
    {
        /* If there is an error on the socket, return the error if this
         * is a connected socket.
         */
        if (sockptr->s_error != 0)
        {
            return_status = sockptr->s_error;

            /* Reset the socket error value */
            sockptr->s_error = 0;
        }

        else if (!(sockptr->s_state & SS_DEVICEDOWN))
        {
            /* Send the data. */
            count = UDP_Send(uprt, buff, nbytes);

            /*  Let them know if it worked OK. */
            if (count >= 0)
            {
                /* return number of bytes transferred */
                return_status = count;
            }

            else
            {
                /* If the status is NO BUFFERS then we will suspend until
                 * more buffers become available.
                 */
                if (count == NU_NO_BUFFERS)
                {
                    /* if NON blocking then return_status because we cannot suspend */
                    if (local_s_flags & SF_BLOCK)
                    {
                        /* Add this task to the element. */
                        waiting_for_buffer.waiting_task = NU_Current_Task_Pointer();

                        /* Set up the reference to the suspending task */
                        task_entry.task = NU_Current_Task_Pointer();

                        /* Get a reference to the memory buffer element */
                        task_entry.buff_elmt = &waiting_for_buffer;

                        /* Put this on the suspended TX list */
                        DLL_Enqueue(&sockptr->s_TXTask_List, &task_entry);

                        /* Setup a reference to the socket */
                        waiting_for_buffer.socketd = socketd;

                        /* Setup a reference to the task entry */
                        waiting_for_buffer.list_entry = &task_entry;

                        /* Put this element on the buffer suspension list */
                        DLL_Enqueue(&MEM_Buffer_Suspension_List, &waiting_for_buffer);

                        /* Get the socket ID to verify the socket after suspension */
                        socket_id = sockptr->s_struct_id;

                        /* Suspend this task. */
                        SCK_Suspend_Task (waiting_for_buffer.waiting_task);

                        /* Make sure this entry is removed from the
                           buffer suspension list. */
                        DLL_Remove(&MEM_Buffer_Suspension_List, &waiting_for_buffer);

                        /* If this is a different socket or the socket has been
                         * closed, handle it appropriately.
                         */
                        if ( (!SCK_Sockets[socketd]) ||
                             (SCK_Sockets[socketd]->s_struct_id != socket_id) ||
                             (SCK_Sockets[socketd]->s_state & SS_DEVICEDOWN) )
                        {
                            return_status = NU_SOCKET_CLOSED;
                            break;
                        }
                    }

                    else
                    {
                        return_status = NU_WOULD_BLOCK;
                        break;
                    }
                }
                else
                    /* return an error status */
                    return_status = NU_NO_DATA_TRANSFER;
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
    }
    /* While the socket descriptor is not NULL and the count == NU_NO_BUFFERS we
     * will continue trying to send
     */
    while ( (SCK_Sockets[socketd] != NU_NULL) && (count == NU_NO_BUFFERS) );

    /* Trace log */
    T_SOCK_TX_LAT_STOP(nbytes, socketd, return_status);
    
    return (return_status);

} /* UDP_Send_Datagram */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Send
*
*   DESCRIPTION
*
*       Send some data out in a udp packet.
*
*       Returns 0 on ok send, non-zero for an error
*
*   INPUTS
*
*       *uptr                   Pointer to UDP port list
*       *buffer                 Pointer to the buffer to send from
*       nbytes                  The number of bytes to send
*
*   OUTPUTS
*
*       stat
*       NU_NO_BUFFERS           There are not enough buffers in the
*                               system to fulfill the request.
*       NU_INVALID_PARM         The buffer is NULL when using Zero Copy.
*       -1
*       nbytes
*
*************************************************************************/
STATIC INT32 UDP_Send(UDP_PORT *uptr, CHAR *buffer, INT32 nbytes)
{
    NET_BUFFER          *buf_ptr;
    STATUS              stat;
    struct sock_struct  *sock_ptr = SCK_Sockets[uptr->up_socketd];
    UINT16              checksum;

#if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE))
    UDPLAYER            *udp_pkt;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    IP6_S_OPTIONS       ip6_options;
    UINT8               *src_addr;
#endif

    /* Before we do anything else make sure a route to the host is up. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (uptr->up_faddr_struct.up_family == SK_FAM_IP6)
    {
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        stat = UDP6_Cache_Route(uptr, uptr->up_faddrv6);

        /* If we successfully cached the route, extract the local IP address
         * to use from the route.
         */
        if (stat == NU_SUCCESS)
        {
            memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));

            /* Set up the parameters to be used for the transmission */
            stat = IP6_Setup_Options(uptr->up_ancillary_data,
                                     uptr->up_sticky_options, &ip6_options,
                                     uptr->up_faddrv6, &uptr->up_routev6,
                                     (UINT8)uptr->up_ttl);

            if (stat != NU_SUCCESS)
                return (stat);

            /* If an address was specified as Ancillary data, use that
             * address for this datagram.
             */
            if ( (uptr->up_ancillary_data) &&
                 (uptr->up_ancillary_data->tx_source_address) )
            {
                NU_BLOCK_COPY(uptr->up_laddrv6,
                              uptr->up_ancillary_data->tx_source_address,
                              IP6_ADDR_LEN);

                /* If this address does not exist on the specified interface, return
                 * an error.
                 */
                if (DEV6_Find_Target_Address(ip6_options.tx_route.rt_route->rt_device,
                                             uptr->up_laddrv6) == NU_NULL)
                    return (-1);
            }

            /* Check if a source address has been specified using a Sticky
             * Option.
             */
            else if ( (uptr->up_sticky_options) &&
                      (uptr->up_sticky_options->tx_source_address) )
            {
                NU_BLOCK_COPY(uptr->up_laddrv6,
                              uptr->up_sticky_options->tx_source_address,
                              IP6_ADDR_LEN);

                /* If this address does not exist on the specified interface, return
                 * an error.
                 */
                if (DEV6_Find_Target_Address(ip6_options.tx_route.rt_route->rt_device,
                                             uptr->up_laddrv6) == NU_NULL)
                    return (-1);
            }

            /* If a specific address was not bound to, select an address of the
             * same scope as the destination.
             */
            else if ( (!(sock_ptr->s_flags & SF_BIND)) ||
                      (IP_ADDR(sock_ptr->s_local_addr.ip_num.is_ip_addrs)
                       == IP_ADDR_ANY) )
            {
                src_addr =
                    in6_ifawithifp(ip6_options.tx_route.rt_route->rt_device,
                                   uptr->up_faddrv6);

                /* If an address of the same scope does not exist, return an error */
                if (src_addr)
                    NU_BLOCK_COPY(uptr->up_laddrv6, src_addr, IP6_ADDR_LEN);

                else
                    return (-1);
            }

            /* Otherwise, use the bound-to address as the source address of
             * the packet.
             */
            else
            {
                NU_BLOCK_COPY(uptr->up_laddrv6,
                              sock_ptr->s_local_addr.ip_num.is_ip_addrs,
                              IP6_ADDR_LEN);
            }

            ip6_options.tx_source_address = uptr->up_laddrv6;
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
        stat = UDP4_Cache_Route(uptr, uptr->up_faddr);

        /* If we successfully cached the route, extract the local IP address
         * to use from the route.
         */
        if (stat == NU_SUCCESS)
        {
            /* If a specific address was not bound to, use the primary address of
             * the interface associated with the route as the source address of
             * the packet.
             */
            if ( (!(sock_ptr->s_flags & SF_BIND)) ||
                 (IP_ADDR(sock_ptr->s_local_addr.ip_num.is_ip_addrs) == IP_ADDR_ANY) )
                uptr->up_laddr = uptr->up_route.rt_route->rt_entry_parms.
                                 rt_parm_device->dev_addr.dev_addr_list.
                                 dv_head->dev_entry_ip_addr;

            /* Otherwise, use the bound-to address as the source address of
             * the packet.
             */
            else
                uptr->up_laddr = IP_ADDR(sock_ptr->s_local_addr.ip_num.is_ip_addrs);
        }
        else
            return (stat);
    }
#endif

    /* If this is a Zero Copy buffer, the data is already contained in
     * NET buffers.
     */
    if (sock_ptr->s_flags & SF_ZC_MODE)
    {
        /* Ensure a buffer was passed into the routine. */
        if (buffer)
        {
            buf_ptr = (NET_BUFFER*)buffer;
        }

        /* Otherwise, return an error. */
        else
        {
            return (NU_INVALID_PARM);
        }
    }

    /* Otherwise, allocate a chain of NET buffers and copy the data */
    else
    {
        buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                           nbytes + UDP_HEADER_LEN);

        /* If the buffer is NULL, return an error */
        if (buf_ptr == NU_NULL)
            return (NU_NO_BUFFERS);
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Set the UDP port pointer. This will enable IPsec to use cached
     * information in the port structure when adding its IPsec headers.
     */
    buf_ptr->mem_port = uptr;

#endif

    /* Set the data pointer to start just past the UDP header */
    buf_ptr->data_ptr = buf_ptr->mem_parent_packet + UDP_HEADER_LEN;

    /* If UDP checksumming is not disabled, this is not a Zero Copy buffer
     * and the hardware has not been configured to compute the checksum.
     */
    if ( (!(sock_ptr->s_options & SO_UDP_NOCHECKSUM)) &&
         (!(sock_ptr->s_flags & SF_ZC_MODE))
#if (HARDWARE_OFFLOAD == NU_TRUE)
         && ((uptr->up_route.rt_route)&& (!(uptr->up_route.rt_route->rt_entry_parms.rt_parm_device->
            dev_hw_options_enabled & HW_TX_UDP_CHKSUM)))
#endif
        )
    {
        /* Set the flag indicating that checksumming should be done in unison
         * with data copy.
         */
        buf_ptr->mem_flags |= NET_BUF_SUM;
    }

    /* Copy the data into the chain of NET buffers.  This function also
     * updates the data length of each buffer, so it is used for
     * zerocopy buffers too.
     */
    nbytes = MEM_Copy_Data(buf_ptr, buffer, nbytes, sock_ptr->s_flags);

    /* Increment the total data length by the size of the UDP Header */
    buf_ptr->mem_total_data_len += UDP_HEADER_LEN;

    /* Set the data pointer to the beginning of the UDP Header. */
    buf_ptr->data_ptr -= UDP_HEADER_LEN;

    /* Initialize the local and foreign port numbers. */
    PUT16(buf_ptr->data_ptr, UDP_SRC_OFFSET, uptr->up_lport);
    PUT16(buf_ptr->data_ptr, UDP_DEST_OFFSET, uptr->up_fport);

#if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE))
    /* Set the UDP header pointer to that it can be filled in. */
    udp_pkt = (UDPLAYER *)buf_ptr->data_ptr;
#endif

    /* Get the length of the buffer. */
    PUT16(buf_ptr->data_ptr, UDP_LENGTH_OFFSET, (UINT16)buf_ptr->mem_total_data_len);

    /* Clear the checksum field before calculating the real checksum. */
    PUT16(buf_ptr->data_ptr, UDP_CHECK_OFFSET, 0);

    /* Increment the data length of the first buffer by the size of the
     * UDP Header
     */
    buf_ptr->data_len += UDP_HEADER_LEN;

#if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE))
    /* Log the UDP header info */
    NLOG_UDP_Info(udp_pkt, NLOG_TX_PACK);
#endif

    /* Calculate the checksum. */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    if (uptr->up_faddr_struct.up_family == SK_FAM_IP6)
    {
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
#if (HARDWARE_OFFLOAD == NU_TRUE)
        if ((uptr->up_routev6.rt_route)&&(uptr->up_routev6.rt_route->rt_entry_parms.rt_parm_device->
            dev_hw_options_enabled & HW_TX_UDP6_CHKSUM))
        {
            checksum = 0;
        }
        else
#endif
        {
            checksum = UTL6_Checksum(buf_ptr, ip6_options.tx_source_address,
                                     uptr->up_faddrv6, buf_ptr->mem_total_data_len,
                                     IP_UDP_PROT, IP_UDP_PROT);

            /* If a checksum of zero is computed, insert 0xffff. */
            if (checksum == 0)
                checksum = 0xFFFF;
        }
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    }
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    {
        /* If the socket option is set to not compute a checksum or the
         * checksum will be computed in hardware, do not compute a
         * checksum in software.
         */
        if ( (sock_ptr->s_options & SO_UDP_NOCHECKSUM)
#if (HARDWARE_OFFLOAD == NU_TRUE)
              || ((uptr->up_route.rt_route)&& (uptr->up_route.rt_route->rt_entry_parms.rt_parm_device->
                  dev_hw_options_enabled & HW_TX_UDP_CHKSUM))
#endif
           )
            checksum = 0;

        /* Otherwise, compute the checksum */
        else
        {
            checksum = UTL_Checksum(buf_ptr, uptr->up_laddr,
                                    uptr->up_faddr, IP_UDP_PROT);

            /* If a checksum of zero is computed, insert 0xffff. */
            if (checksum == 0)
                checksum = 0xFFFF;
        }
    }
#endif

    /* Put the checksum in the packet */
    PUT16(buf_ptr->data_ptr, UDP_CHECK_OFFSET, checksum);

    /* Set the deallocation list pointer. */
    buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

    /* Send this packet. */
#if (INCLUDE_IPV6 == NU_TRUE)

    if (uptr->up_faddr_struct.up_family == SK_FAM_IP6)
    {
        ip6_options.tx_dest_address = uptr->up_faddrv6;

        stat = IP6_Send(buf_ptr, &ip6_options, IP_UDP_PROT,
                        &ip6_options.tx_route, 0, sock_ptr->s_moptions_v6);
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

        stat = IP_Send(buf_ptr, &uptr->up_route,
                       uptr->up_route.rt_ip_dest.sck_addr, uptr->up_laddr,
                       IP_ALLOWBROADCAST, uptr->up_ttl, IP_UDP_PROT,
                       uptr->up_tos, sock_ptr->s_moptions_v4);
#endif

    /* If the packet was successfully sent, increment the number of
     * UDP datagrams transmitted.
     */
    if (stat == NU_SUCCESS)
    {
        MIB2_udpOutDatagrams_Inc;
    }

    /* The packet was not sent.  Deallocate the buffer.  If the packet was
     * transmitted it will be deallocated later by the appropriate MAC layer
     * TX routine.
     */
    else
    {
        /* If this is a zero copy buffer, the application is responsible
         * for freeing the buffer in the event of an error.
         */
        if (!(sock_ptr->s_flags & SF_ZC_MODE))
            MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        /* Restore the values of the buffer data pointer and lengths */
        else
        {
            buf_ptr->data_ptr += UDP_HEADER_LEN;
            buf_ptr->mem_total_data_len -= UDP_HEADER_LEN;
            buf_ptr->data_len -= UDP_HEADER_LEN;
        }

        return (stat);
    }

    /*  If the send went ok, then return the number of bytes sent. */
    return (nbytes);

} /* UDP_Send */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Port_Cleanup
*
*   DESCRIPTION
*
*       This function frees any resources associate with a UDP port when
*       that port is closed.
*
*   INPUTS
*
*       uport_index             The index of the port that is closing.
*       *sockptr                A pointer to the socket structure
*                               associated with the port.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS UDP_Port_Cleanup(UINT16 uport_index, struct sock_struct *sockptr)
{
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_STATIC_BUILD == NU_TRUE) )
    INT     j;
#endif

#if ( ((INCLUDE_IPV6 == NU_FALSE) || (INCLUDE_IPV4 == NU_FALSE)) && \
      (INCLUDE_SR_SNMP == NU_FALSE) )
    UNUSED_PARAMETER(sockptr);
#endif

    /* Free the cached route */
    if (UDP_Ports[uport_index]->up_route.rt_route)
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        /* If the socket type is IPv6 and the foreign address is not
         * IPv4-Mapped.
         */
        if ( (sockptr->s_family == SK_FAM_IP6)
#if (INCLUDE_IPV4 == NU_TRUE)
             &&
             (!(IPV6_IS_ADDR_V4MAPPED(sockptr->s_foreign_addr.ip_num.is_ip_addrs)))
#endif
             )
            RTAB_Free((ROUTE_ENTRY*)UDP_Ports[uport_index]->up_routev6.rt_route,
                      NU_FAMILY_IP6);

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            RTAB_Free((ROUTE_ENTRY*)UDP_Ports[uport_index]->up_route.rt_route,
                      NU_FAMILY_IP);

#if (INCLUDE_SR_SNMP == NU_TRUE)
            /* Update the UDP Listen Table. */
            SNMP_udpListenTableUpdate(SNMP_DELETE,
                                      sockptr->s_local_addr.ip_num.is_ip_addrs,
                                      (INT32)(sockptr->s_local_addr.port_num));
#endif
        }
#endif
    }

#if (INCLUDE_IPV6 == NU_TRUE)
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Traverse the flag array to find the used memory location*/
    for (j = 0; j != NSOCKETS; j++)
    {
        /* If this is the memory area being released */
        if (&NET_Sticky_Options_Memory[j] ==
            UDP_Ports[uport_index]->up_sticky_options)
        {
            /* Turn the memory flag off */
            NET_Sticky_Options_Memory_Flags[j] = NU_FALSE;
        }
    }

#else
    /* If there is memory allocated for Sticky Options, deallocate it */
    if (UDP_Ports[uport_index]->up_sticky_options)
    {
        if (NU_Deallocate_Memory((VOID*)UDP_Ports[uport_index]->
                                 up_sticky_options) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for sticky options",
                           NERR_SEVERE, __FILE__, __LINE__);
    }
#endif
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /*  Clear this port list entry.  */
    if (NU_Deallocate_Memory((VOID*)UDP_Ports[uport_index]) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for UDP port",
                       NERR_SEVERE, __FILE__, __LINE__);
#endif

    /*  Indicate that this port is no longer used. */
    UDP_Ports[uport_index] = NU_NULL;

    return (NU_SUCCESS);

} /* UDP_Port_Cleanup */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Make_Port
*
*   DESCRIPTION
*
*       This is the initialization for UDP based communication.  When a
*       port needs to be created, this routine is called to do as much
*       pre-initialization as possible to save overhead during operation.
*
*       This structure is created upon open of a port, either listening
*       or wanting to send.
*
*   INPUTS
*
*       myport                  The port to make
*       socketd                 The socket descriptor
*
*   OUTPUTS
*
*       INT16
*
*************************************************************************/
INT32 UDP_Make_Port(UINT16 myport, INT socketd)
{
    INT16       i, retval;
    UDP_PORT    *p;
    VOID        *return_ptr;       /* pointer to memory block */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    STATUS      status;
#else
#if (INCLUDE_IPV6 == NU_TRUE)
    INT         j;
#endif
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check to see if there are any available ports. */
    for (i = 0; (i < UDP_MAX_PORTS) && (UDP_Ports[i] != NU_NULL); i++)
        ;

    /* If an unused UDP port was not found return an error. */
    if (i >= UDP_MAX_PORTS)
    {
        NLOG_Error_Log ("No unused UDP ports available",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        retval = -1;
    }
    else
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Allocate_Memory(MEM_Cached, (VOID**)&return_ptr,
                                    (UNSIGNED)sizeof(struct uport),
                                    (UNSIGNED)NU_NO_SUSPEND);

        /* check status of memory allocation */
        if (status != NU_SUCCESS)
        {
            /* ERROR memory allocation error. */
            NLOG_Error_Log ("Unable to allocate memory for the UDP port struct",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Return to user mode */
            NU_USER_MODE();
            return (-1);               /* out of room for ports */
        } /* end if */

        else

#else
        /* Assign memory to the new UDP port */
        return_ptr = &NET_UDP_Ports_Memory[i];
#endif

        {
            return_ptr = (UINT16 *)TLS_Normalize_Ptr(return_ptr);
            p = (UDP_PORT *)return_ptr;
        }

#if ( (INCLUDE_STATIC_BUILD == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )

        /* Traverse the flag array to find the unused memory location*/
        for (j = 0; j != NSOCKETS; j++)
        {
            /* If this memory is available, use it */
            if (NET_Sticky_Options_Memory_Flags[j] != NU_TRUE)
            {
                /* Assign sticky options pointer to this memory */
                p->up_sticky_options =
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

        UDP_Ports[i] = p;
        retval = i;

        /* Clear the UDP port structure. */
        UTL_Zero((CHAR *)p, sizeof(*p));

        /* If my port number has not been specified then find a unique one
         * for me.
         */
        if (myport == 0)
            myport =
                PRT_Get_Unique_Port_Number((UINT16)NU_PROTO_UDP,
                                           SCK_Sockets[socketd]->s_family);

        p->up_lport = myport;                   /* save for incoming comparison */

        /* The socket with which this port is associated is unknown at this time. */
        p->up_socketd = socketd;

        p->up_route.rt_route = NU_NULL;

#if ( (INCLUDE_SR_SNMP == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

        if (SCK_Sockets[socketd]->s_family == NU_FAMILY_IP)
        {
            /* Update the UDP Listen Table. */
            SNMP_udpListenTableUpdate(SNMP_ADD,
                                      SCK_Sockets[socketd]->s_local_addr.ip_num.is_ip_addrs,
                                      (INT32)(myport));
        }
#endif
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (retval);

} /* UDP_Make_Port */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Handle_Datagram_Error
*
*   DESCRIPTION
*
*       This function finds the appropriate socket for an incoming
*       ICMP error message, sets the error field of the socket and
*       resumes any tasks that are suspended.
*
*   INPUTS
*
*       family                  The family - either SK_FAM_IP for IPv4
*                               or SK_FAM_IP6 for IPv6.
*       *buf_ptr                A pointer to the packet.
*       *src_addr               A pointer to the source address of the
*                               error packet.
*       *dest_addr              A pointer to the destination address of
*                               the error packet.
*       error                   The error received.
*
*   OUPUTS
*
*       NU_SUCCESS
*       -1                      A matching port does not exist or there
*                               is already an error on the socket.
*
*************************************************************************/
STATUS UDP_Handle_Datagram_Error(INT16 family, const NET_BUFFER *buf_ptr,
                                 const UINT8 *src_addr, const UINT8 *dest_addr,
                                 INT32 error)
{
    UINT16              dest_port, src_port;
    INT32               port_index;
    struct sock_struct  *sckptr;
    STATUS              status;

#if (INCLUDE_IPSEC == NU_TRUE)
    UINT8               prt_found;
    UDP_PORT            *prt;
#endif

#if (INCLUDE_IPSEC == NU_TRUE)

    /* The ICMP Destination Unreachable message generated for the IPsec
     * packet encapsulates the original IP header with 8 data bytes of the
     * offending datagram. For IPsec these data bytes are part of the AH
     * or ESP header. Therefore the source and destination ports are not
     * available.
     *
     * In order to find the corresponding socket for setting the error
     * iterate through all UDP ports. Matching criteria is based on
     * source and destination addresses
     */

    status = NU_SUCCESS;

    /* Check whether IPsec is enabled for the device which received this
     * packet.
     */
    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
    {
        for (port_index = 0; port_index < UDP_MAX_PORTS; port_index++)
        {
            prt_found = NU_FALSE;

            prt = UDP_Ports[port_index];

            if (prt != NU_NULL)
            {
#if (INCLUDE_IPV4 == NU_TRUE)
                /* Compare ports family */
                if (SCK_Sockets[prt->up_socketd]->s_family == SK_FAM_IP)
                {
                    /* Compare the ports local and foreign addresses */
                    if ( (prt->up_laddr == IP_ADDR(src_addr)) &&
                         (prt->up_faddr == IP_ADDR(dest_addr)) )
                    {
                        /* Address matched, port found */
                        prt_found = NU_TRUE;
                    }
                }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                /* Compare ports family */
                if (SCK_Sockets[prt->up_socketd]->s_family == SK_FAM_IP6)
                {
                    /* Compare the ports local and foreign addresses */
                    if ( (memcmp(prt->up_laddrv6, src_addr,
                                 IP6_ADDR_LEN) == 0) &&
                         (memcmp(prt->up_faddrv6, dest_addr,
                                 IP6_ADDR_LEN) == 0) )
                    {
                        /* Address matched, port found */
                        prt_found = NU_TRUE;
                    }
                }
#endif
            }

            /* If this is the port we want, assign an error code */
            if (prt_found == NU_TRUE)
            {
                if (prt->up_socketd >= 0)
                {
                    /* Get a pointer to the socket data structure */
                    sckptr = SCK_Sockets[prt->up_socketd];

                    /* Set the socket error */
                    status = SCK_Set_Socket_Error(sckptr, error);
                }
            }
        }
    }

    else
#endif
    {
        /* Get the destination and source port */
        dest_port = GET16(buf_ptr->data_ptr, UDP_DEST_OFFSET);
        src_port = GET16(buf_ptr->data_ptr, UDP_SRC_OFFSET);

        /* Find the port index matching the given parameters */
        port_index = PRT_Find_Matching_Port(family, NU_PROTO_UDP, src_addr,
                                            dest_addr, src_port, dest_port);

        /* If a target port was found */
        if (port_index >= 0)
        {
            /* If the port is pointing to a valid socket index, and the socket
             * is connected, set the error.
             */
            if (UDP_Ports[port_index]->up_socketd >= 0)
            {
                /* Get a pointer to the socket data structure */
                sckptr = SCK_Sockets[UDP_Ports[port_index]->up_socketd];

                /* Set the socket error */
                if (sckptr->s_state & SS_ISCONNECTED)
                    status = SCK_Set_Socket_Error(sckptr, error);
                else
                    status = -1;
            }
            else
                status = -1;
        }
        else
            status = -1;
    }

    return (status);

} /* UDP_Handle_Datagram_Error */

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Get_Pnum
*
*   DESCRIPTION
*
*       This function is responsible for returning a UDP_Ports entry
*       number to a caller based on a match with the port information
*       sent in the socket descriptor.
*
*   INPUTS
*
*       *socket_desc            A pointer to the socket structure
*
*   OUTPUTS
*
*       INT16                   Port list index
*
*************************************************************************/
INT16 UDP_Get_Pnum(const struct sock_struct *socket_desc)
{
    INT16 pnum = NU_IGNORE_VALUE;    /* TCP_Ports entry number initialized */
    INT16 index;
    struct uport *uprt;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    for (index = 0; index < UDP_MAX_PORTS; index++)
    {
        uprt = UDP_Ports[index];

        if ( (uprt != NU_NULL) &&
             (uprt->up_lport == socket_desc->s_local_addr.port_num) &&
             (SCK_Sockets[uprt->up_socketd]->s_family == socket_desc->s_family) )
        {
            pnum=index;
            break;
        }
    } /* end for */

    /* Return to user mode */
    NU_USER_MODE();

    /* return a UDP_Ports entry number to the caller */
    return (pnum);

} /* UDP_Get_Pnum */

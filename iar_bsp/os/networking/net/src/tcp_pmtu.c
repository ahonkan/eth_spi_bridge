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

/***********************************************************************
*
*   FILE NAME
*
*       tcp_pmtu.c
*
*   COMPONENT
*
*       PMTU - Path MTU Discovery
*
*   DESCRIPTION
*
*       This file contains the routines that affect managing the PMTU
*       of a TCP connection.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCP_PMTU_Repacket
*       TCP_PMTU_Increase_SMSS
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

extern TCP_BUFFER_LIST  TCP_Buffer_List;

/************************************************************************
*
*   FUNCTION
*
*       TCP_PMTU_Repacket
*
*   DESCRIPTION
*
*       This routine repackets all data on the retransmission list for
*       a TCP connection to fit within the bounds of the new Path MTU
*       as learned from Path MTU Discovery.
*
*   INPUTS
*
*       *prt                    A pointer to the port.
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TCP_PMTU_Repacket(TCP_PORT *prt)
{
    NET_BUFFER          *current_buffer = NU_NULL;
    NET_BUFFER          *new_buffer, *send_buffer;
    TCPLAYER            *tcp_header;
    INT32               bytes_left;
    UINT8               tcp_hlen;
    INT32               path_mtu, interface_pmtu, bytes_to_copy;
    UINT32              seq_number;
    TCP_BUFFER_LIST     send_list;
    TCP_BUFFER          *tcp_buf, *new_tcp_buf, *first_buf = NU_NULL;
    INT                 old_level;

    send_list.tcp_head = NU_NULL;
    send_list.tcp_tail = NU_NULL;

    /* If the connection is not using the Host Route, find the host
     * route to the destination.
     */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    if (prt->portFlags & TCP_FAMILY_IPV6)
    {
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        if (prt->tp_routev6.rt_route->rt_route_node->rt_submask_length != 128)
        {
            /* Free the network route before setting the cached route to
             * the newly created host route.
             */
            RTAB_Free((ROUTE_ENTRY*)prt->tp_routev6.rt_route, NU_FAMILY_IP6);

            prt->tp_routev6.rt_route = NU_NULL;
            IP6_Find_Route(&prt->tp_routev6);
        }

        /* Calculate the Path MTU */
        interface_pmtu = (INT32)(prt->tp_routev6.rt_route->rt_path_mtu -
            (IP6_HEADER_LEN + IP_HEADER_LEN));

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    {
        if (prt->tp_route.rt_route->rt_route_node->rt_submask_length != 32)
        {
            /* Free the network route before setting the cached route to
             * the newly created host route.
             */
            RTAB_Free((ROUTE_ENTRY*)prt->tp_route.rt_route, NU_FAMILY_IP);

            prt->tp_route.rt_route = NU_NULL;
            IP_Find_Route(&prt->tp_route);
        }

        /* Calculate the Path MTU */
        interface_pmtu = (INT32)(prt->tp_route.rt_route->rt_path_mtu - IP_HEADER_LEN);
    }
#endif

    tcp_buf = DLL_Dequeue(&prt->out.packet_list);

    /* Re-packet each item on the Retransmission List */
    while (tcp_buf)
    {
        /* We must lock out interrupts to check the flags parameter of the
         * buffer since a network driver can modify this value from a HISR.
         */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Check that the packet is not still on an interface's output queue. */
        if (!(tcp_buf->tcp_buf_ptr->mem_flags & NET_TX_QUEUE))
        {
            NU_Local_Control_Interrupts(old_level);

            /* Get a pointer to the first buffer in the list */
            current_buffer = tcp_buf->tcp_buf_ptr;

            /* Save a pointer to the TCP Header */
            tcp_header = (TCPLAYER*)current_buffer->data_ptr;

            /* Determine the length of the TCP Header */
            tcp_hlen = (UINT8)(GET8(tcp_header, TCP_HLEN_OFFSET) >> 2);

            /* Decrement the Path MTU by the TCP Header Length */
            path_mtu = (interface_pmtu - tcp_hlen);

            /* Determine the number of bytes of data to copy from this packet.
             * Do not include the TCP Header.
             */
            bytes_left = (INT32)(current_buffer->mem_total_data_len - tcp_hlen);

            /* Save a pointer to this buffer as the first one that is being
             * put back onto the retransmission list, indicating that all
             * buffers have been processed in the list.
             */
            if (!first_buf)
            {
                first_buf = tcp_buf;
            }

            /* Only repacket this packet if the number of TCP bytes violates
             * the new Path MTU recorded by ICMP.
             */
            if (bytes_left > path_mtu)
            {
                /* Zero out the checksum */
                PUT16(tcp_header, TCP_CHECK_OFFSET, 0);

                current_buffer->mem_flags &= ~NET_BUF_SUM;

                /* Remove the data in the first packet from the number of bytes
                 * to repacket since the first packet will reuse the buffers
                 * already allocated.
                 */
                bytes_left -= path_mtu;

                /* Repacket the remaining TCP data into new TCP packet(s) */
                do
                {
                    /* If bytes_left is greater than path_mtu, copy path_mtu
                     * bytes of data into the new buffer chain; otherwise,
                     * copy bytes_left bytes of data into the new buffer chain.
                     */
                    bytes_to_copy =
                        (bytes_left > path_mtu) ? path_mtu : bytes_left;

                    /* Allocate a new buffer chain into which to copy the
                     * data.  Don't forget room for the TCP Header.
                     */
                    new_buffer =
                        MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                                 bytes_to_copy + tcp_hlen);

                    /* If a buffer chain was allocated, fill it in */
                    if (new_buffer)
                    {
                        /* Set up the data pointer and dlist */
                        new_buffer->data_ptr = new_buffer->mem_parent_packet;

                        new_buffer->mem_dlist = NU_NULL;

                        /* Copy the original TCP header into the buffer */
                        NU_BLOCK_COPY(new_buffer->data_ptr, tcp_header, tcp_hlen);

                        /* Increment the data lengths to reflect the data in
                         * the packet.
                         */
                        new_buffer->mem_total_data_len = new_buffer->data_len =
                            tcp_hlen;

                        /* Copy the data from the original buffer */
                        MEM_Chain_Copy(new_buffer, current_buffer,
                                       (INT32)(current_buffer->mem_total_data_len -
                                       bytes_left), bytes_to_copy);

#if (INCLUDE_IPSEC == NU_TRUE)

                        /* Set mem_port of the newly allocated NET buffer. */
                        new_buffer->mem_port = current_buffer->mem_port;
#endif

                        /* Increment the data length to reflect the data just
                         * copied into the buffer.
                         */
                        new_buffer->mem_total_data_len += bytes_to_copy;

                        /* Determine the number of bytes remaining to be copied
                         * from the old buffer.
                         */
                        bytes_left -= bytes_to_copy;

                        /* Update the number of TCP bytes in the packet */
                        new_buffer->mem_tcp_data_len = (UINT16)bytes_to_copy;

                        new_tcp_buf = DLL_Dequeue(&TCP_Buffer_List);

                        new_tcp_buf->tcp_buf_ptr = new_buffer;

                        /* Put this packet on the list of packets to send */
                        DLL_Enqueue(&send_list, new_tcp_buf);
                    }

                    /* Otherwise, get out of the loop */
                    else
                        break;

                } while (bytes_left > 0);

                /* Adjust the SMSS */
                prt->p_smss = (UINT16)path_mtu;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

                /* If congestion control is enabled on the socket. */
                if (!(prt->portFlags & TCP_DIS_CONGESTION))
                {
                    /* RFC 1191 - section 6.4 - A Datagram Too Big Message should
                     * trigger the Slow-Start mechanism.
                     */
                    prt->p_ssthresh = TCP_SLOW_START_THRESHOLD;
                }
#endif

                /* If an error occurred, put the original buffer back on the
                 * retransmit list, and exit the loop.
                 */
                if (new_buffer == NU_NULL)
                {
                    DLL_Enqueue(&prt->out.packet_list, tcp_buf);

                    new_tcp_buf = DLL_Dequeue(&send_list);

                    /* Free each of the repacketed buffers awaiting retransmission */
                    while (new_tcp_buf)
                    {
                        /* Get a pointer to the buffer and free the buffer */
                        current_buffer = new_tcp_buf->tcp_buf_ptr;

                        MEM_One_Buffer_Chain_Free(current_buffer,
                                                  &MEM_Buffer_Freelist);

                        /* Put the TCP Buffer element back on the list of free
                         * elements
                         */
                        DLL_Enqueue(&TCP_Buffer_List, new_tcp_buf);

                        /* Get the next element on the list */
                        new_tcp_buf = DLL_Dequeue(&send_list);
                    }

                    break;
                }

                /* Add the TCP header length of the first packet back onto the
                 * Path MTU.
                 */
                path_mtu += tcp_hlen;

                /* Adjust the total data length of this chain and the data length
                 * of each buffer in the chain to reflect the truncation of the
                 * data.
                 */
                for (new_buffer = current_buffer, bytes_to_copy = 0;
                     new_buffer && (bytes_to_copy < path_mtu);
                     new_buffer = new_buffer->next_buffer)
                {
                    /* If the number of bytes in this buffer will violate the
                     * new Path MTU, decrement the number of bytes in this
                     * buffer.
                     */
                    if ((INT32)(new_buffer->data_len + bytes_to_copy) > path_mtu)
                    {
                        new_buffer->data_len = (UINT32)(path_mtu - bytes_to_copy);

                        /* Free the extra buffers appended to buffer chain */
                        MEM_One_Buffer_Chain_Free(new_buffer->next_buffer,
                                                  &MEM_Buffer_Freelist);

                        /* Truncate the chain */
                        new_buffer->next_buffer = NU_NULL;
                    }

                    bytes_to_copy += new_buffer->data_len;
                }

                /* Save a pointer to the sequence number */
                seq_number = GET32(current_buffer->data_ptr, TCP_SEQ_OFFSET);

                /* Set the total data length of the first buffer to the Path
                 * MTU plus the TCP Header.
                 */
                current_buffer->mem_total_data_len = (UINT32)bytes_to_copy;

                current_buffer->mem_tcp_data_len = (UINT16)(bytes_to_copy - tcp_hlen);

                /* Get a pointer to the first buffer to put on the retransmit
                 * list
                 */
                send_buffer = current_buffer;

                /* Update the sequence number of each buffer in the list
                 * and put it on the retransmission list.
                 */
                do
                {
                    /* Store the sequence number */
                    PUT32(send_buffer->data_ptr, TCP_SEQ_OFFSET, seq_number);

                    /* Set the sequence number for the buffer */
                    send_buffer->mem_seqnum = seq_number;

                    /* Update the sequence number of the next buffer */
                    seq_number += send_buffer->mem_tcp_data_len;

                    /* Set the flag indicating that this packet should not
                     * affect the congestion window.
                     */
                    send_buffer->mem_flags |= NET_TCP_PMTU;

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)
                    /* Do not recompute the checksum here if the timestamp option is
                     * enabled since the checkusm will be updated when the packet is
                     * retransmitted.
                     */
                    if (!(prt->portFlags & TCP_REPORT_TIMESTAMP))
#endif
                    {
#if (INCLUDE_IPV6 == NU_TRUE)

                        if (prt->portFlags & TCP_FAMILY_IPV6)
                        {
                            PUT16(send_buffer->data_ptr, TCP_CHECK_OFFSET,
                                  UTL6_Checksum(send_buffer, prt->tcp_laddrv6,
                                                prt->tcp_faddrv6,
                                                send_buffer->mem_total_data_len,
                                                IP_TCP_PROT, IP_TCP_PROT));
                        }

#if (INCLUDE_IPV4 == NU_TRUE)
                        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                        {
                            PUT16(send_buffer->data_ptr, TCP_CHECK_OFFSET,
                                  UTL_Checksum(send_buffer, prt->tcp_laddrv4,
                                               prt->tcp_faddrv4, IP_TCP_PROT));
                        }
#endif
                    }

                    /* Put this buffer on the retransmission list */
                    DLL_Enqueue(&prt->out.packet_list, tcp_buf);

                    /* Get a pointer to the next buffer to send */
                    tcp_buf = DLL_Dequeue(&send_list);

                    /* Get a pointer to the NET buffer */
                    if (tcp_buf)
                        send_buffer = tcp_buf->tcp_buf_ptr;

                } while (tcp_buf);

            } /* if (bytes_left > path_mtu) */

            else
            {
                /* Put this buffer on the retransmission list.  Preserve the original
                 * ordering of the packets.
                 */
                DLL_Enqueue(&prt->out.packet_list, tcp_buf);
            }
        }

        /* Otherwise, do not attempt to repacket or retransmit this
         * packet since it is still on the device's output queue.
         */
        else
        {
            NU_Local_Control_Interrupts(old_level);

            /* Put this packet back on the list. */
            DLL_Enqueue(&prt->out.packet_list, tcp_buf);
        }

        /* If all the buffers have been processed, exit the loop. */
        if (prt->out.packet_list.tcp_head == first_buf)
        {
            break;
        }

        /* Get a pointer to the next buffer in the list */
        tcp_buf = DLL_Dequeue(&prt->out.packet_list);

    } /* while (tcp_buf) */

} /* TCP_PMTU_Repacket */

/************************************************************************
*
*   FUNCTION
*
*       TCP_PMTU_Increase_SMSS
*
*   DESCRIPTION
*
*       This routine increases the p_smss of the ports in the TCP
*       port list.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TCP_PMTU_Increase_SMSS(VOID)
{
    INT     i;

    for (i = 0; i < TCP_MAX_PORTS; i++)
    {
        if (TCP_Ports[i])
        {
#if (INCLUDE_IPV6 == NU_TRUE)

            if (TCP_Ports[i]->portFlags & TCP_FAMILY_IPV6)
            {
                /* If this is a host route that has had its PMTU updated */
                if ( (TCP_Ports[i]->tp_routev6.rt_route) &&
                     (TCP_Ports[i]->tp_routev6.rt_route->rt_route_node->
                      rt_submask_length == 128) &&
                     (TCP_Ports[i]->tp_routev6.rt_route->rt_pmtu_timestamp == 0) &&
                     (!(TCP_Ports[i]->tp_routev6.rt_route->rt_flags & RT_STOP_PMTU)) )
                {
                    /* If the Path MTU of the route is less than the
                     * other side's MSS, set the send size to the Path
                     * MTU of the route.
                     */
                    if (TCP_Ports[i]->tp_routev6.rt_route->rt_path_mtu <
                        TCP_Ports[i]->sendsize)
                        TCP_Ports[i]->p_smss =
                            (UINT16)(TCP_Ports[i]->tp_routev6.rt_route->rt_path_mtu -
                                    (IP6_HEADER_LEN +
#if (INCLUDE_IPV4 == NU_TRUE)
                                     IP_HEADER_LEN  +
#endif
                                     TCP_HEADER_LEN));
                }
            }

            else
#endif
            {
                /* If this is a host route that has had its PMTU updated */
                if ( (TCP_Ports[i]->tp_route.rt_route) &&
                     (TCP_Ports[i]->tp_route.rt_route->rt_route_node->
                      rt_submask_length == 32) &&
                     (TCP_Ports[i]->tp_route.rt_route->rt_pmtu_timestamp == 0) &&
                     (!(TCP_Ports[i]->tp_route.rt_route->rt_flags & RT_STOP_PMTU)) )
                {
                    /* If the Path MTU of the route is less than the
                     * other side's MSS, set the send size to the Path
                     * MTU of the route.
                     */
                    if (TCP_Ports[i]->tp_route.rt_route->rt_path_mtu <
                        TCP_Ports[i]->sendsize)
                        TCP_Ports[i]->p_smss =
                            (UINT16)(TCP_Ports[i]->tp_route.rt_route->rt_path_mtu -
                            (IP_HEADER_LEN + TCP_HEADER_LEN));
                }
            }

        } /* if (TCP_Ports[i]) */

    } /* for (i = 0; i < TCP_MAX_PORTS; i++) */

} /* TCP_PMTU_Increase_SMSS */

#endif


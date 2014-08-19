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
*       icmp.c
*
*   COMPONENT
*
*       ICMP - Internet Control Message Protocol
*
*   DESCRIPTION
*
*       This file contains the implementation of the ICMP protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       ICMP_Init
*       ICMP_Interpret
*       ICMP_Process_Error
*       ICMP_Process_Datagram_Too_Big
*       ICMP_Echo_Reply
*       ICMP_Reflect
*       ICMP_Send_Error
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if ( (INCLUDE_PMTU_DISCOVERY == NU_TRUE) && (INCLUDE_LITE_ICMP == NU_FALSE) )

STATIC VOID ICMP_Process_Datagram_Too_Big(const NET_BUFFER *);

#endif

extern ICMP_ECHO_LIST   ICMP_Echo_List;

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Init
*
*   DESCRIPTION
*
*       Initialize the ICMP module. In this case is only consists of
*       nulling the ICMP_Echo_List.
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
VOID ICMP_Init(VOID)
{
    /* Null the head and tail pointers of the echo list. */
    ICMP_Echo_List.icmp_head = NU_NULL;
    ICMP_Echo_List.icmp_tail = NU_NULL;

} /* ICMP_Init */

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Interpret
*
*   DESCRIPTION
*
*       Process received ICMP datagrams.
*
*   INPUTS
*
*       *pkt                    A pointer to the start of the IPv4
*                               header.
*       *buf_ptr                A pointer to the net buffer pointer
*       ip_source               The IP source.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       -1                      Failure
*
*************************************************************************/
STATUS ICMP_Interpret(IPLAYER *pkt, NET_BUFFER *buf_ptr, UINT32 ip_source)
{
    ICMP_ECHO_LIST_ENTRY    *echo_entry;
    ICMP_LAYER              *icp;
    INT                     i;
#if (INCLUDE_LITE_ICMP == NU_FALSE)
    UINT32                  src, dst, gateway;
#endif
    STATUS                  status;

    UNUSED_PARAMETER (pkt);

    icp = (ICMP_LAYER *)(buf_ptr->data_ptr);

    /* Increment the total number of ICMP messages received. */
    MIB2_icmpInMsgs_Inc;

    i = GET8(icp, ICMP_TYPE_OFFSET);

    if (GET16(icp, ICMP_CKSUM_OFFSET))
    {        /* ignore if chksum=0 */
        if (TLS_IP_Check_Buffer_Chain(buf_ptr))
        {
            NLOG_Error_Log("Invalid ICMP checksum", NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            /* Increment the number of ICMP messages received with errors. */
            MIB2_icmpInErrors_Inc;

            return (-1);
        } /* end if */

        /* Clear the flag indicating a running sum is held in the parent buffer. */
        buf_ptr->mem_flags &= ~NET_BUF_SUM;

    } /* end if */

#if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE))
    /* Store/Print the ICMP header info */
    NLOG_ICMP_Info(icp, NLOG_RX_PACK);
#endif

    switch (i)
    {
        case ICMP_ECHO:                    /* ping request sent to me */

            /* Increment the number of Echo requests received. */
            MIB2_icmpInEchos_Inc;

            PUT8(icp, ICMP_TYPE_OFFSET, 0);           /* echo reply type */

            /* Remove the buffer from the buffer list. The buffer will be
               reused, it will be sent back as a echo reply. */
            MEM_Buffer_Dequeue(&MEM_Buffer_List);

            if (ICMP_Echo_Reply(buf_ptr) != NU_SUCCESS)
                NLOG_Error_Log("Failed to transmit ICMP Echo Reply", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Increment the number of Echo replies sent. */
            MIB2_icmpOutEchoReps_Inc;

            break;

        case ICMP_ECHOREPLY:                /* ping reply requested by me */

            /* Increment the number of Echo Replies received. */
            MIB2_icmpInEchoReps_Inc;

            /* Search the list looking for a matching ID and seq num. */
            for (echo_entry = ICMP_Echo_List.icmp_head;
                (echo_entry) && (!((GET16(icp, ICMP_ID_OFFSET) == ICMP_ECHO_REQ_ID)
                && (GET16(icp, ICMP_SEQ_OFFSET) == echo_entry->icmp_echo_seq_num)));
                echo_entry = echo_entry->icmp_next)
                {
                    ;
                }

            /* If an entry was found mark the status as success, resume
               the requesting task, and unset the timeout timer. */
            if (echo_entry)
            {
                /* set status as success */
                echo_entry->icmp_echo_status = NU_SUCCESS;

                /* unset the timeout timer. */
                TQ_Timerunset(ICMP_ECHO_TIMEOUT, TQ_CLEAR_ALL_EXTRA,
                              (UNSIGNED)echo_entry->icmp_echo_seq_num, 0);

                /* resume the task */
                status = NU_Resume_Task(echo_entry->icmp_requesting_task);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to resume task", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }
            }

            /* Return the buffer back to the free buffer pool. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;

#if (INCLUDE_LITE_ICMP == NU_FALSE)

        case ICMP_REDIRECT:

            dst = GET32(icp, ICMP_IP_OFFSET + IP_DEST_OFFSET);
            gateway = GET32(icp, ICMP_GWADDR_OFFSET);
            src = ip_source;

            RTAB4_Redirect(dst, gateway, RT_GATEWAY | RT_HOST, src);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            /* Increment the number of Redirect messages received. */
            MIB2_icmpInRedirects_Inc;

            break;
#endif

        case ICMP_UNREACH:

#if ( (INCLUDE_PMTU_DISCOVERY == NU_TRUE) && (INCLUDE_LITE_ICMP == NU_FALSE) )

            if (GET8(icp, ICMP_CODE_OFFSET) == ICMP_UNREACH_NEEDFRAG)
                ICMP_Process_Datagram_Too_Big(buf_ptr);
#endif

        case ICMP_TIMXCEED:

#if (INCLUDE_LITE_ICMP == NU_FALSE)

        case ICMP_SOURCEQUENCH:
        case ICMP_PARAPROB:

#endif

            ICMP_Process_Error(buf_ptr);

            /* Return the buffer back to the free buffer pool. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;

#if (INCLUDE_LITE_ICMP == NU_FALSE)

        case ICMP_TIMESTAMP:

            MIB2_icmpInTimeStamps_Inc;

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;

        case ICMP_TIMESTAMPREPLY:

            MIB2_icmpInTimeStampReps_Inc;

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;

        case ICMP_ADDRMSKREQUEST:

            MIB2_icmpInAddrMasks_Inc;

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;

        case ICMP_ADDRMSKREPLY:

            MIB2_icmpInAddrMaskReps_Inc;

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;
#endif

        default:

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            /* Increment the number of ICMP messages received with errors. */
            MIB2_icmpInErrors_Inc;
            break;

    } /* end switch */

    return (NU_SUCCESS);

} /* ICMP_Interpret() */

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Process_Error
*
*   DESCRIPTION
*
*       This function processes an incoming ICMP error.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the packet.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ICMP_Process_Error(NET_BUFFER *buf_ptr)
{
    INT32                   protocol;
    INT                     hlen;
    ICMP_ECHO_LIST_ENTRY    *echo_entry;
    STATUS                  dbg_status;
    UINT8                   icmp_type, icmp_code;
    INT32                   error;

#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IPV6 == NU_TRUE))
    UINT8                   *dest_addr;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    UINT32  data = 0;
#endif

#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE))
    UINT8   *src_addr;
#endif

    /* Get the error code to process */
    switch (buf_ptr->data_ptr[ICMP_TYPE_OFFSET])
    {
		case ICMP_UNREACH:

			MIB2_icmpInDestUnreachs_Inc;
			break;

        case ICMP_TIMXCEED:

            MIB2_icmpInTimeExcds_Inc;
            break;

#if (INCLUDE_LITE_ICMP == NU_FALSE)

        case ICMP_SOURCEQUENCH:

            MIB2_icmpInSrcQuenchs_Inc;
            break;

        case ICMP_PARAPROB:

#if (INCLUDE_IPV6 == NU_TRUE)

            /* Save off the pointer into the packet where the error
             * occurred.
             */
            data = GET8(buf_ptr->data_ptr, ICMP_PPTR_OFFSET);

#endif
            MIB2_icmpInParmProbs_Inc;
            break;
#endif

        default:

            break;
    }

    icmp_type = buf_ptr->data_ptr[ICMP_TYPE_OFFSET];
    icmp_code = buf_ptr->data_ptr[ICMP_CODE_OFFSET];

    /* Determine the error to report based on the type and code in the
     * packet.
     */
    error = ICMP_MAP_ERROR(icmp_type, icmp_code);

    /* Set the data pointer to the beginning of the IP header contained
     * in the data portion of the ICMP packet.
     */
    buf_ptr->data_ptr += ICMP_DATA_OFFSET;

    /* Extract the source and destination addresses from the packet. */
#if ( (INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) )
    src_addr = (UINT8 *)(&buf_ptr->data_ptr[IP_SRC_OFFSET]);
#endif

#if ( (INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IPV6 == NU_TRUE) )
    dest_addr = (UINT8 *)(&buf_ptr->data_ptr[IP_DEST_OFFSET]);
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
    /* Extract the protocol field from the encapsulated IP header's
     * 8 data bytes if this is an ICMP Destination Unreachable message.
     */
    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE && \
                buf_ptr->data_ptr[IP_PROTOCOL_OFFSET] == IPPROTO_AUTH)
        protocol = buf_ptr->data_ptr[IP_HEADER_LEN];
    else
#endif
    /* Extract the protocol type from the IP header. */
    protocol = buf_ptr->data_ptr[IP_PROTOCOL_OFFSET];

    /* Extract the IP header length. */
    hlen =
        (UINT16)((GET8(buf_ptr->data_ptr,
                       IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* Set the data pointer to the beginning of the protocol specific
     * header.
     */
    buf_ptr->data_ptr += hlen;

    /* If the protocol type is UDP or TCP, process the error */
    switch (protocol)
    {
#if (INCLUDE_UDP == NU_TRUE)

    case IP_UDP_PROT:

        if (UDP_Handle_Datagram_Error(NU_FAMILY_IP, buf_ptr, src_addr,
                                      dest_addr, error) != NU_SUCCESS)
            NLOG_Error_Log("UDP failed to handle datagram error",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        break;

#endif

#if (INCLUDE_TCP == NU_TRUE)

    case IP_TCP_PROT:

        if (TCP_Handle_Datagram_Error(NU_FAMILY_IP, buf_ptr, src_addr,
                                      dest_addr, error) != NU_SUCCESS)
            NLOG_Error_Log("TCP failed to handle datagram error",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        break;

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    case IPPROTO_IPV6:

        /* Decrement the data length */
        buf_ptr->mem_total_data_len -= hlen;
        buf_ptr->data_len -= hlen;

        /* An IPv4 error was received for an IPv6 packet that was
         * encapsulated in an IPv4 packet.
         */
        ICMP6_Process_IPv4_Error(buf_ptr, dest_addr, icmp_type, icmp_code,
                                 data);

        break;

#endif

    case IP_ICMP_PROT:

        /* If the Time Exceeded message is received in response to an
         * ICMP Echo Request sent from this node, wake up the waiting
         * ICMP task and return the error to the application.
         */
        if (GET8(buf_ptr->data_ptr, ICMP_TYPE_OFFSET) == ICMP_ECHO)
        {
            /* Search the list looking for a matching ID and seq num. */
            echo_entry = ICMP_Echo_List.icmp_head;

            while (echo_entry)
            {
                if ( (GET16(buf_ptr->data_ptr, ICMP_ID_OFFSET) == ICMP_ECHO_REQ_ID) &&
                     (GET16(buf_ptr->data_ptr, ICMP_SEQ_OFFSET) ==
                      echo_entry->icmp_echo_seq_num) )
                {
                    /* Stop the timer */
                    TQ_Timerunset(ICMP_ECHO_TIMEOUT, TQ_CLEAR_ALL_EXTRA,
                                  (UNSIGNED)echo_entry->icmp_echo_seq_num, 0);

                    /* Set the status to return to the application layer */
                    echo_entry->icmp_echo_status = (STATUS)error;

                    /* Wake up the task */
                    dbg_status =
                        NU_Resume_Task(echo_entry->icmp_requesting_task);

                    if (dbg_status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to resume task",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);

                        NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    break;
                }

                else
                    echo_entry = echo_entry->icmp_next;
            }
        }

        break;

    default:

#if (INCLUDE_IPSEC == NU_TRUE)

        /* In case of IPsec ESP we cannot find the protocol field from the
         * encapsulated IP header and 8 data bytes (of the offending packet)
         * in the ICMP Destination Unreachable message; therefore, loop
         * through all UDP and TCP ports and adjust the matching ports.
         * Matching criteria is based on source and destination addresses.
         */
        if (UDP_Handle_Datagram_Error(NU_FAMILY_IP, buf_ptr, src_addr,
                                      dest_addr, error) != NU_SUCCESS)
        {
            NLOG_Error_Log("UDP failed to handle datagram error",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        if (TCP_Handle_Datagram_Error(NU_FAMILY_IP, buf_ptr, src_addr,
                                      dest_addr, error) != NU_SUCCESS)
        {
            NLOG_Error_Log("TCP failed to handle datagram error",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
#endif

        break;
    }

} /* ICMP_Process_Error */

#if ( (INCLUDE_PMTU_DISCOVERY == NU_TRUE) && (INCLUDE_LITE_ICMP == NU_FALSE) )

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Process_Datagram_Too_Big
*
*   DESCRIPTION
*
*       A Datagram Too Big message is sent by a router in response to a
*       packet that it cannot forward because the packet is larger than
*       the MTU of the outgoing link.  The information in this message is
*       used as part of the Path MTU Discovery Process.
*
*       An incoming Packet Too Big message must be passed to the
*       upper-layer process.
*
*   INPUTS
*
*       *pkt                    A pointer to the IPv4 header of the
*                               Datagram Too Big packet.
*       *device                 A pointer to the device on which the
*                               packet was received.
*       *buf_ptr                A pointer to the Datagram Too Big packet.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID ICMP_Process_Datagram_Too_Big(const NET_BUFFER *buf_ptr)
{
    UINT16              new_mtu;
    RTAB4_ROUTE_ENTRY   *rt_entry;
    SCK_SOCKADDR_IP     de;

    /* Extract the destination address of the offending packet */
    de.sck_addr = GET32(buf_ptr->data_ptr, (ICMP_DATA_OFFSET + IP_DEST_OFFSET));

    de.sck_family = NU_FAMILY_IP;

    /* If there is an existing route entry for the Source of the packet,
     * process the Packet Too Big message.
     */
    rt_entry = RTAB4_Find_Route(&de, RT_BEST_METRIC);

    /* If a route was found and Path MTU Discovery is not disabled on
     * the route.  This function should never get called for a route
     * that has disabled Path MTU Discovery since the packet will be
     * fragmented at the IP layer.
     */
    if ( (rt_entry) && (!(rt_entry->rt_flags & RT_STOP_PMTU)) )
    {
        /* RFC 1191 - section 6.2 - If a per-host route for this path
         * does not exist, then one is created (almost as if a per-host
         * ICMP Redirect is being processed; the new route uses the same
         * first-hop router as the current route).
         */
        if (rt_entry->rt_route_node->rt_submask_length != 32)
        {
            if (RTAB4_Add_Route(rt_entry->rt_device, de.sck_addr, 0xffffffffUL,
                                rt_entry->rt_gateway_v4.sck_addr,
                                rt_entry->rt_flags) != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to add route", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Free the original network route found */
            RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP);

            /* Get a pointer to the new host route just added */
            rt_entry = RTAB4_Find_Route(&de, RT_HOST_MATCH);
        }

        if (rt_entry)
        {
            /* Extract the PMTU being advertised */
            new_mtu =
                GET16((UINT8*)buf_ptr->data_ptr, ICMP_NEXTMTU_OFFSET);

#if (INCLUDE_IPSEC == NU_TRUE)
            /* Check whether IPsec is enabled for the device which
             * received this packet.
             */
            if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
            {
                /* Adjust IPsec headers overhead */
                new_mtu -= IPSEC_HDRS_OVERHEAD;
            }
#endif

            /* RFC 1191 - section 5 - The simplest thing for a host to
             * do when receiving a Datagram Too Big Message from an
             * unmodified router is to assume that the PMTU is the
             * minimum of its currently-assumed PMTU and 576, and to
             * stop setting the DF bit in datagrams sent on that path.
             */
            if (new_mtu == 0)
            {
                new_mtu = 576;
                rt_entry->rt_flags |= RT_STOP_PMTU;
            }

            /* RFC 1191 - section 3 - A host MUST never reduce its
             * estimate of the Path MTU below 68 octets.
             */
            else if (new_mtu < 68)
            {
                new_mtu = 68;
                rt_entry->rt_flags |= RT_STOP_PMTU;
            }

            /* RFC 1191 - section 3 - A host MUST not increase its
             * estimate of the Path MTU in response to the contents
             * of a Datagram Too Big Message.
             */
            if (new_mtu < rt_entry->rt_path_mtu)
            {
                rt_entry->rt_path_mtu = new_mtu;

                /* Set the time when the next increase is due */
                rt_entry->rt_pmtu_timestamp =
                    NU_Retrieve_Clock() + PMTU_INC_TIME;
            }
        }
    }

} /* ICMP_Process_Datagram_Too_Big */

#endif

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Send_Error
*
*   DESCRIPTION
*
*       Send an ICMP error packet.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the NET buffer
*       type                    ICMP type definition
*       code                    ICMP code definition
*       dest                    Destination address
*       *device                 Pointer to the device entry structure
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID ICMP_Send_Error(const NET_BUFFER *buf_ptr, INT type, INT code,
                     UINT32 dest, DV_DEVICE_ENTRY *device)
{
    NET_BUFFER      *send_buf;
    IPLAYER         *ip_pkt;
    ICMP_LAYER      *icmp_shdr, *icmp_recv_hdr;
    UINT8           temp_buf[sizeof(IPLAYER) + 8];

    /* If the device out which to send the message is NULL, do not
     * attempt to transmit an ICMP error message.
     */
    if (!device)
    {
        NLOG_Error_Log("Invalid interface pointer in ICMP_Send_Error",
                       NERR_SEVERE, __FILE__, __LINE__);

        return;
    }

    /* If the rate-limiting interval for ICMP messages has not been
     * exceeded since the first message during the interval was sent.
     */
    if (TQ_Check_Duetime(device->dev_first_error_msg_timestamp +
                         device->dev_error_msg_rate_limit) != 0)
    {
        /* Only allow ICMP error messages to be transmitted if less than
         * dev_max_error_msg error messages have been transmitted.
         */
        if (device->dev_error_msg_count >= device->dev_max_error_msg)
        {
            NLOG_Error_Log("Too many ICMP error messages have been transmitted",
                           NERR_SEVERE, __FILE__, __LINE__);

            return;
        }
    }

    /* The interval has been exceeded.  Reset the message count and the
     * timestamp of the first message in the interval.
     */
    else
    {
        /* Reset the count. */
        device->dev_error_msg_count = 0;

        /* Restart the timer. */
        device->dev_first_error_msg_timestamp = NU_Retrieve_Clock();
    }

    ip_pkt = (IPLAYER *)buf_ptr->data_ptr;

    /* Do not generate an ICMP error message in response to an ICMP
     * error packet.
     */
    if (GET8(ip_pkt, IP_PROTOCOL_OFFSET) == IP_ICMP_PROT)
    {
        /* Get a pointer to the ICMP header */
        icmp_recv_hdr =
            (ICMP_LAYER *)((buf_ptr->data_ptr +
            ((GET8(ip_pkt, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2)));

        /* RFC 1122 - An ICMP error message MUST NOT be sent as the result of
         * receiving an ICMP error message.
         */
        if (ICMP_IS_ERROR_MESSAGE(GET8(icmp_recv_hdr, ICMP_TYPE_OFFSET)))
            return;
    }

    /* Do not send an error in response to a broadcast packet. */
    if (buf_ptr->mem_flags & (NET_BCAST | NET_MCAST))
        return;

    /* Perform additional verification on the packet.  We want to reject the ICMP
       error for the same reasons that we refuse to forward a packet. */
    if (IP_CANFORWARD(GET32(ip_pkt, IP_DEST_OFFSET)) != NU_SUCCESS)
        return;

    /* Allocate a buffer to build the ICMP packet in. */
    send_buf = (NET_BUFFER *)MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    if (send_buf == NU_NULL)
    {
        NLOG_Error_Log("Unable to acquire MEM buffers from freelist",
                       NERR_SEVERE, __FILE__, __LINE__);

        MIB2_icmpOutErrors_Inc;

        return;
    }

    send_buf->mem_dlist = &MEM_Buffer_Freelist;

    /* Begin filling in ICMP header. */
    /* Fill in the type. */
    icmp_shdr = (ICMP_LAYER *)(send_buf->mem_parent_packet +
                               (IP_HEADER_LEN + device->dev_hdrlen));

    PUT8(icmp_shdr, ICMP_TYPE_OFFSET, (UINT8)type);

#if (INCLUDE_LITE_ICMP == NU_FALSE)

    if (type == ICMP_REDIRECT)
    {
        PUT32(icmp_shdr, ICMP_GWADDR_OFFSET, dest);

        MIB2_icmpOutRedirects_Inc;
    }

    else
#endif
    {
        PUT32(icmp_shdr, ICMP_VOID_OFFSET, 0);

        if (type == ICMP_UNREACH)
        {
            if ( (code == ICMP_UNREACH_NEEDFRAG) && (device) )
                PUT16(icmp_shdr, ICMP_NEXTMTU_OFFSET, (UINT16)device->dev_mtu);

            MIB2_icmpOutDestUnreachs_Inc;
        }

#if (INCLUDE_LITE_ICMP == NU_FALSE)

        else if (type == ICMP_PARAPROB)
        {
            /* Update the icmpInParmProbs parameter. */
            MIB2_icmpInParmProbs_Inc;

            PUT8(icmp_shdr, ICMP_PPTR_OFFSET, (UINT8)code);
            code = 0;

            MIB2_icmpOutParmProbs_Inc;
        }
#endif
    }

    PUT8(icmp_shdr, ICMP_CODE_OFFSET, (UINT8)code);

    /* Copy the old IP packet into the data portion of the ICMP packet. */
    /* It would be simpler and more efficient to copy the data straight from the
       original IP packet to the ICMP packet. However, that will not work on
       DSPs. Hence the two copies below. */
    GET_STRING(ip_pkt, 0, temp_buf, sizeof(IPLAYER) + 8);
    PUT_STRING(icmp_shdr, ICMP_IP_OFFSET, temp_buf, sizeof(IPLAYER) + 8);

    /* Point to where the new IP header should begin. */
    send_buf->data_ptr = (UINT8 *)icmp_shdr - IP_HEADER_LEN;

    /* Copy the IP header. */
    memcpy(send_buf->data_ptr, ip_pkt, sizeof(IPLAYER));

    send_buf->data_ptr = (UINT8 *)icmp_shdr;
    send_buf->mem_buf_device = device;

    /* The size of the data so far is the sizeof(ICMP_LAYER) + 8. At first
       it might seem that 20 bytes for the IP header that is part of the
       ICMP data needs to added as well. However, that 20 bytes is included
       in ICMP_LAYER. */
    send_buf->mem_total_data_len = sizeof(ICMP_LAYER) + 8;
    send_buf->data_len = sizeof(ICMP_LAYER) + 8;

    ICMP_Reflect(send_buf);

    /* Increment the number of ICMP error messages transmitted. */
    device->dev_error_msg_count ++;

} /* ICMP_Send_Error */

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Reflect
*
*   DESCRIPTION
*
*       Send an ICMP echo response.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the net buffer
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID ICMP_Reflect(NET_BUFFER *buf_ptr)
{
    IPLAYER             *ip_pkt;
    UINT32              t, icmpsrc = 0;
    DV_DEVICE_ENTRY     *temp_dev;
    ICMP_LAYER          *icmp_l;
    STATUS              stat;
    DEV_IF_ADDR_ENTRY   *source_addr;
    UINT8               tos = 0;

    /* Point to the IP packet.  */
    ip_pkt = (IPLAYER *)(buf_ptr->data_ptr - IP_HEADER_LEN);

    /* Keep a copy of the original destination. */
    t = GET32(ip_pkt, IP_DEST_OFFSET);

    tos = GET8(ip_pkt, IP_SERVICE_OFFSET);

    /* Make the source the new destination. */
    PUT32(ip_pkt, IP_DEST_OFFSET, GET32(ip_pkt, IP_SRC_OFFSET));

    /* Set the new source address.  If an exact match cannot be found then use
       the source address for the receiving interface. */
    for (temp_dev = DEV_Table.dv_head;
         temp_dev;
         temp_dev = temp_dev->dev_next)
    {
        /* Is there an exact match on the IP address. */
        source_addr = DEV_Find_Target_Address(temp_dev, t);

        if (source_addr)
        {
            icmpsrc = source_addr->dev_entry_ip_addr;
            break;
        }
    }

    /* If a match was not found, use an IP address on the receiving
     * device.
     */
    if (!temp_dev)
    {
        /* If there is at least one IP address on this interface. */
        if (buf_ptr->mem_buf_device->dev_addr.dev_addr_list.dv_head)
        {
            icmpsrc =
                buf_ptr->mem_buf_device->dev_addr.dev_addr_list.
                dv_head->dev_entry_ip_addr;
        }

        /* Otherwise, free the buffers and return. */
        else
        {
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);

            NLOG_Error_Log("Failed to find interface to transmit data",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Compute the ICMP checksum. */
    icmp_l = (ICMP_LAYER *)(buf_ptr->data_ptr);

    PUT16(icmp_l, ICMP_CKSUM_OFFSET, 0);
    PUT16(icmp_l, ICMP_CKSUM_OFFSET, TLS_IP_Check_Buffer_Chain(buf_ptr));

    /* Set the data pointer to the start of the IP header */
    buf_ptr->data_ptr -= IP_HEADER_LEN;

    /* Indicate to IP that it does not need to build a new header */
    buf_ptr->mem_flags |= NET_NOHDR;

#if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE))
    /* Store/Print the ICMP header info */
    NLOG_ICMP_Info(icmp_l, NLOG_TX_PACK);
#endif

    /* Send this packet. */
    stat = IP_Send(buf_ptr, NU_NULL, GET32(ip_pkt, IP_DEST_OFFSET),
                   icmpsrc, IP_MAY_FRAGMENT, 0, IP_ICMP_PROT, tos, NU_NULL);

    if (stat != NU_SUCCESS)
    {
        /* The packet was not sent.  Deallocate the buffer.  If the packet was
           transmitted it will be deallocated when the transmit complete
           interrupt occurs. */
        MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
    }

    /* Increment the number of ICMP messages sent. */
    MIB2_icmpOutMsgs_Inc;

} /* ICMP_Reflect */

/****************************************************************************
*
*   FUNCTION
*
*       ICMP_Echo_Reply
*
*   DESCRIPTION
*
*       Send out an ICMP packet, probably in response to a ping operation
*       interchanges the source and destination addresses of the packet,
*       puts in my addresses for the source and sends it
*
*       Does not change any of the ICMP fields, just the IP and layers
*       returns 0 on okay send, nonzero on error
*
*   INPUTS
*
*       *buf_ptr                A pointer to the net buffer structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Reply sent
*       -1                      Reply not sent.
*
******************************************************************************/
STATUS ICMP_Echo_Reply (NET_BUFFER *buf_ptr)
{
    /* Set the deallocation list. */
    buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

    /* Do not send an error in response to a broadcast packet. */
    if (buf_ptr->mem_flags & (NET_BCAST | NET_MCAST))
    {
        MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
        return(-1);
    }

    ICMP_Reflect(buf_ptr);

    return (NU_SUCCESS);

} /* ICMP_Echo_Reply */

#endif

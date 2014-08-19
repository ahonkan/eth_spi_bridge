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
*       ip4.c
*
*   DESCRIPTION
*
*       This file contains the common implementation of the IP component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP_Broadcast_Addr
*       IP_Find_Route
*       IP_Initialize
*       IP_Interpret
*       IP_Send
*       IP_Localaddr
*
*   DEPENDENCIES
*
*       nu_net.h
*       tcp4.h
*       udp4.h
*       ipraw4.h
*       nat_extr.h
*       gre.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#include "networking/tcp4.h"
#include "networking/udp4.h"
#include "networking/ipraw4.h"
#endif

#if (INCLUDE_GRE == NU_TRUE)
#include "networking/gre.h"
#endif

#if (INCLUDE_NAT == NU_TRUE)
#include "networking/nat_extr.h"
#endif

/* Include the Nucleus IPsec specific files. */
#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_externs.h"
#include "networking/ips_api.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_IPSEC == NU_TRUE)
/* Determine if IPSec is at version 1.3 or greater */
#ifndef IPSEC_1_3
#ifdef  IPSEC_ALREADY_RUNNING
#define IPSEC_1_3            1
#endif /* IPSEC_ALREADY_RUNNING */
#endif /* ndef IPSEC_1_3 */
#endif /* (INCLUDE_IPSEC == NU_TRUE) */

/*  This is the id field of outgoing IP packets. */
extern INT16         IP_Ident;

#if (INCLUDE_NAT == NU_TRUE)
extern INT           IP_NAT_Initialize;
#endif

STATIC INT IP_Broadcast_Addr(UINT32, const DV_DEVICE_ENTRY *);

/*************************************************************************
*
*   FUNCTION
*
*       IP_Interpret
*
*   DESCRIPTION
*
*       Called by the packet demuxer to interpret a new ip packet.  Checks
*       the validity of the packet (checksum, flags), if IPsec Headers are
*       found security is applied and then passes it on to the appropriate
*       protocol handler.
*
*   INPUTS
*
*       *pkt                    A pointer to the IP packet
*       *device                 A pointer to the device entry structure
*       *buf_ptr                A pointer to the net buffer structure
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       1                       Packet dropped.
*
*************************************************************************/
STATUS IP_Interpret(IPLAYER *pkt, DV_DEVICE_ENTRY *device,
                    NET_BUFFER *buf_ptr)
{
    UINT16              hlen;
    UINT32              total;
    UINT32              ip_dest;
    DV_DEVICE_ENTRY     *temp_dev = NU_NULL;
    NET_BUFFER          *buf;
    DEV_IF_ADDR_ENTRY   *temp_addr_entry;

#if (INCLUDE_TCP == NU_TRUE || INCLUDE_UDP == NU_TRUE)
    struct pseudotcp    tcp_chk;
#endif

#if (INCLUDE_NAT == NU_TRUE || INCLUDE_UDP == NU_TRUE || INCLUDE_IPSEC == NU_TRUE)
    STATUS              status;
#endif

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    IP_MULTI            *ipm;
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
    UINT8               next_header;
    UINT8               ipsec_reassembly_flag = NU_FALSE;

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    /* Flag used for updating Inbound SA's while forwarding a
     * a secure tunneled packet.
     */
    static STATUS       update_inbound_sa = NU_TRUE;

    /* SA index. */
    IPSEC_INBOUND_INDEX sa_index;
#endif
#endif

    /* Increment SNMP ipInReceives.  This value counts all packets
     * received.  Even those that have errors. */
    MIB2_ipInReceives_Inc;

#if ((INCLUDE_DHCP != NU_TRUE) && (INCLUDE_BOOTP != NU_TRUE))
    /* If neither DHCP or BOOTP will be used to perform IP address
     * discovery then drop all packets until an IP address is attached
     * to the device.
     */
    if (!(device->dev_flags & DV_UP))
    {
        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        MIB2_ipInDiscards_Inc;

        return (NU_SUCCESS);
    }
#endif /* ((INCLUDE_DHCP == NU_TRUE) || (INCLUDE_BOOTP == NU_TRUE)) */

    /* Extract total length of packet. */
    buf_ptr->mem_total_data_len = GET16(pkt, IP_TLEN_OFFSET);

    /* Check to see if the total data length is less than the sum of the
     * data lengths the buffers in the chain. This at first sounds
     * impossible. However data_len comes from the size reported by the
     * driver. It is not unusual to receive a packet that has been padded.
     * The driver does not distinguish between real data and padded data.
     * However, the IP header contains the true data length. We want to
     * use the smaller value.
     */
    for (buf = buf_ptr, total = buf->mem_total_data_len;
         buf;
         buf = buf->next_buffer)
    {
        if (buf->data_len > total)
            buf->data_len = total;

        total -= buf->data_len;
    }

    hlen = (UINT16)((GET8(pkt, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* If the header is too small or inconsistent, discard the packet. */
    if ( (hlen < IP_HEADER_LEN) || (buf_ptr->mem_total_data_len < hlen) )
    {
        NLOG_Error_Log("Error in the size of the header length",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        /* Increment the number of IP packets received with header errors. */
        MIB2_ipInHdrErrors_Inc;

        return (1);                /* drop packet */
    } /* end if */

    /* If the IP header will not fit in the first buffer, drop this
     * packet and log an error.
     */
    if (buf_ptr->data_len < (UINT32)hlen)
    {
        NLOG_Error_Log("All headers do not fit in the first buffer",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        /* Increment the number of IP packets received with header errors. */
        MIB2_ipInHdrErrors_Inc;

        return (1);                /* drop packet */
    } /* end if */

#if (HARDWARE_OFFLOAD == NU_TRUE)
    /* Bypass software checksum calculation if the HW has already done it */
    if (!(buf_ptr->hw_options & HW_RX_IP4_CHKSUM))
#endif
    {
        /* checksum verification of IP header */
        if (TLS_IP_Check((VOID *)pkt, (UINT16)(hlen >> 1)))
        {
            NLOG_Error_Log("Error in the checksum of the incoming IP header",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            /* Increment the number of IP packets received with header errors. */
            MIB2_ipInHdrErrors_Inc;

            return (1);              /* drop packet */
        }
    }

    /* silently toss this legal-but-useless packet */
    if (buf_ptr->mem_total_data_len == hlen)
    {
        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        /* Increment the number of IP packets received with header errors. */
        MIB2_ipInHdrErrors_Inc;

        return (1);
    }

    /* Extract the destination address from the IP header of the packet */
    ip_dest = GET32(pkt, IP_DEST_OFFSET);

    /* Is there an exact match on the IP address. */
    if (DEV_Find_Target_Address(device, ip_dest) != NU_NULL)
        temp_dev = device;

    else
    {
        /* If the device supports broadcast packets check for a match
         * on supported broadcast addresses.
         */
        if (device->dev_flags & DV_BROADCAST)
        {
            /* If the destination is either of the broadcast addresses,
             * then keep it.
             */
            if ( (ip_dest == 0) || (ip_dest == IP_ADDR_BROADCAST) )
                temp_dev = device;

            else
            {
                /* Examine each address in the list of addresses for a matching
                 * broadcast address.
                 */
                for (temp_addr_entry = device->dev_addr.dev_addr_list.dv_head;
                     temp_addr_entry;
                     temp_addr_entry = temp_addr_entry->dev_entry_next)
                {
                    /* Is this a broadcast for our network. */
                    if ( (temp_addr_entry->dev_entry_net == ip_dest) ||
                         (temp_addr_entry->dev_entry_net_brdcast == ip_dest) )
                    {
                        temp_dev = device;
                        break;
                    }
                }
            }
        }

        /* If a matching address was not found on the interface which received
         * the packet, check the other interfaces on the node.
         */
        if (!temp_dev)
        {
            /* The following loop checks to see if the packet is for us.  A match
             * occurs if the destination IP matches one of our IP addresses or if
             * it is a broadcast address on a device that supports broadcasting.
             */
            for (temp_dev = DEV_Table.dv_head;
                 temp_dev;
                 temp_dev = temp_dev->dev_next)
            {
                /* Do not check the receiving interface again */
                if (temp_dev != device)
                {
                    /* Is there an exact match on the IP address. */
                    if (DEV_Find_Target_Address(temp_dev, ip_dest))
                        break;
                }
            }
        }
    }

    /* If this is a NAT Router, call the NAT routine */
#if (INCLUDE_NAT == NU_TRUE)
    if (IP_NAT_Initialize)
    {
        status = NAT_Translate(device, &pkt, &buf_ptr);

        if (status == NU_SUCCESS)
            return (status);

        else if (status != NAT_NO_NAT)
        {
            /* Drop it. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
            return (1);
        }
    }
#endif

#if ((INCLUDE_DHCP == NU_TRUE) || (INCLUDE_BOOTP == NU_TRUE))
    /* Before this packet is rejected check to see if the device has an
     * IP_address.  If not then pass it to UDP.  It could be a DHCP
     * response packet.
     */
    if (device->dev_flags & DV_ADDR_CFG)
    {
        /* If this is a UDP packet, make sure it is destined for the
         * BOOTP or DHCP client port (both are 68, so check only for one).
         */
        if ( (GET8(pkt, IP_PROTOCOL_OFFSET) == IP_UDP_PROT) &&
             (GET16(buf_ptr->data_ptr, hlen + UDP_DEST_OFFSET) == IPPORT_DHCPC) )
        {
            temp_dev = device;
        }

        /* Otherwise, free the buffer and return.  The interface is not
         * ready to accept packets yet.
         */
        else
        {
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
            return (1);
        }
    }
#endif /* ((INCLUDE_DHCP == NU_TRUE) || (INCLUDE_BOOTP == NU_TRUE)) */

    /* If the destination IP address did not match any of ours, then
     * check to see if it is destined for a multicast address.
     */
    if (!temp_dev)
    {
        if (IP_MULTICAST_ADDR(ip_dest))
        {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
            /* NOTE: If/when multicast routing support is added it will
             * need to be handled here. */

            /* Do we belong to the multicast group on the receive device. */
            ipm = IP_Lookup_Multi(ip_dest, &device->dev_addr);

            if (ipm != NU_NULL)
                temp_dev = device;
#else
            /* Multicasting support was not desired so drop the packet by
               placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            MIB2_ipInAddrErrors_Inc;

            return (1);

#endif /* INCLUDE_IP_MULTICASTING */
        }
    }

#if (INCLUDE_IPSEC == NU_TRUE)
    /* Check whether IPsec is enabled for the device which received this
     * packet.
     */
    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
    {
        /* Extract the next header field from the IPv4 header. */
        next_header = (INT16)(GET8(pkt, IP_PROTOCOL_OFFSET));

        /* Create the IPsec packet selector for this packet. This will be
         * used throughout the life of this packet for IPsec processing.
         */
        IP_IPSEC_Pkt_Selector(&IPSEC_Pkt_Selector, pkt, buf_ptr,
                              next_header, IPSEC_IPV4, hlen);
    }
#endif

    if (!temp_dev)
    {
#if (INCLUDE_IP_FORWARDING == NU_TRUE)
        /* This packet is not for us. Attempt to forward the packet if
           possible. */

        /* Remove the buffer that we have been processing from the
         * buffer_list. The IP forwarding function will handle the
         * deallocation.
         */
        buf_ptr = (NET_BUFFER *)MEM_Buffer_Dequeue(&MEM_Buffer_List);

        /* Initialize the deallocation list pointer. */
        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

        if (IP_Forwarding)
        {
#if (INCLUDE_IPSEC == NU_TRUE)

            /* Check whether IPsec is enabled for the device which
             * received this packet.
             */
            if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
            {
                /* Check if we need to look up an SA. */
                if (update_inbound_sa == NU_TRUE)
                {
                    /* Build the inbound SA index. */
                    IP_ADDR_COPY(sa_index.ipsec_dest,
                                 (((UINT8 *)(pkt)) + IP_DEST_OFFSET));

                    sa_index.ipsec_dest_type =
                        (IPSEC_IPV4 | IPSEC_SINGLE_IP);

                    sa_index.ipsec_protocol =
                        GET8(pkt, IP_PROTOCOL_OFFSET);

                    /* Get the respective SPI value. */
                    if (sa_index.ipsec_protocol == IPSEC_AH)
                    {
                        /* If AH is present. */
                        sa_index.ipsec_spi =
                            GET32(pkt, (IP_HEADER_LEN + IPSEC_AH_SPI_OFFSET));
                    }

                    else
                    {
                        /* If IPsec ESP header is present. */
                        sa_index.ipsec_spi =
                            GET32(pkt, (IP_HEADER_LEN + IPSEC_ESP_SPI_OFFSET));
                    }

                    /* Find a matching SA in the inbound SA database. */
                    status = IPSEC_Get_Inbound_SA(device->dev_physical->
                                                  dev_phy_ips_group,
                                                  &sa_index,
                                                  &IPSEC_In_Bundle[IPSEC_SA_Count]);

                    /* Check the status value. */
                    if (status == NU_SUCCESS)
                        IPSEC_SA_Count = 1;
                }

                else
                    IPSEC_SA_Count = 1;

                /* Before forwarding the packet: It is possible that this
                 * packet was received in IPsec Tunnel Mode and we have
                 * already decoded the outer header. Check the IPsec policy
                 * to ensure that the correct SAs have been applied. Even if
                 * no IPsec protocol was applied, still verify that this
                 * packet should be allowed to pass.
                 */
                status =
                    IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                          &IPSEC_Pkt_Selector, IPSEC_In_Bundle,
                                          IPSEC_SA_Count, NU_NULL);

                /* If while checking the policy an error occurred, discard the
                 * packet and log an error.
                 */
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Packet discarded IPsec policy check failed ",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Place it back on the Free List. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                    /* Return. */
                    return (status);
                }
            }

#endif

            if (IP_Forward(buf_ptr) != NU_SUCCESS)
                NLOG_Error_Log("Failed to forward packet", NERR_RECOVERABLE,
                               __FILE__, __LINE__);

#if (INCLUDE_IPSEC == NU_TRUE)
            else
            {
                /* The interface which received the packet is the
                 * IPsec tunnel endpoint. The IPsec packet was decoded
                 * already in the previous call. After decoding, the
                 * encapsulated packet needs to be forwarded out another
                 * interface. In this case, do not update Inbound SAs
                 * again for the IPsec policy check.  Therefore, this
                 * flag is set to NU_FALSE before calling IP_Interpret
                 * recursively
                 */

                /* Since the packet is forwarded, set the flag to NU_TRUE. */
                if (update_inbound_sa == NU_FALSE)
                    update_inbound_sa = NU_TRUE;
            }
#endif
        }

        else
        {
            /* Increment the number of IP packets received with the wrong
             * IP addr.*/
            MIB2_ipInAddrErrors_Inc;

            /* Drop it. */
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
        }

#else
        /* Increment the number of IP packets received with the wrong
         * IP addr.*/
        MIB2_ipInAddrErrors_Inc;

        /* Drop it. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
#endif

        return (1);
    }

#if (INCLUDE_IP_REASSEMBLY == NU_TRUE)

    /* If offset or IP_MF are set then this is a fragment and we must
       reassemble. */
    if (GET16(pkt, IP_FRAGS_OFFSET) & ~IP_DF)
    {
        if (TEST_IP_Reassembly_Input(&pkt, &buf_ptr, hlen) != NU_SUCCESS)
        {
            return (1);
        }

        if (IP_Reassemble_Packet(&pkt, &buf_ptr, hlen) != NU_SUCCESS)
            return (1);

#if (INCLUDE_IPSEC == NU_TRUE)
        /* No need to update the IP header yet as IPsec processing needs
         * to be applied.
         */

        /* Check whether IPsec is enabled for the device which received
         * this packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Mark the reassembly flag as true. */
            ipsec_reassembly_flag = NU_TRUE;

#ifdef IPSEC_1_3

            /* Packet has been reassembled. Update the selector now. This
             * is required for protocols like UDP where fragments do not
             * have transport layer protocol headers. This can also occur
             * if TCP packet is forwarded from a link with higher MTU to
             * a link with  smaller MTU. Since the packet has been
             * reassembled and all headers are in proper place now, update
             * the IPsec selector.
             */
            IP_IPSEC_Pkt_Selector_Update_Ports(&IPSEC_Pkt_Selector,
                                               buf_ptr, next_header, hlen);

#endif /* def IPSEC_1_3 */

        }
#else

        /* Get the header length. */
        hlen = (UINT16)((GET8(pkt, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

        /* Adjust the IP length to not reflect the header. */
        PUT16(pkt, IP_TLEN_OFFSET,
                (UINT16)(GET16(pkt, IP_TLEN_OFFSET) - hlen));
#endif

        /* Strip off the IP header. */
        buf_ptr->mem_total_data_len -= hlen;
        buf_ptr->data_len           -= hlen;
        buf_ptr->data_ptr           += hlen;
    }
#else

    /* If the processing of fragments is not desired, then drop this
     * packet.
     */
    /* If offset and IP_MF are set then this is a fragment. */
    if (GET16(pkt, IP_FRAGS_OFFSET) & ~IP_DF)
    {
        /* Increment the number of IP fragments that have been received,
         * even though we don't process them.
         */
        MIB2_ipReasmReqds_Inc;

        /* Drop the current buffer. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        MIB2_ipInDiscards_Inc;

        return (1);
    }
#endif

    /* This is not a fragment. */
    else
    {
        /* If the buffer includes the running total of the data to
         * be used for the checksum, remove the bytes for the IP header
         */
        if (buf_ptr->mem_flags & NET_BUF_SUM)
        {
            buf_ptr->chk_sum -= TLS_Header_Memsum(buf_ptr->data_ptr, hlen);
        }

        /* Strip off the IP header. */
        buf_ptr->mem_total_data_len -= hlen;
        buf_ptr->data_len           -= hlen;
        buf_ptr->data_ptr           += hlen;
    }

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Check whether IPsec is enabled for the device which received this
     * packet.
     */
    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
    {
        /* Do IPsec processing. */
        status = IP_IPSEC_Interpret(device, buf_ptr, (VOID **)&pkt,
                                    &next_header, &hlen, IPSEC_IPV4);

        /* Stop processing if the IPsec checks were unsuccessful. */
        if (status != NU_SUCCESS)
        {
            /* Place the packet back on the Free List. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            return (status);
        }

#if (INCLUDE_IP_REASSEMBLY == NU_TRUE)

        /* If offset or IP_MF have been set, update the IP header. */
        if (ipsec_reassembly_flag == NU_TRUE)
        {
            /* Adjust the IP length to not reflect the header. */
            PUT16(pkt, IP_TLEN_OFFSET,
                  (UINT16)(GET16(pkt, IP_TLEN_OFFSET) - hlen));
        }
#endif
    }
#endif

#if (INCLUDE_TCP == NU_TRUE || INCLUDE_UDP == NU_TRUE)
    /* Create the pseudo tcp header for upper layer protocols to
     * compute their checksum */
    /* The GET32 macro automatically puts the addresses in the native
     * architectures byte order.  In this case we wish to keep the
     * addresses in big endian or network byte order. So swap the values
     * returned by GET32.
     */
    tcp_chk.source  = LONGSWAP(GET32(pkt, IP_SRC_OFFSET));
    tcp_chk.dest    = LONGSWAP(ip_dest);
    tcp_chk.z       = 0;
    tcp_chk.proto   = GET8(pkt, IP_PROTOCOL_OFFSET);
    tcp_chk.tcplen  = (UINT16)INTSWAP((UINT16)buf_ptr->mem_total_data_len);
#endif

#if ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) ||(PRINT_IP_MSG == NU_TRUE))
    /* Print the IP header info */
    NLOG_IP_Info(pkt, NLOG_RX_PACK);
#endif

    /* which protocol to handle this packet? */
    switch (GET8(pkt, IP_PROTOCOL_OFFSET))
    {

#if (INCLUDE_UDP == NU_TRUE)

    case IP_UDP_PROT:

        /* Increment the number of IP packets delivered. */
        MIB2_ipInDelivers_Inc;

        status = UDP4_Input(pkt, buf_ptr, &tcp_chk);

        /* If a matching port could not be found, return an ICMP Port
         * Unreachable error to the source.
         */
        if (status == NU_DEST_UNREACH_PORT)
        {
            buf_ptr->mem_total_data_len += hlen;
            buf_ptr->data_len           += hlen;
            buf_ptr->data_ptr           -= hlen;

            ICMP_Send_Error(buf_ptr, ICMP_UNREACH, ICMP_UNREACH_PORT,
                            tcp_chk.source, device);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        }

        return (status);
#endif

#if (INCLUDE_TCP == NU_TRUE)

    case IP_TCP_PROT:

        /* Increment the number of IP packets successfully delivered. */
        MIB2_ipInDelivers_Inc;

        /* pass tcplen on to TCP */
        return (TCP4_Input(buf_ptr, &tcp_chk));

#endif

#if (INCLUDE_GRE == NU_TRUE)
    case IP_GRE_PROT:

        /* Increment the number of IP packets successfully delivered. */
        MIB2_ipInDelivers_Inc;

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check if IPsec is enabled on this interface */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Check the packet selector and the applied security
             * associations with the IPsec policy.
             */
            status =
                IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                      &IPSEC_Pkt_Selector, IPSEC_In_Bundle,
                                      IPSEC_SA_Count, NU_NULL);

            /* If while checking the policy an error occurred, discard the
             * packet and log an error.
             */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Packet discarded IPsec policy check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Place the packet back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Return. */
                return (status);
            }
        }
#endif

        return (GRE_Interpret(buf_ptr, GET32(pkt, IP_SRC_OFFSET)));
#endif

    case IP_ICMP_PROT:

        /* Increment the number of IP packets successfully delivered. */
        MIB2_ipInDelivers_Inc;

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check if IPsec is enabled on this interface */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Check the packet selector and the applied security
             * associations with the IPsec policy.
             */
            status =
                IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                      &IPSEC_Pkt_Selector, IPSEC_In_Bundle,
                                      IPSEC_SA_Count, NU_NULL);

            /* If while checking the policy an error occurred, discard the
             * packet and log an error.
             */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Packet discarded IPsec policy check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Place the packet back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Return. */
                return (status);
            }
        }
#endif

        return (ICMP_Interpret(pkt, buf_ptr, GET32(pkt, IP_SRC_OFFSET)));

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    case IP_IGMP_PROT :

        /* Increment the number of IP packets successfully delivered. */
        MIB2_ipInDelivers_Inc;

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check whether IPsec is enabled for the device which received this
         * packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Check the packet selector and the applied security associations
             * with the IPsec policy.
             */
            status =
                IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                      &IPSEC_Pkt_Selector, IPSEC_In_Bundle,
                                      IPSEC_SA_Count, NU_NULL);

            /* If while checking the policy an error occurred, discard the
             * packet and log an error.
             */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Packet discarded IPsec policy check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Place the packet back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Return. */
                return (status);
            }
        }
#endif

        return (IGMP_Interpret(buf_ptr, device));
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

    case IP_RAW_PROT :
    case IP_HELLO_PROT :
    case IP_OSPF_PROT :

    /** Any additional Raw IP protocols that are supported in the
        future must have a case here ***/

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check whether IPsec is enabled for the device which received this
         * packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Check the packet selector and the applied security associations
             * with the IPsec policy.
             */
            status =
                IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                      &IPSEC_Pkt_Selector, IPSEC_In_Bundle,
                                      IPSEC_SA_Count, NU_NULL);

            /* If while checking the policy an error occurred, discard the
             * packet and log an error.
             */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Packet discarded IPsec policy check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Place the packet back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                /* Return. */
                return (status);
            }
        }
#endif

        return (IPRaw4_Interpret(pkt, (NET_BUFFER *)buf_ptr));

#endif

#if (INCLUDE_IPSEC == NU_TRUE)

    case IP_VERSION:

#if (INCLUDE_IP_FORWARDING == NU_TRUE)

        /* Tunnel mode. Don't update Inbound SAs for IPsec policy check
         * when forwarding encapsulated IP packets.
         */
        update_inbound_sa = NU_FALSE;
#endif

        /* Check whether IPsec is enabled for the device which received this
         * packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* This is an IPv4 packet encapsulated in an IPv4 packet.
             * Interpret the inner packet.
             */
            return (IP_Interpret((IPLAYER*)buf_ptr->data_ptr, device, buf_ptr));
        }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    case IPPROTO_IPV6:

        /* This is an IPv6 packet encapsulated in an IPv4 packet.  Pass the
         * packet to the IPv6 Interpret routine.
         */
        return (IP6_Interpret((IP6LAYER*)buf_ptr->data_ptr, device,
                                buf_ptr));

#endif

        default:
        NLOG_Error_Log("Received packet of unknown protocol type",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        /* Increment the number of IP packets received with a invalid
           protocol field. */
        MIB2_ipInUnknownProtos_Inc;

        return (1);
    }  /* end switch */

} /* IP_Interpret */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Send
*
*   DESCRIPTION
*
*       Send an IP packet.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer structure
*       *ro                     Pointer to the RTAB route information
*       dest_ip                 Destination IP address
*       src_ip                  Source IP address
*       flags                   Flags set for the IP
*       ttl                     Time To Live for this IP
*       protocol                Protocol for the IP
*       tos                     Type of service
*       *mopt                   A pointer to multicasting options
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_HOST_UNREACHABLE     The HOST cannot be accessed
*       NU_ACCESS               Access denied
*       NU_MSGSIZE              The size of the message is wrong
*
*************************************************************************/
STATUS IP_Send(NET_BUFFER *buf_ptr, RTAB_ROUTE *ro, UINT32 dest_ip,
               UINT32 src_ip, INT32 flags, INT ttl, INT protocol, INT tos,
               const IP_MULTI_OPTIONS *mopt)
{
    IPLAYER         *ip_dgram;
    UINT32          hlen = IP_HEADER_LEN;
    DV_DEVICE_ENTRY *int_face;
    RTAB_ROUTE      iproute;
    SCK_SOCKADDR_IP *dest;
    STATUS          status = NU_SUCCESS;
    NET_BUFFER      *hdr_buf, *temp_buf;

#if (INCLUDE_IP_MULTICASTING == NU_FALSE)
    UNUSED_PARAMETER(mopt);
#else
    MULTI_SCK_OPTIONS   *sck_opts = (MULTI_SCK_OPTIONS *)mopt;
#endif

    /* Routing must be performed first to determine the length of the
     * link-layer header of the outgoing interface.
     */

    /* If a route was not provided then point to the temporary
       route structure. */
    if (ro == NU_NULL)
    {
        ro = &iproute;
        UTL_Zero((CHAR *)ro, sizeof(*ro));
    }

    /* Point to the destination. */
    dest = &ro->rt_ip_dest;

    /* Check to see if there is a cached route.  If so verify that it is to the
       same destination and that it is still up. If not free it and try again.
    */
    if ( (ro->rt_route) &&
         ((((ro->rt_route->rt_entry_parms.rt_parm_flags & RT_UP) == 0) ||
           (dest->sck_addr != dest_ip)) ||
          ((!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_RUNNING)) &&
           (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_UP)))))
    {
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);
        ro->rt_route = NU_NULL;
    }

    if (ro->rt_route == NU_NULL)
    {
        dest->sck_family = SK_FAM_IP;
        dest->sck_len = sizeof(*dest);
        dest->sck_addr = dest_ip;
    }

    /* NOTE:  Bypassing routing is not necessary yet, but may be supported in
       future releases. */
#ifdef NOT_SUPPORTED
    /* Check to see if the caller specified that routing should be bypassed. */
    if (flags & IP_ROUTETOIF)
    {

    }
    else
#endif /* NOT_SUPPORTED */
    {
        /* If a valid route was not provided, find a route. */
        if (ro->rt_route == NU_NULL)
            IP_Find_Route(ro);

        /* If a route still could not be found, return an error */
        if (ro->rt_route != NU_NULL)
        {
            int_face = ro->rt_route->rt_entry_parms.rt_parm_device;

            ro->rt_route->rt_entry_parms.rt_parm_use++;

            /* If the next hop is a gateway then set the destination ip address to
               the gateway. */
            if (ro->rt_route->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
                dest = &ro->rt_route->rt_gateway_v4;
        }
        else
        {
            /* Return host unreachable error.  The only resource allocated in
               this function is a route, but we failed to find the route so it
               is safe to return here.
            */

            /* Increment the number of packets that could not be delivered
               because a route could not be found. */
            MIB2_ipOutNoRoutes_Inc;

            return (NU_HOST_UNREACHABLE);
        }
    }

    /* If this packet is destined for a multicast address and a multicast
     * option specifying the interface to use was included, use that interface
     * instead of the interface associated with the route.
     */
    if (IP_MULTICAST_ADDR(dest_ip))
    {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* Flag this buffer as containing a multicast packet. */
        buf_ptr->mem_flags |= NET_MCAST;

        /* IP destination address is multicast. Make sure "dest" still points
         * to the address in "ro". It may have been changed to point to a
         * gateway address above
         */
        dest = &ro->rt_ip_dest;

        /* Did the caller provide an interface option */
        if ( (sck_opts != NU_NULL) && (sck_opts->multio_device != NU_NULL) )
            int_face = sck_opts->multio_device;

        /* Confirm that the outgoing interface supports multicast. */
        if ((int_face->dev_flags & DV_MULTICAST) == 0)
        {
            MIB2_ipOutNoRoutes_Inc;

            /* Free the route entry */
            if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && ro->rt_route)
                RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);

            return (NU_HOST_UNREACHABLE);
        }

        /* NOTE: When multicastLoop Back and/or multicast routing are supported
         * this is where it should be done.
         */
#else /* !INCLUDE_IP_MULTICASTING */

        /* If multicasting support was not desired then return an error. */
        MIB2_ipOutNoRoutes_Inc;

        /* Free the route entry */
        if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && ro->rt_route )
            RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);

        return (NU_HOST_UNREACHABLE);

#endif /* INCLUDE_IP_MULTICASTING */
    }

#if (INCLUDE_IP_RAW == NU_TRUE)
    /* If this is a Raw IP Packet, don't modify the header */
    if (flags & IP_RAWOUTPUT)
    {
        /* Get a pointer to the user created IP header. */
        ip_dgram = (IPLAYER*)buf_ptr->data_ptr;

        /* Since the user created the IP header, check the ID field
         * of the header. If the application set this field to zero,
         * generate the ID here.
         */
        if ((GET16(ip_dgram, IP_IDENT_OFFSET)) == 0)
            PUT16(ip_dgram, IP_IDENT_OFFSET, (UINT16)IP_Ident++);

        /* Set the header length to zero */
        hlen = 0;

        /* Increment the number of IP packets transmitted. NOTE: this does
           not include packets that are being forwarded. */
        MIB2_ipOutRequests_Inc;
    }

    else
        ip_dgram = NU_NULL;

#endif

    /* If an IP header is not already included, get a new buffer for the
     * IP and link-layer headers.
     */
    if (!(buf_ptr->mem_flags & NET_NOHDR))
    {
        /* Allocate a new buffer chain for the link-layer and IP header */
        hdr_buf =
            MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                     (INT32)(hlen + int_face->dev_hdrlen));

        /* If a new chain of buffers was allocated, link the buffer in as
         * the first buffer in the list.
         */
        if (hdr_buf)
        {
            /* Link the new buffer in to the list */
            hdr_buf->next_buffer = buf_ptr;

            /* Set the list to which the header buffers will be freed */
            hdr_buf->mem_dlist = &MEM_Buffer_Freelist;

            /* If the buffer chain passed in is flagged as a parent chain,
             * flag the new chain as a parent too.
             */
            if (buf_ptr->mem_flags & NET_PARENT)
                hdr_buf->mem_flags |= NET_PARENT;

            /* Set the data pointer to the beginning of the IP header */
            hdr_buf->data_ptr =
                    hdr_buf->mem_parent_packet + int_face->dev_hdrlen;

            /* Set the total data length of the chain of buffers */
            hdr_buf->mem_total_data_len = buf_ptr->mem_total_data_len;

#if (INCLUDE_IPSEC == NU_TRUE)

            /* Check whether IPsec is enabled for this device */
            if (int_face->dev_flags2 & DV_IPSEC_ENABLE)
            {
                /* Copy the TCP/UDP port pointer. This will be used for
                 * IPsec encoding later.
                 */
                hdr_buf->mem_port = buf_ptr->mem_port;
            }
#endif
        }

        /* If a buffer could not be allocated, return an error. */
        else
        {
            /* Free the route entry */
            if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && ro->rt_route)
                RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);

            return (NU_NO_BUFFERS);
        }
    }

    /* Otherwise, the packet is going out the same interface that it
     * came in, so we can reuse the headers.  The data pointer of the
     * incoming buffer is already set to the IP header.
     */
    else
        hdr_buf = buf_ptr;

#if (INCLUDE_IP_RAW == NU_TRUE)
    /* If this is a Raw IP Packet, don't modify the header */
    if (flags & IP_RAWOUTPUT)
        hlen = (INT16)((GET8(buf_ptr->data_ptr,
                             IP_VERSIONANDHDRLEN_OFFSET) & 0xf) << 2);

    else
#endif
    {
        /* If a Time To Live was not specified, use the default */
        if (ttl == 0)
            ttl = IP_Time_To_Live;

        /* Get a pointer to the IP header. */
        ip_dgram = (IPLAYER*)hdr_buf->data_ptr;

        /* Fill-in the parameters of the IP header. */
        /* Set the IP header length and the IP version. */
        PUT8(ip_dgram, IP_VERSIONANDHDRLEN_OFFSET,
             (UINT8)((hlen >> 2) | (IP_VERSION << 4)) );

        /* Zero out the fragment field */
        PUT16(ip_dgram, IP_FRAGS_OFFSET, 0);

        /* Set the IP packet ID. */
        PUT16(ip_dgram, IP_IDENT_OFFSET, (UINT16)IP_Ident++);

        /* Set the type of service. */
        PUT8(ip_dgram, IP_SERVICE_OFFSET, (UINT8)tos);

        /* Set the total length (data and IP header) of this packet. */
        PUT16(ip_dgram, IP_TLEN_OFFSET,
                    (UINT16)(hdr_buf->mem_total_data_len + hlen));

        /* Set the time to live */
        PUT8(ip_dgram, IP_TTL_OFFSET, (UINT8)ttl);

        /* Set the protocol. */
        PUT8(ip_dgram, IP_PROTOCOL_OFFSET, (UINT8)protocol);

        /* Set the destination IP address. */
        PUT32(ip_dgram, IP_DEST_OFFSET, dest_ip);

        /* Increment the number of IP packets transmitted. NOTE: this does
           not include packets that are being forwarded. */
        MIB2_ipOutRequests_Inc;

        /* Update the length of the header buffer. */
        hdr_buf->data_len           += hlen;
        hdr_buf->mem_total_data_len += hlen;
    }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    /* Is this packet destined for a multicast address */
    if (buf_ptr->mem_flags & NET_MCAST)
    {
        /* Flag this buffer as containing a multicast packet. */
        hdr_buf->mem_flags |= NET_MCAST;

        /* Determine the Time To Live for the packet. */
        if (sck_opts != NU_NULL)
            PUT8(ip_dgram, IP_TTL_OFFSET, (UINT8)sck_opts->multio_ttl);
        else
            PUT8(ip_dgram, IP_TTL_OFFSET, (UINT8)IP_DEFAULT_MULTICAST_TTL);
    }
    else
#endif
    {
        /* If destination address is a broadcast, check that the interface
         * supports broadcasting and that the caller enabled broadcasting.
         */
        if (!(IP_Broadcast_Addr(dest_ip, int_face)))
        {
            /* Make sure the broadcast flag is clear. */
            hdr_buf->mem_flags &= ~NET_BCAST;
        }

        else
        {
            /* Does the interface support broadcasting. */
            if ((int_face->dev_flags & DV_BROADCAST) == 0)
                status = NU_ACCESS;

            /* Did the caller enable broadcasting. */
            else if ((flags & IP_ALLOWBROADCAST) == 0)
                status = NU_ACCESS;

            /* Inform the MAC layer to send this as a link-level broadcast. */
            else
                hdr_buf->mem_flags |= NET_BCAST;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* If a source address has not been specified, use the address of the
         * outgoing interface.  The source address will probably be known at
         * this point.
         */
#if (INCLUDE_IP_RAW == NU_TRUE)
        if (!(flags & IP_RAWOUTPUT))
#endif
        {
            if (src_ip != IP_ADDR_ANY)
                PUT32(ip_dgram, IP_SRC_OFFSET, src_ip);
            else
                PUT32(ip_dgram, IP_SRC_OFFSET,
                      int_face->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr);
        }

#if ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) ||(PRINT_IP_MSG == NU_TRUE))
        /* Print the IP header info */
        NLOG_IP_Info(ip_dgram, NLOG_TX_PACK);
#endif

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check whether IPsec is enabled for this device */
        if (int_face->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Apply the IPsec security. IPsec security headers will be
             * added as required by the IPsec policy. If not required, then
             * no security headers will be added.
             */

            /* If IPsec 2.0 is used */
#ifdef IPSEC_VERSION_COMP
            status = IP_IPSEC_Send(int_face, &hdr_buf, (VOID **)&ip_dgram,
                                   IPSEC_IPV4, (UINT8)protocol, NU_NULL, NU_NULL, NU_NULL, NU_NULL);
#else
                /* Otherwise IPsec 1.x is used. */
            status = IP_IPSEC_Send(int_face, &hdr_buf, (VOID **)&ip_dgram, IPSEC_IPV4,
                                   (UINT8)protocol);
#endif

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Packet discarded, IP_IPSEC_Send failed",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Continue only if status is success. */
        if (status == NU_SUCCESS)
#endif
        {
            /* If this packet is small enough send it.*/
            if ( (GET16(ip_dgram, IP_TLEN_OFFSET) <= ro->rt_route->rt_path_mtu) &&
                 (GET16(ip_dgram, IP_TLEN_OFFSET) <= int_face->dev_mtu) )
            {

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)

                /* If Path MTU Discovery is active for this route, set the Don't
                 * Fragment bit in the packet.
                 */
                if ( (!(ro->rt_route->rt_flags & RT_STOP_PMTU)) &&
                     (!(flags & IP_MAY_FRAGMENT)) )
                    PUT16(ip_dgram, IP_FRAGS_OFFSET,
                          (UINT16)(GET16(ip_dgram, IP_FRAGS_OFFSET) | IP_DF));
#endif

                /* Compute the IP checksum. Note that the length expected by
                 * TLS_IP_Check is the length of the header in 16 bit half-words.
                 */
                PUT16(ip_dgram, IP_CHECK_OFFSET, 0);

#if (HARDWARE_OFFLOAD == NU_TRUE)
                /* If the hardware is not going to compute the checksum, do
                 * it now in software.
                 */
                if (!(int_face->dev_hw_options_enabled & HW_TX_IP4_CHKSUM))
#endif
                {
                    PUT16(ip_dgram, IP_CHECK_OFFSET,
                          TLS_IP_Check((VOID*)ip_dgram, (UINT16)(hlen >> 1)));
                }

                /* Set the packet type that is in the buffer. */
                hdr_buf->mem_flags |= NET_IP;

                /* Send the packet. */
                status = (*(int_face->dev_output))(hdr_buf, int_face, dest, ro);
            }

            /* This packet must be fragmented. */
            else
            {
#if (INCLUDE_IP_FRAGMENT == NU_TRUE)

                /* If the don't fragment bit is set return an error. */
                if (GET16(ip_dgram, IP_FRAGS_OFFSET) & IP_DF)
                {
                    /* Increment the number of IP packets that could not be
                       fragmented. In this case because the don't fragment
                       bit is set. */
                    MIB2_ipFragFails_Inc;

                    status = NU_MSGSIZE;
                }

                else
                {
#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)
                    /* If Path MTU Discovery is active for this route, set the Don't
                     * Fragment bit in the packet.
                     */
                    if ( (!(ro->rt_route->rt_flags & RT_STOP_PMTU)) &&
                         (!(flags & IP_MAY_FRAGMENT)) )
                        PUT16(ip_dgram, IP_FRAGS_OFFSET,
                              (UINT16)(GET16(ip_dgram, IP_FRAGS_OFFSET) | IP_DF));
#endif

                    status = IP_Fragment(hdr_buf, ip_dgram, int_face, dest, ro);
                }
#else
                if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && ro->rt_route )
                    RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);

                status = NU_MSGSIZE;
#endif

            }
        }
    }

    /* If the packet could not be transmitted, and the IP layer built
     * a header for the packet, send the header to the dlist.
     */
    if ( (status != NU_SUCCESS) && (!(hdr_buf->mem_flags & NET_NOHDR)) )
    {
        /* Get a pointer to the first packet in the header chain */
        temp_buf = hdr_buf;

        /* Find the end of the header chain */
        while (temp_buf->next_buffer != buf_ptr && temp_buf->next_buffer != NU_NULL)
            temp_buf = temp_buf->next_buffer;

        /* Terminate the header chain of packets */
        temp_buf->next_buffer = NU_NULL;

        /* Free the header chain of packets */
        MEM_One_Buffer_Chain_Free(hdr_buf, hdr_buf->mem_dlist);
    }

    if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && ro->rt_route )
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP);

    return (status);

} /* IP_Send */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Broadcast_Addr
*
*   DESCRIPTION
*
*       This function checks an IP address to see if it is a broadcast
*       address.
*
*   INPUTS
*
*       dest                    The IP address to be checked.
*       *int_face               Pointer to the interface the IP address
*                               is being used on.
*
*   OUTPUTS
*
*       1                       This is a broadcast address.
*       0                       This is not a broadcast address.
*
*************************************************************************/
STATIC INT IP_Broadcast_Addr(UINT32 dest, const DV_DEVICE_ENTRY *int_face)
{
    DEV_IF_ADDR_ENTRY *current_addr;

    if ( (dest == IP_ADDR_ANY) || (dest == IP_ADDR_BROADCAST) )
        return (1);

    if ( (int_face->dev_flags & DV_BROADCAST) == 0)
        return (0);

    /* Get a pointer to the first address in the list of addresses
     * on the device.
     */
    current_addr = int_face->dev_addr.dev_addr_list.dv_head;

    /* Search the entire list of addresses for a matching network
     * address.
     */
    while (current_addr)
    {
        /* If this is a matching network entry */
        if ( (current_addr->dev_entry_net == dest) ||
             (current_addr->dev_entry_net_brdcast == dest) )
            return (1);

        /* Get a pointer to the next address in the list of addresses
         * on the device.
         */
        current_addr = current_addr->dev_entry_next;
    }

    return (0);

} /* IP_Broadcast_Addr */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Find_Route
*
*   DESCRIPTION
*
*       This function checks to see if a route is still valid. If the
*       route is not valid or if a route has never been allocated. A new
*       route is found.
*
*   INPUTS
*
*       *ro                     Pointer to the RTAB route information
*                               structure
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IP_Find_Route(RTAB_ROUTE *ro)
{
    if ( (ro->rt_route) &&
         (ro->rt_route->rt_entry_parms.rt_parm_device) &&
         (ro->rt_route->rt_entry_parms.rt_parm_flags & RT_UP) )
        return;

    ro->rt_route = RTAB4_Find_Route(&ro->rt_ip_dest, RT_BEST_METRIC);

} /* IP_Find_Route */

/************************************************************************
*
*   FUNCTION
*
*       IP_Localaddr
*
*   DESCRIPTION
*
*       Searches through the attached devices to verify that the IP
*       address passed in is a local IP address on the network.
*
*   INPUTS
*
*       *ip_addr                A pointer to the ip address
*
*   OUTPUTS
*
*       1                       The IP address is on a local device.
*       0                       The IP address is not on a local device.
*
*************************************************************************/
INT IP_Localaddr(UINT32 ip_addr)
{
    DV_DEVICE_ENTRY         *device;
    DEV_IF_ADDR_ENTRY       *current_addr = NU_NULL;

    for (device = DEV_Table.dv_head; device && (current_addr == NU_NULL);
         device = device->dev_next)
    {
        /* Get a pointer to the first address in the list of addresses
         * for the device.
         */
        current_addr = device->dev_addr.dev_addr_list.dv_head;

        while (current_addr)
        {
            /* If the IP addr is directly connected. Return true. */
            if ( (current_addr->dev_entry_net) ==
                 (ip_addr & current_addr->dev_entry_netmask) )
                return (1);

            /* Otherwise, get a pointer to the next address entry in the
             * list of addresses for the device.
             */
            else
                current_addr = current_addr->dev_entry_next;
        }
    }

    /* Return false, the IP addr is not on a local network. */
    return (0);

} /* IP_Localaddr */

#endif

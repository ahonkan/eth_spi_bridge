/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*       ip6.c
*
*   DESCRIPTION
*
*       This file contains those functions necessary to maintain the IP
*       layer for IPv6.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IP6_Initialize
*       IP6_Interpret
*       IP6_Process_Extension_Headers
*       IP6_Process_Routing_Header
*       IP6_Send
*       IP6_Build_Extension_Headers
*       IP6_Find_Route
*       IP6_Find_Link_Local_Addr
*       IP6_Swap_Addresses
*       IP6_Find_Global_Addr
*       IP6_Handle_Solicited_Node_Multi_Event
*       IP6_Join_Solicited_Node_Multi
*       IP6_Leave_Solicited_Node_Multi
*       IP6_Create_IPv4_Mapped_Addr
*       IP6_Setup_Options
*       IP6_Setup_Route
*
*   DEPENDENCIES
*
*       nu_net.h
*       in6.h
*       defrtr6.h
*       nc6.h
*       prefix6.h
*       udp6.h
*       tcp6.h
*       ipraw6.h
*       nd6radv.h
*       externs6.h
*       ip6_mib.h
*
*************************************************************************/

#include "networking/nu_net.h"


#include "networking/in6.h"
#include "networking/defrtr6.h"
#include "networking/nc6.h"
#include "networking/prefix6.h"
#include "networking/udp6.h"
#include "networking/tcp6.h"
#include "networking/ipraw6.h"
#include "networking/nd6radv.h"
#include "networking/externs6.h"
#include "networking/ip6_mib.h"
#include "networking/dad6.h"
#include "networking/mld6.h"

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

#if (INCLUDE_IKE == NU_TRUE)
#include "networking/ike_api.h"
#endif
#endif


UINT8   IP6_Hop_Limit;

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
/* A cached IPv6 forwarding route. */
RTAB6_ROUTE     IP6_Forward_Rt;
#endif

#if (INCLUDE_IP_REASSEMBLY == NU_TRUE)
IP6_QUEUE     IP6_Frag_Queue;
#endif

struct id_struct    IP6_ADDR_ANY;
struct id_struct    IP6_LOOPBACK_ADDR;

UINT8   IP6_All_Nodes_Multi[16] = {0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01};
UINT8   IP6_All_Routers_Multi[16] = {0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02};
UINT8   IP6_All_Hosts_Multi[16] = {0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x03};
UINT8   IP6_Solicited_Node_Multi[16] = {0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0xFF, 0, 0, 0};
UINT8   IP6_Loopback_Address[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
UINT8   IP6_Link_Local_Prefix[LINK_LOCAL_PREFIX_LENGTH] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0};

TQ_EVENT    IP6_Expire_Address_Event;
TQ_EVENT    IP6_Deprecate_Address_Event;
TQ_EVENT    IP6_Verify_Valid_Addr_Event;
TQ_EVENT    IP6_Join_Solicited_Mcast_Event;

IP6_POLICY_TABLE_STRUCT     IP6_Policy_Table;

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)
TQ_EVENT            IP6_Transmit_Rtr_Adv_Event;
#endif

STATUS IP6_Build_Extension_Headers(NET_BUFFER **, IP6LAYER **, UINT8,
                                   IP6_S_OPTIONS *, DV_DEVICE_ENTRY *,
                                   RTAB6_ROUTE *);
STATUS IP6_Process_Extension_Headers(NET_BUFFER **, IP6LAYER **, UINT16 *, UINT32 *,
                                     INT16 *, DV_DEVICE_ENTRY **, UINT8 *);
STATUS IP6_Process_Routing_Header(IP6LAYER *, NET_BUFFER *, UINT16 *, INT16 *,
                                  UINT8 *, UINT16, UINT32 *, INT16 *, DV_DEVICE_ENTRY **);
STATUS IP6_Setup_Route(const UINT8 *, DV_DEVICE_ENTRY *, const RTAB6_ROUTE *,
                       IP6_S_OPTIONS *, UINT8 *);

extern IP6_POLICY_ENTRY IP6_Default_Policy_Entries[];

/****************************************************************************
*
*   FUNCTION
*
*       IP6_Initialize
*
*   DESCRIPTION
*
*       This function initializes the IPv6 specific data structures and
*       events.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
****************************************************************************/
VOID IP6_Initialize(VOID)
{
    INT         i;

    /* The default IPv6 Hop Limit should be 64 */
    IP6_Hop_Limit = IP6_HOP_LIMIT;

    /* Initialize the wildcard address */
    UTL_Zero(IP6_ADDR_ANY.is_ip_addrs, MAX_ADDRESS_SIZE);

    /* Initialize the loopback address */
    NU_BLOCK_COPY(IP6_LOOPBACK_ADDR.is_ip_addrs, IP6_Loopback_Address,
                  IP6_ADDR_LEN);

#if (INCLUDE_IP_REASSEMBLY == NU_TRUE)
    /* Initialize the IPV6 Fragmentation Queue */
    IP6_Frag_Queue.ipq_head = NU_NULL;
    IP6_Frag_Queue.ipq_tail = NU_NULL;
#endif

    /* Register the event to expire an IPv6 address */
    if (EQ_Register_Event(DEV6_Expire_Address,
                          &IP6_Expire_Address_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Address Expiration event",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Register the event to deprecate an IPv6 address */
    if (EQ_Register_Event(DEV6_Deprecate_Address,
                          &IP6_Deprecate_Address_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Address Deprecation event",
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Register the event to verify that the link-local IP address is valid
     * on the link.
     */
    if (EQ_Register_Event(DEV6_Verify_Valid_Addr,
                          &IP6_Verify_Valid_Addr_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Verification event", NERR_SEVERE,
                       __FILE__, __LINE__);

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

    /* Register the event to handle router advertisements */
    if (EQ_Register_Event(ND6RADV_Handle_Event,
                          &IP6_Transmit_Rtr_Adv_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Router Advertisement event",
                       NERR_SEVERE, __FILE__, __LINE__);

#endif

#if (INCLUDE_IP_MULTICASTING)

    /* Register the event to join the solicited-node multicast address. */
    if (EQ_Register_Event(IP6_Handle_Solicited_Node_Multi_Event,
                          &IP6_Join_Solicited_Mcast_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register solicted-node multicast event", NERR_SEVERE,
                       __FILE__, __LINE__);
#endif

    /* Initialize the head and tail of the Policy Table that is used for
     * source and destination address selection.
     */
    IP6_Policy_Table.head = NU_NULL;
    IP6_Policy_Table.tail = NU_NULL;

    /* Initialize i. */
    i = 0;

    /* Add each entry in the Default Policy Table to the system Policy
     * Table.
     */
    for (;;)
    {
        /* Add the policy. */
        IP6_Add_Policy(&IP6_Default_Policy_Entries[i]);

        /* Move on to the next entry in the table. */
        i ++;

        /* If this is the empty entry, exit the loop. */
        if ( (memcmp(IP6_Default_Policy_Entries[i].prefix,
                     IP6_ADDR_ANY.is_ip_addrs, IP6_ADDR_LEN) == 0) &&
             (IP6_Default_Policy_Entries[i].prefix_len == 0) &&
             (IP6_Default_Policy_Entries[i].precedence == 0) &&
             (IP6_Default_Policy_Entries[i].label == 0) )
        {
            break;
        }
    }

#if (INCLUDE_DHCP6 == NU_TRUE)

    if (DHCP6_Init() != NU_SUCCESS)
        NLOG_Error_Log("Failed to initialize DHCPv6 client module",
                       NERR_SEVERE, __FILE__, __LINE__);

#endif

} /* IP6_Initialize */

/****************************************************************************
*
*   FUNCTION
*
*       IP6_Interpret
*
*   DESCRIPTION
*
*       Called by the packet demuxer to interpret a new IPv6 packet.  Checks
*       the validity of the packet (checksum, flags) and then passes it
*       on to the appropriate protocol handler.
*
*   INPUTS
*
*       *pkt                    A pointer to the IPv6 header of the incoming
*                               packet.
*       *device                 A pointer to the device on which the packet
*                               was received.
*       *buf_ptr                A pointer to the incoming packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       1                       Packet dropped.
*
****************************************************************************/
STATUS IP6_Interpret(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr)
{
    UINT8               next_header = 0;
    INT16               icmp_code = -4;
    UINT16              hlen;
    UINT32              icmp_offset = 0;
    UINT32              total;
    DV_DEVICE_ENTRY     *temp_dev;
    struct pseudohdr    pseudo_header;
    NET_BUFFER          *buf;
    STATUS              status;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    IP6_MULTI           *ipm;
#endif

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
#if (INCLUDE_IPSEC == NU_TRUE)
    /* Tunnel Mode */
    static STATUS       update_inbound_sa = NU_TRUE;

    /* Index for the inbound SA. */
    IPSEC_INBOUND_INDEX sa_index;
#endif
#endif

    MIB_ipv6IfStatsInReceives_Inc(device);

    /* If the device on which the packet was received is not up or is
     * not IPv6-enabled, drop the packet.
     */
    if ( (!(device->dev_flags & DV_UP)) ||
         (!(device->dev_flags & DV6_IPV6)) ||
         (!(device->dev_flags2 & DV6_UP)) )
    {
        MIB_ipv6IfStatsInDiscards_Inc(device);

        /* Drop it. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("IPv6 packet dropped because interface is down",
                       NERR_SEVERE, __FILE__, __LINE__);

        return (1);
    }

    /* A Multicast address is never supposed to be the Source Address of a
     * packet, and the Unspecified Address is never supposed to be the
     * Destination Address of a packet.
     */
    if ( (IPV6_IS_ADDR_MULTICAST(pkt->ip6_src)) ||
         (IPV6_IS_ADDR_UNSPECIFIED(pkt->ip6_dest)) )
    {
        MIB_ipv6IfStatsInHdrErrors_Inc(device);

        MIB_ipv6IfStatsInAddrErr_Inc(device);

        /* Drop it. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("IPv6 packet dropped because of invalid source or destination address",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (1);
    }

    /* If the packet is destined for the loopback address, and the packet
     * was not received over the loopback interface, the packet must be
     * rejected.
     */
    if ( (memcmp(IP6_Loopback_Address, pkt->ip6_dest, IP6_ADDR_LEN) == 0) &&
         (device->dev_type != DVT_LOOP) )
    {
        /* Packets destined to the loopback address are accepted only
         * from the loopback interface.
         */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("IPv6 packet dropped due to loopback destination",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (1);
    }

    /* Ensure that the version field is correct. */
    if ((GET32(pkt, IP6_XXX_OFFSET) & 0xf0000000) != IP6_VERSION)
    {
        MIB_ipv6IfStatsInHdrErrors_Inc(device);

        MIB_ipv6IfStatsInAddrErr_Inc(device);

        /* Drop it. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("IPv6 packet dropped because of invalid version",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (1);
    }

    /* Extract total length of the packet.  The payload is all data following
     * the basic IPv6 header.  The payload includes Extension Headers.
     */
    buf_ptr->mem_total_data_len = IP6_HEADER_LEN + GET16(pkt, IP6_PAYLEN_OFFSET);

    /* Initialize the header length to the length of the basic IPv6 header.
     * The variable hlen will hold the length of the IPv6 header and all
     * extension headers.
     */
    hlen = IP6_HEADER_LEN;

    /* Check to see if the total data length is less than the sum of the data
     * lengths the buffers in the chain. This at first sounds impossible. However
     * data_len comes from the size reported by the driver. It is not unusual to
     * receive a packet that has been padded. The driver does not distinguish
     * between real data and padded data. However, the IP header contains the
     * true data length. We want to use the smaller value.
     */
    for (buf = buf_ptr, total = buf->mem_total_data_len;
         buf;
         buf = buf->next_buffer)
    {
        if (buf->data_len > total)
            buf->data_len = total;

        total -= buf->data_len;
    }

    /* Check if the packet is destined for one of the interfaces on this node */
    temp_dev = DEV6_Get_Dev_By_Addr(pkt->ip6_dest);

    /* If the destination IP address did not match any of ours, check to
     * see if it is destined for a multicast address.
     */
    if (!temp_dev)
    {
        if (IPV6_IS_ADDR_MULTICAST(pkt->ip6_dest))
        {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

            /* Flag the packet as multicast */
            buf_ptr->mem_flags |= NET_MCAST;

            /* If we belong to the multicast group, set temp_dev so the checks
             * below will be passed.
             */
            ipm = IP6_Lookup_Multi(pkt->ip6_dest, device->dev6_multiaddrs);

            if (ipm != NU_NULL)
                temp_dev = ipm->ipm6_data.multi_device;

            /* If the interface is operating in promiscuous mode, allow all
             * multicast traffic to pass to the upper layer.
             */
            else if (device->dev_flags & DV_PROMISC)
                temp_dev = device;

            else
            {
                MIB_ipv6IfStatsInHdrErrors_Inc(device);

                MIB_ipv6IfStatsInAddrErr_Inc(device);
            }

#else
            MIB_ipv6IfStatsInHdrErrors_Inc(device);

            MIB_ipv6IfStatsInAddrErr_Inc(device);

            /* Multicasting support was not desired so drop the packet by
             * placing it back on the buffer_freelist.
             */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            NLOG_Error_Log("IPv6 packet dropped because multicasting is not enabled",
                            NERR_INFORMATIONAL, __FILE__, __LINE__);

            return (1);
#endif
        }
    }

    /* Process Extension Headers.  The variable hlen will be incremented
     * according to the length of all combined Extension Headers.
     */
    status = IP6_Process_Extension_Headers(&buf_ptr, &pkt, &hlen,
                                           &icmp_offset, &icmp_code,
                                           &temp_dev, &next_header);

    if (status == NU_INVAL_NEXT_HEADER)
    {
        MIB_ipv6IfStatsInHdrErrors_Inc(device);
        return (1);
    }

    /* Invalidate the next_header variable so an ICMP Error message is
     * triggered.
     */
    else if (status == NU_SEND_ICMP_ERROR)
        next_header = 0;

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Check whether IPsec is enabled for the device which received this
     * packet.
     */
    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
    {
        /* Create the IPsec packet selector for this packet. This will be
         * used throughout the life of this packet for IPsec processing.
         */
        IP_IPSEC_Pkt_Selector(&IPSEC_Pkt_Selector, pkt, buf_ptr, next_header,
                              IPSEC_IPV6, 0);
    }
#endif

    if (!temp_dev)
    {
        /* This packet is not for us. Attempt to forward the packet if
         * possible.
         */
#if INCLUDE_IP_FORWARDING

        if (icmp_code == -4)
        {
            /* Remove the buffer that we have been processing from the
             * buffer_list.  The IPv6 forwarding function will handle the
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
                    if (update_inbound_sa == NU_TRUE)
                    {
                        /* Set the IPsec destination address. */
                        NU_BLOCK_COPY(sa_index.ipsec_dest,
                                      (((UINT8 *)(pkt)) + IP6_DESTADDR_OFFSET),
                                      IP6_ADDR_LEN);

                        /* Set the destination type. */
                        sa_index.ipsec_dest_type =
                            (IPSEC_IPV6 | IPSEC_SINGLE_IP);

                        /* Set the IPsec protocol. */
                        sa_index.ipsec_protocol =
                            GET8(pkt,IP6_NEXTHDR_OFFSET);

                        /* Set the SPI. */
                        if (sa_index.ipsec_protocol == IPSEC_AH)
                        {
                            sa_index.ipsec_spi =
                                GET32(pkt,(IP6_HEADER_LEN + IPSEC_AH_SPI_OFFSET));
                        }

                        else
                        {
                            sa_index.ipsec_spi =
                                GET32(pkt,(IP6_HEADER_LEN + IPSEC_ESP_SPI_OFFSET));
                        }

                        /* Find a matching SA in the inbound Security
                         * Association database.
                         */
                        status =
                            IPSEC_Get_Inbound_SA(device->dev_physical->dev_phy_ips_group,
                                                 &sa_index,
                                                 &IPSEC_In_Bundle[IPSEC_SA_Count]);
                    }

                    if (status == NU_SUCCESS)
                        IPSEC_SA_Count = 1;

                    /* Before we forward the packet: It is possible that
                     * this packet was received in IPsec Tunnel Mode
                     * and we have already decoded the outer header.
                     * Check the IPsec policy to ensure that the correct
                     * SAs have been applied. Even if no IPsec protocol
                     * was applied, verify that this packet should be
                     * allowed to pass.
                     */
                    status =
                        IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                              &IPSEC_Pkt_Selector, IPSEC_In_Bundle,
                                              IPSEC_SA_Count, NU_NULL);

                    /* If while checking the policy an error occurred,
                     * discard the packet and log an error.
                     */
                    if (status != NU_SUCCESS)
                    {
                        /* Log an error. */
                        NLOG_Error_Log("IPsec policy check failed ",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        /* Drop the packet by placing it back on the
                         * Free List.
                         */
                        MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                              &MEM_Buffer_Freelist);

                        /* Return. */
                        return (1);
                    }
                }
#endif

                if (IP6_Forward(pkt, buf_ptr) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to forward packet", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);

#if (INCLUDE_IPSEC == NU_TRUE)

                else
                {
                    /* Tunnel mode */
                    if (update_inbound_sa == NU_FALSE)
                        update_inbound_sa = NU_TRUE;
                }
#endif
            }

            else
            {
                /* Increment the number of IP packets received with the
                 * wrong IP addr.
                 */
                MIB_ipv6IfStatsInHdrErrors_Inc(device);

                MIB_ipv6IfStatsInAddrErr_Inc(device);

                /* Drop it. */
                MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
            }
        }

        else if (icmp_code != -1)
        {
            MIB_ipv6IfStatsInHdrErrors_Inc(device);

            buf_ptr->mem_total_data_len += hlen;
            buf_ptr->data_len           += hlen;
            buf_ptr->data_ptr           -= hlen;

            ICMP6_Send_Error(pkt, buf_ptr, ICMP6_PARAM_PROB, (UINT8)icmp_code,
                             icmp_offset, device);
        }

#else
        MIB_ipv6IfStatsInHdrErrors_Inc(device);

        MIB_ipv6IfStatsInAddrErr_Inc(device);

        NLOG_Error_Log("IPv6 packet dropped because forwarding is not enabled",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        /* Drop it. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
#endif

        return (1);
    }

    /* If the packet was received on a virtual interface, and the source or
     * destination embedded IPv4 address is not a unicast address, drop
     * the packet.
     */
    if ( (temp_dev->dev_flags & DV6_VIRTUAL_DEV) &&
         ((IP_MULTICAST_ADDR(IP_ADDR(&pkt->ip6_src[2]))) ||
          (IP_ADDR(&pkt->ip6_src[2]) == IP_ADDR_BROADCAST) ||
          (IP_ADDR(&pkt->ip6_src[2]) == LOOPBACK_ADDR) ||
          (IP_MULTICAST_ADDR(IP_ADDR(&pkt->ip6_dest[2]))) ||
          (IP_ADDR(&pkt->ip6_dest[2]) == IP_ADDR_BROADCAST) ||
          (IP_ADDR(&pkt->ip6_dest[2]) == LOOPBACK_ADDR)) )
    {
        MIB_ipv6IfStatsInHdrErrors_Inc(device);

        MIB_ipv6IfStatsInAddrErr_Inc(device);

        /* Drop it. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        NLOG_Error_Log("IPv6 packet dropped because of invalid source or destination address",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return (1);
    }

    /* Create the pseudo header for upper layer protocols to compute
     * their checksum.
     */
    NU_BLOCK_COPY(pseudo_header.source, pkt->ip6_src, IP6_ADDR_LEN);
    NU_BLOCK_COPY(pseudo_header.dest, pkt->ip6_dest, IP6_ADDR_LEN);

    pseudo_header.next = next_header;
    pseudo_header.zero = 0;
    pseudo_header.length = LONGSWAP(buf_ptr->mem_total_data_len);

    /* Set the flag to indicate that this is an IPv6 packet */
    buf_ptr->mem_flags |= NET_IP6;


#if (INCLUDE_IP_INFO_LOGGING == NU_TRUE)
    /* Print/store the IP header info */
    NLOG_IP6_Info(pkt, NLOG_RX_PACK);
#endif

    /* which protocol to handle this packet? */
    switch (next_header)
    {
#if (INCLUDE_UDP == NU_TRUE)

    case IP_UDP_PROT:

        MIB_ipv6IfStatsInDelivers_Inc(device);

        status = UDP6_Input(pkt, buf_ptr, &pseudo_header);

        /* If the datagram was not delivered because there is no port on this
         * node that matches the destination port of the UDP header, send an
         * ICMP Port Unreachable error.
         */
        if (status == NU_DEST_UNREACH_PORT)
        {
            buf_ptr->mem_total_data_len += hlen;
            buf_ptr->data_len           += hlen;

            ICMP6_Send_Error(pkt, buf_ptr, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_PORT,
                             0, device);

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
        }

        return (status);

#endif

#if (INCLUDE_TCP == NU_TRUE)

    case IP_TCP_PROT:

        MIB_ipv6IfStatsInDelivers_Inc(device);

        return (TCP6_Input(buf_ptr, &pseudo_header));

#endif

    case IP_ICMPV6_PROT:

        MIB_ipv6IfStatsInDelivers_Inc(device);

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check whether IPsec is enabled for the device which received
         * this packet.
         */
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
                NLOG_Error_Log("IPsec policy in check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Drop the packet by placing it back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                      &MEM_Buffer_Freelist);

                /* Return. */
                return (status);
            }
        }

#endif

        return (ICMP6_Interpret(pkt, temp_dev, buf_ptr, &pseudo_header));

#if (INCLUDE_IP_RAW == NU_TRUE)

    case IP_RAW_PROT :
    case IP_HELLO_PROT :
    case IP_OSPF_PROT :

    /** Any additional Raw IP protocols that are supported in the
        future must have a case here ***/

        MIB_ipv6IfStatsInDelivers_Inc(device);

#if (INCLUDE_IPSEC == NU_TRUE)

        /* Check whether IPsec is enabled for the device which received
         * this packet.
         */
        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* Check the packet selector and the applied security associations
             * with the IPsec policy.
             */
            status =
                IPSEC_Match_Policy_In(device->dev_physical->dev_phy_ips_group,
                                      &IPSEC_Pkt_Selector,IPSEC_In_Bundle,
                                      IPSEC_SA_Count, NU_NULL);

            /* If while checking the policy an error occurred, discard the
             * packet and log an error.
             */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("IPsec policy in check failed ",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Drop the packet by placing it back on the Free List. */
                MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                      &MEM_Buffer_Freelist);

                /* Return. */
                return (status);
            }
        }

#endif

        return (IPRaw6_Interpret(pkt, (NET_BUFFER *)buf_ptr,
                                 &pseudo_header, next_header));

#endif

#if (INCLUDE_IPSEC == NU_TRUE)

    case IPPROTO_IPV6:

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
        /* Tunnel Mode. Do not apply IPsec to encapsulated IP packet. */
        update_inbound_sa = NU_FALSE;
#endif

        if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
        {
            /* This is an IPv6 packet encapsulated in an IPv6 packet.
             * Pass the packet to the IPv6 Interpret routine.
             */
            return (IP6_Interpret((IP6LAYER*)buf_ptr->data_ptr, device,
                                  buf_ptr));
        }
#endif

    /* If the next header is No Next Header, exit the routine without
     * sending an ICMPv6 error message.
     */
    case IPPROTO_NONEXTHDR:

        NLOG_Error_Log("IPv6 packet next header is No Next Header",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        return (NU_SUCCESS);

    default:

        MIB_ipv6IfStatsInUndefProt_Inc(device);

        if (icmp_code != -1)
        {
            /* If no ICMPv6 error code was set when processing extension
             * headers, the next-header is not recognized.  Set the
             * proper error code.
             */
            if (icmp_code == -4)
                icmp_code = ICMP6_PARM_PROB_NEXT_HDR;

            buf_ptr->mem_total_data_len += hlen;
            buf_ptr->data_len           += hlen;
            buf_ptr->data_ptr           -= hlen;

            ICMP6_Send_Error(pkt, buf_ptr, ICMP6_PARAM_PROB, (UINT8)icmp_code,
                             icmp_offset, device);
        }

        NLOG_Error_Log("IPv6 packet dropped because of unknown protocol",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

        return (1);
    }

} /* IP6_Interpret */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Process_Extension_Headers
*
*   DESCRIPTION
*
*       This function processes the Extension Headers on an incoming
*       IPv6 packet.
*
*   INPUTS
*
*       **buf_ptr               A pointer to the buffer.
*       *header_len             The length of the IPv6 header and all
*                               extension headers.
*       *icmp_offset            If an ICMP Error Code is to be transmitted
*                               by the calling routine, the offset into
*                               the original packet where the error
*                               occurred.
*       *icmp_code              If an ICMP Error Code is to be transmitted
*                               by the calling routine, the ICMP Code to
*                               transmit in the packet.
*       **temp_dev              A pointer to the pointer to the device
*                               on which the packet was received.
*       *next_header            The final next-header value in the last
*                               recognized IPv6 extension header.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was parsed successfully.
*       NU_INVAL_NEXT_HEADER    The next-header value is unrecognized.
*       NU_SEND_ICMP_ERROR      An error occurred, and an ICMP error
*                               should be transmitted to the sender.
*
*************************************************************************/
STATUS IP6_Process_Extension_Headers(NET_BUFFER **buf_arg_ptr, IP6LAYER **packet,
                                     UINT16 *header_len, UINT32 *icmp_offset,
                                     INT16 *icmp_code, DV_DEVICE_ENTRY **temp_dev,
                                     UINT8 *next_header)
{
    INT16       protocol = NU_INVAL_NEXT_HEADER;
    UINT8       opt_type;
    NET_BUFFER  *buf_ptr = *buf_arg_ptr;
    IP6LAYER    *pkt = *packet;
    UINT16      opt_length, prev_hdr_len = 0;
    STATUS      status = NU_SUCCESS;
    ICMP6_LAYER *icmp_header;

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Pointer to the interface on which this packet was received. We save
     * it here because temp_dev can be changed while processing extension
     * headers.
     */
    DV_DEVICE_ENTRY *int_face = *temp_dev;

#endif

    *next_header = buf_ptr->data_ptr[IP6_NEXTHDR_OFFSET];
    *icmp_offset = IP6_NEXTHDR_OFFSET;

    /* Check if there are any extension headers present */
    if (!(IP6_IS_NXTHDR_RECPROT(*next_header)))
    {
        /* Increment the data pointer to point to the protocol header */
        buf_ptr->data_ptr += IP6_HEADER_LEN;

        /* While we have not found a valid protocol header and the total
         * number of bytes processed has not exceeded the length of the
         * packet.
         */
        while (*header_len <= buf_ptr->data_len)
        {
            switch (*next_header)
            {
                case IPPROTO_HOPBYHOP:

                    /* If, as a result of processing a header, a node
                     * encounters a Next Header value of zero in any
                     * header other than an IPv6 header, it should
                     * discard the packet and send an ICMP Parameter
                     * Problem message to the source of the packet, with
                     * an ICMP Code value of 1 ("unrecognized Next Header
                     * type encountered") and the ICMP Pointer field
                     * containing the offset of the unrecognized value
                     * within the original packet.
                     */
                    if (*header_len != IP6_HEADER_LEN)
                    {
                        /* Set the code for the ICMP message */
                        *icmp_code = ICMP6_PARM_PROB_NEXT_HDR;

                        /* Set the data pointer for the ICMP error message */
                        *icmp_offset =
                            (*header_len - prev_hdr_len) + IP6_EXTHDR_NEXTHDR_OFFSET;
                    }

                case IPPROTO_DEST:

                    /* Destination Options are processed only by the destination
                     * of the packet.
                     */
                    if ( (*temp_dev) || (*next_header == IPPROTO_HOPBYHOP) )
                    {
                        opt_length = 0;

                        /* Loop through the options */
                        while ( (protocol == NU_INVAL_NEXT_HEADER) &&
                                (opt_length != (UINT16)(6 + (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3))) )
                        {
                            /* Determine the type of option */
                            switch (buf_ptr->data_ptr[IP6_HOPBYHOP_OPTS_OFFSET +
                                                      IP6_TYPE_OFFSET + opt_length])
                            {
                                /* The PAD1 option does not have a Type or Length field,
                                 * only 1-byte of value equal to zero.
                                 */
                                case IP6OPT_PAD1:

                                    /* This option consumes 1 byte of data. */
                                    opt_length += 1;

                                    break;

                                case IP6OPT_PADN:
                                case IP6OPT_ROUTER_ALERT:

                                    /* Determine the length of this option. */
                                    opt_length = (UINT16)(opt_length +
                                        (2 + buf_ptr->data_ptr[IP6_HOPBYHOP_OPTS_OFFSET +
                                                               IP6_LENGTH_OFFSET +
                                                               opt_length]));
                                    break;

                                /* The option type is not recognized.  Determine
                                 * what should be done from the highest 2 bits in
                                 * the option type.
                                 */
                                default:

                                    opt_type =
                                        ((UINT8)(buf_ptr->data_ptr[IP6_HOPBYHOP_OPTS_OFFSET +
                                                                   IP6_TYPE_OFFSET + opt_length] & 0xc0));

                                    /* Discard the packet and, only if the packet's
                                     * Destination Address was not a multicast address,
                                     * send an ICMP Parameter Problem, Code 2, message
                                     * to the packet's Source Address, pointing to the
                                     * unrecognized Option Type.
                                     */
                                    if ((opt_type & 0xc0) == 0xc0)
                                    {
                                        status = protocol = NU_SEND_ICMP_ERROR;

                                        /* If the packet's destination address was not
                                         * multicast, set the ICMP code and data pointer
                                         * to be transmitted in the ICMP error message.
                                         */
                                        if (!(buf_ptr->mem_flags & NET_MCAST))
                                        {
                                            *icmp_code = ICMP6_PARM_PROB_OPTION;

                                            *icmp_offset = *header_len + opt_length + 2;
                                        }
                                        else
                                            *icmp_code = -1;
                                    }

                                    /* Discard the packet and, regardless of whether
                                     * or not the packet's Destination Address was a
                                     * multicast address, send an ICMP Parameter
                                     * Problem, Code 2, message to the packet's Source
                                     * Address, pointing to the unrecognized Option Type.
                                     */
                                    else if ((opt_type & 0x80) == 0x80)
                                    {
                                        status = protocol = NU_SEND_ICMP_ERROR;

                                        /* Set the ICMP code to be transmitted in the
                                         * ICMP error message.
                                         */
                                        *icmp_code = ICMP6_PARM_PROB_OPTION;

                                        *icmp_offset = *header_len + opt_length + 2;
                                    }

                                    /* Discard the packet */
                                    else if ((opt_type & 0x40) == 0x40)
                                    {
                                        status = protocol = NU_SEND_ICMP_ERROR;

                                        /* Discard the packet without transmitting an
                                         * ICMP error message.
                                         */
                                        *icmp_code = -1;
                                    }

                                    opt_length = (UINT16)(opt_length +
                                        (2 + buf_ptr->data_ptr[IP6_HOPBYHOP_OPTS_OFFSET +
                                                               IP6_LENGTH_OFFSET +
                                                               opt_length]));
                                    break;
                            }
                        }
                    }

                    break;

                case IPPROTO_ROUTING:

                    /* A Routing header is not examined or processed until it
                     * reaches the node identified in the Destination Address
                     * field of the IPv6 header.
                     */
                    if ( (*temp_dev) && (!(buf_ptr->mem_flags & NET_MCAST)) )
                    {
                        /* If there was an error processing the Routing Header,
                         * stop processing the packet.
                         */
                        if (IP6_Process_Routing_Header(pkt, buf_ptr, header_len,
                                                       &protocol, next_header,
                                                       prev_hdr_len, icmp_offset,
                                                       icmp_code, temp_dev) != NU_SUCCESS)
                            status = protocol = NU_SEND_ICMP_ERROR;
                    }

                    /* If the IPv6 destination address is multicast, discard
                     * the packet.
                     */
                    else if (IPV6_IS_ADDR_MULTICAST(pkt->ip6_dest))
                    {
                        *next_header =
                            (UINT8)buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

                        protocol =
                            (INT16)buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

                        *icmp_code = -1;
                    }

                    break;

                case IPPROTO_FRAGMENT:

                    /* A Fragment header is not examined or processed until it
                     * reaches the Destination node.
                     */
                    if (*temp_dev)
                    {
#if (INCLUDE_IP_REASSEMBLY == NU_TRUE)

                        status = IP6_Reassemble_Packet(&pkt, &buf_ptr, header_len,
                                                       next_header);

                        if (status == NU_NULL)
                            return (NU_INVAL_NEXT_HEADER);

                        /* Reassembly is done */
                        else
                        {
                            /* If the next header in the list of headers is
                             * a transport layer header.
                             */
                            if (IP6_IS_NXTHDR_RECPROT(*next_header))
                            {
                                buf_ptr->data_ptr += *header_len;
                                protocol = *next_header;
                            }

                            /* Otherwise, process the next extension header */
                            else
                            {
                                protocol = NU_NO_ACTION;
                                buf_ptr->data_ptr += *header_len;
                            }

                            *buf_arg_ptr = buf_ptr;
                            *packet = pkt;
                        }

#else
                        /* Increment the number of IP fragments that have been received,
                         * even though we don't process them.
                         */
                        MIB_ipv6IfStatsReasmReqds_Inc(buf_ptr->mem_buf_device);

                        /* Drop the current buffer. */
                        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

                        MIB_ipv6IfStatsReasmFails_Inc(buf_ptr->mem_buf_device);

                        return (NU_INVAL_NEXT_HEADER);
#endif
                    }

                    /* This node is an intermediate node; therefore, the packet
                     * should not be reassembled until it reaches the destination.
                     */
                    else
                    {
                        protocol = NU_NO_ACTION;
                        *next_header = buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];
                        buf_ptr->data_ptr += 8;
                        *header_len += 8;
                    }

                    break;

#if (INCLUDE_IPSEC == NU_TRUE)

                case IPPROTO_AUTH:
                case IPPROTO_ESP:

                    /* If IPsec is enabled on this interface. */
                    if (buf_ptr->mem_buf_device->dev_flags2 & DV_IPSEC_ENABLE)
                    {
                        /* Note that we support AH only, ESP only, or AH
                         * followed by ESP. In the following function call,
                         * we will check that we do not deviate from this
                         * restriction.
                         */

                        /* First save the current header length. */
                        prev_hdr_len =
                            (UINT16)(8 + (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));

                        /* Interpret the IPsec header. */
                        if (IP_IPSEC_Interpret(int_face, buf_ptr, (VOID **)&pkt,
                                               next_header, header_len,
                                               IPSEC_IPV6) != NU_SUCCESS)
                        {
                            /* We encountered a problem in IPsec processing,
                             * this packet should be dropped. Note that the
                             * buffer has already been returned to the free
                             * list. So just return an error.
                             */
                            return (NU_INVAL_NEXT_HEADER);
                        }

                        /* If the IPsec header was successfully decoded and
                         * the next header is for an upper-layer protocol
                         */
                        else if ( (IP6_IS_NXTHDR_RECPROT(*next_header)) ||
                                  (*next_header == IPPROTO_IPV6) )
                        {
                            /* This is the upper-layer protocol. */
                            protocol = *next_header;
                        }

                        /* Otherwise, the IPsec header was successfully
                         * decoded, but there are still more extension
                         * headers remaining. At the end of the loop
                         * below, we will move to the next header, but it
                         * is not required in this case since we are already
                         * at the next header.
                         */
                        else
                        {
                            /* Specify no action as we are already at the next
                             * header.
                             */
                            protocol = NU_NO_ACTION;
                        }

                        /* Update the IPv6 header pointer since the header was
                         * moved by the ESP processing. In case for AH,
                         * it will remain the same
                         */
                        *packet = pkt;
                    }

                    break;
#endif

                case IPPROTO_NONEXTHDR:

                    /* The value 59 in the Next Header field of an IPv6 header
                     * or any extension header indicates that there is nothing
                     * following that header.
                     */

                default:

                    /* Extract the next-header value of the final recognized
                     * header.
                     */
                    protocol = buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

                    break;
            }

            if (protocol == NU_INVAL_NEXT_HEADER)
            {
                prev_hdr_len = (UINT16)
                    (8 + (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));

                /* Increment the header length */
                *header_len = (UINT16)(*header_len + prev_hdr_len);

                /* Get a pointer to the Next Header in the packet. */
                *next_header = buf_ptr->data_ptr[IP6_EXTHDR_NEXTHDR_OFFSET];

                /* Increment the data pointer to point to the beginning of
                 * the next protocol header
                 */
                buf_ptr->data_ptr += prev_hdr_len;

                /* Set the offset into the packet to point to this next header
                 * value.
                 */
                if (*icmp_code == -4)
                    *icmp_offset = (*header_len - prev_hdr_len);
            }

            /* The packet was successfully reassembled and pointing to
             * the next extension header in the list of extension headers.
             */
            else if (protocol == NU_NO_ACTION)
            {
                /* Do nothing since we are already pointing to the next
                 * extension header.
                 */
                protocol = NU_INVAL_NEXT_HEADER;
            }
            else
                break;
        }
    }

    /* There are no extension headers in the packet.  Increment the data pointer
     * to the protocol header and set the type of the protocol to return.
     */
    else
        buf_ptr->data_ptr += *header_len;

    buf_ptr->mem_total_data_len -= *header_len;
    buf_ptr->data_len           -= *header_len;

    /* If an error was received indicating that a Hop-By-Hop options header
     * was found in a header other than the IPv6 header, check that this is
     * not an ICMPv6 error message before sending an ICMPv6 error message
     * back.
     */
    if (*icmp_code == ICMP6_PARM_PROB_NEXT_HDR)
    {
        if (*next_header == IP_ICMPV6_PROT)
        {
            icmp_header = (ICMP6_LAYER*)(buf_ptr->data_ptr);

            /* If this is an ICMPv6 error message, do not send an ICMPv6
             * error message in response.
             */
            if (ICMP6_ERROR_MSG(icmp_header->icmp6_type))
                status = NU_INVAL_NEXT_HEADER;
            else
                status = NU_SEND_ICMP_ERROR;
        }
        else
            status = NU_SEND_ICMP_ERROR;
    }

    /* Return the protocol type or NU_INVAL_NEXT_HEADER */
    return (status);

} /* IP6_Process_Extension_Headers */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Process_Routing_Header
*
*   DESCRIPTION
*
*       This function processes the Routing Header of an incoming
*       IPv6 packet.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the buffer.
*       *header_len             The length of the IPv6 header and all
*                               extension headers.
*       *icmp_offset            If an ICMP Error Code is to be transmitted
*                               by the calling routine, the offset into
*                               the original packet where the error
*                               occurred.
*       *icmp_code              If an ICMP Error Code is to be transmitted
*                               by the calling routine, the ICMP Code to
*                               transmit in the packet.
*       **temp_dev              A pointer to the device on which the packet
*                               was received
*
*   OUTPUTS
*
*       NU_SUCCESS              The Routing Header was successfully
*                               parsed.
*       -1                      The packet should be discarded.
*
*************************************************************************/
STATUS IP6_Process_Routing_Header(IP6LAYER *pkt, NET_BUFFER *buf_ptr,
                                  UINT16 *header_len, INT16 *protocol,
                                  UINT8 *next_header, UINT16 prev_hdr_len,
                                  UINT32 *icmp_offset, INT16 *icmp_code,
                                  DV_DEVICE_ENTRY **temp_dev)
{
    STATUS          status = NU_SUCCESS;

    UNUSED_PARAMETER(pkt);
    UNUSED_PARAMETER(protocol);
    UNUSED_PARAMETER(next_header);
    UNUSED_PARAMETER(prev_hdr_len);
    UNUSED_PARAMETER(temp_dev);

    /* If Segments Left is non-zero, discard the packet and send
     * an ICMP Parameter Problem, Code 0, message to the packet's
     * Source Address, pointing to the unrecognized Routing Type.
     */
    if (buf_ptr->data_ptr[IP6_ROUTING_SEGLEFT_OFFSET] != 0)
    {
        status = -1;
        *icmp_code = ICMP6_PARM_PROB_HEADER;
        *icmp_offset = *header_len + IP6_ROUTING_TYPE_OFFSET;
    }

    return (status);

} /* IP6_Process_Routing_Header */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Send
*
*   DESCRIPTION
*
*       This function transmits an IPv6 packet.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the buffer to transmit.
*       *source_address         The IP address of the source node.
*       *dest_address           The IP address of the destination node.
*       hop_limit               The Hop Limit to place in the packet.
*       pkt_type                The type of packet being transmitted.
*       *ro                     A pointer to the route to the destination.
*       flags                   The flags to use for transmission.
*       *multi_options          A pointer to the Multicast Options.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       NU_HOST_UNREACHABLE     A route to the host does not exist.
*       NU_MSGSIZE              The size of the message is wrong.
*       NU_INVALID_ADDRESS      There is no address of matching scope
*                               as the destination address on the
*                               interface associated with the route.
*
*************************************************************************/
STATUS IP6_Send(NET_BUFFER *buf_ptr, IP6_S_OPTIONS *pkt_parms,
                UINT8 pkt_type, RTAB6_ROUTE *ro, INT32 flags,
                const IP6_MULTI_OPTIONS *multi_options)
{
    IP6LAYER            *ip_dgram;
    DV_DEVICE_ENTRY     *int_face;
    RTAB6_ROUTE         iproute;
    SCK6_SOCKADDR_IP    *dest;
    STATUS              status;
    NET_BUFFER          *hdr_buf, *temp_buf;

#if INCLUDE_IP_FORWARDING
    UINT32              icmpv6_mtu;
#endif

#if INCLUDE_IP_RAW
    UINT16              icmpv6_checksum;
#endif

    UINT32              hlen = IP6_HEADER_LEN;  /* This variable will hold the
                                                 * total length of the IPv6 Header.
                                                 */

#if (INCLUDE_IP_MULTICASTING == NU_FALSE)
    UNUSED_PARAMETER(multi_options);
#else
    MULTI_SCK_OPTIONS   *sck_opts = (MULTI_SCK_OPTIONS *)multi_options;
#endif

    /* If a route was not provided, point to the temporary route structure. */
    if (ro == NU_NULL)
    {
        ro = &iproute;
        UTL_Zero((CHAR*)ro, sizeof(RTAB6_ROUTE));
    }

    /* Point to the destination. */
    dest = &ro->rt_ip_dest.rtab6_rt_ip_dest;

    /* If there is a cached route, verify that it is to the same destination
     * and that it is still up. If not, free it and try again.
     */
    if ( (!(flags & IP6_DONTROUTE)) && (ro->rt_route) &&
         ((((ro->rt_route->rt_entry_parms.rt_parm_flags & RT_UP) == 0) ||
         (memcmp(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr, pkt_parms->tx_dest_address,
                 IP6_ADDR_LEN) != 0)) ||
         ((!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_RUNNING)) &&
         (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags & DV_UP)) &&
         (!(ro->rt_route->rt_entry_parms.rt_parm_device->dev_flags2 & DV6_UP)))) )
    {
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);
        ro->rt_route = NU_NULL;
    }

    if (ro->rt_route == NU_NULL)
    {
        dest->sck_family = SK_FAM_IP6;
        dest->sck_len = sizeof(SCK6_SOCKADDR_IP);
        NU_BLOCK_COPY(dest->sck_addr, pkt_parms->tx_dest_address, IP6_ADDR_LEN);
    }

    /* NOTE:  Bypassing routing is not necessary yet, but may be supported in
     * future releases.
     */
#ifdef NOT_SUPPORTED
    /* Check if the caller specified that routing should be bypassed. */
    if (flags & IP_ROUTETOIF)
    {

    }
    else
#endif /* NOT_SUPPORTED */
    {
        /* If a route was not provided, find a route */
        if (ro->rt_route == NU_NULL)
        {
            IP6_Find_Route(ro);

            /* A route could not be found */
            if (ro->rt_route == NU_NULL)
            {
                /* Return host unreachable error.  The only resource allocated in
                 * this function is a route, but we failed to find the route so it
                 * is safe to return here.
                 */
                NLOG_Error_Log("IPv6 packet not sent due to no route",
                               NERR_SEVERE, __FILE__, __LINE__);

                return (NU_HOST_UNREACHABLE);
            }
        }

        int_face = ro->rt_ip_dest.rtab6_rt_device;

        ro->rt_route->rt_entry_parms.rt_parm_use ++;
    }

    /* Is this packet destined for a multicast address */
    if (IPV6_IS_ADDR_MULTICAST(pkt_parms->tx_dest_address))
    {
#if INCLUDE_IP_MULTICASTING

        /* Flag this buffer as containing a multicast packet. */
        buf_ptr->mem_flags |= NET_MCAST;

        /* Did the caller provide any multicast options. */
        if ( (sck_opts != NU_NULL) &&
             (sck_opts->multio_device != NU_NULL) )
            int_face = sck_opts->multio_device;

        /* Confirm that the outgoing interface supports multicast. */
        if ((int_face->dev_flags & DV_MULTICAST) == 0)
        {
            NLOG_Error_Log("IPv6 packet not sent because multicasting is not enabled for this interface",
                           NERR_SEVERE, __FILE__, __LINE__);

            /* Free the route */
            if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && (ro->rt_route) )
                RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);

            return (NU_HOST_UNREACHABLE);
        }

        /* NOTE: When multicastLoop Back and/or multicast routing are
         * supported, this is where it should be done.
         */
#else /* !INCLUDE_IP_MULTICASTING */

        NLOG_Error_Log("IPv6 packet not sent because multicasting is not enabled",
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Free the route */
        if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && (ro->rt_route) )
            RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);

        return (NU_HOST_UNREACHABLE);

#endif /* INCLUDE_IP_MULTICASTING */

    }

    /* If the next hop is a gateway then set the destination ip address to
     * the gateway.
     */
    else if (ro->rt_route->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
        dest = &(ro->rt_route->rt_next_hop);

    /* If an IP header is not already included, get a new buffer for the
     * IP and link-layer headers.
     */
    if (!(buf_ptr->mem_flags & NET_NOHDR))
    {
        /* Allocate a new buffer chain for the link-layer and IP header */
        hdr_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
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
            hdr_buf->data_ptr = hdr_buf->mem_parent_packet + int_face->dev_hdrlen;

            /* Set the total data length of the chain of buffers */
            hdr_buf->mem_total_data_len = buf_ptr->mem_total_data_len;

#if (INCLUDE_IPSEC == NU_TRUE)

            /* Copy the TCP/UDP port pointer. This will be used for
             * IPsec encoding later.
             */
            hdr_buf->mem_port = buf_ptr->mem_port;
#endif
        }

        /* If a buffer could not be allocated, return an error. */
        else
        {
            MIB_ipv6IfStatsOutDiscards_Inc(int_face);

            /* Free the route entry */
            if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && (ro->rt_route) )
                RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);

            return (NU_NO_BUFFERS);
        }
    }

    /* Otherwise, the packet is going out the same interface that it
     * came in, so we can reuse the headers.  The data pointer of the
     * incoming buffer is already set to the IP header.
     */
    else
        hdr_buf = buf_ptr;

    /* Get a pointer to the IP header. */
    ip_dgram = (IP6LAYER*)hdr_buf->data_ptr;

    /* If this packet is not a forwarded packet, fill-in the parameters
     * of the IP header.
     */
    if (!(flags & IP_FORWARDING))
    {
        MIB_ipv6IfStatsOutRequests_Inc(int_face);

        /* Put the traffic class into the packet. */
        PUT8(ip_dgram, IP6_XXX_OFFSET, pkt_parms->tx_traffic_class);

        /* Move the traffic class into the proper position in the packet,
         * zero out the flow label field and add the IPv6 version number to
         * the packet.
         */
        PUT32(ip_dgram, IP6_XXX_OFFSET,
              (IP6_VERSION | ((GET32(ip_dgram, IP6_XXX_OFFSET) >> 4) & 0xfff00000UL)));

        /* Set the hop limit. The hop limit for an interface is obtained
         * from Router Advertisements.  At this point, the outgoing interface
         * has been selected.
         */
        if (pkt_parms->tx_hop_limit == 0)
            ip_dgram->ip6_hop = int_face->dev6_cur_hop_limit;
        else
            ip_dgram->ip6_hop = pkt_parms->tx_hop_limit;

        /* Set the source address. */
        NU_BLOCK_COPY(ip_dgram->ip6_src, pkt_parms->tx_source_address, IP6_ADDR_LEN);

        /* Set the destination address. */
        NU_BLOCK_COPY(ip_dgram->ip6_dest, pkt_parms->tx_dest_address, IP6_ADDR_LEN);

        /* Add the IPv6 extension headers to the packet */
        status = IP6_Build_Extension_Headers(&hdr_buf, &ip_dgram, pkt_type,
                                             pkt_parms, int_face, ro);

        if (status == NU_SUCCESS)
        {
            /* The payload length is the total length of the packet, not
             * including the generic 40 bytes of IPv6 header.  The payload
             * length does include the total length of all Extension Headers.
             */
            PUT16(ip_dgram, IP6_PAYLEN_OFFSET,
                  (UINT16)hdr_buf->mem_total_data_len);
        }

        else
        {
            MIB_ipv6IfStatsOutDiscards_Inc(int_face);

            /* Free the route entry */
            if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && (ro->rt_route) )
                RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);

            /* Free any buffers that were pre-pended. */
            if (!(buf_ptr->mem_flags & NET_NOHDR))
            {
                /* Get a pointer to the first packet in the header chain */
                temp_buf = hdr_buf;

                /* Find the end of the header chain */
                while ((temp_buf->next_buffer != buf_ptr) &&
                       (temp_buf->next_buffer != NU_NULL))
                    temp_buf = temp_buf->next_buffer;

                /* Terminate the header chain of packets */
                temp_buf->next_buffer = NU_NULL;

                /* Free the header chain of packets */
                MEM_One_Buffer_Chain_Free(hdr_buf, hdr_buf->mem_dlist);
            }

            return (status);
        }
    }

    /* If this is a forwarded packet, but IP appended a new buffer for
     * the IP and link-layer headers to the buffer chain, copy the old
     * header into the new buffer.
     */
    else if (!(buf_ptr->mem_flags & NET_NOHDR))
    {
        NU_BLOCK_COPY(hdr_buf->data_ptr, buf_ptr->data_ptr, (unsigned int)hlen);

        /* Increment the buffer pointer to point to the data past the
         * IP header since the buffer prepended will now contain the
         * header.
         */
        buf_ptr->data_ptr += hlen;
    }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    /* Is this packet destined for a multicast address */
    if (buf_ptr->mem_flags & NET_MCAST)
    {
        /* Mark this buffer as containing a multicast packet. */
        hdr_buf->mem_flags |= NET_MCAST;

        if (sck_opts != NU_NULL)
        {
            /* Set the Hop Limit provided by the caller. */
            if (sck_opts->multio_hop_lmt)
                ip_dgram->ip6_hop = sck_opts->multio_hop_lmt;

            /* If a Hop Limit was not provided, set it to the default */
            else if (pkt_parms->tx_hop_limit == 0)
                ip_dgram->ip6_hop = int_face->dev6_cur_hop_limit;

            else
                ip_dgram->ip6_hop = pkt_parms->tx_hop_limit;
        }

        /* If a Hop Limit was not provided in the Multicast Options, set the
         * hop limit.
         */
        else
        {
            /* If a Hop Limit was not provided, set it to the default */
            if (pkt_parms->tx_hop_limit == 0)
                ip_dgram->ip6_hop = int_face->dev6_cur_hop_limit;
            else
                ip_dgram->ip6_hop = pkt_parms->tx_hop_limit;
        }
    }
#endif

    /* Update the length and data ptr for the head buffer. */
    hdr_buf->data_len           += hlen;
    hdr_buf->mem_total_data_len += hlen;

#if (INCLUDE_IP_RAW)

    /* RFC 2292 section 3.1 - The kernel will calculate and insert
     * the ICMPv6 checksum for ICMPv6 raw sockets, since this checksum
     * is mandatory.  For other RAW sockets, the application must set
     * the IPV6_CHECKSUM socket option to have the kernel compute and
     * store a checksum for output.
     */
    if (flags & IP6_CHECKSUM)
    {
        /* Clear out the checksum field */
        PUT16(buf_ptr->data_ptr, (unsigned int)pkt_parms->tx_raw_chk_off, 0);

        /* Compute the RAW checksum */
        icmpv6_checksum = UTL6_Checksum(buf_ptr,
                                        ip_dgram->ip6_src,
                                        ip_dgram->ip6_dest,
                                        buf_ptr->mem_total_data_len,
                                        pkt_type, pkt_type);

        /* Put the checksum in the packet */
        PUT16(buf_ptr->data_ptr, (unsigned int)pkt_parms->tx_raw_chk_off,
              icmpv6_checksum);
    }

#endif

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Check whether IPsec is enabled for this device */
    if (int_face->dev_flags2 & DV_IPSEC_ENABLE)
    {
#if (INCLUDE_IP_FORWARDING == NU_TRUE)

        /* Check the IPsec policy. */
        if (flags & IP_FORWARDING)
        {
#ifdef IPSEC_VERSION_COMP
            /* Get the packet type */
            pkt_type = GET8(hdr_buf->data_ptr, IP6_NEXTHDR_OFFSET);

            /* If IPsec 2.0 is used */
            status = IP_IPSEC_Forward(int_face, &hdr_buf,
                                      (VOID **)&ip_dgram, IPSEC_IPV6,
                                      pkt_type, NU_NULL, NU_NULL);
#else
            /* Otherwise IPsec 1.x is used. */
            status = IP_IPSEC_Forward(int_face, &hdr_buf,
                                      (VOID **)&ip_dgram, IPSEC_IPV6,
                                      pkt_type);
#endif
        }

        else
#endif
        {
            /* Encapsulate the packet via IPsec. */
#ifdef IPSEC_VERSION_COMP
            /* If IPsec 2.0 is used */
            status =
                IP_IPSEC_Send(int_face, &hdr_buf, (VOID **)&ip_dgram,
                              IPSEC_IPV6, pkt_type, NU_NULL, NU_NULL, dest, ro);
#else
            /* Otherwise IPsec 1.x is used. */
            status =
                IP_IPSEC_Send(int_face, &hdr_buf, (VOID **)&ip_dgram,
                              IPSEC_IPV6, pkt_type);
#endif
        }
    }

    else
    {
        /* If IPsec is not enabled for the interface, set status to success. */
        status = NU_SUCCESS;
    }

    /* Ensure IPsec was properly applied. */
    if (status == NU_SUCCESS)

#endif /* (INCLUDE_IPSEC == NU_TRUE) */

    {
        /* If this packet is small enough, send it. */
        if ( (hdr_buf->mem_total_data_len <= ro->rt_route->rt_path_mtu) &&
             (hdr_buf->mem_total_data_len <= int_face->dev6_link_mtu) )
        {
            /* Set the packet type that is in the buffer. */
            hdr_buf->mem_flags |= NET_IP6;

#if (INCLUDE_IP_INFO_LOGGING == NU_TRUE)
            /* Print/store the IP header info */
            NLOG_IP6_Info(ip_dgram, NLOG_TX_PACK);
#endif

            /* Send the packet. */
            status = (*(int_face->dev_output))(hdr_buf, int_face, dest, ro);
        }

        /* This packet must be fragmented. */
        else
        {
#if INCLUDE_IP_FORWARDING

            /* If this IPv6 packet is being forwarded and will be encapsulated
             * by this node, transmit ICMPv6 Packet Too Big Messages accordingly.
             */
            if ( (flags & IP_FORWARDING) && ((int_face->dev_type == DVT_6TO4) ||
                 (int_face->dev_type == DVT_CFG6)) )
            {
                /* If the MTU is less than or equal to the IPv6 minimum link
                 * MTU, set the MTU of the ICMPv6 message to the minimum link
                 * MTU.
                 */
                if (ro->rt_route->rt_path_mtu <= IP6_MIN_LINK_MTU)
                    icmpv6_mtu = IP6_MIN_LINK_MTU;

                /* Otherwise, set the MTU of the ICMPv6 message to the link
                 * MTU of the route.
                 */
                else
                    icmpv6_mtu = ro->rt_route->rt_path_mtu;

                /* Send the Packet Too Big error */
                ICMP6_Send_Error(ip_dgram, buf_ptr, ICMP6_PACKET_TOO_BIG, 0,
                                 icmpv6_mtu, buf_ptr->mem_buf_device);

                status = NU_MSGSIZE;
            }

            else

#endif

#if INCLUDE_IP_FRAGMENT

            /* Only host nodes may fragment packets. */
            if (!(flags & IP_FORWARDING))
            {
                status = IP6_Fragment(hdr_buf, ip_dgram, int_face, dest, ro);

                if (status != NU_SUCCESS)
                {
                    MIB_ipv6IfStatsOutFragFail_Inc(int_face);

                    status = NU_MSGSIZE;
                }

                else
                    MIB_ipv6IfStatsOutFragOKs_Inc(int_face);
            }

            else
                status = NU_MSGSIZE;
#else
            {
                MIB_ipv6IfStatsOutFragFail_Inc(int_face);

                NLOG_Error_Log("IPv6 packet not sent because fragmentation is disabled",
                               NERR_SEVERE, __FILE__, __LINE__);

                status = NU_MSGSIZE;
            }
#endif
        }
    }
#if (INCLUDE_IPSEC == NU_TRUE)
    else if (status == IPSEC_PACKET_SENT)
        status = NU_SUCCESS;
#endif

    /* If the packet could not be transmitted, and the IP layer built
     * a header for the packet, send the header to the dlist.
     */
    if ( (status != NU_SUCCESS) && (!(hdr_buf->mem_flags & NET_NOHDR)) )
    {
        /* Get a pointer to the first packet in the header chain */
        temp_buf = hdr_buf;

        /* Find the end of the header chain */
        while ((temp_buf->next_buffer != buf_ptr) &&
               (temp_buf->next_buffer != NU_NULL))
            temp_buf = temp_buf->next_buffer;

        /* Terminate the header chain of packets */
        temp_buf->next_buffer = NU_NULL;

        /* Free the header chain of packets */
        MEM_One_Buffer_Chain_Free(hdr_buf, hdr_buf->mem_dlist);
    }

    if ( (ro == &iproute) && ((flags & IP_ROUTETOIF) == 0) && (ro->rt_route) )
        RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);

    return (status);

} /* IP6_Send */

/*************************************************************************
*
*   FUNCTION
*
*       IP6_Build_Extension_Headers
*
*   DESCRIPTION
*
*       This function adds IPv6 extension headers to the IPv6 packet.
*
*   INPUTS
*
*       **buffer                A pointer to a pointer to the buffer.
*       **header                A pointer to a pointer to the head of the
*                               IPv6 header.
*       pkt_type                The next header value of the final
*                               next header field.
*       *pkt_parms              A pointer to the extension headers to
*                               add to the packet.
*       *device                 A pointer to the device out which the
*                               packet is being transmitted.
*       *ro                     A pointer to the route being used to
*                               transmit the packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              The extension headers were built
*                               successfully.
*       NU_NO_BUFFERS           The tunnel header could not be created
*                               to tunnel the packet through the Home
*                               Agent.
*       NU_INVAL                There is no Binding Update entry
*                               associated with the Home Agent.
*
*************************************************************************/
STATUS IP6_Build_Extension_Headers(NET_BUFFER **buffer, IP6LAYER **header,
                                   UINT8 pkt_type, IP6_S_OPTIONS *pkt_parms,
                                   DV_DEVICE_ENTRY *device, RTAB6_ROUTE *ro)
{
    NET_BUFFER  *buf_ptr = *buffer;
    IP6LAYER    *ip_dgram = *header;
    INT32       ext_header_len = IP6_HEADER_LEN, current_hdr_len;
    UINT8       *prev_buf_ptr = (UINT8*)&ip_dgram->ip6_next;
    STATUS      status = NU_SUCCESS;

    UNUSED_PARAMETER(device);
    UNUSED_PARAMETER(ro);

    /* If a Hop-By-Hop Options header is present, add it to the header */
    if (pkt_parms->tx_hop_opt)
    {
        /* Set the next header value of the previous header */
        *prev_buf_ptr = IPPROTO_HOPBYHOP;

        /* Increment the header length */
        current_hdr_len =
            (8 + (pkt_parms->tx_hop_opt[IP6_EXTHDR_LENGTH_OFFSET] << 3));

        /* Copy the Hop-By-Hop Options header into the buffer */
        NU_BLOCK_COPY(&buf_ptr->data_ptr[ext_header_len],
                      pkt_parms->tx_hop_opt, (unsigned int)current_hdr_len);

        /* Set a pointer to the next header value of this header */
        prev_buf_ptr =
            (UINT8*)&buf_ptr->data_ptr[ext_header_len +
                                       IP6_EXTHDR_NEXTHDR_OFFSET];

        /* Increment the length of the extension headers */
        ext_header_len += current_hdr_len;
    }

    /* If a Type 0 Routing Header is present, add it to the header.
     * If both a Type 0 and a Type 2 Routing Header are present,
     * the Type 2 Routing Header should follow the other routing header.
     */
    if (pkt_parms->tx_route_hdr)
    {
        /* If a Destination Options header was specified to be added before
         * the Routing Header, add it.
         */
        if (pkt_parms->tx_rthrdest_opt)
        {
            /* Set the next header value of the previous header */
            *prev_buf_ptr = IPPROTO_DEST;

            /* Increment the header length */
            current_hdr_len =
                (8 + (pkt_parms->tx_rthrdest_opt[IP6_EXTHDR_LENGTH_OFFSET] << 3));

            /* Copy the routing header into the buffer */
            NU_BLOCK_COPY(&buf_ptr->data_ptr[ext_header_len], pkt_parms->tx_rthrdest_opt,
                          (unsigned int)current_hdr_len);

            /* Set a pointer to the next header value of this header */
            prev_buf_ptr =
                (UINT8*)&buf_ptr->data_ptr[ext_header_len +
                                           IP6_EXTHDR_NEXTHDR_OFFSET];

            /* Increment the length of the extension headers */
            ext_header_len += current_hdr_len;
        }

        /* Set the next header value of the previous header */
        *prev_buf_ptr = IPPROTO_ROUTING;

        /* Calculate the total length of the routing header */
        current_hdr_len =
            sizeof(struct ip6_rthdr0) +
            (((struct ip6_rthdr0*)pkt_parms->tx_route_hdr)->ip6r0_len << 3);

        /* Copy the routing header into the buffer */
        NU_BLOCK_COPY(&buf_ptr->data_ptr[ext_header_len],
                      pkt_parms->tx_route_hdr, (unsigned int)current_hdr_len);

        /* Set a pointer to the next header value of this header */
        prev_buf_ptr =
            (UINT8*)&(((struct ip6_rthdr0*)&(buf_ptr->data_ptr[ext_header_len]))->ip6r0_nxt);

        /* Increment the length of the extension headers */
        ext_header_len += current_hdr_len;
    }

    /* If a Destination Options header is present, add it to the header */
    if (pkt_parms->tx_dest_opt)
    {
        /* Set the next header value of the previous header */
        *prev_buf_ptr = IPPROTO_DEST;

        /* Increment the header length */
        current_hdr_len =
            (8 + (pkt_parms->tx_dest_opt[IP6_EXTHDR_LENGTH_OFFSET] << 3));

        /* Copy the Destination Options header into the buffer */
        NU_BLOCK_COPY(&buf_ptr->data_ptr[ext_header_len], pkt_parms->tx_dest_opt,
                      (unsigned int)current_hdr_len);

        /* Set a pointer to the next header value of this header */
        prev_buf_ptr =
            (UINT8*)&buf_ptr->data_ptr[ext_header_len +
                                       IP6_EXTHDR_NEXTHDR_OFFSET];

        /* Increment the length of the extension headers */
        ext_header_len += current_hdr_len;
    }

    /* Increment the length of this buffer */
    buf_ptr->data_len += (ext_header_len - IP6_HEADER_LEN);

    /* Increment the total length of the buffer chain */
    (*buffer)->mem_total_data_len += (ext_header_len - IP6_HEADER_LEN);

    /* Set the next header value of the previous header */
    *prev_buf_ptr = pkt_type;

    return (status);

} /* IP6_Build_Extension_Headers */

/*************************************************************************
*
*   FUNCTION
*
*       IP6_Find_Route
*
*   DESCRIPTION
*
*       This function performs Next Hop Determination as described in
*       RFC 4861 section 5.2.  It returns the route associated with the
*       next hop to the destination.
*
*       Next-Hop Determination for a given unicast destination operates
*       as follows.  The sender performs a longest Prefix Match against
*       the Prefix List to determine whether the packet's destination is
*       on or off-link.  If the destination is on-link, the next-hop
*       address is the same as the packet's destination address.
*       Otherwise, the sender selects a router from the Default Router
*       List.
*
*   INPUTS
*
*       *ro                     A pointer to the routing data structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP6_Find_Route(RTAB6_ROUTE *ro)
{
    RTAB6_ROUTE_ENTRY   *rt_entry;

    rt_entry =
        RTAB6_Next_Hop_Determination(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr);

    /* Multicast addresses do not require a next-hop */
    if (IPV6_IS_ADDR_MULTICAST(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr))
    {
        if (rt_entry == NU_NULL)
        {
            if (RTAB6_Add_Route(DEV6_Get_Primary_Interface(),
                                ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                                NU_NULL, 128,
                                (RT_UP | RT_GATEWAY | RT_LOCAL | RT_HOST |
                                 RT_SILENT)) == NU_SUCCESS)
                rt_entry = RTAB6_Find_Route(ro->rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                                            RT_HOST_MATCH);
        }

        if (rt_entry)
        {
            /* Set the route to point to the route */
            ro->rt_route = rt_entry;

            ro->rt_ip_dest.rtab6_rt_device = rt_entry->rt_entry_parms.rt_parm_device;
        }
        else
            ro->rt_route = NU_NULL;
    }

    else if (rt_entry)
    {
        /* Set the route to point to the route entry */
        ro->rt_route = rt_entry;

        /* If the route is a GATEWAY route, set the next-hop to the
         * next-hop specified in the route.
         */
        if (ro->rt_route->rt_entry_parms.rt_parm_flags & (RT_GATEWAY | RT_HOST))
        {
            if (ro->rt_route->rt_next_hop_entry)
            {
                ro->rt_ip_dest.rtab6_rt_device =
                    ro->rt_route->rt_next_hop_entry->ip6_neigh_cache_device;
            }
            else
                ro->rt_route = NU_NULL;
        }
        else
            ro->rt_ip_dest.rtab6_rt_device =
                rt_entry->rt_entry_parms.rt_parm_device;
    }

    /* There is no route to this destination. */
    else
    {
        ro->rt_route = NU_NULL;
    }

} /* IP6_Find_Route */

/*************************************************************************
*
*   FUNCTION
*
*       IP6_Find_Link_Local_Addr
*
*   DESCRIPTION
*
*       This function returns the link-local address associated with
*       the given device.
*
*   INPUTS
*
*       *device                 A pointer to the device.
*
*   OUTPUTS
*
*       A pointer to the link-local address or NU_NULL if no link-local
*       address exists.
*
*************************************************************************/
DEV6_IF_ADDRESS *IP6_Find_Link_Local_Addr(const DV_DEVICE_ENTRY *device)
{
    DEV6_IF_ADDRESS *dev_addr = NU_NULL;

    if (device)
    {
        /* Get the first address in the list of addresses for the device */
        dev_addr = device->dev6_addr_list.dv_head;

        /* Search through the list of addresses for the link-local address. */
        while (dev_addr)
        {
            if ( (IPV6_IS_ADDR_LINKLOCAL(dev_addr->dev6_ip_addr)) &&
                 (!(dev_addr->dev6_addr_state & DV6_DETACHED)) &&
                 (!(dev_addr->dev6_addr_state & DV6_ANYCAST)) &&
                 (!(dev_addr->dev6_addr_state & DV6_DUPLICATED)) &&
                 (!(dev_addr->dev6_addr_state & DV6_TENTATIVE)) )
                break;

            dev_addr = dev_addr->dev6_next;
        }
    }

    return (dev_addr);

} /* IP6_Find_Link_Local_Addr */

/*************************************************************************
*
*   FUNCTION
*
*       IP6_Find_Global_Addr
*
*   DESCRIPTION
*
*       This function returns a globally routable unicast address
*       associated with the given device.
*
*   INPUTS
*
*       *device                 A pointer to the device.
*
*   OUTPUTS
*
*       A pointer to the address or NU_NULL if no address exists.
*
*************************************************************************/
DEV6_IF_ADDRESS *IP6_Find_Global_Addr(const DV_DEVICE_ENTRY *device)
{
    DEV6_IF_ADDRESS *dev_addr = NU_NULL;

    if (device)
    {
        /* Get the first address in the list of addresses for the device */
        dev_addr = device->dev6_addr_list.dv_head;

        /* Search through the list of addresses for a global Care-of
         * Address.
         */
        while (dev_addr)
        {
            /* If this is a valid globally routable unicast address,
             * return it.
             */
            if ( (!(dev_addr->dev6_addr_state & DV6_DETACHED)) &&
                 (!(dev_addr->dev6_addr_state & DV6_ANYCAST)) &&
                 (!(dev_addr->dev6_addr_state & DV6_DUPLICATED)) &&
                 (!(dev_addr->dev6_addr_state & DV6_TENTATIVE)) &&
                 (in6_addrscope(dev_addr->dev6_ip_addr) == IPV6_ADDR_SCOPE_GLOBAL) )
                break;

            dev_addr = dev_addr->dev6_next;
        }
    }

    return (dev_addr);

} /* IP6_Find_Global_Addr */

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Handle_Solicited_Node_Multi_Event
*
*   DESCRIPTION
*
*       This function joins the Solicited-Node multicast address group
*       associated with the flagged IP address on the interface.
*
*   INPUTS
*
*       event                   The event being handled.
*       dev_index               The index of the interface on which the
*                               address resides.
*       flags                   Flags associated with the address.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP6_Handle_Solicited_Node_Multi_Event(TQ_EVENT event, UNSIGNED dev_index,
                                           UNSIGNED flags)
{
#if (INCLUDE_DAD6 == NU_TRUE)
    UINT32              tick = 0;
#endif
    DV_DEVICE_ENTRY     *device;
    DEV6_IF_ADDRESS     *target_addr;
    IP6_MULTI           *ipm6;
    UINT8               dev_addr[IP6_ADDR_LEN];

    UNUSED_PARAMETER(event);

    /* Get a pointer to the device associated with the device index */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {
        if (device->dev_flags & DV6_IPV6)
        {
            /* Get a pointer to the first IP address entry in the list */
            target_addr = device->dev6_addr_list.dv_head;

            /* Traverse the list of IP addresses searching for the IP address
             * that has been flagged.
             */
            while (target_addr)
            {
                /* If this is the target entry. */
                if (target_addr->dev6_addr_flags & ADDR6_DELAY_MLD)
                {
                    /* Create the Solicited-Node Multicast address for the link-local
                     * address.
                     */
                    memcpy(dev_addr, IP6_Solicited_Node_Multi, IP6_ADDR_LEN);

                    dev_addr[13] = target_addr->dev6_ip_addr[13];
                    dev_addr[14] = target_addr->dev6_ip_addr[14];
                    dev_addr[15] = target_addr->dev6_ip_addr[15];

                    /* Find the multicast membership for the solicited-node
                     * multicast address.
                     */
                    ipm6 = IP6_Lookup_Multi(dev_addr,
                                            target_addr->dev6_device->dev6_multiaddrs);

                    if (ipm6)
                    {
                        /* Inform MLD of the new group membership. */
                        MLD6_Start_Listening(ipm6, (UINT8)ipm6->ipm6_msg_to_send);

                        /* Remove the flag. */
                        target_addr->dev6_addr_flags &= ~ADDR6_DELAY_MLD;
                    }

                    break;
                }

                /* Get the next address in the list */
                target_addr = target_addr->dev6_next;
            }

            /* If an address was found. */
            if (target_addr)
            {
#if (INCLUDE_DAD6 == NU_TRUE)
                /* If DAD is not disabled for the address. */
                if (!(target_addr->dev6_addr_flags & ADDR6_NO_DAD))
                {
                    /* Invoke DAD */
                    nd6_dad_start(target_addr,
                                  target_addr->dev6_device->dev6_dup_addr_detect_trans,
                                  &tick);

                    /* Set the timer to stop DAD.  Since this address could be using
                     * the same interface identifier as a link-local address undergoing DAD,
                     * still set the timer to stop DAD, even if DAD is disabled for the
                     * address.
                     */
                    if (TQ_Timerset(IP6_Verify_Valid_Addr_Event, target_addr->dev6_id,
                            target_addr->dev6_device->dev6_retrans_timer, 0) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to set the timer to verify the address",
                                       NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Since DAD is disabled for the address, do not delay completing
                 * autoconfiguration.
                 */
                else
#endif /* INCLUDE_DAD6 == NU_TRUE) */
                {
                    DEV6_Verify_Valid_Addr(IP6_Verify_Valid_Addr_Event,
                                           target_addr->dev6_id, 0);
                }
            }
        }
    }

} /* IP6_Handle_Solicited_Node_Multi_Event */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Join_Solicited_Node_Multi
*
*   DESCRIPTION
*
*       This function joins the Solicited-Node multicast address group
*       associated with the passed in IP address.
*
*   INPUTS
*
*       *device                 A pointer to the device on which to
*                               join the Solicited Node Multicast group.
*       *ip_address             A pointer to the IP address to join for
*                               the Solicited Node Multicast group.
*       flags                   If ADDR6_DELAY_MLD is set in the flags,
*                               delay sending the MLD report.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP6_Join_Solicited_Node_Multi(DV_DEVICE_ENTRY *device,
                                   const UINT8 *ip_address, UINT32 flags)
{
    UINT8   dev_addr[IP6_ADDR_LEN];

    /* Create the Solicited-Node Multicast address for the link-local
     * address.
     */
    memcpy(dev_addr, IP6_Solicited_Node_Multi, IP6_ADDR_LEN);

    dev_addr[13] = ip_address[13];
    dev_addr[14] = ip_address[14];
    dev_addr[15] = ip_address[15];

    /* If the flag is not set to delay sending the MLD report, join the
     * group and send the MLD report.
     */
    if (!(flags & ADDR6_DELAY_MLD))
    {
        if (IP6_Add_Multi(dev_addr, device, NU_NULL) == NU_NULL)
            NLOG_Error_Log("Failed to add the IPv6 multicast address",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* RFC 4862 - section 5.4.2 - In order to improve the robustness of
     * the Duplicate Address Detection algorithm, an interface MUST
     * receive and process datagrams sent to the all-nodes multicast
     * address or solicited-node multicast address of the tentative
     * address during the delay period. This does not necessarily
     * conflict with the requirement that joining the multicast group
     * be delayed. ... it is possible for a node to start listening to
     * the group during the delay period before MLD report transmission.
     */
    else
    {
        if (IP6_Allocate_Multi(dev_addr, device, NU_NULL) == NU_NULL)
            NLOG_Error_Log("Failed to add the IPv6 multicast address",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

} /* IP6_Join_Solicited_Node_Multi */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Leave_Solicited_Node_Multi
*
*   DESCRIPTION
*
*       This function leaves the Solicited-Node multicast address group
*       associated with the passed in IP address.
*
*   INPUTS
*
*       *device                 A pointer to the device on which to
*                               join the Solicited Node Multicast group.
*       *ip_address             A pointer to the IP address to join for
*                               the Solicited Node Multicast group.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP6_Leave_Solicited_Node_Multi(DV_DEVICE_ENTRY *device,
                                    const UINT8 *ip_address)
{
    UINT8       dev_addr[IP6_ADDR_LEN];
    IP6_MULTI   *multi_addr;

    /* Create the Solicited-Node Multicast address for the link-local
     * address.
     */
    memcpy(dev_addr, IP6_Solicited_Node_Multi, IP6_ADDR_LEN);

    dev_addr[13] = ip_address[13];
    dev_addr[14] = ip_address[14];
    dev_addr[15] = ip_address[15];

    /* Find the solicited-node multicast address associated with
     * the IPv6 address and leave the multicast group.
     */
    multi_addr = device->dev6_multiaddrs;

    while (multi_addr)
    {
        if (memcmp(multi_addr->ipm6_addr, dev_addr, IP6_ADDR_LEN) == 0)
        {
            if (IP6_Delete_Multi(multi_addr) == NU_NULL)
                NLOG_Error_Log("Failed to leave the IPv6 multicast group",
                               NERR_SEVERE, __FILE__, __LINE__);
            break;
        }

        multi_addr = multi_addr->ipm6_next;
    }

} /* IP6_Leave_Solicited_Node_Multi */

#endif

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Create_IPv4_Mapped_Addr
*
*   DESCRIPTION
*
*       This function creates an IPv6 IPv4-Mapped address out of an IPv4
*       address.
*
*   INPUTS
*
*       *ipv6_addr              A pointer to the memory into which to
*                               put the new IPv6 IPv4-Mapped address
*       ipv4_addr               The 32-bit IPv4 address
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP6_Create_IPv4_Mapped_Addr(UINT8 *ipv6_addr, UINT32 ipv4_addr)
{
    /* Zero out the memory */
    UTL_Zero(ipv6_addr, IP6_ADDR_LEN);

    /* Set bytes 11 and 12 to 0xff */
    ipv6_addr[10] = 0xff;
    ipv6_addr[11] = 0xff;

    /* Bytes 13-16 hold the IPv4 address */
    PUT32(&ipv6_addr[12], 0, ipv4_addr);

} /* IP6_Create_IPv4_Mapped_Addr */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Setup_Options
*
*   DESCRIPTION
*
*       This function determines the source address and hop limit to
*       use for a packet.  If ancillary data was set on the socket,
*       the source address and/or hop limit associated with it is used.
*       Otherwise, if a sticky option was set on the socket, the source
*       address and/or hop limit associated with it is used.  Otherwise,
*       an address of appropriate scope is used and the hop limit
*       associated with the socket is used.
*
*   INPUTS
*
*       *ancillary_data         A pointer to the ancillary data
*                               associated with the port.
*       *sticky_options         A pointer to the sticky options
*                               associated with the port.
*       protocol                The type of socket.
*       *ip6_options            A pointer to the data structure to
*                               fill in.
*       *faddr                  A pointer to the foreign side of the
*                               communications.
*       *cached_rt              A pointer to the cached route.
*       hop_limit               The Hop Limit associated with the
*                               port.
*
*   OUTPUTS
*
*       NU_SUCCESS              The structure was successfully set up.
*       NU_INVALID_SOCKET       The socket pointer is NULL.
*       NU_INVALID_ADDRESS      The address specified does not exist on
*                               the node.
*       NU_NO_ROUTE_TO_HOST     A route could not be found to the host.
*
*************************************************************************/
STATUS IP6_Setup_Options(tx_ancillary_data *ancillary_data,
                         tx_ancillary_data *sticky_options,
                         IP6_S_OPTIONS *ip6_options,
                         UINT8 *faddr, RTAB6_ROUTE *cached_rt,
                         UINT8 hop_limit)
{
    UINT8               *next_hop;
    DV_DEVICE_ENTRY     *dev_ptr;

    /* If there are no sticky options and no ancillary data, use the default
     * values and return.
     */
    if ( (!(ancillary_data)) && (!(sticky_options)) )
    {
        /* Use the cached route */
        ip6_options->tx_route.rt_route = cached_rt->rt_route;
        ip6_options->tx_route.rt_ip_dest = cached_rt->rt_ip_dest;

        /* Set the Hop Limit */
        ip6_options->tx_hop_limit = hop_limit;

        /* Set the traffic class */
        ip6_options->tx_traffic_class = IP6_TCLASS_DEFAULT;
    }

    /* Otherwise, there is ancillary data or at least one sticky option has
     * been set.  Determine which it is and fill in the values.
     */
    else
    {
        /******* FILL IN THE ROUTE *******/

        /* If a sticky option was set, set the cached route to the specifications
         * in the sticky option so the sticky option values do not have to
         * be checked each time data is sent.
         */
        if ( (sticky_options) &&
             ((sticky_options->tx_interface_index) ||
              (sticky_options->tx_next_hop)) )
        {
            /* If the sticky options for the route have not changed, initialize
             * dev_ptr and next_hop according to the cached route.
             */
            if (!(sticky_options->tx_flags & IP6_STCKY_RT_CHNGD))
            {
                dev_ptr = cached_rt->rt_ip_dest.rtab6_rt_device;
                next_hop = cached_rt->rt_route->rt_next_hop.sck_addr;
            }

            /* The sticky options pertaining to the route have changed,
             * so find a new cached route based on the new sticky options.
             */
            else
            {
                /* Unset the route change flag */
                sticky_options->tx_flags &= ~IP6_STCKY_RT_CHNGD;

                /* If an interface index was specified as a Sticky Option */
                if (sticky_options->tx_interface_index)
                    dev_ptr =
                        DEV_Get_Dev_By_Index(*(UINT32*)sticky_options->tx_interface_index);
                else
                    dev_ptr = NU_NULL;

                /* Otherwise, if a next-hop was specified as a Sticky Option */
                if (sticky_options->tx_next_hop)
                    next_hop = sticky_options->tx_next_hop->id.is_ip_addrs;
                else
                    next_hop = NU_NULL;

                /* If the cached route does not match the parameters set by the
                 * sticky option, find a route that does match.
                 */
                if ( (cached_rt->rt_ip_dest.rtab6_rt_device != dev_ptr) ||
                     ((!next_hop) ||
                      (memcmp(cached_rt->rt_route->rt_next_hop.sck_addr,
                              next_hop, IP6_ADDR_LEN) != 0)) )
                {
                    /* Try to find a route based on the new sticky options. */
                    if (IP6_Setup_Route(next_hop, dev_ptr, cached_rt,
                                        ip6_options, faddr) == NU_SUCCESS)
                    {
                        /* If a route was found, set the cached route to the
                         * new route.
                         */
                        cached_rt->rt_ip_dest = ip6_options->tx_route.rt_ip_dest;
                        cached_rt->rt_route = ip6_options->tx_route.rt_route;
                    }

                    /* Otherwise, log an error */
                    else
                        NLOG_Error_Log("Failed to find route based on sticky options",
                                       NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        /* Otherwise, set the device and next-hop to NULL and find a route based
         * on the ancillary data.
         */
        else
        {
            dev_ptr = NU_NULL;
            next_hop = NU_NULL;
        }

        /* If an interface index or next-hop address was specified via ancillary
         * data, find a corresponding route to the destination.
         */
        if ( (ancillary_data) &&
             ((ancillary_data->tx_interface_index) ||
              (ancillary_data->tx_next_hop)) )
        {
            /* If an interface index was specified as ancillary data*/
            if (ancillary_data->tx_interface_index)
            {
                /* Get a pointer to the device referenced by the interface_index */
                dev_ptr =
                    DEV_Get_Dev_By_Index(*(UINT32*)ancillary_data->tx_interface_index);
            }

            /* If a next-hop was specified as ancillary data */
            if (ancillary_data->tx_next_hop)
                next_hop = ancillary_data->tx_next_hop->id.is_ip_addrs;

            /* Find a route based on the ancillary data and/or the ancillary data /
             * sticky options combination.
             */
            if (IP6_Setup_Route(next_hop, dev_ptr, cached_rt, ip6_options,
                                faddr) != NU_SUCCESS)
                NLOG_Error_Log("Failed to find route based on ancillary data",
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Otherwise, use the interface associated with the cached route */
        else
        {
            /* Set the route pointer to the cached route */
            ip6_options->tx_route.rt_route = cached_rt->rt_route;
            ip6_options->tx_route.rt_ip_dest = cached_rt->rt_ip_dest;
        }

        /* Check that a valid route was found */
        if (!ip6_options->tx_route.rt_route)
            return (NU_NO_ROUTE_TO_HOST);

        /******* FILL IN THE HOP LIMIT *******/

        /* If a Hop Limit was specified as Ancillary data, use it for this
         * datagram.
         */
        if ( (ancillary_data) && (ancillary_data->tx_hop_limit) )
             ip6_options->tx_hop_limit = *ancillary_data->tx_hop_limit;

        /* Otherwise, use the Hop Limit on the socket */
        else
            ip6_options->tx_hop_limit = hop_limit;

        /******* FILL IN THE TRAFFIC CLASS *******/

        /* If a Traffic Class was specified as Ancillary data, use it for this
         * datagram.
         */
        if ( (ancillary_data) && (ancillary_data->tx_traffic_class) )
            ip6_options->tx_traffic_class = *ancillary_data->tx_traffic_class;

        /* Otherwise, check if a Traffic Class was specified as a Sticky Option */
        else if ( (sticky_options) && (sticky_options->tx_traffic_class) )
            ip6_options->tx_traffic_class = *sticky_options->tx_traffic_class;

        /* Else, use the Kernel default */
        else
            ip6_options->tx_traffic_class = IP6_TCLASS_DEFAULT;

        /******* FILL IN THE ROUTING HEADER *******/

        /* If a Routing Header was specified as Ancillary data, use it for
         * this datagram.
         */
        if ( (ancillary_data) && (ancillary_data->tx_route_header) )
        {
            /* The application could have specified a Routing Header with length
             * of zero to override a Routing Header set as a Sticky Option.
             * If the length of the Routing Header Extension Header is zero, do
             * not set a Routing Header Extension Header for the packet.
             */
            if ((((struct ip6_rthdr0*)ancillary_data->tx_route_header)->ip6r0_len << 3) != 0)
                ip6_options->tx_route_hdr = ancillary_data->tx_route_header;
            else
                ip6_options->tx_route_hdr = NU_NULL;
        }

        /* Otherwise, check if a Routing Header was specified as a Sticky Option */
        else if ( (sticky_options) && (sticky_options->tx_route_header) )
            ip6_options->tx_route_hdr = sticky_options->tx_route_header;

        else
            ip6_options->tx_route_hdr = NU_NULL;

        /******* FILL IN THE HOP-BY-HOP OPTIONS *******/

        /* If a hop-by-hop options header was specified as Ancillary data, use
         * it for this datagram.
         */
        if ( (ancillary_data) && (ancillary_data->tx_hop_opt) )
        {
            /* The application could have specified a Hop-By-Hop option with length
             * of zero to override a Hop-By-Hop Option set as a Sticky Option.
             * If the length of the Hop-By-Hop Extension Header is zero, do
             * not set a Hop-By-Hop Extension Header for the packet.
             */
            if ((ancillary_data->tx_hop_opt[IP6_EXTHDR_LENGTH_OFFSET] << 3) != 0)
                ip6_options->tx_hop_opt = ancillary_data->tx_hop_opt;
            else
                ip6_options->tx_hop_opt = NU_NULL;
        }

        /* Otherwise, check if a hop-by-hop options header was specified as a
         * Sticky Option
         */
        else if ( (sticky_options) && (sticky_options->tx_hop_opt) )
            ip6_options->tx_hop_opt = sticky_options->tx_hop_opt;

        else
            ip6_options->tx_hop_opt = NU_NULL;

        /******* FILL IN THE DESTINATION OPTIONS *******/

        /* If a destination options header was specified as Ancillary data, use
         * it for this datagram.
         */
        if ( (ancillary_data) && (ancillary_data->tx_dest_opt) )
        {
            /* The application could have specified a Destination option with length
             * of zero to override a Destination Option set as a Sticky Option.
             * If the length of the Destination Option Extension Header is zero, do
             * not set a Destination Option Extension Header for the packet.
             */
            if ((ancillary_data->tx_dest_opt[IP6_EXTHDR_LENGTH_OFFSET] << 3) != 0)
                ip6_options->tx_dest_opt = ancillary_data->tx_dest_opt;
            else
                ip6_options->tx_dest_opt = NU_NULL;
        }

        /* Otherwise, check if a destination options header was specified as a
         * Sticky Option
         */
        else if ( (sticky_options) && (sticky_options->tx_dest_opt) )
            ip6_options->tx_dest_opt = sticky_options->tx_dest_opt;

        else
            ip6_options->tx_dest_opt = NU_NULL;

        /******* FILL IN THE DESTINATION OPTIONS TO PRECEDE THE ROUTING HEADER *******/

        /* If a destination options header was specified as Ancillary data, use
         * it for this datagram.
         */
        if ( (ancillary_data) && (ancillary_data->tx_rthrdest_opt) )
        {
            /* The application could have specified a Destination option with length
             * of zero to override a Destination Option set as a Sticky Option.
             * If the length of the Destination Option Extension Header is zero, do
             * not set a Destination Option Extension Header for the packet.
             */
            if ((ancillary_data->tx_rthrdest_opt[IP6_EXTHDR_LENGTH_OFFSET] << 3) != 0)
                ip6_options->tx_rthrdest_opt = ancillary_data->tx_rthrdest_opt;
            else
                ip6_options->tx_rthrdest_opt = NU_NULL;
        }

        /* Otherwise, check if a destination options header was specified as a
         * Sticky Option
         */
        else if ( (sticky_options) && (sticky_options->tx_rthrdest_opt) )
            ip6_options->tx_rthrdest_opt = sticky_options->tx_rthrdest_opt;

        else
            ip6_options->tx_rthrdest_opt = NU_NULL;
    }

    return (NU_SUCCESS);

} /* IP6_Setup_Options */

/***********************************************************************
*
*   FUNCTION
*
*       IP6_Setup_Route
*
*   DESCRIPTION
*
*       This function determines the source address and hop limit to
*       use for a packet.  If ancillary data was set on the socket,
*       the source address and/or hop limit associated with it is used.
*       Otherwise, if a sticky option was set on the socket, the source
*       address and/or hop limit associated with it is used.  Otherwise,
*       an address of appropriate scope is used and the hop limit
*       associated with the socket is used.
*
*   INPUTS
*
*       *next_hop               A pointer to the next-hop to use for
*                               the route.
*       *dev_ptr                A pointer to the device to use for
*                               the route.
*       *cached_rt              A pointer to the cached route on the
*                               socket.
*       *ip6_options            A pointer to the data structure to
*                               fill in.
*       *faddr                  A pointer to the foreign side of the
*                               communications.
*
*   OUTPUTS
*
*       NU_SUCCESS              The structure was successfully set up.
*       NU_NO_ROUTE_TO_HOST     A route could not be found to the host.
*
*************************************************************************/
STATUS IP6_Setup_Route(const UINT8 *next_hop, DV_DEVICE_ENTRY *dev_ptr,
                       const RTAB6_ROUTE *cached_rt,
                       IP6_S_OPTIONS *ip6_options, UINT8 *faddr)
{
    RTAB6_ROUTE     ro;
    UINT32          flags;
    DV_DEVICE_ENTRY *temp_dev;
    STATUS          status = NU_SUCCESS;

    /* If a next-hop was specified, find or add a route to the
     * destination using the next-hop.
     */
    if (next_hop)
    {
        /* If the cached route for this socket already uses the next-hop
         * specified, set the pointer to the cached route.
         */
        if (memcmp(cached_rt->rt_route->rt_next_hop.sck_addr, next_hop,
                   IP6_ADDR_LEN) == 0)
        {
            /* If no interface index was specified or the interface index
             * of the cached route is the same as the interface index
             * specified, use the cached route as the route.
             */
            if ( (!dev_ptr) ||
                 (cached_rt->rt_route->rt_entry_parms.rt_parm_device->dev_index ==
                  dev_ptr->dev_index) )
            {
                ip6_options->tx_route.rt_route = cached_rt->rt_route;
                ip6_options->tx_route.rt_ip_dest = cached_rt->rt_ip_dest;
            }
        }

        /* If a route was not assigned */
        if (!ip6_options->tx_route.rt_route)
        {
            /* Find a route to the destination through the gateway */
            ip6_options->tx_route.rt_route =
                (RTAB6_ROUTE_ENTRY*)RTAB6_Find_Route_By_Gateway(faddr,
                                                                next_hop, 0);

            /* If an interface index was specified, check that the route found
             * uses that interface.
             */
            if (dev_ptr)
            {
                /* If the route does not use the interface, reset the route to NULL */
                if ( (ip6_options->tx_route.rt_route) &&
                     (ip6_options->tx_route.rt_route->rt_entry_parms.rt_parm_device->dev_index !=
                      dev_ptr->dev_index) )
                    ip6_options->tx_route.rt_route = NU_NULL;
                else
                    ip6_options->tx_route.rt_ip_dest.rtab6_rt_device = dev_ptr;
            }

            /* If a route has not been found, a new one must be added */
            if (!ip6_options->tx_route.rt_route)
            {
                /* Check that a route to the next-hop exists */
                NU_BLOCK_COPY(ro.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                              next_hop, IP6_ADDR_LEN);

                ro.rt_ip_dest.rtab6_rt_ip_dest.sck_family = SK_FAM_IP6;

                IP6_Find_Route(&ro);

                /* If a route to the gateway exists */
                if (ro.rt_route)
                {
                    RTAB_Free((ROUTE_ENTRY*)ro.rt_route, NU_FAMILY_IP6);

                    flags = (RT_UP | RT_LOCAL | RT_HOST);

                    /* If the gateway is not a directly connected interface, flag
                     * the route as a Gateway route.
                     */
                    temp_dev = DEV6_Get_Dev_By_Addr(next_hop);

                    /* If the gateway is a node other than this node */
                    if (temp_dev == NU_NULL)
                    {
                        /* If a device was not specified, use the device associated
                         * with the route to the gateway.
                         */
                        if (!dev_ptr)
                            dev_ptr = ro.rt_route->rt_entry_parms.rt_parm_device;

                        flags |= RT_GATEWAY;
                    }

                    else if (!dev_ptr)
                        dev_ptr = temp_dev;

                    /* Create the host route through the specified device */
                    RTAB6_Add_Route(dev_ptr, faddr, next_hop, 128, flags);

                    /* Find the route to the destination just added */
                    ip6_options->tx_route.rt_route =
                        (RTAB6_ROUTE_ENTRY*)RTAB6_Find_Route_By_Gateway(faddr,
                                                                        next_hop,
                                                                        RT_HOST);
                }

                /* Otherwise, there is no route to the gateway; therefore,
                 * return an error.
                 */
                else
                    status = NU_NO_ROUTE_TO_HOST;
            }

            else if (!dev_ptr)
                dev_ptr = ip6_options->tx_route.rt_route->rt_entry_parms.rt_parm_device;
        }
    }

    /* If an interface index was specified, and a route was not already
     * found above, find a route to the destination using the specified
     * interface.
     */
    else if (dev_ptr)
    {
        /* If the cached route for this socket already uses the
         * interface specified, set the pointer to the cached route.
         */
        if ( (cached_rt->rt_route) &&
             (cached_rt->rt_ip_dest.rtab6_rt_device == dev_ptr) )
        {
            ip6_options->tx_route.rt_route = cached_rt->rt_route;
            ip6_options->tx_route.rt_ip_dest = cached_rt->rt_ip_dest;
        }

        /* Otherwise, find a route to the destination using the
         * specified interface.
         */
        else
        {
            ip6_options->tx_route.rt_route =
                (RTAB6_ROUTE_ENTRY*)
                RTAB6_Find_Route_By_Device(faddr, dev_ptr);

            if (ip6_options->tx_route.rt_route)
                next_hop = ip6_options->tx_route.rt_route->rt_next_hop.sck_addr;
        }
    }

    /* If a route was found using the ancillary data provided, fill in
     * the rtab6_rt_ip_dest structure in the route entry.
     */
    if (ip6_options->tx_route.rt_route != cached_rt->rt_route)
    {
        if (ip6_options->tx_route.rt_route)
        {
            ip6_options->tx_route.rt_ip_dest.rtab6_rt_device = dev_ptr;

            /* If the route has the gateway flag set, copy the next-hop into
             * the structure.  Otherwise, the destination is the next-hop.
             */
            if ( (next_hop) && (cached_rt->rt_route) &&
                 (cached_rt->rt_route->rt_entry_parms.rt_parm_flags & RT_GATEWAY) )
                NU_BLOCK_COPY(ip6_options->tx_route.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                              next_hop, IP6_ADDR_LEN);
            else
                NU_BLOCK_COPY(ip6_options->tx_route.rt_ip_dest.rtab6_rt_ip_dest.sck_addr,
                              faddr, IP6_ADDR_LEN);

            ip6_options->tx_route.rt_ip_dest.rtab6_rt_ip_dest.sck_family =
                SK_FAM_IP6;
        }

        /* Otherwise, return an error */
        else
            status = NU_NO_ROUTE_TO_HOST;
    }

    return (status);

} /* IP6_Setup_Route */


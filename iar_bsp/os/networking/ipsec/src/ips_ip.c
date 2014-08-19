/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_ip.c
*
* COMPONENT
*
*       IPSEC - IP Processing
*
*   DESCRIPTION
*
*       This file contains functions that are specific to IP (v4 as well
*       as v6) and IPsec.
*
*   DATA STRUCTURES
*
*       IPSEC_SA_Count
*       *IPSEC_In_Bundle
*       IPSEC_Pkt_Selector
*
*   FUNCTIONS
*
*       IP_IPSEC_Interpret
*       IP_IPSEC_Send
*       IP_IPSEC_Secure_Packet
*       IP_IPSEC_Pkt_Selector
*       IP_IPSEC_Forward
*       IP4_IPS_Tunnel
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"
#endif

#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/* The following two variables are uses in conjunction with each other to
 * provide information about the security associations that have been
 * applied to the incoming packet. At any time there is only one packet
 * being parsed in NET stack. These variables provide the SA's that have
 * been applied to this point on the packet being parsed.
 */
UINT8                IPSEC_SA_Count;
IPSEC_INBOUND_SA     *IPSEC_In_Bundle[IPSEC_MAX_SA_BUNDLE_SIZE];

/* Packet selector used to match policies and security associations
 * with the incoming packet.
 */
IPSEC_SELECTOR       IPSEC_Pkt_Selector;

/*  This is the id field of outgoing IP packets. */
extern INT16                IP_Ident;

STATIC STATUS IP_IPSEC_Secure_Packet (
                NET_BUFFER **buf_ptr,
                VOID **pkt, UINT8 ver, IPSEC_POLICY *policy_ptr,
                IPSEC_OUTBOUND_BUNDLE *out_bundle,
                IPSEC_SELECTOR *pkt_selector,
                UINT8 *tunnel_src_ip4, UINT8 *tunnel_src_ip6);

/*************************************************************************
*
*   FUNCTION
*
*       IP_IPSEC_Interpret
*
*   DESCRIPTION
*
*       Called by the IP Layer to interpret IPsec headers.
*
*   INPUTS
*
*       *device                 A pointer to the interface on which the
*                               packet was received.
*       *buf_ptr                A pointer to the net buffer structure
*                               that is pointing to the higher layer
*                               protocol.
*       **pkt                   A pointer to the IPv4 or IPv6 packet. Note
*                               that after processing this pointer may be
*                               updated.
*       *next_header            Pointer to the next protocol header.
*       *header_len             Length of the header.
*       ver                     Version of the IP header (v4 or v6).
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       IPSEC_NOT_FOUND         Security Association was not found.
*       IPSEC_PKT_DISCARD       Packet is to be discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
*************************************************************************/
STATUS IP_IPSEC_Interpret(DV_DEVICE_ENTRY *device, NET_BUFFER *buf_ptr,
                          VOID **pkt, UINT8 *next_header,
                          UINT16 *header_len, UINT8 ver)
{
    /* Status of the request. */
    STATUS                      status = NU_SUCCESS;

    /* Variable is used as a counter in the loop below. */
    UINT8                       i;

    /* Index for the inbound SA. */
    IPSEC_INBOUND_INDEX         sa_index;

#if (IPSEC_ANTI_REPLAY == NU_TRUE)
    /* Sequence number used to check for replays. */
    IPSEC_SEQ_NUM               seq_num;
#endif

    /* The inside IP header in case of an ICMP error message. */
    UINT8                       *inside_ip_hdr;
    UINT16                      hlen = (ver == IPSEC_IPV4) ? IP_HEADER_LEN : 0;

#if (INCLUDE_IPV6 == NU_TRUE)
    NET_BUFFER              *last_ext_buf;
    UINT16                  last_ext_hdr_offset;
    UINT16                  ip6_hdrs_len;
    UINT8                   last_ext_hdr_type;
    UINT8                   *inner_ip6_pkt = 0;
    UINT8                   *dest_opt_ext_hdr = 0;
    UINT8                   internal_ipv6_fragment = NU_FALSE;
#endif


    /* Temporary variable used to traverse all the remaining headers. */
    INT16                       temp_header = 0;

    /* The following structure is used to quickly retrieve
     * information about IPsec protocols AH and ESP. This also
     * reduces the code size.
     */
    STATIC IPSEC_PROTOCOL_IP           ipsec_prot[IPSEC_MAX_HDR] =
    {

#if (IPSEC_INCLUDE_AH == NU_TRUE)
        {

#if (INCLUDE_IPV4 == NU_TRUE)
            /* AH decode function for IPv4 packets. */
            IPSEC_Decode_IPv4_AH,
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
            /* AH decode function for IPv6 packets. */
            IPSEC_Decode_IPv6_AH,
#endif
            /* AH specific offsets. */
            IPPROTO_AUTH, IPSEC_AH_SPI_OFFSET, IPSEC_AH_SEQNO_OFFSET},

#endif /* #if (IPSEC_INCLUDE_AH == NU_TRUE) */

#if (IPSEC_INCLUDE_ESP == NU_TRUE)
        {

#if (INCLUDE_IPV4 == NU_TRUE)
            /* ESP decode function for IPv4 packets. */
            IPSEC_Decode_IPv4_ESP,
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
            /* ESP decode function for IPv6 packets. */
            IPSEC_Decode_IPv6_ESP,
#endif
            /* ESP specific offsets. */
            IPPROTO_ESP, IPSEC_ESP_SPI_OFFSET, IPSEC_ESP_SEQNO_OFFSET},
#endif /* #if (IPSEC_INCLUDE_ESP == NU_TRUE) */
    };

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(header_len);
#endif

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((device == NU_NULL)  || (buf_ptr == NU_NULL) ||
       (pkt == NU_NULL)     || (next_header == NU_NULL) ||
                               (header_len == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* We will go through the following loop twice. In the first loop
     * we process AH headers. In the second loop we process ESP headers.
     * Note that we support AH only, ESP only, or AH followed by ESP.
     * This loop takes care of all the cases. If an error occurs, during
     * IPsec processing, this loop will break.
     */

    /* First set the source for this IP packet in to the index
     * because this will not change in the loop.
     */
#if (INCLUDE_IPV4 == NU_TRUE)
    if(ver == IPSEC_IPV4)
    {
        IP_ADDR_COPY(sa_index.ipsec_dest,
                                    (((UINT8 *)(*pkt)) + IP_DEST_OFFSET));
    }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    if(ver == IPSEC_IPV6)
        NU_BLOCK_COPY(sa_index.ipsec_dest,
                (((UINT8 *)(*pkt)) + IP6_DESTADDR_OFFSET),IP6_ADDR_LEN);
#endif

    sa_index.ipsec_dest_type = ver | IPSEC_SINGLE_IP;

    /* Decode both AH and ESP headers. */
    for(i = 0; (i < IPSEC_MAX_HDR); i++)
    {
        /* If the next header matches the required IPsec header. */
        if(ipsec_prot[i].ipsec_hdr == (UINT8)(*next_header))
        {
            /* Get the SPI from the header. */
            sa_index.ipsec_spi = GET32(buf_ptr->data_ptr,
                                        ipsec_prot[i].ipsec_spi_offset);

            /* Set the IPsec protocol. */
            sa_index.ipsec_protocol = ipsec_prot[i].ipsec_hdr;

#if (IPSEC_ANTI_REPLAY == NU_TRUE)
            /* Get the sequence number. */
            seq_num.ipsec_low_order = GET32(buf_ptr->data_ptr,
                                        ipsec_prot[i].ipsec_seq_offset);
#endif

            /* Obtain IPsec Semaphore. */
            status = NU_Obtain_Semaphore(&IPSEC_Resource,
                                         IPSEC_SEM_TIMEOUT);

            /* Check whether we got the semaphore. */
            if(status == NU_SUCCESS)
            {
                /* Find a matching SA in the inbound Security Association
                 * database.
                 */
                status = IPSEC_Get_Inbound_SA(device->dev_physical->
                                            dev_phy_ips_group, &sa_index,
                                      &IPSEC_In_Bundle[IPSEC_SA_Count]);

                /* Check whether we got the SA. */
                if(status == NU_SUCCESS)
                {
#if (INCLUDE_IKE == NU_TRUE)
                    /* Only check if it is established by IKE. */
                    if(&(IPSEC_In_Bundle[IPSEC_SA_Count]->
                                        ipsec_soft_lifetime) != NU_NULL)
                    {
                        /* Mark the SA as being used. */
                        IPSEC_In_Bundle[IPSEC_SA_Count]->
                                ipsec_soft_lifetime.ipsec_flags |=
                                                        IPSEC_IN_SA_USED;
                    }
#endif

/* Only check for replays if the stack is configured for this. */
#if (IPSEC_ANTI_REPLAY == NU_TRUE)

                    /* Set the ESN flag from the corresponding SA . */
                    seq_num.ipsec_is_esn =
                        IPSEC_In_Bundle[IPSEC_SA_Count]->
                            ipsec_antireplay.ipsec_last_seq.ipsec_is_esn;

                    /* Check the Anti-replay window for replays. */
                    status = IPSEC_Replay_Check(
                     &(IPSEC_In_Bundle[IPSEC_SA_Count]->ipsec_antireplay),
                     &seq_num);
#endif
                    /* Only proceed if status is success. */
                    if(status == NU_SUCCESS)
                    {

#if (INCLUDE_IPV4 == NU_TRUE)

                    /* If everything was okay and this is an IPv4 packet.
                     */
                    if(ver == IPSEC_IPV4)
                    {
                        /* Apply the IPsec protocol. This will update the
                         * next header in the IP header.
                         */
                        status = (ipsec_prot[i].ipsec_dec_ip4_func)
                                    (buf_ptr, (IPLAYER **)pkt,
                                     IPSEC_In_Bundle[IPSEC_SA_Count],
                                     &seq_num );

                        if (status == NU_SUCCESS)
                        {
                            /* Get the next header from the updated IP
                             * header.
                             */
                            *next_header = GET8(*pkt, IP_PROTOCOL_OFFSET);

                            /* For packet selector construction. */
                            temp_header = *next_header;

                            /* Tunneled packet handling. */
                            if(*next_header == IP_VERSION)
                            {
                                /* Now get the next header from
                                 * encapsulated IP packet header.
                                 */
                                temp_header = GET8(buf_ptr->data_ptr,
                                                   IP_PROTOCOL_OFFSET);
                            }
                        }
                    }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

                    /* If everything was okay and this is an IPv6 packet.
                     */
                    if(ver == IPSEC_IPV6)
                    {
                        /* Apply the IPsec protocol. */
                        status = (ipsec_prot[i].ipsec_dec_ip6_func)
                                    (buf_ptr, (IP6LAYER **)pkt,
                                    next_header, header_len,
                                    IPSEC_In_Bundle[IPSEC_SA_Count],
                                    &seq_num);

                        temp_header = *next_header;
                        inner_ip6_pkt = *pkt;

                        /* If the IPsec protocol was successfully applied and
                         * the following header is not higher layer protocol.
                         */
                        if(status == NU_SUCCESS && !(IP6_IS_NXTHDR_RECPROT(temp_header)))
                        {
                            /* Traverse the remaining IPsec extension
                             * headers to make sure that there do not
                             * exist any illegal headers.
                             */
                            switch (*next_header)
                            {
                                case IPPROTO_ESP:
                                    if(ipsec_prot[i].ipsec_hdr == IPPROTO_ESP)
                                        status = NU_INVAL_NEXT_HEADER;
                                    break;
                                case IPPROTO_DEST:
                                {
                                    /* Preserve the location of dest_opt_ext_hdr
                                     *  position.
                                     */
                                    dest_opt_ext_hdr = buf_ptr->data_ptr;

                                    /* Move data_ptr to header next to DEST_OPT
                                     * Which has to be either Higher layer protocol
                                     * or IPv6. */
                                    buf_ptr->data_ptr += (UINT16)(8 +
                                            (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));

                                    /* If header next to dest_opt is higher layer. */
                                    if(IP6_IS_NXTHDR_RECPROT(dest_opt_ext_hdr[
                                                     IP6_EXTHDR_NEXTHDR_OFFSET]))
                                    {
                                        break;
                                    }
                                    else if(dest_opt_ext_hdr[IP6_EXTHDR_NEXTHDR_OFFSET] !=
                                                                   IPPROTO_IPV6)
                                    {
                                        status = NU_INVAL_NEXT_HEADER;
                                        break;
                                    }
                                }
                                case IPPROTO_IPV6:
                                {
                                /* data_ptr is pointing at inner ipv6 header. */

                                    /* If we are here it means there is encapsulated
                                     * IPv6, either after dest_opt extension header
                                     * or after last processed IPSec header.
                                     */
                                    inner_ip6_pkt = buf_ptr->data_ptr;

                                    /* Extract the next header from inner IPv6 header. */
                                    temp_header = GET8(buf_ptr->data_ptr,IP6_NEXTHDR_OFFSET);

                                    /* Move data_ptr to next header which could
                                     * be higher layer or extension header.
                                     */
                                    buf_ptr->data_ptr += IP6_HEADER_LEN;

                                    /* If it was not higher layer, skip all
                                     * extension headers to reach higher layer. */
                                    while(!(IP6_IS_NXTHDR_RECPROT(temp_header)))
                                    {
                                        /* If encountered a fragment header. Check
                                         * if this is not the first fragment. */
                                        if(temp_header == IPPROTO_FRAGMENT &&
                                                GET16(buf_ptr->data_ptr, IP6_FRAGMENT_FRGOFFSET_OFFSET) != 0 &&
                                                IPSEC_In_Bundle[IPSEC_SA_Count]->ipsec_security.
                                                            ipsec_security_mode == IPSEC_TUNNEL_MODE)
                                        {
                                            /* This internal IPv6 is a fragment
                                             * without higher layer, so we won't
                                             * be able to verify it's selector.
                                             */
                                            status = NU_SUCCESS;
                                            internal_ipv6_fragment = NU_TRUE;
                                            break;
                                        }

                                        /* Get next header from IPv6 extension
                                         * header next header field.
                                         */
                                        temp_header = GET8(buf_ptr->data_ptr,
                                                     IP6_EXTHDR_NEXTHDR_OFFSET);

                                        buf_ptr->data_ptr += (UINT16)(8 +
                                            (buf_ptr->data_ptr[IP6_EXTHDR_LENGTH_OFFSET] << 3));
                                    }
                                    break;
                                }
                                default:
                                    status = NU_INVAL_NEXT_HEADER;
                                    break;
                            }
                        }
                    }
#endif
                    }

                    /* If the IPsec protocol was successfully applied.
                     */
                    if(status == NU_SUCCESS)
                    {
#if (INCLUDE_IPV6 == NU_TRUE)
                        if(ver == IPSEC_IPV6)
                        {
                            /* If the internal IPv6 packet (of a tunneled packet)
                             * has been identified as fragment, with fragment
                             * offset greater then zero. Then this IPv6 packet
                             * would not contain any transport layer header 
                             * and we can't create packet selector out of it.
                             */
                            if(internal_ipv6_fragment == NU_FALSE)
                                /* Update the packet selector for this packet.
                                 * This is required because, fields that were
                                 * previously opaque may now be known.
                                 */
                                IP_IPSEC_Pkt_Selector(&IPSEC_Pkt_Selector, inner_ip6_pkt,
                                                  buf_ptr, temp_header, ver,
                                                  0);

                            /* point the data_ptr back to it's original state. */
                            if(dest_opt_ext_hdr != NULL)
                            {
                                buf_ptr->data_ptr = dest_opt_ext_hdr;
                            }
                            /* If inner_ip6_pkt is not the same as external ipv6
                             * pkt then we encountered encapsulated ipv6 header.
                             */
                            else if(inner_ip6_pkt != *pkt)
                            {
                                buf_ptr->data_ptr = inner_ip6_pkt;
                            }
                        }
                        else
                        {
#endif /*INCLUDE_IPV6 == NU_TRUE*/
                            /* Update the packet selector for this packet.
                             * This is required because, fields that were
                             * previously opaque may now be known.
                             */
                            IP_IPSEC_Pkt_Selector(&IPSEC_Pkt_Selector, *pkt,
                                                  buf_ptr, temp_header, ver,
                                                  0);
#if (INCLUDE_IPV6 == NU_TRUE)
                        }
#endif /*INCLUDE_IPV6 == NU_TRUE*/

                        /* No point to check transport at this stage if
                         * there is still some nested IPsec header
                         * present. We first need to decrypt to completely
                         * visible packet selector.
                         */
                        if(
#if (INCLUDE_IPV6 == NU_TRUE)
                            /* If the internal IPv6 packet of a tunneled packet
                             * has been identified as fragment, with fragment
                             * offset greater then zero. We can not check it against
                             * selector, because there is no transport header.
                             */
                            (internal_ipv6_fragment == NU_FALSE) &&
#endif /* INCLUDE_IPV6 == NU_TRUE */
                            (IPSEC_Pkt_Selector.ipsec_transport_protocol
                                                        != IPSEC_AH) &&
                           (IPSEC_Pkt_Selector.ipsec_transport_protocol
                                                        != IPSEC_ESP))
                        {
                            /* Now verify that the selector in the SA
                             * matches the selector in the packet.
                             */
                            if(IPSEC_Match_Selectors(
                                    &(IPSEC_In_Bundle[IPSEC_SA_Count]->
                                                        ipsec_select),
                                        &IPSEC_Pkt_Selector, NU_FALSE)
                                                        == NU_FALSE)
                            {

                                /* Mark the status as bad. */
                                status = IPSEC_NOT_FOUND;
                            }

#if (INCLUDE_IPV4 == NU_TRUE)
                            /* If the SA did not match the packet selector but this
                             * is an ICMP error packet, then it is possible that the SA
                             * will match the IP packet inside the ICMP packet.
                             */
                            if ( ( status != NU_SUCCESS ) &&
                         ( IPSEC_Pkt_Selector.ipsec_transport_protocol == IP_ICMP_PROT )
                         &&
                         ( ICMP_IS_ERROR_MESSAGE(IPSEC_Pkt_Selector.ipsec_icmp_msg) ) )
                            {
                                /* We need to create the packet selector from the
                                 * inside packet.
                                 */
                                inside_ip_hdr = buf_ptr->data_ptr + IP_HEADER_LEN +
                                                                    ICMP_IP_OFFSET;

                                /* Advance the NET buffer to inner IP as well. */
                                buf_ptr->data_ptr += IP_HEADER_LEN + ICMP_IP_OFFSET;

                                /* Now update the packet selector. */
                                IP_IPSEC_Pkt_Selector( &IPSEC_Pkt_Selector, inside_ip_hdr,
                                                       buf_ptr, GET8(inside_ip_hdr,
                                                       IP_PROTOCOL_OFFSET),
                                                       ver, hlen);

                                if(IPSEC_Match_Selectors(
                                    &(IPSEC_In_Bundle[IPSEC_SA_Count]->
                                                      ipsec_select),
                                    &IPSEC_Pkt_Selector, NU_FALSE)
                                                    == NU_TRUE)
                                {
                                    status = NU_SUCCESS;
                                }

                            }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                            else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

                            /* If the SA did not match the packet selector but this
                             * is an ICMPv6 error packet, then it is possible that the SA
                             * will match the IP packet inside the ICMP packet.
                             */
                            if ( ( status != NU_SUCCESS ) &&
                             ( IPSEC_Pkt_Selector.ipsec_transport_protocol == IP_ICMPV6_PROT )
                             &&
                             ( ICMP6_ERROR_MSG(IPSEC_Pkt_Selector.ipsec_icmp_msg) ) )
                            {
                                /* We need to create the packet selector from the
                                 * inside packet.
                                 */
                                inside_ip_hdr = buf_ptr->data_ptr + IP6_HEADER_LEN +
                                    IP6_ICMP_DATA_OFFSET;

                                /* Advance the NET buffer to inner IP as well. */
                                buf_ptr->data_ptr += IP6_HEADER_LEN + IP6_ICMP_DATA_OFFSET;

                                IPSEC_IPv6_Get_Next_Prot_Hdr( buf_ptr,
                                    &last_ext_buf,
                                    &last_ext_hdr_offset,
                                    &ip6_hdrs_len,
                                    &last_ext_hdr_type );

                                /* Now update the packet selector. */
                                IP_IPSEC_Pkt_Selector( &IPSEC_Pkt_Selector, inside_ip_hdr,
                                                       buf_ptr, last_ext_hdr_type,
                                                       ver, hlen);

                                if(IPSEC_Match_Selectors(
                                      &(IPSEC_In_Bundle[IPSEC_SA_Count]->
                                      ipsec_select),
                                      &IPSEC_Pkt_Selector, NU_FALSE)
                                      == NU_TRUE)
                                {
                                    status = NU_SUCCESS;
                                }
                            }

#endif

                            if ( status == NU_SUCCESS )
                            {
/* Update the replay window if the stack is configured for this. */
#if (IPSEC_ANTI_REPLAY == NU_TRUE)

                                /* Now update the windows as well. */
                                status = IPSEC_Replay_Update(
                                    &(IPSEC_In_Bundle[IPSEC_SA_Count]->
                                            ipsec_antireplay), &seq_num);

#endif
                            }
                        }
                    }
                }

                /* Release the semaphore. */
                if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
                {
                    /* Failed to release the semaphore. */
                    NLOG_Error_Log("Failed to release IPsec semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                /* If an error was encountered, log an error and discard
                 * the packet.
                 */
                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("IPsec processing Failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Drop the packet by placing it back on the
                     * buffer_freelist.
                     */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List,
                                          &MEM_Buffer_Freelist);
                }
                else
                {
                    /* Increment the number of SAs that have been used
                     * in the processing of the current packet.
                     */
                    IPSEC_SA_Count++;
                }
            }
        }
    }

    return (status);

} /* IP_IPSEC_Interpret */

/*************************************************************************
*
*   FUNCTION
*
*       IP_IPSEC_Send
*
*   DESCRIPTION
*
*       Called by the IPv4 and IPv6 Layers to do outgoing processing for
*       IPsec. For IPv6 tunnel mode packets, the routine apply IPSec and
*       also transmits the packet.
*
*   INPUTS
*
*       *int_face               Pointer to the interface that will be used
*                               to send packets.
*       **hdr_buf               Pointer to the packet that is to be sent.
*       **ip_dgram              Pointer to the IP header in the packet.
*       ver                     Version of the IP packet (v4 or v6).
*       protocol                Higher layer protocol.
*       *tunnel_src_ip4         IPv4 tunnel source address.
*       *tunnel_src_ip6         IPv6 tunnel source address.
*       *dest                   Destination IP (SCK6_SOCKADDR_IP *).
*       *ro                     Route (RTAB6_ROUTE *).
*
*   OUTPUTS
*
*       NU_SUCCESS              IPSec application successful.
*       IPSEC_PACKET_SENT       Packet transmitted successfully.
*       NU_ACCESS               Access is denied.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
*************************************************************************/
STATUS IP_IPSEC_Send(DV_DEVICE_ENTRY *int_face, NET_BUFFER **hdr_buf,
                     VOID **ip_dgram, UINT8 ver, UINT8 protocol,
                     UINT8 *tunnel_src_ip4, UINT8 *tunnel_src_ip6,
                     VOID *dest_ptr, VOID *ro_ptr)
{
    /* Status of the request. */
    STATUS                  status;

    /* Packet selector is used to search for the IPsec policy matching this
     * packet.
     */
    IPSEC_SELECTOR          packet_selector;

    /* Pointer to the outbound SA bundle that will be applied on this
     * packet.
     */
    IPSEC_OUTBOUND_BUNDLE   *out_bundle;

    /* Policy to be get. */
    IPSEC_POLICY            *policy_ptr = NU_NULL;

#if (INCLUDE_UDP == NU_TRUE)

    /* If the packet being processed is UDP, then the following pointer
     * will point to the UDP port for the packet.
     */
    UDP_PORT                *uptr;

#endif

#if (INCLUDE_TCP == NU_TRUE)

    /* If the packet being processed is TCP, then the following pointer
     * will point to the TCP port for the packet.
     */
    TCP_PORT                *tcp_prt;
    NET_BUFFER              *tcp_buf;
    NET_BUFFER_HEADER       *hdr_dlist;
#endif

    IPSEC_SELECTOR          swap_selector;

    /* The inside IP header in case of an ICMP error message. */
    UINT8                   *inside_ip_hdr;
    UINT16                  hlen = (ver == IPSEC_IPV4) ? IP_HEADER_LEN : 0;

#if (INCLUDE_IPV6 == NU_TRUE)
    NET_BUFFER              *last_ext_buf;
    UINT16                  last_ext_hdr_offset;
    UINT16                  ip6_hdrs_len;
    UINT8                   last_ext_hdr_type;
    NET_BUFFER              *work_buf = NU_NULL;
    NET_BUFFER              *f_buf;
    STATUS                  inner_ip6_fragmented = NU_FALSE;
    SCK6_SOCKADDR_IP        *dest = (SCK6_SOCKADDR_IP *)dest_ptr;
    RTAB6_ROUTE             *ro = (RTAB6_ROUTE *)ro_ptr;
#else
    UNUSED_PARAMETER(dest_ptr);
    UNUSED_PARAMETER(ro_ptr);
#endif

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((int_face == NU_NULL) || (hdr_buf  == NU_NULL) ||
        (ip_dgram == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Obtain IPsec Semaphore to protect global structures. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, NU_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Create the packet selector. pass in a zero for the higher-layer
         * protocol and length. These are used to get the port numbers for
         * TCP/UDP protocols. We will get these later.
         */
        IP_IPSEC_Pkt_Selector(&packet_selector, *ip_dgram, *hdr_buf,
                              protocol, ver, hlen);

#if (INCLUDE_UDP == NU_TRUE)
        /* If the higher layer protocol is UDP, use the UDP port structure
         * to determine the outbound bundle.
         */
        if(protocol == IP_UDP_PROT)
        {
            /* Get a pointer to the UDP port. */
            uptr = (UDP_PORT *)((*hdr_buf)->mem_port);

            /* Complete the packet selector ports and higher layer
             * protocol.
             */
            packet_selector.ipsec_destination_port =
                GET16((*hdr_buf)->next_buffer->data_ptr, UDP_DEST_OFFSET);

            packet_selector.ipsec_source_port =
                GET16((*hdr_buf)->next_buffer->data_ptr, UDP_SRC_OFFSET);

            /* Compare the packet selector with the selector in the UDP
             * ports structure. If they match, that means that the UDP
             * ports structure already contains the outbound SA bundle
             * required. This will happen when we are exchanging data with
             * the same remote UDP port. We also check the policy group,
             * these could be different if we are working with multiple
             * interfaces.
             */
            if((uptr->up_ips_out_group == NU_NULL) ||
               (uptr->up_ips_out_group !=
                int_face->dev_physical->dev_phy_ips_group) ||
                (memcmp(&packet_selector, &(uptr->up_ips_out_select),
                                        sizeof(IPSEC_SELECTOR))) != 0)
            {
                /* Look through the policy database for policies matching
                 * this packet. Get the list of SA's that need to be
                 * applied on this packet.
                 */
                status = IPSEC_Match_Policy_Out(
                                int_face->dev_physical->dev_phy_ips_group,
                                &packet_selector,
                                &(uptr->up_ips_out_policy),
                                &(uptr->up_ips_out_bundle) );

                /* If the policy check succeeded, copy the packet selector
                 * to the selector in the UDP port.
                 */
                if(status == NU_SUCCESS)
                {
                    /* Update the UDP port selector. */
                    NU_BLOCK_COPY(&uptr->up_ips_out_select,
                                  &packet_selector,
                                  sizeof(IPSEC_SELECTOR));

                    /* Also update the policy group. */
                    uptr->up_ips_out_group = int_face->dev_physical->
                                                    dev_phy_ips_group;

                    /* Also update the policy used. */
                    policy_ptr = uptr->up_ips_out_policy;
                }
            }
            else
            {
                /* Get the policy from UDP port. */
                policy_ptr = uptr->up_ips_out_policy;
            }

            /* Get the bundle that will be used to apply IPsec. */
            out_bundle = uptr->up_ips_out_bundle;
        }
#endif

#if ((INCLUDE_UDP == NU_TRUE) && (INCLUDE_TCP == NU_TRUE))
        else
#endif

#if (INCLUDE_TCP == NU_TRUE)
        /* If the higher layer protocol is TCP, use TCP port structure
         * to determine outbound TCP bundle.
         */
        if(protocol == IP_TCP_PROT)
        {
            /* Get a pointer to the TCP port. */
            tcp_prt = (TCP_PORT *)((*hdr_buf)->mem_port);

            /* Complete the packet selector ports and higher layer
             * protocol.
             */
            packet_selector.ipsec_destination_port =
                GET16((*hdr_buf)->next_buffer->data_ptr, TCP_DEST_OFFSET);

            packet_selector.ipsec_source_port =
                GET16((*hdr_buf)->next_buffer->data_ptr, TCP_SRC_OFFSET);

            /* Compare the policy group of the interface with the policy
             * group in the TCP port. There is a possibility that this
             * will be different, when we have two interfaces and we were
             * communicating with the other interface before. If this is
             * the case we will need to review the policy for the new
             * interface. If no port is specified, we will also search
             * for the policy.
             */
            if((tcp_prt == NU_NULL) ||
               (tcp_prt->tp_ips_group != int_face->dev_physical->
                                         dev_phy_ips_group))
            {
                /* Look through the policy database for policies matching
                 * this packet. Get the list of SA's that need to be
                 * applied on this packet.
                 */
                status = IPSEC_Match_Policy_Out(
                                int_face->dev_physical->dev_phy_ips_group,
                            &packet_selector, &policy_ptr, &(out_bundle) );

                /* If we know the TCP port, save the information there.
                 * This will be used in subsequent packets.
                 */
                if((status == NU_SUCCESS) && (tcp_prt != NU_NULL))
                {
                    /* Update the policy group. */
                    tcp_prt->tp_ips_group =
                                int_face->dev_physical->dev_phy_ips_group;

                    /* Update the outgoing policy. */
                    tcp_prt->tp_ips_out_policy = policy_ptr;

                    /* Also update the bundle. */
                    tcp_prt->tp_ips_out_bundle = out_bundle;
                }
            }
            else
            {
                /* We have a TCP port, and the last packet received on
                 * this port is from the same interface that we have
                 * been receiving on previously.
                 */

                /* Get the policy that will be used to apply IPsec. We
                 * already know this from the past.
                 */
                policy_ptr = tcp_prt->tp_ips_out_policy;

                /* Get the bundle that will be used to apply IPsec. We
                 * already know this from the past.
                 */
                out_bundle     = tcp_prt->tp_ips_out_bundle;
            }

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* We have to make a copy of TCP segment and then pass it
                 * on to IPsec layer. This is to cater the TCP
                 * retransmissions.
                 */

                /* First get a buffer chain large enough to hold the TCP
                 * segment.
                 */
                tcp_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                        (INT32)((*hdr_buf)->next_buffer->
                                                    mem_total_data_len));

                /* Check if we get the buffer chain or not. */
                if(tcp_buf != NU_NULL)
                {
                    /* Initialize the data pointer*/
                    tcp_buf->data_ptr = tcp_buf->mem_parent_packet;

                    /* Now copy the TCP segment to the newly created
                     * buffer chain.
                     */
                    MEM_Chain_Copy(tcp_buf, (*hdr_buf)->next_buffer, 0,
                                        (INT32)((*hdr_buf)->next_buffer->
                                            mem_total_data_len));

                    /* Set the list to which the this buffer chain will
                     * be freed.
                     */
                    tcp_buf->mem_dlist = &MEM_Buffer_Freelist;

                    /* Store the mem_dlist temporarily. */
                    hdr_dlist = (*hdr_buf)->mem_dlist;

                    /* Set the mem_dlist to NULL so that first buffer
                     * which contains IP header does not get deallocated.
                     * We only want to free the remaining chain.
                     */
                    (*hdr_buf)->mem_dlist = NU_NULL;

                    /* Free the buffer chain. */
                    MEM_Multiple_Buffer_Chain_Free(*hdr_buf);

                    /* Now restore the mem_dlist for the parent buffer
                     * containing IP header.
                     */
                    (*hdr_buf)->mem_dlist = hdr_dlist;

                    /* Now link the IP header buffer and newly build
                       chain. */
                    (*hdr_buf)->next_buffer = tcp_buf;
                }
            }
        }

        /* Otherwise, this is neither TCP nor UDP, we will need to look
         * up the IPsec policy every time. There is no cache for these
         * connections.
         */
        else
#endif
        {
            if(protocol == IP_ICMP_PROT
#if (INCLUDE_IPV6 == NU_TRUE)
            || protocol == IP_ICMPV6_PROT
#endif
            )
            {
                packet_selector.ipsec_transport_protocol = protocol;
                /* Both icmpv6 and icmp have their type and code at same offset */
                packet_selector.ipsec_icmp_msg =
                        GET8((*hdr_buf)->next_buffer, ICMP_TYPE_OFFSET);
                packet_selector.ipsec_icmp_code =
                        GET8((*hdr_buf)->next_buffer, ICMP_CODE_OFFSET);

                packet_selector.ipsec_icmp_msg_high = packet_selector.ipsec_icmp_msg;
                packet_selector.ipsec_icmp_code_high = packet_selector.ipsec_icmp_code;
            }
            /* Look through the policy database for policies matching
             * this packet. Get the list of SA's that need to be
             * applied on this packet.
             */
            status = IPSEC_Match_Policy_Out(
                            int_face->dev_physical->dev_phy_ips_group,
                            &packet_selector, &policy_ptr, &out_bundle );

            /* If we did not find a policy (and therefore are planning
             * to discard the packet, we check whether this is an ICMP
             * error message. Section 6.2 of RFC 4301 requires us to
             * check whether the packet payload for which this ICMP error
             * is being is sent has a policy which can be used to send
             * the packet.
             */
#if (INCLUDE_IPV4 == NU_TRUE)

            if ( ( status != NU_SUCCESS ) &&
                 ( packet_selector.ipsec_transport_protocol == IP_ICMP_PROT ) &&
                 ( ICMP_IS_ERROR_MESSAGE(packet_selector.ipsec_icmp_msg) ) )
            {
               /* We need to create the packet selector from the inside packet. */
               inside_ip_hdr = (*hdr_buf)->data_ptr + IP_HEADER_LEN + ICMP_IP_OFFSET;

               /* Advance the NET buffer to inner IP as well. */
               (*hdr_buf)->data_ptr += IP_HEADER_LEN + ICMP_IP_OFFSET;

               /* Now update the packet selector. */
               IP_IPSEC_Pkt_Selector( &packet_selector, inside_ip_hdr, *hdr_buf,
                                      GET8(inside_ip_hdr, IP_PROTOCOL_OFFSET), ver, hlen);

               /* Reset the data_ptr for the NET buffer. */
               (*hdr_buf)->data_ptr -= ( IP_HEADER_LEN + ICMP_IP_OFFSET );

               /* Swap the selectors. */
               swap_selector.ipsec_src_tid.ipsec_src_port =
                   packet_selector.ipsec_dst_tid.ipsec_dst_port;

               memcpy( swap_selector.ipsec_source_ip.ipsec_addr,
                       packet_selector.ipsec_dest_ip.ipsec_addr, IP_ADDR_LEN );

               swap_selector.ipsec_source_type = packet_selector.ipsec_dest_type;

               packet_selector.ipsec_dst_tid.ipsec_dst_port =
                   packet_selector.ipsec_src_tid.ipsec_src_port;

               memcpy( packet_selector.ipsec_dest_ip.ipsec_addr,
                       packet_selector.ipsec_source_ip.ipsec_addr, IP_ADDR_LEN );

               packet_selector.ipsec_dest_type = packet_selector.ipsec_source_type;

               packet_selector.ipsec_src_tid.ipsec_src_port =
                   swap_selector.ipsec_src_tid.ipsec_src_port;

               memcpy( packet_selector.ipsec_source_ip.ipsec_addr,
                       swap_selector.ipsec_source_ip.ipsec_addr, IP_ADDR_LEN );

               packet_selector.ipsec_source_type = swap_selector.ipsec_source_type;

               /* Look through the policy database for policies matching
                * this packet. Get the list of SA's that need to be
                * applied on this packet.
                */
               status = IPSEC_Match_Policy_Out(
                              int_face->dev_physical->dev_phy_ips_group,
                              &packet_selector, &policy_ptr, &out_bundle );
            }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

            if ( ( status != NU_SUCCESS ) &&
                 ( packet_selector.ipsec_transport_protocol == IP_ICMPV6_PROT ) &&
                 ( ICMP6_ERROR_MSG(packet_selector.ipsec_icmp_msg) ) )
            {
               /* We need to create the packet selector from the inside packet. */
               inside_ip_hdr = (UINT8 *)ip_dgram + IP6_HEADER_LEN + IP6_ICMP_DATA_OFFSET;

               /* Advance the NET buffer to inner IP as well. */
               (*hdr_buf)->data_ptr += IP6_HEADER_LEN + IP6_ICMP_DATA_OFFSET;

               IPSEC_IPv6_Get_Next_Prot_Hdr( *hdr_buf,
                                             &last_ext_buf,
                                             &last_ext_hdr_offset,
                                             &ip6_hdrs_len,
                                             &last_ext_hdr_type );

               /* Now update the packet selector. */
               IP_IPSEC_Pkt_Selector( &packet_selector, inside_ip_hdr, *hdr_buf,
                                      last_ext_hdr_type, ver, hlen);

               /* Reset the data_ptr for the NET buffer. */
               (*hdr_buf)->data_ptr -= (IP_HEADER_LEN + ICMP_IP_OFFSET);

               /* Swap the selectors. */
               swap_selector.ipsec_src_tid.ipsec_src_port =
                    packet_selector.ipsec_dst_tid.ipsec_dst_port;

               memcpy( swap_selector.ipsec_source_ip.ipsec_addr,
                        packet_selector.ipsec_dest_ip.ipsec_addr, IP6_ADDR_LEN );

               swap_selector.ipsec_source_type = packet_selector.ipsec_dest_type;

               packet_selector.ipsec_dst_tid.ipsec_dst_port =
                        packet_selector.ipsec_src_tid.ipsec_src_port;

               memcpy( packet_selector.ipsec_dest_ip.ipsec_addr,
                        packet_selector.ipsec_source_ip.ipsec_addr, IP6_ADDR_LEN );

               packet_selector.ipsec_dest_type = packet_selector.ipsec_source_type;

               packet_selector.ipsec_src_tid.ipsec_src_port =
                        swap_selector.ipsec_src_tid.ipsec_src_port;

               memcpy( packet_selector.ipsec_source_ip.ipsec_addr,
                        swap_selector.ipsec_source_ip.ipsec_addr, IP6_ADDR_LEN );

               packet_selector.ipsec_source_type = swap_selector.ipsec_source_type;

               /* Look through the policy database for policies matching
                * this packet. Get the list of SA's that need to be
                * applied on this packet.
               */
               status = IPSEC_Match_Policy_Out(
                            int_face->dev_physical->dev_phy_ips_group,
                            &packet_selector, &policy_ptr, &out_bundle );
           }
#endif

        }

        /* If no error occurred so far and the outbound bundle length is
         * not zero then go ahead and apply the IPsec headers
         * specified in the bundle. Note that if the outbound bundle
         * length is zero, then the IPsec policy mandates that security
         * be by-passed.
         */
        if((status == NU_SUCCESS) &&
                            (policy_ptr->ipsec_security_size != 0))
        {
#if (INCLUDE_IPV6 == NU_TRUE)
            /* If the IPSEC security mode is tunnel and this is an IPv6 policy. */
            if(policy_ptr->ipsec_security->ipsec_security_mode == IPSEC_TUNNEL_MODE &&
                    (policy_ptr->ipsec_security->ipsec_flags & IPSEC_IPV6) != 0)
            {
                /* If the tunnel end point is different from end node address find
                 * the route to tunnel end point. */
                if(memcmp(policy_ptr->ipsec_security->ipsec_tunnel_destination,
                                            dest->sck_addr,  IP6_ADDR_LEN) != 0)
                {
                    /* If the inner IPv6 packet needs to be fragmented. */
                    if((*hdr_buf)->mem_total_data_len > ro->rt_route->rt_path_mtu)
                    {
                        /* Create Fragments of inner IP6 packet and mark the flag.
                         * Only host nodes may fragment packets. */
                        status = IP6_SFragment(*hdr_buf, *ip_dgram, int_face, ro, &work_buf);
                        inner_ip6_fragmented = NU_TRUE;
                    }
                    else
                    {
                        /* No internal fragments required. */
                        work_buf = *hdr_buf;
                    }

                    /* Figure out the route for tunnel destination */
                    RTAB_Free((ROUTE_ENTRY*)ro->rt_route, NU_FAMILY_IP6);
                    UTL_Zero((CHAR*)ro, sizeof(RTAB6_ROUTE));
                    dest = &ro->rt_ip_dest.rtab6_rt_ip_dest;
                    /*TODO: Check cached route*/
                    dest->sck_family = SK_FAM_IP6;
                    dest->sck_len = sizeof(SCK6_SOCKADDR_IP);
                    NU_BLOCK_COPY(dest->sck_addr, policy_ptr->ipsec_security->
                                        ipsec_tunnel_destination, IP6_ADDR_LEN);

                    IP6_Find_Route(ro);

                    if (ro->rt_route != NU_NULL)
                    {
                        int_face = ro->rt_ip_dest.rtab6_rt_device;
                        ro->rt_route->rt_entry_parms.rt_parm_use ++;
                        if (ro->rt_route->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
                                dest = &(ro->rt_route->rt_next_hop);
                    }
                    else
                    {
                        /* A route could not be found Return host unreachable error.*/
                        NLOG_Error_Log("IPv6 packet not sent due to no route",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        status = NU_HOST_UNREACHABLE;
                    }

                }
                else
                {
                    /* No internal fragments. */
                    work_buf = *hdr_buf;
                }

                /* Apply IPSEC on each fragment, and transmit. */
                for (f_buf = work_buf; f_buf != NU_NULL && status == NU_SUCCESS;
                                 f_buf = work_buf, *ip_dgram = work_buf->data_ptr)
                {
                    work_buf = f_buf->next;
                    f_buf->next = NU_NULL;
                    *ip_dgram = f_buf->data_ptr;

                    /* Save the flags from the original buffer into this buffer.  Note
                     * that saving the NET_PARENT flag in each buffer will not cause
                     * problems since that flag is used only for TCP, and TCP data
                     * does not get fragmented.
                     */
                    f_buf->mem_flags = (*hdr_buf)->mem_flags;

                    /* First put the device pointer into the buffer header,
                     * as there will be need for the associated device
                     * length.
                     */
                    f_buf->mem_buf_device = int_face;

                    /* Apply the security protocols. */
                    status = IP_IPSEC_Secure_Packet(&f_buf,
                                            (VOID **)ip_dgram, ver, policy_ptr,
                                            out_bundle, &packet_selector,
                                            tunnel_src_ip4, tunnel_src_ip6);

                    /* If the first packet cannot be transmitted successfully, then abort
                     * and free the rest.
                     */
                    if (status == NU_SUCCESS)
                    {
                        /* If the external IPv6 packet needs to be fragmented. */
                        if(f_buf->mem_total_data_len <= ro->rt_route->rt_path_mtu)
                        {
                            /* Set the IPv6 flag in the fragment. */
                            f_buf->mem_flags |= NET_IP6;

                            status = (*(int_face->dev_output))(f_buf, int_face,
                                         (SCK6_SOCKADDR_IP *)dest,
                                         (RTAB6_ROUTE *)ro);

                            /* If transmission successful, free the fragment. */
                            if (status != NU_SUCCESS)
                                MEM_One_Buffer_Chain_Free(f_buf, &MEM_Buffer_Freelist);
                        }
                        else
#if INCLUDE_IP_FRAGMENT
                        {
                            status = IP6_Fragment(f_buf, *ip_dgram, int_face, dest, ro);

                            if (status != NU_SUCCESS)
                            {
                                if(inner_ip6_fragmented == NU_TRUE)
                                {
                                    /* Free the chain of internal IP6 fragments
                                     * and return an error. */
                                    f_buf->next = work_buf;
                                    MEM_One_Buffer_Chain_Free(f_buf, f_buf->mem_dlist);
                                }
                                MIB_ipv6IfStatsOutFragFail_Inc(int_face);

                                status = NU_MSGSIZE;
                            }
                            else
                                MIB_ipv6IfStatsOutFragOKs_Inc(int_face);
                        }
#else
                        {
                            MIB_ipv6IfStatsOutFragFail_Inc(int_face);

                            NLOG_Error_Log("IPv6 packet not sent because fragmentation is disabled",
                                           NERR_SEVERE, __FILE__, __LINE__);

                            status = NU_MSGSIZE;
                        }
#endif
                    }
                    else
                    {
                        MEM_One_Buffer_Chain_Free(f_buf, &MEM_Buffer_Freelist);
                        break;
                    }
                }

                /* If we have fragmented inner ipv6 and successfully
                 * transmitted, then free the original buffer chain passed in.
                 */
                if(inner_ip6_fragmented == NU_TRUE && status == NU_SUCCESS)
                    MEM_One_Buffer_Chain_Free(*hdr_buf, &MEM_Buffer_Freelist);

                if (status != NU_SUCCESS)
                {
                    MIB_ipv6IfStatsOutFragFail_Inc(int_face);
                    status = NU_MSGSIZE;
                }
                else
                {
                    MIB_ipv6IfStatsOutFragOKs_Inc(int_face);
                    status = IPSEC_PACKET_SENT;
                }

            }
            else
#endif
            {
                /* First put the device pointer into the buffer header,
                 * as there will be need for the associated device
                 * length.
                 */
                (*hdr_buf)->mem_buf_device = int_face;

                /* Apply the security protocols. */
                status = IP_IPSEC_Secure_Packet(hdr_buf,
                                        (VOID **)ip_dgram, ver, policy_ptr,
                                        out_bundle, &packet_selector,
                                        tunnel_src_ip4, tunnel_src_ip6);

            }
        }

        /* If an error was returned, indicating that the policy
         * does not permit packets of this type to be sent,
         * discard the packet and return error.
         */
        else if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("IPsec policy check failed. ",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the semaphore. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            /* Failed to release the semaphore. */
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Unable to obtain IPsec semaphore.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IP_IPSEC_Send */

/*************************************************************************
*
*   FUNCTION
*
*       IP_IPSEC_Secure_Packet
*
*   DESCRIPTION
*
*       Called by the IP Layer to secure an outbound IP packet.
*
*   INPUTS
*
*       *int_face               Pointer to the interface that will be used
*                               to send packets.
*       **buf_ptr               A pointer to the NET buffer.
*       **pkt                   A pointer to the IP packet (can be either
*                               IPv4 or IPv6).
*       *policy_ptr             Pointer to IPsec policy.
*       ver                     Defines the version for IP packet.
*       *out_bundle             A bundle for outbound SAs that will be
*                               applied to the IP packet.
*       *pkt_selector           Pointer to packet selector.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       IPSEC_NOT_FOUND         Bundle or SA is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
*************************************************************************/
STATIC STATUS IP_IPSEC_Secure_Packet(
                              NET_BUFFER **buf_ptr,
                              VOID **pkt, UINT8 ver,
                              IPSEC_POLICY *policy_ptr,
                              IPSEC_OUTBOUND_BUNDLE *out_bundle,
                              IPSEC_SELECTOR *pkt_selector,
                              UINT8 *tunnel_src_ip4,
                              UINT8 *tunnel_src_ip6 )
{
    /* Status of the request. */
    STATUS              status;

#if ((IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE))

    /* TOS for the outer IPv4 header. */
    UINT8               tos;

    /* TTL for the outer IPv4 header. */
    UINT8               ttl;

    /* Protocol of the inner packet. */
    UINT8               protocol;

    UINT16              hlen;

    /* DF_bit. */
    UINT8               df_bit = 0;
#endif

    /* Counter for the bundle applied. */
    UINT8               bundle_count;

    /* Array of SA bundle pointers. */
    IPSEC_OUTBOUND_SA   *sa_ptr_bundle[IPSEC_MAX_SA_BUNDLE_SIZE];

    /* Group pointer. */
    IPSEC_POLICY_GROUP  *group_ptr;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((buf_ptr == NU_NULL)     || (pkt == NU_NULL) ||
       (policy_ptr == NU_NULL)  || (out_bundle == NU_NULL) ||
        (pkt_selector == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Get the group pointer. */
    group_ptr =
            (*buf_ptr)->mem_buf_device->dev_physical->dev_phy_ips_group;

    /* Get the SA pointers from their indices present in the bundle. */
    status = IPSEC_Get_Bundle_SA_Entries((*buf_ptr)->mem_buf_device->
                                    dev_index, group_ptr, policy_ptr,
                                out_bundle, pkt_selector, sa_ptr_bundle);

    /* Apply each SA in the outbound bundle. */
    for(bundle_count = 0;
        (status == NU_SUCCESS) &&
        (bundle_count < policy_ptr->ipsec_security_size);
        bundle_count++)
    {

#if (INCLUDE_IKE == NU_TRUE)
        /* Only check if it is established by IKE. */
        if(sa_ptr_bundle[bundle_count]->ipsec_soft_lifetime != NU_NULL)
        {
            /* First mark the SA as used. */
            sa_ptr_bundle[bundle_count]->ipsec_soft_lifetime->
                                    ipsec_flags |= IPSEC_OUT_SA_USED;
        }
#endif

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)

        /* If this SA is to be applied in tunnel mode then
         * we need to encode the IP header.
         */
        if((sa_ptr_bundle[bundle_count]->ipsec_security.
                            ipsec_security_mode) == IPSEC_TUNNEL_MODE)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            /* If the current header is IPv4 get the value for TOS.
             */
            if((ver == IPSEC_IPV4) && ((sa_ptr_bundle[bundle_count]->
                            ipsec_security.ipsec_flags) & IPSEC_IPV4))
            {
                tos         = GET8(*pkt, IP_SERVICE_OFFSET);
                ttl         = GET8(*pkt, IP_TTL_OFFSET);
                protocol    = IP_VERSION;

                /* Get the header length. */
                hlen = (UINT16)((GET8(*pkt,
                            IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

                /* Compute the IP checksum. Note that the length
                 * expected by TLS_IP_Check is the length of the
                 * header in 16 bit half-words.
                 */
                PUT16(*pkt, IP_CHECK_OFFSET, 0);
                PUT16(*pkt, IP_CHECK_OFFSET,
                    TLS_IP_Check((VOID*)(*pkt), (UINT16)(hlen >> 1)));

                /* Encode the new IP packet header. The source of the tunnel,
                 * will either be an IP address that is set in the policy or
                 * the IP address of the interface on which the packet is being
                 * sent.
                 */
                if ( ( sa_ptr_bundle[bundle_count]->ipsec_security.ipsec_flags
                       & IPSEC_SRC_FROM_INTERFACE ) &&
                      ( tunnel_src_ip4 != NU_NULL ) )
                {
                    status = IP4_IPS_Tunnel(buf_ptr, (IPLAYER **)pkt,
                                  sa_ptr_bundle[bundle_count]->
                                   ipsec_security.ipsec_tunnel_destination,
                                  tunnel_src_ip4,
                                  ttl, protocol, tos, df_bit);
                }

                else
                {
                    status = IP4_IPS_Tunnel(buf_ptr, (IPLAYER **)pkt,
                                  sa_ptr_bundle[bundle_count]->
                                   ipsec_security.ipsec_tunnel_destination,
                                  sa_ptr_bundle[bundle_count]->
                                   ipsec_security.ipsec_tunnel_source,
                                  ttl, protocol, tos, df_bit);
                }
            }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
            /* If the outer IP header is IPv6. */
            if((sa_ptr_bundle[bundle_count]->
                            ipsec_security.ipsec_flags) & IPSEC_IPV6)
            {
                /* Construct the new IPv6 header. */
                /* Encode the new IPv6 packet header. The source of the tunnel,
                 * will either be an IP address that is set in the policy or
                 * the IP address of the interface on which the packet is being
                 * sent.
                 */
                if ( ( sa_ptr_bundle[bundle_count]->ipsec_security.ipsec_flags
                       & IPSEC_SRC_FROM_INTERFACE ) &&
                      ( tunnel_src_ip6 != NU_NULL ) )
                {
                    status = IP6_IPS_Tunnel(buf_ptr, (IP6LAYER **)pkt,
                                  sa_ptr_bundle[bundle_count]->
                                   ipsec_security.ipsec_tunnel_destination,
                                   tunnel_src_ip6);
                }

                else
                {
                    status = IP6_IPS_Tunnel(buf_ptr, (IP6LAYER **)pkt,
                                  sa_ptr_bundle[bundle_count]->
                                   ipsec_security.ipsec_tunnel_destination,
                                  sa_ptr_bundle[bundle_count]->
                                   ipsec_security.ipsec_tunnel_source);


                }

                /* Update the IP version with the new IP header. */
                ver = IPSEC_IPV6;
            }
#endif
        }
#endif

#if (IPSEC_INCLUDE_AH == NU_TRUE)
        /* If the required IPsec protocol is AH. */
        if((sa_ptr_bundle[bundle_count]->ipsec_security.ipsec_protocol)
                                                            == IPSEC_AH)
        {

#if (INCLUDE_IPV4 == NU_TRUE)
            /* If we are protecting an IPv4 packet. */
            if(ver == IPSEC_IPV4)
            {
                /* This function will actually do the AH encoding
                 * for IPv4.
                 */
                status = IPSEC_Encode_IPv4_AH(buf_ptr, (IPLAYER **)pkt,
                                            sa_ptr_bundle[bundle_count]);
            }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
            /* If we are protecting an IPv6 packet. */
            if(ver == IPSEC_IPV6)
            {
                /* This function will actually do the AH encoding
                 * for IPv6.
                 */
                status = IPSEC_Encode_IPv6_AH(buf_ptr, (IP6LAYER **)pkt,
                                            sa_ptr_bundle[bundle_count]);
            }
#endif
        }
#endif

#if ((IPSEC_INCLUDE_AH == NU_TRUE) && (IPSEC_INCLUDE_ESP == NU_TRUE))
        else
#endif

#if (IPSEC_INCLUDE_ESP == NU_TRUE)

        {
            /* If the required IPsec protocol is ESP. */
            if((sa_ptr_bundle[bundle_count]->ipsec_security.
                                        ipsec_protocol) == IPSEC_ESP)
            {

#if (INCLUDE_IPV4 == NU_TRUE)
                /* If we are protecting an IPv4 packet. */
                if(ver == IPSEC_IPV4)
                {
                    /* This function will actually do the ESP encoding
                     * for IPv4.
                     */
                    status = IPSEC_Encode_IPv4_ESP(buf_ptr,
                                        (IPLAYER **)pkt,
                                        sa_ptr_bundle[bundle_count]);
                }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                /* If we are protecting an IPv6 packet. */
                if(ver == IPSEC_IPV6)
                {
                    /* This function will actually do the ESP
                     * encoding for IPv6.
                     */
                    status = IPSEC_Encode_IPv6_ESP(buf_ptr,
                                            (IP6LAYER **)pkt,
                                        sa_ptr_bundle[bundle_count]);
                }
#endif
            }
        }
#endif
    } /* End of 'for' loop. */

    /* Return the status. */
    return (status);

} /* IP_IPSEC_Secure_Packet */

/*************************************************************************
*
*   FUNCTION
*
*       IP_IPSEC_Pkt_Selector
*
*   DESCRIPTION
*
*       Called by the IPv4 and IPv6 Layers to create a packet selector.
*
*   INPUTS
*
*       *ipsec_selector         A pointer to the selector that will be
*                               filled up.
*       *pkt                    A pointer to the IP packet.
*       *buf_ptr                A pointer to the net buffer structure
*                               that is pointing to the higher layer
*                               protocol.
*       next_header             Specifies the higher layer protocol.
*       ver                     Specifies the IP version of packet.
*       hlen                    Specifies the header length of the packet.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_IPSEC_Pkt_Selector(IPSEC_SELECTOR *ipsec_selector, VOID *pkt,
                           NET_BUFFER *buf_ptr, UINT8 next_header,
                           UINT8 ver, UINT16 hlen)
{
    /* Pointer to the start of the IP header that is used to compute
     * the packet selector.
     */
    UINT8                       *ip_dgram;

    /* Clear the IPsec selector. We will at times do a memory compare
     * with other selectors. By zeroing out the unused portions of
     * memory we ensure that the comparison will not fail because
     * of unused portions of memory.
     */
    UTL_Zero(ipsec_selector, sizeof(IPSEC_SELECTOR));

#if (INCLUDE_IPV4 == NU_TRUE)

    /* If the packet is IPv4. */
    if(ver == IPSEC_IPV4)
    {
        /* Since this is an IPv4 packet, both source and destination
         * addresses for the selector will be single IPv4 addresses.
         */
        ipsec_selector->ipsec_source_type = IPSEC_IPV4 |IPSEC_SINGLE_IP;
        ipsec_selector->ipsec_dest_type   = IPSEC_IPV4 |IPSEC_SINGLE_IP;
    }

#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If the packet is IPv6. */
    if(ver == IPSEC_IPV6)
    {
        /* Since this is an IPv6 packet, both source and destination
        * addresses for the selector will be single IPv6 addresses.
        */
        ipsec_selector->ipsec_source_type = IPSEC_IPV6 |IPSEC_SINGLE_IP;
        ipsec_selector->ipsec_dest_type   = IPSEC_IPV6 |IPSEC_SINGLE_IP;
    }

#endif

     /* If the higher layer protocol is IP, this means that IPsec was
     * applied in tunnel mode. We need to create the packet selector
     * from the inner IP header.
     */

#if (INCLUDE_IPV4 == NU_TRUE)

    /* If the higher layer is IPv4. */
    if(next_header == IP_VERSION)
    {
        /* Get a pointer to the inner IP header. */
        ip_dgram = (UINT8 *)buf_ptr->data_ptr;

        /* Update the version of the IP packet. */
        ver = IPSEC_IPV4;
    }

#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If the higher layer is IPv6. */
    if(next_header == IPPROTO_IPV6)
    {
        /* Get a pointer to the inner IP header. */
        ip_dgram = (UINT8 *)buf_ptr->data_ptr;

        /* Update the version of the IPv6 packet. */
        ver = IPSEC_IPV6;
    }

#endif

    /* Otherwise, the packet selector is created from the passed IP
     * header.
     */
    else
    {
        if(pkt != NU_NULL)
        {
            ip_dgram = (UINT8 *)pkt;
        }
        else
        {
            /* This is an error so abort processing. */
            return;
        }
    }

    /* Now get the source and destination addresses. */
#if (INCLUDE_IPV4 == NU_TRUE)

    /* If the inner IP packet is IPv4. */
    if(ver == IPSEC_IPV4)
    {
        /* Get the packets destination. */
        IP_ADDR_COPY(ipsec_selector->ipsec_dest_ip.ipsec_addr,
                      ip_dgram + IP_DEST_OFFSET);

        /* Get the packets source. */
        IP_ADDR_COPY(ipsec_selector->ipsec_source_ip.ipsec_addr,
                      ip_dgram + IP_SRC_OFFSET);
    }

#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    else
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If the inner IP packet is IPv6. */
    if(ver == IPSEC_IPV6)
    {
        /* Get the packets destination. */
        NU_BLOCK_COPY(ipsec_selector->ipsec_dest_ip.ipsec_addr,
                    ip_dgram + IP6_DESTADDR_OFFSET, IP6_ADDR_LEN);

        /* Get the packets source. */
        NU_BLOCK_COPY(ipsec_selector->ipsec_source_ip.ipsec_addr,
                    ip_dgram + IP6_SRCADDR_OFFSET, IP6_ADDR_LEN);
    }

#endif

    /* Get the higher layer protocol. */
    ipsec_selector->ipsec_transport_protocol = next_header;

    IP_IPSEC_Pkt_Selector_Update_Ports(ipsec_selector, buf_ptr, next_header, hlen);

} /* IP_IPSEC_Pkt_Selector. */



/*************************************************************************
*
*   FUNCTION
*
*       IP_IPSEC_Pkt_Selector_Update_Ports
*
*   DESCRIPTION
*
*       Called by IP_IPSEC_Pkt_Selector and the IPv4 and IPv6 Layers to
*       add or update ports information in the packet selector.
*
*   INPUTS
*
*       *ipsec_selector         A pointer to the selector that will be
*                               filled up.
*       *buf_ptr                A pointer to the net buffer structure
*                               that is pointing to the higher layer
*                               protocol.
*       next_header             Specifies the higher layer protocol.
*       hlen                    Specifies the header length of the packet.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_IPSEC_Pkt_Selector_Update_Ports(IPSEC_SELECTOR *ipsec_selector,
                                        NET_BUFFER *buf_ptr,
                                        UINT8 next_header, UINT16 hlen)
{
    /* Pointer to the start of the higher-layer protocol header. */
    UINT8   *higher_layer;

    /* The higher layer protocol will follow after the IP header. */
    higher_layer = (UINT8 *)buf_ptr->data_ptr + hlen;

    switch(next_header)
    {

#if (INCLUDE_TCP == NU_TRUE)
    case IP_TCP_PROT:

        /* If the higher layer protocol is TCP then get the source and
         * destination TCP ports.
         */
        ipsec_selector->ipsec_destination_port =
            GET16(higher_layer, TCP_DEST_OFFSET);

        ipsec_selector->ipsec_source_port =
            GET16(higher_layer, TCP_SRC_OFFSET);
        break;
#endif

#if (INCLUDE_UDP == NU_TRUE)
    case IP_UDP_PROT:

        /* If the higher layer protocol is UDP then get the
         * source and destination UDP ports.
         */
        ipsec_selector->ipsec_destination_port =
            GET16(higher_layer, UDP_DEST_OFFSET);

        ipsec_selector->ipsec_source_port =
            GET16(higher_layer, UDP_SRC_OFFSET);

        break;

#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    case IP_ICMP_PROT:

        /* If the higher layer protocol is ICMP then get the
         * ICMP message type and code.
         */
        ipsec_selector->ipsec_icmp_msg =
            GET8( higher_layer, ICMP_TYPE_OFFSET );

        ipsec_selector->ipsec_icmp_code =
            GET8( higher_layer, ICMP_CODE_OFFSET );

        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    case IP_ICMPV6_PROT:
        /* If the higher layer protocol is ICMPv6 then get the
         * ICMP message type and code.
         */

        ipsec_selector->ipsec_icmp_msg = GET8( higher_layer,
                                                      IP6_ICMP_TYPE_OFFSET );

        ipsec_selector->ipsec_icmp_code = GET8( higher_layer,
                                                      IP6_ICMP_CODE_OFFSET );

        ipsec_selector->ipsec_icmp_msg_high = ipsec_selector->ipsec_icmp_msg;

        ipsec_selector->ipsec_icmp_code_high = ipsec_selector->ipsec_icmp_code;

        break;
#endif

    }

} /* IP_IPSEC_Pkt_Selector_Update_Ports */

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       IP_IPSEC_Forward
*
*   DESCRIPTION
*
*       Called by the IPv4 layer to do outgoing processing for
*       IPsec in case of IP forwarding.
*
*   INPUTS
*
*       *int_face               Pointer to the interface that will be used
*                               to send packets.
*       **hdr_buf               Pointer to the packet that is to be sent.
*       **ip_dgram              Pointer to the IP header in the packet.
*       ver                     Version of the IP packet (v4 or v6).
*       protocol                Higher layer protocol.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       NU_ACCESS               Access is denied.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
*************************************************************************/
STATUS IP_IPSEC_Forward(DV_DEVICE_ENTRY *int_face, NET_BUFFER **hdr_buf,
                        VOID **ip_dgram, UINT8 ver, UINT8 protocol,
                        UINT8 *tunnel_src_ip4, UINT8 *tunnel_src_ip6)
{
    /* Status of the request. */
    STATUS                  status;

    /* Packet selector is used to search for the IPsec policy matching
     * this packet.
     */
    IPSEC_SELECTOR          packet_selector;

    /* Pointer to the outbound SA bundle that will be applied on this
     * packet.
     */
    IPSEC_OUTBOUND_BUNDLE   *out_bundle;

    /* Policy to be get. */
    IPSEC_POLICY            *policy_ptr = NU_NULL;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((int_face == NU_NULL) || (hdr_buf  == NU_NULL) ||
        (ip_dgram == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Obtain IPsec Semaphore to protect global structures. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, NU_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Create the packet selector. pass in a zero for the higher-layer
         * protocol and length. These are used to get the port numbers for
         * TCP/UDP protocols. We will get these later.
         */
        IP_IPSEC_Pkt_Selector(&packet_selector, *ip_dgram, *hdr_buf,
                              protocol, ver, 0);

        /* If the higher layer protocol is UDP. */
        if(protocol == IP_UDP_PROT)
        {
            /* Complete the packet selector ports and higher layer
             * protocol.
             */
            packet_selector.ipsec_destination_port =
                            GET16((*hdr_buf)->data_ptr,
                                  (UDP_DEST_OFFSET + IP_HEADER_LEN));
            packet_selector.ipsec_source_port =
                            GET16((*hdr_buf)->data_ptr,
                                  (UDP_SRC_OFFSET + IP_HEADER_LEN));
        }

        /* If the higher layer protocol is TCP. */
        else if(protocol == IP_TCP_PROT)
        {
            /* Complete the packet selector ports and higher layer
             * protocol.
             */
            packet_selector.ipsec_destination_port =
                            GET16((*hdr_buf)->data_ptr,
                                  (TCP_DEST_OFFSET + IP_HEADER_LEN));
            packet_selector.ipsec_source_port =
                            GET16((*hdr_buf)->data_ptr,
                                  (TCP_SRC_OFFSET + IP_HEADER_LEN));
        }

        /* Look through the policy database for policies matching
         * this packet. Get the list of SA's that need to be
         * applied on this packet.
         */
        status = IPSEC_Match_Policy_Out(
                            int_face->dev_physical->dev_phy_ips_group,
                            &packet_selector, &policy_ptr, &out_bundle );

        /* If no error occurred so far and the outbound bundle length is
         * not zero then go ahead and apply the IPsec headers
         * specified in the bundle. Note that if the outbound bundle
         * length is zero, then the IPsec policy mandates that security
         * be by-passed.
         */
        if((status == NU_SUCCESS) &&
                                (policy_ptr->ipsec_security_size != 0))
        {
            /* First put the device pointer into the buffer header,
             * as there will be need for the associated device
             * length.
             */
            (*hdr_buf)->mem_buf_device = int_face;

            /* Apply the security protocols. */
            status = IP_IPSEC_Secure_Packet(hdr_buf,
                                    (VOID **)ip_dgram, ver, policy_ptr,
                                    out_bundle, &packet_selector,
                                    tunnel_src_ip4, tunnel_src_ip6);
        }

        /* If an error was returned, indicating that the policy
         * does not permit packets of this type to be sent,
         * discard the packet and return error.
         */
        else if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("IPsec check policy out failed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the semaphore. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            /* Failed to release the semaphore. */
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Unable to obtain IPsec semaphore.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

    }

    return (status);

} /* IP_IPSEC_Forward */
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

/************************************************************************
*
* FUNCTION
*
*       IP4_IPS_Tunnel
*
* DESCRIPTION
*
*       This function adds a tunnel header to the passed IP packet.
*       Passed parameters are copied onto the tunnel header.
*
* INPUTS
*
*       **buf_ptr               Pointer to the buffer chain
*       **pkt                   Pointer to the IP packet
*       *dest_ip                Destination IP address
*       *src_ip                 Source IP address
*       ttl                     TTL value to be set
*       protocol                Higher layer protocol
*       tos                     TOS value to be set
*       df_bit                  DF bit value to be set
*
* OUTPUTS
*
*       NU_SUCCESS              Tunnel added successfully
*       NU_NO_BUFFERS           Buffers not available.
*       IPSEC_NOT_FOUND         The group was not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IP4_IPS_Tunnel(NET_BUFFER **buf_ptr, IPLAYER **pkt, UINT8 *dest_ip,
                      UINT8 *src_ip, UINT8 ttl, UINT8 protocol,
                      UINT8 tos, UINT8 df_bit)
{
    /* Set the status to error by default. If we succeed we will update
     * this status.
     */
    STATUS                  status = NU_NO_BUFFERS;

    /* The new parent buffer. */
    NET_BUFFER              *hdr_buf;

    /* Variable specifying IP header length. */
    UINT32                  hlen = IP_HEADER_LEN;

    /* IPsec Policy Group pointer*/
    IPSEC_POLICY_GROUP      *grp_ptr;

    /* IPv4 Flags and fragment offset. */
    UINT16                  flags_frag_offset = 0;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (pkt    == NU_NULL) ||
        (dest_ip == NU_NULL) || (src_ip == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Allocate a new buffer chain for the link-layer and IP header */
    hdr_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                (INT32)(hlen + (*buf_ptr)->mem_buf_device->dev_hdrlen));

    grp_ptr =
        (*buf_ptr)->mem_buf_device->dev_ext.dev_phy->dev_phy_ips_group;

    /* If a new chain of buffers was allocated, link the buffer in as
     * the first buffer in the list.
     */
    if (hdr_buf != NU_NULL)
    {
        /* Link the new buffer in to the list */
        hdr_buf->next_buffer = (*buf_ptr);

        /* Set the list to which the header buffers will be freed. */
        hdr_buf->mem_dlist = &MEM_Buffer_Freelist;

        /* If the buffer chain passed in is flagged as a parent chain,
         * flag the new chain as a parent too.
         */
        if ((*buf_ptr)->mem_flags & NET_PARENT)
               hdr_buf->mem_flags |= NET_PARENT;

        /* Set the data pointer to the beginning of the IP header. */
        hdr_buf->data_ptr = hdr_buf->mem_parent_packet +
                          (*buf_ptr)->mem_buf_device->dev_hdrlen;

        /* Set the total data length of the chain of buffers. */
        hdr_buf->mem_total_data_len = (*buf_ptr)->mem_total_data_len;

        /* Get a pointer to the IP header. */
        (*pkt) = (IPLAYER*)hdr_buf->data_ptr;

        /* Fill-in the parameters of the IP header. */
        /* Set the IP header length and the IP version. */
        PUT8((*pkt), IP_VERSIONANDHDRLEN_OFFSET,
                            (UINT8)((hlen >> 2) | (IP_VERSION << 4)) );

        /* Check type of processing need to be applied. */
        if((grp_ptr->ipsec_df_processing & (UINT8)IPSEC_COPY_DF_BIT) ==
                                                (UINT8)IPSEC_COPY_DF_BIT)
        {
            /* If DF bit set then we also need to set. */
            if(df_bit == 1)
                flags_frag_offset |= (UINT16)IP_DF;
        }
        else if((grp_ptr->ipsec_df_processing & (UINT8)IPSEC_SET_DF_BIT)
                                             == (UINT8)IPSEC_SET_DF_BIT)
        {
            /* We always have to set the DF bit. */
            flags_frag_offset |=  (UINT16)IP_DF;
        }

        /* Zero out the fragment field. */
        PUT16((*pkt), IP_FRAGS_OFFSET, flags_frag_offset);

        /* Set the IP packet ID. */
        PUT16((*pkt), IP_IDENT_OFFSET, (UINT16)IP_Ident++);

        /* Set the type of service. */
        PUT8((*pkt), IP_SERVICE_OFFSET, (UINT8)tos);

        /* Set the total length (data and IP header) of this packet. */
        PUT16((*pkt), IP_TLEN_OFFSET,
                    (UINT16)(hdr_buf->mem_total_data_len + hlen));

        /* Set the time to live. */
        PUT8((*pkt), IP_TTL_OFFSET, (UINT8)ttl);

        /* Set the protocol. */
        PUT8((*pkt), IP_PROTOCOL_OFFSET, (UINT8)protocol);

        /* Set the destination IP address. */
        IP_ADDR_COPY((&(*pkt)->ip_dest), dest_ip);

        /* Set the source IP address. */
        IP_ADDR_COPY((&(*pkt)->ip_src), src_ip);

        /* Update the length of the header buffer. */
        hdr_buf->data_len           += hlen;
        hdr_buf->mem_total_data_len += hlen;

        /* Update the new parent buffer to the double-pointer which is to
         * be returned. This will update the packet outside this function.
         */
        *buf_ptr = hdr_buf;

        /* We have successfully created the outer-IP header. update the
         * status.
         */
        status = NU_SUCCESS;
    }

    return (status);

} /* IP4_IPS_Tunnel */

#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */

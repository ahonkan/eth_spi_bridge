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
*       ips6_ip.c
*
* COMPONENT
*
*       IPSEC - IP Processing
*
* DESCRIPTION
*
*       This file contains functions that are specific to IPv6 and IPsec.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IP6_IPS_Tunnel
*
* DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)

#include "networking/nu_net6.h"
#include "networking/ips_api.h"

/*************************************************************************
*
*   FUNCTION
*
*       IP6_IPS_Tunnel
*
*   DESCRIPTION
*
*       Called by the IPv6 Layer to do outgoing processing for IPsec.
*
*   INPUTS
*
*       **buf_ptr               A double Pointer to Net Buffer structure
*                               holding IPv6 datagram to be tunneled.
*       **pkt                   A double Pointer to IPv6 datagram.
*       *dest_ip                A Pointer to tunnel destination IPv6
*                               address.
*       *src_ip                 A Pointer to tunnel source IPv6 address.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*       NU_NO_BUFFERS           Unable to allocate Net Buffer.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
*************************************************************************/
STATUS IP6_IPS_Tunnel(NET_BUFFER **buf_ptr, IP6LAYER **pkt,
                      UINT8 *dest_ip, UINT8 *src_ip)
{
    /* Set the status to error by default. If we succeed we will update
     * this status.
     */
    STATUS                  status = NU_NO_BUFFERS;

    /* The new parent buffer. */
    NET_BUFFER              *hdr_buf;

    /* Variable specifying IP header length. */
    UINT32                  hlen = IP6_HEADER_LEN;

    /* Hop Limit of original IPv6 header. */
    UINT8                   hop_limit = (*pkt)->ip6_hop;


    /* Version, Traffic Class and Flow label of original IPv6 header. */
    UINT32                  ip6_hdr_ver;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (pkt == NU_NULL) ||
       (dest_ip == NU_NULL) || (src_ip == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Copy Version, Traffic Class and Flow label of original IPv66 header
     */
    ip6_hdr_ver = GET32(*pkt, 0);

    /* Allocate a new buffer chain for the link-layer and IP header. */
    hdr_buf = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                (INT32)(hlen + (*buf_ptr)->mem_buf_device->dev_hdrlen));

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
        (*pkt) = (IP6LAYER*)hdr_buf->data_ptr;

        /* Set Version no, Traffic Class and Flow label. */
        PUT32((*pkt), IP6_XXX_OFFSET, (UINT32)ip6_hdr_ver);

        /* Set the total length (data and IP header) of this packet. */
        PUT16((*pkt), IP6_PAYLEN_OFFSET,
                    (UINT16)(hdr_buf->mem_total_data_len));

        /* Set the protocol. */
        PUT8((*pkt), IP6_NEXTHDR_OFFSET, (UINT8)IPPROTO_IPV6);

        /* Set Hop Limit. */
        PUT8((*pkt), IP6_HOPLMT_OFFSET, (UINT8)hop_limit);

        /* Set the destination IP address. */
        NU_BLOCK_COPY((*pkt)->ip6_dest, dest_ip, IP6_ADDR_LEN);

        /* Set the source IP address. */
        NU_BLOCK_COPY((*pkt)->ip6_src, src_ip, IP6_ADDR_LEN);

        /* Update the length of the header buffer. */
        hdr_buf->data_len           += hlen;
        hdr_buf->mem_total_data_len += hlen;

        /* Update the new parent buffer to the double-pointer which is to
         * be returned. This will update the packet outside this function.
         */
        *buf_ptr = hdr_buf;
    }

    return (status);

} /* IP6_IPS_Tunnel. */

#endif /* #if (INCLUDE_IPV6 == NU_TRUE) */

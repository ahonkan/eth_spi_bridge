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
*       ips6_ah.c
*
* COMPONENT
*
*       AH (Authentication Header)
*
* DESCRIPTION
*
*       Implementation of IP Authentication Header protocol. This file
*       includes encoding and decoding of IPv6 based packet for
*       IPsec protocol.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_IPv6_Get_Next_Prot_Hdr
*       IPSEC_Encode_IPv6_AH
*       IPSEC_Decode_IPv6_AH
*       IPSEC_Clear_Mutable_Fields_IPv6
*
* DEPENDENCIES
*
*       nu_net.h
*       ip6.h
*       ips_api.h
*       ips_ah.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/ip6.h"
#include "networking/ips_api.h"
#include "networking/ips_ah.h"

#if (IPSEC_INCLUDE_AH == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       IPSEC_IPv6_Get_Next_Prot_Hdr
*
*   DESCRIPTION
*
*       This function traverses all extension headers up to the upper
*       layer protocol or IPsec protocols (AH or ESP) from the packet and
*       returns the protocol type.  Upon entering the function,
*       buf_ptr->data_ptr is pointing to the generic portion of the IP
*       header.  After successful completion of the function,
*       last_ext_hdr_offset will contain the offset of the last extension
*       header.
*
*   INPUTS
*
*       *ip6_hdr_buf            A pointer to the buffer.
*       **last_ext_buf          A pointer to the buffer which will
*                               contain the last extension header.
*       *last_ext_hdr_offset    The offset of the last extension header.
*       *ip6_hdrs_len           IPv6 header plus extension headers length.
*
*   OUTPUTS
*
*       The protocol type of the packet.
*
*************************************************************************/
VOID IPSEC_IPv6_Get_Next_Prot_Hdr(NET_BUFFER *ip6_hdr_buf,
                                  NET_BUFFER **last_ext_buf,
                                  UINT16 *last_ext_hdr_offset,
                                  UINT16 *ip6_hdrs_len,
                                  UINT8 *last_ext_hdr_type)
{
    UINT8 HUGE          *data_ptr = ip6_hdr_buf->data_ptr;

    /* Initialize the offset length. */
    UINT16              ext_hdr_len = 0;
    UINT16              current_hdr_len;
    UINT8               next_hdr = GET8(ip6_hdr_buf->data_ptr,
                                                      IP6_NEXTHDR_OFFSET);

    /* Set IPv6 header length. */
    *ip6_hdrs_len = IP6_HEADER_LEN;

    /* Getting the pointer to the data in the buffer. */
    *last_ext_buf = ip6_hdr_buf;

    if ( ip6_hdr_buf->data_len > (UINT32)(*ip6_hdrs_len) )
    {
        /* Initialize the offset length. */
        ext_hdr_len = IP6_HEADER_LEN;
    }

    /* This buffer does not have the next option. Go to next buffer if it
     * exists.
     */
    else if (ip6_hdr_buf->next_buffer != NU_NULL)
    {
        *last_ext_buf = ip6_hdr_buf->next_buffer;
    }

    /* Check the upper layer protocol. */
    if(!(IP6_IS_NXTHDR_RECPROT(next_hdr)))
    {
        do
        {
            /* Set extension header offset. */
            *last_ext_hdr_offset = ext_hdr_len;

            /* If the next header is AH. */
            if(next_hdr == IPSEC_AH)
            {
                /* Get the AH header length. AH header length is define
                   in terms of 32 bits units minus 2. So getting the
                   length. */
                current_hdr_len = (UINT16)((2 +
                                        GET8((data_ptr + ext_hdr_len),
                                        IPSEC_AH_PLOADLEN_OFFSET)) << 2);
            }
            else if((next_hdr == IPSEC_ESP) || (next_hdr == IPPROTO_IPV6))
            {
                /* Set current header length to zero if next header after
                   IPv6 header is ESP or IPv6 inner header. */
                current_hdr_len  = 0;

                /* Store the offset length. */
                *last_ext_hdr_offset = ext_hdr_len;
            }
            else
            {
                /* Getting extension header length. */
                current_hdr_len = (UINT16)(8 +
                    (GET8((data_ptr + ext_hdr_len),
                    IP6_EXTHDR_LENGTH_OFFSET) << 3));

                /* Store the offset length. */
                *last_ext_hdr_offset = ext_hdr_len;
            }

            /* Get the next extension header. */
            if(current_hdr_len != 0)
                next_hdr = GET8((data_ptr + ext_hdr_len),
                                IP6_EXTHDR_NEXTHDR_OFFSET);

            /* If the next extension header is in the next buffer. */
            if((*last_ext_buf)->data_len <= (UINT32)ext_hdr_len)
            {

                /* Get the data pointer from the next buffer. */
                data_ptr        = (*last_ext_buf)->data_ptr;

                /* Get the next buffer. */
                if((*last_ext_buf)->next_buffer != NU_NULL)
                {
                    *last_ext_buf   = (*last_ext_buf)->next_buffer;
                }

                else /* We have reached the end of buffer chain. */
                {
                    break;
                }

                /* Again initialize the length offset to zero. */
                ext_hdr_len     = 0;
            }

            /* Increment the offset length. */
            ext_hdr_len     = (ext_hdr_len + current_hdr_len);
            *ip6_hdrs_len   = (*ip6_hdrs_len + current_hdr_len);
        }
        /* Check if there are any extension headers present. */
        while(!(IP6_IS_NXTHDR_RECPROT(next_hdr) || (next_hdr == IPSEC_ESP)
                                        || (next_hdr == IPPROTO_IPV6)));
    }
    else
    {
        /* Return the offset length of the last extension header
           if present otherwise it will be of IPv6 header offset. */
        *last_ext_hdr_offset = ext_hdr_len;
    }

    /* Return the last extension header type. */
    *last_ext_hdr_type = next_hdr;

} /* IPSEC_IPv6_Get_Next_Prot_Hdr. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Encode_IPv6_AH
*
* DESCRIPTION
*
*       This function, respective to IPv6, encodes the passed NET buffer
*       chain with AH protocol according to the parameters supplied
*       through the given SA.
*
* INPUTS
*
*       **buffer                NET buffer chain to be encoded.
*       **ip6_pkt               Pointer to the IPv6 header.
*       *out_sa                 SA to be applied.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful AH encoding.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Encode_IPv6_AH(NET_BUFFER **buffer ,IP6LAYER **ip6_pkt,
                            IPSEC_OUTBOUND_SA *out_sa)
{
    STATUS              status;
    UINT8               ipsec_auth_algo_id;
    UINT16              ip_tlen;
    AHLAYER             *ah_header;
    NET_BUFFER          *buf_ptr;
    NET_BUFFER          *ip6_last_ext_hdr_buf = NU_NULL;
    NET_BUFFER          *temp_buf  = NU_NULL;
    UINT16              last_ext_hdr_offset = 0;
    UINT16              buf_capacity;
    UINT8               last_ext_hdr_type;
    UINT8               ip6_hdr_backup[IP6_HEADER_EIGHT_BYTES];
    IPSEC_AUTH_REQ      request;
    UINT16              ip_header_len = 0;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (ip6_pkt == NU_NULL) ||
                              (out_sa  == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    buf_ptr = *buffer;

    /* Calculate IPv6 header length */
    IPSEC_IPv6_Get_Next_Prot_Hdr(buf_ptr, &ip6_last_ext_hdr_buf,
                                 &last_ext_hdr_offset,&ip_header_len,
                                 &last_ext_hdr_type);
    /* Build AH header. */
    status = IPSEC_Build_AH_Hdr(&buf_ptr, *ip6_pkt, &ah_header, out_sa,
                                ip_header_len);

    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        /* Update IPv6 header pointer for the caller. */
        *ip6_pkt = (IP6LAYER *)buf_ptr->data_ptr;

        /* First get the ipsec authentication algorithm id. */
        ipsec_auth_algo_id = out_sa->ipsec_security.ipsec_auth_algo;

        /* Set the protocol field of IP header as AH protocol ID. */
        PUT8(buf_ptr->data_ptr, (IP6_NEXTHDR_OFFSET), IPSEC_AH);

        /* Set the next protocol field of AH header. */
        PUT8(buf_ptr->data_ptr, (IP6_EXTHDR_NEXTHDR_OFFSET +
                                    ip_header_len), last_ext_hdr_type);

        /* Getting the current IP datagram length. */
        ip_tlen = (UINT16)(GET16(buf_ptr->data_ptr, IP6_PAYLEN_OFFSET) +
                             (UINT16)IPSEC_AH_HDR_LEN(ipsec_auth_algo_id));

        /* Update the total length of the packet in the IP header. */
        PUT16(buf_ptr->data_ptr, IP6_PAYLEN_OFFSET, ip_tlen);

        /* Before clearing the mutable fields, take backup of the IP
           header, as we will need these fields to reset them. */
        NU_BLOCK_COPY(ip6_hdr_backup, buf_ptr->data_ptr,
                                                   IP6_HEADER_EIGHT_BYTES);

        /* Call the IPv6 specific function for clearing up the IPv6
           mutable fields in the transit.  */
        IPSEC_Clear_Mutable_Fields_IPv6((IP6LAYER*)buf_ptr->data_ptr);

        /* If ESN is enabled, then we need to place the higher 32-bits.
         * of the sequence number for the digest calculation. This
         * will be removed below.
         */
        if ( out_sa->ipsec_seq_num.ipsec_is_esn )
        {
            /* Traverse to the last buffer in the chain. */
            for(temp_buf = buf_ptr; temp_buf->next_buffer != NU_NULL;
                temp_buf = temp_buf->next_buffer)
                ;

            /* Make sure this buffer has the capacity to append four
             * bytes.
             */
            buf_capacity = (temp_buf->mem_flags & NET_PARENT) ?
                            NET_PARENT_BUFFER_SIZE :
                            NET_MAX_BUFFER_SIZE;

            if ( buf_capacity < temp_buf->data_len + sizeof (UINT32) )
            {
                temp_buf->next_buffer = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

                if( temp_buf->next_buffer != NU_NULL )
                {
                    temp_buf = temp_buf->next_buffer;

                    /* Set the data pointer. */
                    temp_buf->data_ptr  = temp_buf->mem_packet;

                    /* Set the list to which this buffer will be freed. */
                    temp_buf->mem_dlist = &MEM_Buffer_Freelist;
                }
                else
                {
                    return ( NU_NO_BUFFERS );
                }
            }

            PUT32( (UINT8*)temp_buf->data_ptr, temp_buf->data_len,
                    out_sa->ipsec_seq_num.ipsec_high_order );

            temp_buf->data_len += sizeof (UINT32);
        }

        /* Set the authentication key. */
        request.ipsec_key           = out_sa->ipsec_auth_key;

        /* Set the authentication key length. */
        request.ipsec_key_len       =
                           IPSEC_GET_AUTH_KEY_LEN(ipsec_auth_algo_id);

        /* Set the authentication algo ID. */
        request.ipsec_hash_algo = ipsec_auth_algo_id;

        /* Set the digest length required. */
        request.ipsec_digest_len    =
                        IPSEC_GET_AUTH_DIGEST_LEN(ipsec_auth_algo_id);

        /* Assign the pointer for the digest to be stored. */
        request.ipsec_digest        = (UINT8 *)ah_header +
                                             IPSEC_AH_AUTHDATA_OFFSET;
        /* Now calculate the digest. */
        status = IPSEC_Calculate_Digest(buf_ptr, &request);

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* Remove the higher order 32-bit sequence number. If ESN is
             * enabled.
             */
            if ( out_sa->ipsec_seq_num.ipsec_is_esn )
            {
                temp_buf->data_len -= sizeof (UINT32);
            }

            /* Now set the IP header to its original values. */
            NU_BLOCK_COPY(buf_ptr->data_ptr, ip6_hdr_backup,
                                                   IP6_HEADER_EIGHT_BYTES);

            /* Update the total length of the packet in the IP header. */
            PUT16(buf_ptr->data_ptr, IP6_PAYLEN_OFFSET, ip_tlen);

            /* Set back the modified buffer pointer. */
            *buffer = buf_ptr;
        }
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Encode_IPv6_AH. */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Decode_IPv6_AH
*
* DESCRIPTION
*
*       This function decodes the passed NET buffer chain buffers encoded
*       with AH protocol according to the parameters supplied through the
*       given SA. This routine is for IPv6 based traffic only.
*
* INPUTS
*
*       *buffer                 NET buffer chain to be decoded.
*       **ip6_pkt               Pointer to the IPv6 header.
*       *next_header            next header type after AH header to be
*                               used by upper layer protocol processing.
*       *header_len             IPv6 header length plus extension headers
*                               length.
*       *in_sa                  Incoming SA to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful AH decoding.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Decode_IPv6_AH (NET_BUFFER *buffer, IP6LAYER **ip6_pkt,
                             UINT8 *next_header, UINT16 *header_len,
                             IPSEC_INBOUND_SA *in_sa,
                             IPSEC_SEQ_NUM *seq_num)
{
    STATUS              status;
    UINT8 HUGE          *curr_data_ptr;
    UINT8               digest_buff[IPSEC_AH_MAX_AUTHDATA_LEN];
    UINT16              ipsec_auth_algo_id;
    AHLAYER             *ah_header;
    NET_BUFFER          *buf_ptr = buffer;
    IPSEC_AUTH_REQ      request;
    NET_BUFFER          *ip6_last_ext_hdr_buf = NU_NULL;
    NET_BUFFER          *temp_buf = NU_NULL;
    UINT16              last_ext_hdr_offset = 0;
    UINT16              buf_capacity;
    UINT8               ip6_hdr_backup[IP6_HEADER_LEN];
    UINT16              payload_length;
    UINT8               last_ext_hdr_type;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer      == NU_NULL) || (ip6_pkt    == NU_NULL) ||
       (next_header == NU_NULL) || (header_len == NU_NULL) ||
                                   (in_sa      == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First get the authentication algorithm ID from the SA. */
    ipsec_auth_algo_id = in_sa->ipsec_security.ipsec_auth_algo;

    /* Set the data_ptr to the start of IP header. */
    buffer->data_ptr = (UINT8*)*ip6_pkt;

    /* Get the IP header length. */
    IPSEC_IPv6_Get_Next_Prot_Hdr(buffer, &ip6_last_ext_hdr_buf,
                                 &last_ext_hdr_offset, header_len,
                                 &last_ext_hdr_type);

    *next_header = last_ext_hdr_type;

    /* First check that AH header is present in this buffer or is it
       in the next buffer. */
    if(buf_ptr->data_len >= (UINT32)(IPSEC_AH_HDR_LEN(ipsec_auth_algo_id)))
    {
        /* AH header is present in the current buffer. Get it. */
        ah_header = (AHLAYER *)(buf_ptr->data_ptr + last_ext_hdr_offset);

        /* Take backup of IP header before clearing the mutable fields. */
        NU_BLOCK_COPY(ip6_hdr_backup, *ip6_pkt, IP6_HEADER_LEN);

        /* Clearing up the mutable fields so that digest can be
           calculated over the whole datagram. */
        IPSEC_Clear_Mutable_Fields_IPv6((IP6LAYER*)buf_ptr->data_ptr);

        /* If ESN is enabled, then we need to place the higher 32-bits.
         * of the sequence number for the digest calculation. This
         * will be removed below.
         */
        if ( seq_num->ipsec_is_esn )
        {
            /* Traverse to the last buffer in the chain. */
            for(temp_buf = buf_ptr; temp_buf->next_buffer != NU_NULL;
                temp_buf = temp_buf->next_buffer)
                ;
            /* Make sure this buffer has the capacity to append four
             * bytes.
             */
            buf_capacity = (temp_buf->mem_flags & NET_PARENT) ?
                            NET_PARENT_BUFFER_SIZE :
                            NET_MAX_BUFFER_SIZE;

            if ( buf_capacity < temp_buf->data_len + sizeof (UINT32) )
            {
                temp_buf->next_buffer = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

                if( temp_buf->next_buffer != NU_NULL )
                {
                    temp_buf = temp_buf->next_buffer;

                    /* Set the data pointer. */
                    temp_buf->data_ptr  = temp_buf->mem_packet;

                    /* Set the list to which this buffer will be freed. */
                    temp_buf->mem_dlist = &MEM_Buffer_Freelist;
                }
                else
                {
                    return ( NU_NO_BUFFERS );
                }
            }

            PUT32( (UINT8*)temp_buf->data_ptr, temp_buf->data_len,
                    seq_num->ipsec_high_order );

            temp_buf->data_len += sizeof (UINT32);
        }

        /* Set the auth key. */
        request.ipsec_key       = in_sa->ipsec_auth_key;

        /* Set the auth key length. */
        request.ipsec_key_len   =
                            IPSEC_GET_AUTH_KEY_LEN(ipsec_auth_algo_id);

        /* Set the authentication algorithm to be used. */
        request.ipsec_hash_algo = ipsec_auth_algo_id;

        /* Set the digest length required. */
        request.ipsec_digest_len    =
                        IPSEC_GET_AUTH_DIGEST_LEN(ipsec_auth_algo_id);

        /* Assign the pointer for the digest to be stored. */
        request.ipsec_digest = (UINT8 *)ah_header + IPSEC_AH_HDRFIXED_LEN;

        /* Store the incoming digest for future comparison. */
        NU_BLOCK_COPY(digest_buff,
                      (UINT8 *)ah_header + IPSEC_AH_HDRFIXED_LEN,
                      request.ipsec_digest_len);

        /* Now clear the digest from the packet as well. */
        UTL_Zero(((UINT8 *)ah_header + IPSEC_AH_HDRFIXED_LEN),
                                                request.ipsec_digest_len);

        /* Store the current data pointer. */
        curr_data_ptr       = buf_ptr->data_ptr;

        /* Set the data pointer with the IP header. */
        buf_ptr->data_ptr   = (UINT8 *)*ip6_pkt;

        /* Now calculate the digest. */
        status = IPSEC_Calculate_Digest(buf_ptr, &request);

        /* Set back the data pointer as well. */
        buf_ptr->data_ptr   = curr_data_ptr;

        /* Decrement the buffer length by IP header length. */
        buf_ptr->data_ptr   += (*header_len);

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* Remove the higher order 32-bit sequence number. If ESN is
             * enabled.
             */
            if ( seq_num->ipsec_is_esn )
            {
                temp_buf->data_len -= sizeof (UINT32);
            }

            /* Now compare the stored digest (ICV) and the calculated
                digest over the packet. */
            if(memcmp(request.ipsec_digest, digest_buff,
                    request.ipsec_digest_len) != 0)
            {
                /* Two digests are not equal, hence unauthenticated. */
                status = IPSEC_INVALID_DIGEST;
            }
            else
            {
                /* Also update the next protocol IDs. */
                PUT8(ip6_hdr_backup, IP6_NEXTHDR_OFFSET,
                                                      ah_header->ah_nhdr);

                /* Now set the IP header to its original values. */
                NU_BLOCK_COPY((buf_ptr->data_ptr - IP6_HEADER_LEN),
                               ip6_hdr_backup, IP6_HEADER_LEN);

                /* Also update the passed IP pointer. */
                *ip6_pkt = (IP6LAYER *)(buf_ptr->data_ptr -
                                                          IP6_HEADER_LEN);

                /* Get the payload length field. */
                payload_length = GET16(*ip6_pkt, IP6_PAYLEN_OFFSET);

                /* Subtract AH header length. */
                payload_length -= IPSEC_AH_HDR_LEN(ipsec_auth_algo_id);

                /* Update the payload length field. */
                PUT16(*ip6_pkt, IP6_PAYLEN_OFFSET, payload_length);
            }
        }
    }
    else
    {
        /* Headers are not of valid length, discard packet here. */
        status = IPSEC_PKT_DISCARD;
    }

    /* Return the status variable. */
    return (status);

} /* IPSEC_Decode_IPv6_AH. */

/************************************************************************
*
* FUNCTION
*
*        IPSEC_Clear_Mutable_Fields_IPv6
*
* DESCRIPTION
*
*        This function zero outs the IPv6 header mutable fields
*        and return the original header as backup. Caller should take
*        backup of the header before calling this function.
*
* INPUTS
*
*        *ip6                IPv6 header.
*
* OUTPUTS
*
*        None
*
************************************************************************/
VOID IPSEC_Clear_Mutable_Fields_IPv6(IP6LAYER *ip6)
{
    /* Following fields are  mutable as specified in the RFC 2402. */

    /* Muting TRAFFIC CLASS field. */
    PUT8(ip6, IP6_TRAFFIC_CLASS_OFFSET, 0);

    /* Muting FLOW LABEL field. */
    PUT16(ip6, IP6_FLOW_LABEL_OFFSET, 0);

    /* Muting HOP LIMIT field. */
    PUT8(ip6, IP6_HOPLMT_OFFSET, 0);

    return;

} /* IPSEC_Clear_Mutable_Fields_IPv6. */

#endif /* #if (IPSEC_INCLUDE_AH == NU_TRUE) */

#endif /* #if (INCLUDE_IPV6 == NU_TRUE) */

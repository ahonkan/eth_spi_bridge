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
*       ips6_esp.c
*
* COMPONENT
*
*       ESP (Encapsulation Security Payload)
*
* DESCRIPTION
*
*       Implementation of IPsec Encapsulation Security Payload component
*       for IPv6 based packets.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Encode_IPv6_ESP
*       IPSEC_Decode_IPv6_ESP
*
* DEPENDENCIES
*
*       nu_net.h
*       ip6.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/ip6.h"
#include "networking/ips_api.h"

#if (IPSEC_INCLUDE_ESP == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Encode_IPv6_ESP
*
* DESCRIPTION
*
*       This function, respective to IPv6, encodes the passed NET buffer
*       chain with ESP protocol according to the parameters supplied
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
STATUS IPSEC_Encode_IPv6_ESP(NET_BUFFER **buffer, IP6LAYER **ip6_pkt,
                             IPSEC_OUTBOUND_SA *out_sa)
{
    STATUS          status;
    UINT8           auth_algo_index;
    UINT8           iv_len;
    UINT8           last_ext_hdr_type;
    UINT16          ip_hlen;
    UINT16          last_ext_hdr_offset = 0;
    NET_BUFFER      *buf_ptr;
    ESP_TRAILER     *esp_trailer;
    NET_BUFFER      *ip6_last_ext_hdr_buf = NULL;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters */
    if((buffer == NU_NULL) || (ip6_pkt == NU_NULL) || (out_sa == NU_NULL))
    {
        return(IPSEC_INVALID_PARAMS);
    }

#endif

    /* Get last header type. */
    IPSEC_IPv6_Get_Next_Prot_Hdr(*buffer, &ip6_last_ext_hdr_buf,
                                 &last_ext_hdr_offset, &ip_hlen,
                                 &last_ext_hdr_type);

    /* First build the ESP header. */
    status = IPSEC_Build_ESP_Hdr(buffer, *ip6_pkt, out_sa, &iv_len,
                                 ip_hlen);

    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        /* Update the passed buffer pointer. */
        buf_ptr = *buffer;

        /* Update the IP layer pointer as well for the caller. */
        *ip6_pkt = (IP6LAYER *)buf_ptr->data_ptr;

        /* Get authentication algorithm index. */
        auth_algo_index = out_sa->ipsec_security.ipsec_auth_algo;

        /* Calculate ESP padding, initialize it and get the ESP trailer
           pointer. */
        status = IPSEC_Get_ESP_Trailer(buf_ptr, ip_hlen, iv_len,
                                       auth_algo_index, &esp_trailer);

        if (status == NU_SUCCESS)
        {
            /* Now set the next header (upper layer) in the ESP trailer.*/
            PUT8(esp_trailer, IPSEC_ESP_NHDR_OFFSET, last_ext_hdr_type);

            buf_ptr->mem_total_data_len +=
                (esp_trailer->esp_pad_length + sizeof(ESP_TRAILER));

            /* Update the IP header next header field. */
            if(ip_hlen == IP6_HEADER_LEN)
            {
                PUT8(*ip6_pkt, IP6_NEXTHDR_OFFSET, IPSEC_ESP);
            }
            else
            {
                PUT8((buf_ptr->data_ptr + last_ext_hdr_offset),
                     IP6_EXTHDR_NEXTHDR_OFFSET, IPSEC_ESP);
            }

            /* Update payload length field. */
            PUT16(*ip6_pkt, IP6_PAYLEN_OFFSET,
                (UINT16)(GET16(*ip6_pkt, IP6_PAYLEN_OFFSET) +
                IPSEC_ESP_HDRFIXED_LEN + iv_len +
                esp_trailer->esp_pad_length + sizeof(ESP_TRAILER)));

            /* Encode IPv6 packet. */
            status = IPSEC_Encode_ESP(buf_ptr, ip_hlen, iv_len, out_sa);

            if (status == NU_SUCCESS)
            {
                /* Update the IP total payload field. */
                PUT16(*ip6_pkt, IP6_PAYLEN_OFFSET,
                    (UINT16)(GET16(*ip6_pkt, IP6_PAYLEN_OFFSET) +
                    IPSEC_ESP_AUTHDATA_LEN(auth_algo_index)));
            }
            else
            {
                NLOG_Error_Log("Failed to Encode the packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Discard packet. */
                status = IPSEC_PKT_DISCARD;
            }
        }
        else
        {
            NLOG_Error_Log("Failed to get ESP trailer",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Discard packet. */
            status = IPSEC_PKT_DISCARD;
        }
    }
    else
    {
        NLOG_Error_Log("Failed to build ESP header",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Discard packet. */
        status = IPSEC_PKT_DISCARD;
    }

    /* Return the status value. */
    return(status);

} /* IPSEC_Encode_IPv6_ESP */

/************************************************************************
* FUNCTION
*
*       IPSEC_Decode_IPv6_AH
*
* DESCRIPTION
*
*       This function decodes the passed NET buffer chain encoded
*       with ESP protocol according to the parameters supplied through the
*       given SA. This routine is for IPv6 based traffic only.
*
* INPUTS
*
*       *buffer                 NET buffer chain to be decoded.
*       **ip6_pkt               Pointer to the IPv6 layer.
*       *next_header            Next header after the ESP header
*                               to be used by the upper layer
*       *header_len             IPv6 header length plus extension
*                               headers length.
*       *in_sa                  Incoming SA to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful AH decoding.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Decode_IPv6_ESP(NET_BUFFER *buffer, IP6LAYER **ip6_pkt,
                             UINT8 *next_header, UINT16 *header_len,
                             IPSEC_INBOUND_SA *in_sa, IPSEC_SEQ_NUM *seq_num )
{
    STATUS          status;
    UINT8           esp_total_hdr_len;
    UINT8           auth_algo_index;
    UINT8           ip6_hdr_backup[IP6_HEADER_LEN];
    UINT8           last_ext_hdr_type;
    UINT16          last_ext_hdr_offset = 0;
    ESP_TRAILER     esp_tail;
    NET_BUFFER      *ip6_last_ext_hdr_buf;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (ip6_pkt == NU_NULL) ||
        (next_header == NU_NULL) || (header_len == NU_NULL) ||
        (in_sa == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Move the data pointer to start of IPv6 header. */
    buffer->data_ptr -= IP6_HEADER_LEN;

    /* Get header length. */
    IPSEC_IPv6_Get_Next_Prot_Hdr(buffer, &ip6_last_ext_hdr_buf,
                                 &last_ext_hdr_offset, header_len,
                                 &last_ext_hdr_type);

    /* Exclude the IP header for digest calculation. */
    buffer->data_ptr        += *header_len;
    buffer->data_len        -= *header_len;

    /* Decode IPv6 packet using the Inbound Security Association in_sa. */
    status = IPSEC_Decode_ESP(buffer, in_sa, &esp_tail,
                              &esp_total_hdr_len, seq_num);

    if (status == NU_SUCCESS)
    {

        /* Now set back the next protocol header in the IP header. */
        if(*header_len == IP6_HEADER_LEN)
            (*ip6_pkt)->ip6_next = esp_tail.esp_nhdr;

        else
        {
            PUT8((buffer->data_ptr - *header_len - last_ext_hdr_offset),
                IP6_EXTHDR_NEXTHDR_OFFSET, esp_tail.esp_nhdr);
        }

        *next_header = esp_tail.esp_nhdr;


        /* Remove the ESP header from the packet.
         * Move the upper layer header and payload next to
         * IP header.
         */

        NU_BLOCK_COPY(ip6_hdr_backup,*ip6_pkt,IP6_HEADER_LEN);

        NU_BLOCK_COPY((buffer->data_ptr -
            (*header_len - esp_total_hdr_len)),ip6_hdr_backup,
            IP6_HEADER_LEN);

        /* Update the IP header pointer as well. */
        *ip6_pkt = (IP6LAYER *)(buffer->data_ptr - (*header_len -
            esp_total_hdr_len));

        /* Update the data pointer to the upper layer header.
         */
        buffer->data_ptr += esp_total_hdr_len;

        /* Decrement the length by ESP header length. */
        buffer->data_len -= esp_total_hdr_len;


        /* Get authentication algorithm index. */
        auth_algo_index =  in_sa->ipsec_security.ipsec_auth_algo;

        /* Decrement the total data length as well. Excluding
           ESP header, pad length (if any), trailer and
           authentication data. */
        buffer->mem_total_data_len -= (esp_total_hdr_len +
            esp_tail.esp_pad_length + sizeof(ESP_TRAILER) +
            IPSEC_ESP_AUTHDATA_LEN(auth_algo_index));

        /* Update the IP payload field in the IP header. */
        PUT16(*ip6_pkt, IP6_PAYLEN_OFFSET, (UINT16)buffer->data_len);

        /* Include the IPv6 header length. */
        buffer->data_len += *header_len;
    }

    /* Return the status value. */
    return(status);

} /* IPSEC_Decode_IPv6_ESP */

#endif /* #if (IPSEC_INCLUDE_ESP == NU_TRUE) */

#endif /* #if (INCLUDE_IPV6      == NU_TRUE) */

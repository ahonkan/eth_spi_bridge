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
*       ips4_esp.c
*
* COMPONENT
*
*       ESP (Encapsulation Security Payload)
*
* DESCRIPTION
*
*       Implementation of IPsec Encapsulation Security Payload component
*       containing IPv4 specific routines.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Encode_IPv4_ESP
*       IPSEC_Decode_IPv4_ESP
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#if (IPSEC_INCLUDE_ESP == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Encode_IPv4_ESP
*
* DESCRIPTION
*
*       This function, respective to IPv4, encodes the passed
*       NET buffer chain with ESP protocol according to the parameters
*       supplied through the given SA.
*
* INPUTS
*
*       **buffer                List of buffers to be encoded.
*       **ip                    Pointer to the IPv4 layer.
*       *out_sa                 SA to be applied.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful ESP encoding.
*       NU_NO_BUFFERS           Buffers not available.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Encode_IPv4_ESP(NET_BUFFER **buffer, IPLAYER **ip,
                             IPSEC_OUTBOUND_SA *out_sa)
{
    STATUS              status;
    UINT8               iv_len;
    UINT8               auth_algo_index;
    UINT16              ip_hlen;
    NET_BUFFER          *buf_ptr;
    ESP_TRAILER         *esp_trailer;


#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (ip == NU_NULL) || (out_sa == NU_NULL))
    {
        return(IPSEC_INVALID_PARAMS);
    }

#endif

    /* Get IP header length. */
    ip_hlen =
        (UINT16)((GET8(*ip, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* First build the ESP header. */
    status = IPSEC_Build_ESP_Hdr(buffer, *ip, out_sa, &iv_len, ip_hlen);

    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        /* Update the passed buffer pointer. */
        buf_ptr = *buffer;

        /* Update the IP layer pointer as well for the caller. */
        *ip = (IPLAYER *)buf_ptr->data_ptr;

        /* Get authentication algorithm index. */
        auth_algo_index = out_sa->ipsec_security.ipsec_auth_algo;

        /* Calculate ESP padding, initialize it and get the ESP trailer
           pointer. */
        status = IPSEC_Get_ESP_Trailer(buf_ptr, ip_hlen, iv_len,
                                       auth_algo_index, &esp_trailer);

        if (status == NU_SUCCESS)
        {
            /* Now set the next header (upper layer) in the ESP trailer.*/
            PUT8(esp_trailer, IPSEC_ESP_NHDR_OFFSET, (*ip)->ip_protocol);

            /* Increment the buffer length and total packet length also.*/
            buf_ptr->mem_total_data_len +=
                (esp_trailer->esp_pad_length + sizeof(ESP_TRAILER));

            /* Also update the IP header next header field. */
            PUT8(*ip, IP_PROTOCOL_OFFSET, IPSEC_ESP);

            /* Update the IP payload field also. */
            PUT16(*ip, (UINT16)IP_TLEN_OFFSET,
                (UINT16)(GET16(*ip, (UINT16)IP_TLEN_OFFSET) +
                IPSEC_ESP_HDRFIXED_LEN + iv_len +
                esp_trailer->esp_pad_length + sizeof(ESP_TRAILER)));

            /* Encode the IPv4 packet. */
            status = IPSEC_Encode_ESP(buf_ptr, ip_hlen, iv_len, out_sa);

            if (status == NU_SUCCESS)
            {
                /* Update the IP total payload field. */
                PUT16(*ip, IP_TLEN_OFFSET,
                    (UINT16)(GET16(*ip, IP_TLEN_OFFSET) +
                    IPSEC_ESP_AUTHDATA_LEN(auth_algo_index)));
            }
            else
            {
                NLOG_Error_Log("Failed to encode the IPv4 packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Failed to get ESP trailer",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to build ESP header",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Encode_IPv4_ESP. */


/************************************************************************
* FUNCTION
*
*       IPSEC_Decode_IPv4_ESP
*
* DESCRIPTION
*
*       This routine authenticate and decrypt ESP packet.
*
* INPUTS
*
*       *buffer                 NET buffer chain to be decoded.
*       **ip                    Pointer to the IP header.
*       *in_sa                  Incoming SA to be used.
*       *seq_num                Sequence number received in the packet.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful ESP decoding.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Decode_IPv4_ESP(NET_BUFFER *buffer, IPLAYER **ip,
                             IPSEC_INBOUND_SA *in_sa,
                             IPSEC_SEQ_NUM *seq_num )
{
    STATUS          status;
    UINT8           i;
    UINT8           ip_hlen;
    UINT8           esp_total_hdr_len;
    UINT8           auth_algo_index;
    ESP_TRAILER     esp_tail;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (ip == NU_NULL) || (in_sa == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Calculate the IP header length. */
    ip_hlen = (UINT8)((GET8(*ip, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* Decode IPv4 packet using the Inbound Security Association in_sa. */
    status = IPSEC_Decode_ESP(buffer, in_sa,&esp_tail,
                              &esp_total_hdr_len, seq_num);

    if (status == NU_SUCCESS)
    {
        /* Now set back the next protocol header in the IP header. */
        (*ip)->ip_protocol = esp_tail.esp_nhdr;

        /*
         * Also remove the ESP header from the packet.
         * Move the upper layer header and payload next to
         * IP header.
         */
        if(esp_total_hdr_len < ip_hlen)
        {
            for(i = ip_hlen; i > 0; i--)
            {
                /* Copy the data adjacent to upper layer header.
                 */
                PUT8(buffer->data_ptr,
                     (esp_total_hdr_len - (ip_hlen - i) - 1),
                      ((UINT8 *)(*ip))[i - 1]);
            }
        }
        else
        {
            /* Copy the whole IP header */
            NU_BLOCK_COPY(buffer->data_ptr +
                         (esp_total_hdr_len - ip_hlen), *ip,
                         ip_hlen);
        }

        /* Update the IP header pointer as well. */
        *ip = (IPLAYER *)(buffer->data_ptr -
                         (ip_hlen - esp_total_hdr_len));

        /* Update the data pointer to the upper layer header. */
        buffer->data_ptr += esp_total_hdr_len;

        /* Decrement the length by ESP header length. */
        buffer->data_len -= esp_total_hdr_len;

        /* Get authentication algorithm index. */
        auth_algo_index = in_sa->ipsec_security.ipsec_auth_algo;

        /* Decrement the total data length as well. Excluding ESP
           header, pad length (if any), trailer and
           authentication data. */
        buffer->mem_total_data_len -= (esp_total_hdr_len +
                        esp_tail.esp_pad_length + sizeof(ESP_TRAILER) +
                        IPSEC_ESP_AUTHDATA_LEN(auth_algo_index));

        /* Update the IP payload field in the IP header. */
        PUT16(*ip, IP_TLEN_OFFSET, (UINT16)buffer->mem_total_data_len);
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Decode_IPv4_ESP. */

#endif /* #if (IPSEC_INCLUDE_ESP == NU_TRUE) */

#endif /* #if (INCLUDE_IPV4      == NU_TRUE) */

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
*       ips4_ah.c
*
* COMPONENT
*
*       AH (Authentication Header)
*
* DESCRIPTION
*
*       Implementation of IP Authentication Header protocol. This file
*       includes encoding and decoding of IPv4 based packets for
*       IPsec protocol.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Encode_IPv4_AH
*       IPSEC_Decode_IPv4_AH
*       IPSEC_Clear_Mutable_Fields_IPv4
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

#if (IPSEC_INCLUDE_AH == NU_TRUE)

/************************************************************************
* FUNCTION
*
*        IPSEC_Encode_IPv4_AH
*
* DESCRIPTION
*
*        This function, specific to IPv4, encodes the passed NET buffer
*        chain with AH protocol according to the parameters
*        supplied through the given SA.
*
* INPUTS
*
*        **buffer               Buffer chain to be encoded.
*        **ip                   Pointer to the IPv4 header.
*        *out_sa                SA to be applied.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful AH encoding.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Encode_IPv4_AH(NET_BUFFER **buffer, IPLAYER **ip,
                            IPSEC_OUTBOUND_SA *out_sa)
{
    STATUS          status;
    UINT8           ipsec_auth_algo_id;
    UINT16          ip_tlen;
    UINT16          ip_hlen;
    UINT16          buf_capacity;
    AHLAYER         *ah_dgram;
    NET_BUFFER      *buf_ptr = *buffer;
    NET_BUFFER      *temp_buf = NU_NULL;
    UINT8           ip_hdr[IP_HEADER_LEN];
    IPSEC_AUTH_REQ  request;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (*ip == NU_NULL) || (out_sa == NU_NULL))
        return (IPSEC_INVALID_PARAMS);

#endif

    /* Calculate IP header length. */
    ip_hlen =
        (UINT16)((GET8(*ip, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* First build the AH header. */
    status = IPSEC_Build_AH_Hdr(&buf_ptr, *ip, &ah_dgram, out_sa,
                                ip_hlen);
    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        /* Before clearing the mutable fields, take backup of the IP
           header, as we will need these fields to reset them. */
        NU_BLOCK_COPY(ip_hdr, buf_ptr->data_ptr, IP_HEADER_LEN);

        /* First get the ipsec auth algo id. */
        ipsec_auth_algo_id = out_sa->ipsec_security.ipsec_auth_algo;

        /* Set the protocol field of IP header as AH protocol ID. */
        PUT8(buf_ptr->data_ptr, IP_PROTOCOL_OFFSET, IPSEC_AH);

        /* Getting the current IP datagram length. */
        ip_tlen = (UINT16)(GET16(buf_ptr->data_ptr, IP_TLEN_OFFSET) +
                            (UINT16)IPSEC_AH_HDR_LEN(ipsec_auth_algo_id));

        /* Update the total length of the packet in the IP header. */
        PUT16(buf_ptr->data_ptr, IP_TLEN_OFFSET, ip_tlen);

        /* Call the IPv4 specific function for clearing up the IPv4
           mutable fields in the transit.  */
        IPSEC_Clear_Mutable_Fields_IPv4((IPLAYER *)buf_ptr->data_ptr);

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
        request.ipsec_digest        = (UINT8*)ah_dgram +
                                      (INT)IPSEC_AH_AUTHDATA_OFFSET;

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
            NU_BLOCK_COPY(buf_ptr->data_ptr, ip_hdr, IP_HEADER_LEN);

            /* Again set the next protocol with the AH protocol. */
            PUT8(buf_ptr->data_ptr, IP_PROTOCOL_OFFSET, IPSEC_AH);

            /* Update the total length of the packet in the IP header. */
            PUT16(buf_ptr->data_ptr, IP_TLEN_OFFSET, ip_tlen);

            /* Update the IP layer pointer as well. */
            *ip = (IPLAYER *)buf_ptr->data_ptr;

            /* Set back the modified buffer pointer. */
            *buffer = buf_ptr;
        }
        else
        {
            NLOG_Error_Log("Failed to calculate digest",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to build IPsec AH header",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Encode_IPv4_AH */

/************************************************************************
*
* FUNCTION
*
*        IPSEC_Decode_IPv4_AH
*
* DESCRIPTION
*
*        This function decodes the passed NET buffer chain encoded
*        with AH protocol according to the parameters supplied through the
*        given SA. This routine is for IPv4 based traffic only.
*
* INPUTS
*
*        *buffer                Buffer chain to be decoded.
*        **ip                   Pointer to the IP header.
*        *in_sa                 Incoming SA to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful AH decoding.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Decode_IPv4_AH(NET_BUFFER *buffer, IPLAYER **ip,
                            IPSEC_INBOUND_SA *in_sa,
                            IPSEC_SEQ_NUM *seq_num)
{
    STATUS          status;
    UINT8 HUGE      *curr_data_ptr;
    UINT8           ip_buff[IP_HEADER_LEN];
    UINT8           digest_buff[IPSEC_AH_MAX_AUTHDATA_LEN];
    UINT16          ipsec_auth_algo_id;
    UINT16          ip_hlen;
    UINT16          buf_capacity;
    AHLAYER         *ah_dgram;
    NET_BUFFER      *buf_ptr = buffer;
    NET_BUFFER      *temp_buf = NU_NULL;
    IPSEC_AUTH_REQ  request;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (*ip == NU_NULL) || (in_sa == NU_NULL))
        return (IPSEC_INVALID_PARAMS);

#endif

    /* First get the auth algo ID from the SA. */
    ipsec_auth_algo_id = in_sa->ipsec_security.ipsec_auth_algo;

    /* Get the IP header length. */
    ip_hlen =
        (UINT16)((GET8(*ip, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f) << 2);

    /* First check that AH header is present in this buffer or is it
       in the next buffer. */
    if(buf_ptr->data_len >=
                        (UINT32)(IPSEC_AH_HDR_LEN(ipsec_auth_algo_id)))
    {
        /* AH header is present in the current buffer. Get it. */
        ah_dgram = (AHLAYER *)buf_ptr->data_ptr;

        /* Check the length of AH header */
        if(((ah_dgram->ah_pload_len + 2) << 2) ==
                                    IPSEC_AH_HDR_LEN(ipsec_auth_algo_id))
        {
            /* Take backup of IP header before clearing the mutable
               fields. */
            NU_BLOCK_COPY(ip_buff, *ip, ip_hlen);

            /* Clearing up the mutable fields so that digest can be
               calculated over the whole datagram. */
            IPSEC_Clear_Mutable_Fields_IPv4(*ip);

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

            /* Set the auth algo to be used. */
            request.ipsec_hash_algo = ipsec_auth_algo_id;

            /* Set the digest length required. */
            request.ipsec_digest_len    =
                            IPSEC_GET_AUTH_DIGEST_LEN(ipsec_auth_algo_id);

            /* Assign the pointer for the digest to be stored. */
            request.ipsec_digest  =
                                (UINT8 *)ah_dgram + IPSEC_AH_HDRFIXED_LEN;

            /* Store the incoming digest for future comparison. */
            NU_BLOCK_COPY(digest_buff,
                         (UINT8 *)ah_dgram + IPSEC_AH_HDRFIXED_LEN,
                         request.ipsec_digest_len);

            /* Now clear the digest from the packet as well. */
            UTL_Zero(((UINT8 *)ah_dgram + IPSEC_AH_HDRFIXED_LEN),
                     request.ipsec_digest_len);

            /* Do include the IP header length. We will exclude it
               after calculating the digest over the whole packet. */
            buf_ptr->data_len   += ip_hlen;

            /* Store the current data pointer. */
            curr_data_ptr       = buf_ptr->data_ptr;

            /* Set the data pointer with the IP header. */
            buf_ptr->data_ptr   = (UINT8 *)*ip;

            /* Now calculate the digest. */
            status = IPSEC_Calculate_Digest(buf_ptr, &request);

            /* Set back the data pointer as well. */
            buf_ptr->data_ptr   = curr_data_ptr;

            /* Decrement the buffer length by IP header length. */
            buf_ptr->data_len   -= ip_hlen;

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
                    PUT8(ip_buff, IP_PROTOCOL_OFFSET, ah_dgram->ah_nhdr);

                    /* Now set the IP header to its original values. */
                    NU_BLOCK_COPY(buf_ptr->data_ptr +
                        (IPSEC_AH_HDR_LEN(ipsec_auth_algo_id) - ip_hlen),
                                 ip_buff, ip_hlen);

                    /* Also update the passed IP pointer. */
                    *ip = (IPLAYER *)(buf_ptr->data_ptr +
                        (IPSEC_AH_HDR_LEN(ipsec_auth_algo_id) - ip_hlen));

                    /* Update the data pointer also i.e. point to the next
                       protocol header. */
                    buf_ptr->data_ptr +=
                                    IPSEC_AH_HDR_LEN(ipsec_auth_algo_id);

                    /* Decrement the data length. */
                    buf_ptr->data_len -=
                                    IPSEC_AH_HDR_LEN(ipsec_auth_algo_id);

                    /* Now decrement the total data length of the packet
                       as well. */
                    buf_ptr->mem_total_data_len -=
                                    IPSEC_AH_HDR_LEN(ipsec_auth_algo_id);
                }
            }
            else
            {
                NLOG_Error_Log("Failed to calculate digest",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Discard packet. */
                status = IPSEC_PKT_DISCARD;
            }
        }
        else
        {
            NLOG_Error_Log("AH header length check failed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Headers are not of valid length, discard packet here. */
            status = IPSEC_PKT_DISCARD;
        }
    }
    else
    {
        NLOG_Error_Log("AH header not present",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* AH header not present. */
        status = IPSEC_PKT_DISCARD;
    }

    /* Return the status variable. */
    return (status);

} /* IPSEC_Decode_IPv4_AH */

/************************************************************************
* FUNCTION
*
*        IPSEC_Clear_Mutable_Fields_IPv4
*
* DESCRIPTION
*
*        This function clear out the IPv4 header mutable fields.
*
* INPUTS
*
*        *ip                    Pointer to IP header.
*
* OUTPUTS
*
*        NU_SUCCESS             On successful operation.
*
************************************************************************/
VOID IPSEC_Clear_Mutable_Fields_IPv4(IPLAYER *ip)
{

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if(ip == NU_NULL)
    {
        return;
    }

#endif

    /* Now zero out the mutable fields of the IP header. Starting
       with type of service field. */
    PUT8(ip, IP_SERVICE_OFFSET, 0);

    /* Zero out the fragment offset field. */
    PUT16(ip, IP_FRAGS_OFFSET, 0);

    /* Zero out the time to live field. */
    PUT8(ip, IP_TTL_OFFSET, 0);

    /* Zero out the header checksum field. */
    PUT16(ip, IP_CHECK_OFFSET, 0);

    return;

} /* IPSEC_Clear_Mutable_Fields_IPv4 */

#endif /* #if (IPSEC_INCLUDE_AH == NU_TRUE) */

#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */

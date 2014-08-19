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
*       ips_esp.c
*
* COMPONENT
*
*       ESP (Encapsulation Security Payload)
*
* DESCRIPTION
*
*       Common implementation of IPsec Encapsulation Security Payload
*       component for both IPv4 and IPv6 protocols.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Build_ESP_Hdr
*       IPSEC_Get_ESP_Trailer
*       IPSEC_Encode_ESP
*       IPSEC_Decode_ESP
*       IPSEC_Alloc_Buffer
*
* DEPENDENCIES
*
*       evp.h
*       nu_net.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ips_esn.h"
#include "openssl/evp.h"
#include "networking/ips_externs.h"

#if (IPSEC_INCLUDE_ESP == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Build_ESP_Hdr
*
* DESCRIPTION
*
*       This routine builds and inserts ESP header between IP
*       header and upper layer protocol headers.
*
* INPUTS
*
*       **buffer                NET Buffer list holding the IPv4 or IPv6
*                               datagram.
*       *ip_layer               IPv4 or IPv6 header.
*       *out_sa                 Outbound SA to be applied.
*       *iv_len                 IV length.
*       ip_hlen                 IPv4 or IPv6 header length.
*
* OUTPUTS
*
*       *iv_len                 Initialization vector length of the
*                               encryption algorithm.
*       NU_SUCCESS              On successful operation.
*       NU_NO_BUFFERS           Buffers not available.
*       IPSEC_INVAL_SEQ_NO      Current Sequence number is not valid.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Build_ESP_Hdr(NET_BUFFER **buffer, VOID *ip_layer,
                           IPSEC_OUTBOUND_SA *out_sa, UINT8 *iv_len,
                           UINT16 ip_hlen)
{
    STATUS          status = NU_SUCCESS;
    UINT8           encrypt_algo_index;
    ESP_HDR         *esp_hdr = NU_NULL;
    NET_BUFFER      *hdr_buf;
    NET_BUFFER      *buf_ptr;
    const EVP_CIPHER*cipher_type;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (ip_layer == NU_NULL) ||
       (out_sa == NU_NULL) || (iv_len   == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    encrypt_algo_index = out_sa->ipsec_security.ipsec_encryption_algo;
    buf_ptr = *buffer;

    /* First increment the sequence number. as it is unique for each
       ESP header. */
    IPSEC_SEQ_INC(out_sa->ipsec_seq_num);

    /* Check the sequence no. if it is fine to transmit the current
       packet with incremented sequence number */
    if( IPSEC_SEQ_HAS_CYCLED(out_sa->ipsec_seq_num) )
    {

#if (IPSEC_ANTI_REPLAY == NU_TRUE)

        /* The counter has been cycled so discard the packet and log
           the error. See RFC 2402 section 2.5. */
        NLOG_Error_Log("The SA sequence no. has been cycled",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

#endif

        /* Mark the status with error code. */
        status = IPSEC_INVAL_SEQ_NO;
    }

    else
    {

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)

        /* If NULL encryption is requested. */
        if(encrypt_algo_index == IPSEC_NULL_CIPHER)
            *iv_len = 0;                    /* Required by RFC 2410. */
        else

#endif
        {
            /* Get the IV length which is always equal to the block size
               of the encryption algorithm. */
            cipher_type = (EVP_CIPHER*)IPSEC_Encryption_Algos[out_sa->ipsec_encrypt_req->ipsec_algo_index].evp_cipher();
            *iv_len = EVP_CIPHER_iv_length(cipher_type);
        }

        /* Check if there is any space for ESP buffer immediately after
         * IP header i.e. if only IP header is present in the parent
         * buffer.
         */
        if(buf_ptr->data_len != ip_hlen)
        {
            /*
             * Since there is already some upper layer data present ,we
             * will get a new buffer for IP and ESP header and the
             * current parent buffer will just contain upper layer data.
             */
            hdr_buf = IPSEC_Alloc_Buffer(buf_ptr, ip_layer, ip_hlen);

            /* If a buffer has been allocated. */
            if(hdr_buf != NU_NULL)
            {

                /* Set the data length of the buffer. */
                hdr_buf->data_len = ip_hlen +
                                      IPSEC_ESP_HDRFIXED_LEN + (*iv_len);

                /* Increment the total data length of the buffer by ESP
                   header. */
                hdr_buf->mem_total_data_len += (IPSEC_ESP_HDRFIXED_LEN +
                                               (*iv_len));

                /* Also update the parent buffer as well. */
                *buffer = hdr_buf;

                /* Get a pointer to the ESP header. */
                esp_hdr = (ESP_HDR *)(hdr_buf->data_ptr + ip_hlen);
            }
            else
            {
                /* Unable to get the buffer. */
                status = NU_NO_BUFFERS;
            }
        }
        else
        {
            /* Space is available for ESP header. */

            /* Get a pointer to the ESP header. */
            esp_hdr = (ESP_HDR *)(buf_ptr->data_ptr + ip_hlen);

            /* Increment the data length of the parent buffer. */
            buf_ptr->data_len += (IPSEC_ESP_HDRFIXED_LEN + (*iv_len));

            /* Increment the total length of the buffer as well. */
            buf_ptr->mem_total_data_len +=
                                 (IPSEC_ESP_HDRFIXED_LEN + (*iv_len));
        }

        if(status != NU_NO_BUFFERS)
        {
            /* Now make up the ESP header. First specify the SPI. */
            PUT32(esp_hdr, IPSEC_ESP_SPI_OFFSET, out_sa->ipsec_remote_spi);

            /* Set the sequence no. */
            PUT32(esp_hdr, IPSEC_ESP_SEQNO_OFFSET, out_sa->ipsec_seq_num.ipsec_low_order);
        }
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Build_ESP_Hdr */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_ESP_Trailer
*
* DESCRIPTION
*
*       This routine calculates the ESP padding length and initialize
*       padding. It also return the ESP trailer to the caller.
*
* INPUTS
*
*       *buf_ptr                NET Buffer list holding the IPv4 or IPv6
*                               datagram.
*       ip_hlen                 IPv4 or IPv6 header length.
*       iv_len                  IV length.
*       auth_algo               Index of the authentication algorithm to
*                               be applied.
*
* OUTPUTS
*
*       **esp_trailer           ESP trailer.
*       NU_SUCCESS              On successful operation.
*       NU_NO_BUFFERS           Buffers not available.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_ESP_Trailer(NET_BUFFER *buf_ptr, UINT16 ip_hlen,
                             UINT8 iv_len, UINT8 auth_algo,
                             ESP_TRAILER **esp_trailer)
{
    STATUS              status = NU_SUCCESS;
    UINT8               i;
    UINT8               pad_len;
    UINT16              data_len;
    UINT16              buf_capacity;
    UINT32              iv_mask;
    NET_BUFFER          *temp_buf;
    NET_BUFFER          *pad_buf;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (esp_trailer == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /*
     * Now before encryption and authentication processes first
     * check for padding requirements. We will first check for the
     * CBC mode algorithms input requirements and then for the pad
     * length field/next header and authentication data field
     * alignments requirements.
     */

    /*
     * We will use IV length for payload padding alignment purposes
     * since IV length is equal to block size of the CBC encryption
     * algorithm. The plain text should be integral multiple of block
     * size of the encryption algorithm.
     */

    /* IV length will be zero in case of NULL encryption algorithm. */
    if(iv_len != 0)
    {
        iv_mask = (UINT32)iv_len;
        iv_mask--;

        /* Calculate the required pad length. */
        pad_len = (UINT8)
            (((buf_ptr->mem_total_data_len +(UINT8)sizeof(ESP_TRAILER)) -
            (ip_hlen + IPSEC_ESP_HDRFIXED_LEN + iv_len)) & iv_mask);

        /* Calculate the no. of bytes to be padded. */
        if((pad_len != 0) && (pad_len < IPSEC_ESP_MAX_IV))
        {
            pad_len = iv_len - pad_len;
        }
    }
    else
    {
        iv_mask = (UINT32)4;
        iv_mask--;

        /* 4 bytes alignment of ESP trailer is required in case of
           NULL encryption */
        pad_len = (UINT8)((buf_ptr->mem_total_data_len +
                                       sizeof(ESP_TRAILER)) & iv_mask);

        if(pad_len != 0)
        {
            /* Calculate the pad length required. */
            pad_len = 4 - pad_len;
        }
    }

    /*
     * Padding needs to be added. First traverse to the last
     * buffer in the buffer chain.
     */
    for(temp_buf = buf_ptr;
        temp_buf->next_buffer != NU_NULL;
        temp_buf = temp_buf->next_buffer)
        ;

    /*
     * Check if there is any space available in the last buffer for
     * padding, otherwise get a new buffer.
     */

    /* Calculate total data length. */
    data_len = (UINT16)(temp_buf->data_len + pad_len +
                sizeof(ESP_TRAILER) + IPSEC_ESP_AUTHDATA_LEN(auth_algo));

    /* Calculate the capacity of the buffer. */
    buf_capacity = (temp_buf->mem_flags & NET_PARENT) ?
                                          NET_PARENT_BUFFER_SIZE :
                                          NET_MAX_BUFFER_SIZE;

    if(data_len <= buf_capacity)
    {
        /* Padding will be added to the last buffer. */
        pad_buf = temp_buf;
    }
    else
    {
        /* We need a new buffer for padding. */
        pad_buf = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

        /* Check if we got any free buffer. */
        if(pad_buf != NU_NULL)
        {
            /* Set the data pointer. */
            pad_buf->data_ptr  = pad_buf->mem_packet;

            /* Set the list to which this buffer will be freed. */
            pad_buf->mem_dlist = &MEM_Buffer_Freelist;

            /* Add this buffer to the packet chain. */
            temp_buf->next_buffer = pad_buf;
        }
        else
        {
            /* Buffers not available. */
            status = NU_NO_BUFFERS;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Default processing (RFC 2406) for pad contents is going
           to be used. */
        for(i = 0; i < pad_len; i++)
        {
            /* Fill the buffer with the incrementing
               pad values according to the required pad length. */
            pad_buf->data_ptr[pad_buf->data_len + i] = (i + 1);
        }

        /* Get the pointer to the ESP trailer. */
        *esp_trailer = (ESP_TRAILER*)
                    (pad_buf->data_ptr + (pad_buf->data_len + pad_len));

        /* First set the total length of the pad. */
        PUT8(*esp_trailer, IPSEC_ESP_PADLEN_OFFSET, (UINT8)pad_len);

        /* Increment the buffer length and total packet length also.*/
        pad_buf->data_len += (pad_len + sizeof(ESP_TRAILER));
    }

    return (status);

} /* IPSEC_Get_ESP_Trailer */

/************************************************************************
* FUNCTION
*
*       IPSEC_Encode_ESP
*
* DESCRIPTION
*
*       This routine encrypt and calculate authentication digest and
*       assign IV and digest to ESP headers.
*
* INPUTS
*
*       *buf_ptr                NET Buffer list holding the IPv4 or IPv6
*                               datagram.
*       ip_hlen                 IPv4 or IPv6 header length.
*       iv_len                  IV length.
*       IPSEC_OUTBOUND_SA       Security Association to be applied.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful ESP encoding.
*       NU_NO_BUFFERS           Buffers not available.
*       IPSEC_PKT_DISCARD       Packet has been discarded.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Encode_ESP(NET_BUFFER *buf_ptr, UINT16 ip_hlen, UINT8 iv_len,
                        IPSEC_OUTBOUND_SA *out_sa)
{
    STATUS              status = NU_SUCCESS;
    UINT8               auth_algo_index;
    IPSEC_AUTH_REQ      auth_req;
    NET_BUFFER          *temp_buf;
    UINT8               digest[IPSEC_ESP_MAX_AUTHDATA_LEN];

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    UINT8               encrypt_algo_index;
#endif

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (out_sa == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#if(IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    else if((IPSEC_GET_ENCRYPT_ID(out_sa->ipsec_security.ipsec_encryption_algo) \
                                                          == IPSEC_NULL_CIPHER) &&
                     (IPSEC_GET_AUTH_ID(out_sa->ipsec_security.ipsec_auth_algo) \
                                                          == IPSEC_NULL_AUTH))
    {
        /*
         * Note that although both confidentiality and authentication are optional,
         * at least one of these services MUST be selected hence both algorithms
         * MUST NOT be simultaneously NULL.(see rfc2406 section 3.2 Algorithms).
         */
        return (IPSEC_INVALID_PARAMS);
    }
#endif
#endif

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    /* Get encryption algorithm index. */
    encrypt_algo_index = out_sa->ipsec_security.ipsec_encryption_algo;
#endif

    /* Get authentication algorithm index. */
    auth_algo_index = out_sa->ipsec_security.ipsec_auth_algo;

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)

    /* No need to go further if it is null cipher algorithm.*/
    if(encrypt_algo_index != IPSEC_NULL_CIPHER)
    {

#endif

        /* IP and ESP header (present in the parent buffer) should
           not be encrypted. */
        out_sa->ipsec_encrypt_req->buffer =
            (UINT8*)buf_ptr->next_buffer->data_ptr;

        out_sa->ipsec_encrypt_req->text_len =
            (UINT16)buf_ptr->next_buffer->data_len;

        /* Now call the encryption function. */
        status =
            IPSEC_Cipher_Operation(buf_ptr->next_buffer,
                                   out_sa->ipsec_encrypt_req,
                                   ((UINT8*)(buf_ptr->data_ptr + ip_hlen +
                                   IPSEC_ESP_HDRFIXED_LEN)), iv_len);

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
    }
#endif

    /* Check the status. */
    if((status == NU_SUCCESS)
#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
        && (auth_algo_index != IPSEC_NULL_AUTH)
#endif
        )
    {
        /* Now build the authentication request. */

        /* Set the authentication key. */
        auth_req.ipsec_key = out_sa->ipsec_auth_key;

        /* Set the authentication key length. */
        auth_req.ipsec_key_len = IPSEC_GET_AUTH_KEY_LEN(auth_algo_index);

        /* Set the authentication algorithm. */
        auth_req.ipsec_hash_algo = auth_algo_index;

        /* Set the digest length required. */
        auth_req.ipsec_digest_len =
                                IPSEC_ESP_AUTHDATA_LEN(auth_algo_index);

        /* Traverse to the last buffer in the chain. */
        for(temp_buf = buf_ptr; temp_buf->next_buffer != NU_NULL;
            temp_buf = temp_buf->next_buffer)
            ;

        /* If ESN is enabled, then we need to place the higher 32-bits.
         * of the sequence number for the digest calculation. This
         * will be removed below.
         */
        if ( out_sa->ipsec_seq_num.ipsec_is_esn )
        {
            PUT32( (UINT8*)temp_buf->data_ptr, temp_buf->data_len,
                   out_sa->ipsec_seq_num.ipsec_high_order );

            temp_buf->data_len += sizeof (UINT32);
        }

        /* Assign the pointer for the digest to be stored. */
        auth_req.ipsec_digest = digest;

        /* Temporarily exclude the IP header. */
        buf_ptr->data_ptr       += ip_hlen;
        buf_ptr->data_len       -= ip_hlen;

        /* Now calculate the digest. */
        status = IPSEC_Calculate_Digest(buf_ptr, &auth_req);

        if(status == NU_SUCCESS)
        {
            /* Include back IP header length and the digest
               length. */
            buf_ptr->data_len += ip_hlen;
            buf_ptr->data_ptr -= ip_hlen;

            /* Remove the higher order 32-bit sequence number. If ESN is
             * enabled.
             */
            if ( out_sa->ipsec_seq_num.ipsec_is_esn )
            {
                temp_buf->data_len -= sizeof (UINT32);
            }

            /* Now copy the digest. */
            NU_BLOCK_COPY ( (UINT8*)temp_buf->data_ptr +
                               (INT)temp_buf->data_len,
                             digest,
                             IPSEC_ESP_AUTHDATA_LEN(auth_algo_index) );

            /* Include the placed digest length as well. */
            temp_buf->data_len += IPSEC_ESP_AUTHDATA_LEN(auth_algo_index);

            /* Now increment the data length of the last buffer by
               the digest length as packet now contains
               authentication data. */
            buf_ptr->mem_total_data_len +=
                          IPSEC_ESP_AUTHDATA_LEN(auth_algo_index);
        }
    }

    return (status);

} /* IPSEC_Encode_ESP */

/************************************************************************
* FUNCTION
*
*       IPSEC_Decode_ESP
*
* DESCRIPTION
*
*       This function decodes the passed NET buffer chain encoded
*       with ESP protocol according to the parameters supplied through
*       the given SA.
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
STATUS IPSEC_Decode_ESP(NET_BUFFER *buf_ptr, IPSEC_INBOUND_SA *in_sa,
                        ESP_TRAILER *esp_tail, UINT8 *esp_total_hdr_len,
                        IPSEC_SEQ_NUM *seq_num)
{
    STATUS          status = NU_SUCCESS;
    UINT8           i, j;
    UINT8           mine_auth_data[IPSEC_ESP_MAX_AUTHDATA_LEN];
    UINT8           theirs_auth_data[IPSEC_ESP_MAX_AUTHDATA_LEN];
    UINT8           iv_len;
    UINT8           encrypt_algo_index ;
    UINT8           auth_algo_index;
    UINT8           *pad_ptr;
    UINT32          padding_start_idx = 0;
    UINT32          tmp_len = 0;
    NET_BUFFER      *last_buff = buf_ptr;
    NET_BUFFER      *temp_buff = NU_NULL;
    IPSEC_AUTH_REQ  auth_req;
    const EVP_CIPHER *cipher_type;
    UINT32          esp_len=0; /* Number of bytes from first byte of esp[spi] to last byte of esp[auth_data]*/

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (in_sa == NU_NULL) || (esp_tail == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#if ((IPSEC_INCLUDE_NULL_AUTH == NU_TRUE) && \
                                (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE))
    else if((IPSEC_GET_ENCRYPT_ID(in_sa->ipsec_security.ipsec_encryption_algo) \
                                                           == IPSEC_NULL_CIPHER) &&
                      (IPSEC_GET_AUTH_ID(in_sa->ipsec_security.ipsec_auth_algo) \
                                                    == IPSEC_NULL_AUTH))
    {
        /*
         * Note that although both confidentiality and authentication are optional,
         * at least one of these services MUST be selected hence both algorithms
         * MUST NOT be simultaneously NULL.(see rfc2406 section 3.2 Algorithms).
         */
        return (IPSEC_INVALID_PARAMS);
    }

#endif
#endif

    /* Get encryption algorithm index. */
    encrypt_algo_index = in_sa->ipsec_security.ipsec_encryption_algo;

    /* Get authentication algorithm index. */
    auth_algo_index =  in_sa->ipsec_security.ipsec_auth_algo;

    /* Add length of data in buffer to esp length. */
    esp_len += buf_ptr->data_len;

    /* Traverse to the last buffer of the incoming packet. */
    while(last_buff->next_buffer != NU_NULL)
    {
        temp_buff = last_buff;
        last_buff = last_buff->next_buffer;

        /* Add length of data in buffer to esp length. */
        esp_len += last_buff->data_len;
    }

#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
    /* If null authentication algorithm is not present. */
    if(auth_algo_index != IPSEC_NULL_AUTH)
#endif
    {
        /* Now build the authentication request for authenticator. */

        /* Set the authentication key. */
        auth_req.ipsec_key      = in_sa->ipsec_auth_key;

        /* Set the authentication key length. */
        auth_req.ipsec_key_len  = IPSEC_GET_AUTH_KEY_LEN(auth_algo_index);

        /* Set the authentication algorithm to be used. */
        auth_req.ipsec_hash_algo = auth_algo_index;

        /* Assign the pointer for the digest to be stored. */
        auth_req.ipsec_digest   = mine_auth_data;

        /* Set the digest length required. */
        auth_req.ipsec_digest_len =
                                IPSEC_ESP_AUTHDATA_LEN(auth_algo_index);

        /* Check if incoming digest is divided in two buffers. */
        if(last_buff->data_len <
                           (UINT32)IPSEC_ESP_AUTHDATA_LEN(auth_algo_index)
                           && temp_buff != NU_NULL)
        {
            /* Store the difference in a temporary variable */
            tmp_len = IPSEC_ESP_AUTHDATA_LEN(auth_algo_index) -
                      last_buff->data_len;

            /* Exclude the authentication data (digest) from the last buffer. */
            last_buff->data_len = 0;

            /* Also exclude the first part of digest from the second last
               buffer. */
            temp_buff->data_len -= tmp_len;

            /* Now copy the first part of received digest. */
            NU_BLOCK_COPY ( theirs_auth_data,
                            temp_buff->data_ptr + temp_buff->data_len,
                            tmp_len );

            /* Copy the second part of the digest. */
            NU_BLOCK_COPY ( theirs_auth_data + tmp_len,
                            last_buff->data_ptr,
                            IPSEC_ESP_AUTHDATA_LEN(auth_algo_index) -
                            tmp_len );
        }
        else
        {
            /* Do exclude authentication data (digest) from the last buffer. */
            last_buff->data_len -=
                                IPSEC_ESP_AUTHDATA_LEN(auth_algo_index);

            /* Copy the received digest. */
            NU_BLOCK_COPY ( theirs_auth_data,
                            last_buff->data_ptr + last_buff->data_len,
                            IPSEC_ESP_AUTHDATA_LEN(auth_algo_index) );
        }

        /* In case of ESN, for the purpose of our calculation, we will
         * need to put the higher order 32-bits in to the message. It
         * is obvious that the last buffer will have at least 4 bytes
         * at the end. We will use this space to put our sequence number.
         */
        if ( seq_num->ipsec_is_esn )
        {
            PUT32( last_buff->data_ptr,
                   last_buff->data_len,
                   seq_num->ipsec_high_order );

            last_buff->data_len += sizeof ( UINT32 );
        }

        /* Now calculate the digest. */
        status = IPSEC_Calculate_Digest(buf_ptr, &auth_req);

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* For ESN, first remove the higher order 32-bits of the
             * sequence numbers.
             */
            if ( seq_num->ipsec_is_esn )
            {
                last_buff->data_len -= sizeof ( UINT32 );
            }

            /* Compare the two digests. */
            if(memcmp( mine_auth_data,
                       theirs_auth_data,
                       IPSEC_ESP_AUTHDATA_LEN(auth_algo_index) ) != 0)
            {
                /* Two digests are not equal, hence unauthenticated
                 * packet.
                 */
                status = IPSEC_INVALID_DIGEST;
            }

            if(last_buff->data_len == 0)
            {
                /* Now we need to update last buffer pointer as well. */
                last_buff = temp_buff;
            }

        } /* End of 'if(status == NU_SUCCESS)'. */
    }
#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
    else
    {
        /* Clear the digest length of the null authentication request. */
        auth_req.ipsec_digest_len = 0;
    }
#endif
    /* Check if packet has been authenticated and whether it needs to
       be decrypted or not. */
    if(status == NU_SUCCESS)
    {
        /*
         * Since packet has been authenticated, go ahead with the
         * decryption process.
         */
#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
        /* If null encryption is being used. */
        if(encrypt_algo_index == IPSEC_NULL_CIPHER)
        {
            iv_len = 0;

            /* As there is no IV present. */
            *esp_total_hdr_len = IPSEC_ESP_HDRFIXED_LEN;
        }
        else
        {
#endif
            /* Get the IV length which is always equal to the block size
               of the encryption algorithm. */
            cipher_type = (EVP_CIPHER*)IPSEC_Encryption_Algos[in_sa->ipsec_encrypt_req->ipsec_algo_index].evp_cipher();
            iv_len = EVP_CIPHER_iv_length(cipher_type);

            /* Calculate the total ESP header length. */
            *esp_total_hdr_len = IPSEC_ESP_HDRFIXED_LEN + iv_len;

            /* Build the encryption request structure. */
            in_sa->ipsec_encrypt_req->buffer =
                        (UINT8*)buf_ptr->data_ptr + *esp_total_hdr_len;
            in_sa->ipsec_encrypt_req->text_len =
                        (UINT16)buf_ptr->data_len - *esp_total_hdr_len;

            /* Now call the encryption function. */
            status = IPSEC_Cipher_Operation(buf_ptr,
                                            in_sa->ipsec_encrypt_req,
                 (in_sa->ipsec_encrypt_req->buffer - iv_len), iv_len);

#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
        }
#endif
        /* Check the status. */
        if(status == NU_SUCCESS && last_buff != NU_NULL)
        {
            /* If the ESP trailer is divided between last and second last
               buffer. */
            if(last_buff->data_len < sizeof(ESP_TRAILER) &&
                                                temp_buff != NU_NULL)
            {
                /* First byte will always contain 'next header and Last
                 * byte of second last buffer will always contain
                 * 'pad length'.
                 */
                esp_tail->esp_nhdr       = GET8(last_buff->data_ptr, 0);
                esp_tail->esp_pad_length = GET8(temp_buff->data_ptr,
                                         (UINT16)temp_buff->data_len - 1);
            }
            else
            {
                /* Complete ESP trailer is present in last buffer. */
                esp_tail->esp_pad_length = GET8(last_buff->data_ptr,
                       (UINT16)last_buff->data_len - sizeof(ESP_TRAILER));

                esp_tail->esp_nhdr       = GET8(last_buff->data_ptr,
                                         (UINT16)last_buff->data_len - 1);
            }

            /*
             * Inspect the padding contents according to the default
             * padding scheme (RFC 2406 - In case no special padding
             * scheme is dictated by the employing algorithm).
             */

            if (esp_tail->esp_pad_length > 0)
            {
                padding_start_idx = esp_len - IPSEC_ESP_AUTHDATA_LEN(auth_algo_index) -
                                    sizeof(ESP_TRAILER) - esp_tail->esp_pad_length;

                /* We now need to parse the entire buffer chain to get to the
                 * beginning of the Padding data
                 */

                /* Start from the beginning of the buffer chain */
                temp_buff = buf_ptr;

                /* Reinitialize the temporary length variable.
                 * Use this now to keep track of the size of data preceding
                 * padding.
                 */
                tmp_len = 0;

                /* Traverse the buffer chain to locate the beginning of the padding data. */
                while(temp_buff->next_buffer != NU_NULL)
                {
                    if ((tmp_len + temp_buff->data_len) > padding_start_idx)
                    {
                        /* Padding data found */
                        break;
                    }
                    else
                    {
                        /* Look for padding data in the next buffer */
                        tmp_len += temp_buff->data_len;
                        temp_buff = temp_buff->next_buffer;
                    }
                }

                /* Get a pointer to the padding data */
                pad_ptr = (UINT8*)temp_buff->data_ptr +
                          (padding_start_idx - tmp_len);

                /* Initialize the padding data iterator */
                i = 0;

                 /* Start verifying the padding contents. */
                for(;
                    ((padding_start_idx - tmp_len) < temp_buff->data_len) &&
                            (i < esp_tail->esp_pad_length);
                    i++, padding_start_idx++
                   )
                {
                    /* Checking from the start of the padding. */
                    if(pad_ptr[i] != (i + 1))
                    {
                        /* Padding has been corrupted,
                           discard the packet. */
                        status = IPSEC_PKT_DISCARD;
                        break;
                    }
                }

                if (status == NU_SUCCESS)
                {
                    /* Exclude unwanted data from the packet. */
                    if (i < esp_tail->esp_pad_length)
                    {
                        /* This buffer contains only padding data, exclude it */
                        temp_buff->data_len -= i;
                    }
                    else
                    {
                        /* This buffer contains padding data and ESP trailer, exclude them */
                        temp_buff->data_len -= (i + sizeof(ESP_TRAILER));
                    }

                    /* See if there is more padding data to be read */
                    while (i < esp_tail->esp_pad_length)
                    {
                        /* Seems like there is more padding data to be parsed,
                         * move on to the next buffer to get it.
                         */
                        temp_buff = temp_buff->next_buffer;

                        /* Since this is not the first buffer containing padding data, the
                         * padding data in this buffer would start from the first data byte
                         */
                        pad_ptr = (UINT8*)temp_buff->data_ptr;

                        /* Verify the padding data. */
                        for (j = 0;
                             ((j < temp_buff->data_len) && ((i + j) < esp_tail->esp_pad_length)) != NU_NULL;
                             j++
                            )
                        {
                            /* Checking from the start of the padding. */
                            if(pad_ptr[j] != (i + j + 1))
                            {
                                /* Padding has been corrupted,
                                   discard the packet. */
                                status = IPSEC_PKT_DISCARD;
                                break;
                            }
                        }

                        if (status == NU_SUCCESS)
                        {
                            /* Update the padding data iterator */
                            i += j;

                            /* Exclude unwanted data from the packet. */
                            if (i < esp_tail->esp_pad_length)
                            {
                                /* This buffer contains only padding data, exclude it */
                                temp_buff->data_len -= j;
                            }
                            else
                            {
                                /* This buffer contains padding data and ESP trailer, exclude them */
                                temp_buff->data_len -= (j + sizeof(ESP_TRAILER));
                            }
                        }
                    }
                }
            }
            else
            {
                /* There is no padding data in this buffer, exclude the ESP trailer */
                last_buff->data_len -= sizeof(ESP_TRAILER);
            }
            
            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to validate ESP padding",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Deallocate all the buffers that are empty */
                for (last_buff = buf_ptr; last_buff; last_buff = last_buff->next_buffer)
                {
                    /* If there is a buffer after the current one and that buffer contains
                       no data then free that buffer and any that follow it. */
                    if (last_buff->next_buffer && (last_buff->next_buffer->data_len == 0))
                    {
                        MEM_One_Buffer_Chain_Free (last_buff->next_buffer,
                                                   &MEM_Buffer_Freelist);
                        last_buff->next_buffer = NU_NULL;
                    }
                }
            }
        }
        else
        {
            NLOG_Error_Log("Failed to decrypt ESP packet",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Discard packet. */
            status = IPSEC_PKT_DISCARD;
        }
    }
    else
    {
        NLOG_Error_Log("Failed to authenticate ESP packet",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Discard packet. */
        status = IPSEC_PKT_DISCARD;
    }

    return (status);

} /* IPSEC_Decode_ESP */

#endif /* #if (IPSEC_INCLUDE_ESP == NU_TRUE) */

/************************************************************************
* FUNCTION
*
*       IPSEC_Alloc_Buffer
*
* DESCRIPTION
*
*       This function adds a new head to NET buffer chain and copies
*       buffer header to newly added head. It also copies the IPv4 or
*       IPv6 header to the newly allocated head.
*
* INPUTS
*
*       *buf_ptr                NET buffer chain to which a new head is to
*                               added.
*       *ip_layer               Pointer to IPv4 or IPv6 header.
*       ip_hlen                 IPv4 or IPv6 header length.
*
* OUTPUTS
*
*       *NET_BUFFER             On success.
*       *NU_NULL                On failure.
*
************************************************************************/
NET_BUFFER* IPSEC_Alloc_Buffer(NET_BUFFER *buf_ptr, VOID *ip_layer,
                               UINT16 ip_hlen)
{
    NET_BUFFER          *hdr_buf;

    /*
     * Since there is already some upper layer data present ,we
     * will get a new buffer for IP and ESP header and the
     * current parent buffer will just contain upper layer data.
     */
    hdr_buf = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    /* If a buffer has been allocated. */
    if(hdr_buf != NU_NULL)
    {
        /* First put the device pointer into the buffer header,
         * as there will be need for the associated device
         * length.
         */
        hdr_buf->mem_buf_device = buf_ptr->mem_buf_device;

        /* Set the data pointer to the beginning of the packet
           leaving behind the space for the link layer header. */
        hdr_buf->data_ptr   = hdr_buf->mem_parent_packet +
                              hdr_buf->mem_buf_device->dev_hdrlen;

        hdr_buf->mem_total_data_len = buf_ptr->mem_total_data_len;
        hdr_buf->mem_dlist          = &MEM_Buffer_Freelist;
        hdr_buf->mem_flags          |= NET_PARENT;

        /* Prefix the new buffer to the list as parent.
            i.e. IP -> ESP -> upper layer buffers. */
        hdr_buf->next_buffer   = buf_ptr;

        /* Copy IP header to the newly created parent buffer. */
        NU_BLOCK_COPY(hdr_buf->data_ptr, ip_layer, ip_hlen);

        /* Make sure the buffer pointer is aligned. */
        if(((UNSIGNED)(buf_ptr->data_ptr + ip_hlen)) &
                                      ((UINT32)REQ_ALIGNMENT - 1))
        {
            NU_BLOCK_COPY(buf_ptr->mem_packet,
                          buf_ptr->data_ptr + ip_hlen,
                          (INT)(buf_ptr->data_len - ip_hlen));

            buf_ptr->data_ptr = buf_ptr->mem_packet;
        }
        else
        {
            /* Also adjust the data pointer of previous
               parent buffer for the upper layer protocol. */
            buf_ptr->data_ptr += ip_hlen;
        }

        /* Update the data length. */
        buf_ptr->data_len      -= ip_hlen;
    }

    return (hdr_buf);

} /* IPSEC_Alloc_Buffer */

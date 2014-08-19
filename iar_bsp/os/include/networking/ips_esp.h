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
*       ips_esp.h
*
* COMPONENT
*
*       ESP (Encapsulation Security Payload)
*
* DESCRIPTION
*
*       Definitions required for implementation of IP Encapsulating
*       Security Payload component.
*
* DATA STRUCTURES
*
*       ESP_HDR
*       ESP_TRAILER
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_ESP_H
#define IPS_ESP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

/* ESP header, defining only fixed length fields. */
typedef struct esp_hdr
{
    UINT32      esp_spi;                    /* Security Parameter index. */
    UINT32      esp_seq_no;                 /* ESP sequence no. */
}ESP_HDR;

typedef struct esp_trailer
{
    UINT8       esp_pad_length;             /* ESP pad length. */
    UINT8       esp_nhdr;                   /* ESP next header field. */
}ESP_TRAILER;

/* ESP header length (in bytes) containing just SPI and sequence no. */
#define IPSEC_ESP_HDRFIXED_LEN      8
#define IPSEC_ESP_TRAILER_LEN       sizeof(ESP_TRAILER)
#define IPSEC_ESP_MAX_PAD           7

/* By default IV length of all the supported CBC cipher algorithms is 8
   bytes and 16 in case of AES. */
#if (IPSEC_INCLUDE_AES == NU_FALSE)
#define IPSEC_ESP_MAX_IV            8
#else
#define IPSEC_ESP_MAX_IV            16
#endif

/* Define the ESP header offsets. */
#define IPSEC_ESP_SPI_OFFSET        0
#define IPSEC_ESP_SEQNO_OFFSET      4
#define IPSEC_ESP_PAYLOAD_OFFSET    8
#define IPSEC_ESP_IV_OFFSET         8       /* As IV is the start of the
                                             * payload as well. */

/* Define the ESP trailer offsets and Authentication data offset. */
#define IPSEC_ESP_PADLEN_OFFSET     0
#define IPSEC_ESP_NHDR_OFFSET       1
#define IPSEC_ESP_AUTHDATA_OFFSET   2

/* Maximum digest length for the ESP protocol. This is maximum possible
   value of the digest for any auth algorithm supported. */
#define IPSEC_ESP_MAX_AUTHDATA_LEN      IPSEC_AH_MAX_AUTHDATA_LEN

/* Authentication algorithm digest length for ESP. */
#define IPSEC_ESP_AUTHDATA_LEN(algo_index) \
        IPSEC_GET_AUTH_DIGEST_LEN(algo_index)

/*** Function Prototypes. */
STATUS IPSEC_Build_ESP_Hdr(NET_BUFFER **buffer, VOID *ip_layer,
                           IPSEC_OUTBOUND_SA *out_sa, UINT8 *iv_len,
                           UINT16 ip_hlen);

STATUS IPSEC_Get_ESP_Trailer(NET_BUFFER *buf_ptr, UINT16 ip_hlen,
                             UINT8 iv_len,
                             UINT8 auth_algo, ESP_TRAILER **esp_trailer);

STATUS IPSEC_Encode_ESP(NET_BUFFER *buf_ptr, UINT16 ip_hlen, UINT8 iv_len,
                        IPSEC_OUTBOUND_SA *out_sa);

STATUS IPSEC_Decode_ESP(NET_BUFFER *buf_ptr, IPSEC_INBOUND_SA *in_sa,
                        ESP_TRAILER *esp_tail, UINT8 *esp_total_hdr_len,
                        IPSEC_SEQ_NUM *seq_num);

NET_BUFFER* IPSEC_Alloc_Buffer(NET_BUFFER *buf_ptr, VOID *ip_layer,
                               UINT16 ip_hlen);

STATUS IPSEC_Encode_IPv4_ESP(NET_BUFFER **buffer, IPLAYER **ip_layer,
                             IPSEC_OUTBOUND_SA *out_sa);

STATUS IPSEC_Decode_IPv4_ESP(NET_BUFFER *buffer, IPLAYER **ip,
                             IPSEC_INBOUND_SA *in_sa,
                             IPSEC_SEQ_NUM *seq_num);
#if (INCLUDE_IPV6 == NU_TRUE)
STATUS IPSEC_Encode_IPv6_ESP(NET_BUFFER **buffer, IP6LAYER **ip6_pkt,
                             IPSEC_OUTBOUND_SA *out_sa);

STATUS IPSEC_Decode_IPv6_ESP(NET_BUFFER *buffer, IP6LAYER **ip6_pkt,
                             UINT8 *next_header, UINT16 *header_len,
                             IPSEC_INBOUND_SA *in_sa,
                             IPSEC_SEQ_NUM *seq_num);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_ESP_H */

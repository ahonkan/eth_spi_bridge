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
*       ips_ah.h
*
* COMPONENT
*
*       AH (Authentication Header)
*
* DESCRIPTION
*
*       Definitions required for implementation of IP Authentication
*       Header protocol.
*
* DATA STRUCTURES
*
*       AHLAYER
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_AH_H
#define IPS_AH_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

/* AH Protocol Header */
typedef struct ahlayer
{
    UINT8   ah_nhdr;                        /* Next header. */
    UINT8   ah_pload_len;                   /* Payload length. */
    UINT16  ah_reserved;                    /* Reserved field. */
    UINT32  ah_spi;                         /* Security parameter index. */
    UINT32  ah_seq_no;                      /* Sequence number. */
}AHLAYER;

/* IPsec AH fixed header length in bytes. */
#define IPSEC_AH_HDRFIXED_LEN           12

/* Maximum possible value of digest data length for any authentication
   algorithm supported. */
#define IPSEC_AH_MAX_AUTHDATA_LEN       12

/* AH Standard case of 96-bit authentication data size in bytes. */
#define IPSEC_AH_AUTHDATA_LEN           12

/* Total AH header size including standard digest length also. */
#define IPSEC_AH_HDR_LEN(algo_id)   (IPSEC_AH_HDRFIXED_LEN + \
                                     IPSEC_GET_AUTH_DIGEST_LEN(algo_id))

/* Define the AH header Offsets. */
#define IPSEC_AH_NHDR_OFFSET            0
#define IPSEC_AH_PLOADLEN_OFFSET        1
#define IPSEC_AH_RSVD_OFFSET            2
#define IPSEC_AH_SPI_OFFSET             4
#define IPSEC_AH_SEQNO_OFFSET           8
#define IPSEC_AH_AUTHDATA_OFFSET        12

/*** Function Prototypes. ***/
STATUS IPSEC_Encode_IPv4_AH(NET_BUFFER **buffer, IPLAYER **ip_layer,
                            IPSEC_OUTBOUND_SA *out_sa);
STATUS IPSEC_Build_AH_Hdr(NET_BUFFER **buffer, VOID *ip_layer,
                          AHLAYER **ah_layer, IPSEC_OUTBOUND_SA *out_sa,
                          UINT16 ip_hlen);
STATUS IPSEC_Decode_IPv4_AH(NET_BUFFER *buffer, IPLAYER **ip,
                            IPSEC_INBOUND_SA *in_sa,
                            IPSEC_SEQ_NUM *seq_num);
VOID IPSEC_Clear_Mutable_Fields_IPv4(IPLAYER *buffer);


#if (INCLUDE_IPV6 == NU_TRUE)
VOID IPSEC_Clear_Mutable_Fields_IPv6(IP6LAYER *ip6);
VOID IPSEC_IPv6_Get_Next_Prot_Hdr(NET_BUFFER *ip6_hdr_buf,
                                  NET_BUFFER **last_ext_buf,
                                  UINT16 *last_ext_hdr_offset,
                                  UINT16 *ip6_hdr_plus_ext_hdrs_len,
                                  UINT8 *last_ext_hdr_type);
STATUS IPSEC_Encode_IPv6_AH(NET_BUFFER **buffer ,IP6LAYER **ip6_pkt,
                             IPSEC_OUTBOUND_SA *out_sa);
STATUS IPSEC_Decode_IPv6_AH(NET_BUFFER *buffer, IP6LAYER **ip6_pkt,
                                UINT8 *next_header, UINT16 *header_len,
                                IPSEC_INBOUND_SA *in_sa,
                                IPSEC_SEQ_NUM *seq_num);
#endif

#ifdef          __cplusplus
}
#endif  /* _cplusplus */

#endif /* #ifndef IPS_AH_H */

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
*       ips.h
*
* COMPONENT
*
*       IPSEC
*
* DESCRIPTION
*
*       Definitions required for IPsec module.
*
* DATA STRUCTURES
*
*       IPSEC_PROTOCOL_IP
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_H
#define IPS_H

#ifdef          __cplusplus
extern  "C" {                             /* C declarations in C++ */
#endif /* _cplusplus */

/* This define is used to match everything with a certain value. */
#define IPSEC_WILDCARD              0

/* IPsec enabling of a network interface device  */
#define IPSEC_ENABLE                NU_TRUE
#define IPSEC_DISABLE               NU_FALSE

/* Define IPsec protocols IDs. */
#define IPSEC_AH                    IPPROTO_AUTH
#define IPSEC_ESP                   IPPROTO_ESP

/* Define the maximum IPsec protocols headers. Depending whether
 * AH or ESP or both are included in to the build.
 */
#define IPSEC_MAX_HDR               (IPSEC_INCLUDE_AH + IPSEC_INCLUDE_ESP)

/* Defining tunnel and transport modes constants.
 * Do NOT modify these constants. They have been mapped to
 * the values defined in RFC 2407.
 */
#define IPSEC_TUNNEL_MODE           1
#define IPSEC_TRANSPORT_MODE        2

/* 'Next Header' field encapsulation value in AH header when used in
 *  tunnel mode otherwise its IP_TCP_PROT or IP_UDP_PROT in transport.
 */
#define IPSEC_IP_IN_IP              4
#define IPSEC_IP6_IN_IP6            41

/* Policy address types and IP version flag. */
#define IPSEC_IPV4                  0x01
#define IPSEC_IPV6                  0x02
#define IPSEC_SINGLE_IP             0x04
#define IPSEC_RANGE_IP              0x08
#define IPSEC_SUBNET_IP             0x10
#define IPSEC_SRC_FROM_INTERFACE    0x20
#define IPSEC_ENABLE_ESN            0x40

#define IPSEC_ADDRESS_TYPE(flag)    (flag & 0xfc)
#define IPSEC_ADDR_CATEGORY(flag)   (flag & 0x03)

#define IPSEC_DESTINATION           0x01
#define IPSEC_SOURCE                0x02

/* Note that the SPI values from 0 to 255 are reserved for future use
 * by IANA. The following macros define the SPI range for manually keyed
 * IPsec inbound SAs.
 */
#define IPSEC_SPI_START             256

/* Defining a limit to SPI value, so that further values can be used
 * by IKE for establishing SAs.
 */
#define IPSEC_SPI_END               10000

/* Defining selector choice options. */
#define IPSEC_VALUE_FROM_PACKET     0xFF    /* All SA selectors are taken
                                             * from packet.
                                             */

#define IPSEC_VALUE_FROM_POLICY     0x00    /* All SA selectors are taken
                                             * from policy.
                                             */

#define IPSEC_LADDR_FROM_PACKET     0x01    /* Local address for SA selector
                                             * is taken from packet.
                                             */

#define IPSEC_RADDR_FROM_PACKET     0x02    /* Remote address for SA selector
                                             * is taken from packet.
                                             */

#define IPSEC_NEXT_FROM_PACKET      0x04    /* Next layer protocol for SA selector
                                             * is taken from packet.
                                             */

#define IPSEC_LPORT_FROM_PACKET     0x08    /* Local port ICMP message type/code or
                                             * Mobility Header type (depending on the
                                             * next layer protocol) for SA selector is
                                             * taken from packet.
                                             */

#define IPSEC_RPORT_FROM_PACKET     0x10    /* Remote port ICMP message type/code or
                                             * Mobility Header type (depending on the
                                             * next layer protocol) for SA selector is
                                             * taken from packet.
                                             */

/* Allowed range for IPsec error constants is from -5001 to -5250. */
#define IPSEC_NOT_FOUND             -5001
#define IPSEC_LENGTH_IS_SHORT       -5002
#define IPSEC_INVALID_PARAMS        -5003
#define IPSEC_DEV_NOT_REMOVED       -5004
#define IPSEC_PKT_DISCARD           -5005
#define IPSEC_MISALIGNED_PKT        -5006
#define IPSEC_INVALID_DIGEST        -5007
#define IPSEC_INVALID_ALGO_ID       -5008
#define IPSEC_INVALID_KEY_LEN       -5009
#define IPSEC_INVAL_SEQ_NO          -5010
#define IPSEC_ALREADY_EXISTS        -5011
#define IPSEC_POLICY_IN_USE         -5012
#define IPSEC_INVALID_SPI           -5013
#define IPSEC_ALREADY_RUNNING       -5014
#define IPSEC_PACKET_SENT           -5015
#define IPSEC_CRYPTO_ERROR          -5016
#define IPSEC_INVAL_NEXT_HEADER     NU_INVAL_NEXT_HEADER

/* Defines used for getting and setting group attributes. */
#define IPSEC_IS_GROUP              1
#define IPSEC_NEXT_GROUP            2
#define IPSEC_DF_PROCESSING         3

#define IPSEC_SET_DF_BIT            0x01
#define IPSEC_CLEAR_DF_BIT          0x02
#define IPSEC_COPY_DF_BIT           0x04

/* Defines related to Policy and SA attributes. */
#define IPSEC_SECURITY              1
#define IPSEC_SELECT                2
#define IPSEC_FLAGS                 3
#define IPSEC_SOFT_LIFETIME         4
#define IPSEC_HARD_LIFETIME         5
#define IPSEC_IS_POLICY             6
#define IPSEC_NEXT_POLICY           7
#define IPSEC_LIFETIME              8
#define IPSEC_SA_BUNDLE_LIFETIME    9
#define IPSEC_PFS_GROUP_DESC        10
#define IPSEC_AUTH_KEY              11
#define IPSEC_ENCRYPTION_KEY        12
#define IPSEC_IS_SA                 13
#define IPSEC_NEXT_SA               14
#define IPSEC_ANTIREPLAY            15


/* Defines related to Policy flags. */
#define IPSEC_DISCARD               0x01
#define IPSEC_BY_PASS               0x02
#define IPSEC_APPLY                 0x04
#define IPSEC_INBOUND               0x08
#define IPSEC_OUTBOUND              0x10
#define IPSEC_DUAL_ASYNCHRONOUS     (IPSEC_INBOUND | IPSEC_OUTBOUND)

/* Define used for retrieving the policy action. */
#define IPSEC_POLICY_ACTION(flag)   (flag & 0x07)
#define IPSEC_POLICY_FLOW(flag)     (flag & 0x18)

/* Related to SAs */
#define IPSEC_OUT_SA_USED           0x01
#define IPSEC_IN_SA_USED            0x02
#define IPSEC_IKE_SA                0x04

/* Number of bits in a byte. */
#define IPSEC_BITS_PER_BYTE         8

/* Flag identifiers to check for multiple initializations of IPsec. */
#define IPSEC_STOPPED               0
#define IPSEC_INITIALIZED           1

/* Transform openssl status to nucleus status. */
#define NUCLEUS_STATUS(a)           (a)? NU_SUCCESS : IPSEC_CRYPTO_ERROR

/* Only if IPv4 is included. */
#if (INCLUDE_IPV4 == NU_TRUE)

/* Function pointer definitions for decoding and encoding IPsec Headers.
 */
typedef STATUS (*IPSEC_DEC_IP4_HEADER)(NET_BUFFER *buffer,
                               IPLAYER **ip_pkt, IPSEC_INBOUND_SA *in_sa,
                               IPSEC_SEQ_NUM *seq_num );
#endif

/* Only if IPv6 is included. */
#if (INCLUDE_IPV6 == NU_TRUE)

/* The following function pointers are used for encoding and decoding
 * IPsec headers in IPv6.
 */
typedef STATUS (*IPSEC_DEC_IP6_HEADER)
        (NET_BUFFER *buffer, IP6LAYER **ip_pkt, UINT8 *next_header,
                             UINT16 *header_len, IPSEC_INBOUND_SA *in_sa,
                             IPSEC_SEQ_NUM *seq_num);
#endif

/* The following structure is used to specify AH and ESP parameters during
 * IPsec processing for IPv4 or IPv6 packets.
 */
typedef struct ipsec_protocol_ip
{

#if (INCLUDE_IPV4 == NU_TRUE)
    /* Pointer to the function that will be used for decoding IPv4
     * packets.
     */
    IPSEC_DEC_IP4_HEADER        ipsec_dec_ip4_func;
#endif


#if (INCLUDE_IPV6 == NU_TRUE)
    /* Pointer to the function that will be used for decoding IPv6
     * packets.
     */
    IPSEC_DEC_IP6_HEADER        ipsec_dec_ip6_func;
#endif

    /* Type of IPsec header, this will be either AH or ESP. */
    UINT8                       ipsec_hdr;

    /* SPI offset for this IPsec header. */
    UINT8                       ipsec_spi_offset;

    /* Sequence offset for this IPsec header. */
    UINT8                       ipsec_seq_offset;

    /* Pad the structure. */
    UINT8                       ipsec_pad[1];
}IPSEC_PROTOCOL_IP;

/***** Function Prototypes. *****/
STATUS IPSEC_Initialize(NU_MEMORY_POOL *memory_pool);
STATUS IPSEC_Apply_To_Interface(CHAR *if_name, UINT8 state_flag);
STATUS IPSEC_Convert_Subnet6(UINT8 *subnet6);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_H */

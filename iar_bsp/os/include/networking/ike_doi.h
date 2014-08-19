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
*       ike_doi.h
*
* COMPONENT
*
*       IKE - DOI
*
* DESCRIPTION
*
*       This file contains the constants defined in RFC 2407,
*       RFC 2409 and RFC 3602 for the IKE and IPsec DOIs.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_DOI_H
#define IKE_DOI_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/* DOI identifiers. */
#define IKE_DOI_ISAKMP                  0
#define IKE_DOI_IPSEC                   1

/* Attribute format bit values. */
#define IKE_DOI_ATTRIB_AF_TLV           0x0000
#define IKE_DOI_ATTRIB_AF_TV            0x8000

/**** IKE DOI and other Phase 1 constants defined in RFC 2409. ****/

/* Certificate types. */
#define IKE_CERT_NONE                   0
#define IKE_CERT_PKCS7_X509             1
#define IKE_CERT_PGP                    2
#define IKE_CERT_DNS_SIGNEDKEY          3
#define IKE_CERT_X509_SIG               4
#define IKE_CERT_X509_KE                5
#define IKE_CERT_KERBEROS               6
#define IKE_CERT_CRL                    7
#define IKE_CERT_ARL                    8
#define IKE_CERT_SPKI                   9
#define IKE_CERT_X509_ATTRIB            10

/* Notification message error types. */
#define IKE_NOTIFY_INVALID_PLOAD_TYPE   1
#define IKE_NOTIFY_DOI_UNSUPPORTED      2
#define IKE_NOTIFY_SITU_UNSUPPORTED     3
#define IKE_NOTIFY_INVALID_COOKIE       4
#define IKE_NOTIFY_INVALID_MJR_VER      5
#define IKE_NOTIFY_INVALID_MNR_VER      6
#define IKE_NOTIFY_INVALID_XCHG_TYPE    7
#define IKE_NOTIFY_INVALID_FLAGS        8
#define IKE_NOTIFY_INVALID_MSGID        9
#define IKE_NOTIFY_INVALID_PROTID       10
#define IKE_NOTIFY_INVALID_SPI          11
#define IKE_NOTIFY_INVALID_TRANSID      12
#define IKE_NOTIFY_ATTRIB_UNSUPPORTED   13
#define IKE_NOTIFY_NO_PROP_CHOSEN       14
#define IKE_NOTIFY_BAD_PROP_SYNTAX      15
#define IKE_NOTIFY_PLOAD_MALFORMED      16
#define IKE_NOTIFY_INVALID_KEY_INFO     17
#define IKE_NOTIFY_INVALID_ID_INFO      18
#define IKE_NOTIFY_INVALID_CERT_ENCODE  19
#define IKE_NOTIFY_INVALID_CERT         20
#define IKE_NOTIFY_CERT_UNSUPPORTED     21
#define IKE_NOTIFY_INVALID_CERT_AUTH    22
#define IKE_NOTIFY_INVALID_HASH_INFO    23
#define IKE_NOTIFY_AUTH_FAILED          24
#define IKE_NOTIFY_INVALID_SIG          25
#define IKE_NOTIFY_ADDRESS_NOTIFY       26
#define IKE_NOTIFY_SA_LIFETIME          27
#define IKE_NOTIFY_CERT_UNAVAILABLE     28
#define IKE_NOTIFY_UNSUPPORTED_XCHG     29
#define IKE_NOTIFY_UNEQUAL_PLOAD_LEN    30

/* Notification message status types. */
#define IKE_NOTIFY_STAT_CONNECTED       16384

/* Identification type values of the Generic DOI (0). */
#define IKE_ID_IPV4                     0
#define IKE_ID_IPV4_SUBNET              1
#define IKE_ID_IPV6                     2
#define IKE_ID_IPV6_SUBNET              3

/* ISAKMP SA attribute classes. Note that the basic attributes
 * (TV format) must always be encoded as basic. However, variable
 * length attributes (TLV format) MAY be encoded as basic if they
 * can fit in 2 octets. The format of each class has been encoded
 * with its type identifier.
 */
#define IKE_ATTRIB_ENC_ALGO             (IKE_DOI_ATTRIB_AF_TV  | 1)
#define IKE_ATTRIB_HASH_ALGO            (IKE_DOI_ATTRIB_AF_TV  | 2)
#define IKE_ATTRIB_AUTH_METHOD          (IKE_DOI_ATTRIB_AF_TV  | 3)
#define IKE_ATTRIB_GRP_DESC             (IKE_DOI_ATTRIB_AF_TV  | 4)
#define IKE_ATTRIB_GRP_TYPE             (IKE_DOI_ATTRIB_AF_TV  | 5)
#define IKE_ATTRIB_GRP_PRIME            (IKE_DOI_ATTRIB_AF_TLV | 6)
#define IKE_ATTRIB_GRP_GEN_1            (IKE_DOI_ATTRIB_AF_TLV | 7)
#define IKE_ATTRIB_GRP_GEN_2            (IKE_DOI_ATTRIB_AF_TLV | 8)
#define IKE_ATTRIB_GRP_CURV_A           (IKE_DOI_ATTRIB_AF_TLV | 9)
#define IKE_ATTRIB_GRP_CURV_B           (IKE_DOI_ATTRIB_AF_TLV | 10)
#define IKE_ATTRIB_LIFE_TYPE            (IKE_DOI_ATTRIB_AF_TV  | 11)
#define IKE_ATTRIB_LIFE_DURATION        (IKE_DOI_ATTRIB_AF_TLV | 12)
#define IKE_ATTRIB_PRF                  (IKE_DOI_ATTRIB_AF_TV  | 13)
#define IKE_ATTRIB_KEY_LEN              (IKE_DOI_ATTRIB_AF_TV  | 14)
#define IKE_ATTRIB_FIELD_LEN            (IKE_DOI_ATTRIB_AF_TV  | 15)
#define IKE_ATTRIB_GRP_ORDER            (IKE_DOI_ATTRIB_AF_TLV | 16)

/* ISAKMP SA attribute values for Encryption Algorithms. */
#define IKE_VAL_DES_CBC                 1
#define IKE_VAL_IDEA_CBC                2
#define IKE_VAL_BF_CBC                  3
#define IKE_VAL_RC5_R16_B64_CBC         4
#define IKE_VAL_3DES_CBC                5
#define IKE_VAL_CAST_CBC                6
#define IKE_VAL_AES_CBC                 7

/* ISAKMP SA attribute values for Authentication Algorithms. */
#define IKE_VAL_MD5                     1
#define IKE_VAL_SHA                     2
#define IKE_VAL_TIGER                   3

/* ISAKMP SA attribute values for Authentication Method. */
#define IKE_VAL_PSK                     1
#define IKE_VAL_DSS_SIG                 2
#define IKE_VAL_RSA_SIG                 3
#define IKE_VAL_RSA_ENC                 4
#define IKE_VAL_REV_RSA_ENC             5

/* ISAKMP SA attribute values for Group Description. Some of these
 * constants are defined by RFC 3526 and RFC 5114.
 */
#define IKE_VAL_MODP_768                1
#define IKE_VAL_MODP_1024               2
#define IKE_VAL_EC2N_155                3
#define IKE_VAL_EC2N_185                4
#define IKE_VAL_MODP_1536               5
#define IKE_VAL_MODP_2048               14
#define IKE_VAL_MODP_3072               15
#define IKE_VAL_MODP_4096               16
#define IKE_VAL_MODP_6144               17
#define IKE_VAL_MODP_8192               18
#define IKE_VAL_MODP_1024_160_POS       22
#define IKE_VAL_MODP_2048_224_POS       23
#define IKE_VAL_MODP_2048_256_POS       24

/* ISAKMP SA attribute values for Group Type. */
#define IKE_VAL_MODP                    1
#define IKE_VAL_ECP                     2
#define IKE_VAL_EC2N                    3

/* ISAKMP SA attribute values for Life Type. */
#define IKE_VAL_SECS                    1
#define IKE_VAL_KB                      2

/* Nonce data length range. */
#define IKE_MIN_NONCE_DATA_LEN          8
#define IKE_MAX_NONCE_DATA_LEN          256

/**** IPsec DOI constants defined in RFC 2407. ****/

/* The default IPsec encapsulation mode. */
#define IKE_IPS_DEFAULT_ENCAP_MODE      IPSEC_TRANSPORT_MODE

/* The default IPsec SA lifetime in seconds. */
#define IKE_IPS_DEFAULT_SA_LIFETIME     28800

/* Protocol IDs. */
#define IKE_PROTO_ISAKMP                1
#define IKE_PROTO_AH                    2
#define IKE_PROTO_ESP                   3
#define IKE_PROTO_IPCOMP                4

/* Identification type values. */
#define IKE_IPS_ID_IPV4                 1
#define IKE_IPS_ID_FQDN                 2
#define IKE_IPS_ID_USER_FQDN            3
#define IKE_IPS_ID_IPV4_SUBNET          4
#define IKE_IPS_ID_IPV6                 5
#define IKE_IPS_ID_IPV6_SUBNET          6
#define IKE_IPS_ID_IPV4_RANGE           7
#define IKE_IPS_ID_IPV6_RANGE           8
#define IKE_IPS_ID_DER_ASN1_DN          9
#define IKE_IPS_ID_DER_ASN1_GN          10
#define IKE_IPS_ID_KEY_ID               11

/* IPsec Situation length. */
#define IKE_IPS_SITUATION_LEN           4

/* IPsec SPI length. */
#define IKE_IPS_SPI_LEN                 4

/* Bit-mask of the 4 octet Situation. */
#define IKE_IPS_SIT_ID_ONLY             1
#define IKE_IPS_SIT_SECRECY             2
#define IKE_IPS_SIT_INTEGRITY           4

/* Transform ID for Phase 1 key exchange. */
#define IKE_IPS_TRANS_IKE_KE            1

/* Transform IDs for AH. */
#define IKE_IPS_TRANS_AH_MD5            2
#define IKE_IPS_TRANS_AH_SHA            3
#define IKE_IPS_TRANS_AH_DES            4

/* Transform IDs for ESP. */
#define IKE_IPS_TRANS_ESP_DES_IV64      1
#define IKE_IPS_TRANS_ESP_DES           2
#define IKE_IPS_TRANS_ESP_3DES          3
#define IKE_IPS_TRANS_ESP_RC5           4
#define IKE_IPS_TRANS_ESP_IDEA          5
#define IKE_IPS_TRANS_ESP_CAST          6
#define IKE_IPS_TRANS_ESP_BLOWFISH      7
#define IKE_IPS_TRANS_ESP_3IDEA         8
#define IKE_IPS_TRANS_ESP_DES_IV32      9
#define IKE_IPS_TRANS_ESP_RC4           10
#define IKE_IPS_TRANS_ESP_NULL          11
#define IKE_IPS_TRANS_ESP_AES           12

/* Transform IDs for IP Compression (IPCOMP). */
#define IKE_IPS_TRANS_IPCOMP_OUI        1
#define IKE_IPS_TRANS_IPCOMP_DEFLATE    2
#define IKE_IPS_TRANS_IPCOMP_LZS        3

/* IPsec SA attribute classes. */
#define IKE_IPS_ATTRIB_LIFE_TYPE        (IKE_DOI_ATTRIB_AF_TV  | 1)
#define IKE_IPS_ATTRIB_LIFE_DURATION    (IKE_DOI_ATTRIB_AF_TLV | 2)
#define IKE_IPS_ATTRIB_GROUP_DESC       (IKE_DOI_ATTRIB_AF_TV  | 3)
#define IKE_IPS_ATTRIB_ENCAP_MODE       (IKE_DOI_ATTRIB_AF_TV  | 4)
#define IKE_IPS_ATTRIB_AUTH_ALGO        (IKE_DOI_ATTRIB_AF_TV  | 5)
#define IKE_IPS_ATTRIB_KEY_LEN          (IKE_DOI_ATTRIB_AF_TV  | 6)
#define IKE_IPS_ATTRIB_KEY_RND          (IKE_DOI_ATTRIB_AF_TV  | 7)
#define IKE_IPS_ATTRIB_COMP_DICT_LEN    (IKE_DOI_ATTRIB_AF_TV  | 8)
#define IKE_IPS_ATTRIB_COMP_PRIV_ALGO   (IKE_DOI_ATTRIB_AF_TLV | 9)

/* SA attribute values for Life Type. */
#define IKE_IPS_VAL_SECS                1
#define IKE_IPS_VAL_KB                  2

/* SA attribute values for Encapsulation Mode. */
#define IKE_IPS_VAL_TUNNEL              1
#define IKE_IPS_VAL_TRANSPORT           2

/* SA attribute value for Authentication Algorithms. */
#define IKE_IPS_VAL_HMAC_MD5            1
#define IKE_IPS_VAL_HMAC_SHA            2
#define IKE_IPS_VAL_DES_MAC             3
#define IKE_IPS_VAL_KPDK                4

/* IPsec DOI notification message status types. */
#define IKE_IPS_NOTIFY_RESPOND_LIFE  24576
#define IKE_IPS_NOTIFY_REPLAY_STATUS 24577
#define IKE_IPS_NOTIFY_INIT_CONTACT  24578

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_DOI_H */

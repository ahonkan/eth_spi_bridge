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
*       ike2_pload.h
*
* COMPONENT
*
*       IKE2 Payloads
*
* DESCRIPTION
*
*       This file defines constants, data structures and function
*       prototypes required to implement the IKEv2 Payloads.
*
* DATA STRUCTURES
*
*       IKE2_ENCODE_PAYLOAD_FUNCS
*       IKE2_GEN_HDR
*       IKE2_HDR_STRUCT
*       IKE2_TRANSFORM_ATTRIB_PAYLOAD
*       IKE2_TRANSFORMS_PAYLOAD
*       IKE2_PROPOSAL_PAYLOAD
*       IKE2_SA_PAYLOAD
*       IKE2_KE_PAYLOAD
*       IKE2_ID_PAYLOAD
*       IKE2_CERT_PAYLOAD
*       IKE2_CERT_REQ_PAYLOAD
*       IKE2_AUTH_PAYLOAD
*       IKE2_NONCE_PAYLOAD
*       IKE2_NOTIFY_PAYLOAD
*       IKE2_DELETE_PAYLOAD
*       IKE2_VENDOR_ID_PAYLOAD
*       IKE2_TS
*       IKE2_TS_PAYLOAD
*       IKE2_ENCRYPTED_PAYLOAD
*       IKE2_CONFIG_ATTRIBS_PAYLOAD
*       IKE2_CONFIG_PAYLOAD
*       IKE2_EAP_MESSAGE
*       IKE2_EAP_PAYLOAD
*       IKE2_MESSAGE
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE2_PLOAD_H
#define IKE2_PLOAD_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/* Macro used to fill reserve bytes with 0s */
#define IKE2_RSVRD_VALUE                0

#define IKE2_REQUIRED_PAYLOAD_ID         IKE_REQUIRED_PAYLOAD_ID
#define IKE2_OPTIONAL_PAYLOAD_ID         IKE_OPTIONAL_PAYLOAD_ID

/* Macro to specify which payloads MUST be present in inbound packet */
#define IKE2_SET_INBOUND_REQUIRED(dec_payload, payload)                   \
 ((IKE2_GEN_HDR*)(payload))->ike2_payload_type = IKE2_REQUIRED_PAYLOAD_ID;\
 (dec_payload) = (payload);

/* Macro to specify which payloads MAY be present in inbound packet */
#define IKE2_SET_INBOUND_OPTIONAL(dec_payload, payload)                   \
 ((IKE2_GEN_HDR*)(payload))->ike2_payload_type = IKE2_OPTIONAL_PAYLOAD_ID;\
 (dec_payload) = (payload);

/* Macro to specify which payloads to send in an outbound packet */
#define IKE2_SET_OUTBOUND(enc_msg_payload, pload)                         \
    (enc_msg_payload) = (pload)

/* Macro to check for payload presence. This is used when
 * a message has been decoded.
 */
#define IKE2_PAYLOAD_IS_PRESENT(pload)   !((((IKE2_GEN_HDR*)pload)->      \
    ike2_payload_type == IKE2_REQUIRED_PAYLOAD_ID) ||                     \
    (((IKE2_GEN_HDR*)pload)->ike2_payload_type                            \
    == IKE2_OPTIONAL_PAYLOAD_ID))

/* Macro to initialize a chain of payloads. */
#define IKE2_INIT_CHAIN(chain, first, type)                               \
    (chain) = (IKE2_GEN_HDR*)(first);                                     \
    ((chain)->ike2_payload_type) = (type);                                \
    ((chain)->ike2_critical) = (0)

/* Macro to add a payload to a chain of payloads. */
#define IKE2_ADD_TO_CHAIN(chain, next, type)                              \
    (((IKE2_GEN_HDR*)(next))->ike2_payload_type) = (type);                \
    ((chain)->ike2_next_payload) = (IKE2_GEN_HDR*)(next);                 \
    (chain) = (IKE2_GEN_HDR*)(next)

/* Macro to terminate a chain of payloads. */
#define IKE2_END_CHAIN(chain)                                             \
    ((chain)->ike2_next_payload) = (IKE2_GEN_HDR*)NU_NULL

/* Macros indicating type of exchange being used */
#define IKE2_SA_INIT                    34
#define IKE2_AUTH                       35
#define IKE2_CREATE_CHILD_SA            36
#define IKE2_INFORMATIONAL              37

/* Macros indicating IPsec protocol identifier for current negotiation */
#define IKE2_PROTO_ID_RESERVED          0
#define IKE2_PROTO_ID_IKE               IKE_PROTO_ISAKMP
#define IKE2_PROTO_ID_AH                IKE_PROTO_AH
#define IKE2_PROTO_ID_ESP               IKE_PROTO_ESP

/* Macro to indicate if an attrib array field is not valid */
#define IKE2_TRANS_INVALID_FIELD      0

/* Macros indicating type for current transform */
#define IKE2_TRANS_TYPE_RESERVED        0
#define IKE2_TRANS_TYPE_ENCR            1 /* (IKE and ESP) */
#define IKE2_TRANS_TYPE_PRF             2 /* (IKE) */
#define IKE2_TRANS_TYPE_INTEG           3 /* (IKE, AH, optional in ESP)*/
#define IKE2_TRANS_TYPE_DH              4 /* (IKE, optional in AH & ESP)*/
#define IKE2_TRANS_TYPE_ESN             5 /* (AH and ESP) */

/* Macros indicating if current is last transform or not */
#define IKE2_TRANS_LAST_VALUE           0
#define IKE2_TRANS_MORE_VALUE           3

/* Number of Proposals in SA Payload */
#define IKE2_NUM_OF_PROPOSALS           4
/* Minimum number of transforms in an IKE proposal */
#define IKE2_MIN_TRANSFORMS             4
/* Maximum number of transforms our implementation can hold with in a single
 * proposal.
 */
#define IKE2_MAX_TRANSFORMS             8

/* Number of transform attributes in a transform. */
#define IKE2_NUM_OF_TRANSFRM_ATTRIBS    1

/* Defined Transform ID's for transform type encryption algorithm
 * NOTE: Not all of the following are supported.
 */
#define IKE2_ENCR_RESERVED              0
#define IKE2_ENCR_DES_IV64              1
#define IKE2_ENCR_DES                   2
#define IKE2_ENCR_3DES                  3
#define IKE2_ENCR_RC5                   4
#define IKE2_ENCR_IDEA                  5
#define IKE2_ENCR_CAST                  6
#define IKE2_ENCR_BLOWFISH              7
#define IKE2_ENCR_3IDEA                 8
#define IKE2_ENCR_DES_IV32              9
#define IKE2_RESERVED                   10
#define IKE2_ENCR_NULL                  11
#define IKE2_ENCR_AES_CBC               12
#define IKE2_ENCR_AES_CTR               13

/* Defined Transform ID's for transform type Pseudo Random function
 * algorithm.
 */
#define IKE2_PRF_HMAC_MD5               1
#define IKE2_PRF_HMAC_SHA1              2
#define IKE2_PRF_HMAC_TIGER             3   /* Not supported. */
#define IKE2_PRF_AES128_XCBC            4

/* Defined Transform ID's for transform type Integrity (hash) algorithm */
#define IKE2_AUTH_HMAC_MD5_96           1
#define IKE2_AUTH_HMAC_SHA1_96          2
#define IKE2_AUTH_DES_MAC               3
#define IKE2_AUTH_KPDK_MD5              4   /* Not supported. */
#define IKE2_AUTH_AES_XCBC_96           5

/* Defined Transform ID's for transform type Extended Sequence Numbers. */
#define IKE2_NO_ESN_VALUE               0
#define IKE2_ENABLED_ESN_VALUE          1

/* Macros indicating types of Certificates in Cert Payload */
#define IKE2_CERT_RESERVED              0
#define IKE2_CERT_PKCS_7_WRAPPED_X509   1
#define IKE2_CERT_PGP                   2
#define IKE2_CERT_DNS_SIGNED_KEY        3
#define IKE2_CERT_X509_SIGNATURE        4
#define IKE2_CERT_KERBEROS_TOKEN        6
#define IKE2_CERT_REVOCATION_LIST       7
#define IKE2_CERT_AUTH_REVOCATION_LIST  8
#define IKE2_CERT_SPKI                  9
#define IKE2_CERT_X509_ATTRIBUTE        10
#define IKE2_CERT_RAW_RSA_KEY           11
#define IKE2_CERT_HASH_URL_X509         12
#define IKE2_CERT_HASH_URL_X509_BUNDLE  13

/* Macros indicating types of exchange represented in configuration payload. */
#define IKE2_CONFIG_CFG_TYPE_RESERVED   0
#define IKE2_CONFIG_CFG_REQUEST         1
#define IKE2_CONFIG_CFG_REPLY           2
#define IKE2_CONFIG_CFG_SET             3
#define IKE2_CONFIG_CFG_ACK             4

/* Macros indicating types of individual configuration attribute. */
#define IKE2_CONFIG_ATTRIB_RESERVED1        0
#define IKE2_CONFIG_ATTRIB_IP4_ADD          1
#define IKE2_CONFIG_ATTRIB_IP4_NETMASK      2
#define IKE2_CONFIG_ATTRIB_IP4_DNS          3
#define IKE2_CONFIG_ATTRIB_IP4_NBNS         4
#define IKE2_CONFIG_ATTRIB_ADD_EXPIRY       5
#define IKE2_CONFIG_ATTRIB_IP4_DHCP         6
#define IKE2_CONFIG_ATTRIB_APP_VRSN         7
#define IKE2_CONFIG_ATTRIB_IP6_ADD          8
#define IKE2_CONFIG_ATTRIB_RESERVED2        9
#define IKE2_CONFIG_ATTRIB_IP6_DNS          10
#define IKE2_CONFIG_ATTRIB_IP6_NBNS         11
#define IKE2_CONFIG_ATTRIB_IP6_DHCP         12
#define IKE2_CONFIG_ATTRIB_IP4_SUBNET       13
#define IKE2_CONFIG_ATTRIB_SUPPRTD_ATTRIBS  14
#define IKE2_CONFIG_ATTRIB_IP6_SUBNET       15

/* Macros indicating payload and their fields lengths (RFC4306)*/
#define IKE2_HDR_TOTAL_LEN              28 /*IKEv2 header is 28 octets*/
#define IKE2_HDR_SA_SPI_LEN             8  /*It should be 8 octets */
#define IKE2_HDR_NXT_PYLD_LEN           1
#define IKE2_HDR_VRSN_LEN               1

#define IKE2_GEN_HDR_TOTAL_LEN          4

/* Offsets and bit shift counts of IKEv2 header fields. */
#define IKE2_HDR_MJR_VRSN_SHFT          4
#define IKE2_HDR_MNR_VRSN_MASK          0x0F

#define IKE2_HDR_SA_ISPI_OFST           0
#define IKE2_HDR_SA_RSPI_OFST           8
#define IKE2_HDR_NXT_PYLD_OFST          16
#define IKE2_HDR_VRSN_OFST              17
#define IKE2_HDR_XCHG_TYPE_OFST         18
#define IKE2_HDR_FLAGS_OFST             19
#define IKE2_HDR_MSG_ID_OFST            20
#define IKE2_HDR_LEN_OFST               24

/* Bit masks for flags in IKEv2 header. */
#define IKE2_HDR_INITIATOR_FLAG         0x08
#define IKE2_HDR_VERSION_FLAG           0x10
#define IKE2_HDR_RESPONSE_FLAG          0x20

#define IKE2_GEN_HDR_NXT_PYLD_OFST      0
#define IKE2_GEN_HDR_CRITICAL_OFST      1
#define IKE2_GEN_HDR_CRTCL_VALUE        0
#define IKE2_GEN_HDR_PYLD_LEN_OFST      2
#define IKE2_GEN_HDR_CRITICAL_SHFT      7
#define IKE2_GEN_HDR_RSVRD_MASK         0x80

#define IKE2_PRPSL_ISLAST_OFST          0
#define IKE2_PRPSL_RSVRD_OFST           1
#define IKE2_PRPSL_LEN_OFST             2
#define IKE2_PRPSL_NUM_OFST             4
#define IKE2_PRPSL_PROTO_ID_OFST        5
#define IKE2_PRPSL_SPI_SIZE_OFST        6
#define IKE2_PRPSL_NO_OF_TRNSFRMS_OFST  7
#define IKE2_PRPSL_SPI_OFST             8
#define IKE2_TRNSFRM_ISLAST_OFST        0
#define IKE2_TRNSFRM_FRST_RSVRD_OFST    1
#define IKE2_TRNSFRM_LEN_OFST           2
#define IKE2_TRNSFRM_TYPE_OFST          4
#define IKE2_TRNSFRM_SEC_RSVRD_OFST     5
#define IKE2_TRNSFRM_ID_OFST            6
#define IKE2_TRNSFRM_ATTRIBS_OFST       8

#define IKE2_PRPSL_LAST_VALUE           0
#define IKE2_PRPSL_MORE_VALUE           2

/* Masks for the Attribute Type/Format field. */
#define IKE2_TRANS_ATTRIB_AF_MASK       0x8000
/* Attribute format bit values. */
#define IKE2_TRANS_ATTRIB_AF_TLV        0x0000
#define IKE2_TRANS_ATTRIB_AF_TV         0x8000

/* Flag values indicating different transform types for algorithm matching */
#define IKE2_ENCR_FLAG                  0x01
#define IKE2_PRF_FLAG                   0x02
#define IKE2_INTEG_FLAG                 0x04
#define IKE2_DIFFIE_HELLMAN_FLAG        0x08
#define IKE2_OPTIONAL_INTEG_FLAG        0x10
#define IKE2_ESN_FLAG                   0x20

/* The id of transform attribute which tells us that it is key length.
 * It is the only attribute supported presently as per RFC 4306.
 */
#define IKE2_KEY_LEN_ID                14

/* Offsets into Transform Attribute fields. */
#define IKE2_ATTRIB_AFTYPE_OFST         0
#define IKE2_ATTRIB_LENVAL_OFST         2
#define IKE2_ATTRIB_VAL_OFST            4

/* Offsets into Key Exchange payload */
#define IKE2_KE_DHG_NUM_OFST            4
#define IKE2_KE_RSVRD_OFST              6
#define IKE2_KE_DATA_OFST               8

/* Macro to indicate invalid D-H group length */
#define IKE2_KE_INVALID_GRP_LEN_VALUE   0

/* Offsets into ID payload */
#define IKE2_ID_TYPE_OFST               4
#define IKE2_ID_RSVRD1_OFST             5
#define IKE2_ID_RSVRD2_OFST             6
#define IKE2_ID_DATA_OFST               8

/* Offsets into Authentication payload */
#define IKE2_AUTH_METHOD_OFST           4
#define IKE2_AUTH_RSVRD1_OFST           5 /*3 bytes divided into 2 offsets */
#define IKE2_AUTH_RSVRD2_OFST           6
#define IKE2_AUTH_DATA_OFST             8

/* Offset into Nonce payload */
#define IKE2_NONCE_DATA_OFST            4

/* Offset into Vendor ID payload */
#define IKE2_VENDOR_ID_OFST             4

/* Offset into Traffic Selector Head payload */
#define IKE2_TS_HEAD_NUM_OF_TS_OFST     4
#define IKE2_TS_HEAD_RSVRD1_OFST        5
#define IKE2_TS_HEAD_RSVRD2_OFST        6
#define IKE2_TS_HEAD_SELECTORS_OFST     8

/* Offset into Traffic Selector payload */
#define IKE2_TS_TYPE_OFST               0
#define IKE2_TS_PROTOCOL_ID_OFST        1
#define IKE2_TS_SELEC_LEN_OFST          2
#define IKE2_TS_START_PORT_OFST         4
#define IKE2_TS_END_PORT_OFST           6
#define IKE2_TS_IPV4_STRT_ADDR_OFST     8
#define IKE2_TS_IPV4_END_ADDR_OFST      12
#define IKE2_TS_IPV6_STRT_ADDR_OFST     8
#define IKE2_TS_IPV6_END_ADDR_OFST      24

/* Macros for Traffic Selector types */
#define IKE2_TS_IPV4_ADDR_RANGE         7
#define IKE2_TS_IPV6_ADDR_RANGE         8

/* Macros for IP address length values used for Traffic selectors */
#define IKE2_TS_IPV4_ADDR_VALUE         4
#define IKE2_TS_IPV6_ADDR_VALUE         16

/* Maximum selectors that can be sent in on TS payload. */
#define IKE2_MAX_SELECTORS_IN_TS        1

/* Macros for Encrypted payload processing */
#define IKE2_ENCRYPTD_PAD_FIELD         1
#define IKE2_ENCRYPTD_PAD_VALUE         0

/* Macro indicating this is not the last proposal in the SA */
#define IKE2_PROPOSAL_PAYLOAD_ID        2
/* Macro indicating this is not the last transform in the Proposal */
#define IKE2_TRANSFORM_PAYLOAD_ID       3

/* The only possible values for critical bit in Generic header */
#define IKE2_GEN_HDR_CRITICAL_BIT_ON    1
#define IKE2_GEN_HDR_CRITICAL_BIT_OFF   0

#define IKE2_GEN_HDR_TOTAL_LEN          4

/* Offsets into Certificate payload fields. */
#define IKE2_CERT_ENCODING_OFFSET       4
#define IKE2_CERT_DATA_OFFSET           5

/* Offsets into Certificate Request payload fields. */
#define IKE2_CERTREQ_ENCODING_OFFSET    4
#define IKE2_CERTREQ_AUTH_OFFSET        5

/* Offsets into Notify payload. */
#define IKE2_NOTIFY_PROTO_ID_OFFSET     4
#define IKE2_NOTIFY_SPI_LEN_OFFSET      5
#define IKE2_NOTIFY_MSG_TYPE_OFFSET     6
#define IKE2_NOTIFY_SPI_OFFSET          8

/* Offsets into Delete payload. */
#define IKE2_DELETE_PROTO_ID_OFFSET     4
#define IKE2_DELETE_SPI_SIZE_OFFSET     5
#define IKE2_DELETE_NUM_OF_SPIS_OFFSET  6
#define IKE2_DELETE_SPIS_OFFSET         8

/* Offsets into Configuration and it's attributes payload. */
#define IKE2_CONFIG_CFG_TYPE_OFFSET     4
#define IKE2_CONFIG_RSVRD1_OFFSET       5
#define IKE2_CONFIG_RSVRD2_OFFSET       6
#define IKE2_CONFIG_ATTRIBS_OFFSET      8
#define IKE2_CONFIG_ATTRIB_LEN_OFFSET   2
#define IKE2_CONFIG_ATTRIB_VALUE_OFFSET 4

/* Offset into EAP Payload. */
#define IKE2_EAP_MSG_CODE_OFFSET        4
#define IKE2_EAP_MSG_IDENTIFIER_OFFSET  5
#define IKE2_EAP_MSG_LEN_OFFSET         6
#define IKE2_EAP_MSG_TYPE_OFFSET        8
#define IKE2_EAP_MSG_TYPE_DATA_OFFSET   9

/* Minimum lengths of payloads. Generic header and mandatory fields like
 * reserved bytes combined.
 */
#define IKE2_MIN_SA_PAYLOAD_LEN         4
#define IKE2_MIN_PRPSL_PAYLOAD_LEN      8
#define IKE2_MIN_TRANS_PAYLOAD_LEN      8
#define IKE2_MIN_TRANS_ATTRIB_LEN       4
#define IKE2_MIN_KE_PAYLOAD_LEN         8
#define IKE2_MIN_TS_HEAD_PAYLOAD_LEN    8
#define IKE2_TS_SELECTOR_HDR_LEN        8
#define IKE2_MIN_NOTIFY_PAYLOAD_LEN     8
#define IKE2_MIN_DELETE_PAYLOAD_LEN     8
#define IKE2_MIN_CERT_PAYLOAD_LEN       5
#define IKE2_MIN_CERT_REQ_PAYLOAD_LEN   5
#define IKE2_MIN_ID_PAYLOAD_LEN         8
#define IKE2_MIN_AUTH_PAYLOAD_LEN       8
#define IKE2_MIN_NONCE_PAYLOAD_LEN      4
#define IKE2_MIN_VENDOR_ID_PAYLOAD_LEN  4
#define IKE2_MIN_CFG_ATTRIB_PAYLOAD_LEN 4

/* This field is used to compute variable type data length from total length
 * field in EAP message with in an EAP payload.
 */
#define IKE2_MIN_EAP_MSG_LEN            5

/* Payload types. */
#define IKE2_NONE_PAYLOAD_ID             0
#define IKE2_SA_PAYLOAD_ID              33
#define IKE2_KE_PAYLOAD_ID              34
#define IKE2_ID_I_PAYLOAD_ID            35
#define IKE2_ID_R_PAYLOAD_ID            36
#define IKE2_CERT_PAYLOAD_ID            37
#define IKE2_CERTREQ_PAYLOAD_ID         38
#define IKE2_AUTH_PAYLOAD_ID            39
#define IKE2_NONCE_PAYLOAD_ID           40
#define IKE2_NOTIFY_PAYLOAD_ID          41
#define IKE2_DELETE_PAYLOAD_ID          42
#define IKE2_VID_PAYLOAD_ID             43
#define IKE2_TS_I_PAYLOAD_ID            44
#define IKE2_TS_R_PAYLOAD_ID            45
#define IKE2_ENCRYPT_PAYLOAD_ID         46
#define IKE2_CONFIG_PAYLOAD_ID          47
#define IKE2_EAP_PAYLOAD_ID             48

typedef STATUS (*IKE2_ENCODE_PAYLOAD_FUNCS)(UINT8 *buff, UINT16 buff_len,
                                            VOID *payload);

/* Macros defining identification type field in ID payload */
#define IKE2_ID_TYPE_RSVRD              0
#define IKE2_ID_TYPE_IPV4_ADDR          1
#define IKE2_ID_TYPE_FQDN               2
#define IKE2_ID_TYPE_RFC822_ADDR        3
#define IKE2_ID_TYPE_IPV6_ADDR          5
#define IKE2_ID_TYPE_DER_ASN1_DN        9
#define IKE2_ID_TYPE_DER_ASN1_GN        10
#define IKE2_ID_TYPE_DER_KEY_ID         11

/* Fixed length values if ID type belongs to IP addresses */
#define IKE2_ID_TYPE_IPV4_ADDR_VALUE    4
#define IKE2_ID_TYPE_IPV6_ADDR_VALUE    16

/* Values defining different Methods in Authentication Payload */
#define IKE2_AUTH_METHOD_RSA_DS         1 /* digital signature */
#define IKE2_AUTH_METHOD_SKEY_MIC       2 /* shared message integrity code */
#define IKE2_AUTH_METHOD_DSS_DS         3 /* DSS digital signature */

#define IKE2_NOTIFY_SPI_SIZE            256
#define IKE2_CONFIG_ATTRIB_MAX_LEN      20

#define IKE2_CONFIG_RSVRD_MASK          0x8000

/* Number of maximum config attributes in the config payload. */
#define IKE2_MAX_CONFIG_ATTRIBS         5

/* NOTIFY MESSAGE - ERROR TYPES
 * (Not all are supported or used)
 */
#define IKE2_NOTIFY_RESERVED                        0
#define IKE2_NOTIFY_INVALID_IKE_SPI                 4
#define IKE2_NOTIFY_INVALID_MAJOR_VERSION           5
#define IKE2_NOTIFY_INVALID_SYNTAX                  7
#define IKE2_NOTIFY_INVALID_MESSAGE_ID              9
#define IKE2_NOTIFY_INVALID_SPI                     11
#define IKE2_NOTIFY_NO_PROPOSAL_CHOSEN              14
#define IKE2_NOTIFY_INVALID_KE_PAYLOAD              17
#define IKE2_NOTIFY_AUTHENTICATION_FAILED           24
#define IKE2_NOTIFY_SINGLE_PAIR_REQUIRED            34
#define IKE2_NOTIFY_NO_ADDITIONAL_SAS               35
#define IKE2_NOTIFY_INTERNAL_ADDRESS_FAILURE        36
#define IKE2_NOTIFY_FAILED_CP_REQUIRED              37
#define IKE2_NOTIFY_TS_UNACCEPTABLE                 38
#define IKE2_NOTIFY_INVALID_SELECTORS               39

/* NOTIFY MESSAGE - STATUS TYPES
 * (Not all are supported or used)
 */
#define IKE2_NOTIFY_INITIAL_CONTACT                 16384
#define IKE2_NOTIFY_SET_WINDOW_SIZE                 16385
#define IKE2_NOTIFY_ADDITIONAL_TS_POSSIBLE          16386
#define IKE2_NOTIFY_IPCOMP_SUPPORTED                16387
#define IKE2_NOTIFY_NAT_DETECTION_SOURCE_IP         16388
#define IKE2_NOTIFY_NAT_DETECTION_DESTINATION_IP    16389
#define IKE2_NOTIFY_COOKIE                          16390
#define IKE2_NOTIFY_USE_TRANSPORT_MODE              16391
#define IKE2_NOTIFY_HTTP_CERT_LOOKUP_SUPPORTED      16392
#define IKE2_NOTIFY_REKEY_SA                        16393
#define IKE2_NOTIFY_ESP_TFC_PADDING_NOT_SUPPORTED   16394
#define IKE2_NOTIFY_NON_FIRST_FRAGMENTS_ALSO        16395

/* Macros for EAP Message Codes. */
#define IKE2_EAP_MSG_CODE_REQUEST       1
#define IKE2_EAP_MSG_CODE_RESPONSE      2
#define IKE2_EAP_MSG_CODE_SUCCESS       3
#define IKE2_EAP_MSG_CODE_FAILURE       4

/* Macros for EAP Message Types. */
#define IKE2_EAP_MSG_TYPE_IDENTITY      1
#define IKE2_EAP_MSG_TYPE_NOTIFICATION  2
#define IKE2_EAP_MSG_TYPE_NAK           3
#define IKE2_EAP_MSG_TYPE_MD5           4
#define IKE2_EAP_MSG_TYPE_OTP           5
#define IKE2_EAP_MSG_TYPE_GENERIC_TOKEN 6

/* Data structures for maintaining IKEv2 payloads. Please note that except
 * those payloads which in embedded in others, all the rest contain an IKEv2
 * generic header struct as it's first member. Reason for making it the first
 * member is that in several occasions in the implementation the type of a
 * payload is unknown and it is type casted into gen header to process further
 * which will not work unless gen header is the first element.
 */

typedef struct ike2_gen_hdr
{
    struct ike2_gen_hdr     *ike2_next_payload; /* Pointer to next payload. */

    UINT8                   ike2_payload_type; /* Type of Current Payload */

    /* (Critical (1 bit) Reserved (7 bits))
    * MUST be set to zero if the sender wants the recipient to skip this
    * payload if it does not understand the payload type code in the Next
    * Payload field of the previous payload. MUST be set to one if the
    * sender wants the recipient to reject this entire message if it does
    * not understand the payload type. MUST be ignored by the recipient if
    * the recipient understands the payload type code. MUST be set to
    * zero for payload types defined in the RFC. Note that the
    * critical bit applies to the current payload rather than the "next"
    * payload whose type code appears in the first octet. The reasoning
    * behind not setting the critical bit for payloads defined in this
    * document is that all implementations MUST understand all payload
    * types defined in this document and therefore must ignore the Critical
    * bit's value.  Skipped payloads are expected to have valid Next
    * Payload and Payload Length fields.
    * MUST be sent as zero; MUST be ignored on receipt.
    */
    UINT8                   ike2_critical;

    /* Length in octets of the current payload, including the generic
    * payload header.
    */
    UINT16                  ike2_payload_length;
} IKE2_GEN_HDR;

typedef struct ike2_hdr_struct
{
    /* The IKE2_GEN_HDR member below is only to facilitate chain processing
     * of payloads. Actual IKEv2 header does not contain this substructure.
     */
    IKE2_GEN_HDR            ike2_gen;

    /* Value chosen by the initiator to identify a unique IKE SA.
     * It MUST NOT be zero
     */
    UINT8                   ike2_sa_spi_i[IKE2_HDR_SA_SPI_LEN];

    /* Value chosen by the responder to identify a unique IKE SA.
     * It MUST be zero in the first message of an IKE Initial Exchange
     * (including repeats of that message including a cookie) and MUST NOT
     * be zero in any other message.
     */
    UINT8                   ike2_sa_spi_r[IKE2_HDR_SA_SPI_LEN];

    /* Indicates the type of payload that immediately follows header. */
    UINT8                   ike2_next_payload;

    /* Indicates the major version of the IKE protocol in use.
     * Implementations based on this version of IKE MUST set the Major
     * Version to 2. Implementations based on previous versions of IKE and
     * ISAKMP MUST set the Major Version to 1. Implementations based on
     * this version of IKE MUST reject or ignore messages containing a
     * version number greater than 2.
     */
    UINT8                   ike2_major_version;

    /* Implementations based on this version of IKE MUST set the Minor
     * Version to 0.  They MUST ignore the minor version number of
     * received messages.
     */
    UINT8                   ike2_minor_version;

    /* Indicates the type of exchange being used. This constrains the
     * payloads sent in each message and orderings of messages in an
     * exchange. Following are possible values (IKE_SA_INIT | IKE_AUTH |
     * CREATE_CHILD_SA | INFORMATIONAL)
     */
    UINT8                   ike2_exchange_type;

    /* Message identifier used to control retransmission of lost packets
     * and matching of requests and responses. It is essential to the
     * security of the protocol because it is used to prevent message
     * replay attacks.
     */
    UINT32                  ike2_msg_id;

    /* Length of total message (header + payloads)in octets. */
    UINT32                  ike2_length;

    /* Indicates specific options that are set for the message. Presence of
     * options are indicated by the appropriate bit in the flags field
     * being set. The bits are defined LSB first, so bit 0 would be the
     * least significant bit of the Flags octet. A bit being 'set' means
     * its value is '1', while 'cleared' means its value is '0'.
     */
    UINT8                   ike2_flags;
    UINT8                   ike2_pad[3];
} IKE2_HDR;

typedef struct ike2_transform_attrib_payload
{
    UINT16                  ike2_attrib_type;
    UINT16                  ike2_attrib_lenval; /* Attribute value or length */
    UINT8                   *ike2_attrib_value;
} IKE2_TRANS_ATTRIB_PAYLOAD;

typedef struct ike2_transforms_payload
{
    /* Only encryption algorithm (AES based)has an attribute to specify the
     * key length. So number of attributes is always one as stated in RFC 4305.
     */
    IKE2_TRANS_ATTRIB_PAYLOAD   ike2_transform_attrib[IKE2_NUM_OF_TRANSFRM_ATTRIBS];
    UINT16                      ike2_transform_length;
    UINT16                      ike2_transform_id;
    UINT8                       ike2_transform_type;
    UINT8                       ike2_more_transforms;

    /* number of attribs in this payload */
    UINT8                       ike2_transform_attrib_num;
    UINT8                       ike2_pad;
} IKE2_TRANSFORMS_PAYLOAD;

typedef struct ike2_proposal_payload
{
    UINT8                   ike2_more_proposals;
    UINT8                   ike2_proposal_num;
    UINT16                  ike2_proposal_len;
    UINT8                   ike2_spi[IKE2_SPI_LENGTH];

    /* Maximum number of transforms in a proposal is fixed in this implementation */
    IKE2_TRANSFORMS_PAYLOAD ike2_transforms[IKE2_MAX_TRANSFORMS];
    UINT8                   ike2_protocol_id;   /* IKE | AH | ESP */
    UINT8                   ike2_spi_size;
    UINT8                   ike2_no_of_transforms;
    UINT8                   ike2_pad;
} IKE2_PROPOSAL_PAYLOAD;


typedef struct ike2_sa_payload
{
    IKE2_GEN_HDR            ike2_gen;
    IKE2_PROPOSAL_PAYLOAD   ike2_proposals[IKE2_NUM_OF_PROPOSALS];
    UINT8                   ike2_proposals_num;
    UINT8                   ike2_pad[3];
} IKE2_SA_PAYLOAD;


typedef struct ike2_ke_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT16                  ike2_dh_group_no;
    UINT16                  ike2_ke_data_len;
    UINT8                   *ike2_ke_data;
} IKE2_KE_PAYLOAD;

typedef struct ike2_id_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT32                  ike2_rsvd;
    UINT16                  ike2_id_data_len;
    UINT8                   ike2_id_type;
    UINT8                   ike2_pad;
    UINT8                   ike2_id_data[IKE_MAX_ID_DATA_LEN];
} IKE2_ID_PAYLOAD;

typedef struct ike2_cert_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT8                   *ike2_cert_data;
    UINT16                  ike2_cert_data_len; /* Length of the cert data. */
    UINT8                   ike2_cert_encoding;
    UINT8                   ike2_pad;
} IKE2_CERT_PAYLOAD;

typedef struct ike2_cert_req_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT8                   *ike2_ca;
    /* This is field is not encoded but is needed to encode cert authority. */
    UINT16                  ike2_ca_len;
    UINT8                   ike2_cert_encoding;
    UINT8                   ike2_pad;
} IKE2_CERT_REQ_PAYLOAD;

typedef struct ike2_auth_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT8                   *ike2_auth_data; /* variable len data */
    UINT16                  ike2_auth_data_len;
    UINT8                   ike2_auth_method;
    UINT8                   ike2_pad;
} IKE2_AUTH_PAYLOAD;

typedef struct ike2_nonce_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT8                   *ike2_nonce_data; /* variable length */
    UINT16                  ike2_nonce_data_len;
    UINT8                   ike2_pad[2];
} IKE2_NONCE_PAYLOAD;

typedef struct ike2_notify_payload
{
    IKE2_GEN_HDR                ike2_gen;
    struct ike2_notify_payload  *ike2_next;
    UINT8                       ike2_protocol_id;
    UINT8                       ike2_spi_size;
    UINT16                      ike2_notify_message_type;
    UINT8                       ike2_spi[IKE2_NOTIFY_SPI_SIZE];
    UINT8                       *ike2_notify_data;

    /* Not encoded but required to encode notify data */
    UINT16                      ike2_notify_data_len;
    UINT8                      ike2_pad[2];
} IKE2_NOTIFY_PAYLOAD;

typedef struct ike2_delete_payload
{
    IKE2_GEN_HDR                ike2_gen;
    struct ike2_delete_payload  *ike2_next;
    UINT8                       ike2_protocol_id;
    UINT8                       ike2_spi_size;
    UINT16                      ike2_no_of_spis;
    UINT8                       ike2_spi_data[IKE2_NOTIFY_SPI_SIZE];
} IKE2_DELETE_PAYLOAD;

typedef struct ike2_vendor_id_payload
{
    IKE2_GEN_HDR            ike2_gen;
    UINT8                   *ike2_vendor_id; /* variable length */
    UINT16                  ike2_vendor_id_len;
    UINT8                   ike2_pad[2];
} IKE2_VENDOR_ID_PAYLOAD;

typedef struct ike2_ts
{
    UINT8                   ike2_ts_type;
    UINT8                   ike2_ip_protocol_id;
    UINT16                  ike2_selector_length;
    UINT16                  ike2_start_port;
    UINT16                  ike2_end_port;
    struct id_struct        ike2_start_addr;    /* 4 or 16 bytes */
    struct id_struct        ike2_end_addr;      /* 4 or 16 bytes */
} IKE2_TS;

typedef struct ike2_ts_payload
{
    IKE2_GEN_HDR            ike2_gen;
    IKE2_TS                 *ike2_ts;
    UINT8                   ike2_ts_count;
    UINT8                   ike2_pad[3];
} IKE2_TS_PAYLOAD;

typedef struct ike2_config_attribs_payload
{
    UINT16                  ike2_attrib_type;
    UINT16                  ike2_attrib_len;
    UINT8                   ike2_attrib_value[IKE2_CONFIG_ATTRIB_MAX_LEN];
} IKE2_CONFIG_ATTRIBS_PAYLOAD;

typedef struct ike2_config_payload
{
    IKE2_GEN_HDR                ike2_gen;
    IKE2_CONFIG_ATTRIBS_PAYLOAD ike2_cfg_attributes[IKE2_MAX_CONFIG_ATTRIBS];

    /* CFG Type: (CFG_REQUEST|CFG_REPLY|CFG_SET|CFG_ACK) */
    UINT8                       ike2_cfg_type;
    UINT8                       ike2_num_config_attribs;
    UINT8                       ike2_pad[2];
} IKE2_CONFIG_PAYLOAD;


typedef struct ike2_eap_message
{
    UINT8                   ike2_code;/*REQUEST|RESPONSE|SUCCESS|FAILURE*/
    UINT8                   ike2_identifier;
    UINT16                  ike2_length;
    UINT8                   ike2_type;  /*for REQUEST|RESPONSE*/
    UINT8                   ike2_pad[3];
    UINT8                   *ike2_eap_data;
} IKE2_EAP_MSG;

typedef struct ike2_eap_payload
{
    IKE2_GEN_HDR            ike2_gen;
    IKE2_EAP_MSG            ike2_eap_msg;
} IKE2_EAP_PAYLOAD;

typedef struct ike2_message
{
    IKE2_GEN_HDR            *ike2_last;
    IKE2_HDR                *ike2_hdr;
    IKE2_SA_PAYLOAD         *ike2_sa;
    IKE2_KE_PAYLOAD         *ike2_ke;
    IKE2_ID_PAYLOAD         *ike2_id_i;
    IKE2_ID_PAYLOAD         *ike2_id_r;
    IKE2_CERT_PAYLOAD       *ike2_cert;
    IKE2_CERT_REQ_PAYLOAD   *ike2_cert_req;
    IKE2_AUTH_PAYLOAD       *ike2_auth;
    IKE2_NONCE_PAYLOAD      *ike2_nonce;
    IKE2_NOTIFY_PAYLOAD     *ike2_notify;
    IKE2_DELETE_PAYLOAD     *ike2_del;
    IKE2_VENDOR_ID_PAYLOAD  *ike2_vid;
    IKE2_TS_PAYLOAD         *ike2_ts_i;
    IKE2_TS_PAYLOAD         *ike2_ts_r;
    IKE2_CONFIG_PAYLOAD     *ike2_cfg;
    IKE2_EAP_PAYLOAD        *ike2_eap;
} IKE2_MESSAGE;

/* Payload encoding functions. */
VOID IKE2_Add_IKE_Attribute(UINT16 type, IKE2_TRANS_ATTRIB_PAYLOAD *attribs,
          UINT8 *attrib_num, UINT16 value);

STATUS IKE2_Encode_Message(UINT8 *buffer, IKE2_MESSAGE *enc_msg);
STATUS IKE2_Encode_IKE_Header(UINT8 * buffer, UINT16 buffer_len,
                              IKE2_HDR * hdr);
STATUS IKE2_Encode_IKE_Gen_Header(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_GEN_HDR *hdr );
STATUS IKE2_Encode_IKE_Trans_Attrib_Payload(UINT8 *buffer,
                                            UINT16 buffer_len,
                                            IKE2_TRANS_ATTRIB_PAYLOAD *attrib,
                                            UINT16 *attrib_len);
STATUS IKE2_Encode_IKE_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_TRANSFORMS_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                        IKE2_PROPOSAL_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_SA_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_KE_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_KE_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_ID_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Auth_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_AUTH_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                     IKE2_NONCE_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Vendor_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_VENDOR_ID_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_TS(UINT8 *buffer, UINT16 buffer_len, IKE2_TS *pload);
STATUS IKE2_Encode_IKE_TS_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_TS_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Encrypted_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         UINT16 enc_p_chain_len, IKE2_SA *sa,
                                         UINT8 first_payload_type);
STATUS IKE2_Encode_IKE_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_CERT_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                                       IKE2_CERT_REQ_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_NOTIFY_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_DELETE_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_Config_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_CONFIG_PAYLOAD *pload);
STATUS IKE2_Encode_IKE_EAP_Payload(UINT8 *buffer, UINT16 buffer_len,
                                   IKE2_EAP_PAYLOAD *pload);

/* Payload Decoding functions. */
STATUS IKE2_Decode_Message(UINT8 *buffer, IKE2_MESSAGE *dec_msg);
STATUS IKE2_Decode_IKE_Header(UINT8 *buffer, UINT16 buffer_len,
                              IKE2_HDR *hdr);
STATUS IKE2_Decode_IKE_Gen_Header(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_GEN_HDR *hdr, UINT8 *next_pload);
STATUS IKE2_Decode_IKE_Trans_Attrib_Payload(UINT8 *buffer, UINT16 buffer_len,
                                            IKE2_TRANS_ATTRIB_PAYLOAD *pload);
STATUS IKE2_Decode_IKE_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_TRANSFORMS_PAYLOAD *pload);
STATUS IKE2_Decode_IKE_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                        IKE2_PROPOSAL_PAYLOAD *pload);
STATUS IKE2_Decode_IKE_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_SA_PAYLOAD *pload, UINT8 *next_pload);
STATUS IKE2_Decode_IKE_KE_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_KE_PAYLOAD *pload, UINT8 *next_pload);
STATUS IKE2_Decode_IKE_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_ID_PAYLOAD *pload, UINT8 *next_pload);
STATUS IKE2_Decode_IKE_Auth_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_AUTH_PAYLOAD *pload,
                                    UINT8 *next_pload);
STATUS IKE2_Decode_IKE_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                     IKE2_NONCE_PAYLOAD *pload,
                                     UINT8 *next_pload);
STATUS IKE2_Decode_IKE_Vendor_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_VENDOR_ID_PAYLOAD *pload,
                                         UINT8 *next_pload);
STATUS IKE2_Decode_IKE_TS(UINT8 *buffer, UINT16 buffer_len, IKE2_TS *pload);
STATUS IKE2_Decode_IKE_TS_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE2_TS_PAYLOAD *pload, UINT8 *next_pload);
STATUS IKE2_Decode_IKE_Encrypted_Payload(UINT8 *buffer, UINT16 buffer_len,
                                         IKE2_SA *sa);
STATUS IKE2_Decode_IKE_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE2_CERT_PAYLOAD *pload,
                                    UINT8 *next_pload);
STATUS IKE2_Decode_IKE_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                      IKE2_NOTIFY_PAYLOAD **pload,
                                      UINT8 *next_pload);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_PLOAD_H */

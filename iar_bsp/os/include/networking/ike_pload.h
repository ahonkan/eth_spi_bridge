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
*       ike_pload.h
*
* COMPONENT
*
*       IKE - Payload Processing
*
* DESCRIPTION
*
*       This file contains constants, data structures and function
*       prototypes for encoding and decoding IKE payloads.
*
* DATA STRUCTURES
*
*       IKE_GEN_HDR
*       IKE_DATA_ATTRIB
*       IKE_ENC_HDR
*       IKE_DEC_HDR
*       IKE_TRANSFORM_PAYLOAD
*       IKE_PROPOSAL_ENC_PAYLOAD
*       IKE_PROPOSAL_DEC_PAYLOAD
*       IKE_SA_ENC_PAYLOAD
*       IKE_SA_DEC_PAYLOAD
*       IKE_KEYXCHG_ENC_PAYLOAD
*       IKE_KEYXCHG_DEC_PAYLOAD
*       IKE_ID_ENC_PAYLOAD
*       IKE_ID_DEC_PAYLOAD
*       IKE_HASH_ENC_PAYLOAD
*       IKE_HASH_DEC_PAYLOAD
*       IKE_SIGNATURE_ENC_PAYLOAD
*       IKE_SIGNATURE_DEC_PAYLOAD
*       IKE_NONCE_ENC_PAYLOAD
*       IKE_NONCE_DEC_PAYLOAD
*       IKE_NOTIFY_ENC_PAYLOAD
*       IKE_NOTIFY_DEC_PAYLOAD
*       IKE_DELETE_ENC_PAYLOAD
*       IKE_DELETE_DEC_PAYLOAD
*       IKE_VID_ENC_PAYLOAD
*       IKE_VID_DEC_PAYLOAD
*       IKE_ENC_MESSAGE
*       IKE_DEC_MESSAGE
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_PLOAD_H
#define IKE_PLOAD_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** ISAKMP related constants and macros. ****/

/* Macro used to control whether IKE should continue processing
 * an incoming SA payload even if the number of proposals in it
 * is more than IKE_MAX_PROPOSALS. If set to NU_TRUE, only the
 * first (IKE_MAX_PROPOSALS - 1) proposal payloads are
 * considered and the rest are ignored. If set to NU_FALSE, any
 * proposal which contains more than IKE_MAX_PROPOSALS proposals
 * is discarded.
 */
#define IKE_DECODE_PARTIAL_SA           NU_TRUE

/* Default value of RESERVED fields. */
#define IKE_RESERVED                    0

/* Macro used to validate first payload ID. */
#define IKE_VALID_FIRST_PAYLOAD(type)   (((type) >= IKE_SA_PAYLOAD_ID) && \
                                         ((type) <= IKE_VID_PAYLOAD_ID))

/* Macro to get type of next payload, given the payload header. */
#define IKE_NEXT_PAYLOAD_TYPE(hdr)      ((hdr.ike_next_payload) ?         \
                                         (hdr.ike_next_payload->ike_type):\
                                         (IKE_NONE_PAYLOAD_ID))

/* This macro marks the specified payload as optional. It
 * is used when the payload is not part of the message
 * structure.
 */
#define IKE_SET_OPTIONAL(pload)                                           \
            ((IKE_GEN_HDR*)(pload))->ike_type = IKE_OPTIONAL_PAYLOAD_ID

/* Macro to specify a required inbound payload. The first
 * parameter must be a member of the IKE_DEC_MESSAGE
 * structure and the second parameter must be the payload
 * pointer.
 */
#define IKE_SET_INBOUND_REQUIRED(dec_msg_member, pload)                   \
            ((IKE_GEN_HDR*)(pload))->ike_type = IKE_REQUIRED_PAYLOAD_ID;  \
            (dec_msg_member) = (pload)

/* Macro to specify an optional inbound payload. This
 * is similar to the previous macro.
 */
#define IKE_SET_INBOUND_OPTIONAL(dec_msg_member, pload)                   \
            ((IKE_GEN_HDR*)(pload))->ike_type = IKE_OPTIONAL_PAYLOAD_ID;  \
            (dec_msg_member) = (pload)

/* Macro to specify an outbound payload. The first
 * parameter must be a member of the IKE_ENC_MESSAGE
 * structure and the second parameter must be the payload
 * pointer.
 */
#define IKE_SET_OUTBOUND(enc_msg_member, pload)                           \
            (enc_msg_member) = (pload)

/* Macro to initialize a chain of payloads. */
#define IKE_INIT_CHAIN(chain, first, type)                                \
            (chain) = (IKE_GEN_HDR*)(first);                              \
            ((chain)->ike_type) = (type)

/* Macro to add a payload to a chain of payloads. */
#define IKE_ADD_TO_CHAIN(chain, next, type)                               \
            (((IKE_GEN_HDR*)(next))->ike_type) = (type);                  \
            ((chain)->ike_next_payload) = (IKE_GEN_HDR*)(next);           \
            (chain) = (IKE_GEN_HDR*)(next)

/* Macro to terminate a chain of payloads. */
#define IKE_END_CHAIN(chain)                                              \
            ((chain)->ike_next_payload) = (IKE_GEN_HDR*)NU_NULL

/* Macro to set the type of a payload. */
#define IKE_SET_PAYLOAD_TYPE(pload, type)                                 \
            ((pload)->ike_hdr.ike_type) = (type)

/* Macro to check for payload presence. This is used when
 * a message has been decoded.
 */
#define IKE_PAYLOAD_IS_PRESENT(pload)   !((((IKE_GEN_HDR*)pload)->ike_type\
                                           == IKE_REQUIRED_PAYLOAD_ID) || \
                                          (((IKE_GEN_HDR*)pload)->ike_type\
                                           == IKE_OPTIONAL_PAYLOAD_ID))

/* Macro to check if the specified payload is marked as required. */
#define IKE_PAYLOAD_IS_REQUIRED(pload)  (((IKE_GEN_HDR*)pload)->ike_type  \
                                           == IKE_REQUIRED_PAYLOAD_ID)

/* Maximum number of proposals allowed in Phase 1. Always one. */
#define IKE_MAX_PHASE1_PROPOSALS        1

/* Number of transforms in proposal selection. Always one. */
#define IKE_SELECTED_PHASE1_TRANSFORMS  1

/* Default phase 1 proposal number. */
#define IKE_PHASE1_PROPOSAL_NUMBER      1

/* SPI length for Phase 1. This should be set to zero. */
#define IKE_PHASE1_SPI_LEN              0

/* Default Phase 2 proposal number. */
#define IKE_PHASE2_PROPOSAL_NUMBER      1

/* Default Phase 2 transform number. */
#define IKE_PHASE2_TRANSFORM_NUMBER     1

/* Number of transforms in the Phase 2 proposal created
 * by the Initiator. This would always be 1.
 */
#define IKE_PHASE2_NUM_TRANSFORMS       1

/* Number of transform in each proposal payload
 * after negotiation. This would always be 1.
 */
#define IKE_NUM_NEGOTIATED_TRANSFORMS   1

/* Internally used payload IDs which specify whether a
 * payload is required or optional in an incoming message.
 */
#define IKE_REQUIRED_PAYLOAD_ID         IKE_NONE_PAYLOAD_ID
#define IKE_OPTIONAL_PAYLOAD_ID         127

/**** ISAKMP constants defined in RFC 2408. ****/

/* Payload types. */
#define IKE_NONE_PAYLOAD_ID             0
#define IKE_SA_PAYLOAD_ID               1
#define IKE_PROPOSAL_PAYLOAD_ID         2
#define IKE_TRANSFORM_PAYLOAD_ID        3
#define IKE_KEYXCHG_PAYLOAD_ID          4
#define IKE_ID_PAYLOAD_ID               5
#define IKE_CERT_PAYLOAD_ID             6
#define IKE_CERTREQ_PAYLOAD_ID          7
#define IKE_HASH_PAYLOAD_ID             8
#define IKE_SIGNATURE_PAYLOAD_ID        9
#define IKE_NONCE_PAYLOAD_ID            10
#define IKE_NOTIFY_PAYLOAD_ID           11
#define IKE_DELETE_PAYLOAD_ID           12
#define IKE_VID_PAYLOAD_ID              13

/* Lengths of IKE header and different payloads in raw form. */
#define IKE_HDR_LEN                     28
#define IKE_GEN_HDR_LEN                 4
#define IKE_MIN_ATTRIB_LEN              4
#define IKE_MIN_SA_PAYLOAD_LEN          8
#define IKE_MIN_PROPOSAL_PAYLOAD_LEN    8
#define IKE_MIN_TRANSFORM_PAYLOAD_LEN   8
#define IKE_MIN_KEYXCHG_PAYLOAD_LEN     4
#define IKE_MIN_ID_PAYLOAD_LEN          8
#define IKE_MIN_CERT_PAYLOAD_LEN        5
#define IKE_MIN_CERTREQ_PAYLOAD_LEN     5
#define IKE_MIN_HASH_PAYLOAD_LEN        4
#define IKE_MIN_SIGNATURE_PAYLOAD_LEN   4
#define IKE_MIN_NONCE_PAYLOAD_LEN       4
#define IKE_MIN_NOTIFY_PAYLOAD_LEN      12
#define IKE_MIN_DELETE_PAYLOAD_LEN      12
#define IKE_MIN_VID_PAYLOAD_LEN         4

/* Offsets and bit shift counts of IKE header fields. */
#define IKE_MAJOR_VERSION_SHL           4
#define IKE_MINOR_VERSION_MASK          0x0f
#define IKE_HDR_ICOOKIE_OFFSET          0
#define IKE_HDR_RCOOKIE_OFFSET          8
#define IKE_HDR_NEXT_PLOAD_OFFSET       16
#define IKE_HDR_VERSION_OFFSET          17
#define IKE_HDR_XCHG_TYPE_OFFSET        18
#define IKE_HDR_FLAGS_OFFSET            19
#define IKE_HDR_MSG_ID_OFFSET           20
#define IKE_HDR_LENGTH_OFFSET           24

/* Offsets into the generic payload header fields. */
#define IKE_GEN_HDR_NEXT_PLOAD_OFFSET   0
#define IKE_GEN_HDR_RESERVED_OFFSET     1
#define IKE_GEN_HDR_PLOAD_LENGTH_OFFSET 2

/* Offsets into raw payload Attribute fields. */
#define IKE_ATTRIB_AFTYPE_OFFSET        0
#define IKE_ATTRIB_LENVAL_OFFSET        2
#define IKE_ATTRIB_VAL_OFFSET           4

/* Offsets into SA payload fields. */
#define IKE_SA_DOI_OFFSET               4
#define IKE_SA_SITUATION_OFFSET         8

/* Offsets into Proposal payload fields. */
#define IKE_PROPOSAL_NO_OFFSET          4
#define IKE_PROPOSAL_PROTO_ID_OFFSET    5
#define IKE_PROPOSAL_SPI_LEN_OFFSET     6
#define IKE_PROPOSAL_TRANS_NUM_OFFSET   7
#define IKE_PROPOSAL_SPI_OFFSET         8

/* Offsets into Transform payload fields. */
#define IKE_TRANSFORM_NO_OFFSET         4
#define IKE_TRANSFORM_ID_OFFSET         5
#define IKE_TRANSFORM_RESERVED2_OFFSET  6
#define IKE_TRANSFORM_SA_ATTRIB_OFFSET  8

/* Offsets into Key Exchange payload fields. */
#define IKE_KEYXCHG_DATA_OFFSET         4

/* Offsets into Identification payload fields. */
#define IKE_ID_TYPE_OFFSET              4
#define IKE_ID_PROTO_ID_OFFSET          5
#define IKE_ID_PORT_OFFSET              6
#define IKE_ID_DATA_OFFSET              8

/* Offsets into Certificate payload fields. */
#define IKE_CERT_ENCODING_OFFSET        4
#define IKE_CERT_DATA_OFFSET            5

/* Offsets into Certificate Request payload fields. */
#define IKE_CERTREQ_TYPE_OFFSET         4
#define IKE_CERTREQ_AUTH_OFFSET         5

/* Offsets into Hash payload fields. */
#define IKE_HASH_DATA_OFFSET            4

/* Offsets into Signature payload fields. */
#define IKE_SIGNATURE_DATA_OFFSET       4

/* Offsets into Nonce payload fields. */
#define IKE_NONCE_DATA_OFFSET           4

/* Offsets into Notification payload fields. */
#define IKE_NOTIFY_DOI_OFFSET           4
#define IKE_NOTIFY_PROTO_ID_OFFSET      8
#define IKE_NOTIFY_SPI_LEN_OFFSET       9
#define IKE_NOTIFY_MSG_TYPE_OFFSET      10
#define IKE_NOTIFY_SPI_OFFSET           12

/* Offsets into Delete payload fields. */
#define IKE_DELETE_DOI_OFFSET           4
#define IKE_DELETE_PROTO_ID_OFFSET      8
#define IKE_DELETE_SPI_LEN_OFFSET       9
#define IKE_DELETE_NUM_SPI_OFFSET       10
#define IKE_DELETE_SPI_OFFSET           12

/* Offsets into Vendor ID payload fields. */
#define IKE_VID_DATA_OFFSET             4

/* Bit masks of flags in the IKE header. */
#define IKE_HDR_ENC_MASK                0x01
#define IKE_HDR_COMMIT_MASK             0x02
#define IKE_HDR_AUTH_MASK               0x04

/* Masks for the Attribute Type/Format field. */
#define IKE_ATTRIB_AF_MASK              0x8000
#define IKE_ATTRIB_TYPE_MASK            0x7fff

/* Attribute format bit values. */
#define IKE_ATTRIB_AF_TLV               0x0000
#define IKE_ATTRIB_AF_TV                0x8000

/* Non-configurable payload buffer lengths. */
#define IKE_MAX_SPI_LEN                 16
#define IKE_MAX_SA_ATTRIBS              8

/* Maximum length of outgoing attribute values
 * for attributes in TLV format.
 */
#define IKE_MAX_ATTRIB_VAL_LEN          4

/* Buffer Lengths for Certificate and Certificate Request payloads. */
#define IKE_MAX_CERT_DATA_LEN           256
#define IKE_MAX_CERT_AUTH_LEN           256

/* Following macros define buffer lengths of payloads
 * not currently supported by the IKE implementation of
 * Nucleus IPsec.
 */
#define IKE_MAX_VID_DATA_LEN            IKE_MAX_HASH_DATA_LEN

/* Following macros define encoding of Type of Certificate requested
 * in a CERT_REQ payload.
 * (Available types are listed for completeness.)
 */
#define IKE_CERT_ENCODING_NONE              0
#define IKE_CERT_ENCODING_PKCS7             1
#define IKE_CERT_ENCODING_PGP               2
#define IKE_CERT_ENCODING_DNS_SIGNED        3
#define IKE_CERT_ENCODING_X509_SIG          4
#define IKE_CERT_ENCODING_X509_KE           5
#define IKE_CERT_ENCODING_KERBEROS_TOKEN    6
#define IKE_CERT_ENCODING_CRL               7
#define IKE_CERT_ENCODING_ARL               8
#define IKE_CERT_ENCODING_SPKI              9
#define IKE_CERT_ENCODING_X509_ATTRIB       10

/**** Data structures. ****/

/* Typedef for the encode function pointer. */
typedef STATUS (*IKE_PAYLOAD_ENCODE)(UINT8 *buffer, UINT16 buffer_len,
                                     VOID *pload);

/* IKE generic header used by all payloads. */
typedef struct ike_gen_hdr
{
    struct ike_gen_hdr *ike_next_payload;   /* Pointer to next payload. */
    UINT16          ike_payload_len;        /* Length of payload. */
    UINT8           ike_type;               /* Type of payload. */

    UINT8           ike_pad[1];
} IKE_GEN_HDR;

/* Data attributes structure used in Transform payloads. */
typedef struct ike_data_dec_attrib
{
    UINT8           *ike_attrib_val;        /* Attribute value. */
    UINT16          ike_attrib_type;        /* Attribute type. */
    UINT16          ike_attrib_lenval;      /* Attribute length or
                                             * value.
                                             */
} IKE_DATA_ATTRIB;

/* IKE header used for encoding. */
typedef struct ike_enc_hdr
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_msg_id;             /* Message ID of this. */
    UINT32          ike_length;             /* Length of the message. */
    UINT8           *ike_icookie;           /* Initiator cookie. */
    UINT8           *ike_rcookie;           /* Responder cookie. */
    UINT8           ike_exchange_type;      /* Exchange type. */
    UINT8           ike_flags;              /* Flags of the payload. */

    UINT8           ike_pad[2];
} IKE_ENC_HDR;

/* IKE header used for decoding. */
typedef struct ike_dec_hdr
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_msg_id;             /* Message ID. */
    UINT32          ike_length;             /* Length of the payload. */
    UINT8           *ike_icookie;           /* Initiator cookie. */
    UINT8           *ike_rcookie;           /* Responder cookie. */
    UINT8           ike_version;            /* Version of protocol. */
    UINT8           ike_exchange_type;      /* Exchange type. */
    UINT8           ike_flags;              /* Flags of the payload. */
    UINT8           ike_first_payload;      /* Type of first payload. */
} IKE_DEC_HDR;

/* Structure for encoding and decoding the Transform payload. */
typedef struct ike_transform_payload
{
    /* Note that the Transform payload structure is stripped
     * of its generic header. The header is not needed
     * because the number of transforms contained within the
     * proposal are stored in a static array and need not be
     * traversed using the generic payload headers.
     */

    /* Attributes of the SA. */
    IKE_DATA_ATTRIB ike_sa_attributes[IKE_MAX_SA_ATTRIBS];

    UINT8           ike_num_attributes;     /* Number of attributes. */
    UINT8           ike_transform_no;       /* Transform number. */
    UINT8           ike_transform_id;       /* Identifier of transform. */

    UINT8           ike_pad[1];
} IKE_TRANSFORM_PAYLOAD;

/* Structure for encoding the Proposal payload. The
 * Transform payloads are considered a part of the
 * proposal payload.
 */
typedef struct ike_proposal_enc_payload
{
    /* Note that the Proposal payload structure is stripped
     * of its generic header. The header is not needed
     * because the number of proposals contained within the
     * SA payload are stored in a static array and need not be
     * traversed using the generic payload headers.
     */

    /* Transforms contained within the Proposal. */
    IKE_TRANSFORM_PAYLOAD   ike_transforms[IKE_MAX_TRANSFORMS];

    /* SPI - either two eight-octet ISAKMP
     * cookies or one four-octet IPSEC SPI.
     */
    UINT8           ike_spi[IKE_MAX_SPI_LEN];

    UINT8           ike_proposal_no;        /* Proposal number. */
    UINT8           ike_protocol_id;        /* Protocol identifier. */
    UINT8           ike_spi_len;            /* Length of the SPI. */
    UINT8           ike_num_transforms;     /* Number of transforms. */
} IKE_PROPOSAL_ENC_PAYLOAD;

/* Structure for decoding the Proposal payload. The
 * Transform payloads are considered a part of the
 * proposal payload.
 */
typedef struct ike_proposal_dec_payload
{
    /* Note that the Proposal payload structure is stripped
     * of its generic header. The header is not needed
     * because the number of proposals contained within the
     * SA payload are stored in a static array and need not be
     * traversed using the generic payload headers.
     */

    /* Transforms contained within the Proposal. */
    IKE_TRANSFORM_PAYLOAD   ike_transforms[IKE_MAX_TRANSFORMS];

    /* SPI - either two eight-octet ISAKMP
     * cookies or one four-octet IPSEC SPI.
     */
    UINT8           *ike_spi;

    UINT8           ike_proposal_no;        /* Proposal number. */
    UINT8           ike_protocol_id;        /* Protocol identifier. */
    UINT8           ike_spi_len;            /* Length of the SPI. */
    UINT8           ike_num_transforms;     /* Number of transforms. */
} IKE_PROPOSAL_DEC_PAYLOAD;

/* IKE SA payload. Proposals+Transforms are a part of SA payload. */
typedef struct ike_sa_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_doi;                /* DOI identifier. */

    /* Proposals contained in the SA payload. */
    IKE_PROPOSAL_ENC_PAYLOAD ike_proposals[IKE_MAX_PROPOSALS];

    UINT16          ike_situation_len;      /* Length of situation. */

    /* DOI specific situation. */
    UINT8           ike_situation[IKE_IPS_SITUATION_LEN];

    UINT8           ike_num_proposals;      /* Number of proposals. */

    UINT8           ike_pad[1];
} IKE_SA_ENC_PAYLOAD;

/* IKE SA payload. Proposals+Transforms are a part of SA payload. */
typedef struct ike_sa_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_doi;                /* DOI identifier. */

    /* Proposals contained in the SA payload. */
    IKE_PROPOSAL_DEC_PAYLOAD ike_proposals[IKE_MAX_PROPOSALS];

    UINT8           *ike_situation;         /* DOI specific situation. */
    UINT16          ike_situation_len;      /* Length of situation. */
    UINT8           ike_num_proposals;      /* Number of proposals. */

#if (IKE_DECODE_PARTIAL_SA == NU_TRUE)
    UINT8           ike_partial_proposals;  /* Flag to indicate that the
                                             * number of proposals was too
                                             * many so only the first few
                                             * proposal payloads were
                                             * decoded.
                                             */
#else
    UINT8           ike_pad[1];
#endif
} IKE_SA_DEC_PAYLOAD;

/* Structure for encoding the Key Exchange Payload. */
typedef struct ike_key_keyxchg_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_keyxchg_data;      /* DOI specific key data. */
    UINT16          ike_keyxchg_data_len;   /* Key data length. */

    UINT8           ike_pad[2];
} IKE_KEYXCHG_ENC_PAYLOAD;

/* Structure for decoding the Key Exchange Payload. */
typedef struct ike_key_keyxchg_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_keyxchg_data;      /* DOI specific key data. */
    UINT16          ike_keyxchg_data_len;   /* Key data length. */

    UINT8           ike_pad[2];
} IKE_KEYXCHG_DEC_PAYLOAD;

/* Structure for encoding the Identification payload. */
typedef struct ike_id_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */

    UINT16          ike_port;               /* The host Port. */
    UINT16          ike_id_data_len;        /* Length of ID data. */
    UINT8           ike_protocol_id;        /* Protocol ID. */
    UINT8           ike_id_type;            /* Identification type. */

    /* Identification data which is DOI specific. */
    UINT8           ike_id_data[IKE_MAX_ID_DATA_LEN];

    UINT8           ike_pad[2];
} IKE_ID_ENC_PAYLOAD;

/* Structure for decoding the Identification payload. */
typedef struct ike_id_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */

    /* Identification data which is DOI specific. */
    UINT8           *ike_id_data;

    UINT16          ike_port;               /* The host Port. */
    UINT16          ike_id_data_len;        /* Length of ID data. */
    UINT8           ike_protocol_id;        /* Protocol ID. */
    UINT8           ike_id_type;            /* Identification type. */

    UINT8           ike_pad[2];
} IKE_ID_DEC_PAYLOAD;

/* Structure for encoding the Hash payload. */
typedef struct ike_hash_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           ike_hash_data_len;      /* Hash data length. */
    UINT8           ike_hash_data[IKE_MAX_HASH_DATA_LEN]; /* Hash data. */

    UINT8           ike_pad[3];
} IKE_HASH_ENC_PAYLOAD;

/* Structure for decoding the Hash payload. */
typedef struct ike_hash_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_hash_data;         /* Hash data. */
    UINT8           ike_hash_data_len;      /* Hash data length. */

    UINT8           ike_pad[3];
} IKE_HASH_DEC_PAYLOAD;

/* Structure for encoding the Signature payload. */
typedef struct ike_signature_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_signature_data;    /* Signature data. */
    UINT16          ike_signature_data_len; /* Signature data length. */

    UINT8           ike_pad[2];
} IKE_SIGNATURE_ENC_PAYLOAD;

/* Structure for decoding the Signature payload. */
typedef struct ike_signature_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_signature_data;    /* Signature data. */
    UINT16          ike_signature_data_len; /* Signature data length. */

    UINT8           ike_pad[2];
} IKE_SIGNATURE_DEC_PAYLOAD;

/* Structure for encoding the Nonce payload. */
typedef struct ike_nonce_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_nonce_data;        /* Random nonce data. */
    UINT16          ike_nonce_data_len;     /* Nonce data length. */

    UINT8           ike_pad[2];
} IKE_NONCE_ENC_PAYLOAD;

/* Structure for decoding the Nonce payload. */
typedef struct ike_nonce_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_nonce_data;        /* Random nonce data. */
    UINT16          ike_nonce_data_len;     /* Nonce data length. */

    UINT8           ike_pad[2];
} IKE_NONCE_DEC_PAYLOAD;

/* Structure for encoding the Notification payload. */
typedef struct ike_notify_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_doi;                /* DOI identifier. */
    UINT8           *ike_notify_data;       /* Error or other
                                               information. */
    UINT16          ike_notify_type;        /* Type of notification. */
    UINT16          ike_notify_data_len;    /* Notify data length. */

    /* Unique SPI data for this notification payload. */
    UINT8           ike_spi[IKE_MAX_SPI_LEN];

    UINT8           ike_protocol_id;        /* Protocol identifier. */
    UINT8           ike_spi_len;            /* Length of SPI. */

    UINT8           ike_pad[2];
} IKE_NOTIFY_ENC_PAYLOAD;

/* Structure for decoding the Notification payload. */
typedef struct ike_notify_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_doi;                /* DOI identifier. */
    UINT8           *ike_spi;               /* Unique SPI. */
    UINT8           *ike_notify_data;       /* Error or other
                                               information. */
    UINT16          ike_notify_type;        /* Type of notification. */
    UINT16          ike_notify_data_len;    /* Notify data length. */
    UINT8           ike_protocol_id;        /* Protocol identifier. */
    UINT8           ike_spi_len;            /* Length of SPI. */

    UINT8           ike_pad[2];
} IKE_NOTIFY_DEC_PAYLOAD;

/* Structure for encoding the Delete payload. */
typedef struct ike_delete_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_doi;                /* DOI identifier. */
    UINT16          ike_num_spi;            /* Number of SPIs. */

    /* SPIs of SAs which are to be deleted.*/
    UINT8           ike_spis[IKE_MAX_DELETE_SPI][IKE_MAX_SPI_LEN];

    UINT8           ike_protocol_id;        /* Protocol identifier. */
    UINT8           ike_spi_len;            /* Length of each SPI. */
} IKE_DELETE_ENC_PAYLOAD;

/* Structure for decoding the Delete payload. */
typedef struct ike_delete_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT32          ike_doi;                /* DOI identifier. */
    UINT8           *ike_spis[IKE_MAX_DELETE_SPI]; /* SPIs to delete. */
    UINT16          ike_num_spi;            /* Number of SPIs. */
    UINT8           ike_protocol_id;        /* Protocol identifier. */
    UINT8           ike_spi_len;            /* Length of each SPI. */
} IKE_DELETE_DEC_PAYLOAD;

/* Structure for encoding the Vendor ID payload. */
typedef struct ike_vid_enc_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT16          ike_vid_data_len;       /* VID data length. */

    /* Vendor identification data. */
    UINT8           ike_vid_data[IKE_MAX_VID_DATA_LEN];

    UINT8           ike_pad[2];
} IKE_VID_ENC_PAYLOAD;

/* Structure for decoding the Vendor ID payload. */
typedef struct ike_vid_dec_payload
{
    IKE_GEN_HDR     ike_hdr;                /* Generic header. */
    UINT8           *ike_vid_data;          /* Vendor ID hash. */
    UINT16          ike_vid_data_len;       /* VID data length. */

    UINT8           ike_pad[2];
} IKE_VID_DEC_PAYLOAD;

/* Structure for encoding the Certificate payload. */
typedef struct ike_cert_enc_payload
{
    IKE_GEN_HDR     ike_hdr;            /* IKE Generic Header. */
    UINT8           *ike_cert_data;     /* DER encoded certificate. */
    UINT16          ike_cert_data_len;  /* Length of the encoded
                                         * certificate.
                                         */
    UINT8           ike_cert_encoding;  /* Encoding scheme of
                                         * certificate.
                                         */
    UINT8           ike_pad[1];
} IKE_CERT_ENC_PAYLOAD;

/* Structure for decoding the Certificate payload. */
typedef struct ike_cert_dec_payload
{
    IKE_GEN_HDR     ike_hdr;            /* IKE Generic Header. */
    UINT8           *ike_cert_data;     /* DER encoded certificate. */
    UINT16          ike_cert_data_len;  /* Length of the encoded
                                         * certificate.
                                         */
    UINT8           ike_cert_encoding;  /* Encoding scheme of certificate.
                                         */
    UINT8           ike_pad[1];
} IKE_CERT_DEC_PAYLOAD;

/* Structure for encoding the Certificate Request payload. */
typedef struct ike_certreq_enc_payload
{
    IKE_GEN_HDR     ike_hdr;            /* IKE Generic Header. */
    UINT16          ike_ca_dn_length;   /* Length of the CA's distinguished
                                         * name.
                                         */
    UINT8           ike_ca_dn[IKE_MAX_CERT_AUTH_LEN];   /* Distinguished
                                           name of CA acceptable to us. */
    UINT8           ike_cert_type;      /* Type (encoding) of certificate.
                                         */
    UINT8           ike_pad[1];
} IKE_CERTREQ_ENC_PAYLOAD;

/* Structure for decoding the Certificate Request payload. */
typedef struct ike_certreq_dec_payload
{
    IKE_GEN_HDR     ike_hdr;            /* IKE Generic Header. */
    UINT8           *ike_ca_dn;         /* Distinguished name of CA
                                         * acceptable to us.
                                         */
    UINT16          ike_ca_dn_length;   /* Length of the CA's distinguished
                                         * name.
                                         */
    UINT8           ike_cert_type;      /* Type (encoding) of certificate.
                                         */
    UINT8           ike_pad[1];
} IKE_CERTREQ_DEC_PAYLOAD;

/* Structure to store pointers to all encoded payloads
 * currently supported by the IKE implementation of
 * Nucleus IPsec.
 *
 * Following payloads are not supported:
 * - Vendor ID Payload
 */
typedef struct ike_enc_message
{
    IKE_GEN_HDR               *ike_last;    /* Last item in chain. */
    IKE_ENC_HDR               *ike_hdr;     /* Outbound ISAKMP header. */
    IKE_SA_ENC_PAYLOAD        *ike_sa;      /* SA payload. */
    IKE_KEYXCHG_ENC_PAYLOAD   *ike_key;     /* Key exchange payload. */
    IKE_ID_ENC_PAYLOAD        *ike_id_i;    /* Initiator's ID payload. */
    IKE_ID_ENC_PAYLOAD        *ike_id_r;    /* Responder's ID payload. */
    IKE_CERT_ENC_PAYLOAD      *ike_cert;    /* Certificate Payload */
    IKE_CERTREQ_ENC_PAYLOAD   *ike_certreq; /* Certificate Request
                                               Payload */
    IKE_HASH_ENC_PAYLOAD      *ike_hash;    /* Hash payload. */
    IKE_SIGNATURE_ENC_PAYLOAD *ike_sig;     /* Signature payload. */
    IKE_NONCE_ENC_PAYLOAD     *ike_nonce;   /* Nonce payload. */
    IKE_NOTIFY_ENC_PAYLOAD    *ike_notify;  /* Delete payload. */
    IKE_DELETE_ENC_PAYLOAD    *ike_del;     /* Delete payload. */
} IKE_ENC_MESSAGE;

/* Structure to store pointers to all decoded payloads
 * currently supported by IKE implementation of
 * Nucleus IPsec.
 *
 * Following payloads are not supported:
 * - Vendor ID Payload (ignored if received)
 *
 * WARNING: IKE_Check_Missing_Payloads assumes that the
 * ISAKMP header is the first member of this structure
 * and all following members are pointers to payloads.
 * Therefore do not modify without accordingly modifying
 * the above function.
 */
typedef struct ike_dec_message
{
    IKE_DEC_HDR               *ike_hdr;     /* Inbound ISAKMP header. */
    IKE_SA_DEC_PAYLOAD        *ike_sa;      /* SA payload. */
    IKE_KEYXCHG_DEC_PAYLOAD   *ike_key;     /* Key exchange payload. */
    IKE_ID_DEC_PAYLOAD        *ike_id_i;    /* Initiator's ID payload. */
    IKE_ID_DEC_PAYLOAD        *ike_id_r;    /* Responder's ID payload. */
    IKE_CERT_DEC_PAYLOAD      *ike_cert;    /* Certificate Payload */
    IKE_CERTREQ_DEC_PAYLOAD   *ike_certreq; /* Certificate Request
                                               Payload */
    IKE_HASH_DEC_PAYLOAD      *ike_hash;    /* Hash payload. */
    IKE_SIGNATURE_DEC_PAYLOAD *ike_sig;     /* Signature payload. */
    IKE_NONCE_DEC_PAYLOAD     *ike_nonce;   /* Nonce payload. */
    IKE_NOTIFY_DEC_PAYLOAD    *ike_notify;  /* Notify payload. */
    IKE_DELETE_DEC_PAYLOAD    *ike_del;     /* Delete payload. */
} IKE_DEC_MESSAGE;

/**** Function prototypes. ****/

VOID IKE_Add_Attribute(UINT16 type, IKE_DATA_ATTRIB *attribs,
                       UINT8 *attrib_num, UINT16 value);
VOID IKE_Add_Variable_Attribute(UINT16 type, IKE_DATA_ATTRIB *attribs,
                                UINT8 *attrib_num, UINT32 value,
                                UINT8 *buffer);
VOID IKE_Encode_Attribute_Value(UINT32 value, UINT8 *buffer,
                                IKE_DATA_ATTRIB *attrib);
STATUS IKE_Decode_Attribute_Value(UINT32 *value,
                                  IKE_DATA_ATTRIB *attrib);
STATUS IKE_Encode_Attribute(UINT8 *buffer, UINT16 buffer_len,
                            IKE_DATA_ATTRIB *attrib,
                            UINT16 *attrib_len);
STATUS IKE_Decode_Attribute(UINT8 *buffer, UINT16 buffer_len,
                            IKE_DATA_ATTRIB *attrib,
                            UINT16 *attrib_len);
VOID IKE_Set_Header(IKE_ENC_HDR *hdr, UINT32 msg_id, UINT8 *cookies,
                    UINT8 xchg_mode, UINT8 flags);
STATUS IKE_Encode_Header(UINT8 *buffer, UINT16 buffer_len,
                         IKE_ENC_HDR *hdr);
STATUS IKE_Decode_Header(UINT8 *buffer, UINT16 buffer_len,
                         IKE_DEC_HDR *hdr);
STATUS IKE_Get_Message_Length(UINT8 *buffer, UINT16 buffer_len,
                              UINT16 *data_len);
STATUS IKE_Extract_Raw_Payload(UINT8 *buffer, UINT8 **ret_raw,
                               UINT16 *ret_raw_len, UINT8 search_pload);
STATUS IKE_Encode_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_SA_ENC_PAYLOAD *pload);
STATUS IKE_Decode_SA_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_SA_DEC_PAYLOAD *pload,
                             UINT8 *next_pload);
STATUS IKE_Encode_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                   IKE_PROPOSAL_ENC_PAYLOAD *pload,
                                   UINT8 is_last, UINT16 *pload_len);
STATUS IKE_Decode_Proposal_Payload(UINT8 *buffer, UINT16 buffer_len,
                                   IKE_PROPOSAL_DEC_PAYLOAD *pload,
                                   UINT8 *next_pload, UINT16 *pload_len);
STATUS IKE_Encode_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_TRANSFORM_PAYLOAD *pload,
                                    UINT8 is_last, UINT16 *pload_len);
STATUS IKE_Decode_Transform_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_TRANSFORM_PAYLOAD *pload,
                                    UINT8 *next_pload, UINT16 *pload_len);
STATUS IKE_Encode_KeyXchg_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_KEYXCHG_ENC_PAYLOAD *pload);
STATUS IKE_Decode_KeyXchg_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_KEYXCHG_DEC_PAYLOAD *pload,
                                  UINT8 *next_pload);
STATUS IKE_Encode_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_ID_ENC_PAYLOAD *pload);
STATUS IKE_Decode_ID_Payload(UINT8 *buffer, UINT16 buffer_len,
                             IKE_ID_DEC_PAYLOAD *pload, UINT8 *next_pload);
STATUS IKE_Encode_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_CERT_ENC_PAYLOAD *pload);
STATUS IKE_Decode_Cert_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_CERT_DEC_PAYLOAD *pload,
                               UINT8 *next_pload);
STATUS IKE_Encode_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_CERTREQ_ENC_PAYLOAD *pload);
STATUS IKE_Decode_CertReq_Payload(UINT8 *buffer, UINT16 buffer_len,
                                  IKE_CERTREQ_DEC_PAYLOAD *pload,
                                  UINT8 *next_pload);
STATUS IKE_Encode_Hash_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_HASH_ENC_PAYLOAD *pload);
STATUS IKE_Decode_Hash_Payload(UINT8 *buffer, UINT16 buffer_len,
                               IKE_HASH_DEC_PAYLOAD *pload,
                               UINT8 *next_pload);
STATUS IKE_Encode_Signature_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_SIGNATURE_ENC_PAYLOAD *pload);
STATUS IKE_Decode_Signature_Payload(UINT8 *buffer, UINT16 buffer_len,
                                    IKE_SIGNATURE_DEC_PAYLOAD *pload,
                                    UINT8 *next_pload);
STATUS IKE_Encode_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                IKE_NONCE_ENC_PAYLOAD *pload);
STATUS IKE_Decode_Nonce_Payload(UINT8 *buffer, UINT16 buffer_len,
                                IKE_NONCE_DEC_PAYLOAD *pload,
                                UINT8 *next_pload);
STATUS IKE_Encode_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_NOTIFY_ENC_PAYLOAD *pload);
STATUS IKE_Decode_Notify_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_NOTIFY_DEC_PAYLOAD *pload,
                                 UINT8 *next_pload);
STATUS IKE_Encode_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_DELETE_ENC_PAYLOAD *pload);
STATUS IKE_Decode_Delete_Payload(UINT8 *buffer, UINT16 buffer_len,
                                 IKE_DELETE_DEC_PAYLOAD *pload,
                                 UINT8 *next_pload);
STATUS IKE_Encode_VID_Payload(UINT8 *buffer, UINT16 buffer_len,
                              IKE_VID_ENC_PAYLOAD *pload);
STATUS IKE_Decode_VID_Payload(UINT8 *buffer, UINT16 buffer_len,
                              IKE_VID_DEC_PAYLOAD *pload,
                              UINT8 *next_pload);
STATUS IKE_Encode_Message(UINT8 *buffer, IKE_ENC_HDR *hdr);
STATUS IKE_Decode_Message(UINT8 *buffer, IKE_DEC_MESSAGE *dec_msg);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_PLOAD_H */

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
*       ike_db.h
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file includes constants, data structures and function
*       prototypes required for implementing Groups, Security
*       Association (SADB) and Security Policy (SPDB) databases.
*
* DATA STRUCTURES
*
*       IKE_SA_LIFETIME
*       IKE_ATTRIB
*       IKE_SA2
*       IKE_SA2_DB
*       IKE_PHASE1_HANDLE
*       IKE_PHASE2_HANDLE
*       IKE_PHASE2_DB
*       IKE_SA
*       IKE_SADB
*       IKE_SELECTOR_IP
*       IKE_POLICY_SELECTOR
*       IKE_IDENTIFIER
*       IKE_IPS_ID
*       IKE_POLICY
*       IKE_SPDB
*       IKE_POLICY_GROUP
*       IKE_POLICY_GROUP_DB
*       IKE_PRESHARED_KEY
*       IKE_PRESHARED_KEY_DB
*
* DEPENDENCIES
*
*       ike.h
*       pem.h
*
*************************************************************************/
#ifndef IKE_DB_H
#define IKE_DB_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#include "networking/ike.h"
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE && IKE_INCLUDE_PEM == NU_TRUE)
#include "openssl/pem.h"
#endif

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#include "networking/ike2.h"
#endif

/* Type of identifier and selector addresses. Do NOT modify
 * these constants as they have been mapped to the IPsec
 * DOI ID constants:
 */
#define IKE_IPV4                        1
#define IKE_IPV4_SUBNET                 4
#define IKE_IPV6                        5
#define IKE_IPV6_SUBNET                 6
#define IKE_IPV4_RANGE                  7
#define IKE_IPV6_RANGE                  8
#define IKE_DER_ASN1_DN                 9
#define IKE_DOMAIN_NAME                 2
#define IKE_USER_DOMAIN_NAME            3
#define IKE_WILDCARD                    0

/* Macro to check whether an identifier or selector address
 * is a single IPv4 or IPv6 IP address.
 */
#define IKE_IS_SINGLE_IP(id_type)       (((id_type) == IKE_IPV4) || \
                                         ((id_type) == IKE_IPV6))

/* Authentication method identifiers. Following additional
 * methods for Authentication are defined in ike_cfg.h:
 *    - IKE_RSA
 */
#define IKE_PSK                         IKE_VAL_PSK

/* Macro to determine whether the Authentication
 * method is based on digital signatures.
 */
#define IKE_IS_SIGN_METHOD(auth)        ((auth) == IKE_RSA)

/* Macro to determine whether the Authentication
 * method is based on Pre-shared Key.
 */
#define IKE_IS_PSK_METHOD(auth)         ((auth) == IKE_PSK)

/* Policy flag (possible values and meanings):
 * IKE_VERIFY_ID : used to enable identification verification. If this flag
 *                 is set, the Phase 1 negotiation will fail if the remote
 *                 peer's identification data does not match.
 * IKE_VERIFY_CERT: When set, the certificate is checked to be signed by
 *                  the specified CA.
 * IKE_VERIFY_AGAINST_CRL: When set, the received certificate is checked
 *                         against the specified CRL.
 * IKE_CA_IN_CERTREQ : Specify the name of CA in outgoing CERT-REQ
 * IKE_INBAND_CERT_XCHG : Exchange certificates in-band. Do not set this
 *                        flag if you already have peer's certificate.
 *                        CERT-REQ is sent when this flag is set.
 * IKE_SEND_CERT_PROACTIVELY : If set, the certificate is sent even if no
 *                             CERT-REQ is received. If not set, CERT will
 *                             only be sent if CERT-REQ is received.
 * IKE_NEGOTIATE_NAT: Negotiation of NAT Traversal can be configured by
 * setting the appropriate flag in the IKE policy.
 */
#define IKE_VERIFY_ID                   0x01
#define IKE_VERIFY_CERT                 0x02
#define IKE_VERIFY_AGAINST_CRL          0x04
#define IKE_CA_IN_CERTREQ               0x08
#define IKE_INBAND_CERT_XCHG            0x10
#define IKE_SEND_CERT_PROACTIVELY       0x20

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
#define IKE_NEGOTIATE_NATT              0x40
#endif

/* Policy flag specific to IKEv2 (possible values and meanings):
 * IKE2_USE_INIT_COOKIE: used to enforce the use of cookies in the
 *                       IKE_SA_INIT exchange.
 * IKE2_SEND_TS_PAIR:    Indicates Traffic selector pair is to be sent.
 * IKE2_SEND_TS_RANGE:   Indicate Traffic selector range is to be sent.
 * IKE2_SA_REKEY:        The SA should be re keyed upon lifetime expiry.
 * IKE2_SA_DELETE:       The SA should be deleted upon lifetime expiry.
 * The traffic selector payload is used to exchange information about the
 * policies set up. Since it is available from the IKE policy selector,
 * thus no new member will be needed.
 * IKE2_USE_IPS_ESN      Extended sequence numbers are supported
 */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
/* Following are additional flags used in the ike2_flag field of the 
 * exchange handle that are defined in IKEv1 header files - these hex 
 * values should not be reused to define flags for IKEv2 in this 
 * section:
 
 #define IKE_CERTREQ_RECEIVED            0x01
 #define IKE_RESPONDER                   0x40
 #define IKE_INITIATOR                   0x80
*/

#define IKE2_USE_TRANSPORT_MODE         0x02
#define IKE2_SA_REKEY                   0x04
#define IKE2_SA_DELETE                  0x08
#define IKE2_USE_IPS_ESN                0x10
#define IKE2_SEND_TS_PAIR               0x20
#define IKE2_SEND_TS_RANGE              0x100
#define IKE2_WAIT_DELETE                0x200
#define IKE2_USE_INIT_COOKIE            0x400
#endif

/* Flag used in IKE_PHASE2_HANDLE. ike_flags member to mark
 * a Handle as deleted.
 */
#define IKE_DELETE_FLAG                 0x01

/* Phase 1 and 2 exchange mode flags for IKE policy. */
#define IKE_XCHG_MAIN_FLAG              0x01
#define IKE_XCHG_AGGR_FLAG              0x02
#define IKE_XCHG_QUICK_FLAG             0x04
#define IKE_XCHG_INFO_FLAG              0x08

/* Macros which specify the IKE SA state. */
#define IKE_SA_ESTABLISHED              1
#define IKE_SA_INCOMPLETE               2

/* Option specifiers for IKE_Get_Group_Opt API. */
#define IKE_TOTAL_POLICIES              1
#define IKE_IS_GROUP                    2
#define IKE_NEXT_GROUP                  3

/* Option specifiers for IKE_Get_Policy_Opt API. */
#define IKE_SELECTOR                    1
#define IKE_IDS                         2
#define IKE_PHASE1_XCHG                 3
#define IKE_PHASE2_XCHG                 4
#define IKE_XCHG1_ATTRIBS               5
#define IKE_IS_POLICY                   6
#define IKE_NEXT_POLICY                 7
#define IKE_FLAGS                       8

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#define IKE2_FLAGS_OPT                  9
#define IKE2_VERSION_OPT                10
#define IKE2_SA_TIMEOUT_OPT             11

/* SPI length to be used in IKEv2 */
#define IKE2_SPI_LENGTH                 8
#endif

/* Values to be passed as the 'match' parameter
 * to the IKE_Get_Policy_By_Selector function.
 */
#define IKE_MATCH_SELECTORS             IKE_Match_Selectors
#define IKE_MATCH_SELECTORS_ABS         IKE_Match_Selectors_Abs

/* Values to be passed as the 'match' parameter to the
 * IKE_Get_Preshared_Key_Entry function. Note that
 * identifiers are actually the same data type as the
 * selectors so the selector matching functions are used
 * here.
 */
#define IKE_MATCH_IDENTIFIERS           ((IKE_IDENTIFIER_MATCH_FUNC) \
                                         IKE_Match_Selectors)
#define IKE_MATCH_IDENTIFIERS_ABS       ((IKE_IDENTIFIER_MATCH_FUNC) \
                                         IKE_Match_Selectors_Abs)

/* Values to be passed as the 'match' parameter to the
 * IKE_Get_SA function.
 */
#define IKE_MATCH_SA_IP                 IKE_Match_SA_IP
#define IKE_MATCH_SA_PARTIAL_COOKIE     IKE_Match_SA_Partial_Cookie
#define IKE_MATCH_SA_COOKIE             IKE_Match_SA_Cookie

/* Macros for selecting entity or ID type from SA flags. */
#define IKE_SA_ENTITY(flags)            ((flags) & 0xc0)
#define IKE_SA_IDTYPE(flags)            ((flags) & 0x3f)

/**** Definitions for SADB. ****/

/* Structure for storing the SA lifetime. Only those
 * fields relevant to IKE are present. For Phase 2,
 * this is mapped to the IPSEC_SA_LIFETIME structure.
 *
 * NOTE: Lifetime in number of bytes transferred using
 * an SA is not supported because it leads to
 * unsynchronized IPsec SAs.
 */
typedef struct ike_sa_lifetime
{
    UINT32          ike_no_of_secs;         /* Lifetime in seconds. */
    UINT32          ike_attrib_secs_buffer; /* Internal buffer for handling
                                             * Variable length attributes.
                                             */
} IKE_SA_LIFETIME;

/* This structure defines attributes for a Phase 1
 * exchange. These are used in the policy and Phase 1
 * IKE SAs.
 *
 * NOTE: The hash and encryption algorithm members of
 * this structure contain the actual IKE algorithm ID
 * within the IKE policy. However, when this structure
 * is used within IKE SAs, these members contain the
 * index into the algorithm array.
 */
typedef struct ike_attrib
{
    IKE_SA_LIFETIME ike_sa_lifetime;    /* Lifetime of this SA. */

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
    UINT8       *ike_local_cert_file;   /* Local certificate file
                                           containing the public key. */
    UINT8       *ike_local_key_file;    /* Local file containing the
                                           private key. */
    UINT8       *ike_ca_cert_file;      /* Local CA certificate file. */
    UINT8       *ike_peer_cert_file;    /* Peer certificate file. */
    UINT8       *ike_crl_file;          /* CRL to check certificate against */

#if (IKE_INCLUDE_PEM == NU_TRUE)
    pem_password_cb *ike_pem_callback;  /* Callback function to input
                                           password for PEM format private
                                           key */
#endif /* #if (IKE_INCLUDE_PEM == NU_TRUE) */

#endif /* #if (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

    UINT8       *ike_remote_key;        /* PSK, used only when pre-shared
                                           key is used. */
    UINT16      ike_remote_key_len;     /* Remote key length. */
    UINT16      ike_encryption_algo;    /* Encryption algorithm ID. */
    UINT16      ike_auth_method;        /* Authentication method. */
    UINT16      ike_hash_algo;          /* Hash algorithm ID. */
    UINT16      ike_key_len;            /* Key length for this SA. */
    UINT16      ike_group_desc;         /* Group description. */
    UINT8       ike_cert_encoding;      /* File encoding of certificate */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    UINT8       ike_pad[1];
    UINT16      ike2_prf_algo;          /* Pseudo random function */
    UINT8       *ike2_psk_local;        /* Local pre-shared key. */
    UINT8       *ike2_psk_remote;       /* Remote pre-shared key. */
    UINT16      ike2_psk_local_len;     /* Local pre-shared key length. */
    UINT16      ike2_psk_remote_len;    /* Remote pre-shared key length. */
#else
    UINT8       ike_pad[3];
#endif

} IKE_ATTRIB;

/* This structure maps an IPsec SA to a representation
 * from IKE's perspective. It contains the negotiated
 * attributes of the IPsec SA. Note that if multiple
 * protocols are negotiated, then multiple IPsec SAs
 * will be established, each under a single SA2.
 */
typedef struct ike_sa2
{
    struct ike_sa2  *ike_flink;                 /* Front link. */
    IPSEC_SECURITY_PROTOCOL ike_ips_security;   /* IPsec security. */

    UINT8           *ike_local_keymat;      /* Inbound KEYMAT. */
    UINT8           *ike_remote_keymat;     /* Outbound KEYMAT. */
    UINT32          ike_local_spi;          /* Inbound SA SPI. */
    UINT32          ike_remote_spi;         /* Outbound SA SPI. */
    UINT32          ike_attrib_secs_buffer; /* Variable length
                                               attribute. */
    UINT16          ike_keymat_len;         /* Key material length. */

    UINT8           ike_pad[2];
} IKE_SA2;

/* List of SA2 items. A single Phase 2 Exchange may
 * contain negotiations of multiple IPsec SAs. Therefore
 * a list of SA2 items is required.
 */
typedef struct ike_sa2_db
{
    IKE_SA2         *ike_flink;             /* Front link. */
    IKE_SA2         *ike_last;              /* Required for SLL head. */

    /* IPsec policy flags. Policy direction in these flags
     * is used to determine tunnel source/destination when
     * establishing IPsec SAs.
     */
    UINT8           ike_ips_flags;

    UINT8           ike_pad[3];
} IKE_SA2_DB;

/* This file has to be included AFTER the definition for struct ike_sa2_db.
 * This structure is instantiated in another structure in ike2_db.h and
 * we need to have its definition before the include below to avoid
 * compilation errors.
 */
#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
#include "networking/ike2_db.h"
#endif

/* There is a cyclic dependency between the IKE_PHASEx_HANDLE
 * and IKE_SA structures. Therefore these structure declarations
 * are needed here.
 */
struct ike_sa;
struct ike_state_params;

/* IKE Phase 1 Handle represents a Phase 1 Exchange.
 *
 * NOTE: Unlike IKE_PHASE2_HANDLE <--> IKE_PHASE2_DB, there
 * is no corresponding database for the IKE_PHASE1_HANDLE.
 * Phase 1 Handles are maintained by IKE_SADB. As long as
 * an exchange is in progress, a pointer to the Phase 1
 * Handle exists in the IKE SA. Therefore, an independent
 * database is not required.
 *
 * WARNING: The first six fields of this structure must
 * not be moved as they are used to provide a common interface
 * to both IKE_PHASE1_HANDLE and IKE_PHASE2_HANDLE. Following
 * these is a padding of smaller fields to keep the structure
 * word aligned.
 */
typedef struct ike_phase1_handle
{
    struct ike_phase1_handle *ike_flink;    /* Front link to next handle. */
    struct ike_state_params *ike_params;    /* Current state parameters. */
    struct ike_sa   *ike_sa;                /* IKE SA being negotiated. */
    UINT8           *ike_last_message;      /* Last message sent. */

    /* MD5 digest of the last received message. */
    UINT8           ike_last_message_hash[IKE_MD5_DIGEST_LEN];

    UINT8           ike_resend_count;       /* Message re-send count. */
    UINT8           ike_xchg_state;         /* State of state machine. */
    UINT16          ike_dh_remote_key_len;  /* Remote DH key length. */

    IKE_KEY_PAIR    ike_dh_key;             /* Diffie-Hellman key pair. */
    UINT8           *ike_dh_remote_key;     /* Remote DH public key. */
    UINT8           *ike_nonce_data;        /* Outbound Nonce data. */
    UINT8           *ike_sa_b;              /* Initiator's SA payload. */
    UINT8           *ike_id_b;              /* ID payload body. */
    UINT16          ike_sa_b_len;           /* Length of raw SA. */
    UINT16          ike_id_b_len;           /* Raw ID payload length. */

    /* CA's DN. Used in outgoing CERT-REQ when 'Send CA DN' flag is set
     * in IKE Policy.
     */
    UINT8           *ike_ca_dn_data;

    UINT8           ike_xchg_mode;          /* Phase 1 mode of exchange. */
    UINT8           ike_flags;              /* Host and address type
                                               flag. */

    UINT8           ike_pad[2];
} IKE_PHASE1_HANDLE;

/* IKE Phase 2 Handle represents a Phase 2 Exchange. Note
 * that multiple IPsec SAs can be negotiated in a single Phase 2.
 *
 * WARNING: The first six fields of this structure must
 * not be moved as they are used to provide a common interface
 * to both IKE_PHASE1_HANDLE and IKE_PHASE2_HANDLE. Following
 * these is a padding of smaller fields to keep the structure
 * word aligned.
 */
typedef struct ike_phase2_handle
{
    struct ike_phase2_handle *ike_flink;    /* Front link. */
    struct ike_state_params *ike_params;    /* Current state parameters. */
    struct ike_sa   *ike_sa;                /* Parent IKE SA. */
    UINT8           *ike_last_message;      /* Last message sent. */

    /* MD5 digest of the last received message. */
    UINT8           ike_last_message_hash[IKE_MD5_DIGEST_LEN];

    UINT8           ike_resend_count;       /* Message re-send count. */
    UINT8           ike_xchg_state;         /* State of exchange. */
    UINT16          ike_dh_remote_key_len;  /* Remote DH key length. */

    struct addr_struct  ike_node_addr;      /* Remote node address. */
    IKE_SA2_DB      ike_sa2_db;             /* DB of SA2 items. */
    IKE_KEY_PAIR    ike_dh_key;             /* Diffie-Hellman key pair. */
    IPSEC_SA_LIFETIME ike_ips_lifetime;     /* Lifetime of IPsec SAs. */
    IPSEC_SELECTOR  ike_ips_select;         /* IPsec selector. */
    UINT32          ike_msg_id;             /* Unique ID for this list. */
    UINT32          ike_ips_policy_index;   /* IPsec policy index. */
    UINT8           *ike_dh_remote_key;     /* Remote DH public key. */
    UINT8           *ike_nonce_i;           /* Initiator's Nonce data. */
    UINT8           *ike_nonce_r;           /* Responder's Nonce data. */
    UINT16          ike_nonce_i_len;        /* Length of nonce_i data. */
    UINT16          ike_nonce_r_len;        /* Length of nonce_r data. */
    UINT16          ike_group_desc;         /* PFS Group description. */
    UINT8           ike_flags;              /* Host and delete flags. */

    /* IPsec policy group name. */
    CHAR            ike_ips_group_name[IKE_MAX_GROUP_NAME_LEN];

    /* Phase 2 encryption and decryption IV, initially
     * generated using the IV stored in the Phase 1 SA.
     */
    UINT8           ike_encryption_iv[IKE_MAX_ENCRYPT_BLOCK_LEN];
    UINT8           ike_decryption_iv[IKE_MAX_ENCRYPT_BLOCK_LEN];

    UINT8           ike_pad[1];
} IKE_PHASE2_HANDLE;

/* This is the root of the Phase 2 Handle database. */
typedef struct ike_phase2_db
{
    IKE_PHASE2_HANDLE   *ike_flink;         /* Front link. */
    IKE_PHASE2_HANDLE   *ike_last;          /* Required for SLL head. */
} IKE_PHASE2_DB;

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
typedef struct ike2_ips_sa_index
{
    struct ike2_ips_sa_index    *flink;     /* Front link in the list. */
    UINT32                      ips_spi;    /* SPI for IPsec SA. */
    UINT8                       protocol;   /* IPsec protocol used */
    UINT8                       pad[3];
}IKE2_IPS_SA_INDEX;

typedef struct ike2_ips_sa_index_list
{
    IKE2_IPS_SA_INDEX   *flink;
    IKE2_IPS_SA_INDEX   *last;
}IKE2_IPS_SA_INDEX_LIST;
#endif

/* The IKE/ISAKMP SA. This is the structure used to
 * store a Phase 1 IKE SA.
 */
typedef struct ike_sa
{
    struct ike_sa       *ike_flink;         /* Front link to next SA. */
    struct addr_struct  ike_node_addr;      /* Node address. */

    IKE_ATTRIB      ike_attributes;         /* Attributes of this SA. */
    UINT8           *ike_skeyid;            /* Shared key material. */
    UINT8           *ike_skeyid_e;          /* Shared key material. */
    UINT8           *ike_skeyid_a;          /* Shared key material. */
    UINT8           *ike_skeyid_d;          /* Shared key material. */
    UINT8           *ike_encryption_key;    /* Key of encryption
                                               algorithm. */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

    /* For authentication, MAC is calculated over the first and second
     * messages of the exchange. We need to save them until after the
     * next pair is exchanged. (RFC4306 section 2.15)
     */
    UINT8           *ike2_local_auth_data;
    UINT8           *ike2_peer_auth_data;
    UINT16          ike2_local_auth_len;
    UINT16          ike2_peer_auth_len;

    /* IKEv2 uses separate keys for initiator and responder generated
     * traffic. Following values are used to store the responder specific
     * keys. The members of this structure from version 1 (above) are
     * renamed (using #defines below) to facilitate consistent naming
     * conventions
     */
    UINT8           *ike2_skeyseed;
    UINT8           *ike2_sk_er;
    UINT8           *ike2_sk_ar;
    UINT8           *ike2_sk_pr;

    UINT16          ike2_a_len;
    UINT16          ike2_p_len;

#endif

    /* Database of Phase 2 Exchanges currently queued or
     * in progress under this IKE SA.
     */
    IKE_PHASE2_DB   ike_phase2_db;

    /* Pointer to the Phase 1 Handle. This is only valid
     * as long as an IKE SA is being negotiated. It is set
     * to NU_NULL once the IKE SA is established.
     */

    IKE_PHASE1_HANDLE   *ike_phase1;

    /* Initiator and Responder cookies. */
    UINT8           ike_cookies[IKE_COOKIE_LEN * IKE_NUM_ENTITIES];

    /* Phase 1 encryption and decryption IV. The encryption IV
     * buffer is also used to temporarily encode the initial
     * proposal's variable attribute data. Therefore, it MUST
     * be at least 4 bytes in length.
     */
    UINT8           ike_encryption_iv[IKE_MAX_ENCRYPT_BLOCK_LEN];
    UINT8           ike_decryption_iv[IKE_MAX_ENCRYPT_BLOCK_LEN];

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    IKE2_EXCHANGE_HANDLE    *ike2_current_handle;
    IKE2_EXCHANGE_DB        xchg_db;
    UINT8                   ike2_local_spi[IKE2_SPI_LENGTH];
    UINT8                   ike2_remote_spi[IKE2_SPI_LENGTH];
    IKE2_IPS_SA_INDEX_LIST  ips_sa_index;

    UINT32                  ike2_switch_role;

    UINT16                  ike2_skeyseed_len;

    /* IKE version under which this SA is being/has been negotiated. */
    UINT8               ike2_version;
    UINT8               ike2_flags;
#endif

    UINT8           ike_skeyid_len;         /* SKEYID_* length. */
    UINT8           ike_state;              /* State of this SA. */

    UINT8           ike_pad[2];
} IKE_SA;

#define ike2_sk_d               ike_skeyid
#define ike2_sk_ei              ike_skeyid_e
#define ike2_sk_ai              ike_skeyid_a
#define ike2_sk_pi              ike_skeyid_d

/* List of IPsec SAs. */
typedef struct ike_sadb
{
    IKE_SA          *ike_flink;             /* Front link. */
    IKE_SA          *ike_last;              /* Required for SLL head. */
} IKE_SADB;

/**** Definitions for SPDB. ****/

/* IP address data structure used by the selector. */
typedef struct ike_selector_ip
{
    /* The first IP address which contains a single IP,
     * start of an IP range or a subnet address.
     */
    UINT8           ike_addr1[MAX_ADDRESS_SIZE];

    /* Union of the second IP address and prefix length. */
    union
    {
        /* Contains the end of an IP range or an IPv4 subnet mask. */
        UINT8       ike_addr2[MAX_ADDRESS_SIZE];

        /* Contains the prefix length, for an IPv6 subnet. */
        UINT8       ike_prefix_len;
    } ike_ext_addr;
} IKE_SELECTOR_IP;

/* Address selector for a policy. */
typedef struct ike_policy_selector
{
    /* Union of the IP address and domain structures. */
    union
    {
        IKE_SELECTOR_IP     ike_ip;
        CHAR                *ike_domain;
    } ike_addr;

    /* Type of selector address. */
    UINT8           ike_type;

    UINT8           ike_pad[3];
} IKE_POLICY_SELECTOR;

/* Define a Identifier data type to be used to send and
 * verify received Identification payload data.
 */
typedef IKE_POLICY_SELECTOR IKE_IDENTIFIER;

/* This structure defines the identities of the Phase 2
 * Exchange peers. Phase 2 Exchanges would only be allowed
 * for peers whose identity matches in the IKE Policy.
 */
typedef struct ike_ips_id
{
    UINT16          ike_port;               /* TCP or UDP. */
    UINT8           ike_protocol_id;        /* Higher layer protocol. */

    UINT8           ike_pad[1];
} IKE_IPS_ID;

/* This structure defines an IKE policy.
 *
 * WARNING: The first two items in this structure must not
 * be moved as they are used to provide a common interface
 * to both IKE_POLICY and IKE_PRESHARED_KEY.
 */
typedef struct ike_policy
{
    /* Front link to next policy. */
    struct ike_policy   *ike_flink;

    /* The policy's selector. */
    IKE_POLICY_SELECTOR ike_select;

    /* Unique ID of policy. */
    UINT32              ike_index;

    /* The policy's selector. */
    IKE_IDENTIFIER      ike_my_id;

    /* The policy's selector. */
    IKE_IDENTIFIER      ike_peers_id;

    /* An array which specifies the ports/protocols for which
     * Phase 2 SAs can be negotiated for this policy.
     */
    IKE_IPS_ID          *ike_ids;

    /* An array of a set of attributes that can be negotiated. This
     * array is sorted in descending order of preference.
     */
    IKE_ATTRIB          *ike_xchg1_attribs;

    /* List of negotiated Phase 1 IKE SAs. */
    IKE_SADB            ike_sa_list;

    /* Permissible Phase 1 exchange modes. */
    UINT8               ike_phase1_xchg;

    /* Permissible Phase 2 exchange modes. */
    UINT8               ike_phase2_xchg;

    /* Number of elements in ike_ids above. */
    UINT8               ike_ids_no;

    /* Number of elements in ike_xchg1_attribs above. */
    UINT8               ike_xchg1_attribs_no;

    /* Flags for this policy. These include the action to
     * be taken when the policy is matched.
     */
    UINT8               ike_flags;

    /* IKEv2 specific elements */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

    /* Major version number */
    UINT8               ike_version;

    /* Flags that are specific to IKEv2. Following are valid values:
     * (IKE2_USE_INIT_COOKIE, IKE2_SEND_TS_PAIR or
     *  IKE2_SEND_TS_RANGE, IKE_SA_DELETE or IKE_SA_REKEY)
     */
    UINT8               ike2_flags;

    UINT8               ike_pad[1];

    /* Time out value for IKEv2 sa in number of seconds */
    UINT32               ike2_sa_timeout;

#else
    /* Padding needs to be added only when IKEv2 is not included */
    UINT8               ike_pad2[3];
#endif

} IKE_POLICY;

/* This structure defines a database of policies. */
typedef struct ike_spdb
{
    IKE_POLICY      *ike_flink;             /* Front link to next policy. */
    IKE_POLICY      *ike_last;              /* Required for SLL. */
    UINT32          ike_next_policy_index;  /* Unique ID of new policy. */
} IKE_SPDB;

/**** Definitions for IKE Groups. ****/

/* This structure defines a single IKE group.
 *
 * WARNING: Do not modify the first three fields. They should
 * be the same as that of the IPsec Group structure, to allow
 * code sharing.
 */
typedef struct ike_policy_group
{
    struct ike_policy_group *ike_flink;     /* Front link to next group. */
    CHAR            *ike_group_name;        /* Name of this group. */
    IKE_SPDB        ike_policy_list;        /* List of IKE policies. */
} IKE_POLICY_GROUP;

/* The Group Database contains a list of all IKE groups. */
typedef struct ike_policy_group_db
{
    IKE_POLICY_GROUP    *ike_flink;         /* First item in list. */
    IKE_POLICY_GROUP    *ike_last;          /* Required for SLL head. */
} IKE_POLICY_GROUP_DB;

/**** Definitions for Pre-shared Keys. ****/

/* This structure stores a single pre-shared key and its ID.
 *
 * WARNING: The first two items in this structure must not
 * be moved as they are used to provide a common interface
 * to both IKE_POLICY and IKE_PRESHARED_KEY.
 */
typedef struct ike_preshared_key
{
    struct ike_preshared_key *ike_flink;    /* Front link to next PSK. */
    IKE_IDENTIFIER  ike_id;                 /* Entity ID. */
    UINT8           *ike_key;               /* Pre-shared key data. */
    UINT16          ike_index;              /* Unique identifier. */
    UINT8           ike_key_len;            /* Pre-shared key length. */

    UINT8           ike_pad[1];
} IKE_PRESHARED_KEY;

/* Pre-shared key database contains a list of all pre-shared keys. */
typedef struct ike_preshared_key_db
{
    IKE_PRESHARED_KEY   *ike_flink;         /* First item in list. */
    IKE_PRESHARED_KEY   *ike_last;          /* Required for SLL head. */
    UINT16              ike_next_psk_index; /* Unique ID of new PSK. */

    UINT8               ike_pad[2];
} IKE_PRESHARED_KEY_DB;

/**** Miscellaneous internal definitions. ****/

/* Define the type for the selector matching function. */
typedef INT (*IKE_SELECTOR_MATCH_FUNC)(IKE_POLICY_SELECTOR *a,
                                       IKE_POLICY_SELECTOR *b);

/* Define the type for the identifier matching function. */
typedef INT (*IKE_IDENTIFIER_MATCH_FUNC)(IKE_IDENTIFIER *a,
                                         IKE_IDENTIFIER *b);

/* Define type for the SA match function. */
typedef INT (*IKE_SA_MATCH_FUNC)(IKE_SA *sa, VOID *search_data);

/**** Function prototypes. ****/

/* Functions related to Groups. */
STATUS IKE_Initialize_Groups(VOID);
STATUS IKE_Deinitialize_Groups(VOID);
STATUS IKE_Add_Group(CHAR *group_name);
STATUS IKE_Add_To_Group(CHAR *group_name, CHAR *interface_name);
STATUS IKE_Get_Group(CHAR *interface_name, CHAR *return_group,
                     UINT32 *total_len);
STATUS IKE_Get_Group_Entry_By_Device(UINT32 dev_index,
                                     IKE_POLICY_GROUP **ret_group);
STATUS IKE_Get_Group_Opt(CHAR *group_name, INT optname, VOID *optval,
                         INT *optlen);
STATUS IKE_Empty_Group(IKE_POLICY_GROUP *group);
STATUS IKE_Remove_From_Group(CHAR *interface_name);
STATUS IKE_Remove_Group(CHAR *group_name);

/* Utility functions related to Groups. */
STATUS IKE_Get_Group_Entry(CHAR *group_name, IKE_POLICY_GROUP **group);
STATUS IKE_Get_Next_Group(CHAR *group_name, IKE_POLICY_GROUP **group);

/* Functions related to SPDB. */
STATUS IKE_Add_Policy(CHAR *group_name, IKE_POLICY *new_policy,
                      UINT32 *index);
STATUS IKE_Get_Policy_Index(CHAR *group_name,
                            IKE_POLICY_SELECTOR *selector,
                            UINT32 *return_index);
STATUS IKE_Get_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                          VOID *optval, INT *optlen);
STATUS IKE_Set_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                          VOID *optval, INT optlen);
STATUS IKE_Remove_Policy(CHAR *group_name, UINT32 index);
STATUS IKE_Flush_Policies(IKE_SPDB *spdb);

/* Utility functions related to SPDB. */
STATUS IKE_Get_Policy_Entry(CHAR *group_name, IKE_POLICY **ret_policy,
                            UINT32 index);
STATUS IKE_Get_Policy_By_Selector(CHAR *group_name,
                                  IKE_POLICY_SELECTOR *selector,
                                  IKE_POLICY *start_policy,
                                  IKE_POLICY **ret_policy,
                                  IKE_SELECTOR_MATCH_FUNC match);
INT IKE_Match_Selectors(IKE_POLICY_SELECTOR *a,
                        IKE_POLICY_SELECTOR *b);
INT IKE_Match_Selectors_Abs(IKE_POLICY_SELECTOR *a,
                            IKE_POLICY_SELECTOR *b);

/* Functions related to IKE SADB (not APIs). */
INT IKE_Match_SA_IP(IKE_SA *sa, VOID *search_data);
INT IKE_Match_SA_Partial_Cookie(IKE_SA *sa, VOID *search_data);
INT IKE_Match_SA_Cookie(IKE_SA *sa, VOID *search_data);
STATUS IKE_Add_SA(IKE_SADB *sadb, IKE_SA *sa, IKE_SA **ret_sa);
STATUS IKE_Get_SA(IKE_SADB *sadb, VOID *search_data,
                  IKE_SA_MATCH_FUNC match, IKE_SA **ret_sa);
STATUS IKE_Remove_SA(IKE_SADB *sadb, VOID *search_data,
                     IKE_SA_MATCH_FUNC match);
STATUS IKE_Sync_Remove_SAs(IKE_SADB *sadb, VOID *search_data,
                           IKE_SA_MATCH_FUNC match, IKE_SA *skip_sa);
STATUS IKE_Free_Local_SA(IKE_SA *sa);
STATUS IKE_Free_SA(IKE_SA *sa);
STATUS IKE_Flush_SAs(IKE_SADB *sadb);
STATUS IKE_Add_SA2(IKE_SA2_DB *sa2db, IKE_SA2 *sa2);
STATUS IKE_Add_Phase2(IKE_PHASE2_DB *ph2db, IKE_PHASE2_HANDLE *phase2,
                      IKE_PHASE2_HANDLE **ret_phase2);
STATUS IKE_Get_Phase2(IKE_PHASE2_DB *ph2db, UINT32 msg_id,
                      IKE_PHASE2_HANDLE **ret_phase2);
STATUS IKE_Remove_Phase1(IKE_SA *sa);
STATUS IKE_Remove_Phase2(IKE_PHASE2_DB *ph2db, UINT32 msg_id);
STATUS IKE_Free_Phase1(IKE_PHASE1_HANDLE *phase1);
VOID IKE_Flush_SA2(IKE_SA2_DB *sa2db);
STATUS IKE_Free_Local_Phase2(IKE_PHASE2_HANDLE *phase2);
STATUS IKE_Free_Phase2(IKE_PHASE2_HANDLE *phase2);

/* Functions related to Pre-shared Keys. */
STATUS IKE_Initialize_Preshared_Keys(VOID);
STATUS IKE_Deinitialize_Preshared_Keys(VOID);
STATUS IKE_Add_Preshared_Key(IKE_PRESHARED_KEY *psk, UINT16 *index);
STATUS IKE_Get_Preshared_Key_Index(IKE_IDENTIFIER *id,
                                   UINT16 *return_index);
STATUS IKE_Remove_Preshared_Key(UINT16 index);

/* Utility functions related to Pre-shared Keys. */
STATUS IKE_Get_Preshared_Key_Entry(IKE_PRESHARED_KEY **ret_psk,
                                   UINT16 index);
STATUS IKE_Get_Preshared_Key_By_ID(IKE_IDENTIFIER *id,
                                   IKE_PRESHARED_KEY **ret_psk,
                                   IKE_IDENTIFIER_MATCH_FUNC match);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_DB_H */

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
*       ike.h
*
* COMPONENT
*
*       IKE - General
*
* DESCRIPTION
*
*       This file contains constants, data structures and function
*       prototypes which are used throughout IKE.
*
* DATA STRUCTURES
*
*       IKE_ENCRYPTION_ALGO
*       IKE_HASH_ALGO
*       IKE_SIGN_ALGO
*       IKE_SHARED_KEY
*       IKE_KEY_PAIR
*       IKE_STRUCT
*
* DEPENDENCIES
*
*       nucleus.h
*
*************************************************************************/
#ifndef IKE_H
#define IKE_H

#include "nucleus.h"

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Constants used throughout IKE. ****/

/* UDP port on which to listen for incoming IKE packets and
 * on which to send outgoing IKE packets, respectively.
 * Modifying these is not recommended as the default value
 * is specified by RFC 2408.
 */
#define IKE_RECV_UDP_PORT               500
#define IKE_SEND_UDP_PORT               500

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
#define IKE_RECV_NATT_UDP_PORT          4500
#define IKE_SEND_NATT_UDP_PORT          4500
#endif

/* IKE versions numbers in case IKEv2 is included */
#define IKE_VERSION_1                   1
#define IKE_VERSION_2                   2

/* Major and Minor versions. */
#define IKE_MAJOR_VERSION               1
#define IKE_MINOR_VERSION               0

/* Identifiers for initiator and responder. These masks
 * are stored in the SA flags.
 */
#define IKE_INITIATOR                   0x80
#define IKE_RESPONDER                   0x40
#if(IKE_INCLUDE_SIG_AUTH == NU_TRUE)
/* Identifier for flag stored in Phase 1 handle (along with the above two).
 * It specifies that a certificate request has been received and that
 * a certificate should be sent.
 */
#define IKE_CERTREQ_RECEIVED            0x01
#endif

/* Number of entities involved in the exchange. Always 2. */
#define IKE_NUM_ENTITIES                2

/* Some functions only need to identify the local and remote
 * side, rather than the Initiator and Responder identification.
 */
#define IKE_LOCAL                       1
#define IKE_REMOTE                      2

/* Free message ID items in the message ID array are
 * set to this value.
 */
#define IKE_BLANK_MSG_ID                0

/* Cookie length, in bytes. */
#define IKE_COOKIE_LEN                  8

/* IKE function error and status codes. Range -4751 to -5000. */
#define IKE_INVALID_PARAMS              IPSEC_INVALID_PARAMS
#define IKE_NOT_FOUND                   IPSEC_NOT_FOUND
#define IKE_LENGTH_IS_SHORT             -4760
#define IKE_ALREADY_EXISTS              -4761
#define IKE_NO_MEMORY                   -4762
#define IKE_NO_UPDATE                   -4763
#define IKE_INVALID_LENGTH              -4764
#define IKE_INVALID_STATE               -4765
#define IKE_INVALID_DOMAIN              -4766
#define IKE_INVALID_SELECTOR            -4767
#define IKE_UNSUPPORTED_METHOD          -4768
#define IKE_UNSUPPORTED_IDTYPE          -4769
#define IKE_UNSUPPORTED_NOTIFY          -4770
#define IKE_UNALLOWED_MODE              -4771
#define IKE_SA2_NOT_FOUND               -4772
#define IKE_UNEXPECTED_MESSAGE          -4773
#define IKE_TRANSFORM_MISMATCH          -4774
#define IKE_INDEX_NOT_FOUND             -4775
#define IKE_NOT_BUFFERED                -4776
#define IKE_IS_RESEND                   -4777
#define IKE_NO_KEYMAT                   -4778
#define IKE_INTERNAL_ERROR              -4779
#define IKE_LENGTH_IS_LONG              -4780
#define IKE_PHASE1_INCOMPLETE           -4781
#define IKE_PHASE2_INCOMPLETE           -4782
#define IKE_PHASE1_TIMED_OUT            -4783
#define IKE_PHASE2_TIMED_OUT            -4784
#define IKE_SA_TIMED_OUT                -4785
#define IKE_ADDR_MISMATCH               -4786
#define IKE_UNALLOWED_XCHG              -4787
#define IKE_UNALLOWED_XCHG2             -4788
#define IKE_INVALID_VERSION             -4789
#define IKE_ALREADY_RUNNING             -4790
#define IKE_CRYPTO_ERROR                -4791

/* The range -4830 to -4899 is reserved for IKEv2. */

/* IKE Notification error based status codes. These errors
 * have an internally assigned range of -4900 to -5000. These
 * errors have an equivalent Notification error type specified
 * by the IKE protocol. Make sure the IKE_NOTIFY_ERROR_MIN
 * macro is updated if more status values are appended to this
 * list.
 */
#define IKE_UNEQUAL_PLOAD_LEN           -4900
#define IKE_INVALID_COOKIE              -4901
#define IKE_INVALID_MJR_VER             -4902
#define IKE_INVALID_MNR_VER             -4903
#define IKE_INVALID_PLOAD_TYPE          -4904
#define IKE_INVALID_XCHG_TYPE           -4905
#define IKE_INVALID_FLAGS               -4906
#define IKE_INVALID_MSGID               -4907
#define IKE_INVALID_PAYLOAD             -4908
#define IKE_DUPLICATE_PAYLOAD           -4909
#define IKE_TOO_MANY_TRANSFORMS         -4910
#define IKE_TOO_MANY_PROPOSALS          -4911
#define IKE_SA_NOT_FOUND                -4912
#define IKE_ATTRIB_TOO_LONG             -4913
#define IKE_INVALID_SPI                 -4914
#define IKE_INVALID_PROTOCOL            -4915
#define IKE_INVALID_TRANSFORM           -4916
#define IKE_INVALID_KEYLEN              -4917
#define IKE_INVALID_PROPOSAL            -4918
#define IKE_UNSUPPORTED_DOI             -4919
#define IKE_UNSUPPORTED_ALGO            -4920
#define IKE_UNSUPPORTED_ATTRIB          -4921
#define IKE_MISSING_ATTRIB              -4922
#define IKE_MISSING_PAYLOAD             -4923
#define IKE_UNEXPECTED_PAYLOAD          -4924
#define IKE_NOT_NEGOTIABLE              -4925
#define IKE_PROPOSAL_TAMPERED           -4926
#define IKE_AUTH_FAILED                 -4927
#define IKE_VERIFY_FAILED               -4928
#define IKE_ID_MISMATCH                 -4929
#define IKE_UNSUPPORTED_SITU            -4930
#define IKE_INVALID_ID                  -4931

#define IKE_GEN_ERROR                   -4932
#define IKE_CERT_FILE_ERROR             -4933
#define IKE_CERT_ERROR                  -4934


/* Range of internal status codes mapped to notify errors.
 * Note that no extra codes should be left blank in this
 * range.
 */
#define IKE_NOTIFY_ERROR_MAX            -4900
#define IKE_NOTIFY_ERROR_MIN            -4934
#define IKE_UNDEFINED_TS_TYPE           -4935

/* Macro to check whether exchange error code has
 * an equivalent notification error type.
 */
#define IKE_IS_NOTIFY_ERROR(err)        (((err) >= IKE_NOTIFY_ERROR_MIN)  \
                                        && ((err) <= IKE_NOTIFY_ERROR_MAX))

/* Total number of internal status codes mapped to
 * notification errors.
 */
#define IKE_TOTAL_NOTIFY_ERRORS         (IKE_NOTIFY_ERROR_MAX -     \
                                         IKE_NOTIFY_ERROR_MIN + 1)

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
/* Macros to get the IP address length based on family or flags. */
#define IKE_IP_LEN(family)              (((family) == NU_FAMILY_IP) ? \
                                         IP_ADDR_LEN : IP6_ADDR_LEN)

#define IKE_IP_LEN_BY_FLAGS(flags)      (((flags) == IKE_IPV4) ?       \
                                         IP_ADDR_LEN : IP6_ADDR_LEN)
#elif (INCLUDE_IPV4 == NU_TRUE)
/* Macros to get the IP address length based on family or flags. */
#define IKE_IP_LEN(family)              IP_ADDR_LEN

#define IKE_IP_LEN_BY_FLAGS(flags)      IP_ADDR_LEN
#else
/* Macros to get the IP address length based on family or flags. */
#define IKE_IP_LEN(family)              IP6_ADDR_LEN

#define IKE_IP_LEN_BY_FLAGS(flags)      IP6_ADDR_LEN
#endif

/* Macro to convert IP family identifier to corresponding IKE flags. */
#define IKE_FAMILY_TO_FLAGS(family)     (((family) == NU_FAMILY_IP) ? \
                                         IKE_IPV4 : IKE_IPV6)

/* Macro to convert IKE flags to corresponding IP family identifier. */
#define IKE_FLAGS_TO_FAMILY(flags)      (((flags) & IKE_IPV4) ?       \
                                         NU_FAMILY_IP : NU_FAMILY_IP6)

/* Conversion from number of bits to bytes. */
#define IKE_BITS_TO_BYTES(x)            ((x) >> 3)

/* Conversion from number of bytes to bits. */
#define IKE_BYTES_TO_BITS(x)            ((x) << 3)

/* Macro returns a non-zero value if the cookie is set. */
#define IKE_COOKIE_IS_SET(ptr)          ((UINT8)((ptr)[0] | (ptr)[1] | \
                                                 (ptr)[2] | (ptr)[3] | \
                                                 (ptr)[4] | (ptr)[5] | \
                                                 (ptr)[6] | (ptr)[7]))

/* Macros for looking up algorithms. */
#define IKE_HASH_ALGO_INDEX(id, ind)    IKE_Get_Algo_Index((id),        \
                                            IKE_Hash_Algos,             \
                                            sizeof(IKE_HASH_ALGO),      \
                                            IKE_TOTAL_HASH_ALGO,        \
                                            &(ind))
#define IKE_SIGN_ALGO_INDEX(id, ind)    IKE_Get_Algo_Index((id),        \
                                            IKE_Sign_Algos,             \
                                            sizeof(IKE_SIGN_ALGO),      \
                                            IKE_TOTAL_SIGN_ALGO,        \
                                            &(ind))
#define IKE_ENCRYPTION_ALGO_INDEX(id, ind) IKE_Get_Algo_Index((id),     \
                                            IKE_Encryption_Algos,       \
                                            sizeof(IKE_ENCRYPTION_ALGO),\
                                            IKE_TOTAL_ENCRYPTION_ALGO,  \
                                            &(ind))

/* MD5 digest length. */
#define IKE_MD5_DIGEST_LEN              16

/* MD5_96 digest length. */
#define IKE_MD5_96_DIGEST_LEN           96/8

/* Event flag in the IKE Event Group which is used by
 * the event handler task to monitor the event list.
 * This must be the last bit in the Group.
 */
#define IKE_NEW_EVENT_FLAG              0x80000000UL

/* Bit mask used to specify the IKE Event Group bits
 * used by blocking exchange requests.
 */
#define IKE_WAIT_EVENT_MASK             0x7fffffffUL

/* Possible states of IKE service. The current state is
 * always stored in IKE_Daemon_State. The first two states
 * for stopping are used to terminate each of the two
 * IKE tasks. States could only change in the listed order,
 * starting from the first to the last and then rolling
 * back to the first state.
 */
#define IKE_DAEMON_STOPPED              0
#define IKE_DAEMON_RUNNING              1
#define IKE_DAEMON_STOPPING_LISTEN      2
#define IKE_DAEMON_STOPPING_EVENTS      3
#define IKE_DAEMON_STOPPING_MISC        4

/* The interval after which IKE daemon state is polled for
 * changes by the IKE tasks. This value into the number of
 * stopping states (i.e. 3) is the maximum time that the
 * daemon should take to shutdown. The value is in CPU ticks.
 */
#define IKE_DAEMON_POLL_INTERVAL        (TICKS_PER_SECOND * 2)

/**** Data Structures. ****/

/* Structure for storing an encryption algorithm's attributes.
 *
 * WARNING: The first member of this structure must not be
 * moved as it is used to give a common interface to all
 * the algorithm structures.
 */
typedef struct ike_encryption_algo
{
    UINT8           crypto_algo_id;
    UINT8           ike_algo_identifier;    /* Algorithm identifier. */

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    UINT8           ike2_algo_identifier;

    UINT8           ike_pad;
#else
    UINT8           ike_pad[2];
#endif

} IKE_ENCRYPTION_ALGO;

/* Structure for storing a Hash algorithm's attributes.
 *
 * WARNING: The first member of this structure must not be
 * moved as it is used to give a common interface to all
 * the algorithm structures.
 */
typedef struct ike_hash_algo
{
    UINT8           crypto_algo_id;
    UINT8           ike_algo_identifier;    /* Algorithm identifier. */

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    UINT8           ike2_algo_identifier;

    UINT8           ike_pad;
#else
    UINT8           ike_pad[2];
#endif

} IKE_HASH_ALGO;

/* Structure for storing a Signature algorithm's attributes.
 *
 * WARNING: The first member of this structure must not be
 * moved as it is used to give a common interface to all
 * the algorithm structures.
 */
typedef struct ike_sign_algo
{
    UINT8           crypto_algo_id;
    UINT8           ike_algo_identifier;    /* Algorithm identifier. */

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    UINT8           ike2_algo_identifier;

    UINT8           ike_pad;
#else
    UINT8           ike_pad[2];
#endif
} IKE_SIGN_ALGO;

/* Structure for storing the shared key. */
typedef struct ike_shared_key
{
    UINT8           *ike_key;               /* Shared key material. */
    UINT16          ike_key_len;            /* Length of shared key,
                                             * in bytes. */

    UINT8           ike_pad[2];
} IKE_SHARED_KEY;

/* Structure for storing public/private key pair.
 *
 * WARNING: First item of this structure MUST be the public key
 * as it is mapped to the allocated buffer by crypto functions.
 * The memory block should be deallocated using the first element.
 */
typedef struct ike_key_pair
{
    UINT8           *ike_public_key;        /* Public key material. */
    UINT8           *ike_private_key;       /* Private key material. */
    UINT16          ike_public_key_len;     /* Public key length. */
    UINT16          ike_private_key_len;    /* Private key length. */
} IKE_KEY_PAIR;

/* Structure for storing commonly used IKE data. */
typedef struct ike_struct
{
    NU_MEMORY_POOL  *ike_memory;            /* IKE memory pool. */
    NU_TASK         ike_task_cb;            /* IKE Task CB. */
    NU_TASK         ike_event_task_cb;      /* IKE Event Task CB. */
    VOID            *ike_task_stack;        /* IKE task stack pointer. */
    VOID            *ike_event_task_stack;  /* Event task stack pointer. */
    NU_SEMAPHORE    ike_semaphore;          /* IKE semaphore. */
    INT             ike_socket;             /* UDP socket. */

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
    /* UDP socket used when we have a NAT box between us and the peer. */
    INT             ike_natt_socket;
#endif

    /* Index of IPsec inbound SPIs. This variable is initialized
     * to the start of the SPI range assigned to IKE and generates
     * SPIs sequentially.
     */
    UINT32          ike_spi_index;

    /* IKE event group. Last bit of this group is used by the
     * event handler task. All other bits are used to service
     * blocking exchange requests.
     */
    NU_EVENT_GROUP  ike_event_group;

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    NU_EVENT_GROUP  ike2_event_group;
#endif

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
    /* List of message IDs on which a caller is waiting. */
    UINT32          ike_msg_ids[IKE_MAX_WAIT_EVENTS];

    /* Status of phase 2 exchanges corresponding to the
     * entries in ike_msg_ids above.
     */
    STATUS          ike_status[IKE_MAX_WAIT_EVENTS];
#endif
} IKE_STRUCT;

/**** Global variables. ****/

/* Structures for access to all algorithms supported by IKE. */
extern const IKE_ENCRYPTION_ALGO IKE_Encryption_Algos[];
extern const IKE_HASH_ALGO IKE_Hash_Algos[];
extern const IKE_SIGN_ALGO IKE_Sign_Algos[];

/* Structure which contains commonly used IKE data. */
extern IKE_STRUCT IKE_Data;

/* Flag variable to specify current state of the IKE daemon. */
extern UINT8 IKE_Daemon_State;

/**** Function prototypes. ****/

STATUS IKE_Initialize(NU_MEMORY_POOL *memory_pool);
STATUS IKE_Shutdown(VOID);
STATUS IKE_Get_Exchange_Index(UINT32 msg_id, INT *index);
STATUS IKE_Get_Algo_Index(UINT16 ike_id, const VOID *algos, INT algos_size,
                          UINT16 algos_no, UINT16 *index);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_H */

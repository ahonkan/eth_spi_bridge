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
*       ike2.h
*
* COMPONENT
*
*       IKEv2 - Configuration
*
* DESCRIPTION
*
*       Settings and default configuration values for IKEv2.
*
* DATA STRUCTURES
*
*       IKE2_PRF_ALGO           Data structure to hold information about
*                               IKEv2 PRF algorithms.
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef IKE2_H
#define IKE2_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#define IKE2_PSK                        IKE2_AUTH_METHOD_SKEY_MIC
#define IKE2_RSA                        IKE2_AUTH_METHOD_RSA_DS
#define IKE2_EAP                        0

#define IKE2_IS_PSK_METHOD(auth)        ((auth) == IKE2_PSK)
#define IKE2_IS_RSA_METHOD(auth)        ((auth) == IKE2_RSA)
#define IKE2_IS_EAP_METHOD(auth)        ((auth) == IKE2_EAP)

/* Major and minor version number used during encoding messages. */
#define IKE2_MAJOR_VERSION              2
#define IKE2_MINOR_VERSION              0

/* IKE SA time out limits in seconds */
#define IKE_SA_MAX_TIMEOUT              (86400*365)

/* Length of cookie in bytes. */
#define IKE2_COOKIE_LENGTH              64

/* Fixed length of hash to be computed for cookie generation.
 * hash + id = Lengh.of cookie.
 */
#define IKE2_HASH_LENGTH                16

/* Flags for identification as being initiator or responder.
 * This information is stored in the SA flags.
 */
#define IKE2_INITIATOR                  IKE_INITIATOR
#define IKE2_RESPONDER                  IKE_RESPONDER

/* Defines for types being used from version 1 to make naming
 * convention consistent for version 2.
 */
typedef struct ike_sa               IKE2_SA;
typedef struct ike_packet           IKE2_PACKET;
typedef struct ike_policy           IKE2_POLICY;
typedef struct ike_policy_group     IKE2_POLICY_GROUP;
typedef struct ike_initiate_req     IKE2_INITIATE_REQ;
typedef struct ike_sadb             IKE2_SADB;
typedef struct ike_key_pair         IKE2_KEY_PAIR;
typedef struct ike_sa2              IKE2_IPS_SA;
typedef struct ike_sa2_db           IKE2_IPS_SADB;
typedef struct ike_event_db         IKE2_EVENT_DB;
typedef struct ike_event            IKE2_EVENT;
typedef struct ike_attrib           IKE2_ATTRIB;
typedef struct ike_policy_selector  IKE2_POLICY_SELECTOR;

typedef struct ike2_hmac_algos
{
    UINT8       ike2_algo_id;       /* Algorithm ID as used in IKEv2. */
    UINT8       ike2_key_len;       /* Key length for this algorithm. */
    UINT8       ike2_output_len;    /* Output length. */
    UINT8       ike2_pad;
}IKE2_HMAC_ALGOS;

typedef struct ike2_prf_algo
{
    /* The first member of this structure is used for consistency with
     * IKEv1's algorithm ID structures and to dereference correct member
     * when this structure is cast to IKE_HASH_ALGO*.
     */
    UINT8           crypto_algo_id;
    UINT8           ikev1_pad;
    UINT8           ike2_algo_identifier;    /* Algorithm identifier. */
    UINT8           ike_pad;
} IKE2_PRF_ALGO;

typedef struct ike_encryption_algo  IKE2_ENCRYPTION_ALGO;
typedef struct ike_hash_algo        IKE2_HASH_ALGO;
typedef struct ike_sign_algo        IKE2_SIGN_ALGO;

extern const IKE2_PRF_ALGO IKE2_PRF_Algos[];
extern const IKE2_HMAC_ALGOS IKE2_PRF_HMAC_Algos[];
extern const IKE2_HMAC_ALGOS IKE2_AUTH_HMAC_Algos[];

/* Macros for looking up algorithms in IKEv2. */
#define IKE2_HASH_ALGO_INDEX(id, ind)       IKE2_Get_Algo_Index((id),     \
                                             IKE_Hash_Algos,              \
                                             sizeof(IKE2_HASH_ALGO),      \
                                             IKE_TOTAL_HASH_ALGO,         \
                                             &(ind))
#define IKE2_SIGN_ALGO_INDEX(id, ind)       IKE2_Get_Algo_Index((id),     \
                                             IKE_Sign_Algos,              \
                                             sizeof(IKE2_SIGN_ALGO),      \
                                             IKE_TOTAL_SIGN_ALGO,         \
                                             &(ind))
#define IKE2_ENCRYPTION_ALGO_INDEX(id, ind) IKE2_Get_Algo_Index((id),     \
                                             IKE_Encryption_Algos,        \
                                             sizeof(IKE2_ENCRYPTION_ALGO),\
                                             IKE_TOTAL_ENCRYPTION_ALGO,   \
                                             &(ind))
#define IKE2_PRF_ALGO_INDEX(id, ind)        IKE2_Get_Algo_Index((id),     \
                                             IKE2_PRF_Algos,              \
                                             sizeof(IKE2_PRF_ALGO),       \
                                             IKE2_TOTAL_PRF_ALGOS,        \
                                             &(ind))

#define IKE2_DEBUG          IKE_DEBUG
#define IKE2_DEBUG_LOG(str) IKE_DEBUG_LOG(str)

/* Error Codes for IKEv2. The range for these error codes is
 * -4830 to -4899. Some error codes are mapped to IKEv1 errors
 * where causes of error are similar.
 */
#define IKE2_SA_INIT_TIMEOUT            -4830
#define IKE2_CREATE_CHILD_SA_TIMEOUT    -4831
#define IKE2_TOO_FEW_TRANSFORMS         -4832
#define IKE2_INVALID_PROPOSAL_NUM       -4833
#define IKE2_INVALID_SPI_SIZE           -4834
#define IKE2_TS_MISMATCH                -4835
#define IKE2_INVALID_MSG_TYPE           -4836
#define IKE2_INVALID_KE_PAYLOAD         -4837

#define IKE2_INVALID_PARAMS             IKE_INVALID_PARAMS
#define IKE2_INVALID_MNR_VER            IKE_INVALID_MNR_VER
#define IKE2_INVALID_MSGID              IKE_INVALID_MSGID
#define IKE2_UNEXPECTED_PAYLOAD         IKE_UNEXPECTED_PAYLOAD
#define IKE2_UNEXPECTED_MESSAGE         IKE_UNEXPECTED_MESSAGE
#define IKE2_INVALID_SPI                IKE_INVALID_SPI
#define IKE2_COOKIE_MISMATCHED          IKE_INVALID_COOKIE

/* Macros for policy algorithm match / mismatch */
#define IKE2_NOT_FOUND                   NU_FALSE
#define IKE2_MATCHED                     NU_TRUE
#define IKE2_NOT_MATCHED                 IKE_NOT_FOUND

STATUS IKE2_Get_Algo_Index(UINT16 ike_id, const VOID *algos, INT algos_size,
                           UINT16 algos_no, UINT16 *index);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_H */

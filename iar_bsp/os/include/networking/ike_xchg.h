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
*       ike_xchg.h
*
* COMPONENT
*
*       IKE - IKE Exchange
*
* DESCRIPTION
*
*       This file defines constants, data structures and function
*       prototypes required to implement the IKE Exchange Modes.
*
* DATA STRUCTURES
*
*       IKE_LARGE_DATA_STRUCTS
*       IKE_AUTH_DEC_PAYLOADS
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_XCHG_H
#define IKE_XCHG_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** IKE Exchange constants. ****/

/* ISAKMP definition of modes from RFC2408. */
#define IKE_XCHG_NONE                   0
#define IKE_XCHG_BASE                   1
#define IKE_XCHG_IDPROT                 2
#define IKE_XCHG_AUTHONLY               3
#define IKE_XCHG_AGGR                   4
#define IKE_XCHG_INFO                   5

/* IKE definition of modes from RFC2409. */
#define IKE_XCHG_MAIN                   IKE_XCHG_IDPROT
#define IKE_XCHG_QUICK                  32
#define IKE_XCHG_NEW_GRP                33

/* Flags for checking attribute presence. */
#define IKE_ATTRIB_ENC_ALGO_FLAG        0x01
#define IKE_ATTRIB_HASH_ALGO_FLAG       0x02
#define IKE_ATTRIB_AUTH_METHOD_FLAG     0x04
#define IKE_ATTRIB_GRP_DESC_FLAG        0x08
#define IKE_ATTRIB_GRP_TYPE_FLAG        0x10
#define IKE_ATTRIB_LIFE_TYPE_FLAG       0x20
#define IKE_ATTRIB_LIFE_DUR_FLAG        0x40
#define IKE_ATTRIB_KEY_LEN_FLAG         0x80

/* Macro to determine whether an exchange mode is for
 * Phase 1 or Phase 2. Informational mode is used in
 * both Phases but is handled like a Phase 1 message.
 */
#define IKE_IS_PHASE1_MODE(mode)        ((mode == IKE_XCHG_MAIN)     || \
                                         (mode == IKE_XCHG_AGGR)     || \
                                         (mode == IKE_XCHG_BASE)     || \
                                         (mode == IKE_XCHG_AUTHONLY) || \
                                         (mode == IKE_XCHG_NEW_GRP))
#define IKE_IS_PHASE2_MODE(mode)        ((mode == IKE_XCHG_QUICK)    || \
                                         (mode == IKE_XCHG_INFO))

/* Values for IKE state machine states. */
#define IKE_COMPLETE_STATE              255
#define IKE_INITIATOR_START_STATE       1
#define IKE_RESPONDER_START_STATE       2

/* Increment value for moving to the next state. */
#define IKE_NEXT_STATE_INC              2

/* Phase 2 states mapped to HASH(x) verification. */
#define IKE_RECV_HASH1_STATE            2
#define IKE_RECV_HASH2_STATE            3
#define IKE_RECV_HASH3_STATE            4
#define IKE_RECV_HASH4_STATE            5
#define IKE_SEND_HASH1_STATE            1
#define IKE_SEND_HASH2_STATE            2
#define IKE_SEND_HASH3_STATE            3
#define IKE_SEND_HASH4_STATE            4

/* Phase 1 states after which Informational Mode
 * encryption is required (not inclusive).
 */
#define IKE_MAIN_ENCRYPT_INFO_STATE     5
#define IKE_AGGR_ENCRYPT_INFO_STATE     3

/* Total states in each state machine. */
#define IKE_TOTAL_MAIN_MODE_STATES      7
#define IKE_TOTAL_AGGR_MODE_STATES      4
#define IKE_TOTAL_QUICK_MODE_STATES     5

/* Macro to calculate the hash digest of a received packet
 * and store it in the Phase 1 or Phase 2 Handle. This must
 * be invoked every time a message is sent successfully.
 */
#define IKE_UPDATE_RESEND(ph12)                                         \
            IKE_MD5_Hash((ph12)->ike_params->ike_packet->ike_data,      \
                         (ph12)->ike_params->ike_packet->ike_data_len,  \
                         (ph12)->ike_last_message_hash)

/* Macros to calculate the hash digest of an encrypted message
 * and store it in a temporary buffer. The second macro is used
 * to copy the calculated digest to the SA. The combination of
 * both these macros for encrypted packets is equivalent to the
 * IKE_UPDATE_RESEND macro for un-encrypted packets.
 */
#define IKE_HASH_ENC_RESEND(ph12, dgst)                                 \
            IKE_MD5_Hash((ph12)->ike_params->ike_packet->ike_data,      \
                         (ph12)->ike_params->ike_packet->ike_data_len,  \
                         (dgst))
#define IKE_UPDATE_ENC_RESEND(ph12, dgst)                               \
            NU_BLOCK_COPY((ph12)->ike_last_message_hash, (dgst),        \
                          IKE_MD5_DIGEST_LEN)

/**** IKE Exchange data structures and data types. ****/

/* The following structure is used to avoid allocation of
 * large IKE Exchange data structures on the stack. Instead,
 * all the large data is declared globally and is protected
 * by the IKE semaphore.
 *
 * WARNING: Converting local data to global data might cause
 * severe synchronization problems. Be careful when modifying
 * this structure or functions which use its data members.
 */
typedef struct ike_large_data_structs
{
    IKE_SA                  ike_sa;         /* IKE SA. */
    IKE_PHASE1_HANDLE       ike_phase1;     /* Phase 1 Handle. */
    IKE_PHASE2_HANDLE       ike_phase2;     /* Phase 2 Handle. */
    IKE_SA_ENC_PAYLOAD      ike_enc_sa;     /* SA payload for encoding. */
    IKE_SA_DEC_PAYLOAD      ike_dec_sa;     /* SA payload for decoding. */
    IKE_ID_ENC_PAYLOAD      ike_enc_idi;    /* ID payload for encoding. */

    /* Union of Responder's ID payload and raw ID data. */
    union
    {
        IKE_ID_ENC_PAYLOAD  ike_enc_idr;
        UINT8               ike_raw_id[IKE_MIN_ID_PAYLOAD_LEN +
                                       IKE_MAX_ID_DATA_LEN];
    } ike_id;
} IKE_LARGE_DATA_STRUCTS;

/* Union to save stack space in the IKE_Aggr_State_3_Recv and
 * IKE_Aggr_4_Main_6_7_Recv functions. Members of this union
 * are used independently in the above functions.
 */
typedef union ike_auth_dec_payloads
{
    IKE_HASH_DEC_PAYLOAD        dec_hash;   /* Hash decoding payload. */
    IKE_SIGNATURE_DEC_PAYLOAD   dec_sig;    /* Signature decoding
                                             * payload.
                                             */
} IKE_AUTH_DEC_PAYLOADS;

/* Function prototype for Main Mode state handler. */
typedef STATUS (*IKE_MAIN_MODE_FUNC)(IKE_PHASE1_HANDLE *ph1);

/* Function prototype for Aggressive Mode state handler. */
typedef STATUS (*IKE_AGGR_MODE_FUNC)(IKE_PHASE1_HANDLE *ph1);

/* Function prototype for Quick Mode state handler. */
typedef STATUS (*IKE_QUICK_MODE_FUNC)(IKE_PHASE2_HANDLE *ph2);

/**** Global variables. ****/

/* Large data structures used by IKE Exchanges. */
extern IKE_LARGE_DATA_STRUCTS IKE_Large_Data;

/**** Function prototypes. ****/

/* Common utility functions shared by different exchange modes. */
STATUS IKE_Construct_Proposal(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Select_Proposal(IKE_PHASE1_HANDLE *ph1,
                           IKE_ATTRIB **ret_policy_attrib);
STATUS IKE_Convert_Attributes(IKE_DATA_ATTRIB *attribs,
                              UINT8 num_attribs,
                              IKE_ATTRIB *sa_attrib);
STATUS IKE_Verify_Attributes(IKE_ATTRIB *attrib,
                             IKE_ATTRIB *attrib_list,
                             UINT8 num_attribs,
                             IKE_ATTRIB **out_attrib);
STATUS IKE_Verify_Selection_SA(IKE_SA_DEC_PAYLOAD *dec_sa);
STATUS IKE_Auth_Parameters(IKE_ATTRIB *policy_attrib, IKE_SA *sa,
                           IKE_ID_DEC_PAYLOAD *dec_id);
STATUS IKE_Generate_KeyXchg_Data(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Generate_Nonce_Data(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Generate_Cert_Data(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Generate_CertReq_Data(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Get_Local_ID(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Generate_ID_Data(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Generate_Hash_Data(IKE_PHASE1_HANDLE *ph1,
                              UINT8 *id_b, UINT16 id_b_len);
STATUS IKE_Verify_Auth_Data(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Aggr_4_Main_6_7_Recv(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Payload_To_Identifier(IKE_ID_DEC_PAYLOAD *id_pload,
                                 IKE_IDENTIFIER *id);
STATUS IKE_Lookup_Preshared_Key(IKE_SA *sa, IKE_ID_DEC_PAYLOAD *dec_id);
STATUS IKE_Free_Phase1_Memory(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Free_Phase2_Memory(IKE_PHASE2_HANDLE *ph2);
STATUS IKE_Finalize_Phase1(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Finalize_Phase2(IKE_PHASE2_HANDLE *ph2);
STATUS IKE_Resume_Waiting_Process(IKE_PHASE2_HANDLE *ph2,
                                  STATUS xchg_status);
STATUS IKE_Resume_Waiting_Processes(IKE_SA *sa, STATUS xchg_status);
STATUS IKE_Update_Phase2_Status(IKE_PHASE2_HANDLE *ph2,
                                STATUS xchg_status);
STATUS IKE_Update_Phase1_Status(IKE_SA *sa, STATUS xchg_status);
STATUS IKE_Check_Resend(IKE_PHASE1_HANDLE *ph1);

/* Functions related to IKE Main Mode. */
STATUS IKE_Process_Main_Mode(IKE_PHASE1_HANDLE *ph1);

/* Functions related to IKE Aggressive Mode. */
STATUS IKE_Process_Aggr_Mode(IKE_PHASE1_HANDLE *ph1);

/* Functions related to IKE Quick Mode. */
STATUS IKE_Process_Quick_Mode(IKE_PHASE2_HANDLE *ph2);

/* Functions related to IKE Informational Mode. */
STATUS IKE_Send_Informational(IKE_SA *sa, IKE_GEN_HDR *enc_pload);
STATUS IKE_Notify_Error(IKE_SA *sa, STATUS error_status);
STATUS IKE_Delete_Notification(IKE_SA *sa, UINT8 proto, UINT8 *spi);
STATUS IKE_Check_Initial_Contact(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_Process_Initial_Contact(IKE_PHASE2_HANDLE *ph2);
STATUS IKE_Process_Delete(IKE_PHASE2_HANDLE *ph2);
STATUS IKE_Process_Notify(IKE_PHASE2_HANDLE *ph2);
STATUS IKE_Process_Info_Mode(IKE_PHASE2_HANDLE *ph2);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_XCHG_H */

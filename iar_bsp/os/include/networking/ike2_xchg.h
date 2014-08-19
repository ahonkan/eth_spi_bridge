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
*       ike2_xchg.h
*
* COMPONENT
*
*       IKE2 - IKE2 Exchange
*
* DESCRIPTION
*
*       This file defines constants, data structures and function
*       prototypes required to implement the IKE2 Exchange Modes.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE2_XCHG_H
#define IKE2_XCHG_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/* IKEv2 exchange states. These correspond to indices in the state
 * machine handler functions array.
 */
#define IKE2_STATE_SA_INIT_I            0
#define IKE2_STATE_SA_INIT_R            1
#define IKE2_STATE_AUTH_I               2
#define IKE2_STATE_AUTH_R               3
#define IKE2_STATE_CREATE_CHILD_SA_I    5
#define IKE2_STATE_CREATE_CHILD_SA_R    6

/* Fixed string to be padded to the end of key before calculating PRF on
 * it. This string is defined by RFC 4306.
 */
#define IKE2_PSK_PAD_STRING            "Key Pad for IKEv2"
#define IKE2_PSK_PAD_LENGTH            17

/* Statuses for IKEv2 state machine. */
#define IKE2_COMPLETE_STATE             IKE_COMPLETE_STATE
#define IKE2_INITIATOR_START_STATE      IKE2_STATE_SA_INIT_I
#define IKE2_RESPONDER_START_STATE      IKE2_STATE_SA_INIT_R

#define IKE2_NONCE_MIN_LEN              32

/* Increment value for moving to the next state. */
#define IKE2_NEXT_STATE_INC             2

/* Macros related to cookie processing. */
#define IKE2_SECRET_LENGTH              16

/* Number of seconds a secret would live. (One day) */
#define IKE2_SECRET_KEEP_ALIVE_TIME     86400


/* IKEv2 Exchanges. */
#define IKE2_TOTAL_EXCHANGE_STATES      8

typedef STATUS (*IKE2_STATE_MACHINE_FUNCS)(IKE2_EXCHANGE_HANDLE *handle);

STATUS IKE2_Construct_Proposal(IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Select_Proposal(IKE2_EXCHANGE_HANDLE *handle,
                            IKE2_ATTRIB **ret_policy_attrib);
STATUS IKE2_Extract_IKE_SA_Attribs(IKE2_ATTRIB *attribs, IKE2_SA *sa);
STATUS IKE2_Construct_IPsec_Proposal(IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Select_IPsec_Proposal(IKE2_EXCHANGE_HANDLE *handle,
                                  IPSEC_SECURITY_PROTOCOL *security,
                                  UINT8 security_size);
STATUS IKE2_Cleanup_Exchange(IKE2_EXCHANGE_HANDLE *handle);

STATUS IKE2_Generate_KE_Data(IKE2_EXCHANGE_HANDLE *handle,
                             IKE2_KE_PAYLOAD *ke);
STATUS IKE2_Generate_Nonce_Data(IKE2_EXCHANGE_HANDLE *handle,
                                IKE2_NONCE_PAYLOAD *nonce);
STATUS IKE2_Generate_ID_Data(IKE2_EXCHANGE_HANDLE *handle,
                             IKE2_ID_PAYLOAD *id);
STATUS IKE2_Generate_AUTH_Data(IKE2_EXCHANGE_HANDLE *handle,
                               IKE2_AUTH_PAYLOAD *auth);
STATUS IKE2_Generate_Cert_Data(IKE2_EXCHANGE_HANDLE *handle,
                              IKE2_CERT_PAYLOAD *cert);
STATUS IKE2_Generate_CertReq_Data(IKE2_EXCHANGE_HANDLE *handle,
                                 IKE2_CERT_REQ_PAYLOAD *cert_req);
STATUS IKE2_Verify_TS(IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Generate_TS(IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Extract_TS_Info(IKE2_EXCHANGE_HANDLE *handle, UINT16 index_i, UINT16 index_r);

STATUS IKE2_Verify_Auth(IKE2_EXCHANGE_HANDLE *handle);

/* IKEv2 IKE_SA_INIT and IKE_AUTH exchanges. */
STATUS IKE2_Process_SA_INIT(IKE2_EXCHANGE_HANDLE *handle);

/*IKEv2 CREATE_CHILD_SA and IKE_INFORMATIONAL exchanges. */
STATUS IKE2_Process_CREATE_CHILD_SA(IKE2_EXCHANGE_HANDLE *handle);

/* Construct the IKEv2 header. */
STATUS IKE2_Construct_Header(IKE2_EXCHANGE_HANDLE *handle, UINT8 xchg,
                             UINT8 flags);

/* Pre-shared key lookup function. */
STATUS IKE2_Lookup_Preshared_Key(IKE2_SA *sa, IKE2_ID_PAYLOAD *id);

/* Calculate the key length for encryption algorithms. */
STATUS IKE2_Compute_Encryption_KeyLen(IPSEC_SECURITY_PROTOCOL *security,
                                      UINT16 *key_len);

/* Replaces the old IKE SA with the newly negotiated (re-keyed) SA. */
STATUS IKE2_Replace_SA(IKE2_EXCHANGE_HANDLE *handle, IKE2_SA *new_sa);

/* Prototype of functions relating to cookies */
STATUS IKE2_Generate_Secret(VOID);
STATUS IKE2_Save_Cookie(IKE2_EXCHANGE_HANDLE *, IKE2_NOTIFY_PAYLOAD *);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_XCHG_H */

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
*       ike2_db.h
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
*       IKE2_EXCHANGE_DB
*       IKE2_EXCHANGE_HANDLE
*       IKE2_STATE_PARAMS

* DEPENDENCIES
*
*       ike_api.h
*       ike2.h
*       ike2_pload.h
*
*************************************************************************/
#ifndef IKE2_DB_H
#define IKE2_DB_H

#include "networking/ike2.h"
#include "networking/ike2_pload.h"

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#define IKE2_SA_ESTABLISHED             IKE_SA_ESTABLISHED
#define IKE2_SA_INIT_INCOMPLETE         IKE_SA_INCOMPLETE

/* Parameters structure containing data that is needed to complete
 * processing at a particular state in the IKEv2 state machine.
 */
typedef struct ike2_state_params
{
    IKE2_MESSAGE        ike2_out;
    IKE2_MESSAGE        ike2_in;
    IKE2_PACKET         *ike2_packet;
    IKE2_POLICY         *ike2_policy;
    IKE2_POLICY_GROUP   *ike2_group;
} IKE2_STATE_PARAMS;

/* The information about and exchange underway.
 */
typedef struct ike2_exchange_handle
{
    struct ike2_exchange_handle *ike2_flink;
    IKE2_STATE_PARAMS           *ike2_params;
    IKE2_SA                     *ike2_sa;
    IKE2_SA                     *ike2_new_sa; /* Used when re-keying */
    IKE2_KEY_PAIR               ike2_dh_keys;
    IPSEC_SELECTOR              ike2_ips_selector;
    IPSEC_SELECTOR              ike2_ips_pol_selector;
    IKE2_IPS_SADB               ike2_sa2_db;
    IPSEC_SA_LIFETIME           ike2_ips_lifetime;
    UINT8                       *ike2_remote_dh;
    IKE2_POLICY                 *ike2_policy;
    UINT32                      ike2_lifetime;
    UINT8                       ike2_cookie[IKE2_COOKIE_LENGTH];
    UINT8                       *ike2_nonce;
    UINT8                       *ike2_peer_nonce;
    UINT8                       *ike2_last_message;
    UINT8                       ike2_last_msg_hash[IKE_MD5_DIGEST_LEN];
    UINT8                       *ike2_encrypt_iv;
    UINT8                       *ike2_decrypt_iv;

    /* IPsec policy group name. */
    CHAR                        ike2_ips_group[IKE_MAX_GROUP_NAME_LEN];
    UINT32                      ike2_ips_policy_index;

    UINT32                      ike2_next_id_nu;
    UINT32                      ike2_next_id_peer;
    UINT32                      ike2_flags;    
    UINT16                      ike2_nonce_len;
    UINT16                      ike2_peer_nonce_len;

    /* Need to save the socket from which we received the packet. This
     * can either be 500 or 4500 (when NAT-T is enabled).
     * NOTE: NAT-T is not yet supported.
     */
    INT                         ike2_socket;

    UINT16                      ike2_remote_dh_len;
    UINT8                       ike2_state;
    UINT8                       ike2_resend_count;
    UINT8                       ike2_cookie_len;
    UINT8                       ike2_padN[3];

} IKE2_EXCHANGE_HANDLE;

/* Structure to maintain a list of all exchange handles open at any
 * particular time.
 */
typedef struct ike2_exchange_db
{
    IKE2_EXCHANGE_HANDLE       *ike2_flink;
    IKE2_EXCHANGE_HANDLE       *ike2_last_link;

} IKE2_EXCHANGE_DB;


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif

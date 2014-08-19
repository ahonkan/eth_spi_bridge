/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       usm.h                                                    
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used in the User-based Security Model.
*
*   DATA STRUCTURES
*
*       USM_STATS_STRUCT
*       USM_USERS_STRUCT
*       USM_MIB_STRUCT
*       USM_AUTH_PROT_STRUCT
*       USM_PRIV_PROT_STRUCT
*       USM_CACHED_SECURITY_STRUCT
*       SNMP_USM_USER_STRUCT
*       USM_USERS_TABLE_ROOT
*
*   DEPENDENCIES
*
*       snmp_dis.h
*
*************************************************************************/

#ifndef USM_H
#define USM_H

#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* USM Authentication protocols. */
#define USM_NOAUTH                        0
#define USM_MD5                           1
#define USM_SHA                           2

/* USM Privacy protocols. */
#define USM_NOPRIV                        0
#define USM_DES                           1
#define USM_DES_PRIV_KEY_SIZE             16

/* MIB Structures for USM. */
#define USM_KEYCHANGE_MAX_SIZE            40

#define USM_UNSUPPORTED_SECURITY_LEVEL    -2
#define USM_ENCRYPTION_ERROR              -3
#define USM_DECRYPTION_ERROR              -4
#define USM_AUTHENTICATION_ERROR          -5
#define USM_PARSE_ERROR                   -6
#define USM_UNKNOWNENGINEID_ERROR         -7
#define USM_UNKNOWNUSERNAME_ERROR         -8
#define USM_NOTINTIMEWINDOW_ERROR         -9
#define USM_UNKOWNSECURITYNAME_ERROR      -10

/* Defines number of cached data entries. We only support one at this
 * moment.
 */
#define USM_MAX_CACHED_DATA                SNMP_CR_QSIZE

/* Function Types */

/* Authentication */
typedef STATUS (*USM_AUTH_OUTGOING_STRUCT)
 (UINT8 *, UINT8, UINT8 *, UINT32, UINT8 *);

typedef STATUS (*USM_AUTH_INCOMING_STRUCT)
 (UINT8 *, UINT8, UINT8 *, UINT32, UINT8 *, UINT32);

 typedef STATUS (*USM_KEY_CHANGE)
 (UINT8*, UINT8 *, UINT32);

/* Privacy */
typedef STATUS (*USM_ENCRYPT_STRUCT)
 (UINT8 *, UINT8, UINT8 **, UINT32 *, UINT8 *, UINT32 *, UINT16);

typedef STATUS (*USM_DECRYPT_STRUCT)
 (UINT8 *, UINT8, UINT8 *, UINT32, UINT8 **, UINT32 *);

/* Hashing */
typedef STATUS (*USM_HASH_STRUCT)
 (UINT8 *, UINT8, UINT8 *);


typedef struct usm_stats_struct
{
    UINT32    usm_unsupported_sec_level;
    UINT32    usm_not_in_time_win;
    UINT32    usm_unkown_user_name;
    UINT32    usm_unknown_engine_id;
    UINT32    usm_wrong_digests;
    UINT32    usm_decryption_err;
} USM_STATS_STRUCT;

typedef struct usm_users_struct
{
    struct usm_users_struct     *next;
    struct usm_users_struct     *previous;

    UINT32          usm_auth_index;
    UINT32          usm_priv_index;

    UINT8           usm_user_engine_id[SNMP_SIZE_SMALLOBJECTID];
    UINT8           usm_user_name[SNMP_SIZE_SMALLOBJECTID];
    UINT8           usm_security_name[SNMP_SIZE_SMALLOBJECTID];

    /*following three are for clone-from*/
    UINT8           usm_cf_engine_id[SNMP_SIZE_SMALLOBJECTID];
    UINT8           usm_cf_user_name[SNMP_SIZE_SMALLOBJECTID];

    UINT8           usm_auth_key[USM_KEYCHANGE_MAX_SIZE];
    UINT8           usm_own_auth_key[USM_KEYCHANGE_MAX_SIZE];

    UINT8           usm_priv_key[USM_KEYCHANGE_MAX_SIZE];
    UINT8           usm_own_priv_key[USM_KEYCHANGE_MAX_SIZE];
    UINT8           usm_user_public[SNMP_SIZE_SMALLOBJECTID];

    UINT8           usm_cf_engine_id_len;
    UINT8           usm_user_engine_id_len;
#if (INCLUDE_MIB_USM == NU_TRUE)
    UINT8           usm_storage_type;
    UINT8           usm_status;
    UINT8           usm_row_flag;

    /* Make the structure word-aligned. */
    UINT8           usm_pad[3];
#else
    UINT8           usm_pad[2];
#endif
} USM_USERS_STRUCT;

typedef struct usm_users_table_root
{
    struct usm_users_struct      *next;
    struct usm_users_struct      *previous;

}USM_USERS_TABLE_ROOT;

typedef struct usm_mib_struct
{
    USM_STATS_STRUCT             usm_stats_tab;
#if (INCLUDE_MIB_USM == NU_TRUE)
    UINT32                       usm_user_spin_lock;
#endif
    USM_USERS_TABLE_ROOT         usm_user_table;
} USM_MIB_STRUCT;


/* Authentication Protocols Structure. */

typedef struct usm_auth_prot_struct
{
    UINT32                              usm_index;
    USM_AUTH_OUTGOING_STRUCT            usm_secure_cb;
    USM_AUTH_INCOMING_STRUCT            usm_verify_cb;
    USM_HASH_STRUCT                     usm_password_cb;
    USM_KEY_CHANGE                      usm_key_change_cb;
    UINT32                              usm_param_length;
    UINT8                               key_length;

    /* Make the structure word-aligned. */
    UINT8                               usm_pad[3];
} USM_AUTH_PROT_STRUCT;


/* Privacy Protocols Structure. */

typedef struct usm_priv_prot_struct
{
    UINT32                              usm_index;
    USM_ENCRYPT_STRUCT                  usm_encrypt_cb;
    USM_DECRYPT_STRUCT                  usm_decrypt_cb;
    UINT8                               usm_param_length;

    /* Make the structure word-aligned. */
    UINT8                               usm_pad[3];
} USM_PRIV_PROT_STRUCT;

/* Cached Security Entry. */

typedef struct usm_cached_security_struct
{
    USM_AUTH_PROT_STRUCT       *usm_auth_protocol;
    USM_PRIV_PROT_STRUCT       *usm_priv_protocol;
    UINT8                      usm_msg_user_name[SNMP_SIZE_SMALLOBJECTID];
    UINT8                      usm_auth_key[USM_KEYCHANGE_MAX_SIZE];
    UINT8                      usm_priv_key[USM_KEYCHANGE_MAX_SIZE];
    BOOLEAN                    usm_is_occupied;

} USM_CACHED_SECURITY_STRUCT;

typedef struct snmp_usm_user_struct
{
    CHAR        usm_user_name[SNMP_SIZE_SMALLOBJECTID];
    UINT32      usm_auth_index;
    CHAR        usm_auth_password[USM_KEYCHANGE_MAX_SIZE];
    UINT32      usm_priv_index;
    CHAR        usm_priv_password[USM_KEYCHANGE_MAX_SIZE];
} SNMP_USM_USER_STRUCT;

/* Functions invoked by the Security Subsystem. */
STATUS USM_Init(VOID);
STATUS USM_Config(VOID);

STATUS USM_Secure(UINT32 snmp_mp, UINT8 **whole_message, UINT32 *msg_len,
        UINT32 max_msg_size, UINT32 snmp_sm, UINT8 *security_engine_id,
        UINT32 security_engine_id_len, UINT8 *security_name,
        UINT8 security_level, UINT8 *scoped_pdu,
        VOID *security_state_ref); 

STATUS USM_Verify(UINT32 snmp_mp, UINT32 max_msg_size,
                  UINT8 *security_param, UINT32 snmp_sm,
                  UINT8 *security_level, UINT8 **whole_message,
                  UINT32 *msg_len, UINT8 *security_engine_id,
                  UINT32 *security_engine_id_len, UINT8 *security_name,
                  UINT32 *max_response_pdu, VOID **security_state_ref,
                  SNMP_ERROR_STRUCT *error_indication);

STATUS USM_Key_Change(UINT32 usm_auth_index, UINT8 *current_key,
                      UINT8 *new_key, UINT32 new_key_len);
STATUS USM_Add_User_Util(USM_USERS_STRUCT * node,
                         USM_USERS_TABLE_ROOT *root);
STATUS USM_Add_User(USM_USERS_STRUCT *node);
STATUS USM_Remove_User(USM_USERS_STRUCT *user);
USM_USERS_STRUCT* USM_Lookup_Users(UINT8 *msg_user_name);
USM_PRIV_PROT_STRUCT* USM_Lookup_Priv_Prot(UINT32 index);
USM_AUTH_PROT_STRUCT* USM_Lookup_Auth_Prot(UINT32 index);
INT32 USM_Compare_Index(VOID *left_side, VOID *right_side);
STATUS USM_Save_User (USM_USERS_STRUCT *user);

#if (INCLUDE_MIB_USM == NU_TRUE)
USM_USERS_STRUCT *Get_USM_User_Entry_Util(const UINT8 *engine_id,
                                          UINT32 engine_id_len,
                                          const UINT8 *user_name,
                                          UINT32 user_name_len,
                                        const USM_USERS_TABLE_ROOT *root);

USM_USERS_STRUCT *Get_USM_User_Entry(UINT8 *engine_id,
                                   UINT32 engine_id_len, UINT8 *user_name,
                                   UINT32 user_name_len, UINT8 getlfag);
#endif

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* USM_H */

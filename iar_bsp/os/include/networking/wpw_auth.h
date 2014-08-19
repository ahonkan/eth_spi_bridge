/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/************************************************************************
*                                                                       
* FILE NAME                                                            
*                                                                       
*       wpw_auth.h                                         
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Nucleus WebServ
*
* DESCRIPTION                                                           
*                                                                       
*       This file contains the linked list defines used in manipulating  
*       the authentication structure for multiple linked lists.          
*                                                                       
* DATA STRUCTURES                                                       
*             
*       WPW_AUTH_NODE       Holds user ID with associated password
*       WPW_INFO_NODE       Entry into the authentication string
*       WPW_INFO_LIST       Defines the head of the linked list of
*                           the Nucleus WebServ authentication
*                           password structure
*                                                          
* FUNCTIONS                                                             
*
*       None
*                                                                       
* DEPENDENCIES                                                          
*
*       None
*                                                            
************************************************************************/

#ifndef _WPW_AUTH_H
#define _WPW_AUTH_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif

#ifdef NET_5_1
#include "networking/um_defs.h"
#endif
    
#define WPW_BASIC_AUTH_FAILED   -1
#define WPW_BASIC_MAX_SIZE      76

#define WPW_SRG_SIZE            128     
#define WPW_SALT_BYTES          16

/*  Structure to hold the user_id and password for Authetication
 *  algorithm
 */
struct WPW_AUTH_NODE{
    
     CHAR           wpw_user_id[WS_AUTH_NAME_LEN];
     CHAR           wpw_password[WS_AUTH_NAME_LEN];
};

/* This structure defines An entry into the Basic Authentication string. */
typedef struct _WPW_INFO_NODE{
    
    struct _WPW_INFO_NODE    *wpw_list_next;        
    struct _WPW_INFO_NODE    *wpw_list_previous;    
    CHAR                     wpw_user[WS_AUTH_NAME_LEN];          /*  user id  */
    CHAR                     wpw_password[WS_AUTH_NAME_LEN];      /*  Password */
} WPW_INFO_NODE;

/* Define the head of the linked list of the Nucleus WebServ Basic
 * Authetication Password structure.
 */
typedef struct _WPW_INFO_LIST{
    
    WPW_INFO_NODE    *wpw_list_head;         /* Basic Password List Head */
    WPW_INFO_NODE    *wpw_list_tail;         /* Basic Passowrd List Tail */
} WPW_INFO_LIST;


/* Function Prototypes for Nucleus WebServ Basic Authentication */
#if INCLUDE_BASIC_AUTH
INT16   BSC_Auth_Init(VOID);
STATUS  BSC_Parse_Auth(WS_REQUEST *req);
VOID    BSC_Base64_Decode(UINT8 *total_char, UINT8 final_decode[]);
INT16   BSC_Auth_Add_Entry(CHAR *user_id, CHAR *password);
INT16   BSC_Auth_Delete_Entry(CHAR *user_id, CHAR *password);
#endif

/* Function prototypes for DES Authentication */
#if INCLUDE_DES_AUTH
VOID    DES_Auth_Initialize(WS_SERVER *server);
VOID    DES_Check_Timeout(WS_REQUEST * req);
INT     DES_Authenticated_Check(WS_REQUEST * req);
VOID    ENC_Decrypt(CHAR * key, CHAR * data, INT blocks);
VOID    ENC_Encrypt(CHAR * key, CHAR * data, INT blocks);

#define HTTP_Check_Timeout          DES_Check_Timeout
#define HTTP_Authenticated_Check    DES_Authenticated_Check
#endif


/* Function prototypes for JS Authentication */
#if INCLUDE_JS_AUTH
STATUS  JS_Init_Auth(WS_SERVER *server);
STATUS  JS_Auth_Check(WS_REQUEST *req);
VOID    JS_Check_Timeout(WS_REQUEST *req);
STATUS  JS_Add_User(CHAR *user, CHAR *password);
STATUS  JS_Delete_User(CHAR *user, CHAR *password);
INT     JS_Send_Script(WS_REQUEST *req);
UINT16  JS_Alter_ID_Script(WS_REQUEST *req, UINT8 mode);
VOID    JS_Encrypt_Script(const UINT32 key, const CHAR HUGE *script, CHAR HUGE *buffer);

#define HTTP_Check_Timeout          JS_Check_Timeout
#define HTTP_Authenticated_Check    JS_Auth_Check
#endif

#ifdef __cplusplus
}
#endif /* _cplusplus */

#endif /* _DES_AUTH_H */

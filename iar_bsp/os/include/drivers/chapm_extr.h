/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       chapm_extr.h
*
*   COMPONENT
*
*       MSCHAP - Microsoft Challenge Handshake Authentication Protocol
*
*   DESCRIPTION
*
*       This file contains the function prototypes to support MSCHAP
*       (v1 & v2)
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef PPP_INC_MSCHP_EXTR_H
#define PPP_INC_MSCHP_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

/* Function Prototypes */
VOID     CHAPM_Passwd_Hash(INT8 *passwd, UINT8 *passwd_hash);
VOID     CHAPM_Challenge_Response(UINT8 *chal, UINT8 *passwd_hash,
                UINT8 *response);
VOID     CHAPM_Des_Encrypt(UINT8 *clear, UINT8 *key, UINT8 *cypher);
VOID     CHAPM_Random_Number(UINT8 *random_number, UINT8 bytes);


VOID     CHAPM_1_Chal_Resp(UINT8 *chal, INT8 *passwd, UINT8 *response);
STATUS   CHAPM_1_Chal_Resp_Verify(UINT8 *chal, INT8 *passwd,
                UINT8 *response);


STATUS   CHAPM_2_Check_Success(NET_BUFFER *in_buf_ptr);

STATUS   CHAPM_2_Chal_Resp_Verify(INT8 *passwd, INT8 *user_name,
                UINT8 *peer_chal, UINT8 *auth_chal, UINT8 *recv_response);

VOID     CHAPM_2_Generate_Peer_Response(UINT8 *auth_chal, UINT8 *peer_chal,
                INT8 *user_name, INT8 *passwd, UINT8 *response);

VOID     CHAPM_2_Challenge_Hash(UINT8 *peer_chal, UINT8 *auth_chal,
                                INT8 *user_name, UINT8 *chal);

VOID     CHAPM_2_Hash_Peer_Password_Hash(UINT8 *passwd_hash,
                UINT8 *passwd_hash_hash);

VOID     CHAPM_2_Gen_Auth_Resp(INT8 *passwd, UINT8 *peer_response,
                UINT8 *peer_chal, UINT8 *auth_chal, INT8 *user_name,
                UINT8 *auth_response);

STATUS   CHAPM_2_Check_Auth_Resp(INT8 *passwd, UINT8 *peer_response,
                UINT8 *peer_chal, UINT8 *auth_chal, INT8 *user_name,
                UINT8 *recv_response);

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_MSCHP_EXTR_H */

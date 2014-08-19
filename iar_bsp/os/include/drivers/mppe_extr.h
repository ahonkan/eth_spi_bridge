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
*       mppe_extr.h
*
*   COMPONENT
*
*       MPPE - Microsoft Point to Point Encryption Protocol
*
*   DESCRIPTION
*
*       This file contains function prototypes used by MPPE.
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
#ifndef PPP_INC_MPPE_EXTR_H
#define PPP_INC_MPPE_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

STATUS MPPE_Init(DV_DEVICE_ENTRY *dev_ptr);
STATUS MPPE_Decrypt(NET_BUFFER * buffer, DV_DEVICE_ENTRY *dev_ptr);
STATUS MPPE_Encrypt(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *dev_ptr);
VOID   MPPE_LM_Password_Hash(UINT8* Password, UINT8* PasswordHash);
STATUS MPPE_Get_Start_Key(UINT8* challenge, UINT8* nt_password_hash_hash,  
                          UINT8* initial_session_key);
STATUS MPPE_Get_Master_Key(UINT8* password_hash_hash, UINT8* nt_response, 
                           UINT8* master_key);
STATUS MPPE_Change_Key(DV_DEVICE_ENTRY *dev_ptr, UINT8 in_out_flag);
STATUS MPPE_Get_Asymmetric_Start_Key (UINT8* master_key, UINT8* session_key, 
                                      UINT8 session_key_length, UINT8 is_send, 
                                      UINT8 is_server);
STATUS MPPE_Get_Session_Key (DV_DEVICE_ENTRY *dev_ptr, UINT8 key_length);
STATUS MPPE_Get_Key(UINT8* initial_session_key, UINT8* current_session_key, 
                    UINT8 length_of_desired_key);


#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_MPPE_EXTR_H */

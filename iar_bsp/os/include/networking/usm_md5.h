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
*       usm_md5.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used in HMAC-MD5-96 Digest Authentication Protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/

#ifndef USM_MD5_H
#define USM_MD5_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

STATUS USM_Md5_Secure(UINT8 *key, UINT8 key_length, UINT8 *msg,
                      UINT32 msg_length, UINT8 *auth_param);

STATUS USM_Md5_Verify(UINT8 *key, UINT8 key_length, UINT8 *param,
                      UINT32 param_length, UINT8 *msg, UINT32 msg_length);

STATUS USM_Md5_Key(UINT8 *password, UINT8 password_length,
                   UINT8 *hashed_key);

STATUS USM_Md5_Key_Change(UINT8 *current_key, UINT8 *new_key,
                          UINT32 new_key_len);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* USM_MD5_H */

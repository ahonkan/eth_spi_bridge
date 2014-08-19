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
*       usm_des.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used CBC-DES Symmetric Encryption Protocol.
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

#ifndef USM_DES_H
#define USM_DES_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* __cplusplus */

STATUS USM_Des_Encrypt
 (UINT8 *key, UINT8 key_length, UINT8 **msg, UINT32 *msg_length,
                               UINT8 *param, UINT32 *param_length,
                               UINT16 buf_len);

STATUS USM_Des_Decrypt(UINT8 *key, UINT8 key_length, UINT8 *param,
                    UINT32 param_length, UINT8 **msg, UINT32 *msg_length);

 /* Functions used internally. */
 STATUS USM_Des_Encrypt_Block (UINT8 *input_block, UINT8 *key);
 STATUS USM_Des_Decrypt_Block(UINT8 *input_block, UINT8 *key);
 STATUS USM_Des_Calculate_Key(UINT8 *key_n, UINT8 iteration, UINT8 *key);
 STATUS USM_Des_Calculate_All_Keys(UINT8 *all_keys, UINT8 *key);
 STATUS USM_Des_Cipher_Function(UINT8 *output, UINT8 *input, UINT8 *key);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* USM_DES_H */


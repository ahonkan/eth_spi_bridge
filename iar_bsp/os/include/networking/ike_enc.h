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
*       ike_enc.h
*
* COMPONENT
*
*       IKE - Encryption
*
* DESCRIPTION
*
*       This file contains constants, data structures and function
*       prototypes used for implementing the Encryption module.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_ENC_H
#define IKE_ENC_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Encryption macros. ****/

/* Macro for synchronizing decryption and encryption
 * Initialization Vectors after an encrypted message
 * is sent or received.
 */
#define IKE_SYNC_IV(ivo, ivi, sa)       IKE_Sync_IV((ivo),(ivi),(sa))

/**** Function prototypes. ****/

VOID IKE_Sync_IV(UINT8 *iv_out, UINT8 *iv_in, IKE_SA *sa);
STATUS IKE_Encrypt(IKE_SA *sa, UINT8 *buffer, UINT16 buffer_len,
                   UINT16 *text_len, UINT8 *iv_in, UINT8 *iv_out,
                   UINT8 operation);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_ENC_H */

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
*       ike_auth.h
*
* COMPONENT
*
*       IKE - Authenticator
*
* DESCRIPTION
*
*       This file contains constants, data structures and function
*       prototypes for implementing the Authenticator module.
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
#ifndef IKE_AUTH_H
#define IKE_AUTH_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** IKE Authentication constants. ****/

/* Maximum length of HMAC key length, in bytes. */
#define IKE_MAX_PRF_KEY_LEN             64

/* Map IKE ID to OpenSSL NID. */
#if ((IKE_INCLUDE_MD5 == NU_TRUE) && (IKE_INCLUDE_SHA1 == NU_TRUE))
#define IKE_HASHID_TO_NID(hid)          (((hid) == IKE_MD5) ? \
                                         (NID_md5)      : \
                                         (NID_sha))
#elif (IKE_INCLUDE_MD5 == NU_TRUE)
#define IKE_HASHID_TO_NID(hid)          (NID_md5)
#else
#define IKE_HASHID_TO_NID(hid)          (NID_sha)
#endif

/**** Function prototypes. ****/

STATUS IKE_Sign(IKE_SA *sa, UINT8 *msg, UINT8 msg_len, UINT8 **sign,
                UINT *sign_len);
STATUS IKE_Sign_Verify(IKE_SA *sa, UINT8 *msg, UINT8 msg_len,
                       UINT8 *sign, UINT16 sign_len);
STATUS IKE_Generate_Cookie(UINT8 *cookie, struct addr_struct *addr);
STATUS IKE_Generate_Message_ID(UINT32 *msg_id);
STATUS IKE_MD5_Hash(UINT8 *msg, UINT16 msg_len, UINT8 *digest);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_AUTH_H */

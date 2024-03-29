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
*       ike2_keymat.h
*
* COMPONENT
*
*       IKEv2 - Key Generation
*
* DESCRIPTION
*
*       This file contains defines and function prototypes related
*       to generation of keying material for IKEv2 SAs and IPsec SAs
*       generated by IKEv2.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       None.
*
*************************************************************************/
#ifndef IKE2_KEYMAT_H
#define IKE2_KEYMAT_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/* RFC 4306, Section 2.13 specifies this value to be padded to the text
 * before calculating HMAC during derivation of keys.
 */
#define IKE2_KEYMAT_PAD_LEN             1
#define IKE2_KEYMAT_PAD_INIT_VAL        0x01
#define IKE2_KEYMAT_PAD_INCREMENT       0x01

STATUS IKE2_Generate_KEYMAT(IKE2_EXCHANGE_HANDLE *handle);
STATUS IKE2_Generate_CHILD_SA_KEYMAT(IKE2_EXCHANGE_HANDLE *handle);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE2_KEYMAT_H */

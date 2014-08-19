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
*       ike_oak.h
*
* COMPONENT
*
*       IKE - Oakley
*
* DESCRIPTION
*
*       This file contains macros and function prototypes
*       of utility functions used by different IKE Exchanges.
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
#ifndef IKE_OAK_H
#define IKE_OAK_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Oakley constants. ****/

/* Constants to specify HASH(1), HASH(2) or HASH(3) to
 * the IKE_Hash_x function.
 */
#define IKE_HASH_1                      1
#define IKE_HASH_2                      2
#define IKE_HASH_3                      3

/**** Function prototypes. ****/

STATUS IKE_SKEYID_Allocate(IKE_SA *sa);
STATUS IKE_SKEYID_Preshared_Key(IKE_SA *sa, UINT8 *nonce_i,
                                UINT16 nonce_i_len, UINT8 *nonce_r,
                                UINT16 nonce_r_len);
STATUS IKE_SKEYID_Signatures(IKE_SA *sa, UINT8 *gxy, UINT16 gxy_len,
                             UINT8 *nonce_i, UINT16 nonce_i_len,
                             UINT8 *nonce_r, UINT16 nonce_r_len);
STATUS IKE_SKEYID_dae(IKE_SA *sa, UINT8 *gxy, UINT16 gxy_len);
STATUS IKE_Hash_x(IKE_PHASE2_HANDLE *ph2, UINT8 *msg, UINT16 msg_len,
                  UINT8 *dgst, UINT8 *dgst_len, UINT8 hash_type);
STATUS IKE_Verify_Hash_x(IKE_PHASE2_HANDLE *ph2, UINT8 *msg,
                         UINT16 msg_len, UINT8 *dgst, UINT8 dgst_len);
STATUS IKE_Phase1_Encryption_Material(IKE_SA *sa, UINT8 *dhpub_i,
                                      UINT16 dhpub_i_len,
                                      UINT8 *dhpub_r,
                                      UINT16 dhpub_r_len);
STATUS IKE_Phase1_Key_Material(IKE_PHASE1_HANDLE *ph1);
STATUS IKE_KEYMAT_Allocate(IKE_SA2 *sa2);
STATUS IKE_KEYMAT_x(IKE_PHASE2_HANDLE *ph2, IKE_SA2 *sa2,
                    UINT8 *dh_gxy, UINT16 dh_gxy_len, UINT8 side);
STATUS IKE_Phase2_Key_Material(IKE_PHASE2_HANDLE *ph2);
STATUS IKE_Phase2_IV(IKE_PHASE2_HANDLE *ph2);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_OAK_H */

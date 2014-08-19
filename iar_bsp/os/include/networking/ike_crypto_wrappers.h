/*************************************************************************
*
*               Copyright 2011 Mentor Graphics Corporation
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
*       ike_crypto_wrappers.h
*
* COMPONENT
*
*       IKE
*
* DESCRIPTION
*
*       This file contains the definitions of Crypto wrapper functions.
*
* DATA STRUCTURES
*
*       None
*
*
* DEPENDENCIES
*
*       evp.h
*       hmac.h
*       rand.h
*       ike_api.h
*
*************************************************************************/

#ifndef IKE_CRYPTO_WRAPPERS_H
#define IKE_CRYPTO_WRAPPERS_H

#include "openssl/evp.h"
#include "openssl/hmac.h"
#include "openssl/rand.h"
#include "networking/ike_api.h"

#ifdef  __cplusplus
extern  "C" {                   /* C declarations in C++ */
#endif /* __cplusplus */

/**** Constants ****/

#define IKE_ENCRYPT 1
#define IKE_DECRYPT 0

/* IKE/OpenSSL algorithm types */

/* Encryption algorithms */
#define IKE_OPENSSL_DES         10
#define IKE_OPENSSL_3DES        11
#define IKE_OPENSSL_BF          12
#define IKE_OPENSSL_CAST        13
#define IKE_OPENSSL_AES         14

/* Hash/HMAC algorithms */
#define IKE_OPENSSL_MD5         20
#define IKE_OPENSSL_SHA1        21
#define IKE_OPENSSL_AES_XCBC    22
#define IKE_OPENSSL_DES_MAC     23

/* Signature algorithms */
#define IKE_OPENSSL_RSA         30
#define IKE_OPENSSL_RSA_DS      31

/**** Data Structures ****/

/**** Macro Functions ****/

/**** Function Prototypes ****/

STATUS IKE_Crypto_Digest_Len(UINT8 hash_algo, UINT8 *len);
STATUS IKE_Crypto_IV_Len(UINT8 algo, UINT8 *len);
STATUS IKE_Crypto_Enc_Key_Len(UINT8 enc_algo, UINT16 *len);

/* HMAC functions */
STATUS IKE_HMAC_Init(UINT8 type, VOID **ctx, UINT8 *key, INT len);
STATUS IKE_HMAC_Update(VOID *ctx, UINT8 *data, INT len);
STATUS IKE_HMAC_Final(VOID *ctx, UINT8 *md, UINT8* md_len);
STATUS IKE_HMAC(UINT8 type, UINT8 *key, INT key_len,
                UINT8 *data, INT data_len,
                UINT8 *md, UINT8* md_len);
UINT8  IKE_HMAC_Size(VOID* ctx);

/* Hash functions */
STATUS IKE_Hash_Init(UINT8 type, EVP_MD_CTX *md_ctx);
STATUS IKE_Hash_Update(EVP_MD_CTX *md_ctx, UINT8 *data, INT len);
STATUS IKE_Hash_Final(EVP_MD_CTX *md_ctx, UINT8 *md, UINT8* md_len);

/* D-H functions */
STATUS IKE_DH_Compute_Key(UINT8* p, INT p_len, INT g,
                          IKE_KEY_PAIR* key_pair,
                          UINT8* rpub, INT rpub_len,
                          UINT8** key, INT* key_len,
			  UINT8* g_ext, INT g_ext_len);

STATUS IKE_DH_Generate_Key(UINT8* p, INT p_len, INT g,
                           IKE_KEY_PAIR* key_pair, INT x_bits,
                           UINT8* g_ext, INT g_ext_len);

/* Digital Signatures */

STATUS IKE_Compute_Signature(UINT8 algo_type, IKE_ATTRIB* sa_attrib,
                             UINT8* md, INT md_len, INT md_type,
                             UINT8** sign, UINT* sign_len);

STATUS IKE_Verify_Signature(UINT8 algo_type, IKE_ATTRIB* sa_attrib,
                            UINT8* md, INT md_len, INT md_type,
                            UINT8* sign, INT sign_len);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_CRYPTO_WRAPPERS_H */

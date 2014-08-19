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
*       ips_enc.h
*
* COMPONENT
*
*       ENCRYPTOR
*
* DESCRIPTION
*
*       Definitions required by the Encryptor component. This component
*       interfaces with Nucleus Cipher Suite to provide encryption
*       services.
*
* DATA STRUCTURES
*
*       IPSEC_ENCRYPTION_ALGO
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IPS_ENC_H
#define IPS_ENC_H

#ifdef          __cplusplus
extern  "C" {                             /* C declarations in C++ */
#endif /* _cplusplus */

/*
 *  As per RFC 2451, following are the default sizes (in bytes) of the
 *  keys used by IPsec cipher algorithms.
 */
#define IPSEC_DES_KEY_LEN           8
#define IPSEC_AES_KEY_LEN           16
#define IPSEC_BLOWFISH_KEY_LEN      16
#define IPSEC_CAST_128_KEY_LEN      16
#define IPSEC_3DES_KEY_LEN          24

/* Defining the structure IPSEC_ENCRYPTION_ALGO. */
typedef struct ipsec_encryption_algo
{
    /* Return EVP_CIPHER structure for specfic cipher. */
    const void* (*evp_cipher)(void);

    /* Key length for this algorithm. */
    UINT16  ipsec_key_len;

    /* Identifier used to specify an encryption algorithm in IPsec. */
    UINT8   ipsec_algo_identifier;
} IPSEC_ENCRYPTION_ALGO;
/* End of IPSEC_ENCRYPTION_ALGO structure definition. */

/* Encryption/decryption request structure. */
typedef struct _ips_encrypt_req
{
    UINT8           *buffer;
    UINT8           *cipher_key;
    UINT8           *cipher_iv;
    UINT16          text_len;
    UINT8           operation;
    UINT8           ipsec_algo_index;
} IPSEC_ENCRYPT_REQ;

/* Extern the global IPsec encryption algorithms array. */
extern const IPSEC_ENCRYPTION_ALGO IPSEC_Encryption_Algos[];

/* Define IPsec CBC mode maximum block size in bytes. */
#define IPSEC_CBC_MAX_BLK_SIZE          16

/* Macro for getting IPsec encryption algorithm id from its index. */
#define IPSEC_GET_ENCRYPT_ID(ipsec_index) \
            IPSEC_Encryption_Algos[ipsec_index].ipsec_algo_identifier

/* Macro for getting particular encryption algorithm key length. */
#define IPSEC_GET_ENCRYPT_KEY_LEN(ipsec_index) \
                    IPSEC_Encryption_Algos[ipsec_index].ipsec_key_len

/*** Function Prototypes. ***/
/* This function is passed a NET buffer list as well as a
   request structure.  */
STATUS IPSEC_Cipher_Operation(NET_BUFFER *buf_ptr,
                  IPSEC_ENCRYPT_REQ *request, UINT8 *ret_iv, UINT8 iv_len);

/* Function used for getting IPsec encryption algorithm index from the
   IPsec global array of encryption algorithms. */
STATUS IPSEC_Get_Encrypt_Algo_Index(UINT8 *algo_id);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_ENC_H */

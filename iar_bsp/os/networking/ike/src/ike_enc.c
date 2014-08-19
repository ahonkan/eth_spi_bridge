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
*       ike_enc.c
*
* COMPONENT
*
*       IKE - Encryptor
*
* DESCRIPTION
*
*       This file implements the IKE Encryptor component. The
*       Encryptor is responsible for encrypting and decrypting
*       IKE messages.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Sync_IV
*       IKE_Encrypt
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*       ike_enc.h
*       ike_crypto_wrappers.h
*       des.h
*       blowfish.h
*       cast.h
*       aes.h
*       evp.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"
#include "networking/ike_enc.h"
#include "networking/ike_crypto_wrappers.h"
#include "openssl/des.h"
#include "openssl/blowfish.h"
#include "openssl/cast.h"
#include "openssl/aes.h"
#include "openssl/evp.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_Sync_IV
*
* DESCRIPTION
*
*       This function synchronizes an Initialization Vector (IV)
*       with the specified vector. Length of the IV is determined
*       from the negotiated encryption algorithm in the IKE SA.
*
* INPUTS
*
*       *iv_out                 On return, contains updated IV.
*       *iv_in                  IV used for synchronization.
*       *sa                     Pointer to the IKE SA.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Sync_IV(UINT8 *iv_out, UINT8 *iv_in, IKE_SA *sa)
{
    UINT8       iv_len;
    UINT8       ike_algo_id;

    ike_algo_id = IKE_Encryption_Algos[sa->ike_attributes.ike_encryption_algo]
                                       .crypto_algo_id;

    /* Select length of IV based on algorithm type. */
    IKE_Crypto_IV_Len(ike_algo_id, &iv_len);

    /* Update the IV. */
    if (iv_len > 0)
        NU_BLOCK_COPY(iv_out, iv_in, iv_len);
    else
        NLOG_Error_Log("Failed to find encryption block length",
                       NERR_SEVERE, __FILE__, __LINE__);

} /* IKE_Sync_IV */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Encrypt
*
* DESCRIPTION
*
*       This function encrypts or decrypts a buffer using the
*       algorithms and other encryption parameters negotiated
*       in the Phase 1 exchange.
*
* INPUTS
*
*       *sa                     Pointer to the SA of this exchange.
*       *buffer                 Pointer to the buffer which is to
*                               be encrypted or decrypted.
*       buffer_len              Total length of buffer in bytes.
*       *text_len               Pointer to length of text in the
*                               buffer, in bytes. Note that
*                               buffer_len might need to be more
*                               than this, when encrypting, due
*                               to padding constraints. This might
*                               be NULL if buffer and text lengths
*                               are the same. On return, it contains
*                               the length of resulting text.
*       *iv_in                  Input Initialization Vector. This is
*                               returned unmodified.
*       *iv_out                 Output Initialization Vector. On
*                               return, contains the last CBC
*                               block of the cipher operation.
*       operation               IKE_ENCRYPT / IKE_DECRYPT.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Encrypt(IKE_SA *sa, UINT8 *buffer, UINT16 buffer_len,
                   UINT16 *text_len, UINT8 *iv_in, UINT8 *iv_out,
                   UINT8 operation)
{
    STATUS              status;
    EVP_CIPHER_CTX      ctx;
    const EVP_CIPHER    *cipher = NU_NULL;
    const IKE_ENCRYPTION_ALGO *algo;
    INT                 ret;
    INT                 out_len;
    INT                 in_len;
    INT                 block_mod;
    INT                 total_len = 0;
    UINT8               block;
    UINT8               zero_pad[IKE_MAX_ENCRYPT_BLOCK_LEN];
    UINT8               last_cipher_block[IKE_MAX_ENCRYPT_BLOCK_LEN];

    /* Set padding bytes to all zeros. */
    memset(zero_pad, 0, sizeof(zero_pad));

    /* Set last cipher block to all zeros. */
    memset(last_cipher_block, 0, sizeof(last_cipher_block));

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the pointers are not NULL. */
    if((sa == NU_NULL) || (buffer == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure buffer length is valid. */
    if(buffer_len == 0)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* If text length is specified. */
    if(text_len != NU_NULL)
    {
        /* Make sure text length is valid. */
        if(*text_len == 0)
        {
            return (IKE_INVALID_PARAMS);
        }
    }

    /* Make sure valid Key and IVs are specified. */
    if((sa->ike_encryption_key == NU_NULL) || (iv_in == NU_NULL) ||
       (iv_out == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure operation is valid. */
    if((operation != IKE_ENCRYPT) && (operation != IKE_DECRYPT))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log informational message according to operation. */
    if(operation)
    {
        IKE_DEBUG_LOG("Encrypting IKE message");
    }

    else
    {
        IKE_DEBUG_LOG("Decrypting IKE message");
    }

    /* Set pointer to the encryption algorithm. */
    algo = &IKE_Encryption_Algos[sa->ike_attributes.ike_encryption_algo];

    /* Obtain the block length of this algorithm. */
    if (IKE_Crypto_IV_Len(algo->crypto_algo_id, &block) != NU_SUCCESS)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Initialize ctx structure */
    EVP_CIPHER_CTX_init(&ctx);

    switch(algo->crypto_algo_id)
    {
        case IKE_OPENSSL_DES:
            cipher = EVP_des_cbc();
            break;
        case IKE_OPENSSL_3DES:
            cipher = EVP_des_ede3_cbc();
            break;
        case IKE_OPENSSL_BF:
            cipher = EVP_bf_cbc();
            break;
        case IKE_OPENSSL_CAST:
            cipher = EVP_cast5_cbc();
            break;
        case IKE_OPENSSL_AES:
            if (sa->ike_attributes.ike_key_len == 16)
                cipher = EVP_aes_128_cbc();
            else if (sa->ike_attributes.ike_key_len == 24)
                cipher = EVP_aes_192_cbc();
            else if (sa->ike_attributes.ike_key_len == 32)
                cipher = EVP_aes_256_cbc();
            break;
    }

    if (cipher != NU_NULL)
    {
        /* Set cipher type only. */
        ret = EVP_CipherInit_ex(&ctx, cipher, NU_NULL, NU_NULL, NU_NULL, operation);
        if (!ret)
        {
            EVP_CIPHER_CTX_cleanup(&ctx);
            NLOG_Error_Log("Failed to encrypt data using OpenSSL",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
            return (IKE_CRYPTO_ERROR);
        }

        /* Now, specify the key length. */
        ret = EVP_CIPHER_CTX_set_key_length(&ctx, sa->ike_attributes.ike_key_len);
        if (!ret)
        {
            EVP_CIPHER_CTX_cleanup(&ctx);
            NLOG_Error_Log("Failed to encrypt data using OpenSSL",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
            return (IKE_CRYPTO_ERROR);
        }

        /* Set key and IV. */
        ret = EVP_CipherInit_ex(&ctx, NU_NULL, NU_NULL, sa->ike_encryption_key, iv_in, operation);
        if (!ret)
        {
            EVP_CIPHER_CTX_cleanup(&ctx);
            NLOG_Error_Log("Failed to encrypt data using OpenSSL",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
            return (IKE_CRYPTO_ERROR);
        }

        /* Disable cipher padding. */
        EVP_CIPHER_CTX_set_padding(&ctx, 0);

        in_len = (text_len == NU_NULL) ? buffer_len : (*text_len);

        /* Store the last cipher-text block for updating the IV. */
        if ((operation == IKE_DECRYPT) && (in_len >= block))
            NU_BLOCK_COPY(last_cipher_block, buffer + in_len - block, block);

        ret = EVP_CipherUpdate(&ctx, buffer, &out_len, buffer, in_len);
        if (ret)
        {
            total_len += out_len;
        }
        else
        {
            EVP_CIPHER_CTX_cleanup(&ctx);
            NLOG_Error_Log("Failed to encrypt data using OpenSSL",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
            return (IKE_CRYPTO_ERROR);
        }

        block_mod = in_len % block;

        /* If explicit padding is required. */
        if ((operation == IKE_ENCRYPT) && (block_mod != 0))
        {
            ret = EVP_CipherUpdate(&ctx, buffer + out_len, &out_len,
                                   zero_pad, block - block_mod);
            if (ret)
            {
                total_len += out_len;
            }
            else
            {
                EVP_CIPHER_CTX_cleanup(&ctx);
                NLOG_Error_Log("Failed to encrypt data using OpenSSL",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                return (IKE_CRYPTO_ERROR);
            }
        }

        ret = EVP_CipherFinal_ex(&ctx, buffer, &out_len);
        if (ret)
        {
            total_len += out_len;

            /* Update text_len. */
            if (text_len != NU_NULL)
                *text_len = (UINT16)total_len;

            /* Update the IV if operation is encryption. */
            if (total_len >= block)
            {
                if (operation == IKE_ENCRYPT)
                    NU_BLOCK_COPY(iv_out, buffer + total_len - block,
                                  block);
                else
                    NU_BLOCK_COPY(iv_out, last_cipher_block, block);
            }

            status = NU_SUCCESS;
        }
        else
        {
            NLOG_Error_Log("Failed to encrypt data using OpenSSL",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
            status = IKE_CRYPTO_ERROR;
        }

        EVP_CIPHER_CTX_cleanup(&ctx);
    }

    else
    {
        status = IKE_CRYPTO_ERROR;
        NLOG_Error_Log("Failed to find encryption algorithm",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Encrypt */

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
*       ike_crypto_wrappers.c
*
* COMPONENT
*
*       IKE
*
* DESCRIPTION
*
*       This file contains the implementation of Crypto wrapper functions.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Crypto_Digest_Len
*       IKE_Crypto_IV_Len
*       IKE_Crypto_Enc_Key_Len
*       IKE_HMAC_Init
*       IKE_HMAC_Update
*       IKE_HMAC_Final
*       IKE_HMAC
*       IKE_Hash_Init
*       IKE_Hash_Update
*       IKE_Hash_Final
*       IKE_DH_Compute_Key
*       IKE_DH_Generate_Key
*       IKE_Compute_Signature
*       IKE_Verify_Signature
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_crypto_wrappers.h
*       ike_api.h
*       aes.h
*       blowfish.h
*       cast.h
*       des.h
*       dh.h
*       hmac.h
*       md5.h
*       rsa.h
*       sha.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_crypto_wrappers.h"
#include "networking/ike_api.h"
#include "openssl/aes.h"
#include "openssl/blowfish.h"
#include "openssl/cast.h"
#include "openssl/des.h"
#include "openssl/dh.h"
#include "openssl/hmac.h"
#include "openssl/md5.h"
#include "openssl/rsa.h"
#include "openssl/sha.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_Crypto_Digest_Len
*
* DESCRIPTION
*
*       This function retrieves the digest length for the specified
*       hash algorithm.
*
* INPUTS
*
*       hash_algo               IKE-specific Hash algorithm type.
*       len                     Digest length return parameter.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If digest length could not be found.
*
*************************************************************************/
STATUS IKE_Crypto_Digest_Len(UINT8 hash_algo, UINT8 *len)
{
    switch (hash_algo)
    {
        case IKE_OPENSSL_MD5:       *len = MD5_DIGEST_LENGTH; break;
        case IKE_OPENSSL_SHA1:      *len = SHA_DIGEST_LENGTH; break;
        case IKE_OPENSSL_AES_XCBC:  *len = 12; break;
        default:                    *len = 0; break;
    }

    if (*len == 0)
    {
        return IKE_CRYPTO_ERROR;
    }

    return NU_SUCCESS;

} /* IKE_Crypto_Digest_Len */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Crypto_IV_Len
*
* DESCRIPTION
*
*       This function retrieves the IV length for the specified
*       crypto algorithm.
*
* INPUTS
*
*       algo                    IKE-specific Hash algorithm type.
*       len                     IV length return parameter.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If IV length could not be determined.
*
*************************************************************************/
STATUS IKE_Crypto_IV_Len(UINT8 algo, UINT8 *len)
{
    switch(algo)
    {
        case IKE_OPENSSL_DES:  *len = DES_KEY_SZ;       break;
        case IKE_OPENSSL_3DES: *len = DES_KEY_SZ;       break;
        case IKE_OPENSSL_BF:   *len = BF_BLOCK;         break;
        case IKE_OPENSSL_CAST: *len = CAST_BLOCK;       break;
        case IKE_OPENSSL_AES:  *len = AES_BLOCK_SIZE;   break;
        default :              *len = 0;                break;
    }

    if (*len == 0)
    {
        return IKE_CRYPTO_ERROR;
    }

    return NU_SUCCESS;

} /* IKE_Crypto_IV_Len */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Crypto_Enc_Key_Len
*
* DESCRIPTION
*
*       This function retrieves the encryption key length for the specified
*       crypto algorithm. If the length has been mentioned in the parameter,
*       then it is verified to be supported.
*
* INPUTS
*
*       enc_algo                IKE-specific Hash algorithm type.
*       len                     Key length, in bytes, requested for
*                               return or verification.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If key length could not be determined.
*       IKE_INVALID_KEYLEN      If specified key length is incorrect.
*
*************************************************************************/
STATUS IKE_Crypto_Enc_Key_Len(UINT8 enc_algo, UINT16 *len)
{
    STATUS  status;
    UINT8   key_len;

    if (*len == 0)
    {
        /* key length has been requested */

        switch(enc_algo)
        {
            case IKE_OPENSSL_DES:   key_len = DES_KEY_SZ;       break;
            case IKE_OPENSSL_3DES:  key_len = DES_KEY_SZ*3;     break;
            case IKE_OPENSSL_BF:    key_len = 16;               break;
            case IKE_OPENSSL_CAST:  key_len = CAST_KEY_LENGTH;  break;
            case IKE_OPENSSL_AES:   key_len = 16;               break;
            default:                key_len = 0;                break;
        }

        *len = key_len;
        status = key_len ? NU_SUCCESS : IKE_CRYPTO_ERROR;
    }
    else
    {
        /* key length needs to be verified. */

        status = IKE_INVALID_KEYLEN;

        switch(enc_algo)
        {
            case IKE_OPENSSL_DES:
                if (*len == DES_KEY_SZ) status = NU_SUCCESS;
                break;

            case IKE_OPENSSL_3DES:
               if (*len == (DES_KEY_SZ*3)) status = NU_SUCCESS;
               break;

            case IKE_OPENSSL_BF:
                if ((*len >= 4) && (*len <= 56)) status = NU_SUCCESS;
                break;

            case IKE_OPENSSL_CAST:
                if (*len == CAST_KEY_LENGTH) status = NU_SUCCESS;
                break;

            case IKE_OPENSSL_AES:
                if ((*len == 16) || (*len == 24) || (*len == 32))
                    status = NU_SUCCESS;
                break;
        }
    }

    return (status);

} /* IKE_Crypto_Enc_Key_Len */

/*************************************************************************
*
* FUNCTION
*
*       IKE_HMAC_Init
*
* DESCRIPTION
*
*       Wrapper function for HMAC initializers.
*
* INPUTS
*
*       algo                    HMAC algorithm type.
*       ctx                     Context structure.
*       key                     Key.
*       len                     Key length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If initialization fails.
*
*************************************************************************/
STATUS IKE_HMAC_Init(UINT8 algo, VOID **ctx, UINT8 *key, INT len)
{
    STATUS              status;
    const EVP_MD*       md = NU_NULL;
    HMAC_CTX*           hmac_ctx;
    IPS_CIPHER_MAC_CTX* xcbc_ctx;
    INT*               ptr;
    
    if ( (algo != IKE_OPENSSL_MD5) && (algo != IKE_OPENSSL_SHA1) &&
        (algo != IKE_OPENSSL_AES_XCBC) )
    {
        return IKE_CRYPTO_ERROR;
    }
    
    if (algo == IKE_OPENSSL_AES_XCBC)
    {
        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&ptr,
                                    sizeof(INT) + sizeof(IPS_CIPHER_MAC_CTX), NU_NO_SUSPEND);
    }
    else
    {
        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&ptr,
                                    sizeof(INT) + sizeof(HMAC_CTX), NU_NO_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        *ctx = (VOID*)ptr;
        *ptr++ = algo;
        
        if (algo == IKE_OPENSSL_AES_XCBC)
        {
            xcbc_ctx = (IPS_CIPHER_MAC_CTX*) ptr;
            status = IPSEC_XCBC_MAC_Init(xcbc_ctx, key, len);
        }
        else
        {
            hmac_ctx = (HMAC_CTX*) ptr;
            HMAC_CTX_init(hmac_ctx);

            switch(algo)
            {
                case IKE_OPENSSL_MD5:   md = EVP_md5(); break;
                case IKE_OPENSSL_SHA1:  md = EVP_sha1(); break;
            }
        
            if (HMAC_Init_ex(hmac_ctx, key, len, md, NU_NULL))
            {
                status = NU_SUCCESS;
            }
            else
            {
                status = IKE_CRYPTO_ERROR;
            }
        }
    
        if (status != NU_SUCCESS) NU_Deallocate_Memory(*ctx);
    }

    return (status);

} /* IKE_HMAC_Init */

/*************************************************************************
*
* FUNCTION
*
*       IKE_HMAC_Update
*
* DESCRIPTION
*
*       Wrapper function for HMAC Update functions.
*
* INPUTS
*
*       ctx                     Context structure.
*       data                    Data.
*       len                     Data length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If update fails.
*
*************************************************************************/
STATUS IKE_HMAC_Update(VOID *ctx, UINT8 *data, INT len)
{
    INT                 algo;
    STATUS              status;
    HMAC_CTX*           hmac_ctx;
    IPS_CIPHER_MAC_CTX* xcbc_ctx;

    algo = *((INT*)ctx);

    if (algo == IKE_OPENSSL_AES_XCBC)
    {
        UINT8* data_copy;
        xcbc_ctx = (IPS_CIPHER_MAC_CTX*)((INT*)ctx + 1);
        /* Preserve original data in case of AES-XCBC */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&data_copy, len, NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            NU_BLOCK_COPY(data_copy, data, len);
            status = IPSEC_XCBC_MAC_Update(xcbc_ctx, data_copy, len);
            NU_Deallocate_Memory(data_copy);
        }
    }
    else
    {
        hmac_ctx = (HMAC_CTX*)((INT*)ctx + 1);
        status = HMAC_Update(hmac_ctx, data, len) ? NU_SUCCESS : IKE_CRYPTO_ERROR;
    }

    if (status != NU_SUCCESS) NU_Deallocate_Memory(ctx);

    return status;

} /* IKE_HMAC_Update */

/*************************************************************************
*
* FUNCTION
*
*       IKE_HMAC_Final
*
* DESCRIPTION
*
*       Wrapper function for HMAC Final functions. It also truncates digest
*       if requested.
*
* INPUTS
*
*       ctx                     Context structure.
*       md                      Message digest.
*       md_len                  MD length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If finalize fails.
*
*************************************************************************/
STATUS IKE_HMAC_Final(VOID *ctx, UINT8 *md, UINT8* md_len)
{
    STATUS              status;
    UINT8               digest[EVP_MAX_MD_SIZE];
    UINT                digest_len;
    HMAC_CTX*           hmac_ctx;
    IPS_CIPHER_MAC_CTX* xcbc_ctx;
    INT                 algo;

    algo = *((INT*)ctx);

    if (algo == IKE_OPENSSL_AES_XCBC)
    {
        xcbc_ctx = (IPS_CIPHER_MAC_CTX*)((INT*)ctx + 1);
        status = IPSEC_XCBC_MAC_Final(xcbc_ctx, digest, &digest_len);
    }
    else
    {
        hmac_ctx = (HMAC_CTX*)((INT*)ctx + 1);
        status = HMAC_Final(hmac_ctx, digest, &digest_len) ? NU_SUCCESS : IKE_CRYPTO_ERROR;
        HMAC_CTX_cleanup(hmac_ctx);
    }

    if (status == NU_SUCCESS)
    {
        if (*md_len == 0) *md_len = digest_len;
        NU_Block_Copy(md, digest, *md_len);
    }

    NU_Deallocate_Memory(ctx);

    return (status);

} /* IKE_HMAC_Final */

/*************************************************************************
*
* FUNCTION
*
*       IKE_HMAC
*
* DESCRIPTION
*
*       Wrapper function for HMAC. It also truncates digest if requested.
*
* INPUTS
*
*       algo                    HMAC algorithm type.
*       ctx                     Context structure.
*       key                     Key.
*       key_len                 Key length.
*       data                    Data.
*       data_len                Data length.
*       md                      Message digest.
*       md_len                  MD length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If finalize fails.
*
*************************************************************************/
STATUS IKE_HMAC(UINT8 algo,
                UINT8 *key, INT key_len,
                UINT8 *data, INT data_len,
                UINT8 *md, UINT8* md_len)
{
    STATUS status;
    VOID *ctx;

    status = IKE_HMAC_Init(algo, &ctx, key, key_len);

    if (status == NU_SUCCESS)
    {
        status = IKE_HMAC_Update(ctx, data, data_len);

        if (status == NU_SUCCESS)
        {
            status = IKE_HMAC_Final(ctx, md, md_len);
        }
    }

    return (status);
} /* IKE_HMAC */

/*************************************************************************
*
* FUNCTION
*
*       IKE_HMAC_Size
*
* DESCRIPTION
*
*       Wrapper function for HMAC Size functions.
*
* INPUTS
*
*       ctx                     Context structure.
*
* OUTPUTS
*
*       Size of digest.
*
*************************************************************************/
UINT8 IKE_HMAC_Size(VOID* ctx)
{
    UINT8               size = 0;
    HMAC_CTX*           hmac_ctx;
    INT                 algo;

    algo = *((INT*)ctx);

    if (algo == IKE_OPENSSL_AES_XCBC)
    {
        size = IPS_XCBC_DIGEST_LEN;
    }
    else
    {
        hmac_ctx = (HMAC_CTX*)((INT*)ctx + 1);
        size = HMAC_size(hmac_ctx);
    }

    return (size);

} /* IKE_HMAC_Size */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Hash_Init
*
* DESCRIPTION
*
*       Wrapper function for Hash initializers.
*
* INPUTS
*
*       algo                    Hash algorithm type.
*       md_ctx                  Hash Context structure.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If initialization fails.
*
*************************************************************************/
STATUS IKE_Hash_Init(UINT8 algo, EVP_MD_CTX *md_ctx)
{
    STATUS          status;
    const EVP_MD*   md = NU_NULL;

    EVP_MD_CTX_init(md_ctx);

    switch(algo)
    {
        case IKE_OPENSSL_MD5:   md = EVP_md5(); break;
        case IKE_OPENSSL_SHA1:  md = EVP_sha1(); break;
    }

    if (md != NU_NULL)
    {
        if (EVP_DigestInit_ex(md_ctx, md, NU_NULL))
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = IKE_CRYPTO_ERROR;
        }
    }
    else
    {
        status = IKE_CRYPTO_ERROR;
    }

    return (status);

} /* IKE_Hash_Init */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Hash_Update
*
* DESCRIPTION
*
*       Wrapper function for EVP_DigestUpdate().
*
* INPUTS
*
*       md_ctx                  Hash Context structure.
*       data                    Data.
*       len                     Data length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If update fails.
*
*************************************************************************/
STATUS IKE_Hash_Update(EVP_MD_CTX *md_ctx, UINT8 *data, INT len)
{
    return EVP_DigestUpdate(md_ctx, data, len) ? NU_SUCCESS : IKE_CRYPTO_ERROR;

} /* IKE_Hash_Update */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Hash_Final
*
* DESCRIPTION
*
*       Wrapper function for EVP_DigestFinal_ex(). It also truncates digest
*       if requested.
*
* INPUTS
*
*       md_ctx                  Hash Context structure.
*       md                      Message digest.
*       md_len                  MD length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If finalize fails.
*
*************************************************************************/
STATUS IKE_Hash_Final(EVP_MD_CTX *md_ctx, UINT8 *md, UINT8* md_len)
{
    STATUS  status;
    UINT8   digest[EVP_MAX_MD_SIZE];
    UINT    digest_len;

    if (EVP_DigestFinal_ex(md_ctx, digest, &digest_len))
    {
        status = NU_SUCCESS;

        if (*md_len == 0) *md_len = EVP_MD_CTX_size(md_ctx);
        NU_Block_Copy(md, digest, *md_len);
    }
    else
    {
        status = IKE_CRYPTO_ERROR;
    }

    EVP_MD_CTX_cleanup(md_ctx);

    return (status);

} /* IKE_Hash_Final */

/*************************************************************************
*
* FUNCTION
*
*       IKE_DH_Compute_Key
*
* DESCRIPTION
*
*       This function computes DH key.
*
* INPUTS
*
*       p                       "p".
*       p_len                   Length of "p".
*       g                       "g".
*       key_pair                Key pair structure.
*       rpub                    Remote public key.
*       rpub_len                Remote public key length.
*       key                     Out parameter for allocated key.
*       key_len                 Returned key length.
*       g_ext                   Extended Generator if it's a big number.
*       g_ext_len               Length of extended Generator.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If operation fails.
*
*************************************************************************/
STATUS IKE_DH_Compute_Key(UINT8* p, INT p_len, INT g,
                          IKE_KEY_PAIR* key_pair,
                          UINT8* rpub, INT rpub_len,
                          UINT8** key, INT* key_len,
                          UINT8* g_ext, INT g_ext_len)
{
    STATUS      status = IKE_CRYPTO_ERROR;
    BIGNUM      *bn_rpub;
    DH          *dh;
    INT         len;

    /* Initialize out parameters */
    *key = NU_NULL;
    *key_len = 0;

    /* Convert public key to big-number */
    bn_rpub = BN_bin2bn(rpub, rpub_len, NU_NULL);

    if (bn_rpub != NU_NULL)
    {
        dh = DH_new();

        if(dh != NU_NULL)
        {
            /* Convert the 'p' parameter to a Big Number. */
            dh->p = BN_bin2bn(p, p_len, NU_NULL);

            if(dh->p != NU_NULL)
            {
                /* Convert public key to a Big Number. */
                dh->pub_key = BN_bin2bn(key_pair->ike_public_key,
                                        key_pair->ike_public_key_len, NU_NULL);

                if(dh->pub_key != NU_NULL)
                {
                    /* Convert private key to a Big Number. */
                    dh->priv_key = BN_bin2bn(key_pair->ike_private_key,
                                             key_pair->ike_private_key_len, NU_NULL);

                    if(dh->priv_key != NU_NULL)
                    {
                        /* Set length of remote public key. */
                        dh->length = rpub_len * 8;

                        /* Initialize 'g' to NULL. */
                        dh->g = NU_NULL;

                        if(g != IKE_DH_GENERATOR_POS)
                        {
                            /* Create a new Big Number for 'g'. */
                            dh->g = BN_new();

                            if(dh->g != NU_NULL)
                            {
                                /* Set the 'g' parameter. */
                                if(!BN_set_word(dh->g, (BN_ULONG)g))
                                {
                                    BN_free(dh->g);
                                    dh->g = NU_NULL;
                                }
                            }
                        }
                        else
                        {
                            /* Calculate the Big Number for 'g'.. */
                            dh->g = BN_bin2bn(g_ext, g_ext_len, NU_NULL);
                        }

                        if(dh->g != NU_NULL)
                        {
                            /* Set length of shared key. */
                            *key_len =  DH_size(dh);

                            /* Allocate memory for shared key. */
                            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                         (VOID**)key, *key_len, NU_NO_SUSPEND);

                            if(status == NU_SUCCESS)
                            {
                                /* Compute the shared key. */
                                len = DH_compute_key(*key, bn_rpub, dh);
                                if(len < 0)
                                {
                                    /* Report error on failure. */
                                    status = IKE_CRYPTO_ERROR;
                                    NU_Deallocate_Memory(*key);
                                    *key = NU_NULL;
                                }
                                else
                                {
                                    /* Update shared key length. */
                                    *key_len = len;
                                }
                            }
                        }
                    }
                }
            }

            /* Free the DH structure. */
            DH_free(dh);
        }

        /* Free the DH public number. */
        BN_free(bn_rpub);
    }

    return (status);

} /* IKE_DH_Compute_Key */

/*************************************************************************
*
* FUNCTION
*
*       IKE_DH_Generate_Key
*
* DESCRIPTION
*
*       This function generates DH key.
*
* INPUTS
*
*       p                       "p".
*       p_len                   Length of "p".
*       g                       "g".
*       key_pair                Key pair structure.
*       x_bits                  "X" bits.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        If operation fails.
*
*************************************************************************/
STATUS IKE_DH_Generate_Key(UINT8* p, INT p_len, INT g,
                          IKE_KEY_PAIR* key_pair, INT x_bits,
                          UINT8* g_ext, INT g_ext_len)
{
    BIGNUM      *bn_p;
    DH          *dh;
    STATUS      status = IKE_CRYPTO_ERROR;
    INT         pub_len;
    INT         priv_len;
    UINT8       *key_pair_buffer;


    bn_p = BN_bin2bn(p, p_len, NU_NULL);

    if(bn_p != NU_NULL)
    {
        dh = DH_new();

        if(dh != NU_NULL)
        {
            /* Set 'p' as part of dh struct. */
            dh->p = bn_p;
            bn_p = NU_NULL;

            /* Initialize 'g' to NULL. */
            dh->g = NU_NULL;

            if(g != IKE_DH_GENERATOR_POS)
            {
                /* Create a new Big Number for 'g'. */
                dh->g = BN_new();

                if(dh->g != NU_NULL)
                {
                    /* Set the 'g' parameter. */
                    if(!BN_set_word(dh->g, (BN_ULONG)g))
                    {
                        BN_free(dh->g);
                        dh->g = NU_NULL;
                    }
                }
            }
            else
            {
                /* Calculate the Big Number for 'g'.. */
                dh->g = BN_bin2bn(g_ext, g_ext_len, NU_NULL);
            }

            if(dh->g != NU_NULL)
            {
                /* If number of bits in 'x' not zero. */
                if(x_bits != 0)
                {
                    /* Set the requested length. */
                    dh->length = x_bits;
                }

                /* Generate public and private number. */
                if(DH_generate_key(dh))
                {
                    /* Set public and private key lengths. */
                    pub_len = dh->pub_key->top * BN_BYTES;
                    priv_len = dh->priv_key->top * BN_BYTES;

                    /* Allocate memory for public and private keys in
                       a single block. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                              (VOID**)(&key_pair_buffer),
                              pub_len + priv_len, NU_NO_SUSPEND);
                    if(status == NU_SUCCESS)
                    {
                        /* Point the public and private keys to
                           the dynamically allocated buffer. */
                        key_pair->ike_public_key = key_pair_buffer;
                        key_pair->ike_private_key = key_pair_buffer + pub_len;

                        /* Copy public key to key pair struct. */
                        key_pair->ike_public_key_len = BN_bn2bin(dh->pub_key,
                                                                 key_pair->ike_public_key);

                        /* Copy private key to key pair struct. */
                        key_pair->ike_private_key_len = BN_bn2bin(dh->priv_key,
                                                                  key_pair->ike_private_key);
                    }
                }
            }

            /* Free the dh structure. */
            DH_free(dh);
        }

        /* Free the BN 'p'. */
        BN_free(bn_p);
    }

    /* Return the status. */
    return (status);

} /* IKE_DH_Generate_Key */

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       IKE_Compute_Signature
*
* DESCRIPTION
*
*       This function computes RSA signature.
*
* INPUTS
*
*       algo_type               RSA algorithm type.
*       sa_attrib               IKE SA attributes.
*       md                      Message digest to be signed.
*       md_len                  Length of message digest to be signed.
*       md_type                 Type of message digest to be signed.
*       sign                    Returned signature.
*       sign_len                Returned signature length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CERT_ERROR          If signing fails.
*       IKE_CERT_FILE_ERROR     If certificate file operations fail.
*
*************************************************************************/
STATUS IKE_Compute_Signature(UINT8 algo_type, IKE_ATTRIB* sa_attrib,
                             UINT8* md, INT md_len, INT md_type,
                             UINT8** sign, UINT* sign_len)
{
    STATUS      status = IKE_CERT_FILE_ERROR;
    RSA         *rsa = NU_NULL;
    BIO         *bio;
    EVP_PKEY    *key = NU_NULL;

    /* TODO: how to differentiate between RSA and RSA-no-encode */
    switch (algo_type)
    {
        case IKE_OPENSSL_RSA:
            break;
        case IKE_OPENSSL_RSA_DS:
            break;
        default:
            break;
    }

    /* Read the key from the certificate file. */

    bio = BIO_new(BIO_s_file());

    if(bio != NU_NULL)
    {
        if(BIO_read_filename(bio, sa_attrib->ike_local_key_file) > 0)
        {
            if (sa_attrib->ike_cert_encoding == IKE_X509_FILETYPE_ASN1)
            {
                key = d2i_PrivateKey_bio(bio, NU_NULL);
            }
            else if (sa_attrib->ike_cert_encoding == IKE_X509_FILETYPE_PEM)
            {
                key = PEM_read_bio_PrivateKey(bio, NU_NULL,
                                              sa_attrib->ike_pem_callback, NU_NULL);
            }

            if(key != NU_NULL) status = NU_SUCCESS;
        }

        BIO_free(bio);
    }

    /* Get reference to RSA structure using private key. */

    if (status == NU_SUCCESS)
    {
        rsa = EVP_PKEY_get1_RSA(key);
        EVP_PKEY_free(key);
        if (rsa == NU_NULL) status = IKE_CERT_ERROR;
    }

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the signature. */

        *sign_len = RSA_size(rsa);

        if (*sign_len > 0)
        {
            status =  NU_Allocate_Memory(IKE_Data.ike_memory,
                                         (VOID**)sign,
                                         *sign_len, NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                if (!RSA_sign(md_type, md, md_len, *sign, sign_len, rsa))
                {
                    status = IKE_CERT_ERROR;
                    NU_Deallocate_Memory(*sign);
                }
            }
        }
        else
        {
            status = IKE_CERT_ERROR;
        }

        RSA_free(rsa);
    }

    return (status);

} /* IKE_Compute_Signature */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Verify_Signature
*
* DESCRIPTION
*
*       This function computes RSA signature.
*
* INPUTS
*
*       algo_type               RSA algorithm type.
*       sa_attrib               IKE SA attributes.
*       md                      Message digest to be signed.
*       md_len                  Length of message digest to be signed.
*       md_type                 Type of message digest to be signed.
*       sign                    Signature.
*       sign_len                Signature length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CERT_ERROR          If verification fails.
*
*************************************************************************/
STATUS IKE_Verify_Signature(UINT8 algo_type, IKE_ATTRIB* sa_attrib,
                            UINT8* md, INT md_len, INT md_type,
                            UINT8* sign, INT sign_len)
{
    STATUS      status = IKE_CERT_ERROR;
    RSA         *rsa = NU_NULL;

    /* TODO: how to differentiate between RSA and RSA-no-encode */
    switch (algo_type)
    {
        case IKE_OPENSSL_RSA:
            break;
        case IKE_OPENSSL_RSA_DS:
            break;
        default:
            break;
    }

    /* Extract RSA key from remote key data. */

    rsa = d2i_RSAPublicKey(NU_NULL, (const unsigned char**)&(sa_attrib->ike_remote_key),
                            sa_attrib->ike_remote_key_len);

    if (rsa != NU_NULL)
    {
        /* Verify the signature. */

        if (RSA_verify(md_type, md, md_len, sign, sign_len, rsa))
        {
            status = NU_SUCCESS;
        }

        RSA_free(rsa);
    }

    return (status);

} /* IKE_Verify_Signature */

#endif /* (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

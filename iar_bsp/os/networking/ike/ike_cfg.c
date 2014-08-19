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
*       ike_cfg.c
*
* COMPONENT
*
*       IKE - Configuration
*
* DESCRIPTION
*
*       This file contains IKE configuration data.
*
* DATA STRUCTURES
*
*       IKE_Encryption_Algos    Encryption algorithms interface.
*       IKE_Hash_Algos          Hash algorithms interface.
*       IKE_Sign_Algos          Signature algorithms interface.
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_doi.h
*       ike_crypto_wrappers.h
*       [ike_api.h]
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_doi.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#include "networking/ike_api.h"
#endif

/* Make sure MD5 is always enabled. */
#ifdef OPENSSL_NO_MD5
#error "MD5 hash algorithm must always be enabled in OpenSSL for use in IKE."
#endif

/**** Define IKE global configuration variables. ****/

/* All supported IKE Encryption algorithms and their
 * mapping to the OpenSSL library.
 */
const IKE_ENCRYPTION_ALGO IKE_Encryption_Algos[IKE_TOTAL_ENCRYPTION_ALGO] = {
#if (IKE_INCLUDE_DES == NU_TRUE)
#ifdef OPENSSL_NO_DES
#error "DES encryption enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_DES,
        IKE_DES
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_ENCR_DES
#endif
    },
#endif
#if (IKE_INCLUDE_3DES == NU_TRUE)
#ifdef OPENSSL_NO_DES
#error "3DES encryption enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_3DES,
        IKE_3DES
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_ENCR_3DES
#endif
    },
#endif
#if (IKE_INCLUDE_BLOWFISH == NU_TRUE)
#ifdef OPENSSL_NO_BF
#error "Blowfish encryption enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_BF,
        IKE_BLOWFISH
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_ENCR_BLOWFISH
#endif
    },
#endif
#if (IKE_INCLUDE_CAST128 == NU_TRUE)
#ifdef OPENSSL_NO_CAST
#error "CAST-128 encryption enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_CAST,
        IKE_CAST_128
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_ENCR_CAST
#endif
    },
#endif
#if (IKE_INCLUDE_AES == NU_TRUE)
#ifdef OPENSSL_NO_AES
#error "AES encryption enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_AES,
        IKE_AES
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_ENCR_AES_CBC
#endif
    },
#endif
};

/* All supported IKE Hash algorithms and their
 * mapping to the OpenSSL library.
 */
const IKE_HASH_ALGO IKE_Hash_Algos[IKE_TOTAL_HASH_ALGO] = {
#if (IKE_INCLUDE_MD5 == NU_TRUE)
#ifdef OPENSSL_NO_MD5
#error "MD5 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_MD5,
        IKE_MD5
#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_AUTH_HMAC_MD5_96
#endif
    },
#endif
#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#ifdef OPENSSL_NO_SHA1
#error "SHA1 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_SHA1,
        IKE_SHA1
#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,IKE2_AUTH_HMAC_SHA1_96
#endif
    },
#endif

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#if (IKE_INCLUDE_AES == NU_TRUE)
#ifdef OPENSSL_NO_AES
#error "AES algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_AES_XCBC,
        IKE_AES,
        IKE2_AUTH_AES_XCBC_96
    },
#endif

#if (IKE_INCLUDE_DES == NU_TRUE)
#ifdef OPENSSL_NO_DES
#error "DES algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_DES_MAC,
        0,  /* Not supported in IKEv1 */
        IKE2_AUTH_DES_MAC
    },
#endif

#endif
};

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
/* All supported IKE Signature algorithms and their
 * mapping to the OpenSSL library.
 */
const IKE_SIGN_ALGO IKE_Sign_Algos[IKE_TOTAL_SIGN_ALGO] = {
#if (IKE_INCLUDE_RSA == NU_TRUE)
#ifdef OPENSSL_NO_RSA
#error "RSA signatures (with no encoding) enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_RSA,
        IKE_RSA
#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
        ,0
#endif
    },

#endif

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#if (IKE2_INCLUDE_RSA == NU_TRUE)
#ifdef OPENSSL_NO_RSA
#error "IKEv2 needs RSA signatures (with encoding) enabled in OpenSSL."
#endif
    {
        IKE_OPENSSL_RSA_DS,
        0,
        IKE2_AUTH_METHOD_RSA_DS
    },
#endif
#endif

};
#endif /* (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

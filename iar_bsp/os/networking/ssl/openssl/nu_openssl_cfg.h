/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       nu_openssl_cfg.h
*
* COMPONENT
*
*       OpenSSL - Configuration
*
* DESCRIPTION
*
*       This file contains macros used to configure Nucleus OpenSSL.
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
#ifndef NU_OPENSSL_CFG_H
#define NU_OPENSSL_CFG_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

#include "nucleus_gen_cfg.h"

/* The following macro definition control the build process of OpenSSL.
 * If defined, they disable the relevant algorithms. An algorithm can
 * be enabled by un-defining its relevant macro definition.
 */

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_AES)
#undef OPENSSL_NO_AES
#else
#define OPENSSL_NO_AES
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_BIO)
#undef OPENSSL_NO_BIO
#else
#define OPENSSL_NO_BIO
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_BLOWFISH)
#undef OPENSSL_NO_BF
#else
#define OPENSSL_NO_BF
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_CAMELLIA)
#undef OPENSSL_NO_CAMELLIA
#else
#define OPENSSL_NO_CAMELLIA
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_CAST)
#undef OPENSSL_NO_CAST
#else
#define OPENSSL_NO_CAST
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_COMP)
#undef OPENSSL_NO_COMP
#else
#define OPENSSL_NO_COMP
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_DES)
#undef OPENSSL_NO_DES
#else
#define OPENSSL_NO_DES
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_DH)
#undef OPENSSL_NO_DH
#else
#define OPENSSL_NO_DH
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_DSA)
#undef OPENSSL_NO_DSA
#else
#define OPENSSL_NO_DSA
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_EC)
#undef OPENSSL_NO_EC
#else
#define OPENSSL_NO_EC
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_ECDH)
#undef OPENSSL_NO_ECDH
#else
#define OPENSSL_NO_ECDH
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_ECDSA)
#undef OPENSSL_NO_ECDSA
#else
#define OPENSSL_NO_ECDSA
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_ENGINE)
#undef OPENSSL_NO_ENGINE
#else
#define OPENSSL_NO_ENGINE
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_GMP)
#undef OPENSSL_NO_GMP
#else
#define OPENSSL_NO_GMP
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_GOST)
#undef OPENSSL_NO_GOST
#else
#define OPENSSL_NO_GOST
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_JPAKE)
#undef OPENSSL_NO_JPAKE
#else
#define OPENSSL_NO_JPAKE
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_SSL_INCLUDE_KRB5)
#undef OPENSSL_NO_KRB5
#else
#define OPENSSL_NO_KRB5
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_MD2)
#undef OPENSSL_NO_MD2
#else
#define OPENSSL_NO_MD2
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_MD4)
#undef OPENSSL_NO_MD4
#else
#define OPENSSL_NO_MD4
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_MD5)
#undef OPENSSL_NO_MD5
#else
#define OPENSSL_NO_MD5
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_MDC2)
#undef OPENSSL_NO_MDC2
#else
#define OPENSSL_NO_MDC2
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_SSL_INCLUDE_PSK)
#undef OPENSSL_NO_PSK
#else
#define OPENSSL_NO_PSK
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_RC2)
#undef OPENSSL_NO_RC2
#else
#define OPENSSL_NO_RC2
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_ARC4)
#undef OPENSSL_NO_ARC4
#else
#define OPENSSL_NO_ARC4
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_RIPEMD)
#undef OPENSSL_NO_RIPEMD
#else
#define OPENSSL_NO_RIPEMD
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_RSA)
#undef OPENSSL_NO_RSA
#else
#define OPENSSL_NO_RSA
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_SEED)
#undef OPENSSL_NO_SEED
#else
#define OPENSSL_NO_SEED
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_SHA1)
#undef OPENSSL_NO_SHA
#else
#define OPENSSL_NO_SHA
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_SHA256)
#undef OPENSSL_NO_SHA256
#else
#define OPENSSL_NO_SHA256
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_SHA512)
#undef OPENSSL_NO_SHA512
#else
#define OPENSSL_NO_SHA512
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_SSL_INCLUDE_SSL2)
#undef OPENSSL_NO_SSL2
#else
#define OPENSSL_NO_SSL2
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_SSL_INCLUDE_SSL3)
#undef OPENSSL_NO_SSL3
#else
#define OPENSSL_NO_SSL3
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_STORE)
#undef OPENSSL_NO_STORE
#else
#define OPENSSL_NO_STORE
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_SSL_INCLUDE_TLS1)
#undef OPENSSL_NO_TLS1
#else
#define OPENSSL_NO_TLS1
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_WHIRLPOOL)
#undef OPENSSL_NO_WHIRLPOOL
#else
#define OPENSSL_NO_WHIRLPOOL
#endif

#if (CFG_NU_OS_NET_SSL_OPENSSL_SSL_INCLUDE_X509)
#undef OPENSSL_NO_X509
#else
#define OPENSSL_NO_X509
#endif

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* NU_OPENSSL_CFG_H */

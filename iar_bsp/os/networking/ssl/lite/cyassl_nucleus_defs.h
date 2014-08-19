/***********************************************************************
*
*            Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file defines compiler options to enable or disable features
*       of CyaSSL. These are set using the metadata options.
*
***********************************************************************/

/* Check to see if this file has been included already.  */

#ifndef         CYASSL_NUCLEUS_DEFS_H

#ifdef          __cplusplus
/* C declarations in C++     */
extern          "C" {
#endif
#define         CYASSL_NUCLEUS_DEFS_H

#if CFG_NU_OS_NET_SSL_LITE_NO_CYASSL_CLIENT
    #define NO_CYASSL_CLIENT
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_CYASSL_SERVER
    #define NO_CYASSL_SERVER
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_DES3
    #define NO_DES3
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_DH
    #define NO_DH
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_AES
    #define NO_AES
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_ERROR_STRINGS
    #define NO_ERROR_STRINGS
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_HMAC
    #define NO_HMAC
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_MD4
    #define NO_MD4
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_SHA256
    #define NO_SHA256
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_PSK
    #define NO_PSK
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_PWDBASED
    #define NO_PWDBASED
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_RC4
    #define NO_RC4
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_RABBIT
    #define NO_RABBIT
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_HC128
    #define NO_HC128
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_SESSION_CACHE
    #define NO_SESSION_CACHE
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_TLS
    #define NO_TLS
#endif
#if CFG_NU_OS_NET_SSL_LITE_SMALL_SESSION_CACHE
    #define SMALL_SESSION_CACHE
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_CERT_GEN
    #define CYASSL_CERT_GEN
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_DER_LOAD
    #define CYASSL_DER_LOAD
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_DTLS
    #define CYASSL_DTLS
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_KEY_GEN
    #define CYASSL_KEY_GEN
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_RIPEMD
    #define CYASSL_RIPEMD
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_SHA384
    #define CYASSL_SHA384
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_SHA512
    #define CYASSL_SHA512
#endif
#if CFG_NU_OS_NET_SSL_LITE_DEBUG_CYASSL
    #define DEBUG_CYASSL
#endif
#if CFG_NU_OS_NET_SSL_LITE_HAVE_AESCCM
    #define HAVE_AESCCM
#endif
#if CFG_NU_OS_NET_SSL_LITE_HAVE_AESGCM
    #define HAVE_AESGCM
#endif
#if CFG_NU_OS_NET_SSL_LITE_HAVE_CAMELLIA
    #define HAVE_CAMELLIA
#endif
#if CFG_NU_OS_NET_SSL_LITE_HAVE_CRL
    #define HAVE_CRL
#endif
#if CFG_NU_OS_NET_SSL_LITE_HAVE_ECC
    #define HAVE_ECC
#endif
#if CFG_NU_OS_NET_SSL_LITE_HAVE_OCSP
    #define HAVE_OCSP
#endif
#if CFG_NU_OS_NET_SSL_LITE_OPENSSL_EXTRA
    #define OPENSSL_EXTRA
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_CALLBACKS
    #define CYASSL_CALLBACKS
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_USER_IO
    #define CYASSL_USER_IO
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM
    #define NO_FILESYSTEM
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_INLINE
    #define NO_INLINE
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_DEV_RANDOM
    #define NO_DEV_RANDOM
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_MAIN_DRIVER
    #define NO_MAIN_DRIVER
#endif
#if CFG_NU_OS_NET_SSL_LITE_NO_WRITEV
    #define NO_WRITEV
#endif
#if CFG_NU_OS_NET_SSL_LITE_SINGLE_THREADED
    #define SINGLE_THREADED
#endif
#if CFG_NU_OS_NET_SSL_LITE_USE_CERT_BUFFERS_1024
    #define USE_CERT_BUFFERS_1024
#endif
#if CFG_NU_OS_NET_SSL_LITE_USE_CERT_BUFFERS_2048
    #define USE_CERT_BUFFERS_2048
#endif
#if CFG_NU_OS_NET_SSL_LITE_TFM_TIMING_RESISTANT
    #define TFM_TIMING_RESISTANT
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_SMALL_STACK
    #define CYASSL_SMALL_STACK
#endif
#if CFG_NU_OS_NET_SSL_LITE_USE_FAST_MATH
    #define USE_FAST_MATH
#endif
#if CFG_NU_OS_NET_SSL_LITE_CYASSL_STM32F2
    #define CYASSL_STM32F2
#endif

#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif

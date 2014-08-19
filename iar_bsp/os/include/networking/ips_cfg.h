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
*       ips_cfg.h
*
* COMPONENT
*
*       IPSEC - Configuration
*
* DESCRIPTION
*
*       All configurable parameters for Nucleus IPsec are located here
*       which enables compile-time configurations of Nucleus IPsec.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef IPS_CFG_H
#define IPS_CFG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* _cplusplus */

/* These definitions control which version of IPsec the build should be
 * compatible with. This should allow new versions of IPsec to be shipped
 * but remain compatible with applications or drivers designed for previous
 * versions.
 */
#define IPSEC_2_0                           2       /* IPSEC 2.0 */

/* The version of IPsec for which compatibility is desired. */
#define IPSEC_VERSION_COMP                  IPSEC_2_0


/*
 * The IPsec initialization task is responsible for initializing IPsec
 * when Nucleus Middleware Initialization (NMI) is being used.
 */
#define IPSEC_INIT_STACK_SIZE               3000
#define IPSEC_INIT_PRIORITY                 1
#define IPSEC_INIT_TIME_SLICE               0
#define IPSEC_INIT_PREEMPT                  NU_PREEMPT

/*
 * Macros for OpenSSL generic cipher functions to do
 * encryption or decryption.
 */
#define ENCRYPT 1
#define DECRYPT 0

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with AH support. If AH support is not required, this macro can be set
 * to NU_FALSE.
 */
#define IPSEC_INCLUDE_AH                    NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is
 * compiled with ESP support. If ESP support is not required, this macro
 * can be set to NU_FALSE.
 */
#define IPSEC_INCLUDE_ESP                   NU_TRUE

/* At least one IPsec protocol should be present. */
#if ((IPSEC_INCLUDE_ESP == NU_FALSE) && (IPSEC_INCLUDE_AH == NU_FALSE))
    #error Both AH and ESP cannot be disabled at a time
#endif

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is
 * compiled with support for transport mode. If transport mode support
 * is not required, this macro can be set to NU_FALSE.
 */
#define IPSEC_INCLUDE_TRANSPORT_MODE        NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is
 * compiled with support for tunnel mode. If tunnel mode support is not
 * required, this macro can be set to NU_FALSE.
 */
#define IPSEC_INCLUDE_TUNNEL_MODE           NU_TRUE

/*
 * By default, a policy that is entered first in to the IPsec policy
 * database is given priority over policies that are added later.
 * This macro changes the default schema so that different policies
 * can be assigned different priorities. A policies' priority is set
 * using ipsec_priority in the IPSEC_POLICY structure. Policies with
 * the same priority are sorted according to the order they were
 * entered in to the IPsec database. This macro should be set to NU_TRUE
 * to enable priorities.
 */
#define IPSEC_ENABLE_PRIORITY               NU_FALSE

/*
 * By default this macro is set to NU_FALSE, and the  service
 * is enabled. Nucleus IPsec can be compiled without the anti-replay
 * service by setting the macro to NU_FALSE.
 */
#define IPSEC_ANTI_REPLAY                   NU_TRUE

/*
 * This macro defines the window size for Anti-replay window. The size is
 * given in terms of multiples of 32. For example, if the default value of
 * 1 is used, this actually means a 32-bit window size.
 */
#define IPSEC_SLIDING_WINDOW_SIZE           2

/* This macro defines the maximum number of Security Associations that
 * are allowed in one SA bundle. This macro should be kept to a reasonably
 * small value to ensure that the system is not overburdened.
 */
#define IPSEC_MAX_SA_BUNDLE_SIZE            2

/* Defining default timeout value used while obtaining the semaphore. */
#define IPSEC_SEM_TIMEOUT                   NU_SUSPEND

/*
 * When set to NU_TRUE, this option compiles Nucleus IPsec in debug mode.
 * In debug mode, extensive error checking and error logging is done.
 * Debug mode can be used to detect problems in IPsec configurations.
 */
#define IPSEC_DEBUG                         NU_TRUE

/****   Macros related to Authenticator. ****/
/*
 * Total number of authentication algorithms supported.
 */
#define IPSEC_TOTAL_AUTH_ALGO               5

/*
 * To indicate that all algorithms have a one-to-one mapping between their
 * index and the identifier, this macro must be set to NU_TRUE. Note that
 * by default this macro is set to NU_TRUE.
 */
#define IPSEC_AUTH_1_TO_1                   NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with HMAC-MD5-96 support. This macro can be set to NU_FALSE to exclude
 * the algorithm from the build.
 */
#define IPSEC_INCLUDE_MD5                   NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with HMAC-SHA-1-96 support. This macro can be set to NU_FALSE to
 * exclude the algorithm from the build.
 */
#define IPSEC_INCLUDE_SHA1                  NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with HMAC-SHA-256-96 support. This macro can be set to NU_FALSE to
 * exclude the algorithm from the build.
 */
#define IPSEC_INCLUDE_SHA256                NU_TRUE

/*
 * By default this macro is set to NU_FALSE and Nucleus IPsec is compiled
 * without AES-XCBC-MAC-96 support. This macro can be set to NU_TRUE to
 * include the algorithm in to the build. If this is included, then AES
 * support will also need to be included.
 */
#define IPSEC_INCLUDE_XCBC_MAC_96           NU_FALSE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with support for NULL authentication. This macro can be set to NU_FALSE
 * to exclude the algorithm from the build.
 */
#define IPSEC_INCLUDE_NULL_AUTH             NU_TRUE
/**** End of macros related to Authenticator. ****/

/**** Macros related to Encryptor Component. ****/
/*
 * Total number of encryption algorithms supported.
 */
#define IPSEC_TOTAL_ENCRYPT_ALGO            6

/*
 * To indicate that all algorithms have a one-to-one mapping between
 * their index and the identifier, this macro must be set to NU_TRUE.
 * Note that by default this macro is set to NU_TRUE.
 */
#define IPSEC_ENCRYPT_1_TO_1                NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with DES-CBC support. This macro can be set to NU_FALSE to exclude the
 * algorithm from the build.
 */
#define IPSEC_INCLUDE_DES                   NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with 3DES-CBC support. This macro can be set to NU_FALSE to exclude the
 * algorithm from the build.
 */
#define IPSEC_INCLUDE_3DES                  NU_TRUE

/*
 * By default this macro is set to NU_FALSE, and Nucleus IPsec is
 * compiled without AES-CBC support. This macro can be set to NU_TRUE to
 * include the algorithm in the build.
 */
#ifdef CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_AES
#define IPSEC_INCLUDE_AES                   NU_TRUE
#else
#define IPSEC_INCLUDE_AES                   NU_FALSE
#endif /* CFG_NU_OS_NET_SSL_OPENSSL_CRYPTO_INCLUDE_AES */

#if (IPSEC_INCLUDE_AES == NU_TRUE)
#ifdef OPENSSL_NO_AES
    #error "OPENSSL_NO_AES must not be defined to include AES in OpenSSL."
#endif
#endif

#if ((IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE) && (IPSEC_INCLUDE_AES == NU_FALSE))
    #error IPSEC_INCLUDE_AES must be NU_TRUE to include AES-XCBC-MAC-96
#endif

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with BLOWFISH-CBC support. This macro can be set to NU_FALSE to exclude
 * the algorithm from the build.
 */
#define IPSEC_INCLUDE_BLOWFISH              NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with CAST-128 support. This macro can be set to NU_FALSE to exclude the
 * algorithm from the build.
 */
#define IPSEC_INCLUDE_CAST128               NU_TRUE

/*
 * By default this macro is set to NU_TRUE, and Nucleus IPsec is compiled
 * with NULL Encryption support. This macro can be set to NU_FALSE to
 * exclude the algorithm from the build.
 */
#define IPSEC_INCLUDE_NULL_ENCRYPTION       NU_TRUE

/* Defining authentication algorithms IDs */
#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
#define IPSEC_NULL_CIPHER               0
#else
#define IPSEC_NULL_CIPHER               -1
#endif

#if (IPSEC_INCLUDE_DES == NU_TRUE)
#define IPSEC_DES_CBC                   (IPSEC_INCLUDE_NULL_ENCRYPTION)
#else
#define IPSEC_DES_CBC                   -1
#endif

#if (IPSEC_INCLUDE_3DES == NU_TRUE)
#define IPSEC_3DES_CBC                  (IPSEC_INCLUDE_NULL_ENCRYPTION + \
                                         IPSEC_INCLUDE_DES)
#else
#define IPSEC_3DES_CBC                  -1
#endif

#if (IPSEC_INCLUDE_AES == NU_TRUE)
#define IPSEC_AES_CBC                   (IPSEC_INCLUDE_NULL_ENCRYPTION + \
                                         IPSEC_INCLUDE_DES  + \
                                         IPSEC_INCLUDE_3DES)
#else
#define IPSEC_AES_CBC                   -1
#endif

#if (IPSEC_INCLUDE_BLOWFISH == NU_TRUE)
#define IPSEC_BLOWFISH_CBC              (IPSEC_INCLUDE_NULL_ENCRYPTION + \
                                         IPSEC_INCLUDE_DES  + \
                                         IPSEC_INCLUDE_3DES + \
                                         IPSEC_INCLUDE_AES)
#else
#define IPSEC_BLOWFISH_CBC              -1
#endif

#if (IPSEC_INCLUDE_CAST128 == NU_TRUE)
#define IPSEC_CAST_128                  (IPSEC_INCLUDE_NULL_ENCRYPTION + \
                                         IPSEC_INCLUDE_DES      + \
                                         IPSEC_INCLUDE_3DES     + \
                                         IPSEC_INCLUDE_AES      + \
                                         IPSEC_INCLUDE_BLOWFISH)
#else
#define IPSEC_CAST_128                  -1
#endif
/**** End of macros, related to Encryptor. ****/

/* Defining authentication algorithms IDs */
#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
#define IPSEC_NULL_AUTH                 0
#else
#define IPSEC_NULL_AUTH                 -1
#endif

/* Defining HMAC MD5 algorithm. */
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
#define IPSEC_HMAC_MD5_96               (IPSEC_INCLUDE_NULL_AUTH)
#else
#define IPSEC_HMAC_MD5_96               -1
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
#define IPSEC_HMAC_SHA_1_96             (IPSEC_INCLUDE_NULL_AUTH + \
                                         IPSEC_INCLUDE_MD5)
#else
#define IPSEC_HMAC_SHA_1_96             -1
#endif

#if (IPSEC_INCLUDE_SHA256 == NU_TRUE)
#define IPSEC_HMAC_SHA_256_96           (IPSEC_INCLUDE_NULL_AUTH + \
                                         IPSEC_INCLUDE_MD5  + \
                                         IPSEC_INCLUDE_SHA1)
#else
#define IPSEC_HMAC_SHA_256_96           -1
#endif

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
#define IPSEC_XCBC_MAC_96               (IPSEC_INCLUDE_NULL_AUTH + \
                                         IPSEC_INCLUDE_MD5  + \
                                         IPSEC_INCLUDE_SHA1 + \
                                         IPSEC_INCLUDE_SHA256)
#else
#define IPSEC_XCBC_MAC_96               -1
#endif

/* End of authentication algorithms definitions. */

/* Maximum IPsec headers overhead in TCP/IP stack. This macro dictates how
 * much extra room should be reserved for IPsec headers while calculating
 * or advertising different lengths e.g. TCP MSS.
 */
#if (IPSEC_INCLUDE_ESP == NU_TRUE)
#define IPSEC_HDRS_OVERHEAD             (IPSEC_ESP_HDRFIXED_LEN + \
                                        IPSEC_ESP_MAX_AUTHDATA_LEN + \
                                        IPSEC_ESP_TRAILER_LEN + \
                                        IPSEC_ESP_MAX_PAD + \
                                        IPSEC_ESP_MAX_IV)
#else
#define IPSEC_HDRS_OVERHEAD             (IPSEC_AH_HDRFIXED_LEN + \
                                        IPSEC_AH_MAX_AUTHDATA_LEN)
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* #ifndef IPS_CFG_H */

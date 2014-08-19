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
*       ips_cfg.c
*
* COMPONENT
*
*       IPSEC - Configuration
*
* DESCRIPTION
*
*       Enables compile-time configuration of Nucleus IPsec.
*
* DATA STRUCTURES
*
*       IPSEC_Authentication_Algos
*       IPSEC_Encryption_Algos
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       evp.h
*       nu_net.h
*       ips_api.h
*
************************************************************************/
/* Including the IPsec configuration header file. */
#include "networking/nu_net.h"
#include "openssl/evp.h"
#include "networking/ips_api.h"


/*
 * The global array of authentication algorithms that are supported by
 * Nucleus IPsec. They are defined in the following manner.
 *
 * 1) Authentication algorithm key length
 * 2) Authentication algorithm digest length
 * 3) Authentication algorithm Nucleus IPsec ID
 * 4) Authentication algorithm Nucleus Cipher Suite ID
 */
const IPSEC_AUTH_ALGO IPSEC_Authentication_Algos[IPSEC_TOTAL_AUTH_ALGO] =
                    {
#if (IPSEC_INCLUDE_NULL_AUTH == NU_TRUE)
                        /* First entry defines null algorithm. */
                        {
                            0,
                            0,
                            IPSEC_NULL_AUTH,
                        },
#endif

#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
                        {
                            IPSEC_MD5_KEY_LEN,
                            IPSEC_AH_AUTHDATA_LEN,
                            IPSEC_HMAC_MD5_96,
                        },
#endif

#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
                        {
                            IPSEC_SHA1_KEY_LEN,
                            IPSEC_AH_AUTHDATA_LEN,
                            IPSEC_HMAC_SHA_1_96,
                        },
#endif

#if (IPSEC_INCLUDE_SHA256 == NU_TRUE)
                        {
                            IPSEC_SHA256_KEY_LEN,
                            IPSEC_AH_AUTHDATA_LEN,
                            IPSEC_HMAC_SHA_256_96,
                        },
#endif

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
                        {
                            IPSEC_XCBC_MAC_96_KEY_LEN,
                            IPSEC_AH_AUTHDATA_LEN,
                            IPSEC_XCBC_MAC_96,
                        },
#endif
                    };

/*
 * The global array of encryption algorithms that are supported by
 * Nucleus IPsec. They are defined in the following manner.
 *
 * 1) Encryption algorithm key length
 * 2) Encryption algorithm Nucleus IPsec ID
 * 3) Encryption algorithm Nucleus Cipher Suite ID
 */
const IPSEC_ENCRYPTION_ALGO IPSEC_Encryption_Algos[IPSEC_TOTAL_ENCRYPT_ALGO] =
                        {
#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
                            {
                                /* Key length, defined by RFC 2410. */
                                0,
                                0,
                                IPSEC_NULL_CIPHER,
                            },
#endif

#if (IPSEC_INCLUDE_DES == NU_TRUE)
                            {
                                (const void* (*)(void))EVP_des_cbc,
                                IPSEC_DES_KEY_LEN,
                                IPSEC_DES_CBC,
                            },
#endif

#if (IPSEC_INCLUDE_3DES == NU_TRUE)
                            {
                                (const void* (*)(void))EVP_des_ede3_cbc,
                                IPSEC_3DES_KEY_LEN,
                                IPSEC_3DES_CBC,
                            },
#endif

#if (IPSEC_INCLUDE_AES == NU_TRUE)
                            {
                                (const void* (*)(void))EVP_aes_128_cbc,
                                IPSEC_AES_KEY_LEN,
                                IPSEC_AES_CBC,
                            },
#endif

#if (IPSEC_INCLUDE_BLOWFISH == NU_TRUE)
                            {
                                (const void* (*)(void))EVP_bf_cbc,
                                IPSEC_BLOWFISH_KEY_LEN,
                                IPSEC_BLOWFISH_CBC,
                            },
#endif

#if (IPSEC_INCLUDE_CAST128 == NU_TRUE)
                            {
                                (const void* (*)(void))EVP_cast5_cbc,
                                IPSEC_CAST_128_KEY_LEN,
                                IPSEC_CAST_128,
                            },
#endif
                        };


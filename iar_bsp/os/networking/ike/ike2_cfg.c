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
*       ike2_cfg.c
*
* COMPONENT
*
*       General
*
* DESCRIPTION
*
*       Configuration specific code
*
* FUNCTIONS
*
*       IKE2_Get_Algo_Index
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike_api.h
*       ike_crypto_wrappers.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike_api.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#if (IKE_INCLUDE_SIG_AUTH == NU_FALSE)
#ifndef OPENSSL_NO_MD5
#include "openssl/md5.h"
#endif

#ifndef OPENSSL_NO_SHA1
#include "openssl/sha.h"
#endif
#endif

#if (IKE_INCLUDE_AES == NU_TRUE)
#ifndef OPENSSL_NO_AES
#include "openssl/aes.h"
#endif
#endif

/*
 * Map IKEv2 defines on to Cipher Suite specific values.
 */
const IKE2_PRF_ALGO IKE2_PRF_Algos[IKE2_TOTAL_PRF_ALGOS] = {
/* MD5 as PRF algorithm */
#if (IKE_INCLUDE_MD5 == NU_TRUE)
#ifdef OPENSSL_NO_MD5
#error "MD5 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_MD5,
        0,
        IKE2_PRF_HMAC_MD5,
    },
#endif

/* SHA1 as PRF algorithm */
#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#ifdef OPENSSL_NO_SHA1
#error "SHA1 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_SHA1,
        0,
        IKE2_PRF_HMAC_SHA1,
    },
#endif

/* AES128_XCBC as PRF algorithm */
#if (IKE_INCLUDE_AES == NU_TRUE)
#ifdef OPENSSL_NO_AES
#error "AES algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE_OPENSSL_AES_XCBC,
        0,
        IKE2_PRF_AES128_XCBC,
    },
#endif

}; /* IKE2_PRF_Algos */

/*
 * Information about the PRF HMAC algorithms which are used with IKEv2.
 */
const IKE2_HMAC_ALGOS IKE2_PRF_HMAC_Algos[IKE2_TOTAL_PRF_ALGOS] =
{
#if (IKE_INCLUDE_MD5 == NU_TRUE)
#ifdef OPENSSL_NO_MD5
#error "MD5 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE2_PRF_HMAC_MD5,      /* IKEv2 Algorithm ID */
        16,                     /* Key length */
        IKE_MD5_DIGEST_LEN      /* Output length */
    },
#endif

#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#ifdef OPENSSL_NO_SHA1
#error "SHA1 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE2_PRF_HMAC_SHA1,     /* IKEv2 Algorithm ID */
        20,                     /* Key length */
        20                      /* Output length */
    },
#endif

#if (IKE_INCLUDE_AES == NU_TRUE)
#ifdef OPENSSL_NO_AES
#error "AES algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE2_PRF_AES128_XCBC,   /* IKEv2 Algorithm ID */
        16,                     /* Key length */
        16                      /* Output length */
    },
#endif
}; /* IKE2_PRF_HMAC_Algos */

/*
 * Information about the AUTH HMAC algorithms which are used with IKEv2.
 */
const IKE2_HMAC_ALGOS IKE2_AUTH_HMAC_Algos[IKE_TOTAL_HASH_ALGO] =
{
#if (IKE_INCLUDE_MD5 == NU_TRUE)
#ifdef OPENSSL_NO_MD5
#error "MD5 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE2_AUTH_HMAC_MD5_96,  /* IKEv2 Algorithm ID */
        16,                     /* Key length */
        IKE_MD5_96_DIGEST_LEN   /* Output length */
    },
#endif

#if (IKE_INCLUDE_SHA1 == NU_TRUE)
#ifdef OPENSSL_NO_SHA1
#error "SHA1 hash algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE2_AUTH_HMAC_SHA1_96, /* IKEv2 Algorithm ID */
        20,                     /* Key length */
        12                      /* Output length (in octets) */
    },
#endif

#if (IKE_INCLUDE_AES == NU_TRUE)
#ifdef OPENSSL_NO_AES
#error "AES algorithm enabled in IKE but not in OpenSSL."
#endif
    {
        IKE2_AUTH_AES_XCBC_96,  /* IKEv2 Algorithm ID */
        16,                     /* Key length */
        12                      /* Output length (in octets) */
    },
#endif

}; /* IKE2_AUTH_HMAC_Algos */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Get_Algo_Index
*
* DESCRIPTION
*
*       Finds the index of algorithm in the respective array when given
*       the IKEv2 ID for that algorithm.
*
* INPUTS
*
*       ike_id                  IKEv2 ID for the algorithm.
*       *algos                  Array of algorithms to be searched.
*       algos_size              Size of algorithm array entry.
*       algos_no                Number of entries in the array.
*       *index                  Index to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_UNSUPPORTED_ALGO    Algorithm index was not found.
*
*************************************************************************/
STATUS IKE2_Get_Algo_Index(UINT16 ike_id, const VOID *algos, INT algos_size,
                           UINT16 algos_no, UINT16 *index)
{
    STATUS          status = IKE_UNSUPPORTED_ALGO;
    UINT16          i;

    /* Loop for all algorithms included in IKE. */
    for(i = 0; i < algos_no; i++)
    {
        /* Check if a match is found. */
        if(ike_id == (UINT16)((IKE_HASH_ALGO*)algos)->ike2_algo_identifier)
        {
            /* Set the pointer to be returned. */
            *index = i;

            /* Set status to success and break out of loop. */
            status = NU_SUCCESS;
            break;
        }

        /* Move the algorithm pointer to next item in array. */
        algos = ((UINT8*)algos) + algos_size;
    }

    /* Return the status. */
    return (status);

} /* IKE2_Get_Algo_Index */

#endif

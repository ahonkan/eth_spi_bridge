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
*       ike_auth.c
*
* COMPONENT
*
*       IKE - Authenticator
*
* DESCRIPTION
*
*       This file implements the IKE Authenticator component. The
*       authenticator is responsible for verifying the authenticity
*       of IKE exchanges.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Sign
*       IKE_Sign_Verify
*       IKE_Generate_Cookie
*       IKE_Generate_Message_ID
*       IKE_MD5_Hash
*
* DEPENDENCIES
*
*       nu_net.h
*       md5.h
*       rand.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*       ike_doi.h
*       ike_auth.h
*       ike_cert.h
*       ike_crypto_wrappers.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "openssl/md5.h"
#include "openssl/rand.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"
#include "networking/ike_doi.h"
#include "networking/ike_auth.h"

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)

#include "networking/ike_cert.h"
#include "networking/ike_crypto_wrappers.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_Sign
*
* DESCRIPTION
*
*       This function generates the digital signature for the
*       specified message digest.
*
*       Note that the signature returned by this function MUST
*       be freed using the IKE_Sign_Free function.
*
* INPUTS
*
*       *sa                     Pointer to the SA of this exchange.
*       *msg                    Pointer to digest being signed.
*       msg_len                 Length of digest, in bytes.
*       **sign                  On return, contains a pointer
*                               to the signature, stored in a
*                               dynamically allocated buffer
*       *sign_len               On return, contains the signature
*                               length, in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        Crypto related error.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_UNSUPPORTED_ALGO    A required algorithm is not
*                               supported by IKE.
*
*************************************************************************/
STATUS IKE_Sign(IKE_SA *sa, UINT8 *msg, UINT8 msg_len, UINT8 **sign,
                UINT *sign_len)
{
    STATUS          status;
    UINT16          sign_algo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the pointers are not NULL. */
    if((sa == NU_NULL) || (msg == NU_NULL) || (sign == NU_NULL) ||
       (sign_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure local certificate file is specified. */
    if(sa->ike_attributes.ike_local_key_file == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

#endif /* (IKE_DEBUG == NU_TRUE) */

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating signature");

    /* Get the signature algorithm index. */
    status = IKE_SIGN_ALGO_INDEX(sa->ike_attributes.ike_auth_method,
                                 sign_algo);

    if(status == NU_SUCCESS)
    {
        /* Calculate the signature. */
        status = IKE_Compute_Signature(IKE_Sign_Algos[sign_algo].crypto_algo_id,
                                       &(sa->ike_attributes), msg, msg_len,
                                       IKE_HASHID_TO_NID(IKE_Hash_Algos[sa->ike_attributes.
                                                         ike_hash_algo].ike_algo_identifier),
                                       sign, sign_len);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to generate the signature",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Unknown signature algorithm specified",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Sign */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Sign_Verify
*
* DESCRIPTION
*
*       This function verifies the digital signature for the
*       specified message digest.
*
* INPUTS
*
*       *sa                     Pointer to the SA of this exchange.
*       *msg                    Pointer to digest being verified.
*       msg_len                 Length of digest, in bytes.
*       *sign                   Pointer to the signature.
*       sign_len                Length of signature, in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        Crypto related error.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_VERIFY_FAILED       Signature verification failed.
*
*************************************************************************/
STATUS IKE_Sign_Verify(IKE_SA *sa, UINT8 *msg, UINT8 msg_len,
                       UINT8 *sign, UINT16 sign_len)
{
    STATUS          status;
    UINT16          sign_algo;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the pointers are not NULL. */
    if((sa == NU_NULL) || (msg == NU_NULL) || (sign == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure signature keys are valid. */
    if((sa->ike_attributes.ike_remote_key     == NU_NULL) ||
       (sa->ike_attributes.ike_remote_key_len == 0))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Verifying signature");

    /* Get the signature algorithm index. */
    status = IKE_SIGN_ALGO_INDEX(sa->ike_attributes.ike_auth_method,
                                 sign_algo);

    if(status == NU_SUCCESS)
    {
        /* Verify the signature. */
        status = IKE_Verify_Signature(IKE_Sign_Algos[sign_algo].crypto_algo_id,
                                      &(sa->ike_attributes), msg, msg_len,
                                      IKE_HASHID_TO_NID(IKE_Hash_Algos[sa->ike_attributes.
                                                                       ike_hash_algo].ike_algo_identifier),
                                      sign, sign_len);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Signature verification failed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Set status to IKE error status so that
             * informational messages can be sent.
             */
            status = IKE_VERIFY_FAILED;
        }

        else
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("Signature verification successful");
        }
    }

    else
    {
        NLOG_Error_Log("Unknown signature algorithm specified",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Sign_Verify */

#endif /* (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_Cookie
*
* DESCRIPTION
*
*       This function generates an IKE cookie and stores it
*       in the specified buffer.
*
* INPUTS
*
*       *cookie                 Pointer to the buffer where
*                               the cookie will be stored.
*       *addr                   Destination address to
*                               ensure a unique cookie.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Generate_Cookie(UINT8 *cookie, struct addr_struct *addr)
{
    STATUS          status = IKE_CRYPTO_ERROR;
    MD5_CTX         ctx;
    UNSIGNED        ticks;
    UNSIGNED        rand_val;
    UINT8           digest [16];

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((cookie == NU_NULL) || (addr == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating phase 1 cookie");

    /* Initialize the hash function. */
    MD5_Init(&ctx);

    /* Hash the destination address. */
    MD5_Update(&ctx, (UINT8*)addr, sizeof(struct addr_struct));

    /* Get a random number. */
    if (RAND_bytes((UINT8 *)&rand_val, sizeof(UNSIGNED)))
    {
        /* Get CPU ticks for more randomness. */
        ticks = NU_Retrieve_Clock() + rand_val;

        /* Hash the CPU ticks. */
        MD5_Update(&ctx, (UINT8*)&ticks, sizeof(UNSIGNED));

        /* Finalize the hash digest. */
        MD5_Final(digest, &ctx);

        memcpy(cookie, digest, IKE_COOKIE_LEN);

        status = NU_SUCCESS;
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_Cookie */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Generate_Message_ID
*
* DESCRIPTION
*
*       This function generates an IKE message ID and stores it
*       in the specified buffer.
*
* INPUTS
*
*       *msg_id                 On return, this contains the
*                               new Message ID.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Generate_Message_ID(UINT32 *msg_id)
{
    STATUS          status;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if(msg_id == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating phase 2 Message ID");

    /* Generate random message ID. */
    status = RAND_bytes((UINT8*)msg_id, sizeof(UINT32)) ? NU_SUCCESS : IKE_CRYPTO_ERROR;

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to generate random number using OpenSSL",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Generate_Message_ID */

/*************************************************************************
*
* FUNCTION
*
*       IKE_MD5_Hash
*
* DESCRIPTION
*
*       This function calculates the MD5 digest of the specified
*       message and returns it in the digest buffer. The digest
*       is always 16 bytes in length (IKE_MD5_DIGEST_LEN).
*
* INPUTS
*
*       *msg                    Message to be hashed.
*       msg_len                 Length of the message.
*       *digest                 On return, this buffer contains
*                               the MD5 digest of the message.
*                               The buffer should be 16 bytes
*                               in size.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_MD5_Hash(UINT8 *msg, UINT16 msg_len, UINT8 *digest)
{
    STATUS          status;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if((msg == NU_NULL) || (digest == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Perform MD5 */
    if(NU_NULL != MD5(msg, msg_len, digest))
    {
        status = NU_SUCCESS;
    }
    else
    {
        status = IKE_CRYPTO_ERROR;
        NLOG_Error_Log("Failed to compute MD5 hash",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_MD5_Hash */

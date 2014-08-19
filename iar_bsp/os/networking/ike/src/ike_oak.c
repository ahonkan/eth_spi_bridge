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
*       ike_oak.c
*
* COMPONENT
*
*       IKE - Oakley
*
* DESCRIPTION
*
*       This file contains the implementation of utility
*       functions used by different IKE Exchanges.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_SKEYID_Allocate
*       IKE_SKEYID_Preshared_Key
*       IKE_SKEYID_Signatures
*       IKE_SKEYID_dae
*       IKE_Hash_x
*       IKE_Verify_Hash_x
*       IKE_Phase1_Encryption_Material
*       IKE_Phase1_Key_Material
*       IKE_KEYMAT_Allocate
*       IKE_KEYMAT_x
*       IKE_Phase2_Key_Material
*       IKE_Phase2_IV
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_api.h
*       ike_auth.h
*       ike_dh.h
*       ike_oak.h
*       ike_ips.h
*       ike_crypto_wrappers.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_auth.h"
#include "networking/ike_dh.h"
#include "networking/ike_oak.h"
#include "networking/ike_ips.h"
#include "networking/ike_crypto_wrappers.h"

/*************************************************************************
*
* FUNCTION
*
*       IKE_SKEYID_Allocate
*
* DESCRIPTION
*
*       This function allocates buffers for storing SKEYID,
*       SKEYID_d, SKEYID_a, SKEYID_e. All these buffers are
*       allocated using a single block of memory, starting
*       from SKEYID. The size of the buffers is dependent
*       on the negotiated PRF (i.e. hash) function's digest
*       length, since SKEYIDs are all digest values.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA. On return,
*                               this would contain the dynamic
*                               buffers for SKEYIDs.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_SKEYID_Allocate(IKE_SA *sa)
{
    STATUS          status;
    UINT8           dgst_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the SA pointer is valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Allocating memory for SKEYID");

    /* Get hash digest length. */
    status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                   &dgst_len);

    if(status == NU_SUCCESS)
    {
        /* Allocate memory for all the SKEYID buffers. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&sa->ike_skeyid,
                                    dgst_len * 4,
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Normalize the pointer. */
            sa->ike_skeyid = TLS_Normalize_Ptr(sa->ike_skeyid);

            /* Set the SKEYID_d buffer. */
            sa->ike_skeyid_d = sa->ike_skeyid + dgst_len;

            /* Set the SKEYID_a buffer. */
            sa->ike_skeyid_a = sa->ike_skeyid_d + dgst_len;

            /* Set the SKEYID_e buffer. */
            sa->ike_skeyid_e = sa->ike_skeyid_a + dgst_len;

            /* Set length of the SKEYIDs. */
            sa->ike_skeyid_len = dgst_len;
        }

        else
        {
            NLOG_Error_Log("Failed to allocate memory for SKEYID",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get hash digest length",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_SKEYID_Allocate */

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_SKEYID_Preshared_Key
*
* DESCRIPTION
*
*       This function computes the SKEYID for a Phase 1 Exchange
*       using a pre-shared key. The value calculated is:
*
*           SKEYID = prf(pre-shared-key, Ni_b | Nr_b)
*
*       Note that the SKEYID buffers MUST be allocated before
*       calling this function, using IKE_SKEYID_Allocate.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       *nonce_i                Initiator's Nonce data.
*       nonce_i_len             Initiator's Nonce data length.
*       *nonce_r                Responder's Nonce data.
*       nonce_r_len             Responder's Nonce data length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_SKEYID_Preshared_Key(IKE_SA *sa, UINT8 *nonce_i,
                                UINT16 nonce_i_len, UINT8 *nonce_r,
                                UINT16 nonce_r_len)
{
    STATUS          status = NU_SUCCESS;
    VOID            *ctx;
    UINT8           temp_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((sa      == NU_NULL) || (nonce_i        == NU_NULL) ||
       (nonce_r == NU_NULL) || (sa->ike_skeyid == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the pre-shared key is present in the SA. */
    if((sa->ike_attributes.ike_remote_key     == NU_NULL) ||
       (sa->ike_attributes.ike_remote_key_len == 0))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Computing SKEYID for Pre-shared Keys");

    /* Initialize the PRF. */

    status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                           &ctx, sa->ike_attributes.ike_remote_key,
                           sa->ike_attributes.ike_remote_key_len);

    if(status == NU_SUCCESS)
    {
        /* Add Initiator's Nonce data to the PRF state. */
        status = IKE_HMAC_Update(ctx, nonce_i, nonce_i_len);

        if(status == NU_SUCCESS)
        {
            /* Add Responder's Nonce data to the PRF state. */
            status = IKE_HMAC_Update(ctx, nonce_r, nonce_r_len);

            if(status == NU_SUCCESS)
            {
                /* Finalize the PRF with actual digest length. */
                status = IKE_HMAC_Final(ctx, sa->ike_skeyid, &temp_len);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to finalize PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to update PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to initialize PRF context",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_SKEYID_Preshared_Key */
#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */

#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_SKEYID_Signatures
*
* DESCRIPTION
*
*       This function computes the SKEYID for a Phase 1 Exchange
*       using Signatures. The value calculated is:
*
*           SKEYID = prf(Ni_b | Nr_b, g^xy)
*
*       Note that the SKEYID buffers MUST be allocated before
*       calling this function, using IKE_SKEYID_Allocate.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       *gxy                    The Diffie-Hellman shared key.
*       gxy_len                 Length of Diffie-Hellman key.
*       *nonce_i                Initiator's Nonce data.
*       nonce_i_len             Initiator's Nonce data length.
*       *nonce_r                Responder's Nonce data.
*       nonce_r_len             Responder's Nonce data length.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_SKEYID_Signatures(IKE_SA *sa, UINT8 *gxy, UINT16 gxy_len,
                             UINT8 *nonce_i, UINT16 nonce_i_len,
                             UINT8 *nonce_r, UINT16 nonce_r_len)
{
    STATUS          status;
    VOID            *ctx;
    UINT8           temp_len;
    UINT8           *nonces;
    UINT8           prf_key_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((sa      == NU_NULL) || (gxy     == NU_NULL) ||
       (nonce_i == NU_NULL) || (nonce_r == NU_NULL) ||
       (sa->ike_skeyid == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Computing SKEYID for Signatures");

    /* Allocate a single memory block large enough to
     * store both Initiator and Responder Nonce data.
     */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&nonces,
                                nonce_i_len + nonce_r_len, NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        nonces = TLS_Normalize_Ptr(nonces);

        /* Copy the Nonce data to a single block. */
        NU_BLOCK_COPY(nonces, nonce_i, nonce_i_len);
        NU_BLOCK_COPY(nonces + nonce_i_len, nonce_r, nonce_r_len);

        /* Calculate length of PRF key. */
        if(nonce_i_len + nonce_r_len <= IKE_MAX_PRF_KEY_LEN)
        {
            prf_key_len = (UINT8)(nonce_i_len + nonce_r_len);
        }

        else
        {
            prf_key_len = IKE_MAX_PRF_KEY_LEN;
        }

        /* Initialize the PRF. */
        status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.
                                   ike_hash_algo].crypto_algo_id,
                               &ctx, nonces, prf_key_len);

        if(status == NU_SUCCESS)
        {
            /* Add the Diffie-Hellman key to the PRF state. */
            status = IKE_HMAC_Update(ctx, gxy, gxy_len);

            if(status == NU_SUCCESS)
            {
                /* Finalize the PRF with actual digest length. */
                status = IKE_HMAC_Final(ctx, sa->ike_skeyid, &temp_len);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to finalize PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to initialize PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Now free the Nonce data buffer. */
        if(NU_Deallocate_Memory(nonces) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Nonce buffer",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for Nonce data",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_SKEYID_Signatures */
#endif /* (IKE_INCLUDE_SIG_AUTH == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_SKEYID_dae
*
* DESCRIPTION
*
*       This function computes the SKEYID_d, SKEYID_a and SKEYID_e,
*       where:
*
*           SKEYID_d = prf(SKEYID, g^xy | CKY-I | CKY-R | 0)
*           SKEYID_a = prf(SKEYID, SKEYID_d | g^xy | CKY-I | CKY-R | 1)
*           SKEYID_e = prf(SKEYID, SKEYID_a | g^xy | CKY-I | CKY-R | 2)
*
*       Note that the SKEYID buffers MUST be allocated before
*       calling this function, using IKE_SKEYID_Allocate. The
*       SKEYID MUST also be generated before calling this function
*       using the Authentication method dependent calculation.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA containing
*                               a valid SKEYID.
*       *gxy                    The Diffie-Hellman shared key.
*       gxy_len                 Length of Diffie-Hellman key.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_SKEYID_dae(IKE_SA *sa, UINT8 *gxy, UINT16 gxy_len)
{
    STATUS          status;
    VOID            *ctx;
    UINT8           octet = 0;
    UINT8           temp_len = 0;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((sa               == NU_NULL) || (gxy              == NU_NULL) ||
       (sa->ike_skeyid   == NU_NULL) || (sa->ike_skeyid_d == NU_NULL) ||
       (sa->ike_skeyid_a == NU_NULL) || (sa->ike_skeyid_e == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /*
     * Calculate SKEYID_d.
     */

    /* Log debug message. */
    IKE_DEBUG_LOG("Computing SKEYID_d");

    /* Initialize the PRF. */

    status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                           &ctx, sa->ike_skeyid, sa->ike_skeyid_len);

    if(status == NU_SUCCESS)
    {
        /* Add the shared Diffie-Hellman secret to the PRF state. */
        status = IKE_HMAC_Update(ctx, gxy, gxy_len);

        if(status == NU_SUCCESS)
        {
            /* Add Initiator and Responder cookies to the PRF state. */
            status = IKE_HMAC_Update(ctx, sa->ike_cookies,
                                     IKE_COOKIE_LEN + IKE_COOKIE_LEN);

            if(status == NU_SUCCESS)
            {
                /* Add octet 0 to the PRF state. */
                status = IKE_HMAC_Update(ctx, &octet, (UINT16)sizeof(octet));

                if(status == NU_SUCCESS)
                {
                    /* Finalize the PRF for SKEYID_d with actual
                     * digest length.
                     */
                    status = IKE_HMAC_Final(ctx, sa->ike_skeyid_d, &temp_len);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log(
                            "Failed to finalize PRF context",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to update PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to initialize PRF context",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /*
     * Calculate SKEYID_a.
     */

    if(status == NU_SUCCESS)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Computing SKEYID_a");

        /* Initialize the PRF. */
        status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                               &ctx, sa->ike_skeyid, sa->ike_skeyid_len);

        if(status == NU_SUCCESS)
        {
            /* Add SKEYID_d to the PRF state. */
            status = IKE_HMAC_Update(ctx, sa->ike_skeyid_d, sa->ike_skeyid_len);

            if(status == NU_SUCCESS)
            {
                /* Add the shared Diffie-Hellman secret to PRF state. */
                status = IKE_HMAC_Update(ctx, gxy, gxy_len);

                if(status == NU_SUCCESS)
                {
                    /* Add Initiator and Responder cookies to PRF state. */
                    status = IKE_HMAC_Update(ctx, sa->ike_cookies,
                                             IKE_COOKIE_LEN + IKE_COOKIE_LEN);

                    if(status == NU_SUCCESS)
                    {
                        /* Increment octet value. */
                        octet++;

                        /* Add octet 1 to the PRF state. */
                        status = IKE_HMAC_Update(ctx, &octet, (UINT16)sizeof(octet));

                        if(status == NU_SUCCESS)
                        {
                            /* Finalize PRF with actual digest length. */
                            status = IKE_HMAC_Final(ctx, sa->ike_skeyid_a, &temp_len);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to finalize PRF context",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log(
                                "Failed to update PRF context",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to update PRF context",
                             NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to initialize PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /*
     * Calculate SKEYID_e.
     */

    if(status == NU_SUCCESS)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Computing SKEYID_e");

        /* Initialize the PRF. */
        status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                               &ctx, sa->ike_skeyid, sa->ike_skeyid_len);

        if(status == NU_SUCCESS)
        {
            /* Add SKEYID_a to the PRF state. */
            status = IKE_HMAC_Update(ctx, sa->ike_skeyid_a, sa->ike_skeyid_len);

            if(status == NU_SUCCESS)
            {
                /* Add the shared Diffie-Hellman secret to PRF state. */
                status = IKE_HMAC_Update(ctx, gxy, gxy_len);

                if(status == NU_SUCCESS)
                {
                    /* Add Initiator and Responder cookies to PRF state. */
                    status = IKE_HMAC_Update(ctx, sa->ike_cookies,
                                             IKE_COOKIE_LEN + IKE_COOKIE_LEN);

                    if(status == NU_SUCCESS)
                    {
                        /* Increment octet value. */
                        octet++;

                        /* Add octet 2 to the PRF state. */
                        status = IKE_HMAC_Update(ctx, &octet, sizeof(octet));

                        if(status == NU_SUCCESS)
                        {
                            /* Finalize PRF with actual digest length. */
                            status = IKE_HMAC_Final(ctx, sa->ike_skeyid_e, &temp_len);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to finalize PRF context",
                                               NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to update PRF context",
                                           NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to update PRF context",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to initialize PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_SKEYID_dae */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Hash_x
*
* DESCRIPTION
*
*       This is a utility function used to compute HASH(1),
*       HASH(2) or HASH(3) in Phase 2. These are defined as:
*
*       HASH(1) = prf(SKEYID_a, M-ID | msg-from-SA-onwards)
*       HASH(2) = prf(SKEYID_a, M-ID | Ni_b | msg-from-SA-onwards)
*       HASH(3) = prf(SKEYID_a, M-ID | Ni_b | Nr_b)
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       *msg                    Raw IKE message, including
*                               the ISAKMP header. Not
*                               required for HASH(3).
*       msg_len                 Length of the IKE message.
*                               Not required for HASH(3).
*       *dgst                   Buffer for storing the digest.
*       *dgst_len               Contains the length of the
*                               digest buffer and on
*                               return, it contains the
*                               length of the digest.
*       hash_type               This should be one of
*                               IKE_HASH_1, IKE_HASH_2 or
*                               IKE_HASH_3.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Hash_x(IKE_PHASE2_HANDLE *ph2, UINT8 *msg, UINT16 msg_len,
                  UINT8 *dgst, UINT8 *dgst_len, UINT8 hash_type)
{
    STATUS          status;
    VOID            *prf_ctx;
    UINT16          msg_offset;
    UINT8           octet = 0;
    IKE_SA          *sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((ph2 == NU_NULL) || (dgst == NU_NULL) || (dgst_len == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure hash_type is valid. */
    else if((hash_type != IKE_HASH_1) && (hash_type != IKE_HASH_2) &&
            (hash_type != IKE_HASH_3))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* If Hash type is not HASH(3), then the message must be present. */
    else if((hash_type != IKE_HASH_3) &&
            ((msg      == NU_NULL) || (msg_len == 0)))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set pointer to the IKE SA. */
    sa = ph2->ike_sa;

    /* Initialize the PRF function. */

    status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                           &prf_ctx, sa->ike_skeyid_a, sa->ike_skeyid_len);

    if(status == NU_SUCCESS)
    {
        /* If Hash type is HASH(3). */
        if(hash_type == IKE_HASH_3)
        {
            /* Add a zero octet to PRF state. */
            status = IKE_HMAC_Update(prf_ctx, &octet, sizeof(octet));
        }

        if(status == NU_SUCCESS)
        {
            /* Add Phase 2 Message ID to the digest state. */
            status = IKE_HMAC_Update(prf_ctx, (UINT8*)&ph2->ike_msg_id, sizeof(ph2->ike_msg_id));

            if(status == NU_SUCCESS)
            {
                /* If Hash type is not HASH(1). */
                if(hash_type != IKE_HASH_1)
                {
                    /* Add Ni_b to the PRF state. */
                    status = IKE_HMAC_Update(prf_ctx, ph2->ike_nonce_i, ph2->ike_nonce_i_len);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to update PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        if(status == NU_SUCCESS)
        {
            /* If Hash type is not HASH(3). */
            if(hash_type != IKE_HASH_3)
            {
                /* Get hash digest length in octet. */
                octet = IKE_HMAC_Size(prf_ctx);

                if(octet > 0)
                {
                    /* Calculate the message offset till after the
                     * HASH payload.
                     */
                    msg_offset = (IKE_HDR_LEN + IKE_MIN_HASH_PAYLOAD_LEN +
                                  octet);

                    /* Add the Phase 2 message, excluding the ISAKMP
                     * header and the Hash payload, to the PRF state.
                     */
                    status = IKE_HMAC_Update(prf_ctx, msg + (INT)msg_offset, msg_len - msg_offset);
                }

                else
                {
                    NLOG_Error_Log("Failed to get hash digest length",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                /* Add Nr_b to the PRF state. */
                status = IKE_HMAC_Update(prf_ctx, ph2->ike_nonce_r, ph2->ike_nonce_r_len);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to update PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        if(status == NU_SUCCESS)
        {
            /* Finalize the PRF digest. */
            status = IKE_HMAC_Final(prf_ctx, dgst, dgst_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to finalize PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to update PRF context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to initialize PRF context",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Hash_x */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Verify_Hash_x
*
* DESCRIPTION
*
*       This function verifies a Phase 2 message HASH(1),
*       HASH(2) or HASH(3), depending on the current state
*       of the caller.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*       *msg                    The raw IKE message.
*       msg_len                 Length of the IKE message.
*       *dgst                   The digest being verified.
*       dgst_len                Length of digest, in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_VERIFY_FAILED       Hash verification failed.
*
*************************************************************************/
STATUS IKE_Verify_Hash_x(IKE_PHASE2_HANDLE *ph2, UINT8 *msg,
                         UINT16 msg_len, UINT8 *dgst, UINT8 dgst_len)
{
    STATUS          status;
    UINT8           msg_dgst[IKE_MAX_HASH_DATA_LEN];
    UINT8           msg_dgst_len = dgst_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((ph2 == NU_NULL) || (dgst == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure message is present if HASH(3) is not being calculated. */
    else if((ph2->ike_xchg_state != IKE_RECV_HASH3_STATE) &&
            ((msg == NU_NULL) || (msg_len == 0)))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Determine the current state of IKE State Machine. */
    switch(ph2->ike_xchg_state)
    {
    case IKE_RECV_HASH1_STATE:
    case IKE_RECV_HASH4_STATE:
        /* Log debug message. */
        IKE_DEBUG_LOG("Calculating HASH(1)");

        /* Calculate HASH(1) (same as HASH(4)). */
        status = IKE_Hash_x(ph2, msg, msg_len, msg_dgst,
                            &msg_dgst_len, IKE_HASH_1);
        break;

    case IKE_RECV_HASH2_STATE:
        /* Log debug message. */
        IKE_DEBUG_LOG("Calculating HASH(2)");

        /* Calculate HASH(2). */
        status = IKE_Hash_x(ph2, msg, msg_len, msg_dgst,
                            &msg_dgst_len, IKE_HASH_2);
        break;

    case IKE_RECV_HASH3_STATE:
        /* Log debug message. */
        IKE_DEBUG_LOG("Calculating HASH(3)");

        /* Calculate HASH(3). */
        status = IKE_Hash_x(ph2, msg, msg_len, msg_dgst,
                            &msg_dgst_len, IKE_HASH_3);
        break;

    default:
        /* Unexpected Phase 2 state. */
        status = IKE_INVALID_PARAMS;
        break;
    }

    /* If HASH(x) was successfully Calculated. */
    if(status == NU_SUCCESS)
    {
        /* Make sure digest length is same. */
        if(msg_dgst_len != dgst_len)
        {
            /* Hash mismatch. */
            status = IKE_VERIFY_FAILED;
        }

        else if(memcmp(msg_dgst, dgst, msg_dgst_len) != 0)
        {
            /* Hash mismatch. */
            status = IKE_VERIFY_FAILED;
        }

        else
        {
            /* Log debug message. */
            IKE_DEBUG_LOG("HASH(x) verification successful");
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Verify_Hash_x */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Phase1_Encryption_Material
*
* DESCRIPTION
*
*       This is a utility function used to compute the
*       Encryption Key and Initialization Vector for a
*       Phase 1 SA. This must be called after SKEYID_e
*       has been calculated. It is usually called from
*       IKE_Phase1_Key_Material.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       *dhpub_i                Initiator's Public Diffie-Hellman key.
*       dhpub_i_len             Length of Public Diffie-Hellman key.
*       *dhpub_r                Responder's Public Diffie-Hellman key.
*       dhpub_r_len             Length of Public Diffie-Hellman key.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Phase1_Encryption_Material(IKE_SA *sa, UINT8 *dhpub_i,
                                      UINT16 dhpub_i_len,
                                      UINT8 *dhpub_r,
                                      UINT16 dhpub_r_len)
{
    STATUS              status;
    UINT8 HUGE          *buffer;
    UINT16              key_len;
    UINT8               iv_len = 0;
    UINT16              i;
    UINT8               dgst_len;
    UINT8               req_dgst_len;
    UINT8               octet = 0;
    VOID                *prf_ctx;
    EVP_MD_CTX          hash_ctx;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((sa      == NU_NULL) || (dhpub_i == NU_NULL) ||
       (dhpub_r == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set key length. */
    key_len = sa->ike_attributes.ike_key_len;

    /* Allocate memory for the encryption key and both IVs. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&buffer,
                                key_len, NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        buffer = TLS_Normalize_Ptr(buffer);

        /* Set the encryption key buffer in the SA. */
        sa->ike_encryption_key = (UINT8*)buffer;

        /*
         * Compute the encryption key.
         */

        /* If SKEYID_e is large enough for key length. */
        if((UINT16)sa->ike_skeyid_len >= key_len)
        {
            /* Set SKEYID_e as the encryption key. */
            NU_BLOCK_COPY(sa->ike_encryption_key,
                          sa->ike_skeyid_e, key_len);
        }

        else
        {
            /* Expand SKEYID_e using:
             *      Ka = K1 | K2 | K3 | ...
             * Where:
             *      K1 = prf(SKEYID_e, 0)
             *      K2 = prf(SKEYID_e, K1)
             *      K3 = prf(SKEYID_e, K2)
             */

            /* Set digest length to zero. */
            dgst_len = 0;

            /* Initialize the PRF state to compute K1. */

            status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                   &prf_ctx, sa->ike_skeyid_e, sa->ike_skeyid_len);

            if(status == NU_SUCCESS)
            {
                /* Update the PRF state. */
                status = IKE_HMAC_Update(prf_ctx, &octet, sizeof(octet));

                if(status == NU_SUCCESS)
                {
                    /* Get value of K1. */
                    status = IKE_HMAC_Final(prf_ctx, (UINT8*)buffer, &dgst_len);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to finalize PRF context",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to initialize PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* If K1 has been generated successfully. */
            if(status == NU_SUCCESS)
            {
                /* Set requested digest length. */
                req_dgst_len = dgst_len;

                /* Loop for K2, K3, ... */
                for(i = dgst_len;
                    ((status == NU_SUCCESS) && (i < key_len));
                    i = i + dgst_len)
                {
                    /* Initialize the PRF. */
                    status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                           &prf_ctx, sa->ike_skeyid_e, sa->ike_skeyid_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Add previous 'Kn' to the state. */
                        status = IKE_HMAC_Update(prf_ctx, (UINT8*)buffer, dgst_len);

                        if(status == NU_SUCCESS)
                        {
                            /* Increment destination buffer. */
                            buffer = buffer + dgst_len;

                            /* If number of bytes required are less
                             * than the digest length.
                             */
                            if(key_len - i < (UINT16)dgst_len)
                            {
                                /* Set requested digest length. */
                                req_dgst_len = (UINT8)(key_len - i);
                            }

                            /* Finalize the PRF digest. */
                            status = IKE_HMAC_Final(prf_ctx, (UINT8*)buffer, &req_dgst_len);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to finalize PRF context",
                                               NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to update PRF context",
                                           NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to initialize PRF context",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

        }

        /* If no error occurred. */
        if(status == NU_SUCCESS)
        {
            /*
             * Compute the encryption/decryption IV.
             */

            /* Initialize the Hash function. */
            status = IKE_Hash_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                   &hash_ctx);

            if(status == NU_SUCCESS)
            {
                /* Add Diffie-Hellman public key to Hash state. */
                status = IKE_Hash_Update(&hash_ctx, dhpub_i, dhpub_i_len);

                if(status == NU_SUCCESS)
                {
                    /* Add Diffie-Hellman public key to Hash state. */
                    status = IKE_Hash_Update(&hash_ctx, dhpub_r, dhpub_r_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Finalize the Hash digest (encryption IV). */
                        status = IKE_Hash_Final(&hash_ctx, sa->ike_encryption_iv, &iv_len);

                        if(status == NU_SUCCESS)
                        {
                            /* Copy the value to the decryption IV. */
                            NU_BLOCK_COPY(sa->ike_decryption_iv,
                                          sa->ike_encryption_iv,
                                          iv_len);
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to finalize Hash context",
                                           NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to update Hash context",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update Hash context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to initialize Hash context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for key and IVs",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Phase1_Encryption_Material */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Phase1_Key_Material
*
* DESCRIPTION
*
*       This function computes all the key material for a Phase 1
*       IKE Key Exchange. It must be called only after the IKE Key
*       Exchange payloads have been exchanged.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Phase1_Key_Material(IKE_PHASE1_HANDLE *ph1)
{
    STATUS              status;
    IKE_KEY_PAIR        dh_keys;
    UINT8               *key;
    INT                 key_len;
    UINT8               *nonce_i;
    UINT16              nonce_i_len;
    UINT8               *nonce_r;
    UINT16              nonce_r_len;
    UINT8               *dhpub_i;
    UINT16              dhpub_i_len;
    UINT8               *dhpub_r;
    UINT16              dhpub_r_len;
    IKE_SA              *sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the Handle pointer is valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Check that the local Diffie-Hellman key pair is valid. */
    else if((ph1->ike_dh_key.ike_public_key  == NU_NULL) ||
            (ph1->ike_dh_key.ike_private_key == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the remote Diffie-Hellman public key is valid. */
    else if((ph1->ike_dh_remote_key     == NU_NULL) ||
            (ph1->ike_dh_remote_key_len == 0))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating Phase 1 key material");

    /* Set local pointer to IKE SA in the Handle. */
    sa = ph1->ike_sa;

    /* Determine whether we are the Initiator or Responder. */
    if((ph1->ike_flags & IKE_INITIATOR) != 0)
    {
        /* Local Nonce data is Initiator's and remote is Responder's. */
        nonce_i     = ph1->ike_nonce_data;
        nonce_i_len = IKE_OUTBOUND_NONCE_DATA_LEN;
        nonce_r     = ph1->ike_params->ike_in.ike_nonce->ike_nonce_data;
        nonce_r_len =
            ph1->ike_params->ike_in.ike_nonce->ike_nonce_data_len;

        /* Local Diffie-Hellman key is Initiator's
         * and remote is Responder's.
         */
        dhpub_i     = ph1->ike_dh_key.ike_public_key;
        dhpub_i_len = ph1->ike_dh_key.ike_public_key_len;
        dhpub_r     = ph1->ike_dh_remote_key;
        dhpub_r_len = ph1->ike_dh_remote_key_len;
    }

    else
    {
        /* Local Nonce data is Responder's and remote is Initiator's. */
        nonce_i     = ph1->ike_params->ike_in.ike_nonce->ike_nonce_data;
        nonce_i_len =
            ph1->ike_params->ike_in.ike_nonce->ike_nonce_data_len;
        nonce_r     = ph1->ike_nonce_data;
        nonce_r_len = IKE_OUTBOUND_NONCE_DATA_LEN;

        /* Local Diffie-Hellman key is Responder's
         * and remote is Initiator's.
         */
        dhpub_i     = ph1->ike_dh_remote_key;
        dhpub_i_len = ph1->ike_dh_remote_key_len;
        dhpub_r     = ph1->ike_dh_key.ike_public_key;
        dhpub_r_len = ph1->ike_dh_key.ike_public_key_len;
    }

    /* Fill in the Diffie-Hellman key pair.
     * This key pair was generated in the previous
     * state when the Key Exchange data was sent.
     */
    dh_keys.ike_public_key      = ph1->ike_dh_key.ike_public_key;
    dh_keys.ike_public_key_len  = ph1->ike_dh_key.ike_public_key_len;
    dh_keys.ike_private_key     = ph1->ike_dh_key.ike_private_key;
    dh_keys.ike_private_key_len = ph1->ike_dh_key.ike_private_key_len;

    /* Compute the Diffie-Hellman shared secret. */
    status = IKE_DH_Compute_Key(IKE_Oakley_Group_Prime(sa->ike_attributes.ike_group_desc),
                                IKE_Oakley_Group_Length(sa->ike_attributes.ike_group_desc),
                                IKE_OAKLEY_GROUP_GEN(sa->ike_attributes.ike_group_desc),
                                &dh_keys, ph1->ike_dh_remote_key, ph1->ike_dh_remote_key_len,
                                &key, &key_len, NU_NULL, 0);

    if(status == NU_SUCCESS)
    {
        /* Allocate buffers for the key material. */
        status = IKE_SKEYID_Allocate(sa);

        if(status == NU_SUCCESS)
        {
#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
            /* If authentication method based on pre-shared keys. */
            if(IKE_IS_PSK_METHOD(sa->ike_attributes.ike_auth_method))
            {
                /* Compute SKEYID. Note that the pre-shared
                 * key was looked-up earlier and should now
                 * be present in the IKE SA.
                 */
                status = IKE_SKEYID_Preshared_Key(sa, nonce_i, nonce_i_len,
                                                  nonce_r, nonce_r_len);
            }

            else
#endif
#if (IKE_INCLUDE_SIG_AUTH == NU_TRUE)
            /* Otherwise, if authentication method based on signatures. */
            if(IKE_IS_SIGN_METHOD(sa->ike_attributes.ike_auth_method))
            {
                /* Compute SKEYID for Signatures. */
                status = IKE_SKEYID_Signatures(sa, key, key_len,
                                               nonce_i, nonce_i_len,
                                               nonce_r, nonce_r_len);
            }

            else
#endif
            {
                /* Authentication method is unsupported. */
                status = IKE_UNSUPPORTED_METHOD;
            }

            /* If SKEYID computed successfully. */
            if(status == NU_SUCCESS)
            {
                /* Compute SKEYID_d, SKEYID_a, SKEYID_e. */
                status = IKE_SKEYID_dae(sa, key, key_len);

                if(status == NU_SUCCESS)
                {
                    /* Calculate encryption key and IV. */
                    status = IKE_Phase1_Encryption_Material(sa,
                                 dhpub_i, dhpub_i_len, dhpub_r,
                                 dhpub_r_len);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to allocate SKEYID memory",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Free the shared key. */
        if(NU_Deallocate_Memory(key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to free DH shared key",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to compute Diffie-Hellman shared secret",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Phase1_Key_Material */

/*************************************************************************
*
* FUNCTION
*
*       IKE_KEYMAT_Allocate
*
* DESCRIPTION
*
*       This function computes the required length of the
*       key material and allocates a single buffer for
*       the specified SA2 item. Length of the key material
*       is set within the SA2.
*
* INPUTS
*
*       *sa2                    Pointer to IKE SA2 for which
*                               key material is to be generated.
*                               On return, contains dynamically
*                               allocated buffer for storing the
*                               local and remote key material.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_KEYMAT_Allocate(IKE_SA2 *sa2)
{
    STATUS          status;
    UINT16          keymat_len;
    UINT16          auth_key_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the SA2 pointer is valid. */
    if(sa2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Allocating memory for Phase 2 key material");

    /* Get the authentication and encryption key lengths. */
    status = IKE_IPS_Get_Key_Length(&sa2->ike_ips_security,
                                    &keymat_len, &auth_key_len);

    if(status == NU_SUCCESS)
    {
        /* keymat_len contains encryption algorithm's
         * key length. Add authentication algorithm's
         * key length to this value.
         */
        keymat_len = keymat_len + auth_key_len;

        /* Make sure the key length is greater than zero. */
        if(keymat_len == 0)
        {
            /* Report error. */
            status = IKE_INVALID_PARAMS;
        }

        else
        {
            /* Allocate memory for both local and remote key
             * material in a single memory request.
             */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                         (VOID**)&sa2->ike_local_keymat,
                         keymat_len + keymat_len, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Normalize the pointer. */
                sa2->ike_local_keymat =
                    TLS_Normalize_Ptr(sa2->ike_local_keymat);

                /* Set the remote key material buffer pointer. */
                sa2->ike_remote_keymat = sa2->ike_local_keymat +
                                         (INT)keymat_len;

                /* Also set the length of the key material. */
                sa2->ike_keymat_len    = keymat_len;
            }

            else
            {
                NLOG_Error_Log("Failed to allocate key material memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get algorithm key lengths",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_KEYMAT_Allocate */

/*************************************************************************
*
* FUNCTION
*
*       IKE_KEYMAT_x
*
* DESCRIPTION
*
*       This function computes the IPsec SA key material for
*       a single SPI. Buffer for storing the key material should
*       be provided by the caller and must be large enough to
*       store the requested amount of key material. Length of the
*       key material buffers in the SA2 determine the requested
*       key material length. The material generated is:
*
*       KEYMAT = prf(SKEYID_d, protocol | SPI | Ni_b | Nr_b)
*
*       Key material for PFS is as follows:
*
*       KEYMAT = prf(SKEYID_d, g(qm)^xy | protocol | SPI | Ni_b | Nr_b)
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle
*                               containing Nonce data and the
*                               Diffie-Hellman shared secret, if
*                               PFS is used.
*       *sa2                    Pointer to the IKE SA2 item for
*                               which the key material is to be
*                               generated.
*       *dh_gxy                 Diffie-Hellman shared secret,
*                               required only if using PFS.
*                               Otherwise must be set to NU_NULL.
*       dh_gxy_len              Length of Diffie-Hellman shared
*                               secret.
*       side                    Key material generated for local
*                               SPI if this is IKE_LOCAL and for
*                               remote SPI if this is IKE_REMOTE.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_KEYMAT_x(IKE_PHASE2_HANDLE *ph2, IKE_SA2 *sa2,
                    UINT8 *dh_gxy, UINT16 dh_gxy_len, UINT8 side)
{
    STATUS          status;
    UINT16          i;
    UINT32          spi;
    UINT8           proto;
    UINT8           dgst_len;
    UINT8           req_dgst_len;
    UINT8 HUGE      *buffer;
    VOID            *prf_ctx;
    IKE_SA          *sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((ph2 == NU_NULL) || (ph2->ike_sa == NU_NULL) || (sa2 == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure required key material length is not zero. */
    if(sa2->ike_keymat_len == 0)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure g(qm)^xy is present if PFS is being used. */
    if((ph2->ike_group_desc != IKE_GROUP_NONE) && (dh_gxy == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the 'side' parameter is valid. */
    if((side != IKE_LOCAL) && (side != IKE_REMOTE))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set local pointer to IKE SA in the Handle. */
    sa = ph2->ike_sa;

    /* Determine whether SPI is local or remote. */
    if(side == IKE_LOCAL)
    {
        /* Set SPI to the local SPI in SA2. */
        spi = GET32(&sa2->ike_local_spi, 0);

        /* Set destination buffer to local key material pointer. */
        buffer = sa2->ike_local_keymat;
    }

    else
    {
        /* Set SPI to the remote SPI in SA2. */
        spi = GET32(&sa2->ike_remote_spi, 0);

        /* Set destination buffer to remote key material pointer. */
        buffer = sa2->ike_remote_keymat;
    }

    /* Convert IPsec specific protocol value to IKE value. */
    proto = IKE_Protocol_ID_IPS_To_IKE(
                sa2->ike_ips_security.ipsec_protocol);

    /* Get hash digest length. */

    status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                   &dgst_len);

    if(status == NU_SUCCESS)
    {
        /* Set requested digest length to actual digest length. */
        req_dgst_len = dgst_len;

        /* Loop until the required amount of key material is generated. */
        for(i = 0; i < sa2->ike_keymat_len; i = i + dgst_len)
        {
            /* Initialize the PRF function. */

            status = IKE_HMAC_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                   &prf_ctx, sa->ike_skeyid_d, sa->ike_skeyid_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to initialize PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }

            /* If this is not the first iteration of the loop. */
            if(i != 0)
            {
                /* Expand KEYMAT by adding previous PRF digest to it. */
                status = IKE_HMAC_Update(prf_ctx, (UINT8*)buffer, dgst_len);

                if(status == NU_SUCCESS)
                {
                    /* Increment buffer for next sequence of the
                     * key material.
                     */
                    buffer = buffer + dgst_len;
                }

                else
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }
            }

            /* If PFS is being used. */
            if(ph2->ike_group_desc != IKE_GROUP_NONE)
            {
                /* Add the Diffie-Hellman shared secret to PRF state. */
                status = IKE_HMAC_Update(prf_ctx, dh_gxy, dh_gxy_len);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to update PRF context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    break;
                }
            }

            /* Add IPsec protocol to PRF state. */
            status = IKE_HMAC_Update(prf_ctx, &proto, sizeof(proto));

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }

            /* Add the SPI to PRF state. */
            status = IKE_HMAC_Update(prf_ctx, (UINT8*)&spi, sizeof(spi));

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }

            /* Add Initiator's Nonce data to PRF state. */
            status = IKE_HMAC_Update(prf_ctx, ph2->ike_nonce_i, ph2->ike_nonce_i_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }

            /* Add Responder's Nonce data to PRF state. */
            status = IKE_HMAC_Update(prf_ctx, ph2->ike_nonce_r, ph2->ike_nonce_r_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to update PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }

            /* If number of bytes left is less than digest length. */
            if(sa2->ike_keymat_len - i < (UINT16)dgst_len)
            {
                /* Set requested digest length to number of bytes left. */
                req_dgst_len = (UINT8)(sa2->ike_keymat_len - i);
            }

            /* Finalize the PRF digest. */
            status = IKE_HMAC_Final(prf_ctx, (UINT8*)buffer, &req_dgst_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to finalize PRF context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get hash digest length",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_KEYMAT_x */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Phase2_Key_Material
*
* DESCRIPTION
*
*       This function computes the IPsec SA key material after
*       all SA parameters have been negotiated. Both inbound
*       and outbound key material for each SA2 item is
*       returned in a dynamically allocated buffer.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle
*                               containing the negotiated
*                               IPsec SA parameters in SA2 items.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   IPsec algorithm ID not valid in
*                               the negotiated algorithms.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Phase2_Key_Material(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status = NU_SUCCESS;
    IKE_SA2             *sa2;
    IKE_KEY_PAIR        dh_keys;
    UINT8               *key;
    INT                 key_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if((ph2 == NU_NULL) || (ph2->ike_sa == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the remote Diffie-Hellman public key is
     * present in the Handle, if PFS is being used.
     */
    if((ph2->ike_group_desc != IKE_GROUP_NONE) &&
       (ph2->ike_dh_remote_key == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Generating Phase 2 key material");

    /* If PFS is not being used. */
    if(ph2->ike_group_desc == IKE_GROUP_NONE)
    {
        /* Initialize the Diffie-Hellman compute request to
         * sane values, as they will be passed to another
         * function below.
         */
        key = NU_NULL;
        key_len = 0;
    }

    else
    {
        /* Fill in the Diffie-Hellman key pair.
         * This key pair should be generated before
         * calling this function.
         */
        dh_keys.ike_public_key      = ph2->ike_dh_key.ike_public_key;
        dh_keys.ike_public_key_len  = ph2->ike_dh_key.ike_public_key_len;
        dh_keys.ike_private_key     = ph2->ike_dh_key.ike_private_key;
        dh_keys.ike_private_key_len = ph2->ike_dh_key.ike_private_key_len;

        /* Compute the Diffie-Hellman shared secret. */
        status = IKE_DH_Compute_Key(IKE_Oakley_Group_Prime(ph2->ike_group_desc),
                                    IKE_Oakley_Group_Length(ph2->ike_group_desc),
                                    IKE_OAKLEY_GROUP_GEN(ph2->ike_group_desc),
                                    &dh_keys, ph2->ike_dh_remote_key, ph2->ike_dh_remote_key_len,
                                    &key, &key_len, NU_NULL, 0);
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Set the SA2 pointer to the first item in the DB. */
        sa2 = ph2->ike_sa2_db.ike_flink;

        /* Loop for each SA2 item in the DB. */
        while((sa2 != NU_NULL) && (status == NU_SUCCESS))
        {
            /* Allocate buffers for key material. */
            status = IKE_KEYMAT_Allocate(sa2);

            if(status == NU_SUCCESS)
            {
                /* Calculate the KEYMAT for local SPI. */
                status = IKE_KEYMAT_x(ph2, sa2, key, key_len, IKE_LOCAL);

                if(status == NU_SUCCESS)
                {
                    /* Calculate the KEYMAT for remote SPI. */
                    status = IKE_KEYMAT_x(ph2, sa2, key, key_len, IKE_REMOTE);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log(
                            "Failed to compute remote SPI KEYMAT",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to compute local SPI KEYMAT",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to allocate key material memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Move to the next SA2 item. */
            sa2 = sa2->ike_flink;
        }

        /* If Diffie-Hellman was computed. */
        if(ph2->ike_group_desc != IKE_GROUP_NONE)
        {
            /* Free the shared key. */
            if(NU_Deallocate_Memory(key) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate DH shared key",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Phase2_Key_Material */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Phase2_IV
*
* DESCRIPTION
*
*       This is a utility function used to compute an
*       Initialization Vector (IV) for a new Phase 2
*       Exchange.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Phase2_IV(IKE_PHASE2_HANDLE *ph2)
{
    STATUS          status;
    EVP_MD_CTX      hash_ctx;
    UINT8           iv_len;
    IKE_SA          *sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((ph2 == NU_NULL) || (ph2->ike_sa == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Calculating encryption IV for Phase 2");

    /* Set local pointer to IKE SA in the Handle. */
    sa = ph2->ike_sa;

    /* Select length of IV based on algorithm type. */
    status = IKE_Crypto_IV_Len(IKE_Encryption_Algos[sa->ike_attributes.ike_encryption_algo].crypto_algo_id,
                               &iv_len);

    if(status == NU_SUCCESS)
    {
        /* Initialize the Hash function. */
        status = IKE_Hash_Init(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                               &hash_ctx);

        if(status == NU_SUCCESS)
        {
            /* Add last IV of the Phase 1 Exchange to the digest state. */
            status = IKE_Hash_Update(&hash_ctx, sa->ike_encryption_iv, iv_len);

            if(status == NU_SUCCESS)
            {
                /* Add Phase 2 Message ID to the digest state. */
                status = IKE_Hash_Update(&hash_ctx,
                                         (UINT8*)&ph2->ike_msg_id,
                                         sizeof(ph2->ike_msg_id));

                if(status == NU_SUCCESS)
                {
                    /* Finalize the Hash digest (encryption IV). */
                    status = IKE_Hash_Final(&hash_ctx,
                                            ph2->ike_encryption_iv,
                                            &iv_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Copy the same value to the decryption IV. */
                        NU_BLOCK_COPY(ph2->ike_decryption_iv,
                                      ph2->ike_encryption_iv, iv_len);
                    }

                    else
                    {
                        NLOG_Error_Log(
                            "Failed to finalize PRF context",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to update Hash context",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to update Hash context",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to initialize Hash context",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get IV length",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Phase2_IV */

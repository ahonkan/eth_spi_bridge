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
*       ike2_keymat.c
*
* COMPONENT
*
*       IKEv2 - Key Generation
*
* DESCRIPTION
*
*       This file contains functions for generation of keying material
*       for IKEv2 and IPsec SA's.
*
* FUNCTIONS
*
*       IKE2_Compute_Keys
*       IKE2_Compute_Needed_Len
*       IKE2_Compute_SKEYSEED
*       IKE2_Compute_SKEYSEED_Rekey
*       IKE2_Generate_CHILD_SA_KEYMAT
*       IKE2_Generate_KEYMAT
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike_ips.h
*       ike_crypto_wrappers.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_ips.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/* Local functions. */
STATIC STATUS IKE2_Compute_SKEYSEED(IKE2_SA *sa, UINT8 *nonce_i,
                                    UINT16 nonce_i_len, UINT8 *nonce_r,
                                    UINT16 nonce_r_len, UINT8 *g_ir,
                                    UINT16 g_ir_len);
STATIC STATUS IKE2_Compute_SKEYSEED_Rekey(IKE2_SA *sa, UINT8 *nonce_i,
                                          UINT16 nonce_i_len, UINT8 *nonce_r,
                                          UINT16 nonce_r_len, UINT8 *g_ir,
                                          UINT16 g_ir_len);
STATIC STATUS IKE2_Compute_Keys(IKE2_SA *sa, UINT8 *nonce_i, UINT8 *nonce_r,
                                UINT16 nonce_i_len, UINT16 nonce_r_len,
                                UINT8 flags);

STATIC STATUS IKE2_Compute_Needed_Len(IKE2_IPS_SA *ips_sa, UINT16 *length);

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_KEYMAT
*
* DESCRIPTION
*
*       Generates keying material for IKEv2 SA. Keying material is
*       computed as:
*
*       prf+ (K,S) = T1 | T2 | T3 | T4 | ...
*
*       where:
*       T1 = prf (K, S | 0x01)
*       T2 = prf (K, T1 | S | 0x02)
*       T3 = prf (K, T2 | S | 0x03)
*       T4 = prf (K, T3 | S | 0x04)
*
* INPUTS
*
*       *handle                 Handle for the exchange for which material
*                               is to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE2_Generate_KEYMAT(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS      status;
    UINT8       *nonce_i;
    UINT8       *nonce_r;
    UINT16      nonce_i_len;
    UINT16      nonce_r_len;
    UINT8       *g_ir;
    UINT16      g_ir_len;
    IKE_OAKLEY_GROUP_INFO *oakinfo;

    IKE_KEY_PAIR    dh_keys;
    UINT8           *key;
    INT             key_len;
    UINT8           *g_ext;
    INT             g_ext_len;

#if (IKE2_DEBUG == NU_TRUE)
    /* Parameter sanity check. */
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if((handle->ike2_dh_keys.ike_private_key == NU_NULL)
        || (handle->ike2_dh_keys.ike_public_key == NU_NULL)
        || (handle->ike2_remote_dh == NU_NULL)
        || (handle->ike2_remote_dh_len == 0)
        || (handle->ike2_nonce == NU_NULL)
        || (handle->ike2_peer_nonce == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Generating keying material");

    /* Check who is initiator of this exchange. This is used to decide
     * the order of nonces and DH valued in the computations that follow.
     */
    if((handle->ike2_flags & IKE2_INITIATOR) != 0)
    {
        /* We are initiator. */
        nonce_i = handle->ike2_nonce;
        nonce_r = handle->ike2_peer_nonce;
        nonce_i_len = handle->ike2_nonce_len;
        nonce_r_len = handle->ike2_peer_nonce_len;
    }
    else
    {
        /* We are responder. */
        nonce_r = handle->ike2_nonce;
        nonce_i = handle->ike2_peer_nonce;
        nonce_r_len = handle->ike2_nonce_len;
        nonce_i_len = handle->ike2_peer_nonce_len;
    }

    /* Get the pointers to buffers in the exchange handle to directly
     * populate those buffers.
     */
    dh_keys.ike_private_key = handle->ike2_dh_keys.ike_private_key;
    dh_keys.ike_private_key_len = handle->ike2_dh_keys.ike_private_key_len;
    dh_keys.ike_public_key = handle->ike2_dh_keys.ike_public_key;
    dh_keys.ike_public_key_len = handle->ike2_dh_keys.ike_public_key_len;

    /* Get the Oakley group being used. */
    oakinfo = IKE_Get_Oakley_Group(
                    handle->ike2_sa->ike_attributes.ike_group_desc);

    if(oakinfo == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }

    if(oakinfo->ike_generator != IKE_DH_GENERATOR_POS)
    {
        g_ext = NU_NULL;
        g_ext_len = 0;
    }
    else
    {
        g_ext = oakinfo->ike_generator_ext;
        g_ext_len = oakinfo->ike_generator_ext_len;
    }

    /* Computer the DH values. */
    status = IKE_DH_Compute_Key(oakinfo->ike_prime,
                                oakinfo->ike_prime_len,
                                oakinfo->ike_generator,
                                &dh_keys, handle->ike2_remote_dh, handle->ike2_remote_dh_len,
                                &key, &key_len, g_ext, g_ext_len);

    if(status == NU_SUCCESS)
    {
        /* Get the g^ir value for DH and its length. */
        g_ir = key;
        g_ir_len = key_len;

        /* SKEYSEED is computed differently for new SA's and SA's that are
         * being re-keyed. A flag is set in the exchange handle to indicate
         * which request is being processed.
         */
        if((handle->ike2_flags & IKE2_SA_REKEY) == 0)
        {
            /* This is a new SA being created. Proceed normally. */
            status = IKE2_Compute_SKEYSEED(handle->ike2_sa, nonce_i,
                                           nonce_i_len, nonce_r, nonce_r_len,
                                           g_ir, g_ir_len);

            if(status == NU_SUCCESS)
            {
                /* Now all is done. We have generated the seed value for
                * deriving keys. Compute actual keys now.
                */
                status = IKE2_Compute_Keys(handle->ike2_sa, nonce_i, nonce_r,
                    nonce_i_len, nonce_r_len, handle->ike2_flags);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to compute keys for the SA.",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
        else
        {
            /* An SA is being re-keyed. SKEYSEED will be calculated
             * differently.
             */

            handle->ike2_sa->ike_skeyid_len = IKE2_PRF_HMAC_Algos[
                handle->ike2_sa->ike_attributes.ike2_prf_algo].ike2_key_len;

            /* Allocate memory for SK_D in new SA to be copied from old SA. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                        (VOID**)&handle->ike2_new_sa->ike2_sk_d,
                        handle->ike2_sa->ike_skeyid_len, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                NU_BLOCK_COPY(handle->ike2_new_sa->ike2_sk_d,
                              handle->ike2_sa->ike2_sk_d,
                              handle->ike2_sa->ike_skeyid_len);

                handle->ike2_new_sa->ike_skeyid_len =
                    handle->ike2_sa->ike_skeyid_len;

                /* Now compute SKEYSEED. */
                if((handle->ike2_params->ike2_in.ike2_hdr->ike2_flags) == 0 ||
                    ((handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
                      IKE2_HDR_RESPONSE_FLAG) != 0 &&
                     (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
                      IKE2_HDR_INITIATOR_FLAG) != 0))
                {
                    /* Either we are responder and initiating the re-key
                     * or we are initiator and peer has initiated the
                     * re-key for this SA.
                     */
                    status = IKE2_Compute_SKEYSEED_Rekey(
                        handle->ike2_new_sa, nonce_r, nonce_r_len,
                        nonce_i, nonce_i_len, g_ir, g_ir_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Now all is done. We have generated the seed value for
                         * deriving keys. Compute actual keys now.
                         */
                        status = IKE2_Compute_Keys(handle->ike2_new_sa,
                                    nonce_r, nonce_i, nonce_r_len,
                                    nonce_i_len, (((handle->ike2_params->
                                    ike2_in.ike2_hdr->ike2_flags) == 0)
                                    ? IKE2_RESPONDER : IKE2_INITIATOR));

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to compute keys for the SA.",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
                }

                else
                {
                    /* Either we are initiator and initiating the re-key
                     * or we are responder and peer has initiated the
                     * re-key for this SA.
                     */
                    status = IKE2_Compute_SKEYSEED_Rekey(
                        handle->ike2_new_sa, nonce_i, nonce_i_len,
                        nonce_r, nonce_r_len, g_ir, g_ir_len);

                    if(status == NU_SUCCESS)
                    {
                        /* Now all is done. We have generated the seed value for
                         * deriving keys. Compute actual keys now.
                         */
                        status = IKE2_Compute_Keys(handle->ike2_new_sa,
                                    nonce_i, nonce_r, nonce_i_len,
                                    nonce_r_len, handle->ike2_flags);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to compute keys for the SA.",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
                }

                if(status != NU_SUCCESS)
                {
                    /* Failed to complete the key computation. Delete the
                     * memory allocated for sk_d.
                     */
                    if((handle->ike2_new_sa->ike2_sk_d != NU_NULL) &&
                        (NU_Deallocate_Memory(handle->ike2_new_sa->ike2_sk_d)
                         != NU_SUCCESS))
                    {
                        NLOG_Error_Log("Failed to deallocate memory",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for SK_D while re-keying",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* The memory assigned to 'key' is dynamically allocated and
         * should be freed here. */
        if(key != NU_NULL)
        {
            if(NU_Deallocate_Memory(key) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate DH key memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to compute the DH values.",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
} /* IKE2_Generate_KEYMAT */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Compute_SKEYSEED
*
* DESCRIPTION
*
*       Computes the SKEYSEED for IKE SA. It is computed as:
*       SKEYSEED = prf(Ni | Nr, g^ir)
*
* INPUTS
*
*       *sa                     SA for which Seed is to be generated.
*       *nonce_i                Initiator's nonce.
*       nonce_i_len             Length of initiator's nonce.
*       *nonce_r                Responder's nonce.
*       nonce_r_len             Responder's nonce length.
*       *g_ir                   DH public value.
*       g_ir_len                Length of DH public value
*
* OUTPUTS
*
*       NU_SUCCESS              On successful completion.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC
STATUS IKE2_Compute_SKEYSEED(IKE2_SA *sa, UINT8 *nonce_i,
                             UINT16 nonce_i_len, UINT8 *nonce_r,
                             UINT16 nonce_r_len, UINT8 *g_ir,
                             UINT16 g_ir_len)
{
    STATUS          status;
    VOID            *hmac_ctx;
    UINT8           *hmac_key = NU_NULL;
    UINT8           hmac_key_len;

    /* Following two variables are used to determine how may bytes
     * to use from each nonce. When PRF algorithm uses fixed length key
     * and both nonce lengths do not add up to the needed key length,
     * equal number of bytes should be taken from both the nonces. Total
     * length of two nonces cannot be less than required because each
     * nonce MUST be at least half the size of required key. The only
     * algorithm with fixed length key we support is AES.
     * (RFC4306 section 2.10) */
    UINT16          ni_len;
    UINT16          nr_len;

    hmac_key_len = IKE2_PRF_HMAC_Algos[sa->ike_attributes.ike2_prf_algo].
                        ike2_key_len;

    if((IKE2_PRF_Algos[sa->ike_attributes.ike2_prf_algo].
        ike2_algo_identifier == IKE2_PRF_AES128_XCBC) &&
        (nonce_i_len + nonce_r_len > hmac_key_len))
    {
        ni_len = hmac_key_len / 2;

        if(hmac_key_len%2 == 0)
        {
            /* For even key length */
            nr_len = ni_len;
        }
        else
        {
            /* For odd key length */
            nr_len = ni_len + 1;
        }
    }
    else
    {
        ni_len = nonce_i_len;
        nr_len = nonce_r_len;
    }

    /* Allocate buffer for key (nonces concatenated) */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&hmac_key,
                                ni_len + nr_len, NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Fill in the pieces from both nonces. */
        NU_BLOCK_COPY(hmac_key, nonce_i, ni_len);
        NU_BLOCK_COPY(hmac_key + ni_len, nonce_r, nr_len);

        /* Pick the length of the output for the negotiated PRF algorithm. */
        sa->ike2_skeyseed_len = IKE2_PRF_HMAC_Algos[
            sa->ike_attributes.ike2_prf_algo].ike2_output_len;

        /* Allocate memory for the SKEYSEED. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&sa->ike2_skeyseed,
                                    sa->ike2_skeyseed_len, NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Calculate the SKEYSEED */
            status = IKE_HMAC_Init(IKE2_PRF_Algos[sa->ike_attributes.
                        ike2_prf_algo].crypto_algo_id, &hmac_ctx,
                        hmac_key, (ni_len + nr_len));

            if(status == NU_SUCCESS)
            {
                status = IKE_HMAC_Update(hmac_ctx, g_ir, g_ir_len);

                if(status == NU_SUCCESS)
                {
                    hmac_key_len = (UINT8)sa->ike2_skeyseed_len;
                    status = IKE_HMAC_Final(hmac_ctx,
                                            sa->ike2_skeyseed,
                                            &hmac_key_len);
                    sa->ike2_skeyseed_len = hmac_key_len;

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Error in hash calculation",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Error in hash calculation",
                                   NERR_SEVERE, __FILE__,
                                   __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Error in hash calculation",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Failed to allocate memory for SKEYSEED buffer",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

    }
    else
    {
        NLOG_Error_Log("Failed to allocate memory for key buffer",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(hmac_key != NU_NULL)
    {
        if(NU_Deallocate_Memory(hmac_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for key buffer",
                NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE2_Compute_SKEYSEED */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Compute_SKEYSEED_Rekey
*
* DESCRIPTION
*
*       This function computes SKEYSEED for IKE SA during re-keying of
*       IKE SA. It is computed as:
*       SKEYSEED = prf(SK_d (old), [g^ir (new)] | Ni | Nr)
*
* INPUTS
*
*       *sa                     New SA which will replace the old one.
*       *nonce_i                Initiator's nonce.
*       nonce_i_len             Length of initiator's nonce.
*       *nonce_r                Responder's nonce.
*       nonce_r_len             Responder's nonce length.
*       *g_ir                   DH public value.
*       g_ir_len                Length of DH public value
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATIC
STATUS IKE2_Compute_SKEYSEED_Rekey(IKE2_SA *sa, UINT8 *nonce_i,
                                   UINT16 nonce_i_len, UINT8 *nonce_r,
                                   UINT16 nonce_r_len, UINT8 *g_ir,
                                   UINT16 g_ir_len)
{
    STATUS          status;
    VOID            *hmac_ctx;
    UINT8           *hmac_data = NU_NULL;
    UINT16          hmac_data_len;
    UINT8           local_skeyseed_len = 0;

    /* The total data which we will calculate the PRF over comprises
     * initiator and responder nonces and the DH shared secret value.
     */
    hmac_data_len = g_ir_len + nonce_i_len + nonce_r_len;

    /* Allocate memory for keys. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&hmac_data,
        hmac_data_len, NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Copy in the data. */
        NU_BLOCK_COPY(hmac_data, g_ir, g_ir_len);
        NU_BLOCK_COPY(hmac_data + g_ir_len, nonce_i, nonce_i_len);
        NU_BLOCK_COPY(hmac_data + g_ir_len + nonce_i_len, nonce_r, nonce_r_len);

        /* Pick the length of the output for the negotiated PRF algorithm. */
        sa->ike2_skeyseed_len = IKE2_PRF_HMAC_Algos[
            sa->ike_attributes.ike2_prf_algo].ike2_output_len;

        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&sa->ike2_skeyseed,
                                    sa->ike2_skeyseed_len, NU_NO_SUSPEND);
        if(status == NU_SUCCESS)
        {
            /* Calculate the SKEYSEED. */
            status = IKE_HMAC_Init(IKE2_PRF_Algos[sa->ike_attributes.
                        ike2_prf_algo].crypto_algo_id, &hmac_ctx,
                        sa->ike2_sk_d, sa->ike_skeyid_len);

            if(status == NU_SUCCESS)
            {
                status = IKE_HMAC_Update(hmac_ctx, hmac_data,
                                         hmac_data_len);

                /* skeyseed_len should be a UINT8 parameter,
                 * hence use a local variable.
                 */
                status = IKE_HMAC_Final(hmac_ctx, sa->ike2_skeyseed,
                                        &local_skeyseed_len);

                /* Now copy the length back into the sa. */
                sa->ike2_skeyseed_len = local_skeyseed_len;

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Error in hash calculation",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            }

            else
            {
                NLOG_Error_Log("Failed to initialize the HMAC context",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to allocate memory for skeyseed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for keys",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(hmac_data != NU_NULL)
    {
        if(NU_Deallocate_Memory(hmac_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for key buffer",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }
    return (status);

} /* IKE2_Compute_SKEYSEED_Rekey */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Compute_Keys
*
* DESCRIPTION
*
*       This function computes keys for IKE SA. Keys are computed as:
*
*       {SK_d | SK_ai | SK_ar | SK_ei | SK_er | SK_pi | SK_pr } =
*                                   prf+ (SKEYSEED, Ni | Nr | SPIi | SPIr)
*
* INPUTS
*
*       *sa                     SA for which keys are to be computed.
*       *nonce_i                Initiator nonce.
*       *nonce_r                Responder nonce.
*       nonce_i_len             Initiator nonce length.
*       nonce_r_len             Responder nonce length.
*       flags                   Additional information for computing keys
*                               to identify initiator so that SPI is read
*                               correctly.
*
* OUTPUTS
*
*       NU_SUCCESS              Keys computed successfully.
*       IKE_INVALID_KEYLEN      Key length needed cannot be calculated
*
*************************************************************************/
STATIC
STATUS IKE2_Compute_Keys(IKE2_SA *sa, UINT8 *nonce_i, UINT8 *nonce_r,
                         UINT16 nonce_i_len, UINT16 nonce_r_len, UINT8 flags)
{
    STATUS          status = NU_SUCCESS;
    UINT16          length_prf;
    UINT16          length_needed;
    UINT8           *keys_buffer = NU_NULL;
    UINT8           *data_buffer = NU_NULL;
    UINT8           *temp_data = NU_NULL;
    UINT8           *temp_keys = NU_NULL;
    UINT16          data_buffer_len = 0;
    UINT16          sk_d_len;
    UINT16          sk_a_len;
    UINT16          sk_e_len = 0;
    UINT16          sk_p_len;
    UINT8           *digest;
    UINT8           digest_len;
    UINT8           *text;
    INT             text_len;
    UINT8           *constant_byte = NU_NULL;

    /* Pick up length needed for the negotiated PRF algorithm. */
    length_prf = IKE2_PRF_HMAC_Algos[sa->ike_attributes.ike2_prf_algo].
                    ike2_key_len;

    sk_e_len = sa->ike_attributes.ike_key_len;

    if (sk_e_len == 0)
    {
        /* Find length for encryption algorithm. */
        IKE_Crypto_Enc_Key_Len(IKE_Encryption_Algos[sa->ike_attributes.
                              ike_encryption_algo].crypto_algo_id,
                              &sk_e_len);
    }

    /* Pick the length of key needed for the HMAC AUTH algorithm. */
    sk_a_len = IKE2_AUTH_HMAC_Algos[sa->ike_attributes.ike_hash_algo].
                    ike2_key_len;

    /* Further keys are derived using the same PRF algorithm so sk_d_len
     * is same as sk_p_len (the length of key for PRF algorithm).
     */
    sk_d_len = length_prf;
    sk_p_len = length_prf;

    /* Total length needed for all the keys. */
    length_needed = sk_d_len + (2 * sk_a_len) + (2 * sk_e_len) + (2 * sk_p_len);

    /* Since calculating PRF+ according to RFC requires a single byte
     * mandatory padding which is incremented for each iteration, we
     * cannot perform more than 255 iterations. Check if the length we
     * need requires more iterations of PRF, reject the request because
     * we simply can't!
     */
    if(length_needed > (255 * length_prf))
    {
        NLOG_Error_Log("Needed length is larger than what can be calculated",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Set the appropriate error code. */
        status = IKE_INVALID_KEYLEN;
    }

    if(status == NU_SUCCESS)
    {
        /* Allocate buffer to hold the generated keying material. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)&keys_buffer, length_needed,
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* When iterating to calculate enough keying material, the
             * data used to calculate hash is appended with a constant
             * byte incremented sequentially. Data is also pre-pended
             * with the hash value calculated in previous iteration
             * (except in first iteration). Add these values to the
             * length of buffer.
             */
            data_buffer_len = length_prf + nonce_i_len + nonce_r_len +
                              (2 * IKE2_SPI_LENGTH) + IKE2_KEYMAT_PAD_LEN;

            /* Allocate buffer for concatenating nonces and SPIs into. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                (VOID**)&data_buffer, data_buffer_len, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Memory allocated, now fill in all the values. */
                temp_data = data_buffer + length_prf;
                NU_BLOCK_COPY(temp_data, nonce_i, nonce_i_len);
                temp_data += nonce_i_len;
                NU_BLOCK_COPY(temp_data, nonce_r, nonce_r_len);
                temp_data += nonce_r_len;

                /* Check to see in which order do we need to copy the
                 * SPIs.
                 */
                if((flags & IKE2_INITIATOR) != 0)
                {
                    NU_BLOCK_COPY(temp_data, sa->ike2_local_spi,
                                  IKE2_SPI_LENGTH);
                    temp_data += IKE2_SPI_LENGTH;
                    NU_BLOCK_COPY(temp_data, sa->ike2_remote_spi,
                                  IKE2_SPI_LENGTH);
                }
                else
                {
                    NU_BLOCK_COPY(temp_data, sa->ike2_remote_spi,
                        IKE2_SPI_LENGTH);
                    temp_data += IKE2_SPI_LENGTH;
                    NU_BLOCK_COPY(temp_data, sa->ike2_local_spi,
                        IKE2_SPI_LENGTH);
                }

                temp_data += IKE2_SPI_LENGTH;
                constant_byte = temp_data;
            }
            else
            {
                NLOG_Error_Log("Failed to allocate memory for data buffer",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Failed to allocate memory for output keys buffer",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Everything good so far. Now calculate the actual keys. */
        temp_data = data_buffer;
        temp_keys = keys_buffer;

        digest = temp_data;
        digest_len = 0;

        /* PRF+ is calculated by repeatedly prepending previous hash
         * with the block of text. For the first iteration, there is
         * no previous hash so set the text buffer pointer ahead in
         * the buffer, leaving room for hash to be copied in for next
         * iterations.
         */
        text = temp_data + length_prf;

        /* For first iteration, we do not have a hash value pre-pended
         * to the beginning of the data buffer.
         */
        text_len = data_buffer_len - length_prf;
        *constant_byte = IKE2_KEYMAT_PAD_INIT_VAL;

        /* Keys and data over which PRF is to be calculated are ready. */
        while((INT16)length_needed > 0)
        {
            status = IKE_HMAC(IKE2_PRF_Algos[sa->ike_attributes.ike2_prf_algo].crypto_algo_id,
                              sa->ike2_skeyseed, sa->ike2_skeyseed_len,
                              text, text_len, digest, &digest_len);

            if(status == NU_SUCCESS)
            {
                /* Copy the recently calculated hash to the growing keys
                 * buffer.
                 */
                NU_BLOCK_COPY(temp_keys, temp_data, digest_len);

                temp_keys += digest_len;

                /* If this is the first iteration, the constant byte
                 * added will have its initial value. If it is at its
                 * initial value, set the pointers to new buffers. We need
                 * to do this once and then just keep populating this new
                 * buffer.
                 */
                if(*constant_byte == IKE2_KEYMAT_PAD_INIT_VAL)
                {
                    text = temp_data;
                    text_len = text_len + length_prf;
                }

                *constant_byte += IKE2_KEYMAT_PAD_INCREMENT;

                /* We have computed some keying material. Subtract this
                 * length from the needed length.
                 */
                length_needed = length_needed - digest_len;

                /* If the total length of keys we need is not a multiple
                 * of the length of PRF algorithm's output, we will be
                 * left with some material whose length is less than the
                 * PRF's output. In that case, we only need that many bytes
                 * and NOT the entire output of PRF algorithm.
                 */
                if(length_needed < length_prf)
                {
                    /* We only need "length_needed" bytes from PRF's
                     * output which is less than what the PRF algo will
                     * generate.
                     */
                    digest_len = (UINT8)length_needed;
                }
                else
                {
                    /* We need all the bytes generated by PRF. */
                    digest_len = 0;
                }
            }
            else
            {
                NLOG_Error_Log("Failed to calculate keys for IKEv2 operation",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }

        } /* while */

        temp_keys = keys_buffer;
        if(status == NU_SUCCESS)
        {
            /* Keys have been calculated, now set the appropriate pointers
             * in the SA structure to these keys. The required keys are
             * assumed to be placed in the buffer in the following order:
             * {SK_d | SK_ai | SK_ar | SK_ei | SK_er | SK_pi | SK_pr}
             */
            sa->ike2_sk_d = temp_keys;
            temp_keys += sk_d_len;
            sa->ike2_sk_ai = temp_keys;
            temp_keys += sk_a_len;
            sa->ike2_sk_ar = temp_keys;
            temp_keys += sk_a_len;
            sa->ike2_sk_ei = temp_keys;
            temp_keys += sk_e_len;
            sa->ike2_sk_er = temp_keys;
            temp_keys += sk_e_len;
            sa->ike2_sk_pi = temp_keys;
            temp_keys += sk_p_len;
            sa->ike2_sk_pr = temp_keys;

            sa->ike2_a_len = sk_a_len;
            sa->ike2_p_len = sk_p_len;

        }

    }

    /* The keys buffer we allocated will be used by the SA for its entire
     * lifetime; and will be deallocated when the SA is deleted. The data
     * buffer we allocated is no longer needed and hence can be freed.
     */
    if(data_buffer != NU_NULL)
    {
        if(NU_Deallocate_Memory(data_buffer) != NU_SUCCESS)
        {
            NLOG_Error_Log("Could not deallocate memory for data buffer",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* If some problem occurred and we were not able to complete
     * successfully, delete the memory for keys buffer too.
     */
    if(status != NU_SUCCESS)
    {
        if((keys_buffer != NU_NULL) &&
            (NU_Deallocate_Memory(keys_buffer) != NU_SUCCESS))
        {
            NLOG_Error_Log("Could not deallocate memory for keys buffer",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE2_Compute_Keys */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Compute_Needed_Len
*
* DESCRIPTION
*
*       Computes the total length of keying material needed for IPsec SAs.
*
* INPUTS
*
*       *ips_sa                 IKE_IPS SA under which containing
*                               information about IPsec SA being negotiated.
*       *length                 Required length to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATIC STATUS IKE2_Compute_Needed_Len(IKE2_IPS_SA *ips_sa, UINT16 *length)
{
    STATUS          status = NU_SUCCESS;
    IKE2_IPS_SA     *temp_sa;
    UINT16          enc_len = 0;
    UINT16          auth_len = 0;

    *length = 0;
    temp_sa = ips_sa;

    /* We can have an "SA bundle" specified to be used with IPsec. For
     * example we are requested AH and ESP are both needed to be applied.
     * In that case, we will need keys not only for one ESP processing but
     * AH processing too. Iterate over all the such "bundle elements" and
     * calculate the total length of keys needed by all.
     */
    while(temp_sa != NU_NULL)
    {
        status = IKE_IPS_Get_Key_Length(&temp_sa->ike_ips_security,
                                        &enc_len, &auth_len);
        *length = *length + enc_len + auth_len;

        temp_sa = temp_sa->ike_flink;
    }

    /* This computed length is for one way SAs. We need twice this
     * length worth of keying material to serve pairs of SAs for
     * both directions.
     */
    *length = (*length) * 2;

    return (status);

} /* IKE2_Compute_Needed_Len */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_CHILD_SA_KEYMAT
*
* DESCRIPTION
*
*       Generates keying material for IPsec SAs.
*
*       KEYMAT = prf+(SK_d, Ni | Nr)
*
* INPUTS
*
*       *handle                 Exchange information for the which keying
*                               material is to be generated.
*
* OUTPUTS
*
*       NU_SUCCESS              Keying material generated successfully.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Generate_CHILD_SA_KEYMAT(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS          status = NU_SUCCESS;
    UINT16          len_needed = 0;
    UINT8           *prf_data = NU_NULL;
    UINT16          prf_data_len = 0;
    UINT16          len_prf_output = 0;
    UINT8           *keys_buffer = NU_NULL;
    UINT8           algo_id;
    UINT8           *text;
    INT             text_len;
    UINT8           *digest;
    UINT8           digest_len;
    UINT8           *key;
    INT             key_len;
    UINT8           *constant_byte = NU_NULL;

    UINT8           *temp_data = NU_NULL;
    UINT8           *temp_keys = NU_NULL;

    IKE2_IPS_SA     *ips_sa = NU_NULL;
    UINT16          auth_key_len = 0;
    UINT16          enc_key_len = 0;

#if (IKE2_DEBUG == NU_TRUE)
    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    prf_data_len = handle->ike2_nonce_len + handle->ike2_peer_nonce_len;

    /* Calculate the total length of keys needed for the child SA. */
    status = IKE2_Compute_Needed_Len(handle->ike2_sa2_db.ike_flink,
                                     &len_needed);

    len_prf_output = IKE2_PRF_HMAC_Algos[handle->ike2_sa->ike_attributes.
                        ike2_prf_algo].ike2_output_len;

    /* Check to see if we will need more than 255 iterations of PRF during
     * calculation of PRF+.
     */
    if(len_needed > len_prf_output * 255)
    {
        NLOG_Error_Log("Needed length is larger than what can be calculated",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
        status = IKE_INVALID_KEYLEN;
    }

    if(status == NU_SUCCESS)
    {
        /* Allocate memory for they keys buffer. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&keys_buffer,
                                    len_needed, NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Allocate memory for data over which HMAC is to be
             * computed.
             */
            prf_data_len = prf_data_len + len_prf_output + IKE2_KEYMAT_PAD_LEN;

            status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&prf_data,
                                        prf_data_len, NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                temp_data = prf_data + len_prf_output;
                /* Now copy the nonces into the PRF data buffer. */
                if((handle->ike2_flags & IKE2_INITIATOR) != 0 &&
                    (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
                    IKE2_HDR_RESPONSE_FLAG) != 0)
                {
                    NU_BLOCK_COPY(temp_data, handle->ike2_nonce,
                        handle->ike2_nonce_len);
                    temp_data = temp_data + handle->ike2_nonce_len;
                    NU_BLOCK_COPY(temp_data, handle->ike2_peer_nonce,
                        handle->ike2_peer_nonce_len);
                    temp_data = temp_data + handle->ike2_peer_nonce_len;
                }
                else
                {
                    NU_BLOCK_COPY(temp_data, handle->ike2_peer_nonce,
                        handle->ike2_peer_nonce_len);
                    temp_data = temp_data + handle->ike2_peer_nonce_len;
                    NU_BLOCK_COPY(temp_data, handle->ike2_nonce,
                        handle->ike2_nonce_len);
                    temp_data = temp_data + handle->ike2_nonce_len;
                }

                constant_byte = temp_data;

            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for PRF data.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to allocate memory for IPsec keys",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    if(status == NU_SUCCESS)
    {
        /* All good, calculate PRF+ */
        temp_data = prf_data;
        temp_keys = keys_buffer;

        digest = temp_data;
        digest_len = 0;
        key = handle->ike2_sa->ike2_sk_d;
        key_len = handle->ike2_sa->ike2_skeyseed_len;
        algo_id = IKE2_PRF_Algos[handle->ike2_sa->ike_attributes.ike2_prf_algo].crypto_algo_id;
        text = temp_data + len_prf_output;
        text_len = prf_data_len - len_prf_output;

        *constant_byte = IKE2_KEYMAT_PAD_INIT_VAL;

        /* Compute PRF+ */
        while(len_needed > 0)
        {
            status = IKE_HMAC(algo_id, key, key_len,
                              text, text_len, digest, &digest_len);

            if(status == NU_SUCCESS)
            {
                /* Copy the part of keys we just generated to the growing
                 * keys buffer.
                 */
                NU_BLOCK_COPY(temp_keys, temp_data, digest_len);

                temp_keys += digest_len;

                /* If this is first iteration, set the pointer to start of
                 * buffer. First iteration did not require pre-pending
                 * anything.
                 */
                if(*constant_byte == IKE2_KEYMAT_PAD_INIT_VAL)
                {
                    text = temp_data;
                    text_len = text_len + len_prf_output;
                }

                *constant_byte += IKE2_KEYMAT_PAD_INCREMENT;

                /* Decrement the length needed by the length of material
                 * we just generated.
                 */
                len_needed = len_needed - digest_len;

                /* If the total length of keys is not a multiple of PRF's
                 * output, we will need less bytes in last iteration than
                 * the PRF generates.
                 */
                if(len_needed < len_prf_output)
                {
                    /* We need less bytes than the PRF's output, copy only
                     * len_needed bytes.
                     */
                    digest_len = (UINT8)len_needed;
                }
                else
                {
                    /* Copy all bytes from PRF output. */
                    digest_len = 0;
                }
            }

            else
            {
                NLOG_Error_Log("Failed to calculate keys for IPsec SA",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
                break;
            }
        } /* while */
    }

    if(status == NU_SUCCESS)
    {
        /* Keys have been computed. Now copy them to the IPsec SA information. */
        ips_sa = handle->ike2_sa2_db.ike_flink;
        temp_keys = keys_buffer;

        while(ips_sa != NU_NULL)
        {
            status = IKE_IPS_Get_Key_Length(&ips_sa->ike_ips_security,
                                            &enc_key_len, &auth_key_len);
            if(status == NU_SUCCESS)
            {
                ips_sa->ike_keymat_len = enc_key_len + auth_key_len;

                /* We need to take keying material for initiator before the
                 * responder. Check who is initiator and who is responder.
                 */
                if((handle->ike2_flags & IKE2_RESPONDER) != 0 ||
                    (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
                     IKE2_HDR_RESPONSE_FLAG) == 0)
                {
                    /* We are responder. We need keying material for incoming
                     * traffic first.
                     */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                                (VOID**)&ips_sa->ike_local_keymat,
                                                ips_sa->ike_keymat_len * 2,
                                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        NU_BLOCK_COPY(ips_sa->ike_local_keymat, temp_keys,
                                      ips_sa->ike_keymat_len * 2);
                        temp_keys = temp_keys + ips_sa->ike_keymat_len * 2;
                        ips_sa->ike_remote_keymat = ips_sa->ike_local_keymat +
                                                    ips_sa->ike_keymat_len;
                    }
                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for local \
                                       and remote keymat", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }
                }

                else
                {
                    /* We are initiator. Keying material for outgoing traffic
                     * should be taken first.
                     */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                                (VOID**)&ips_sa->ike_remote_keymat,
                                                ips_sa->ike_keymat_len * 2,
                                                NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        NU_BLOCK_COPY(ips_sa->ike_remote_keymat, temp_keys,
                                      ips_sa->ike_keymat_len * 2);
                        temp_keys = temp_keys + ips_sa->ike_keymat_len * 2;
                        ips_sa->ike_local_keymat = ips_sa->ike_remote_keymat +
                                                   ips_sa->ike_keymat_len;
                    }
                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for local \
                                       and remote keymat", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for IPsec SA keys",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            ips_sa = ips_sa->ike_flink;
        }
    }

    if((prf_data != NU_NULL) &&
        (NU_Deallocate_Memory(prf_data) != NU_SUCCESS))
    {
        NLOG_Error_Log("Failed to deallocate memory for PRF data buffer",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if((keys_buffer != NU_NULL) &&
        (NU_Deallocate_Memory(keys_buffer) != NU_SUCCESS))
    {
        NLOG_Error_Log("Failed to deallocate memory for keys buffer",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Generate_CHILD_SA_KEYMAT */

#endif

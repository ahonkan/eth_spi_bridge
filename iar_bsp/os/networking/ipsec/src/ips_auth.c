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
*       ips_auth.c
*
* COMPONENT
*
*       AUTHENTICATOR
*
* DESCRIPTION
*
*       Contains implementation of Nucleus IPsec Authenticator
*       component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Calculate_Digest
*       IPSEC_Get_Auth_Algo_Index
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ips_externs.h
*       hmac.h
*       md5.h
*       sha.h
*       aes.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ips_externs.h"
#include "openssl/hmac.h"
#include "openssl/md5.h"
#include "openssl/sha.h"
#include "openssl/aes.h"
/************************************************************************
* FUNCTION
*
*       IPSEC_Calculate_Digest
*
* DESCRIPTION
*
*       This function calculates the digest for the passed buffer
*       with the given parameters through the HMAC request structure.
*
* INPUTS
*
*       *buf_ptr                The buffer for which digest needs to be
*                               calculated.
*       *request                Request structure which contains all the
*                               necessary information for calculating
*                               the digest.
* OUTPUTS
*
*       NU_SUCCESS              On successful digest calculation.
*       IPSEC_CRYPTO_ERROR      Crypto related error.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Calculate_Digest(NET_BUFFER *buf_ptr,
                              IPSEC_AUTH_REQ *request)
{
    STATUS              status;
    NET_BUFFER          *buffer = buf_ptr;
    HMAC_CTX            context;
    const EVP_MD        *md;
    UINT8               digest[EVP_MAX_MD_SIZE];
    UINT                dl;

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
    UINT8               *data_ptr;
    IPS_CIPHER_MAC_CTX  aes_mac_ctx;
#endif

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buf_ptr == NU_NULL) || (request == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
    /* Need to do special processing for the XCBC MAC. For now
     * we just support AES, so that is hard-coded here.
     */
    if (request->ipsec_hash_algo == IPSEC_XCBC_MAC_96)
    {
        status = IPSEC_XCBC_MAC_Init(&aes_mac_ctx, request->ipsec_key,
                                     request->ipsec_key_len );
    }
    else
#endif
    {
        /* Initialize hash context. */
        HMAC_CTX_init(&context);

        switch (request->ipsec_hash_algo)
        {
#if (IPSEC_INCLUDE_MD5 == NU_TRUE)
             case IPSEC_HMAC_MD5_96:
                  md = EVP_md5();
             break;
#endif
#if (IPSEC_INCLUDE_SHA1 == NU_TRUE)
             case IPSEC_HMAC_SHA_1_96:
                  md = EVP_sha1();
             break;
#endif
#if (IPSEC_INCLUDE_SHA256 == NU_TRUE)
             case IPSEC_HMAC_SHA_256_96:
                  md = EVP_sha256();
             break;
#endif
             default:
                  return (IPSEC_INVALID_PARAMS);
        }

        status = NUCLEUS_STATUS(HMAC_Init_ex(&context, request->ipsec_key,
                                                request->ipsec_key_len, md, 0));
    }

    /* If context is initialized successfully. */
    if(status == NU_SUCCESS)
    {
        /* Loop through out the buffer chain. */
        while(buffer != NU_NULL)
        {
            /* Update digest function for calculating the digest
               for the passed data. */

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)

            if (request->ipsec_hash_algo == IPSEC_XCBC_MAC_96)
            {
                if(buffer->data_len != 0)
                {
                    /*Allocating memory for message to be copied from net buffer
                    and passed in to Cipher-MAC Update routine*/
                    status = NU_Allocate_Memory(IPSEC_Memory_Pool,
                        (VOID **)&data_ptr,buffer->data_len,NU_NO_SUSPEND);

                    /* Check the status value. */
                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to allocate the memory",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                    else
                    {
                        /*Copying net buffer data to temp buffer data_ptr*/
                        memcpy(data_ptr,buffer->data_ptr,buffer->data_len);

                        status = IPSEC_XCBC_MAC_Update( &aes_mac_ctx,
                                               (UINT8 *)data_ptr,
                                               (UINT16)buffer->data_len );

                        /*The data_ptr contains garbage and is of no use so deallocate
                          its memory*/
                        NU_Deallocate_Memory(data_ptr);
                    }
                }
                else
                    /* The last valid buffer only contains auth digest */
                    break;
            }
            else
#endif
            status = NUCLEUS_STATUS(HMAC_Update(&context,
                        (UINT8 *)buffer->data_ptr, (UINT16)buffer->data_len));

            /* Make sure of the current status value. */
            if(status != NU_SUCCESS)
                break;

            /* Point to the next buffer in the chain. */
            buffer = buffer->next_buffer;
        }

        /* Make sure of the current status value. */
        if(status == NU_SUCCESS)
        {
            /* Now finalize the digest. */

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)

            if (request->ipsec_hash_algo == IPSEC_XCBC_MAC_96)
            {
                status = IPSEC_XCBC_MAC_Final(&aes_mac_ctx, digest, &dl);
            }
            else
#endif
                status = NUCLEUS_STATUS(HMAC_Final(&context, digest, &dl));

            /* Copy the digest to request structure */
            if(status == NU_SUCCESS)
                memcpy(request->ipsec_digest, digest, request->ipsec_digest_len);
        }
    }

#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
    if (request->ipsec_hash_algo != IPSEC_XCBC_MAC_96)
    {
#endif
    /* Clean up hash context. */
    HMAC_CTX_cleanup(&context);
#if (IPSEC_INCLUDE_XCBC_MAC_96 == NU_TRUE)
    }
#endif

    /* Return the status value. */
    return (status);

} /* IPSEC_Calculate_Digest. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Auth_Algo_Index
*
* DESCRIPTION
*
*       This function returns the index of the required algorithm from the
*       global array of authentication algorithms corresponding to the
*       passed algo ID.
*
* INPUTS
*
*       *algo_id                Pointer to the passed algorithm ID.
*                               On successful return this will contain
*                               the index of the required algorithm from
*                               the global array of IPsec algorithms.
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   Authentication algorithms not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Auth_Algo_Index(UINT8 *algo_id)
{
    STATUS              status = NU_SUCCESS;

/* If one-to-one mapping is not present. */
#if (IPSEC_AUTH_1_TO_1 == NU_FALSE)
    UINT8       algo_index;
#endif

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(algo_id == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Make sure that the algo ID lies in the given range. */
    if((*algo_id) >= IPSEC_TOTAL_AUTH_ALGO)
    {
        /* Passed algo id is not valid. */
        status = IPSEC_INVALID_ALGO_ID;
    }

/* If one-to-one mapping is not present. */
#if (IPSEC_AUTH_1_TO_1 == NU_FALSE)
    else
    {
        /* We have to first find the required algorithm from the global
           array of authentication algorithms through iteration. */
        for(algo_index = 0;
            algo_index < IPSEC_TOTAL_AUTH_ALGO;
            algo_index++)
        {
            /* Now match the algorithms IDs. */
            if(IPSEC_Authentication_Algos[algo_index].
                                    ipsec_algo_identifier == (*algo_id))
            {
                /* Break the loop. */
                break;
            }
        }

        /* Make sure we got the required algo. */
        if(algo_index == IPSEC_TOTAL_AUTH_ALGO)
        {
            NLOG_Error_Log("IPsec: Auth algo not found",
                           NERR_RECOVERABLE,__FILE__, __LINE__);

            /* Set the error status. */
            status = IPSEC_INVALID_ALGO_ID;
        }
    }
#endif

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Auth_Algo_Index. */

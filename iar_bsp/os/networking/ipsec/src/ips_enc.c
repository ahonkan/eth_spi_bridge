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
*       ips_enc.c
*
* COMPONENT
*
*       ENCRYPTOR
*
* DESCRIPTION
*
*       Contains implementation of Nucleus IPsec Encryptor component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Cipher_Operation
*       IPSEC_Get_Encrypt_Algo_Index
*
* DEPENDENCIES
*
*       evp.h
*       nu_net.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "openssl/evp.h"

/************************************************************************
* FUNCTION
*
*       IPSEC_Cipher_Operation
*
* DESCRIPTION
*
*       This function is passed a NET buffer chain as well as an IPsec
*       request structure. On return, the NET buffer chain contains the
*       encrypted data. Note that the IV for the encryption algorithm is
*       part of the encrypted data that is returned.
*
*
* INPUTS
*
*       *buffer                 NET Buffer chain
*       *request                Encryption parameters.
*       *iv                     Pointer to initialization vector.
*       iv_len                  Initialization vector length.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful digest calculation.
*       IPSEC_CRYPTO_ERROR      General error indicating that one
*                               of the internal functions failed.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Cipher_Operation(NET_BUFFER *buffer,
                              IPSEC_ENCRYPT_REQ *request, UINT8 *iv,
                              UINT8 iv_len)
{
    STATUS              status = NU_SUCCESS;
    UINT8               curr_buff_pend_len, copy_len;
    UINT8               temp_buff[IPSEC_CBC_MAX_BLK_SIZE];
    NET_BUFFER          *buf_ptr = buffer;
    EVP_CIPHER_CTX      context;
    const EVP_CIPHER    *ct;
    INT                 enc_data_len = 0;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((buffer == NU_NULL) || (request == NU_NULL) || (iv == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* If it is an encryption request. */
    if(request->operation == ENCRYPT)
    {
        /* Now also store this IV into actual packet as well. */
        NU_BLOCK_COPY(iv, request->cipher_iv, iv_len);
    }
    else
    {
        /* Identify where the IV is present in case of decryption. */
        request->cipher_iv = iv;
    }

    /* Initialize context. */
    EVP_CIPHER_CTX_init(&context);

    /* Get openssl evp cipher structure for desired algorithm and initialize. */
    ct = (EVP_CIPHER*)IPSEC_Encryption_Algos[request->ipsec_algo_index].evp_cipher();
    EVP_CipherInit_ex(&context, ct, 0, request->cipher_key, request->cipher_iv,
                                            (UINT)request->operation);

    /* Disable cipher padding. */
    EVP_CIPHER_CTX_set_padding(&context, 0);

    /* Make sure buffer pointer is not null. */
    while(buf_ptr != NU_NULL)
    {
        /* Make sure that the data is in terms of integral multiple of
           block size of the employing algorithm. */
        curr_buff_pend_len = (UINT8)(request->text_len &
                                                    ((UINT16)iv_len - 1));

        /* If it is not. */
        if(curr_buff_pend_len != 0)
        {
            /* Update the length. */
            request->text_len =
                (UINT16)(request->text_len - curr_buff_pend_len);
        }

        /* If text length is zero then no need to encrypt. */
        if(request->text_len != 0)
        {
            /* Call the OPENSSL for encryption. */
            status = NUCLEUS_STATUS(EVP_CipherUpdate(&context,
                        request->buffer, &enc_data_len, request->buffer,
                        request->text_len));

            /* Check the status. */
            if(status != NU_SUCCESS)
                break;
        }

        if(curr_buff_pend_len != 0)
        {
            /* We need to borrow some data from next buffer in the
             * chain to make pending data of current buffer multiple
             * of block size of the employing algorithm.
             */

            /* Copy the pending data from the current buf_ptr to
               the temporary buffer. */
            NU_BLOCK_COPY(temp_buff, &(buf_ptr->data_ptr[
                          buf_ptr->data_len - curr_buff_pend_len]),
                          curr_buff_pend_len);

            copy_len = iv_len - curr_buff_pend_len;

            if ((copy_len != 0) && (copy_len <= (IPSEC_CBC_MAX_BLK_SIZE - curr_buff_pend_len)))
            {
                /* Fill the remaining part of the temporary buffer. */
                NU_BLOCK_COPY(&(temp_buff[curr_buff_pend_len]),
                              buf_ptr->next_buffer->data_ptr,
                              copy_len);
            }

            else
            {
                status = IPSEC_INVALID_PARAMS;
                break;
            }

            /* Now call the API for encryption. */
            status = NUCLEUS_STATUS(EVP_CipherUpdate(&context, temp_buff,
                                                 &enc_data_len, temp_buff, iv_len));

            /* Check the status. */
            if(status != NU_SUCCESS)
            {
                /* break the loop. */
                break;
            }

            /* First part of the temporary encrypted temporary buffer will
               be copied to the current buf_ptr. */
            NU_BLOCK_COPY(&(buf_ptr->data_ptr[buf_ptr->data_len -
                          curr_buff_pend_len]), temp_buff,
                          curr_buff_pend_len);

            copy_len = iv_len - curr_buff_pend_len;

            if ((copy_len != 0) && (copy_len <= (IPSEC_CBC_MAX_BLK_SIZE - curr_buff_pend_len)))
            {
                /* Second part will be copied to the next buf_ptr to
                   which it belongs. */
                NU_BLOCK_COPY(buf_ptr->next_buffer->data_ptr,
                              &(temp_buff[curr_buff_pend_len]),
                              copy_len);
            }

            else
            {
                status = IPSEC_INVALID_PARAMS;
                break;
            }
        }

        /* If whole chain has not been traversed. */
        if(buf_ptr->next_buffer != NU_NULL)
        {
            /* Move onto next buffer. */
            buf_ptr = buf_ptr->next_buffer;

            /* Update the request structure with the next buffer data.
             */
            if(curr_buff_pend_len != 0)
            {
                request->buffer = (UINT8*)buf_ptr->data_ptr +
                                        (iv_len - curr_buff_pend_len);

                request->text_len = (UINT16)buf_ptr->data_len -
                                        (iv_len - curr_buff_pend_len);
            }
            else
            {
                request->buffer     = (UINT8*)buf_ptr->data_ptr;

                request->text_len   = (UINT16)buf_ptr->data_len;
            }
        }
        else
        {
            /* Whole chain has been traversed, just break the loop. */
            break;
        }

    } /* End of while loop. */

    if(status == NU_SUCCESS)
        status = NUCLEUS_STATUS(EVP_CipherFinal_ex(&context,
                                    request->buffer, &enc_data_len));

    /* Remove a reference of static array from return parameter. */
    request->buffer = NU_NULL;

    EVP_CIPHER_CTX_cleanup(&context);

    /* Return the status value. */
    return (status);

} /* IPSEC_Cipher_Operation */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Encrypt_Algo_Index
*
* DESCRIPTION
*
*       This function returns the index of the required algo from the
*       global array of encryption algorithms corresponding to the
*       passed algorithm ID.
*
* INPUTS
*
*       *algo_id                Pointer to the passed algo ID.
*                               On successful return this will contain
*                               the index of the required algo from the
*                               global array of algorithm IDs.
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_INVALID_ALGO_ID   Encrypt algorithm not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Encrypt_Algo_Index(UINT8 *algo_id)
{
    STATUS              status = NU_SUCCESS;

/* If one-to-one mapping is not present. */
#if (IPSEC_ENCRYPT_1_TO_1 == NU_FALSE)
    UINT8               algo_index = 0;
#endif

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(algo_id == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Make sure that the algo ID lies in the given range. */
    if(*algo_id >= IPSEC_TOTAL_ENCRYPT_ALGO)
    {
        /* Passed algo id is not valid. */
        status = IPSEC_INVALID_ALGO_ID;
    }

/* If one-to-one mapping is not present. */
#if (IPSEC_ENCRYPT_1_TO_1 == NU_FALSE)
    else
    {
        /* We have to first find the required algorithm from the global
           array of encryption algorithm through iteration. */
        for(; algo_index < IPSEC_TOTAL_ENCRYPT_ALGO; algo_index++)
        {
            /* Now match the algo IDs. */
            if(IPSEC_Encryption_Algos[algo_index].ipsec_algo_identifier
                                                            == *algo_id)
            {
                /* Break the loop. */
                break;
            }
        }

        /* Make sure we got the required algorithm. */
        if(algo_index == IPSEC_TOTAL_ENCRYPT_ALGO)
        {
            NLOG_Error_Log("IPsec: Encryption algo not found",
                           NERR_RECOVERABLE,__FILE__, __LINE__);

            /* Set the error status. */
            status = IPSEC_INVALID_ALGO_ID;
        }
    }
#endif

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Encrypt_Algo_Index */

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
*       ike2_cookie.c
*
* COMPONENT
*
*       IKEv2 - Cookie Generation and Processing
*
* DESCRIPTION
*
*       This file implements all the functions that process cookies
*       which are used to prevent space and CPU exhaustion attacks.
*
* FUNCTIONS
*
*       IKE2_Generate_Cookie
*       IKE2_Generate_Secret
*       IKE2_Init_Cookie
*       IKE2_Verify_Cookie
*       IKE2_Save_Cookie
*
* DATA STRUCTURES
*
*       IKE2_Cookie_Resource    A semaphore used to protect critical
*                               sections of cookie processing.
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike_buf.h
*       ike_evt.h
*       rand.h
*       sha.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_buf.h"
#include "networking/ike_evt.h"
#include "openssl/rand.h"
#include "openssl/sha.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/* ID incremented whenever a new secret is generated and is used in cookie. */
static UINT32           ike2_ver_id_secret;

/* Two secrets are maintained at a time because one might expire, before
 * processing with the older is completed. Moreover, two pointers are also
 * maintain to avoid copying buffers again and again.
 */
static UINT8            ike2_current_secret[IKE2_SECRET_LENGTH];
static UINT8            ike2_old_secret[IKE2_SECRET_LENGTH];

/* When a new secret is generated, one of pointers is swapped while the other
 * receives newly allocated data.
 */
static UINT8            *ike2_curr_scrt_ptr;
static UINT8            *ike2_old_scrt_ptr;

/* Declaring the Cookie semaphore for protection. */
NU_SEMAPHORE            IKE2_Cookie_Resource;


/*************************************************************************
*
* FUNCTION
*
*       IKE2_Init_Cookie
*
* DESCRIPTION
*
*       This function initializes cookie related data like version id,
*       current and old pointers to secrets. It should be called upon
*       IKEv2's Initialization.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE2_Init_Cookie()
{
    STATUS              status = NU_SUCCESS;

    /* Initialize the cookie version ID. */
    ike2_ver_id_secret = 0;

    /* Set the current secret pointer to the first secret value to
     * be used.
     */
    ike2_curr_scrt_ptr = ike2_current_secret;

    /* We do not have any old secret yet, but set this pointer so that
     * this memory can be used later.
     */
    ike2_old_scrt_ptr = ike2_old_secret;

    /* Create the IKEv2 Cookie semaphore. */
    status = NU_Create_Semaphore(&IKE2_Cookie_Resource, "IKE2_Cookie",
                                 (UNSIGNED)1, NU_FIFO);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create IKEv2 cookie semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Init_Cookie */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_Secret
*
* DESCRIPTION
*
*       This function generates a new secret which in turn is used to
*       calculate cookie.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE2_Generate_Secret()
{
     STATUS  status = NU_SUCCESS;
     UINT8   *temp_secret;

    /* Grab the Cookie semaphore */
    status = NU_Obtain_Semaphore(&IKE2_Cookie_Resource, IKE_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IKEv2 cookie semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    else
    {
        /* Use a temporary pointer for swapping. */
        temp_secret = ike2_curr_scrt_ptr;
        ike2_curr_scrt_ptr = ike2_old_scrt_ptr;

        /* Generate pseudo-random bytes as secret. */

        if(RAND_bytes(ike2_curr_scrt_ptr, IKE2_SECRET_LENGTH))
        {
            /* Change the pointer to current secret to the old one. */
            ike2_old_scrt_ptr = temp_secret;

            /* Increment version id with each new secret generated. */
            ike2_ver_id_secret++;

            /* Sets timer and specifies itself as call back */
            status = IKE_Set_Timer(IKE2_Cookie_Secret_Timeout_Event, NU_NULL,
                                   NU_NULL, IKE2_SECRET_KEEP_ALIVE_TIME);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set timer for IKEv2 Cookie Secret \
                          lifetime.", NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Since we failed, so retain earlier current secret. */
                ike2_curr_scrt_ptr = temp_secret;
            }
        }

        else
        {
            NLOG_Error_Log("Failed to Generate new secret.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the IKEv2 cookie semaphore. */
        status = NU_Release_Semaphore(&IKE2_Cookie_Resource);

        if( status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the IKEv2 cookie semaphore",
                NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE2_Generate_Secret */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_Cookie
*
* DESCRIPTION
*
*       This function generates a new cookie using the secret and other
*       information. It is also called when a packet containing
*       a cookie has arrived and a local cookie is to be generated for
*       verification. It handles the case of recomputing the old cookie
*       if it was expired before receiving a response.
*
* INPUTS
*
*       *handle                 Pointer to the exchange handle in which
*                               cookie is to be populated.
*       *new_cookie             Pointer to buffer where newly generated
*                               cookie is stored.
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS      Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Generate_Cookie(IKE2_EXCHANGE_HANDLE *handle, UINT8 *new_cookie)
{
    STATUS         status = NU_SUCCESS;

    /* Used to track current index in the cookie buffer. */
    UINT16         offset;

    UINT8          *buffer;
    UINT16         buffer_len;
    UINT32         new_cookie_id;

    /* Pointer to secret being used for cookie calculation. */
    UINT8          *secret_ptr;

    /* Local variable used to store the hash. */
    UINT8          sha1_hash[SHA_DIGEST_LENGTH];

    /* Make sure pointers are valid. */
    if(handle == NU_NULL || new_cookie == NU_NULL)
    {
        status = IKE2_INVALID_PARAMS;
    }

    else
    {
        /* Compute total buffer length to be allocated first. */
        buffer_len = handle->ike2_peer_nonce_len + MAX_ADDRESS_SIZE +
            IKE2_HDR_SA_SPI_LEN + IKE2_SECRET_LENGTH;

        /* Need to Allocate memory for temp buffer to concatenate all fields
         * required to generate a cookie.
         */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
            (VOID **)&buffer, buffer_len, NU_NO_SUSPEND);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to allocate memory for temp buffer.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Cookie is generated as follows.
             * Cookie = <VersionIDofSecret> | Hash(Ni | IPi | SPIi | <secret>)
             */

            /* Now concatenate the four fields first. */
            NU_BLOCK_COPY(buffer, handle->ike2_peer_nonce,
                handle->ike2_peer_nonce_len);

            offset = handle->ike2_peer_nonce_len;

            NU_BLOCK_COPY(buffer + offset, handle->ike2_params->ike2_packet->
                ike_remote_addr.id.is_ip_addrs,MAX_ADDRESS_SIZE);

            offset += MAX_ADDRESS_SIZE;

            NU_BLOCK_COPY(buffer + offset, handle->ike2_params->ike2_in
                .ike2_hdr->ike2_sa_spi_i,IKE2_HDR_SA_SPI_LEN);

            offset += IKE2_HDR_SA_SPI_LEN;

            /* Grab the Cookie semaphore */
            status = NU_Obtain_Semaphore(&IKE2_Cookie_Resource, IKE_TIMEOUT);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain IKEv2 cookie semaphore",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Check if exchange handle already contains a valid cookie by
                 * decoding version id.
                 */
                new_cookie_id = GET32(handle->ike2_cookie, 0);

                /* If we have a new cookie generation request or a verify
                 * request with version id same as the current, then in both
                 * cases current version id and secret should be used for
                 * cookie calculation.
                 */
                if(new_cookie_id == 0 || new_cookie_id == ike2_ver_id_secret)
                {
                    new_cookie_id = ike2_ver_id_secret;
                    secret_ptr = ike2_curr_scrt_ptr;
                }

                else
                {
                    /* Cookie was generated from the previous secret which has
                     * expired. So we will use the last expired secret with
                     * last version id in computing cookie. Please note that
                     * this will allow only one formerly valid secret and hence
                     * version id.
                     */
                    new_cookie_id = ike2_ver_id_secret - 1;

                    /* Old secret should be used to calculate cookie. */
                    secret_ptr = ike2_old_scrt_ptr;
                }

                /* Concatenate version id in the new_cookie buffer.*/
                PUT32(new_cookie, 0, new_cookie_id);

                NU_BLOCK_COPY(buffer + offset, secret_ptr, IKE2_SECRET_LENGTH);

                offset += IKE2_SECRET_LENGTH;

                /* Now compute their hash. */

                if (NU_NULL != SHA1(buffer, buffer_len, sha1_hash))
                {
                    /* We need a fixed length hash in order to keep cookie
                     * length fixed.
                     */

                    /* Append the resultant hash with new_cookie buffer. */
                    NU_BLOCK_COPY(new_cookie + sizeof(ike2_ver_id_secret),
                                  sha1_hash, IKE2_HASH_LENGTH);
                }
                else
                {
                    status = IKE_INTERNAL_ERROR;
                    NLOG_Error_Log("Failed to compute IKEv2 cookie hash",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Release the IKEv2 cookie semaphore. */
                if(NU_Release_Semaphore(&IKE2_Cookie_Resource) != NU_SUCCESS)
                {
                    status = IKE2_INVALID_PARAMS;
                    NLOG_Error_Log("Failed to release the IKEv2 cookie "
                                   "semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
                }
            }

            /* Deallocate temp memory buffer. */
            NU_Deallocate_Memory(buffer);
        }
    }

    return (status);

} /* IKE2_Generate_Cookie */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Verify_Cookie
*
* DESCRIPTION
*
*       This function receives a cookie coming from a sender, computes
*       local cookie from available data and compares them both. If they
*       match, it means it was a valid cookie sent earlier by us.
*
*
* INPUTS
*
*       *handle                 Pointer to the exchange handle containing
*                               the current cookie.
*
* OUTPUTS
*
*       NU_SUCCESS              On success. Cookies matched successfully.
*       IKE2_COOKIE_MISMATCHED  Cookies did not match.
*       IKE2_INVALID_PARAMS      Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Verify_Cookie(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS              status = NU_SUCCESS;
    UINT8               *new_cookie;

    /* Make sure pointer is valid. */
    if(handle == NU_NULL)
    {
        status = IKE2_INVALID_PARAMS;
    }

    /* We have to generate a new cookie to compare it with the one present
     * in the current exchange handle. First allocate temp buffer for new
     * cookie.
     */
    status = NU_Allocate_Memory(IKE_Data.ike_memory,
        (VOID **)&new_cookie, IKE2_COOKIE_LENGTH, NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for temp buffer.",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* Generate the new_cookie. */
        status = IKE2_Generate_Cookie(handle, new_cookie);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to generate new cookie.",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Compare both the cookies. */
            if(memcmp(handle->ike2_cookie, new_cookie, IKE2_COOKIE_LENGTH)
                != 0)
            {
                /* Cookies didn't match and sender couldn't be verified. */
                status = IKE2_COOKIE_MISMATCHED;
            }
        }

        /* Deallocate memory reserved for new cookie. */
        NU_Deallocate_Memory(new_cookie);
    }

    return (status);

} /* IKE2_Verify_Cookie */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Save_Cookie
*
* DESCRIPTION
*
*       This function saves the cookie information received from an IKE_SA_INIT
*       response.  The cookie will be sent in the retransmission of the
*       IKE_SA_INIT request.
*
* INPUTS
*
*       *handle                 Pointer to the exchange handle containing
*                               the current cookie.
*       *ntfy_ptr               Pointer to the incoming notify header.
*
* OUTPUTS
*
*       NU_SUCCESS
*       NU_NO_MEMORY            The cookie will overflow the local buffer.
*
*************************************************************************/
STATUS IKE2_Save_Cookie(IKE2_EXCHANGE_HANDLE *handle, IKE2_NOTIFY_PAYLOAD *ntfy_ptr)
{
    STATUS  status;
    
    /* Ensure the cookie data will not overflow the local buffer. */
    if (ntfy_ptr->ike2_notify_data_len <= IKE2_COOKIE_LENGTH)
    {
        /* Save the length of the cookie. */
        handle->ike2_cookie_len = ntfy_ptr->ike2_notify_data_len;
        
        /* Copy the notification data into the cookie string. */
        memcpy(handle->ike2_cookie, ntfy_ptr->ike2_notify_data,
               handle->ike2_cookie_len);
               
        status = NU_SUCCESS;
    }
    
    else
    {
        status = NU_NO_MEMORY;
    }
           
    return (status);
}

#endif

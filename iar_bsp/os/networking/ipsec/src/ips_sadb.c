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
*       ips_sadb.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementation of SA databases.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Add_Outbound_SA_Real
*       IPSEC_Get_Outbound_SA
*       IPSEC_Free_Outbound_SA
*       IPSEC_Get_Outbound_SA_By_Index
*       IPSEC_Add_Inbound_SA_Real
*       IPSEC_Get_Inbound_SA
*       IPSEC_Free_Inbound_SA
*       IPSEC_Remove_In_SA_By_SPI
*       IPSEC_Remove_In_SA_By_Selector
*       IPSEC_Cmp_Index
*       IPSEC_Cmp_Inbound_SAs
*       IPSEC_Add_SA_Pair
*       IPSEC_Get_Inbound_SA_By_Addr
*       IPSEC_Get_Outbound_SA_By_Addr
*       IPSEC_Remove_SA_Pair
*       IPSEC_Remove_SAs_By_Addr
*       IPSEC_Check_Initial_Contact
*       IPSEC_Rehash_Outbound_SAs
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*       ike_cfg.h
*       rand.h
*       evp.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"
#include "networking/ips_esn.h"
#include "openssl/evp.h"
#include "openssl/rand.h"

#if (INCLUDE_IKE == NU_TRUE)
#include "networking/ike_cfg.h"

#if (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE)
/* Local function prototypes. */
STATIC STATUS IPSEC_Get_Inbound_SA_By_Addr(IPSEC_INBOUND_SA **sa_ptr,
                                  IPSEC_SINGLE_IP_ADDR *local_addr,
                                  IPSEC_SINGLE_IP_ADDR *remote_addr);

STATIC STATUS IPSEC_Get_Outbound_SA_By_Addr(IPSEC_OUTBOUND_SA **sa_ptr,
                                  IPSEC_SINGLE_IP_ADDR *local_addr,
                                  IPSEC_SINGLE_IP_ADDR *remote_addr);
#endif

#endif

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_Outbound_SA_Real
*
* DESCRIPTION
*
*       Actual function which adds a new SA to the list of outbound SAs.
*       IPsec semaphore should be obtained before calling this routine.
*
* INPUTS
*
*       *group_name             Name of the group.
*       *sa_entry               The outbound SA to be added.
*       **return_sa_ptr         Pointer to SA in SADB.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*
*       NU_TIMEOUT              The operation timed out.
*       NU_NO_MEMORY            There was not enough memory to satisfy
*                               this request.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
        IPSEC_CRYPTO_ERROR      Failure in crypto routine.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Outbound_SA_Real(CHAR *group_name,
                                  IPSEC_OUTBOUND_SA *sa_entry,
                                  IPSEC_OUTBOUND_SA **return_sa_ptr)
{
    STATUS              status;
    UINT8               iv_len = 0;
    UINT16              mem_size;
    UINT16              auth_key_len;
    UINT16              encrypt_key_len = 0;
    IPSEC_POLICY_GROUP  *group_ptr;
    IPSEC_OUTBOUND_SA   *out_sa;
    const EVP_CIPHER    *cipher_type;

    /* Also verify the authentication algorithm ID and replace it with the
    index of the authentication algorithm from the global array of
    authentication algorithms. */
    status = IPSEC_Get_Auth_Algo_Index(
                        &(sa_entry->ipsec_security.ipsec_auth_algo));

    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        if (sa_entry->ipsec_security.ipsec_protocol == IPSEC_ESP)
        {
            /* Verify the algorithm ID and replace it with the index
               of the algorithm from the global array of encryption
               algorithms. */
            status  = IPSEC_Get_Encrypt_Algo_Index(&sa_entry->
                                    ipsec_security.ipsec_encryption_algo);
        }

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* First get the group */
            status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* First calculate the length of encryption keys. */
                if(sa_entry->ipsec_security.ipsec_protocol == IPSEC_ESP)
                {
#if (IPSEC_INCLUDE_NULL_ENCRYPTION == NU_TRUE)
                    /* If null encryption is being used. */
                    if(IPSEC_GET_ENCRYPT_ID(
                            sa_entry->ipsec_security.ipsec_encryption_algo)
                            == IPSEC_NULL_CIPHER)
                    {
                        encrypt_key_len = 0;
                        iv_len = 0;
                    }
                    else
#endif
                    {
                        cipher_type = (EVP_CIPHER*)IPSEC_Encryption_Algos[
                            sa_entry->ipsec_security.ipsec_encryption_algo].
                                evp_cipher();

                        /* Get encryption key length. */
                        encrypt_key_len = EVP_CIPHER_key_length(cipher_type);

                        /* If some key length is present. */
                        if(encrypt_key_len > 0)
                        {
                            /* Get the IV length which is always equal to the
                            block size of the encryption algorithm. */
                            iv_len = EVP_CIPHER_iv_length(cipher_type);

                            /* Increment it by size of encryption request. */
                            encrypt_key_len += (sizeof(IPSEC_ENCRYPT_REQ) +
                                                iv_len);
                        }
                    }
                }

                /* Get authentication key length. Authentication key
                 * length will be present in both cases of AH and ESP
                 *  protocols.
                 */
                auth_key_len = IPSEC_GET_AUTH_KEY_LEN(
                                sa_entry->ipsec_security.ipsec_auth_algo);

                /* Get the memory size to allocate. */
                mem_size = sizeof(IPSEC_OUTBOUND_SA) +
                           auth_key_len + encrypt_key_len;

                /* Allocate the memory for the SA. */
                status = NU_Allocate_Memory(IPSEC_Memory_Pool,
                            (VOID **)&out_sa, mem_size, NU_NO_SUSPEND);

                /* Check the status. */
                if(status == NU_SUCCESS)
                {
                    /* Normalize the pointer first. */
                    out_sa = TLS_Normalize_Ptr(out_sa);

                    /* Now copy the SA to the newly created memory. */
                    NU_BLOCK_COPY(out_sa, sa_entry,
                                    sizeof(IPSEC_OUTBOUND_SA));

                    /* Initialize internal SA members. */
                    out_sa->ipsec_flink = NU_NULL;

                    /* Only if authentication key is present. */
                    if(auth_key_len != 0)
                    {
                        /* Assigning the memory to the keys. */
                        out_sa->ipsec_auth_key = ((UINT8*)out_sa) +
                                                sizeof(IPSEC_OUTBOUND_SA);

                        /* Copy the authentication keys. */
                        NU_BLOCK_COPY(out_sa->ipsec_auth_key,
                                    sa_entry->ipsec_auth_key,
                                      auth_key_len);
                    }

                    /* Only if encryption key is present. */
                    if(encrypt_key_len != 0)
                    {
                        /* Assign memory to the encryption key. */
                        out_sa->ipsec_encryption_key = ((UINT8*)out_sa) +
                                 sizeof(IPSEC_OUTBOUND_SA) + auth_key_len;

                        /* Copy the encryption keys. */
                        NU_BLOCK_COPY(out_sa->ipsec_encryption_key,
                                    sa_entry->ipsec_encryption_key,
                            (encrypt_key_len -
                                    (sizeof(IPSEC_ENCRYPT_REQ) + iv_len)));

                        /* Now assign the memory to encryption request as
                           well. */
                        out_sa->ipsec_encrypt_req = (IPSEC_ENCRYPT_REQ*)
                                          (out_sa->ipsec_encryption_key +
                             (encrypt_key_len -
                                    (sizeof(IPSEC_ENCRYPT_REQ) + iv_len)));

                        /* Also assign the memory to IV. */
                        out_sa->ipsec_encrypt_req->cipher_iv =
                                (((UINT8 *)out_sa->ipsec_encrypt_req) +
                                sizeof(IPSEC_ENCRYPT_REQ));

                        status =
                            NUCLEUS_STATUS(RAND_bytes(out_sa->ipsec_encrypt_req->cipher_iv,
                                       iv_len));

                        /* First get some random data for initial IV. */
                        if(status != NU_SUCCESS)
                        {
                            /* Failed to generate random no. */
                            NLOG_Error_Log(
                                "Failed to generate random no",
                                NERR_SEVERE, __FILE__, __LINE__);

                            /* Deallocate memory. */
                            NU_Deallocate_Memory(out_sa);

                            return (status);
                        }

                        /* Initialize the encrypt request structure. */
                        out_sa->ipsec_encrypt_req->operation =
                                                            ENCRYPT;
                        out_sa->ipsec_encrypt_req->cipher_key =
                                            out_sa->ipsec_encryption_key;
                        out_sa->ipsec_encrypt_req->ipsec_algo_index = out_sa->
                                    ipsec_security.ipsec_encryption_algo;
                    }
                    else
                    {
                        /* Otherwise, set encryption key and request
                         * pointers to NU_NULL.
                         */
                        out_sa->ipsec_encryption_key = NU_NULL;
                        out_sa->ipsec_encrypt_req    = NU_NULL;
                    }

                    /* Initializing the sequence number. We assume ESN is
                     * disabled, if this is an ESN SA this will be set
                     * to true later.
                     */
                    IPSEC_SEQ_INIT(out_sa->ipsec_seq_num, NU_FALSE);

                    /* Increment the SA index. */
                    ++(group_ptr->ipsec_outbound_sa_list.
                        ipsec_next_sa_index);

                    /* Make sure index has not wrapped. */
                    if(group_ptr->ipsec_outbound_sa_list.
                        ipsec_next_sa_index == 0)
                    {
                        /* Index must always be non-zero. */
                        ++(group_ptr->ipsec_outbound_sa_list.
                            ipsec_next_sa_index);
                    }

                    /* Assigning the index and incrementing the group
                        SA index counter. */
                    out_sa->ipsec_index = (group_ptr->
                        ipsec_outbound_sa_list.ipsec_next_sa_index);

                    /* Append the SA onto the group SADB list. */
                    SLL_Push(&(group_ptr->ipsec_outbound_sa_list), out_sa);

                    /* Also return SA pointer to the caller. */
                    *return_sa_ptr = out_sa;
                }
            }
        }
    }

    /* Return the status value. */
    return(status);

} /* IPSEC_Add_Outbound_SA_Real */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Get_Outbound_SA
*
* DESCRIPTION
*
*       Returns a pointer to the SA corresponding to the passed selector
*       and security protocol.
*
* INPUTS
*
*       *group_ptr              Pointer to IPsec policy group.
*       *selector               Pointer to the selector passed
*       *security               Pointer to the security passed
*       **sa_ptr                This is the first SA from which to start
*                               the search. If this points to a NULL
*                               pointer, then search starts from the first
*                               item of the database. On return this
*                               contains a pointer to a matching SA.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       IPSEC_NOT_FOUND         Corresponding group or SA not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Outbound_SA(IPSEC_POLICY_GROUP *group_ptr,
                             IPSEC_SELECTOR *selector,
                             IPSEC_SECURITY_PROTOCOL *security,
                             IPSEC_OUTBOUND_SA **sa_ptr)
{
    STATUS              status = IPSEC_NOT_FOUND;
    IPSEC_OUTBOUND_SA   *sa_list;
    IPSEC_SELECTOR      pkt_selector;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_ptr == NU_NULL) || (selector == NU_NULL) ||
       (security  == NU_NULL) || (sa_ptr   == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Copy selector passed by caller to local selector. */
    NU_BLOCK_COPY(&pkt_selector, selector, sizeof(IPSEC_SELECTOR));

    /* If no starting point specified for the search. */
    if(*sa_ptr == NU_NULL)
    {
        /* Start search from the first item in the database. */
        sa_list = group_ptr->ipsec_outbound_sa_list.ipsec_head;
    }
    else
    {
        /* Start search from the caller specified SA. */
        sa_list = *sa_ptr;
    }

    /* Find the desired outbound SA by traversing the whole SADB. */
    for(;
        sa_list != NU_NULL;
        sa_list = sa_list->ipsec_flink)
    {
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If current SA specifies tunnel mode. */
        if(sa_list->ipsec_security.ipsec_security_mode ==
            IPSEC_TUNNEL_MODE)
        {
            /* Compare the two selectors. */
            if(IPSEC_Match_Selectors(&(sa_list->ipsec_select),
                                     &pkt_selector, NU_FALSE) == NU_TRUE)
            {
                /* Now compare the two security protocols, as it is
                 * possible that many SAs can satisfy a single selector.
                 */
                if(IPSEC_MATCH_SEC_PROT(&(sa_list->ipsec_security),
                                              security) == NU_TRUE)
                {
                    /* We found the desired outbound SA, cheers. */
                    *sa_ptr = sa_list;

                    /* Mark the status as true. */
                    status = NU_SUCCESS;

                    /* Now break the loop. */
                    break;
                }
            }
        }
        else
#endif
        {
            /* Compare the two selectors. */
            if(IPSEC_Match_Selectors(&(sa_list->ipsec_select),
                                     &pkt_selector, NU_FALSE) == NU_TRUE)
            {
                /* Now compare the two security protocols, as it is
                 * possible that many SAs can satisfy a single selector.
                 */
                if(IPSEC_MATCH_SEC_PROT(&(sa_list->ipsec_security),
                                              security) == NU_TRUE)
                {
                    /* We found the desired outbound SA, cheers. */
                    *sa_ptr = sa_list;

                    /* Mark the status as true. */
                    status = NU_SUCCESS;

                    /* Now break the loop. */
                    break;
                }
            }
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Outbound_SA */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Free_Outbound_SA
*
* DESCRIPTION
*
*       This function deallocates all dynamically allocated members
*       of an outbound SA. It is called when an SA is removed from
*       the SADB. The memory in which the SA itself is stored
*       is also deallocated. Therefore the pointer passed to this
*       function must point to a dynamically allocated SA.
*
* INPUTS
*
*       *sa_ptr                 Pointer to SA which is to
*                               be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successful.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Free_Outbound_SA(IPSEC_OUTBOUND_SA *sa_ptr)
{
    STATUS              status = NU_SUCCESS;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(sa_ptr == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Deallocate the SA memory. */
    status = NU_Deallocate_Memory(sa_ptr);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate outbound SA memory",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Free_Outbound_SA */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Get_Outbound_SA_By_Index
*
* DESCRIPTION
*
*       Returns a pointer to the SA and the group corresponding to the
*       index passed.
*
* INPUTS
*
*       sa_index                Index of the required SA.
*       *sa_start               Pointer to the starting outbound SA.
*       **sa_ptr                Pointer to found outbound SA.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       IPSEC_NOT_FOUND         The group or SA was not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Outbound_SA_By_Index(UINT32 sa_index,
                                      IPSEC_OUTBOUND_SA *sa_start,
                                      IPSEC_OUTBOUND_SA **sa_ptr)
{
    STATUS                  status = IPSEC_NOT_FOUND;
    IPSEC_OUTBOUND_SA       *sa_list;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((sa_start == NU_NULL) || (sa_ptr == NU_NULL))
        return (IPSEC_INVALID_PARAMS);

#endif

    /* Make sure passed SA index is a valid one. */
    if(sa_index > 0)
    {
        /* Find the required SA by index by traversing the outbound
           SADB. */
        for(sa_list = sa_start; sa_list != NU_NULL;
            sa_list = sa_list->ipsec_flink)
        {
            /* Compare the two indices. */
            if(sa_list->ipsec_index == sa_index)
            {
                /* Assign the found SA. */
                *sa_ptr = sa_list;

                /* Successful operation. */
                status = NU_SUCCESS;
                break;
            }
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Outbound_SA_By_Index */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_Inbound_SA_Real
*
* DESCRIPTION
*
*       This function verifies a new inbound SA to be added to the
*       list of inbound SAs. IPsec semaphore should be obtained before
*       calling this routine.
*
* INPUTS
*       *group_name             Name of the group.
*       *sa_entry               The inbound SA to be added.
*       **return_sa_ptr         On return, contains pointer
*                               to SA in the SADB.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       NU_NO_MEMORY            There was not enough memory to satisfy
*                               this request.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_Inbound_SA_Real(CHAR *group_name,
                                 IPSEC_INBOUND_SA *sa_entry,
                                 IPSEC_INBOUND_SA **return_sa_ptr)
{
    STATUS              status;
    UINT16              auth_key_len;
    UINT16              encrypt_key_len = 0;
    UINT16              mem_size;
    IPSEC_POLICY_GROUP  *group_ptr;
    IPSEC_INBOUND_SA    *in_sa;


#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((sa_entry == NU_NULL) || (return_sa_ptr == NU_NULL))
        return (IPSEC_INVALID_PARAMS);

#endif

    /* First get the group */
    status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        /* First calculate the length of encryption keys. */
        if(sa_entry->ipsec_security.ipsec_protocol == IPSEC_ESP)
        {
            /* Get encryption key length. */
            encrypt_key_len = IPSEC_GET_ENCRYPT_KEY_LEN(sa_entry->
                                ipsec_security.ipsec_encryption_algo);

            /* If some key length is present. */
            if(encrypt_key_len > 0)
            {
                /* Increment it by size of encryption request. */
                encrypt_key_len += sizeof(IPSEC_ENCRYPT_REQ);
            }
        }

        /* Get authentication key length. Authentication key length will
           be present in both cases of AH and ESP protocols. */
        auth_key_len = IPSEC_GET_AUTH_KEY_LEN(
                                sa_entry->ipsec_security.ipsec_auth_algo);

        /* Get the memory size to allocate. */
        mem_size = sizeof(IPSEC_INBOUND_SA) +
                   auth_key_len + encrypt_key_len;

        /* Allocate the memory for the SA. */
        status = NU_Allocate_Memory(IPSEC_Memory_Pool, (VOID **)&in_sa,
                                    mem_size, NU_NO_SUSPEND);
        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* First normalize the pointer. */
            in_sa = TLS_Normalize_Ptr(in_sa);

            /* Now copy the SA to the newly created memory. */
            NU_BLOCK_COPY(in_sa, sa_entry, sizeof(IPSEC_INBOUND_SA));

            /* Initialize internal SA member. */
            in_sa->ipsec_flink = NU_NULL;

            /* Zero out the anti-replay window structure. */
            UTL_Zero(&(in_sa->ipsec_antireplay),
                     sizeof(IPSEC_ANTIREPLAY_WIN));

            /* To start off, we set the high bound of the anti-replay
             * window to be one less than the window size (in bits).
             * This allows the current window to start from 0 and
             * be available till the high bound.
             */
            in_sa->ipsec_antireplay.ipsec_last_seq.ipsec_low_order =
                IPSEC_SLIDING_WINDOW_RANGE - 1;

            /* Only if authentication key is present. */
            if(auth_key_len != 0)
            {
                /* Assigning the memory to the keys. */
                in_sa->ipsec_auth_key = ((UINT8*)in_sa) +
                                        sizeof(IPSEC_INBOUND_SA);

                /* Copy the authentication keys. */
                NU_BLOCK_COPY(in_sa->ipsec_auth_key,
                              sa_entry->ipsec_auth_key,
                              auth_key_len);
            }
            else
            {
                /* Otherwise, set key pointer to NU_NULL. */
                in_sa->ipsec_auth_key = NU_NULL;
            }

            /* Only if encryption key is present. */
            if(encrypt_key_len != 0)
            {
                /* Assign memory to the encryption key. */
                in_sa->ipsec_encryption_key = ((UINT8*)in_sa) +
                                sizeof(IPSEC_INBOUND_SA) + auth_key_len;

                /* Copy the encryption keys. */
                NU_BLOCK_COPY(in_sa->ipsec_encryption_key,
                           sa_entry->ipsec_encryption_key,
                           (encrypt_key_len - sizeof(IPSEC_ENCRYPT_REQ)));

                /* Now assign the memory to encryption request as well. */
                in_sa->ipsec_encrypt_req = (IPSEC_ENCRYPT_REQ*)
                                           (in_sa->ipsec_encryption_key +
                            (encrypt_key_len - sizeof(IPSEC_ENCRYPT_REQ)));

                /* Build the encryption request. */
                in_sa->ipsec_encrypt_req->operation = DECRYPT;
                in_sa->ipsec_encrypt_req->ipsec_algo_index = in_sa->
                                    ipsec_security.ipsec_encryption_algo;
                in_sa->ipsec_encrypt_req->cipher_key =
                                            in_sa->ipsec_encryption_key;
            }
            else
            {
                /* Otherwise, set encryption key and request
                 * pointers to NU_NULL.
                 */
                in_sa->ipsec_encryption_key = NU_NULL;
                in_sa->ipsec_encrypt_req    = NU_NULL;
            }

            /* We not need to assign SPI if this call is from IKE
             * as IKE must have assigned one already. Otherwise
             * assign the SPI and increment the group SA
             * index counter respective to security protocol.
             */
            if(sa_entry->ipsec_spi == 0)
            {
                in_sa->ipsec_spi =
                (in_sa->ipsec_security.ipsec_protocol == IPSEC_AH)?
                (group_ptr->ipsec_inbound_sa_list.
                                            ipsec_next_sa_index_ah)++:
                (group_ptr->ipsec_inbound_sa_list.
                                            ipsec_next_sa_index_esp)++;

                /* Make sure we are not exceeding the upper SPI limit. */
                if(in_sa->ipsec_spi >= IPSEC_SPI_END)
                {
                    /* Return the error status. */
                    status = IPSEC_INVALID_SPI;
                }
            }

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* Insert the SA into the group SADB list. */
                status = SLL_Insert_Sorted(&(group_ptr->
                                            ipsec_inbound_sa_list), in_sa,
                                            IPSEC_Cmp_Inbound_SAs);
            }

            /* If the SA has not been successfully added. */
            if(status != NU_SUCCESS)
            {
                /* Free the SA memory. */
                IPSEC_Free_Inbound_SA(in_sa);
            }
            else
            {
                /* Return SA pointer to the caller as well. */
                *return_sa_ptr = in_sa;
            }
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Add_Inbound_SA_Real */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Get_Inbound_SA
*
* DESCRIPTION
*
*       Returns a pointer to the SA corresponding to the passed index
*       tuple.
*
* INPUTS
*
*       *group_ptr              Pointer to the group entry
*       *sa_index               Index of the incoming SA.
*       **sa_ptr                Pointer to the SA.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       IPSEC_NOT_FOUND         Corresponding group or SA not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Inbound_SA(IPSEC_POLICY_GROUP *group_ptr,
                            IPSEC_INBOUND_INDEX *sa_index,
                            IPSEC_INBOUND_SA **sa_ptr)
{
    STATUS              status = IPSEC_NOT_FOUND;
    UINT8               address_size;
    IPSEC_INBOUND_SA    *in_sa_list;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_ptr == NU_NULL) || (sa_index == NU_NULL)||
                                 (sa_ptr   == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Make sure passed group is not null. */
    if(group_ptr != NU_NULL)
    {
        /* Get the inbound SADB. */
        in_sa_list = group_ptr->ipsec_inbound_sa_list.ipsec_head;

        /* Find the required SA from the SADB by traversing the SADB. */
        for(;in_sa_list != NU_NULL;
            in_sa_list = in_sa_list->ipsec_flink)
        {
            /* Compare the two SPIs. */
            if(in_sa_list->ipsec_spi == sa_index->ipsec_spi)
            {
                /* Compare the security protocols. */
                if(in_sa_list->ipsec_security.ipsec_protocol ==
                                                sa_index->ipsec_protocol)
                {
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                    /* Get the no of address bytes to compare. */
                    if(in_sa_list->ipsec_select.ipsec_source_type &
                                                            IPSEC_IPV4)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                        address_size = IP_ADDR_LEN;
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                    else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                        address_size = IP6_ADDR_LEN;
#endif

                    /* Check if this is a tunneled packet. */
                    if(in_sa_list->ipsec_security.ipsec_security_mode ==
                                                        IPSEC_TUNNEL_MODE)
                    {
                        /* Compare the tunnel destination address. */
                        if(memcmp(in_sa_list->ipsec_security.
                                    ipsec_tunnel_destination,
                                sa_index->ipsec_dest, address_size) == 0)
                        {
                            /* Mark the flag as matched. */
                            status = NU_TRUE;
                        }
                    }
                    else
                    {
                        /* Compare the destination address with the
                         * inbound SA selector destination address.
                         */
                        if(memcmp(in_sa_list->ipsec_select.ipsec_dest_ip.
                                    ipsec_addr, sa_index->ipsec_dest,
                                                    address_size) == 0)
                        {
                            /* Mark the flag as matched. */
                            status = NU_TRUE;
                        }
                    }

                    /* Check if we have got the SA or not. */
                    if(status == NU_TRUE)
                    {
                        /* Now assign to the parameter passed. */
                        *sa_ptr = in_sa_list;

                        /* Mark the status as true. */
                        status = NU_SUCCESS;

                        /* Now break the loop. */
                        break;
                    }
                }
            }
            /* Otherwise, if SPI of current SA is greater than
             * the SPI being searched, then the required SA
             * does not exist in the database.
             */
            else if(in_sa_list->ipsec_spi > sa_index->ipsec_spi)
            {
                break;
            }
        } /* End of for loop. */
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Inbound_SA */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Free_Inbound_SA
*
* DESCRIPTION
*
*       This function deallocates all dynamically allocated members
*       of an inbound SA. It is called when an SA is removed from
*       the SADB. The memory in which the SA itself is stored
*       is also deallocated. Therefore the pointer passed to this
*       function must point to a dynamically allocated SA.
*
* INPUTS
*
*       *sa_ptr                 Pointer to SA which is to be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successful.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Free_Inbound_SA(IPSEC_INBOUND_SA *sa_ptr)
{

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(sa_ptr == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Finally, deallocate the SA memory. */
    if(NU_Deallocate_Memory(sa_ptr) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate inbound SA memory",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status value. */
    return (NU_SUCCESS);

} /* IPSEC_Free_Inbound_SA */

/************************************************************************
* FUNCTION
*
*       IPSEC_Remove_In_SA_By_SPI
*
* DESCRIPTION
*
*       This function removes inbound SAs as specified by the SPI.
*
* INPUTS
*
*       *group_name             Name of the group from which the required
*                               SA is to be deleted.
*       sa_spi                  SPI of the desired SA.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_In_SA_By_SPI(CHAR *group_name, UINT32 sa_spi)
{
    STATUS              status;
    IPSEC_INBOUND_SA    *inbound_sa;
    IPSEC_POLICY_GROUP  *group;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameter. */
    if(group_name == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First grab the semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    /* Check the status value. */
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* First get the group pointer from the group name passed. */
        status = IPSEC_Get_Group_Entry(group_name, &group);

        /* Check the status value. */
        if(status == NU_SUCCESS)
        {
            /* Now search the inbound SADB of this group for the given
               SPI. */
            for(inbound_sa = group->ipsec_inbound_sa_list.ipsec_head;
                inbound_sa != NU_NULL;
                inbound_sa = inbound_sa->ipsec_flink)
            {
                /* Match the passed SPI. */
                if(sa_spi == inbound_sa->ipsec_spi)
                {
                    /* Remove it from the inbound SADB. */
                    SLL_Remove(&(group->ipsec_inbound_sa_list),
                               inbound_sa);

                    /* Also deallocated its memory. */
                    IPSEC_Free_Inbound_SA(inbound_sa);

                    /* Break the loop as well. */
                    break;
                }
            }

            /* If we have found the desired SA then remove it. */
            if(inbound_sa == NU_NULL)
            {
                /* Required SA has not been found. */
                status = IPSEC_NOT_FOUND;
            }
        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Remove_In_SA_By_SPI */

/************************************************************************
* FUNCTION
*
*       IPSEC_Remove_In_SA_By_Selector
*
* DESCRIPTION
*
*       This function removes all the inbound SAs which have the same
*       selector as passed selector.
*
* INPUTS
*
*       *group_name             Name of the group from which the required
*                               SAs is to be deleted
*       *selector               Selector to be matched.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_In_SA_By_Selector(CHAR *group_name,
                                      IPSEC_SELECTOR *selector)
{
    STATUS              status;
    IPSEC_INBOUND_SA    *inbound_sa;
    IPSEC_INBOUND_SA    *inbound_sa_next;
    IPSEC_POLICY_GROUP  *group;


#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_name == NU_NULL) || (selector == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First grab the semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    /* Check the status value. */
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* First get the group pointer from the group name passed. */
        status = IPSEC_Get_Group_Entry(group_name, &group);

        /* Check the status value. */
        if(status == NU_SUCCESS)
        {
            /* Now search the inbound SADB of this group for the given
               selector and remove all the SAs which match the passed
               selector. */
            for(inbound_sa = group->ipsec_inbound_sa_list.ipsec_head;
                inbound_sa != NU_NULL;)
            {
                /* Get the next SADB. */
                inbound_sa_next = inbound_sa->ipsec_flink;

                /* Match the two selectors. */
                if(IPSEC_Match_Selectors(&(inbound_sa->ipsec_select),
                                      selector, NU_TRUE) == NU_TRUE)
                {
                    /* Remove it from the inbound SADB. */
                    SLL_Remove(&(group->ipsec_inbound_sa_list),
                               inbound_sa);

                    /* Also deallocated its memory. */
                    IPSEC_Free_Inbound_SA(inbound_sa);
                }

                /* Continue from next SADB. */
                inbound_sa = inbound_sa_next;
            }
        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Remove_In_SA_By_Selector */

/************************************************************************
* FUNCTION
*
*       IPSEC_Cmp_Index
*
* DESCRIPTION
*
*       This is an internal utility function used for comparison
*       of two values based on a specified sorting index.
*
* INPUTS
*
*       a_type                  First value to be compared.
*       b_type                  Second value to be compared.
*       *index                  Pointer to the sorting index.
*       index_length            Length of the index passed.
*
* OUTPUTS
*
*       -1                      If a_type < b_type.
*       1                       If a_type > b_type.
*       0                       If a_type == b_type.
*
************************************************************************/
INT IPSEC_Cmp_Index(UINT8 a_type, UINT8 b_type, UINT8 *index,
                    UINT8 index_length)
{
    UINT8   i;
    INT     result = 0;

    /* Do proceed the following if they are not equal. */
    if(a_type != b_type)
    {
        /* Search the 'order' array starting from the
         * first element. Whichever type is found first,
         * that selector must come before the other.
         */
        for(i = 0; i < index_length; i++)
        {
            /* If type of selector 'b' found. */
            if(b_type == index[i])
            {
                /* Return b < a. */
                result = 1;
                break;
            }
            /* If type of selector 'b' found. */
            else if(a_type == index[i])
            {
                /* Return a < b. */
                result = -1;
                break;
            }
        } /* End for loop. */
    }

    /* Now return the result. */
    return (result);

} /* IPSEC_Cmp_Index */

/************************************************************************
* FUNCTION
*
*       IPSEC_Cmp_Inbound_SAs
*
* DESCRIPTION
*
*       This is an internal utility function used for comparison
*       of two inbound SAs. This function never returns zero
*       because duplicate SA entries are allowed in the SADB.
*
* INPUTS
*
*       *new_sa                 Pointer to the first node of type
*                               SA pointer.
*       *list_sa                Pointer to the second node of type
*                               SA pointer.
*
* OUTPUTS
*
*       -1                      If new_sa <= list_sa.
*       1                       If new_sa > list_sa.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
INT IPSEC_Cmp_Inbound_SAs(VOID *new_sa, VOID *list_sa)
{
    INT                 result = 0;

    /* Declare arrays of indices for sorting order. */
    UINT8 des_addr_index[]  = IPSEC_ADDR_INDEX;
    UINT8 sec_prot_index[]  = IPSEC_SEC_PROT_INDEX;

    /* Getting the SAs. */
    IPSEC_INBOUND_SA *a_sa = (IPSEC_INBOUND_SA *)new_sa;
    IPSEC_INBOUND_SA *b_sa = (IPSEC_INBOUND_SA *)list_sa;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((new_sa == NU_NULL) || (list_sa == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Check if SPIs are not equal. */
    if(a_sa->ipsec_spi != b_sa->ipsec_spi)
    {
        /* Check if sa_a spi is greater then sa_b spi. */
        result = (a_sa->ipsec_spi < b_sa->ipsec_spi) ? -1 : 1;
    }

    if(result == 0)
    {
        /* Now compare the two destination addresses. */
        result = IPSEC_Cmp_Index(a_sa->ipsec_select.ipsec_dest_type,
                                 b_sa->ipsec_select.ipsec_dest_type,
                                des_addr_index, sizeof(des_addr_index));
        if(result == 0)
        {
            /* Compare the two IPsec protocols. */
            result = IPSEC_Cmp_Index(a_sa->ipsec_security.ipsec_protocol,
                                     b_sa->ipsec_security.ipsec_protocol,
                                  sec_prot_index, sizeof(sec_prot_index));
        }
    }

    /* Return the comparison result. */
    return (result);

} /* IPSEC_Cmp_Inbound_SAs */

#if (INCLUDE_IKE == NU_TRUE)

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Add_SA_Pair
*
* DESCRIPTION
*
*       This function is used by IKE to add an IPsec SA pair. The
*       lifetime is shared by both inbound and outbound SAs, so
*       this function is responsible for setting the outbound SA's
*       lifetime pointer to reference inbound SA's lifetime.
*
* INPUTS
*
*       if_index                Index of device for which
*                               the SAs are to be added.
*       *out_sa                 Pointer to outbound SA.
*       *in_sa                  Pointer to inbound SA.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       NU_NO_MEMORY            There was not enough memory to satisfy
*                               this request.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_SA_Pair(UINT32 if_index, IPSEC_OUTBOUND_SA *out_sa,
                         IPSEC_INBOUND_SA *in_sa)
{
    STATUS                  status;
    IPSEC_OUTBOUND_SA       *out_sa_ptr;
    IPSEC_INBOUND_SA        *in_sa_ptr;
    CHAR                    *group_name = NU_NULL;
    DV_DEVICE_ENTRY         *dev_entry;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((out_sa == NU_NULL) || (in_sa == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First grab the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Now grab the IPsec semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* Look-up the device by index. */
            dev_entry = DEV_Get_Dev_By_Index(if_index);

            /* If device is found. */
            if(dev_entry != NU_NULL)
            {
                /* If device is part of an IPsec group. */
                if(dev_entry->dev_physical->dev_phy_ips_group != NU_NULL)
                {
                    /* Get pointer to group name. */
                    group_name = dev_entry->dev_physical->
                                     dev_phy_ips_group->ipsec_group_name;
                }
                else
                {
                    /* Device is not registered with any IPsec group. */
                    status = IPSEC_NOT_FOUND;
                }
            }
            else
            {
                /* Device not found. */
                status = IPSEC_NOT_FOUND;
            }

            /* If no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* First add outbound SA to SADB. */
                status = IPSEC_Add_Outbound_SA_Real(group_name, out_sa,
                                                    &out_sa_ptr);
                if(status == NU_SUCCESS)
                {
                    /* First set the flag that this SA has been
                       established by IKE. */
                    in_sa->ipsec_soft_lifetime.ipsec_flags |=
                                                            IPSEC_IKE_SA;

                    /* Now add the corresponding inbound SA as well. */
                    status = IPSEC_Add_Inbound_SA_Real(group_name, in_sa,
                                                       &in_sa_ptr);

                    /* Check the status value. */
                    if(status == NU_SUCCESS)
                    {
                        /* Set SA pointers in hard lifetime. */
                        in_sa_ptr->ipsec_hard_lifetime.ipsec_in_sa  =
                                                            in_sa_ptr;
                        in_sa_ptr->ipsec_hard_lifetime.ipsec_out_sa =
                                                            out_sa_ptr;

                        /* Set SA pointers in soft lifetime. */
                        in_sa_ptr->ipsec_soft_lifetime.ipsec_out_sa =
                                                            out_sa_ptr;
                        in_sa_ptr->ipsec_soft_lifetime.ipsec_in_sa  =
                                                            in_sa_ptr;

                        /* Set lifetime pointers in outbound SA. */
                        out_sa_ptr->ipsec_hard_lifetime =
                                        &(in_sa_ptr->ipsec_hard_lifetime);
                        out_sa_ptr->ipsec_soft_lifetime =
                                        &(in_sa_ptr->ipsec_soft_lifetime);

                        /* Set hard lifetime timer event for
                         * these SAs, if required.
                         */
                        if((in_sa->ipsec_hard_lifetime.ipsec_no_of_secs
                                                                > 0) &&
                           (in_sa->ipsec_hard_lifetime.ipsec_expiry_action
                                                                != 0))
                        {
                            /* Set hard lifetime for these SAs. */
                            status = TQ_Timerset(IPSEC_Hard_Lifetime_Event,
                                        (UNSIGNED)&in_sa_ptr->
                                            ipsec_hard_lifetime,
                                        (in_sa_ptr->ipsec_hard_lifetime.
                                            ipsec_no_of_secs *
                                            TICKS_PER_SECOND),
                                        (UNSIGNED)if_index);
                        }

                        /* Set soft lifetime timer event for
                         * these SAs, if required.
                         */
                        if((status == NU_SUCCESS) &&
                           (in_sa->ipsec_soft_lifetime.ipsec_no_of_secs
                                                                > 0) &&
                           (in_sa->ipsec_soft_lifetime.ipsec_expiry_action
                                                                != 0))
                        {
                            /* Now set soft lifetime for these SAs. */
                            status = TQ_Timerset(IPSEC_Soft_Lifetime_Event,
                                        (UNSIGNED)&in_sa_ptr->
                                            ipsec_soft_lifetime,
                                        (in_sa_ptr->ipsec_soft_lifetime.
                                            ipsec_no_of_secs *
                                            TICKS_PER_SECOND),
                                        (UNSIGNED)if_index);
                        }
                    }
                }
            }

            /* Release IPsec semaphore. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the IPsec semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        /* Release TCP semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the TCP semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the status value to the caller. */
    return (status);

} /* IPSEC_Add_SA_Pair */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Remove_SA_Pair
*
* DESCRIPTION
*
*       This function removes an outbound SA as specified by the
*       index. The corresponding inbound SA of the SA pair is
*       also removed.
*
* INPUTS
*
*       *index                  Unique identifier of the outbound SA which
*                               is to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The SA or group not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_SA_Pair(IPSEC_OUTBOUND_INDEX_REAL *index)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group_ptr;
    IPSEC_OUTBOUND_SA   *out_sa_ptr = NU_NULL;
    IPSEC_INBOUND_SA    *in_sa_ptr = NU_NULL;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if(index == NU_NULL)
    {
        return (IPSEC_INVALID_PARAMS);
    }
    /* Make sure all members within the SA index are valid. */
    else if((index->ipsec_group == NU_NULL) ||
            (index->ipsec_dest  == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* First get the group pointer. */
        status = IPSEC_Get_Group_Entry(index->ipsec_group, &group_ptr);

        /* Check the status value. */
        if(status == NU_SUCCESS)
        {
            /* Initialize status to failure. */
            status = IPSEC_NOT_FOUND;

            /* Find outbound SA by traversing the whole SADB. */
            for(out_sa_ptr = group_ptr->ipsec_outbound_sa_list.ipsec_head;
                out_sa_ptr != NU_NULL;
                out_sa_ptr = out_sa_ptr->ipsec_flink)
            {
                /* Check if the SPI matches. */
                if(out_sa_ptr->ipsec_remote_spi == index->ipsec_spi)
                {
                    /* Check if the security protocol matches. */
                    if(out_sa_ptr->ipsec_security.ipsec_protocol ==
                                                index->ipsec_protocol)
                    {
                        /* Check if the destination address matches. */
                        if(memcmp(out_sa_ptr->ipsec_select.ipsec_dest_ip.
                                  ipsec_addr, index->ipsec_dest,

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                                  (index->ipsec_dest_type & IPSEC_IPV6) ?
                                  IP6_ADDR_LEN : IP_ADDR_LEN)
#else
#if (INCLUDE_IPV4 == NU_TRUE)
                                    IP_ADDR_LEN)
#else
                                    IP6_ADDR_LEN)
#endif
#endif
                                    == 0)
                        {
                            /* Remove SA from the SADB. */
                            SLL_Remove(&group_ptr->ipsec_outbound_sa_list,
                                       out_sa_ptr);

                            /* If an SA pair is present. */
                            if((out_sa_ptr->ipsec_hard_lifetime != NU_NULL)
                               && (out_sa_ptr->ipsec_hard_lifetime->
                                                ipsec_in_sa != NU_NULL))
                            {
                                /* Set the inbound SA pointer. */
                                in_sa_ptr = out_sa_ptr->
                                    ipsec_hard_lifetime->ipsec_in_sa;

                                /* Remove inbound SA from SADB. */
                                SLL_Remove(&group_ptr->
                                               ipsec_inbound_sa_list,
                                           in_sa_ptr);
                            }

                            /* Indicate successful operation. */
                            status = NU_SUCCESS;

                            /* Break out of search loop. */
                            break;
                        }
                    }
                }
            }
        }

        /* Release the semaphore as it is no longer required. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        if(status == NU_SUCCESS)
        {
            /* Grab the TCP semaphore. */
            status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain TCP semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            else
            {
                /* Remove timer events corresponding to this SA pair. */
                TQ_Timerunset(IPSEC_Soft_Lifetime_Event,
                              TQ_CLEAR_ALL_EXTRA,
                              (UNSIGNED)out_sa_ptr->ipsec_soft_lifetime,
                              0);

                TQ_Timerunset(IPSEC_Hard_Lifetime_Event,
                              TQ_CLEAR_ALL_EXTRA,
                              (UNSIGNED)out_sa_ptr->ipsec_hard_lifetime,
                              0);

                /* Release the TCP semaphore. */
                if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release the semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Release outbound SA memory. */
            IPSEC_Free_Outbound_SA(out_sa_ptr);

            /* If inbound SA is present. */
            if(in_sa_ptr != NU_NULL)
            {
                /* Release the inbound SA memory. */
                IPSEC_Free_Inbound_SA(in_sa_ptr);
            }
        }
    }

    /* Return the status value. */
    return (status);

} /* IPSEC_Remove_SA_Pair */

#if (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Inbound_SA_By_Addr
*
* DESCRIPTION
*
*       This function searches the inbound SADB for an SA
*       which matches the specified local and remote IP
*       addresses. The caller is responsible for obtaining
*       the IPsec semaphore before calling this function.
*       Loops in this function have been slightly unrolled
*       to minimize loop operations since traversing the
*       whole SA list might be quite expensive.
*
* INPUTS
*
*       **sa_ptr                Start search from this SA. On return,
*                               this contains pointer to the matching
*                               SA, if found.
*       *local_addr             Pointer to IP address of the local node.
*                               If NU_NULL, then local address is not
*                               matched.
*       *remote_addr            Pointer to IP address of the remote node.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         SA not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATIC STATUS IPSEC_Get_Inbound_SA_By_Addr(IPSEC_INBOUND_SA **sa_ptr,
                                  IPSEC_SINGLE_IP_ADDR *local_addr,
                                  IPSEC_SINGLE_IP_ADDR *remote_addr)
{
    STATUS              status = IPSEC_NOT_FOUND;
    IPSEC_INBOUND_SA    *inbound_sa;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32              ip4_remote_addr = 0;
    UINT32              ip4_local_addr = 0;
#endif

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((sa_ptr      == NU_NULL) || (*sa_ptr == NU_NULL) ||
       (remote_addr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    /* If the address being searched is IPv4. */
    if((remote_addr->ipsec_type & IPSEC_IPV4) != 0)
    {
        /* Convert the address outside the loops. */
        ip4_remote_addr = IP_ADDR(remote_addr->ipsec_addr);

        /* If local address is specified. */
        if(local_addr != NU_NULL)
        {
            ip4_local_addr = IP_ADDR(local_addr->ipsec_addr);
        }
    }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    /* If this is an IPv4 address. */
    if((remote_addr->ipsec_type & IPSEC_IPV4) != 0)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Loop for all inbound SAs. */
        for(inbound_sa = *sa_ptr;
            inbound_sa != NU_NULL;
            inbound_sa = inbound_sa->ipsec_flink)
        {
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            /* Check if current SA is for IPv4. */
            if(inbound_sa->ipsec_select.ipsec_source_type &
               IPSEC_IPV4)
#endif
            {
                /* Check if the source address matches. */
                if(IP_ADDR(inbound_sa->ipsec_select.
                               ipsec_source_ip.ipsec_addr)
                           == ip4_remote_addr)
                {
                    if(local_addr != NU_NULL)
                    {
                        /* Check if the destination address matches. */
                        if(IP_ADDR(inbound_sa->ipsec_select.
                                       ipsec_dest_ip.ipsec_addr)
                                   == ip4_local_addr)
                        {
                            /* Return current SA pointer to the caller. */
                            *sa_ptr = inbound_sa;

                            status = NU_SUCCESS;
                            break;
                        }
                    }
                    else
                    {
                        /* Return current SA pointer to the caller. */
                        *sa_ptr = inbound_sa;

                        status = NU_SUCCESS;
                        break;
                    }
                }
            }
        }
    }
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    /* Otherwise, an IPv6 address is being searched. */
    else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* Loop for all inbound SAs. */
        for(inbound_sa = *sa_ptr;
            inbound_sa != NU_NULL;
            inbound_sa = inbound_sa->ipsec_flink)
        {
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            /* Check if current SA is for IPv4. */
            if(inbound_sa->ipsec_select.ipsec_source_type &
               IPSEC_IPV4)
#endif
            {
                /* Check if the source address matches. */
                if(memcmp(inbound_sa->ipsec_select.
                              ipsec_source_ip.ipsec_addr,
                          remote_addr->ipsec_addr, IP6_ADDR_LEN) == 0)
                {
                    if(local_addr != NU_NULL)
                    {
                        /* Check if the destination address matches. */
                        if(memcmp(inbound_sa->ipsec_select.ipsec_dest_ip.
                                      ipsec_addr,
                                  local_addr->ipsec_addr,
                                  IP6_ADDR_LEN) == 0)
                        {
                            /* Return current SA pointer to the caller. */
                            *sa_ptr = inbound_sa;

                            status = NU_SUCCESS;
                            break;
                        }
                    }
                    else
                    {
                        /* Return current SA pointer to the caller. */
                        *sa_ptr = inbound_sa;

                        status = NU_SUCCESS;
                        break;
                    }
                }
            }
        }
    }
#endif

    /* Return the status. */
    return (status);

} /* IPSEC_Get_Inbound_SA_By_Addr. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Outbound_SA_By_Addr
*
* DESCRIPTION
*
*       This function searches the outbound SADB for an SA
*       which matches the specified local and remote IP
*       addresses. The caller is responsible for obtaining
*       the IPsec semaphore before calling this function.
*       Loops in this function have been slightly unrolled
*       to minimize loop operations since traversing the
*       whole SA list might be quite expensive.
*
* INPUTS
*
*       **sa_ptr                Start search from this SA. On return,
*                               this contains pointer to the matching
*                               SA, if found.
*       *local_addr             Pointer to IP address of the local node.
*                               If NU_NULL, then local address is not
*                               matched.
*       *remote_addr            Pointer to IP address of the remote node.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         SA not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATIC STATUS IPSEC_Get_Outbound_SA_By_Addr(IPSEC_OUTBOUND_SA **sa_ptr,
                                  IPSEC_SINGLE_IP_ADDR *local_addr,
                                  IPSEC_SINGLE_IP_ADDR *remote_addr)
{
    STATUS              status = IPSEC_NOT_FOUND;
    IPSEC_OUTBOUND_SA   *outbound_sa;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32              ip4_remote_addr = 0;
    UINT32              ip4_local_addr = 0;
#endif

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((sa_ptr      == NU_NULL) || (*sa_ptr == NU_NULL) ||
       (remote_addr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    /* If the address being searched is IPv4. */
    if((remote_addr->ipsec_type & IPSEC_IPV4) != 0)
    {
        /* Convert the address outside the loops. */
        ip4_remote_addr = IP_ADDR(remote_addr->ipsec_addr);

        /* If local address is specified. */
        if(local_addr != NU_NULL)
        {
            ip4_local_addr = IP_ADDR(local_addr->ipsec_addr);
        }
    }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    /* If this is an IPv4 address. */
    if((remote_addr->ipsec_type & IPSEC_IPV4) != 0)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Loop for all inbound SAs. */
        for(outbound_sa = *sa_ptr;
            outbound_sa != NU_NULL;
            outbound_sa = outbound_sa->ipsec_flink)
        {
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            /* Check if current SA is for IPv4. */
            if(outbound_sa->ipsec_select.ipsec_source_type &
               IPSEC_IPV4)
#endif
            {
                /* Check if the destination address matches. */
                if(IP_ADDR(outbound_sa->ipsec_select.
                               ipsec_dest_ip.ipsec_addr)
                           == ip4_remote_addr)
                {
                    if(local_addr != NU_NULL)
                    {
                        /* Check if the source address matches. */
                        if(IP_ADDR(outbound_sa->ipsec_select.
                                       ipsec_source_ip.ipsec_addr)
                                   == ip4_local_addr)
                        {
                            /* Return current SA pointer to the caller. */
                            *sa_ptr = outbound_sa;

                            status = NU_SUCCESS;
                            break;
                        }
                    }
                    else
                    {
                        /* Return current SA pointer to the caller. */
                        *sa_ptr = outbound_sa;

                        status = NU_SUCCESS;
                        break;
                    }
                }
            }
        }
    }
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
    /* Otherwise, an IPv6 address is being searched. */
    else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    {
        /* Loop for all inbound SAs. */
        for(outbound_sa = *sa_ptr;
            outbound_sa != NU_NULL;
            outbound_sa = outbound_sa->ipsec_flink)
        {
#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
            /* Check if current SA is for IPv4. */
            if(outbound_sa->ipsec_select.ipsec_source_type &
               IPSEC_IPV4)
#endif
            {
                /* Check if the destination address matches. */
                if(memcmp(outbound_sa->ipsec_select.
                              ipsec_dest_ip.ipsec_addr,
                          remote_addr->ipsec_addr, IP6_ADDR_LEN) == 0)
                {
                    if(local_addr != NU_NULL)
                    {
                        /* Check if the source address matches. */
                        if(memcmp(outbound_sa->ipsec_select.
                                      ipsec_source_ip.ipsec_addr,
                                  local_addr->ipsec_addr,
                                  IP6_ADDR_LEN) == 0)
                        {
                            /* Return current SA pointer to the caller. */
                            *sa_ptr = outbound_sa;

                            status = NU_SUCCESS;
                            break;
                        }
                    }
                    else
                    {
                        /* Return current SA pointer to the caller. */
                        *sa_ptr = outbound_sa;

                        status = NU_SUCCESS;
                        break;
                    }
                }
            }
        }
    }
#endif

    /* Return the status. */
    return (status);

} /* IPSEC_Get_Outbound_SA_By_Addr */

/************************************************************************
* FUNCTION
*
*       IPSEC_Remove_SAs_By_Addr
*
* DESCRIPTION
*
*       This function deletes any IPsec SAs that are established
*       between the specified addresses. This is used by IKE
*       to remove stale SAs in response to an INITIAL-CONTACT
*       notification. Loops in this function have been slightly
*       unrolled to minimize loop operations since traversing the
*       whole SA list might be quite expensive.
*
* INPUTS
*
*       *group_name             Name of the IPsec group which is to be
*                               searched.
*       *local_addr             Pointer to IP address of the local node.
*       *remote_addr            Pointer to IP address of the remote node.
*
* OUTPUTS
*
*       NU_SUCCESS              If initial contact has occurred.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         Group not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_SAs_By_Addr(CHAR *group_name,
                                IPSEC_SINGLE_IP_ADDR *local_addr,
                                IPSEC_SINGLE_IP_ADDR *remote_addr)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group;
    IPSEC_INBOUND_SA    *inbound_sa;
    IPSEC_OUTBOUND_SA   *outbound_sa;
    VOID                *next_sa;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((group_name  == NU_NULL) || (local_addr  == NU_NULL) ||
       (remote_addr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Get the specified IPsec group pointer. */
        status = IPSEC_Get_Group_Entry(group_name, &group);

        if(status == NU_SUCCESS)
        {
            /* Start search from the first item in the SADB. */
            inbound_sa = group->ipsec_inbound_sa_list.ipsec_head;

            /* Loop until the whole SADB has been traversed. */
            while(inbound_sa != NU_NULL)
            {
                if(IPSEC_Get_Inbound_SA_By_Addr(&inbound_sa, local_addr,
                                              remote_addr) == NU_SUCCESS)
                {
                    /* Save pointer to the next SA. */
                    next_sa = inbound_sa->ipsec_flink;

                    /* Remove SA from SADB. */
                    SLL_Remove(&(group->ipsec_inbound_sa_list),
                               inbound_sa);

                    /* Deallocate the SA. */
                    IPSEC_Free_Inbound_SA(inbound_sa);

                    /* Move to next SA stored above. */
                    inbound_sa = (IPSEC_INBOUND_SA *)next_sa;
                }
                else
                {
                    break;
                }
            }

            /* Start search from the first item in the SADB. */
            outbound_sa = group->ipsec_outbound_sa_list.ipsec_head;

            /* Loop until the whole SADB has been traversed. */
            while(outbound_sa != NU_NULL)
            {
                if(IPSEC_Get_Outbound_SA_By_Addr(&outbound_sa,
                       local_addr, remote_addr) == NU_SUCCESS)
                {
                    /* Save pointer to the next SA. */
                    next_sa = outbound_sa->ipsec_flink;

                    /* Remove SA from SADB. */
                    SLL_Remove(&(group->ipsec_outbound_sa_list),
                               outbound_sa);

                    /* Deallocate the SA. */
                    IPSEC_Free_Outbound_SA(outbound_sa);

                    /* Move to next SA stored above. */
                    outbound_sa = (IPSEC_OUTBOUND_SA *)next_sa;
                }
                else
                {
                    break;
                }
            }
        }

        /* Release the semaphore as it is no longer required. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Remove_SAs_By_Addr */

/************************************************************************
* FUNCTION
*
*       IPSEC_Check_Initial_Contact
*
* DESCRIPTION
*
*       This function checks whether any IPsec SAs are established
*       with the specified remote address. This is used by IKE
*       to check whether to send an INITIAL-CONTACT notification
*       or not. Loops in this function have been slightly
*       unrolled to minimize loop operations since traversing the
*       whole SA list might be quite expensive.
*
* INPUTS
*
*       *group_name             Name of the IPsec group which is to be
*                               searched.
*       *remote_addr            Pointer to IP address of the remote node.
*
* OUTPUTS
*
*       NU_SUCCESS              If initial contact has occurred.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_ALREADY_EXISTS    If not an initial contact.
*       IPSEC_NOT_FOUND         Group not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Check_Initial_Contact(CHAR *group_name,
                                   IPSEC_SINGLE_IP_ADDR *remote_addr)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group;
    VOID                *sa_ptr;

#if (IPSEC_DEBUG == NU_TRUE)
    /* Validate input parameters. */
    if((group_name == NU_NULL) || (remote_addr == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }
#endif

    /* Grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Get the specified IPsec group pointer. */
        status = IPSEC_Get_Group_Entry(group_name, &group);

        if(status == NU_SUCCESS)
        {
            /* Get pointer to the first item in the inbound SADB. */
            sa_ptr = group->ipsec_inbound_sa_list.ipsec_head;

            if(sa_ptr != NU_NULL)
            {
                /* Search the inbound SADB for a matching SA. */
                if(IPSEC_Get_Inbound_SA_By_Addr(
                       (IPSEC_INBOUND_SA **)&sa_ptr, NU_NULL,
                       remote_addr) == NU_SUCCESS)
                {
                    /* SA found. This is not an initial contact. */
                    status = IPSEC_ALREADY_EXISTS;
                }
            }

            if(status == NU_SUCCESS)
            {
                /* Get pointer to the first item in the outbound SADB. */
                sa_ptr = group->ipsec_outbound_sa_list.ipsec_head;

                if(sa_ptr != NU_NULL)
                {
                    /* Search the outbound SADB for a matching SA. */
                    if(IPSEC_Get_Outbound_SA_By_Addr(
                           (IPSEC_OUTBOUND_SA **)&sa_ptr, NU_NULL,
                           remote_addr) == NU_SUCCESS)
                    {
                        /* SA found. This is not an initial contact. */
                        status = IPSEC_ALREADY_EXISTS;
                    }
                }
            }
        }

        /* Release the semaphore as it is no longer required. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the contact status. */
    return (status);

} /* IPSEC_Check_Initial_Contact */

#endif /* #if (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE) */

/************************************************************************
* FUNCTION
*
*       IPSEC_Rehash_Outbound_SAs
*
* DESCRIPTION
*
*       This function rehashes all IPsec outbound SAs which match the
*       specified selector and security protocol. It is called by IKE
*       when a new outbound SA is added to the database. Rehashing is
*       required to ensure that the most recent SA is always used for
*       outbound packet processing. This is achieved by re-assigning
*       the SA index to all matching SAs. This causes the SA to be
*       searched by selector, instead of by index. The new SA always
*       occurs first in the SADB, so all bundles are updated.
*
* INPUTS
*
*       *group_name             Name of the IPsec group which is to be
*                               searched.
*       *selector               Pointer to an outbound SA selector.
*       *security               Pointer to a security protocol structure.
*
* OUTPUTS
*
*       NU_SUCCESS              If at least one outbound SA was disabled.
*       NU_TIMEOUT              Operation timed out.
*       IPSEC_NOT_FOUND         Group not found or no matching outbound
*                               SA was found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Rehash_Outbound_SAs(CHAR *group_name,
                                 IPSEC_SELECTOR *selector,
                                 IPSEC_SECURITY_PROTOCOL *security)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group_ptr;
    IPSEC_OUTBOUND_SA   *outbound_sa = NU_NULL;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_name == NU_NULL) || (selector == NU_NULL) ||
                                  (security == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Get the specified IPsec group pointer. */
        status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

        if(status == NU_SUCCESS)
        {
            /* Initialize status to indicate that the SA was not found. */
            status = IPSEC_NOT_FOUND;

            /* Loop until matching SAs are found. */
            do
            {
                /* Search for an outbound SA. */
                if(IPSEC_Get_Outbound_SA(group_ptr, selector,
                                         security, &outbound_sa)
                                         == NU_SUCCESS)
                {
                    /* Update the status. */
                    status = NU_SUCCESS;

                    /* Increment the SA index. */
                    ++(group_ptr->ipsec_outbound_sa_list.
                        ipsec_next_sa_index);

                    /* Make sure index has not wrapped. */
                    if(group_ptr->ipsec_outbound_sa_list.
                        ipsec_next_sa_index == 0)
                    {
                        /* Index must always be non-zero. */
                        ++(group_ptr->ipsec_outbound_sa_list.
                            ipsec_next_sa_index);
                    }

                    /* Rehash the SA by assigning it a new index. */
                    outbound_sa->ipsec_index = (group_ptr->
                        ipsec_outbound_sa_list.ipsec_next_sa_index);

                    /* Start next search from the next SA. */
                    outbound_sa = outbound_sa->ipsec_flink;
                }
                else
                {
                    /* Terminate the search. */
                    outbound_sa = NU_NULL;
                }
            } while(outbound_sa != NU_NULL);
        }

        /* Release the semaphore as it is no longer required. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IPSEC_Rehash_Outbound_SAs */

#endif /* (INCLUDE_IKE == NU_TRUE) */

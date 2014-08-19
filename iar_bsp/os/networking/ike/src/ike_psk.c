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
*       ike_psk.c
*
* COMPONENT
*
*       IKE - Pre-shared Keys
*
* DESCRIPTION
*
*       This file implements the IKE pre-shared keys database.
*
* DATA STRUCTURES
*
*       IKE_Preshared_Key_DB    The IKE pre-shared keys database.
*
* FUNCTIONS
*
*       IKE_Initialize_Preshared_Keys
*       IKE_Add_Preshared_Key
*       IKE_Get_Preshared_Key_By_ID
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)

/*** Global variables related to IKE pre-shared keys. ***/

/* Declaring the global pre-shared key database. */
IKE_PRESHARED_KEY_DB IKE_Preshared_Key_DB;

/************************************************************************
*
* FUNCTION
*
*       IKE_Initialize_Preshared_Keys
*
* DESCRIPTION
*
*       This function initializes the IKE pre-shared keys database.
*       It sets the database to contain zero items.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful initialization.
*
************************************************************************/
STATUS IKE_Initialize_Preshared_Keys(VOID)
{
    /* Log debug message. */
    IKE_DEBUG_LOG("Initializing Pre-shared Keys");

    /* Set link pointers of the pre-shared key DB to NULL. */
    IKE_Preshared_Key_DB.ike_flink = NU_NULL;
    IKE_Preshared_Key_DB.ike_last = NU_NULL;

    /* Set the next pre-shared key index to zero. */
    IKE_Preshared_Key_DB.ike_next_psk_index = 0;

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Initialize_Preshared_Keys */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_Preshared_Key
*
* DESCRIPTION
*
*       This function adds a new pre-shared key (PSK) to the
*       pre-shared keys database. A copy of the pre-shared
*       key, and all its dynamic fields, is made before
*       being added to the database.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *psk                    Pointer to the pre-shared key to
*                               be added.
*       *index                  On return, this contains the unique
*                               index assigned to the new
*                               pre-shared key.
*
* OUTPUTS
*
*       NU_SUCCESS              PSK was successfully added.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters passed are invalid.
*       IKE_ALREADY_EXISTS      Pre-shared key by this ID already
*                               exists in the database.
*
************************************************************************/
STATUS IKE_Add_Preshared_Key(IKE_PRESHARED_KEY *psk, UINT16 *index)
{
    STATUS              status = NU_SUCCESS;
    IKE_PRESHARED_KEY   *new_psk;
    UNSIGNED            mem_size;
    IKE_IDENTIFIER      find_id;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure all pointers are valid. */
    if((psk == NU_NULL) || (index == NU_NULL))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Verify identifier type. */
    else if(
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
            (psk->ike_id.ike_type != IKE_DOMAIN_NAME)      &&
            (psk->ike_id.ike_type != IKE_USER_DOMAIN_NAME) &&
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            (psk->ike_id.ike_type != IKE_IPV4)             &&
            (psk->ike_id.ike_type != IKE_IPV4_SUBNET)      &&
            (psk->ike_id.ike_type != IKE_IPV4_RANGE)       &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (psk->ike_id.ike_type != IKE_IPV6)             &&
            (psk->ike_id.ike_type != IKE_IPV6_SUBNET)      &&
            (psk->ike_id.ike_type != IKE_IPV6_RANGE)       &&
#endif
            (psk->ike_id.ike_type != IKE_WILDCARD))
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
    /* Verify the selector address if it is a domain name. */
    else if(((psk->ike_id.ike_type == IKE_DOMAIN_NAME)       ||
             (psk->ike_id.ike_type == IKE_USER_DOMAIN_NAME)) &&
            ((psk->ike_id.ike_addr.ike_domain == NU_NULL)    ||
             (*psk->ike_id.ike_addr.ike_domain == 0)         ||
             (strlen(psk->ike_id.ike_addr.ike_domain) >=
              IKE_MAX_DOMAIN_NAME_LEN)))
    {
        status = IKE_INVALID_PARAMS;
    }
#endif

    /* Verify the pre-shared key material. */
    else if((psk->ike_key == NU_NULL) || (psk->ike_key_len == 0))
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Make a copy of the ID because we might need to modify it. */
        NU_BLOCK_COPY(&find_id, &psk->ike_id, sizeof(IKE_IDENTIFIER));

#if (INCLUDE_IPV6 == NU_TRUE)
        /* If an IPv6 subnet has been specified. */
        if(find_id.ike_type == IKE_IPV6_SUBNET)
        {
            /* Convert prefix length to an IPv6 subnet mask, */
            status = IPSEC_Convert_Subnet6(
                        find_id.ike_addr.ike_ip.ike_ext_addr.ike_addr2);
        }
#endif
    }

    if(status == NU_SUCCESS)
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Make sure a pre-shared key with the specified identifier
             * does not already exist in the database.
             */
            status = IKE_Get_Preshared_Key_By_ID(&find_id, &new_psk,
                         IKE_MATCH_IDENTIFIERS_ABS);

            if(status == NU_SUCCESS)
            {
                NLOG_Error_Log("Pre-shared key already exists in database",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Item already exists. */
                status = IKE_ALREADY_EXISTS;
            }

            else
            {
                /* Initialize required memory size to size of
                 * the pre-shared key structure.
                 */
                mem_size = sizeof(IKE_PRESHARED_KEY);

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
                /* If identifier is a domain name. */
                if((psk->ike_id.ike_type == IKE_DOMAIN_NAME) ||
                   (psk->ike_id.ike_type == IKE_USER_DOMAIN_NAME))
                {
                    /* Add length of domain name to memory size. */
                    mem_size += 1 +
                        strlen(psk->ike_id.ike_addr.ike_domain);
                }
#endif

                /* Allocate the memory for the new pre-shared key. */
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                            (VOID**)&new_psk,
                                            mem_size, NU_NO_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    /* Normalize the pointer. */
                    new_psk = TLS_Normalize_Ptr(new_psk);

                    /* Now copy the specified pre-shared key. */
                    NU_BLOCK_COPY(new_psk, psk, sizeof(IKE_PRESHARED_KEY));

                    /* Update its ID as it might have been modified. */
                    NU_BLOCK_COPY(&new_psk->ike_id, &find_id,
                                  sizeof(IKE_IDENTIFIER));

                    /* Increment the index and then assign the unique
                     * identifier to the pre-shared key index.
                     */
                    IKE_Preshared_Key_DB.ike_next_psk_index++;
                    new_psk->ike_index =
                        IKE_Preshared_Key_DB.ike_next_psk_index;

#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
                    /* If identifier is a domain name. */
                    if((new_psk->ike_id.ike_type == IKE_DOMAIN_NAME) ||
                       (new_psk->ike_id.ike_type == IKE_USER_DOMAIN_NAME))
                    {
                        /* Assign the memory following the pre-shared
                         * structure to the domain name.
                         */
                        new_psk->ike_id.ike_addr.ike_domain =
                            (CHAR*)(new_psk + 1);

                        /* Copy domain name into the memory. */
                        strcpy(new_psk->ike_id.ike_addr.ike_domain,
                               psk->ike_id.ike_addr.ike_domain);
                    }
#endif

                    /* Add this newly created pre-shared key to the
                     * pre-shared keys database.
                     */
                    SLL_Enqueue(&IKE_Preshared_Key_DB, new_psk);

                    /* Return the pre-shared key index. */
                    *index = new_psk->ike_index;
                }

                else
                {
                    NLOG_Error_Log("Failed to allocate memory for PSK",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* Now everything is done, release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IKE semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IKE_Add_Preshared_Key */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Preshared_Key_By_ID
*
* DESCRIPTION
*
*       This is a utility function which searches the pre-shared
*       key database to find a pre-shared key that matches the
*       given identifier. Two methods can be used for matching
*       the identifiers:
*
*       1. IKE_MATCH_IDENTIFIERS: Logically match the identifiers.
*          This is the same method used for matching identifiers
*          of incoming IKE messages.
*       2. IKE_MATCH_IDENTIFIERS_ABS: The identifiers are matched
*          literally. IP ranges are not compared to single IPs
*          and only the exact same identifiers match.
*
* INPUTS
*
*       *id                     Identifier to be searched.
*       **ret_psk               On return, this contains the
*                               pointer to the matching pre-shared
*                               key.
*       match                   This is a pointer to the match
*                               function. Valid values are:
*                               IKE_MATCH_IDENTIFIERS or
*                               IKE_MATCH_IDENTIFIERS_ABS.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Pre-shared key not found.
*
************************************************************************/
STATUS IKE_Get_Preshared_Key_By_ID(IKE_IDENTIFIER *id,
                                   IKE_PRESHARED_KEY **ret_psk,
                                   IKE_IDENTIFIER_MATCH_FUNC match)
{
    STATUS              status = IKE_NOT_FOUND;
    IKE_PRESHARED_KEY   *cur_psk;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((id == NU_NULL) || (ret_psk == NU_NULL) || (match == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }

    /* If the identifier type is a domain name, make sure the
     * domain name is specified within the identifier.
     */
    if((id->ike_type == IKE_DOMAIN_NAME) ||
       (id->ike_type == IKE_USER_DOMAIN_NAME))
    {
#if (IKE_ENABLE_DOMAIN_NAME_ID == NU_TRUE)
        if(id->ike_addr.ike_domain == NU_NULL)
        {
            return (IKE_INVALID_PARAMS);
        }
        if(*(id->ike_addr.ike_domain) == 0)
        {
            return (IKE_INVALID_PARAMS);
        }
#else
        return (IKE_INVALID_PARAMS);
#endif
    }
#endif

    /* Start search from first item in the database. */
    cur_psk = IKE_Preshared_Key_DB.ike_flink;

    /* Find the required pre-shared key. */
    while(cur_psk != NU_NULL)
    {
        /* Compare the two identifiers. */
        if(match(&cur_psk->ike_id, id) == NU_TRUE)
        {
            /* Match found. Assign the value to the
             * return pointer.
             */
            *ret_psk = cur_psk;

            /* Set the return status to success. */
            status = NU_SUCCESS;

            /* Break out of the search loop. */
            break;
        }

        /* Move to the next item in the database. */
        cur_psk = cur_psk->ike_flink;
    }

    /* Return the status value. */
    return (status);

} /* IKE_Get_Preshared_Key_By_ID */

#endif /* (IKE_INCLUDE_PSK_AUTH == NU_TRUE) */

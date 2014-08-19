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
*       ike_sadb.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       Implementation of the IKE SA database. This file also
*       includes the closely integrated IKE SA2 database and
*       Phase 1 & 2 Handle database.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Match_SA_IP
*       IKE_Match_SA_Partial_Cookie
*       IKE_Match_SA_Cookie
*       IKE_Add_SA
*       IKE_Get_SA
*       IKE_Remove_SA
*       IKE_Sync_Remove_SAs
*       IKE_Free_Local_SA
*       IKE_Free_SA
*       IKE_Add_SA2
*       IKE_Add_Phase2
*       IKE_Get_Phase2
*       IKE_Remove_Phase1
*       IKE_Remove_Phase2
*       IKE_Free_Phase1
*       IKE_Flush_SA2
*       IKE_Free_Local_Phase2
*       IKE_Free_Phase2
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_api.h
*       ike_buf.h
*       ike_evt.h
*
************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_buf.h"
#include "networking/ike_evt.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Match_SA_IP
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       SAs by their IP address. It is usually called from the
*       IKE_Get_SA function.
*
* INPUTS
*
*       *sa                     Pointer to the SA.
*       *search_data            Pointer to the IP to be matched.
*                               This should point to an
*                               addr_struct structure.
*
* OUTPUTS
*
*       NU_TRUE                 If the SA matches.
*       NU_FALSE                If SA does not match the search
*                               criteria.
*
************************************************************************/
INT IKE_Match_SA_IP(IKE_SA *sa, VOID *search_data)
{
    INT                 result = NU_FALSE;
    struct addr_struct  *ip    = (struct addr_struct *)search_data;

    /* Check if the SA IP type matches. */
    if(sa->ike_node_addr.family == ip->family)
    {
        /* Check if the SA IP type matches. */
        if(sa->ike_node_addr.port == ip->port)
        {
            /* Check if the SA IP matches. */
            if(memcmp(sa->ike_node_addr.id.is_ip_addrs,
                      ip->id.is_ip_addrs,
                      IKE_IP_LEN(ip->family)) == 0)
            {
                /* Match found so return success. */
                result = NU_TRUE;
            }
        }
    }

    /* Return the result. */
    return (result);

} /* IKE_Match_SA_IP */

/************************************************************************
*
* FUNCTION
*
*       IKE_Match_SA_Partial_Cookie
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       SAs by their cookies. It is usually called from the
*       IKE_Get_SA function. Note that this function only
*       matches the Initiator's cookie.
*
* INPUTS
*
*       *sa                     Pointer to the SA.
*       *search_data            Pointer to the cookie which
*                               is being searched.
*
* OUTPUTS
*
*       NU_TRUE                 If the SA matches.
*       NU_FALSE                If SA does not match the search
*                               criteria.
*
************************************************************************/
INT IKE_Match_SA_Partial_Cookie(IKE_SA *sa, VOID *search_data)
{
    INT             result;

    /* Check if the first cookie matches. */
    if(memcmp(sa->ike_cookies, (UINT8*)search_data, IKE_COOKIE_LEN) == 0)
    {
        /* Match found so return success. */
        result = NU_TRUE;
    }

    /* First cookie does not match. */
    else
    {
        result = NU_FALSE;
    }

    /* Return the result. */
    return (result);

} /* IKE_Match_SA_Partial_Cookie */

/************************************************************************
*
* FUNCTION
*
*       IKE_Match_SA_Cookie
*
* DESCRIPTION
*
*       This is an internal utility function used for matching
*       SAs by their cookies. It is usually called from the
*       IKE_Get_SA function. This function matches both the
*       Initiator and Responder cookies.
*
* INPUTS
*
*       *sa                     Pointer to the SA.
*       *search_data            Pointer to the two cookies which
*                               are being searched.
*
* OUTPUTS
*
*       NU_TRUE                 If the SA matches.
*       NU_FALSE                If SA does not match the search
*                               criteria.
*
************************************************************************/
INT IKE_Match_SA_Cookie(IKE_SA *sa, VOID *search_data)
{
    INT             result;

    /* Check if the cookies matches. */
    if(memcmp(sa->ike_cookies, (UINT8*)search_data,
              IKE_COOKIE_LEN * IKE_NUM_ENTITIES) == 0)
    {
        /* Match found so return success. */
        result = NU_TRUE;
    }

    /* Cookies do not match. */
    else
    {
        result = NU_FALSE;
    }

    /* Return the result. */
    return (result);

} /* IKE_Match_SA_Cookie */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_SA
*
* DESCRIPTION
*
*       This function adds a new SA to the SA Database (SADB).
*       The new SA is added as is without any error checking
*       because this is an internal function, and the caller
*       is responsible for verifying the SA fields. A copy of
*       the SA is added to the database, so the one passed
*       can be overwritten by the caller. A copy of the
*       Phase 1 Handle is also made, if it is present. However,
*       all other dynamic fields of the SA are not copied into
*       separate buffers. They must be allocated _after_ adding
*       an SA to the SADB. All dynamic fields are deallocated
*       when an SA is removed from the SADB.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *sadb                   Database to which the new SA is
*                               to be added.
*       *sa                     Pointer to the SA which is to be
*                               added.
*       **ret_sa                If not NULL then on return,
*                               contains pointer to
*                               the newly added SA. This is a
*                               copy of the original SA passed
*                               to this function.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
************************************************************************/
STATUS IKE_Add_SA(IKE_SADB *sadb, IKE_SA *sa, IKE_SA **ret_sa)
{
    STATUS          status;
    IKE_SA          *new_sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the database pointer is not NULL. */
    if(sadb == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the SA pointer is not NULL. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Allocate memory for the new SA. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&new_sa,
                                sizeof(IKE_SA), NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        new_sa = TLS_Normalize_Ptr(new_sa);

        /* Copy the SA content to the allocated memory. */
        NU_BLOCK_COPY(new_sa, sa, sizeof(IKE_SA));

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
        /* Set version of the SA to IKEv1. */
        new_sa->ike2_version = IKE_VERSION_1;
#endif

        /* If a Phase 1 Handle is present. */
        if(sa->ike_phase1 != NU_NULL)
        {
            /* Allocate memory for the Phase 1 Handle. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                        (VOID**)&new_sa->ike_phase1,
                                        sizeof(IKE_PHASE1_HANDLE),
                                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                /* Normalize the pointer. */
                new_sa->ike_phase1 = TLS_Normalize_Ptr(new_sa->ike_phase1);

                /* Copy the Handle content to allocated memory. */
                NU_BLOCK_COPY(new_sa->ike_phase1, sa->ike_phase1,
                              sizeof(IKE_PHASE1_HANDLE));

                /* Also update the SA back-pointer in the Handle. */
                new_sa->ike_phase1->ike_sa = new_sa;
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for Handle",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Deallocate SA memory which was allocated above. */
                if(NU_Deallocate_Memory(new_sa) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate IKE SA memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        if(status == NU_SUCCESS)
        {
            /* Add the SA to the database linked list. */
            SLL_Enqueue(sadb, new_sa);

            if(ret_sa != NU_NULL)
            {
                /* Return pointer to the new SA. */
                *ret_sa = new_sa;
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for an IKE SA",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Add_SA */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_SA
*
* DESCRIPTION
*
*       This function searches the SA database (SADB) for an
*       SA that matches the given parameters. The search
*       criteria for the SA is defined by the 'match'
*       parameter.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *sadb                   Database to be searched.
*       *search_data            Data to be searched. This is
*                               specific to the match function
*                               being used.
*       match                   This is a pointer to the function
*                               which is to be used for matching.
*                               See the IKE_MATCH_SA_* macros
*                               in the header file for possible
*                               values.
*       **ret_sa                On successful return, this would
*                               contain the address of the SA
*                               being searched.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Required item not found.
*
************************************************************************/
STATUS IKE_Get_SA(IKE_SADB *sadb, VOID *search_data,
                  IKE_SA_MATCH_FUNC match, IKE_SA **ret_sa)
{
    IKE_SA          *sa;
    STATUS          status = IKE_NOT_FOUND;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((sadb  == NU_NULL) || (search_data == NU_NULL) ||
       (match == NU_NULL) || (ret_sa      == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set SA to the start of the SADB. */
    sa = sadb->ike_flink;

    /* Traverse the SA list. */
    while(sa != NU_NULL)
    {
        /* Check if match is found and is not to be skipped. */
        if(match(sa, search_data) == NU_TRUE)
        {
            /* Match found so return the SA. */
            *ret_sa = sa;

            /* Set status to success. */
            status = NU_SUCCESS;

            /* Stop the search loop. */
            break;
        }

        /* Move to the next item in the list. */
        sa = sa->ike_flink;
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_SA */

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_SA
*
* DESCRIPTION
*
*       This function removes the specified SA from the SA database
*       (SADB). The search criteria for the SA being removed is
*       defined by the 'match' parameter.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *sadb                   Database to be searched.
*       *search_data            Data to be searched for finding
*                               the SA which is to be removed. This
*                               is specific to the match function
*                               being used.
*       match                   This is a pointer to the function
*                               which is to be used for matching.
*                               See the IKE_MATCH_SA_* macros
*                               in the header file for possible
*                               values.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Required item not found.
*
************************************************************************/
STATUS IKE_Remove_SA(IKE_SADB *sadb, VOID *search_data,
                     IKE_SA_MATCH_FUNC match)
{
    STATUS          status;
    IKE_SA          *ret_sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the SADB pointer is not NULL. */
    if(sadb == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the search data pointer is not NULL. */
    if(search_data == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the match function pointer is not NULL. */
    if(match == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Retrieve the SA matching the given cookies. */
    status = IKE_Get_SA(sadb, search_data, match, &ret_sa);

    if(status == NU_SUCCESS)
    {
        /* Remove the SA from the database. */
        SLL_Remove(sadb, ret_sa);

        /* Deallocate all dynamic fields of the SA and
         * all Phase 2 Handles.
         */
        if(IKE_Free_SA(ret_sa) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate SA",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("IKE SA not found",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Remove_SA */

#if ((IKE_INCLUDE_INFO_MODE == NU_TRUE) && \
     (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE))
/************************************************************************
*
* FUNCTION
*
*       IKE_Sync_Remove_SAs
*
* DESCRIPTION
*
*       This function removes multiple established IKE SAs
*       which match the search criteria specified in the
*       'match' parameter. The 'skip_sa' parameter specifies
*       an SA which is not to be deleted. This function is
*       called after an INITIAL-CONTACT notification.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *sadb                   Database to be searched.
*       *search_data            Data to be searched for finding
*                               the SA which is to be removed. This
*                               is specific to the match function
*                               being used.
*       match                   This is a pointer to the function
*                               which is to be used for matching.
*                               See the IKE_MATCH_SA_* macros
*                               in the header file for possible
*                               values.
*       *skip_sa                An SA which is not to be deleted
*                               even if it matches the search
*                               criteria. This can be NULL if
*                               no such check is to be performed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           No match found.
*
************************************************************************/
STATUS IKE_Sync_Remove_SAs(IKE_SADB *sadb, VOID *search_data,
                           IKE_SA_MATCH_FUNC match, IKE_SA *skip_sa)
{
    STATUS          status = IKE_NOT_FOUND;
    IKE_SA          *sa;
    IKE_SA          *next_sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((sadb  == NU_NULL) || (search_data == NU_NULL) ||
       (match == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set SA to the start of the SADB. */
    sa = sadb->ike_flink;

    /* Traverse the SA list. */
    while(sa != NU_NULL)
    {
        /* Save pointer to the next SA. */
        next_sa = sa->ike_flink;

        /* Check if match is found. */
        if(match(sa, search_data) == NU_TRUE)
        {
            /* Make sure only established SAs are deleted. */
            if(sa->ike_state == IKE_SA_ESTABLISHED)
            {
                /* Make sure this is not the SA which is to
                 * be skipped.
                 */
                if(sa != skip_sa)
                {
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                    /* Resume waiting processes with the
                     * current exchange status.
                     */
                    if(IKE_Resume_Waiting_Processes(sa, IKE_NO_UPDATE)
                       == NU_SUCCESS)
                    {
                        IKE_DEBUG_LOG("Waiting processes resumed");
                    }
#endif

                    /* Grab the TCP semaphore. */
                    status = NU_Obtain_Semaphore(&TCP_Resource,
                                                 IKE_TIMEOUT);

                    if(status == NU_SUCCESS)
                    {
                        /* Remove all IKE timer events, which reference
                         * the current SA, from NET TQ.
                         */
                        if(IKE_Unset_Matching_Timers((UNSIGNED)sa, 0,
                                                     TQ_CLEAR_ALL_EXTRA)
                                                     != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to unset IKE timers",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                        /* Similarly, remove all matching events from
                         * the IKE timer list.
                         */
                        IKE_Unset_Matching_Events((UNSIGNED)sa, 0,
                                                  TQ_CLEAR_ALL_EXTRA);

                        /* Release the TCP semaphore. */
                        if(NU_Release_Semaphore(&TCP_Resource) !=
                           NU_SUCCESS)
                        {
                            NLOG_Error_Log(
                                "Failed to release the semaphore",
                                NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to obtain the semaphore",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }

                    /* Remove the SA from the database. */
                    SLL_Remove(sadb, sa);

                    /* Deallocate all dynamic fields of the
                     * SA and all its Phase 2 Handles.
                     */
                    if(IKE_Free_SA(sa) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to deallocate SA",
                            NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* Log the event. */
                    IKE_DEBUG_LOG("Forced IKE SA deletion");
                }
            }
        }

        /* Move to the next item in the list. */
        sa = next_sa;
    }

    /* Return the status. */
    return (status);

} /* IKE_Sync_Remove_SAs */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_Local_SA
*
* DESCRIPTION
*
*       This is a utility function used to deallocate all
*       dynamic fields of a Phase 1 IKE SA which has not
*       been added to the database.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA which
*                               is to be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE_Free_Local_SA(IKE_SA *sa)
{
    /* If SKEYID is allocated. */
    if(sa->ike_skeyid != NU_NULL)
    {
        /* Deallocate SKEYID. */
        if(NU_Deallocate_Memory(sa->ike_skeyid) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set SKEYID dynamic fields to NULL. */
        sa->ike_skeyid   = NU_NULL;
        sa->ike_skeyid_d = NU_NULL;
        sa->ike_skeyid_a = NU_NULL;
        sa->ike_skeyid_e = NU_NULL;
    }

    /* If encryption key is allocated. */
    if(sa->ike_encryption_key != NU_NULL)
    {
        /* Deallocate encryption key. */
        if(NU_Deallocate_Memory(sa->ike_encryption_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set encryption key field to NULL. */
        sa->ike_encryption_key = NU_NULL;
    }

#if (IKE_INCLUDE_PSK_AUTH == NU_TRUE)
    /* If Authentication method of this SA is pre-shared key. */
    if((IKE_IS_PSK_METHOD(sa->ike_attributes.ike_auth_method) == NU_TRUE)
       && (sa->ike_attributes.ike_remote_key != NU_NULL))
    {
        /* Deallocate the dynamically allocated pre-shared key. */
        if(NU_Deallocate_Memory(sa->ike_attributes.ike_remote_key) !=
           NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set remote key field to NULL. */
        sa->ike_attributes.ike_remote_key = NU_NULL;
    }
#endif

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_Local_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_SA
*
* DESCRIPTION
*
*       This is a utility function used to deallocate all
*       dynamic fields of the Phase 1 IKE SA. The SA pointer
*       passed to this function should point to a
*       dynamically allocated SA. The SA memory is also
*       deallocated. Note that the caller is responsible for
*       removing the IKE SA from any SADB it might be a part
*       of.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA which
*                               is to be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Free_SA(IKE_SA *sa)
{
    IKE_PHASE2_HANDLE   *phase2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    /* Make sure the SA being freed is owned by IKEv1. */
    if (sa->ike2_version == IKE_VERSION_2)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Free all dynamic fields of the IKE SA. */
    if(IKE_Free_Local_SA(sa) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate local SA members",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If the Phase 1 Handle exists. */
    if(sa->ike_phase1 != NU_NULL)
    {
        /* Deallocate the Phase 1 Handle structure. */
        if(IKE_Free_Phase1(sa->ike_phase1) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Phase 1 Handle",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the Handle pointer to NULL. */
        sa->ike_phase1 = NU_NULL;
    }

    /* Set pointer to the first Phase 2 Handle in the database. */
    phase2 = sa->ike_phase2_db.ike_flink;

    /* Loop for all Phase 2 Handles in the database. */
    while(phase2 != NU_NULL)
    {
        /* Remove the Phase 2 Handle from the database. */
        if(IKE_Remove_Phase2(&sa->ike_phase2_db, phase2->ike_msg_id) !=
           NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to remove Phase 2 Handle from SA",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Move to the next Phase 2 Handle. */
        phase2 = phase2->ike_flink;
    }

    /* Finally, deallocate the memory of the SA itself. */
    if(NU_Deallocate_Memory(sa) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate the memory",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_SA */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_SA2
*
* DESCRIPTION
*
*       This function allocates memory for a new SA2,
*       copies the passed SA2 into the memory and adds it
*       to the specified SA2DB. Copies of the dynamic
*       fields are not made. However, the component does
*       take responsibility of freeing the dynamic fields
*       when the SA2 item is removed from the database.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *sa2db                  Pointer to the SA2DB to
*                               which the SA2 is to be added.
*       *sa2                    Pointer to the new SA2.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
************************************************************************/
STATUS IKE_Add_SA2(IKE_SA2_DB *sa2db, IKE_SA2 *sa2)
{
    STATUS          status;
    IKE_SA2         *sa2_ptr;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the pointers are not NULL. */
    if((sa2db == NU_NULL) || (sa2 == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Allocate memory for the new SA2. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                (VOID**)&sa2_ptr,
                                sizeof(IKE_SA2),
                                NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        sa2_ptr = TLS_Normalize_Ptr(sa2_ptr);

        /* Copy the contents of the SA2 item. */
        NU_BLOCK_COPY(sa2_ptr, sa2, sizeof(IKE_SA2));

        /* Add the SA2 item to SA2DB. */
        SLL_Enqueue(sa2db, sa2_ptr);
    }

    /* Return the status. */
    return (status);

} /* IKE_Add_SA2 */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_Phase2
*
* DESCRIPTION
*
*       This function adds a new Phase 2 Handle to the
*       database. Memory for the Handle is dynamically
*       allocated and the passed Handle is copied onto that
*       block, as is. Copies of the dynamic fields in the
*       Handle are not made. However, this component does
*       take the responsibility of freeing these dynamic
*       fields when the Handle is removed.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *ph2db                  Pointer to the Phase 2
*                               database to which the new entry
*                               is to be added.
*       *phase2                 Pointer to the Phase 2 Handle
*                               which is to be added.
*       **ret_phase2            If not NULL then on return,
*                               contains pointer to the
*                               Phase 2 item which was added.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
************************************************************************/
STATUS IKE_Add_Phase2(IKE_PHASE2_DB *ph2db, IKE_PHASE2_HANDLE *phase2,
                      IKE_PHASE2_HANDLE **ret_phase2)
{
    STATUS              status;
    IKE_PHASE2_HANDLE   *new_phase2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the database pointer is not NULL. */
    if(ph2db == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the Handle pointer is not NULL. */
    if(phase2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Allocate memory for the new Handle. */
    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID**)&new_phase2,
                                sizeof(IKE_PHASE2_HANDLE), NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        /* Normalize the pointer. */
        new_phase2 = TLS_Normalize_Ptr(new_phase2);

        /* Copy Handle to the allocated memory. */
        NU_BLOCK_COPY(new_phase2, phase2, sizeof(IKE_PHASE2_HANDLE));

        /* Add Handle to the Phase 2 database. */
        SLL_Enqueue(ph2db, new_phase2);

        if(ret_phase2 != NU_NULL)
        {
            /* Return the Handle pointer. */
            *ret_phase2 = new_phase2;
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for Phase 2 Handle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Add_Phase2 */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Phase2
*
* DESCRIPTION
*
*       This function retrieves the Phase 2 Handle identified
*       by its unique Message ID.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *ph2db                  Pointer to the Phase 2 database
*                               which is to be searched.
*       msg_id                  Unique Message ID of the SA2.
*       **ret_phase2            On return, this contains a
*                               pointer to the required Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Unable to find the required list.
*
************************************************************************/
STATUS IKE_Get_Phase2(IKE_PHASE2_DB *ph2db, UINT32 msg_id,
                      IKE_PHASE2_HANDLE **ret_phase2)
{
    STATUS              status = IKE_NOT_FOUND;
    IKE_PHASE2_HANDLE   *phase2;

#if (IKE_DEBUG == NU_TRUE)
    /* Verify that the database pointer is not NULL. */
    if(ph2db == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the return pointer is not NULL. */
    if(ret_phase2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Get the first item of the database. */
    phase2 = ph2db->ike_flink;

    /* Traverse the whole database. */
    while(phase2 != NU_NULL)
    {
        /* Check if the required message ID is found.*/
        if(phase2->ike_msg_id == msg_id)
        {
            /* Set the return Handle pointer. */
            *ret_phase2 = phase2;

            /* Set status to success. */
            status = NU_SUCCESS;

            /* End the search. */
            break;
        }

        /* Move to the next item in the list. */
        phase2 = phase2->ike_flink;
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Phase2 */

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_Phase1
*
* DESCRIPTION
*
*       This function removes the Phase 1 Handle from the
*       specified IKE SA. All dynamic fields of the Handle
*       are deallocated before it is removed.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *sa                     IKE SA from which to remove
*                               the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           No Handle found in SA.
*
************************************************************************/
STATUS IKE_Remove_Phase1(IKE_SA *sa)
{
    STATUS              status;
    IKE_PHASE1_HANDLE   *phase1;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the SA pointer is valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    if(sa->ike_phase1 == NU_NULL)
    {
        status = IKE_NOT_FOUND;
    }

    else
    {
        /* Set the Handle pointer. */
        phase1 = sa->ike_phase1;

        /* Remove the Handle from the SA. */
        sa->ike_phase1 = NU_NULL;

        /* Deallocate all dynamic fields of the Handle. */
        status = IKE_Free_Phase1(phase1);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Phase 1 Handle",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Remove_Phase1 */

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_Phase2
*
* DESCRIPTION
*
*       This function searches the specified database for
*       the given Message ID. If found, that Phase 2 Handle
*       is deallocated and removed from the list. All SA2
*       items that belong to the Handle are also deallocated.
*
*       Note that the caller is responsible for obtaining
*       the IKE semaphore before calling this function.
*
* INPUTS
*
*       *ph2db                  Pointer to the database
*                               from which the Handle is to
*                               be removed.
*       msg_id                  Message ID of the Handle which
*                               is to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Unable to find the required list.
*
************************************************************************/
STATUS IKE_Remove_Phase2(IKE_PHASE2_DB *ph2db, UINT32 msg_id)
{
    STATUS              status;
    IKE_PHASE2_HANDLE   *ret_phase2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the database pointer is not NULL. */
    if(ph2db == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Retrieve the Handle being removed. */
    status = IKE_Get_Phase2(ph2db, msg_id, &ret_phase2);

    if(status == NU_SUCCESS)
    {
        /* Remove the Handle from the database. */
        SLL_Remove(ph2db, ret_phase2);

        /* Deallocate all dynamic fields of the Handle and
         * all SA2 items from the SA2DB.
         */
        status = IKE_Free_Phase2(ret_phase2);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate Phase 2 Handle",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Unable to get Phase 2 Handle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Remove_Phase2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_Phase1
*
* DESCRIPTION
*
*       This is a utility function used to deallocate all
*       dynamic fields of a Phase 1 Handle. The pointer
*       passed to this function must point to a dynamically
*       allocated Handle. The Handle itself is also
*       deallocated. Note that the caller is responsible for
*       removing the Handle from any database or SA it might
*       be a part of.
*
*       The 'ike_sa' and 'ike_params' members of the Phase 1
*       Handle are not modified/deallocated by this function.
*       The caller is responsible for deallocating them.
*
* INPUTS
*
*       *phase1                 Pointer to the Handle which
*                               is to be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Free_Phase1(IKE_PHASE1_HANDLE *phase1)
{
#if (IKE_DEBUG == NU_TRUE)
    /* Make sure Handle pointer is valid. */
    if(phase1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If the last message is buffered. */
    if(phase1->ike_last_message != NU_NULL)
    {
        /* Return buffer to IKE Buffer component. */
        if(IKE_Deallocate_Buffer(phase1->ike_last_message) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to return message buffer",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the last message buffer field to NULL. */
        phase1->ike_last_message = NU_NULL;
    }

    /* If Nonce data is allocated. */
    if(phase1->ike_nonce_data != NU_NULL)
    {
        /* Deallocate the Nonce data. */
        if(NU_Deallocate_Memory(phase1->ike_nonce_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Nonce data field to NULL. */
        phase1->ike_nonce_data = NU_NULL;
    }

    /* If Initiator's raw SA is allocated. */
    if(phase1->ike_sa_b != NU_NULL)
    {
        /* Deallocate the Initiator's raw SA. */
        if(NU_Deallocate_Memory(phase1->ike_sa_b) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Initiator's raw SA field to NULL. */
        phase1->ike_sa_b = NU_NULL;
    }

    /* If Initiator's raw ID payload is allocated. */
    if(phase1->ike_id_b != NU_NULL)
    {
        /* Deallocate the Initiator's raw ID payload. */
        if(NU_Deallocate_Memory(phase1->ike_id_b) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Initiator's raw ID field to NULL. */
        phase1->ike_id_b = NU_NULL;
    }

    /* If the Diffie-Hellman remote public key is allocated. */
    if(phase1->ike_dh_remote_key != NU_NULL)
    {
        /* Deallocate the remote public key. */
        if(NU_Deallocate_Memory(phase1->ike_dh_remote_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the remote public key field to NULL. */
        phase1->ike_dh_remote_key = NU_NULL;
    }

    /* If the Diffie-Hellman key pair is allocated. */
    if(phase1->ike_dh_key.ike_public_key != NU_NULL)
    {
        /* Deallocate the Diffie-Hellman key pair. */
        if(NU_Deallocate_Memory(phase1->ike_dh_key.ike_public_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set Diffie-Hellman key pair fields to NULL. */
        phase1->ike_dh_key.ike_public_key  = NU_NULL;
        phase1->ike_dh_key.ike_private_key = NU_NULL;
    }

    if(phase1->ike_ca_dn_data != NULL)
    {
        /* Deallocate the CA's DN. */
        if(NU_Deallocate_Memory(phase1->ike_ca_dn_data) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory.", NERR_SEVERE,
                            __FILE__, __LINE__);
        }
    }

    /* Finally, deallocate the memory of Phase 1 Handle. */
    if(NU_Deallocate_Memory(phase1) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate the memory",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_Phase1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Flush_SA2
*
* DESCRIPTION
*
*       This is a utility function used to deallocate all
*       SA2 items from the SA2DB in the Phase 2 Handle.
*
* INPUTS
*
*       *sa2db                  Pointer the SA2DB which is
*                               to be flushed.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Flush_SA2(IKE_SA2_DB *sa2db)
{
    IKE_SA2         *cur_sa2;
    IKE_SA2         *prev_sa2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure SA2DB pointer is valid. */
    if(sa2db == NU_NULL)
    {
        return;
    }
#endif

    /* Get the first SA2 item in the SA2DB. */
    cur_sa2 = sa2db->ike_flink;

    /* Traverse the whole SA2 list. */
    while(cur_sa2 != NU_NULL)
    {
        /* Keep reference of current SA2. */
        prev_sa2 = cur_sa2;

        /* Move to next SA2. */
        cur_sa2 = cur_sa2->ike_flink;

        /* If the key material is allocated. */
        if(prev_sa2->ike_local_keymat != NU_NULL)
        {
            /* Deallocate the both local and remote key
             * material. Both are allocated in a single
             * block within the SA2.
             */
            if(NU_Deallocate_Memory(prev_sa2->ike_local_keymat) !=
               NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate the memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        /* Deallocate memory of the previous SA2. */
        if(NU_Deallocate_Memory(prev_sa2) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Set pointers of SA2DB to NULL. */
    sa2db->ike_flink = NU_NULL;
    sa2db->ike_last  = NU_NULL;

} /* IKE_Flush_SA2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_Local_Phase2
*
* DESCRIPTION
*
*       This is a utility function used to deallocate all
*       dynamic fields of a Phase 2 Handle which has not
*       been added to the database yet.
*
* INPUTS
*
*       *phase2                 Pointer to the Handle which
*                               is to be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE_Free_Local_Phase2(IKE_PHASE2_HANDLE *phase2)
{
    /* If the last message is buffered. */
    if(phase2->ike_last_message != NU_NULL)
    {
        /* Return buffer to IKE Buffer component. */
        if(IKE_Deallocate_Buffer(phase2->ike_last_message) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to return message buffer",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the last message buffer field to NULL. */
        phase2->ike_last_message = NU_NULL;
    }

    /* If Initiator's Nonce data is allocated. */
    if(phase2->ike_nonce_i != NU_NULL)
    {
        /* Deallocate the Initiator's Nonce data. */
        if(NU_Deallocate_Memory(phase2->ike_nonce_i) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the Initiator's Nonce data field to NULL. */
        phase2->ike_nonce_i = NU_NULL;
    }

    /* If Responder's Nonce data is allocated. */
    if(phase2->ike_nonce_r != NU_NULL)
    {
        /* Deallocate the Responder's Nonce data. */
        if(NU_Deallocate_Memory(phase2->ike_nonce_r) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the Responder's Nonce data field to NULL. */
        phase2->ike_nonce_r = NU_NULL;
    }

    /* If the Diffie-Hellman remote public key is allocated. */
    if(phase2->ike_dh_remote_key != NU_NULL)
    {
        /* Deallocate the remote public key. */
        if(NU_Deallocate_Memory(phase2->ike_dh_remote_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate the memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the remote public key field to NULL. */
        phase2->ike_dh_remote_key = NU_NULL;
    }

    /* If the Diffie-Hellman key pair is allocated. */
    if(phase2->ike_dh_key.ike_public_key != NU_NULL)
    {
        /* Deallocate the key pair. */
        if(NU_Deallocate_Memory(phase2->ike_dh_key.ike_public_key) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Set the key pair fields to NULL. */
        phase2->ike_dh_key.ike_public_key  = NU_NULL;
        phase2->ike_dh_key.ike_private_key = NU_NULL;
    }

    /* Flush the SA2DB. */
    IKE_Flush_SA2(&phase2->ike_sa2_db);

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_Local_Phase2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Free_Phase2
*
* DESCRIPTION
*
*       This is a utility function used to deallocate all
*       dynamic fields of a Phase 2 Handle. The pointer
*       passed to this function must point to a dynamically
*       allocated Handle. The Handle itself is also
*       deallocated. Note that the caller is responsible for
*       removing the Handle from any database it might be a
*       part of.
*
* INPUTS
*
*       *phase2                 Pointer to the Handle which
*                               is to be deallocated.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE_Free_Phase2(IKE_PHASE2_HANDLE *phase2)
{
#if (IKE_DEBUG == NU_TRUE)
    /* Make sure Handle pointer is valid. */
    if(phase2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Free all dynamic fields of the Phase 2 Handle. Return
     * value of this function can be safely ignored.
     */
    if(IKE_Free_Local_Phase2(phase2) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate local Handle",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Also deallocate the memory of Phase 2 Handle. */
    if(NU_Deallocate_Memory(phase2) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate the memory",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Free_Phase2 */

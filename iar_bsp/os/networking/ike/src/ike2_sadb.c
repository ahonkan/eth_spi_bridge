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
*       ike2_sadb.c
*
* COMPONENT
*
*       IKEv2 - Database
*
* DESCRIPTION
*
*       This file contains functions related to management of SA and
*       exchange handle databases. It contains functions for addition,
*       deletion and searching from these databases.
*
* FUNCTIONS
*
*       IKE2_Add_IKE_SA
*       IKE2_Delete_IKE_SA
*       IKE2_Find_SADB_By_SA
*       IKE2_Add_Exchange_Handle
*       IKE2_Find_SA
*       IKE2_Exchange_Lookup
*       IKE2_Exchange_Lookup_By_ID
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike_ips.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_ips.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

extern IKE_POLICY_GROUP_DB IKE_Group_DB;

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Add_IKE_SA
*
* DESCRIPTION
*
*       Adds an IKE SA to the SADB specified.
*
* INPUTS
*
*       *sadb                   SA database the SA is to be added to.
*       *sa                     SA that needs to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              Successful addition of SA in the database.
*       IKE_GEN_ERROR           Failed to add SA.
*
*************************************************************************/
STATUS IKE2_Add_IKE_SA(IKE2_SADB *sadb, IKE2_SA *sa)
{
    STATUS status = NU_SUCCESS;

#if(IKE2_DEBUG == NU_TRUE)
    if((sadb == NU_NULL) || (sa == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Set version of the SA to IKEv2. */
    sa->ike2_version = IKE_VERSION_2;

    /* Enqueue the SA in the SADB. */
    if(SLL_Enqueue(sadb, sa) == NU_NULL)
    {
        status = IKE_GEN_ERROR;
    }

    return (status);

} /* IKE2_Add_IKE_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Delete_IKE_SA
*
* DESCRIPTION
*
*       Deletes an IKE SA from SADB specified.
*
* INPUTS
*
*       *sadb                   Database from which to delete the SA.
*       *sa                     SA to be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful deletion of SA.
*
*************************************************************************/
STATUS IKE2_Delete_IKE_SA(IKE2_SADB *sadb, IKE2_SA *sa)
{
    STATUS                      status = NU_SUCCESS;
    IKE2_IPS_SA_INDEX           *ipsec_index;
    IKE2_IPS_SA_INDEX           *ipsec_index_next;
    IKE2_EXCHANGE_HANDLE        *handle;
    IKE2_EXCHANGE_HANDLE        *next_handle;

    IPSEC_OUTBOUND_INDEX_REAL   ips_index;

#if (IKE2_DEBUG == NU_TRUE)
    if((sadb == NU_NULL) || (sa == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the SA being removed is owned by IKEv2. */
    if (sa->ike2_version != IKE_VERSION_2)
    {
        return (IKE2_INVALID_PARAMS);
    }

    ipsec_index = sa->ips_sa_index.flink;

    /* Loop through indices of all IPsec SAs negotiated under this IKE
     * SA. Delete all of them since all the child SAs must be deleted
     * when the IKE SA they were negotiated under is deleted.
     */
    while(ipsec_index != NU_NULL)
    {
        /* Save the next pointer to use after this structure has been
         * deallocated.
         */
        ipsec_index_next = ipsec_index->flink;

        ips_index.ipsec_spi = ipsec_index->ips_spi;
        ips_index.ipsec_group = sa->ike2_current_handle->ike2_ips_group;
        ips_index.ipsec_protocol = ipsec_index->protocol;
        ips_index.ipsec_dest = sa->ike_node_addr.id.is_ip_addrs;

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
        ips_index.ipsec_dest_type = (IPSEC_SINGLE_IP |
                                     IKE_IPS_FAMILY_TO_FLAGS(
                                        sa->ike_node_addr.family));
#elif (INCLUDE_IPV4 == NU_TRUE)
        ips_index.ipsec_dest_type = IPSEC_SINGLE_IP | IPSEC_IPV4;
#elif (INCLUDE_IPV6 == NU_TRUE)
        ips_index.ipsec_dest_type = IPSEC_SINGLE_IP | IPSEC_IPV6;
#endif

        /* Remove the SA pair identified by ips_index. */
        if(IPSEC_Remove_SA_Pair(&ips_index) == NU_SUCCESS)
        {
            IKE2_DEBUG_LOG("Deleted IPsec SA pair on remote request");
        }

        else
        {
            /* This is not an error so only log it as a debug message. */
            IKE2_DEBUG_LOG("Failed to delete IPsec SA on request");
        }

        /* Deallocate the IPsec SA index structure. */
        if(NU_Deallocate_Memory(ipsec_index) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for IPsec index",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Use the next pointer we saved. */
        ipsec_index = ipsec_index_next;
    }

    /* Remove the SA from the SADB list. */
    if(SLL_Remove(sadb, sa) != NU_NULL)
    {
        /* SA was successfully removed. Clean up any buffers allocated
         * for this SA.
         */

        if(sa->ike2_skeyseed != NU_NULL)
        {
            if(NU_Deallocate_Memory(sa->ike2_skeyseed) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate the SKEYSEED buffer",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            sa->ike2_skeyseed = NU_NULL;
        }

        if(sa->ike2_sk_d != NU_NULL)
        {
            if(NU_Deallocate_Memory(sa->ike2_sk_d) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate the SA keys buffer",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            sa->ike2_sk_d = NU_NULL;
        }

        if(sa->ike2_local_auth_data != NU_NULL)
        {
            if(NU_Deallocate_Memory(sa->ike2_local_auth_data)
                                    != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate local auth data",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
            sa->ike2_local_auth_data = NU_NULL;
        }

        if(sa->ike2_peer_auth_data != NU_NULL)
        {
            if(NU_Deallocate_Memory(sa->ike2_peer_auth_data)
                                    != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate peer auth data",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
            sa->ike2_peer_auth_data = NU_NULL;
        }

        /* Delete all Exchange Handles associated with this IKE2 SA. */
        handle = sa->xchg_db.ike2_flink;

        while(handle != NU_NULL)
        {
            next_handle = handle->ike2_flink;

            if (IKE2_Cleanup_Exchange(handle) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate Exchange Handle",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            handle = next_handle;
        }

        /* Now delete the memory allocated for the SA structure itself. */
        if((sa != NU_NULL) && (NU_Deallocate_Memory(sa) != NU_SUCCESS))
        {
            NLOG_Error_Log("Failed to deallocate memory for SA",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* IKE2_Delete_IKE_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Find_SADB_By_SA
*
* DESCRIPTION
*
*       This is a utility function which searches the complete
*       IKE Groups and Policies database to locate the SADB in
*       which the specified SA is contained. This function is
*       an inefficient way of locating the SADB and must not be
*       used except in unavoidable situations where a direct
*       reference to the required SADB is not available.
*
* INPUTS
*
*       *sa                     SA to be searched.
*       **out_sadb              The SADB to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              If the required SADB is found.
*       IKE_NOT_FOUND           If SA is not found in any SADB.
*
*************************************************************************/
STATUS IKE2_Find_SADB_By_SA(IKE2_SA *sa, IKE2_SADB **out_sadb)
{
    STATUS              status = IKE_NOT_FOUND;
    IKE2_SA             *cur_sa;
    IKE_POLICY_GROUP    *cur_group;
    IKE_POLICY          *cur_policy;

#if (IKE2_DEBUG == NU_TRUE)
    if((sa == NU_NULL) || (out_sadb == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Make sure the SA being removed is owned by IKEv2. */
    if (sa->ike2_version != IKE_VERSION_2)
    {
        return (IKE2_INVALID_PARAMS);
    }

    cur_group = IKE_Group_DB.ike_flink;

    /* Loop for all Groups in the database. */
    while((cur_group != NU_NULL) && (status == IKE_NOT_FOUND))
    {
        cur_policy = cur_group->ike_policy_list.ike_flink;

        /* Loop for all Policies in the current Group. */
        while((cur_policy != NU_NULL) && (status == IKE_NOT_FOUND))
        {
            cur_sa = (IKE2_SA *)(cur_policy->ike_sa_list.ike_flink);

            /* Loop for all SAs in the current Policy. */
            while(cur_sa != NU_NULL)
            {
                /* If the matching SA is found. */
                if(cur_sa == sa)
                {
                    *out_sadb = (IKE2_SADB *)(&cur_policy->ike_sa_list);
                    status = NU_SUCCESS;
                    break;
                }

                cur_sa = cur_sa->ike_flink;
            }

            cur_policy = cur_policy->ike_flink;
        }

        cur_group = cur_group->ike_flink;
    }

    return (status);

} /* IKE2_Find_SADB_By_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Add_Exchange_Handle
*
* DESCRIPTION
*
*       Adds an exchange handle to an exchange handle database.
*
* INPUTS
*
*       *db                     Database to add exchange handle to.
*       *handle                 Exchange handle to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              On successful addition.
*       IKE_GEN_ERROR           Handle could not be added.
*
*************************************************************************/
STATUS IKE2_Add_Exchange_Handle(IKE2_EXCHANGE_DB *db,
                                IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS status = NU_SUCCESS;

#if(IKE2_DEBUG == NU_TRUE)
    if((db == NU_NULL) || (handle == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Enqueue the handle structure in the handles database. */
    if(SLL_Enqueue(db, handle) == NU_NULL)
    {
        /* Handle could not be added. */
        status = IKE_GEN_ERROR;
    }

    return (status);
} /* IKE2_Add_Exchange_Handle */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Find_SA
*
* DESCRIPTION
*
*       Searches for an IKE SA in the specified SADB based on local and
*       remote SPI's.
*
* INPUTS
*
*       *db                     Database to be searched.
*       *spi_local              Local SPI.
*       *spi_peer               Remote SPI.
*       **out_sa                Pointer to SA to be returned.
*
* OUTPUTS
*
*       NU_SUCCESS              SA was found in the SADB.
*       IKE_SA_NOT_FOUND        SA not found in SADB.
*       IKE2_INVALID_PARAMS     Parameters passed were not correct.
*
*************************************************************************/
STATUS IKE2_Find_SA(IKE2_SADB *db, UINT8 *spi_local, UINT8 *spi_peer,
                    IKE2_SA **out_sa)
{
    STATUS      status = IKE_SA_NOT_FOUND;
    IKE2_SA     *ret_sa;

#if (IKE2_DEBUG == NU_TRUE)
    /* Input sanity checks. */
    if((db == NU_NULL) || (spi_local == NU_NULL) || (spi_peer == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Get the first SA in the database given. */
    ret_sa = (IKE2_SA*)(db->ike_flink);

    while(ret_sa != NU_NULL)
    {
        /* Match the local SPI of the SA. */
        if(memcmp(spi_local, ret_sa->ike2_local_spi, IKE2_SPI_LENGTH) == 0)
        {
            /* Local SPI matched. Now match remote SPI. An SA is good
             * for us only if both these SPI's match.
             */
            if(memcmp(spi_peer, ret_sa->ike2_remote_spi, IKE2_SPI_LENGTH)
                == 0)
            {
                status = NU_SUCCESS;
                break;
            }

            /* If we were initiator, the remote SPI is not yet updated
             * in the SA. Check if remote SPI is zero. If yes, we have
             * a match.
             */
            if (memcmp(ret_sa->ike2_remote_spi,
                       "\x00\x00\x00\x00\x00\x00\x00\x00",
                       IKE2_SPI_LENGTH) == 0)
            {
                status = NU_SUCCESS;
                break;
            }
        }

        ret_sa = ret_sa->ike_flink;
    }

    /* If we really found a matching SA, set the out parameter to point
     * at the SA we matched.
     */
    if(status == NU_SUCCESS)
    {
        *out_sa = ret_sa;
    }

    return (status);
} /* IKE2_Find_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Exchange_Lookup
*
* DESCRIPTION
*
*       Looks up for an exchange handle related to the SA supplied.
*
* INPUTS
*
*       *sa                     SA for which exchange handle is to be found
*       *select                 Selector to match.
*       **handle                Pointer to exchange handle to be returned.
*
* OUTPUTS
*
*       IKE_NOT_FOUND           Exchange handle not found.
*       NU_SUCCESS              Exchange handle found.
*       IKE2_INVALID_PARAMS     Parameters are not correct.
*
*************************************************************************/
STATUS IKE2_Exchange_Lookup(IKE2_SA *sa, IKE2_POLICY_SELECTOR *select,
                            IKE2_EXCHANGE_HANDLE **handle)
{
    STATUS                  status = IKE_NOT_FOUND;
    IKE2_EXCHANGE_HANDLE    *xchg_handle;
    IKE_POLICY_SELECTOR     selector;

#if (IKE2_DEBUG == NU_TRUE)
    /* Parameter sanity checks. */
    if((sa == NU_NULL) || (select == NU_NULL) || (handle == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    xchg_handle = sa->xchg_db.ike2_flink;

    while(xchg_handle != NU_NULL)
    {
        /* Convert selector to format understood by IKE. */
        IKE_IPS_Selector_To_IKE(&xchg_handle->ike2_ips_selector, &selector);

        /* Match selectors. */
        if(IKE_Match_Selectors(select, &selector) == NU_TRUE)
        {
            /* Selector matched. Exchange handle found. */
            *handle = xchg_handle;
            sa->ike2_current_handle = xchg_handle;
            status = NU_SUCCESS;
            break;
        }

        xchg_handle = xchg_handle->ike2_flink;
    }

    return (status);
} /* IKE2_Exchange_Lookup */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Exchange_Lookup_By_ID
*
* DESCRIPTION
*
*       Search for exchange handle associated with an SA based on
*       message ID.
*
* INPUTS
*
*       *sa                     SA to find exchange handle for.
*       msg_id                  Message ID to be looked for.
*       **handle                Pointer to the handle to be returned.
*
* OUTPUTS
*
*       IKE_NOT_FOUND           Exchange handle not found.
*       NU_SUCCESS              Exchange handle found.
*
*************************************************************************/
STATUS IKE2_Exchange_Lookup_By_ID(IKE2_SA *sa, UINT32 msg_id,
                                  IKE2_EXCHANGE_HANDLE **handle)
{
    STATUS                  status = IKE_NOT_FOUND;
    IKE2_EXCHANGE_HANDLE    *xchg_handle;

#if (IKE2_DEBUG == NU_TRUE)
    if((sa == NU_NULL) || (handle == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    xchg_handle = sa->xchg_db.ike2_flink;

    while(xchg_handle != NU_NULL)
    {
        /* Different exchanges can have same message ID's. Check to see
         * if message ID and IKE SA pointer both match. Otherwise, we
         * could pick some exchange handle with wrong SA pointer. Multiple
         * exchange handles can point to same IKE SA in the case when there
         * are multiple child SA's created under the same IKE SA. Hence,
         * both the checks.
         */
        if((xchg_handle->ike2_next_id_nu == msg_id || xchg_handle->ike2_next_id_peer == msg_id)
            && (xchg_handle->ike2_sa == sa))
        {
            /* Handle found, break out of the loop. */
            status = NU_SUCCESS;
            break;
        }

        xchg_handle = xchg_handle->ike2_flink;
    }

    /* Set the pointer to be returned. */
    if(status == NU_SUCCESS)
    {
        *handle = xchg_handle;
    }

    return (status);
} /* IKE2_Exchange_Lookup_By_ID */

#endif

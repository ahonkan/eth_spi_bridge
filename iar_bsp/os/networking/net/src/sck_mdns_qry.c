/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       sck_mdns_qry.c
*
*   DESCRIPTION
*
*       This file contains the routines to start and stop an mDNS
*       continuous query.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Start_mDNS_Query
*       NU_Stop_mDNS_Query
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_MDNS == NU_TRUE)
extern MDNS_QUERY_LIST      MDNS_Query_List;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Start_mDNS_Query
*
*   DESCRIPTION
*
*       This routine is used to invoke an mDNS continuous query for the
*       indicated record type.  This routine returns immediately, and the
*       query operation occurs in the background in a separate thread.
*       When an answer is received from a foreign node, the caller of
*       this routine is signaled with a Nucleus PLUS signal.
*
*   INPUTS
*
*       *data                   Pointer to the data to use in the question
*                               part of the outgoing DNS query.
*       type                    The type of record to query; DNS_TYPE_A
*                               DNS_TYPE_AAAA, DNS_TYPE_PTR, DNS_TYPE_SRV,
*                               and DNS_TYPE_TXT are currently supported.
*       family                  The family of the target record;
*                               NU_FAMILY_IP, NU_FAMILY_IP6 or
*                               NU_FAMILY_UNSPEC if the record does not
*                               resolve to an IP address.
*       *ptr                    Currently unused.  Intended for future
*                               functionality as required.
*
*   OUTPUTS
*
*       NU_SUCCESS              The query has been started.  The calling
*                               thread will be signaled when a new record
*                               for the query is found.
*       NU_INVALID_PARM         An input parameter is invalid.
*       An operating system specific error is returned otherwise.
*
*************************************************************************/
STATUS NU_Start_mDNS_Query(CHAR *data, INT type, INT16 family, VOID *ptr)
{
    STATUS              status;

#if (INCLUDE_MDNS == NU_TRUE)
    MDNS_QUERY          *qry_ptr;
    MDNS_NTFY_STRUCT    *ntfy_struct;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input. */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (data == NU_NULL) || ((type != DNS_TYPE_PTR) && (type != DNS_TYPE_SRV) &&
         (type != DNS_TYPE_TXT) &&
#if (INCLUDE_IPV4 == NU_TRUE)
         (type != DNS_TYPE_A)
#if (INCLUDE_IPV6 == NU_TRUE)
            &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
         (type != DNS_TYPE_AAAA)
#endif
         ) || (
#if (INCLUDE_IPV4 == NU_TRUE)
         (family != NU_FAMILY_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
         &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
         (family != NU_FAMILY_IP6)
#endif
            && ((family != NU_FAMILY_UNSPEC) || ((type != DNS_TYPE_PTR) &&
                (type != DNS_TYPE_SRV) && (type != DNS_TYPE_TXT)) )) )
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore. */
    status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Check if there is already a query matching this data. */
        qry_ptr = MDNS_Find_Matching_Query_By_Data(data, type, family);

        /* If there is not a matching query, create one. */
        if (!qry_ptr)
        {
            qry_ptr = MDNS_Add_Query(data, type, family);
        }

        /* If a query pointer was found or created, set up the callback structure. */
        if ( (qry_ptr) && (status == NU_SUCCESS) )
        {
            /* Allocate memory for the callback structure. */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&ntfy_struct,
                                        sizeof(MDNS_NTFY_STRUCT), NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                memset(ntfy_struct, 0, sizeof(MDNS_NTFY_STRUCT));

                /* Set up the data structure. */
                ntfy_struct->callback_ptr = NU_Current_Task_Pointer();

                /* Add this entry to the list. */
                DLL_Enqueue(&qry_ptr->mdns_callback, ntfy_struct);

                /* The initial timer should expire between 20 and 100 ms. */
                qry_ptr->mdns_qry_expire = (UTL_Rand() % MDNS_INIT_QUERY_MAX_DELAY) +
                         MDNS_INIT_QUERY_MIN_DELAY;

                /* If the timer is not already running, set it now.  If it is already running,
                 * this routine will return an error, which can be ignored.
                 */
                NU_Reset_Timer(&qry_ptr->mdns_qry_timer, MDNS_Query_Handler, qry_ptr->mdns_qry_expire,
                               0, NU_ENABLE_TIMER);
            }
        }

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

#else

    status = NU_INVALID_PARM;

#endif

    return (status);

} /* NU_Start_mDNS_Query */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Stop_mDNS_Query
*
*   DESCRIPTION
*
*       This routine is used to stop an mDNS continuous query that was
*       initiated previously via a call to NU_Start_mDNS_Query().
*
*   INPUTS
*
*       *data                   Pointer to the data to use in the question
*                               part of the outgoing DNS query.
*       type                    The type of record to query; DNS_TYPE_A
*                               DNS_TYPE_AAAA, DNS_TYPE_PTR, DNS_TYPE_SRV,
*                               and DNS_TYPE_TXT are currently supported.
*       family                  The family of the target record;
*                               NU_FAMILY_IP, NU_FAMILY_IP6 or
*                               NU_FAMILY_UNSPEC if the record does not
*                               resolve to an IP address.
*       *ptr                    Currently unused.  Intended for future
*                               functionality as required.
*
*   OUTPUTS
*
*       NU_SUCCESS              The query has been stopped.  The calling
*                               thread will be signaled when a new record
*                               for the query is found.
*       NU_INVALID_PARM         An input parameter is invalid.
*       NU_NO_ACTION            The caller is not currently querying the
*                               record.
*
*       An operating system specific error is returned otherwise.
*
*************************************************************************/
STATUS NU_Stop_mDNS_Query(CHAR *data, INT type, INT16 family, VOID *ptr)
{
    STATUS              status;

#if (INCLUDE_MDNS == NU_TRUE)
    MDNS_QUERY          *qry_ptr;
    MDNS_NTFY_STRUCT    *ntfy_struct;
    MDNS_RESPONSE       *res_ptr;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (data == NU_NULL) || ((type != DNS_TYPE_PTR) && (type != DNS_TYPE_SRV) &&
         (type != DNS_TYPE_TXT) &&
#if (INCLUDE_IPV4 == NU_TRUE)
         (type != DNS_TYPE_A)
#if (INCLUDE_IPV6 == NU_TRUE)
            &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
         (type != DNS_TYPE_AAAA)
#endif
         ) || (
#if (INCLUDE_IPV4 == NU_TRUE)
         (family != NU_FAMILY_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
         &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
         (family != NU_FAMILY_IP6)
#endif
            && ((family != NU_FAMILY_UNSPEC) || ((type != DNS_TYPE_PTR) &&
                (type != DNS_TYPE_SRV) && (type != DNS_TYPE_TXT)) )) )
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore. */
    status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the query structure. */
        qry_ptr = MDNS_Find_Matching_Query_By_Data(data, type, family);

        /* If a query structure exists. */
        if (qry_ptr)
        {
            ntfy_struct = qry_ptr->mdns_callback.head;

            /* Find this task on the list of callback pointers. */
            while (ntfy_struct)
            {
                if (ntfy_struct->callback_ptr == NU_Current_Task_Pointer())
                    break;

                ntfy_struct = ntfy_struct->next;
            }

            if (ntfy_struct)
            {
                /* Remove this task from the callback structure. */
                DLL_Remove(&qry_ptr->mdns_callback, ntfy_struct);

                /* Deallocate this memory. */
                NU_Deallocate_Memory(ntfy_struct);

                /* If the callback list is empty, there are no other application layer
                 * tasks interested in this query.  Delete the query.
                 */
                if (qry_ptr->mdns_callback.head == NU_NULL)
                {
                    /* Stop the timer. */
                    NU_Control_Timer(&qry_ptr->mdns_qry_timer, NU_DISABLE_TIMER);

                    /* Deallocate all the response pointers. */
                    while (qry_ptr->mdns_host_list.dns_head)
                    {
                        res_ptr = qry_ptr->mdns_host_list.dns_head;

                        /* Clean up the timer. */
                        if (res_ptr->mdns_host->mdns_timer)
                        {
                            /* Disable the timer that is running for querier cache
                             * maintenance for this record since there are no users
                             * interested in this record any more.
                             */
                            NU_Control_Timer(res_ptr->mdns_host->mdns_timer, NU_DISABLE_TIMER);

                            /* Since the timer is no longer being used for continuous
                             * queries, delete the timer.
                             */
                            NU_Delete_Timer(res_ptr->mdns_host->mdns_timer);

                            /* Deallocate memory for the timer. */
                            NU_Deallocate_Memory(res_ptr->mdns_host->mdns_timer);
                            res_ptr->mdns_host->mdns_timer = NU_NULL;
                        }

                        /* Break the link between the response and the query. */
                        res_ptr->mdns_host->mdns_query = NU_NULL;

                        /* Remove this response record from the query's list. */
                        DLL_Remove(&qry_ptr->mdns_host_list, res_ptr);

                        /* Deallocate the memory. */
                        NU_Deallocate_Memory(res_ptr);
                    }

                    /* Remove this query from the list. */
                    DLL_Remove(&MDNS_Query_List, qry_ptr);

                    /* Delete the memory for this query. */
                    NU_Deallocate_Memory(qry_ptr);
                }
            }

            /* This task is not querying the indicated record. */
            else
            {
                status = NU_NO_ACTION;
            }
        }

        /* No matching query was found. */
        else
        {
            status = NU_NO_ACTION;
        }

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

#else

    status = NU_INVALID_PARM;

#endif

    return (status);

} /* NU_Stop_mDNS_Query */

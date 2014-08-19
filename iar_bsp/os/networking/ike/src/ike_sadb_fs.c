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
*       ike_sadb_fs.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Flush_SAs.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Flush_SAs
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_api.h
*       ike_evt.h
*
************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_evt.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Flush_SAs
*
* DESCRIPTION
*
*       This function removes all SAs from the specified database
*       (SADB). This is a utility function used internally by
*       IKE.
*
*       The caller is responsible for obtaining the IKE semaphore
*       before calling this function.
*
*       If this function is not called in response to an
*       IKE_Shutdown request, then it obtains the TCP semaphore.
*
* INPUTS
*
*       *sadb                   Database to be flushed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
************************************************************************/
STATUS IKE_Flush_SAs(IKE_SADB *sadb)
{
    STATUS          status = NU_SUCCESS;
    IKE_SA          *sa;
    IKE_SA          *next_sa;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the SADB pointer is not NULL. */
    if(sadb == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* If IKE is not shutting down. */
    if(IKE_Daemon_State != IKE_DAEMON_STOPPING_MISC)
    {
        /* Grab the TCP semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);
    }

    if(status == NU_SUCCESS)
    {
        /* Start from the first SA in the database. */
        sa = sadb->ike_flink;

        /* Loop for all SAs. */
        while(sa != NU_NULL)
        {
            /* Save pointer to the next SA. */
            next_sa = sa->ike_flink;

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

            /* If IKE is not shutting down. */
            if(IKE_Daemon_State != IKE_DAEMON_STOPPING_MISC)
            {
                /* Remove all IKE timer events, which reference the
                 * current SA, from NET TQ. Return value of this
                 * function can be safely ignored.
                 */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                if (sa->ike2_version == IKE_VERSION_1)
                {
#endif
                    if(IKE_Unset_Matching_Timers((UNSIGNED)sa, 0,
                                             TQ_CLEAR_ALL_EXTRA)
                                             != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to unset IKE timers",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                }
                else
                {
                    if(IKE2_Unset_Matching_Timers((UNSIGNED)sa, 0,
                                             TQ_CLEAR_ALL_EXTRA)
                                             != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to unset IKE2 timers",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
#endif
                /* Similarly, remove all matching events from the
                 * IKE timer list.
                 */
                IKE_Unset_Matching_Events((UNSIGNED)sa, 0,
                                          TQ_CLEAR_ALL_EXTRA);
            }

            /* Deallocate all dynamic fields of the SA and
             * all Phase 2 Handles. No need to remove using
             * SLL_Remove because all SAs are being removed
             * from the SADB.
             */
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
            if (sa->ike2_version == IKE_VERSION_2)
            {
                if(IKE_Daemon_State != IKE_DAEMON_STOPPING_MISC)
                {
                    /* Release the TCP semaphore. */
                    if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                if(IKE2_Delete_IKE_SA(sadb, sa) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate IKEv2 SA",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                if(IKE_Daemon_State != IKE_DAEMON_STOPPING_MISC)
                {
                    if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT)
                        != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to obtain semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
            }
            else
#endif
            if(IKE_Free_SA(sa) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate SA",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Move to the next SA. */
            sa = next_sa;
        }

        /* Set SADB pointers to NULL. */
        sadb->ike_flink = NU_NULL;
        sadb->ike_last  = NU_NULL;

        /* If IKE is not shutting down. */
        if(IKE_Daemon_State != IKE_DAEMON_STOPPING_MISC)
        {
            /* Release the TCP semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Flush_SAs */

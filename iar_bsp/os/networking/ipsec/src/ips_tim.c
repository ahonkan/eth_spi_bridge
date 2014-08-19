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
*       ips_tim.c
*
* COMPONENT
*
*       LIFETIMES
*
* DESCRIPTION
*
*       This file contains implementation for IPsec SAs and outbound
*       bundles lifetime component including initialization and
*       event handlers.
*
* DATA STRUCTURES
*
*       IPSEC_Soft_Lifetime_Event
*       IPSEC_Hard_Lifetime_Event
*       IPSEC_Out_Bundle_Lifetime_Event
*
* FUNCTIONS
*
*       IPSEC_Lifetimes_Init
*       IPSEC_Soft_Lifetime_Expired
*       IPSEC_Hard_Lifetime_Expired
*       IPSEC_Bundle_Lifetime_Expired
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*       ike_api.h
*       ike_evt.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"


#if (INCLUDE_IKE == NU_TRUE)
#include "networking/ike_api.h"
#include "networking/ike_evt.h"
#endif

/* Declare the timer event for the soft lifetimes of IPsec SAs. */
TQ_EVENT        IPSEC_Soft_Lifetime_Event;

/* Declare the timer event for the hard lifetimes of IPsec SAs. */
TQ_EVENT        IPSEC_Hard_Lifetime_Event;

/* Declare the timer event for the outbound bundles. */
TQ_EVENT        IPSEC_Out_Bundle_Lifetime_Event;

/* This component will be only enabled when IKE is present.*/

#if (INCLUDE_IKE == NU_TRUE)

/************************************************************************
* FUNCTION
*
*       IPSEC_Lifetimes_Init
*
* DESCRIPTION
*
*       This function initializes the Lifetimes component of the Nucleus
*       IPsec.
*
* INPUTS
*
*        None
*
* OUTPUTS
*
*       NU_SUCCESS              In case of successful initialization.
*       Error Status            Error status returned by
*                               EQ_Register_Event() call.
*
************************************************************************/
STATUS IPSEC_Lifetimes_Init(VOID)
{
    STATUS      status;

    /* Register the event for the soft lifetimes of the SAs. */
    status = EQ_Register_Event(IPSEC_Soft_Lifetime_Expired,
                              &IPSEC_Soft_Lifetime_Event);

    /* Check the status value before going ahead. */
    if(status == NU_SUCCESS)
    {
        /* Register the event for the hard lifetimes of the SAs. */
        status = EQ_Register_Event(IPSEC_Hard_Lifetime_Expired,
                                  &IPSEC_Hard_Lifetime_Event);

        /* Check the status value before going ahead. */
        if(status == NU_SUCCESS)
        {
            /* Register the event for the hard lifetimes of the SAs. */
            status = EQ_Register_Event(IPSEC_Bundle_Lifetime_Expired,
                                      &IPSEC_Out_Bundle_Lifetime_Event);

            if(status != NU_SUCCESS)
            {
                /* Unregister the SAs event for hard lifetime. */
                status = EQ_Unregister_Event(IPSEC_Hard_Lifetime_Event);

                if (status == NU_SUCCESS)
                {
                    /* Unregister the SAs event for soft lifetime. */
                    status =
                        EQ_Unregister_Event(IPSEC_Soft_Lifetime_Event);
                }
            }
        }
        else
        {
            /* Unregister the SAs event for soft lifetime. */
            status = EQ_Unregister_Event(IPSEC_Soft_Lifetime_Event);
        }
    }

    /* Return the status value to the caller. */
    return (status);

} /* IPSEC_Lifetimes_Init. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Soft_Lifetime_Expired
*
* DESCRIPTION
*
*       This function is the event handler, triggered upon expiration
*       of soft lifetime of an SA. If requested, a warning will be logged
*       and a request will be made to IKE for refreshing the expired SA.
*
* INPUTS
*
*       event                   The event that has occurred.
*       lifetime                Soft lifetime of an SA for which event
*                               has occurred.
*       dev_id                  Current device ID.
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID IPSEC_Soft_Lifetime_Expired(TQ_EVENT event, UNSIGNED lifetime,
                                 UNSIGNED dev_id)
{
    IKE_INITIATE_REQ        *ike_req;
    IPSEC_SA_LIFETIME       *sa_lifetime = (IPSEC_SA_LIFETIME *)lifetime;

    UNUSED_PARAMETER(event);

    /* First grab the semaphore. */
    if(NU_Obtain_Semaphore(&IPSEC_Resource,IPSEC_SEM_TIMEOUT) !=
                                                            NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Check if it is required to log the warning. */
        if((sa_lifetime->ipsec_expiry_action & IPSEC_LOG_WARNING) != 0)
        {
            NLOG_Error_Log("SA soft lifetime expired",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* Now check if it is required to refresh the SA as well. */
        if((sa_lifetime->ipsec_expiry_action & IPSEC_REFRESH_SA) != 0)
        {
            /* First check if the attached outbound SA has used at
             * least once or not. If not then no need to refresh this SA.
             * No communication has been made with the other peer so far.
             *
             * Also Make sure that IKE daemon thread is up. If
             * not then no need to request anything.
             */
            if(((sa_lifetime->ipsec_flags &
                        (IPSEC_OUT_SA_USED | IPSEC_IN_SA_USED)) != 0) &&
                        (IKE_Daemon_State == IKE_DAEMON_RUNNING))
            {
                /* Now get memory for an IKE request structure.
                 * This memory will be freed by IKE.
                 */
                if(NU_Allocate_Memory(IPSEC_Memory_Pool,(VOID **)&ike_req,
                                sizeof(IKE_INITIATE_REQ),
                                            NU_NO_SUSPEND) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to allocate the memory ",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
                else
                {
                    /* First normalize the pointer. */
                    ike_req = TLS_Normalize_Ptr(ike_req);

                    /* Copy the device ID. */
                    ike_req->ike_dev_index = dev_id;

                    /* Copy the security protocol. */
                    NU_BLOCK_COPY(&(ike_req->ike_ips_security),
                        &(sa_lifetime->ipsec_out_sa->ipsec_security),
                        sizeof(IPSEC_SECURITY_PROTOCOL));

                    /* Copy the selector. */
                    NU_BLOCK_COPY(&(ike_req->ike_ips_select),
                            &(sa_lifetime->ipsec_out_sa->ipsec_select),
                            sizeof(IPSEC_SELECTOR));

                    /* This should always be a non blocking call. */
                    ike_req->ike_suspend = NU_NO_SUSPEND;

                    /* Now call the IKE routine for refreshing the SA. */
                    IKE_SYNC_INITIATE(ike_req);
                }
            } /* End of if() check. */
        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

} /* IPSEC_Soft_Lifetime_Expired. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Hard_Lifetime_Expired
*
* DESCRIPTION
*
*       This function is the event handler, triggered upon expiration
*       of an SA. If requested, a warning is logged and SA entry
*       is removed from its corresponding DB for both outbound and
*       inbound SAs.
*
* INPUTS
*
*       event                   The event that has occurred.
*       lifetime                Hard lifetime of an SA for which event
*                               has occurred.
*       dev_id                  Current device ID
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID IPSEC_Hard_Lifetime_Expired(TQ_EVENT event, UNSIGNED lifetime,
                                 UNSIGNED dev_id)
{
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)

    IKE_INITIATE_REQ    *ike_req;

#endif

    IPSEC_SA_LIFETIME   *sa_lifetime    = (IPSEC_SA_LIFETIME *)lifetime;
    IPSEC_POLICY_GROUP  *policy_group;
    DV_DEVICE_ENTRY     *device;

    UNUSED_PARAMETER(event);

    /* First grab the semaphore. */
    if(NU_Obtain_Semaphore(&IPSEC_Resource,IPSEC_SEM_TIMEOUT) !=
                                                            NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Check if it is required to log the warning. */
        if((sa_lifetime->ipsec_expiry_action & IPSEC_LOG_WARNING) != 0)
        {
            NLOG_Error_Log("SA hard lifetime expired",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
        /* Try to send an SA delete notification message to the
         * remote node to ensure synchronization. Make sure that
         * the IKE daemon thread is up. If not then there is no
         * need for this request.
         */
        if(IKE_Daemon_State == IKE_DAEMON_RUNNING)
        {
            /* Now get memory for an IKE request structure.
             * This will be freed by IKE.
             */
            if(NU_Allocate_Memory(IPSEC_Memory_Pool,(VOID **)&ike_req,
                                  sizeof(IKE_INITIATE_REQ),
                                  NU_NO_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to allocate the memory.",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
            else
            {
                /* First normalize the pointer. */
                ike_req = TLS_Normalize_Ptr(ike_req);

                /* Set the device ID. */
                ike_req->ike_dev_index = dev_id;

                /* Copy the security protocol. */
                NU_BLOCK_COPY(&(ike_req->ike_ips_security),
                    &(sa_lifetime->ipsec_out_sa->ipsec_security),
                    sizeof(IPSEC_SECURITY_PROTOCOL));

                /* The tunnel source must contain the SPI of the
                 * inbound SA. This member has been reused to save
                 * space since the tunnel source is unused in an
                 * SA delete notification request.
                 */
                PUT32(ike_req->ike_ips_security.ipsec_tunnel_source,
                      0, sa_lifetime->ipsec_in_sa->ipsec_spi);

                /* Copy the selector. */
                NU_BLOCK_COPY(&(ike_req->ike_ips_select),
                    &(sa_lifetime->ipsec_out_sa->ipsec_select),
                    sizeof(IPSEC_SELECTOR));

                /* This should always be a non blocking call. */
                ike_req->ike_suspend = NU_NO_SUSPEND;

                /* Make the request for an SA delete notification. */
                IKE_SYNC_DELETE_NOTIFY(ike_req);
            }
        }
#endif /* #if (IKE_INCLUDE_INFO_MODE == NU_TRUE) */

        /* Get device from its Index or ID. TCP semaphore is already
            obtained. */
        device = DEV_Get_Dev_By_Index(dev_id);

        /* Get the IPsec Policy group associated with the device. */
        policy_group = device->dev_physical->dev_phy_ips_group;

        /* Remove the outbound SA first from outbound SADB. */
        SLL_Remove(&(policy_group->ipsec_outbound_sa_list),
                    sa_lifetime->ipsec_out_sa);

        /* Now remove inbound SA from inbound SADB. */
        SLL_Remove(&(policy_group->ipsec_inbound_sa_list),
                    sa_lifetime->ipsec_in_sa);

        /* Deallocate the outbound SA memory. */
        if(NU_Deallocate_Memory(sa_lifetime->ipsec_out_sa) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory.",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Now deallocate the inbound SA memory. */
        if(NU_Deallocate_Memory(sa_lifetime->ipsec_in_sa) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory.", NERR_SEVERE,
                           __FILE__, __LINE__);

        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

} /* IPSEC_Hard_Lifetime_Expired. */

/************************************************************************
* FUNCTION
*
*       IPSEC_Bundle_Lifetime_Expired
*
* DESCRIPTION
*
*       This function is the event handler, triggered upon expiration
*       of lifetime of an outbound bundle. Bundle's entry is removed from
*       the policy with which it is attached and then its memory is
*       deallocated.
*
* INPUTS
*
*       event                   The event that has occurred.
*       bundle                  Hard lifetime of an SA for which event
*                               has occurred.
*       policy                  The policy to which corresponding this
*                               bundle belongs.
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID IPSEC_Bundle_Lifetime_Expired(TQ_EVENT event, UNSIGNED bundle,
                                       UNSIGNED policy)
{
    IPSEC_OUTBOUND_BUNDLE   *out_bundle = (IPSEC_OUTBOUND_BUNDLE *)bundle;
    IPSEC_POLICY            *policy_ptr = (IPSEC_POLICY *)policy;

    UNUSED_PARAMETER(event);

    /* First grab the semaphore. */
    if(NU_Obtain_Semaphore(&IPSEC_Resource,IPSEC_SEM_TIMEOUT) !=
                                                            NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

    }
    else
    {
        /* Remove the outbound bundle first from the policy. */
        SLL_Remove(&(policy_ptr->ipsec_bundles), out_bundle);

        /* Deallocate the outbound bundle memory. */
        if(NU_Deallocate_Memory(out_bundle) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the outbound bundle memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the IPsec semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
} /* IPSEC_Bundle_Lifetime_Expired */

#endif /* #if (INCLUDE_IKE == NU_TRUE). */

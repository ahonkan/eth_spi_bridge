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
*       ike2_evt.c
*
* COMPONENT
*
*       IKEv2 - Events
*
* DESCRIPTION
*
*       This file contains the implementation of the IKEv2 Events
*       which are based on the Nucleus NET timers.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IKE2_Message_Reply_Handler
*       IKE2_SA_Timeout_Handler
*       IKE2_Cookie_Secret_Timeout_Handler
*       IKE2_SA_Remove_Event
*       IKE2_Exchange_Timeout_Event
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_api.h
*       ike_evt.h
*       ike2_evt.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_evt.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#include "networking/ike2_evt.h"

/* Event ID of each type of event expected by IKE2 */
TQ_EVENT IKE2_Message_Reply_Event;
TQ_EVENT IKE2_SA_Timeout_Event;
TQ_EVENT IKE2_INIT_AUTH_Timeout;
TQ_EVENT IKE2_Cookie_Secret_Timeout_Event;

/* The below list and event handler function is no longer used since we are
 * utilizing the list from v1.
 */

/* List of allocated and free IKE2 events. */
IKE2_EVENT_DB IKE2_Event_List;
IKE2_EVENT_DB IKE2_Event_Freelist;

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Message_Reply_Handler
*
* DESCRIPTION
*
*       This function handles the IKE2_Message_Reply_Event event.
*       It re-sends the message which was last
*       transmitted.
*
* INPUTS
*
*       *sa                     Pointer to the IKEv2 SA.
*       *handle                 Pointer to the Exchange Handle.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE2_Message_Reply_Handler(IKE2_SA *sa, IKE2_EXCHANGE_HANDLE *handle)
{
    /* Log the event. */
    IKE2_DEBUG_LOG("Exchange message reply timed-out");

    /* If the handle pointer is valid. */
    if(handle != NU_NULL)
    {
        /* Re-send the  message. */
        if(IKE2_Resend_Packet(handle) != NU_SUCCESS)
        {
            /* Failed to re-send message. */
            NLOG_Error_Log("Failed to re-send IKEv2 message on timeout",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If re-send count has not expired. */
        if(handle->ike2_resend_count)
        {
            /* Set the message reply timer again with exponential back-off. */
            if(IKE_Set_Timer(IKE2_Message_Reply_Event, (UNSIGNED)sa,
                (UNSIGNED)handle, (UNSIGNED)(IKE_RESEND_INTERVAL *
                (IKE_RESEND_COUNT - handle->ike2_resend_count + 1)))
                != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set message resend timer",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    return;

} /* IKE2_Message_Reply_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_SA_Timeout_Handler
*
* DESCRIPTION
*
*       This function handles the IKE2_SA_Timeout_Event and
*       IKE_SA_REMOVE_EVENT events. The difference between
*       the two is that the delete SA notification is only
*       sent for the first event.
*
* INPUTS
*
*       event                   Event identifier.
*       *sa                     Pointer to the IKEv2 SA.
*       *sadb                   Pointer to the SADB.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE2_SA_Timeout_Handler(TQ_EVENT event, IKE2_SA *sa, IKE2_SADB *sadb)
{
    UINT32  ret_id;
    IKE2_STATE_PARAMS    params;

    /* Log the event. */
    IKE2_DEBUG_LOG("IKEv2 SA timed-out");

    if((event == IKE2_SA_Timeout_Event) &&
        (sa->ike_state == IKE_SA_ESTABLISHED))
    {
        if((sa->ike2_flags & IKE2_SA_REKEY) != 0)
        {
            IKE2_Initiate_IKE2_Child_Exchange(NU_NULL,
                                    sa->ike2_current_handle->ike2_policy,
                                    NU_NULL, sa, &ret_id);
        }

        else
        {
            if(sa->ike2_current_handle != NU_NULL)
            {
                /* SA needs to be deleted. */
                UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));
                params.ike2_policy = sa->ike2_current_handle->ike2_policy;
                sa->ike2_current_handle->ike2_params = &params;

                if(IKE2_Send_Info_Delete(sa->ike2_current_handle,
                                         IKE2_PROTO_ID_IKE,
                                         0, 0, NU_NULL) != NU_SUCCESS)
                {
                    NLOG_Error_Log(
                        "Unable to send delete notification for IKEv2 SA",
                        NERR_INFORMATIONAL, __FILE__, __LINE__);
                }

                sa->ike2_current_handle->ike2_params = NU_NULL;
            }

            /* Remove the IKEv2 SA. */
            if(IKE2_Delete_IKE_SA(sadb, sa) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to remove SA from SADB",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        /* Grab the NET semaphore. */
        if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        else
        {
            /* Remove conflicting events containing the deleted SA
             * from the NET TQ timer list.
             */
            TQ_Timerunset(IKE2_Message_Reply_Event,
                TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);
            TQ_Timerunset(IKE2_SA_Timeout_Event,
                TQ_CLEAR_EXACT, (UNSIGNED)sa, (UNSIGNED)sadb);
            TQ_Timerunset(IKE2_Cookie_Secret_Timeout_Event,
                TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);

            /* Also remove conflicting events from the IKE event list. */
            IKE_Unset_Matching_Events((UNSIGNED)sa, 0,
                                      TQ_CLEAR_ALL_EXTRA);

            /* Release the NET semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return;

} /* IKE2_SA_Timeout_Handler. */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Cookie_Secret_Timeout_Handler
*
* DESCRIPTION
*
*       Event handler for cookie generation timeout event. We need to
*       periodically generate new cookie values.
*
* INPUTS
*
*       event                   Event identifier.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IKE2_Cookie_Secret_Timeout_Handler(TQ_EVENT event)
{
    UNUSED_PARAMETER(event);

    /* Call the function to generate new secret from cookie module.*/
    IKE2_Generate_Secret();

    return;
} /* IKE2_Cookie_Secret_Timeout_Handler */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_SA_Remove_Event
*
* DESCRIPTION
*
*       Handle event to remove an IKE SA.
*
* INPUTS
*
*       event                   Event identifier.
*       *sa                     IKE SA to be removed.
*       *sadb                   SADB to which this SA belongs.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IKE2_SA_Remove_Event(TQ_EVENT event, IKE2_EXCHANGE_HANDLE *handle,
                          IKE2_SADB *sadb)
{
    IKE2_DEBUG_LOG("IKE SA timed out");

    if((event == IKE_SA_Timeout_Event) &&
       (handle->ike2_sa->ike_state == IKE2_SA_ESTABLISHED))
    {
        /* Try to send delete notification to remote node. */
        if(IKE2_Send_Info_Delete(handle, IKE2_PROTO_ID_IKE, 0,
            0, NU_NULL) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to send delete notification for IKE SA",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    /* Grab the NET semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain the semaphore",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    else
    {
        /* Remove conflicting events containing the deleted SA
         * from the NET TQ timer list.
         */
        TQ_Timerunset(IKE2_Message_Reply_Event, TQ_CLEAR_ALL_EXTRA,
            (UNSIGNED)handle->ike2_sa, 0);
        TQ_Timerunset(IKE2_SA_Timeout_Event, TQ_CLEAR_EXACT,
            (UNSIGNED)handle->ike2_sa, (UNSIGNED)sadb);
        TQ_Timerunset(IKE2_INIT_AUTH_Timeout, TQ_CLEAR_EXACT,
            (UNSIGNED)handle->ike2_sa, (UNSIGNED)sadb);

        /* Also remove conflicting events from the IKE event list. */
        IKE_Unset_Matching_Events((UNSIGNED)handle->ike2_sa, 0,
            TQ_CLEAR_ALL_EXTRA);

        /* Remove the IKE SA. */
        if(IKE2_Delete_IKE_SA(sadb, handle->ike2_sa) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to remove SA from SADB",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return;
} /* IKE2_SA_Remove_Event */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Exchange_Timeout_Event
*
* DESCRIPTION
*
*       Handles the exchange timeout event. The resources need to be
*       cleaned up that were being held by this exchange handle.
*
* INPUTS
*
*       event                   Event identifier.
*       *sa                     IKE SA on which exchange timed out.
*       *handle                 The Exchange Handle which has timed out.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IKE2_Exchange_Timeout_Event(TQ_EVENT event, IKE2_SA *sa,
                                 IKE2_EXCHANGE_HANDLE *handle)
{
    IKE2_SADB       *sadb;
    STATUS          status;

    UNUSED_PARAMETER(event);

    if((sa != NU_NULL) && (handle != NU_NULL))
    {
        /* Make sure the SA is not in an established state. If it is,
         * then do nothing in this handler as the Exchange Handle will
         * be freed when the SA times out.
         *
         * Also note that if a CHILD SA Exchange has failed on an
         * established IKE SA then we won't take any action for that
         * because we assume use of a single Exchange Handle for all
         * Exchanges taking place on an IKE SA. If this assumption is
         * not true in the future, then removing the Exchange Handle
         * for such a failure should be handled here.
         */
        if(sa->ike_state != IKE2_SA_ESTABLISHED)
        {
            /* Grab the NET semaphore. */
            if(NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT) !=
                NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain the semaphore",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Remove conflicting events containing the
                 * deleted SA.
                 */
                TQ_Timerunset(IKE2_Message_Reply_Event,
                    TQ_CLEAR_ALL_EXTRA, (UNSIGNED)sa, 0);
                TQ_Timerunset(IKE2_SA_Timeout_Event, TQ_CLEAR_ALL_EXTRA,
                    (UNSIGNED)sa, 0);
                TQ_Timerunset(IKE2_INIT_AUTH_Timeout, TQ_CLEAR_ALL_EXTRA,
                    (UNSIGNED)sa, 0);

                /* Also remove conflicting events from the
                 * IKE event list.
                 */
                IKE_Unset_Matching_Events((UNSIGNED)sa, 0,
                    TQ_CLEAR_ALL_EXTRA);

                status = IKE2_Find_SADB_By_SA(sa, &sadb);

                /* Release the NET semaphore now. */
                if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    /* Failed to release semaphore. */
                    NLOG_Error_Log("Failed to release semaphore",
                        NERR_SEVERE, __FILE__, __LINE__);
                }

                if(status == NU_SUCCESS)
                {
                    /* Remove the IKE SA. This call will also free the
                     * exchange handle pointed to be "handle" because
                     * it is contained in the handle database.
                     */
                    if(IKE2_Delete_IKE_SA(sadb, sa) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to remove SA from SADB",
                                NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Failed to find SADB of the SA",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Free the Exchange Handle and all data contained
                     * within it. Use this explicit call only if the
                     * corresponding SA was not freed above.
                     */
                    if(IKE2_Cleanup_Exchange(handle) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Unable to clean-up Exchange Handle",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }
    }

    else
    {
        NLOG_Error_Log("The exchange for this SA has already been deleted",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return;
} /* IKE2_Exchange_Timeout_Event */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Unset_Matching_Timers
*
* DESCRIPTION
*
*       This function removes all registered IKEv2 timer
*       events which match the specified criteria, from
*       the Nucleus NET TQ component.
*
*       The caller is responsible for obtaining the IKE
*       and TCP semaphores before calling this function.
*
* INPUTS
*
*       dat                     Arbitrary event parameter.
*       ext_dat                 Arbitrary event parameter.
*       match_type              The matching type argument
*                               expected by the TQ_Timerunset
*                               function.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*
*************************************************************************/
STATUS IKE2_Unset_Matching_Timers(UNSIGNED dat, UNSIGNED ext_dat,
                                  INT16 match_type)
{
    STATUS          status;

    /* Remove matching message reply events. */
    status = TQ_Timerunset(IKE2_Message_Reply_Event, match_type, dat,
                           ext_dat);

    if(status == NU_SUCCESS)
    {
        /* Remove matching SA timeout events. */
        status = TQ_Timerunset(IKE2_SA_Timeout_Event, match_type, dat,
                               ext_dat);

        if(status == NU_SUCCESS)
        {
            /* Remove matching exchange timeout events. */
            status = TQ_Timerunset(IKE2_INIT_AUTH_Timeout, match_type,
                                   dat, ext_dat);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to unset exchange timeout events",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to unset SA timeout events",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to unset message reply events",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Unset_Matching_Timers */

#endif

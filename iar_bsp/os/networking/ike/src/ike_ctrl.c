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
*       ike_ctrl.c
*
* COMPONENT
*
*       IKE - Control
*
* DESCRIPTION
*
*       This file implements the IKE Control functions which are
*       responsible for carrying out external requests.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Initiate
*       IKE_Initiate_Phase1
*       IKE_Initiate_Phase2
*       IKE_Dispatch
*       IKE_Dispatch_Phase1
*       IKE_Dispatch_Phase2
*       IKE_Resume_Phase2
*       IKE_New_Phase1
*       IKE_New_Phase2
*       IKE_Select_Phase2_Security
*       IKE_Set_Phase2_Security
*       IKE_Send_Delete_Notification
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_cfg.h
*       ips_api.h
*       ike_api.h
*       ike_auth.h
*       ike_ips.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_cfg.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_auth.h"
#include "networking/ike_ips.h"

/* Local function prototypes. */
STATIC STATUS IKE_Initiate_Phase1(IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy,
                                  IKE_INITIATE_REQ *request,
                                  struct addr_struct *remote_addr,
                                  UINT32 *ret_msg_id);
STATIC STATUS IKE_Initiate_Phase2(IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy,
                                  IKE_INITIATE_REQ *request,
                                  IKE_SA *sa, UINT32 *ret_msg_id);
STATIC STATUS IKE_Dispatch_Phase1(IKE_PACKET *pkt, IKE_DEC_HDR *hdr,
                                  IKE_SA *sa, IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy);
STATIC STATUS IKE_Dispatch_Phase2(IKE_PACKET *pkt, IKE_DEC_HDR *hdr,
                                  IKE_SA *sa, IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy);
STATIC STATUS IKE_Resume_Phase2(IKE_SA *sa, IKE_STATE_PARAMS *params);
STATIC STATUS IKE_Set_Phase2_Security(IKE_PHASE2_HANDLE *phase2,
                                      IKE_INITIATE_REQ *request);

/*************************************************************************
*
* FUNCTION
*
*       IKE_Initiate
*
* DESCRIPTION
*
*       For version 1, this function initiates a Phase 1 or Phase 2
*       exchange. For version 2, when enabled, it initiates either an
*       IKE_SA_INIT or CREATE_CHILE_SA exchange. It is called when
*       IPsec needs to establish a new SA. Only the first message is
*       sent by this function, and the rest of the exchange is handled
*       by the state machine. Note that a successful IKE exchange
*       establishes IPsec SAs in both directions using the same policy,
*       even if the policy direction is outbound only.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *request                The IKE Initiate request.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         IPsec group was not found or
*                               there is no policy corresponding
*                               to the passed selector.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_INVALID_STATE       IKE daemon is not in running state.
*       IKE_INDEX_NOT_FOUND     Too many exchanges in progress
*                               so wait for completion.
*       IKE_NOT_FOUND           No matching policy found for
*                               the destination address.
*       IKE_UNALLOWED_XCHG      Exchange not allowed by IKE policy.
*       IKE_UNALLOWED_XCHG2     Exchange not allowed by IPsec policy.
*       Exchange Status         If the initiate request is
*                               blocking, the final status of the
*                               requested IKE exchange is returned.
*
*************************************************************************/
STATUS IKE_Initiate(IKE_INITIATE_REQ *request)
{
    STATUS              status;
    UINT32              msg_id;
    struct addr_struct  remote_addr;
    IKE_POLICY_SELECTOR select;
    IKE_SA              *sa;
    IKE_POLICY          *policy = NU_NULL;
    IKE_POLICY_GROUP    *group = NU_NULL;

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
    STATUS              xchg_status;
    INT                 xchg_index = 0;
    UNSIGNED            req_events = 0;
    UNSIGNED            ret_events;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the initiate request is not NULL. */
    if(request == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_FALSE)
    else if(request->ike_suspend != NU_NO_SUSPEND)
    {
        status = IKE_INVALID_PARAMS;
    }
#endif

    else
    {
        /* Make sure the IPsec selector is valid. */
        status = IPSEC_Validate_Selector(&request->ike_ips_select);
    }

    if(status == NU_SUCCESS)
    {
        IKE_DEBUG_LOG("Processing IKE initiate request");

        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* IKE daemon must be running. */
            if(IKE_Daemon_State != IKE_DAEMON_RUNNING)
            {
                status = IKE_INVALID_STATE;
            }

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
            /* If the caller is waiting for this exchange to complete. */
            if((status == NU_SUCCESS) &&
               (request->ike_suspend != NU_NO_SUSPEND))
            {
                /* If caller blocked without time constraint. */
                if(request->ike_suspend == NU_SUSPEND)
                {
                    /* Keep looking for a free exchange index
                     * until one is free.
                     */
                    do
                    {
                        /* Get a free index into the exchanges array. */
                        status = IKE_Get_Exchange_Index(IKE_BLANK_MSG_ID,
                                                        &xchg_index);

                        if(status != NU_SUCCESS)
                        {
                            /* Sleep for as long as a Phase 2 exchange
                             * is expected to take. Then there are more
                             * chances of finding a free index.
                             */
                            NU_Sleep(TICKS_PER_SECOND *
                                     IKE_PHASE2_TIMEOUT);
                        }
                    } while(status != NU_SUCCESS);
                }

                else
                {
                    /* Get a free index into the exchanges array. */
                    status = IKE_Get_Exchange_Index(IKE_BLANK_MSG_ID,
                                                    &xchg_index);
                }

                if(status == NU_SUCCESS)
                {
                    /* Set the requested event flag. */
                    req_events = (UNSIGNED)(1 << xchg_index);

                    /* Reset the event for this exchange index to
                     * handle the case when the previous caller's
                     * suspend timed out before it retrieved the
                     * event.
                     */
                    status = NU_Set_Events(&IKE_Data.ike_event_group,
                                 (IKE_WAIT_EVENT_MASK & (~req_events)),
                                 NU_AND);
                }

                else
                {
                    NLOG_Error_Log(
                        "Too many IKE blocking exchanges in progress",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
#endif

            /* If no error occurred. */
            if(status == NU_SUCCESS)
            {
                /* Get the IKE Group for this exchange. */
                status =
                    IKE_Get_Group_Entry_By_Device(request->ike_dev_index,
                                                  &group);

                if(status == NU_SUCCESS)
                {
                    /* Convert IPsec selector to IKE selector. */
                    IKE_IPS_Selector_To_IKE(&request->ike_ips_select,
                                            &select);

                    /* Get IKE Policy which applies to this exchange. */
                    status = IKE_Get_Policy_By_Selector(
                                 group->ike_group_name,
                                 &select, NU_NULL, &policy,
                                 IKE_MATCH_SELECTORS);

                    if(status == NU_SUCCESS)
                    {
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                        if(policy->ike_version == IKE_VERSION_1)
                        {
#endif
                            /* Make sure the IKE policy allows the
                            * requested Phase 2 exchange.
                            */
                            status = IKE_IPS_Phase2_Allowed(policy,
                                &request->ike_ips_select);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Phase 2 port/protocol not allowed",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                        }
#endif
                    }

                    else
                    {
                        NLOG_Error_Log("No matching IKE policy found",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Unable to get IKE group from device",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* If a valid IKE policy is found. */
            if(status == NU_SUCCESS)
            {
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
                /* If tunnel mode is being used. */
                if(request->ike_ips_security.ipsec_security_mode ==
                   IPSEC_TUNNEL_MODE)
                {
                    /* Convert IPsec security to IKE address. */
                    IKE_IPS_Security_To_Remote_IP(
                        &request->ike_ips_security,
                        &remote_addr);
                }

                else
#endif
                {
                    /* Convert IPsec selector to IKE address. */
                    IKE_IPS_Selector_To_Remote_IP(&request->ike_ips_select,
                                                  &remote_addr);
                }

                /* Search for a Phase 1 IKE SA. */
                status = IKE_Get_SA(&policy->ike_sa_list, &remote_addr,
                                    IKE_MATCH_SA_IP, &sa);

                /* If a Phase 1 SA was not found. */
                if(status != NU_SUCCESS)
                {
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                    if(policy->ike_version == IKE_VERSION_1)
                    {
#endif
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                        /* Set the current exchange status. */
                        xchg_status = IKE_PHASE1_INCOMPLETE;
#endif

                        /* Initiate a Phase 1 exchange. */
                        status = IKE_Initiate_Phase1(group, policy,
                            request, &remote_addr, &msg_id);
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                    }

                    else if(policy->ike_version == IKE_VERSION_2)
                    {
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                        /* Set the current exchange status. */
                        xchg_status = IKE2_SA_INIT_INCOMPLETE;
#endif

                        status = IKE2_Initiate_IKE2_Exchange(group, policy,
                            request, &remote_addr, &msg_id);
                    }
#endif

                }

                else
                {
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                    if(policy->ike_version == IKE_VERSION_1)
                    {
#endif
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                        /* Set the current exchange status. */
                        xchg_status = IKE_PHASE2_INCOMPLETE;
#endif

                        /* Initiate a Phase 2 exchange. */
                        status = IKE_Initiate_Phase2(group, policy,
                            request, sa, &msg_id);
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                    }

                    else if(policy->ike_version == IKE_VERSION_2)
                    {
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                        /* Set the current exchange status. */
                        xchg_status = IKE2_CREATE_CHILD_SA_INCOMPLETE;
#endif

                        status = IKE2_Initiate_IKE2_Child_Exchange(group,
                            policy, request, sa, &msg_id);
                    }
#endif

                }

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                /* If no error occurred and if the caller is waiting. */
                if((status == NU_SUCCESS) &&
                   (request->ike_suspend != NU_NO_SUSPEND))
                {
                    /* Set the message ID of the exchange. */
                    IKE_Data.ike_msg_ids[xchg_index] = msg_id;

                    /* Set the current status of the exchange. */
                    IKE_Data.ike_status[xchg_index] = xchg_status;
                }
#endif
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

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
    /* If no error occurred and if the caller is waiting. */
    if((status == NU_SUCCESS) && (request->ike_suspend != NU_NO_SUSPEND))
    {
        IKE_DEBUG_LOG("Suspending caller due to blocking call");

        /* Wait for the exchange to complete. */
        status = NU_Retrieve_Events(&IKE_Data.ike_event_group,
                                    req_events, NU_OR_CONSUME,
                                    &ret_events, request->ike_suspend);

        if(status == NU_SUCCESS)
        {
            /* Get the status of the exchange. It is safe to access
             * this array without obtaining the semaphore because
             * this thread is itself responsible for removing its
             * entry from the array.
             */
            status = IKE_Data.ike_status[xchg_index];
        }

        else
        {
            /* Report the error. */
            NLOG_Error_Log(
                "Failed to wait for Phase 2 exchange completion",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Remove exchange item from array. */
        IKE_Data.ike_msg_ids[xchg_index] = IKE_BLANK_MSG_ID;
    }
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status. */
    return (status);

} /* IKE_Initiate */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Initiate_Phase1
*
* DESCRIPTION
*
*       This function is called by IKE_Initiate if a Phase 1
*       SA is not found for a negotiation. It initiates a
*       Phase 1 exchange and sends the first message of the
*       exchange to the remote node.
*
* INPUTS
*
*       *group                  IKE Group to be used.
*       *policy                 Policy which applies to the
*                               Phase 1 exchange.
*       *request                The initiate request.
*       *remote_addr            Pointer to remote address.
*       *ret_msg_id             On return, this contains the
*                               Phase 2 exchange's message ID.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         IPsec group or policy not found.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_UNALLOWED_XCHG2     Exchange not allowed by IPsec
*                               policy because security protocol
*                               does not match.
*       IKE_INVALID_XCHG_TYPE   Mode of exchange is invalid.
*       Exchange Status         Status of Phase 1 state machine
*                               is returned.
*
*************************************************************************/
STATIC STATUS IKE_Initiate_Phase1(IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy,
                                  IKE_INITIATE_REQ *request,
                                  struct addr_struct *remote_addr,
                                  UINT32 *ret_msg_id)
{
    STATUS              status;
    IKE_STATE_PARAMS    params;
    IKE_SA              *sa_ptr = NU_NULL;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((group   == NU_NULL) || (policy      == NU_NULL) ||
       (request == NU_NULL) || (remote_addr == NU_NULL) ||
       (ret_msg_id == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Phase 1 SA not found - initiating Phase 1");

    /* Initialize state machine parameters to zero. */
    UTL_Zero(&params, sizeof(IKE_STATE_PARAMS));

    /* Set available state machine parameters. */
    params.ike_group  = group;
    params.ike_policy = policy;

    /* Generate a new Phase 1 Handle and SA (being negotiated). */
    status = IKE_New_Phase1(&IKE_Large_Data.ike_phase1,
                            &IKE_Large_Data.ike_sa, remote_addr, &params);

    if(status == NU_SUCCESS)
    {
        /* Add the SA to the policy's SADB. This SA's state will
         * be updated in the SADB as the exchange proceeds.
         */
        status = IKE_Add_SA(&policy->ike_sa_list,
                            &IKE_Large_Data.ike_sa, &sa_ptr);

        if(status == NU_SUCCESS)
        {
            /* Generate Phase 2 Handle. */
            status = IKE_New_Phase2(&IKE_Large_Data.ike_phase2,
                                    sa_ptr, &params);

            if(status == NU_SUCCESS)
            {
                /* Set Message ID to be returned to the caller. */
                *ret_msg_id = IKE_Large_Data.ike_phase2.ike_msg_id;

                /* Add security protocols to the Phase 2 Handle. */
                status = IKE_Set_Phase2_Security(
                             &IKE_Large_Data.ike_phase2, request);

                if(status == NU_SUCCESS)
                {
                    /* Add Phase 2 Handle to the database. */
                    status = IKE_Add_Phase2(&sa_ptr->ike_phase2_db,
                                            &IKE_Large_Data.ike_phase2,
                                            NU_NULL);
                }

                else
                {
                    NLOG_Error_Log("Unable to set IPsec security",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to create local Phase 2 Handle",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to add SA to IKE SADB",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to create local Phase 1 Handle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* If no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Determine the IKE exchange mode. */
        switch(sa_ptr->ike_phase1->ike_xchg_mode)
        {
#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)
        case IKE_XCHG_MAIN:
            /* Dispatch message to Main Mode state machine. */
            status = IKE_Process_Main_Mode(sa_ptr->ike_phase1);
            break;
#endif

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
        case IKE_XCHG_AGGR:
            /* Call the Aggressive mode handler. */
            status = IKE_Process_Aggr_Mode(sa_ptr->ike_phase1);
            break;
#endif

        default:
            /* Unsupported ISAKMP mode of exchange. */
            status = IKE_INVALID_XCHG_TYPE;
            break;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Initiate_Phase1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Initiate_Phase2
*
* DESCRIPTION
*
*       This function is called by IKE_Initiate if a Phase 1
*       SA is found. If the Phase 1 SA is established, it
*       initiates a Phase 2 exchange and sends the first
*       message of the exchange to the remote node. If the
*       Phase 1 SA is currently being negotiated, it queues
*       the Phase 2 exchange - Phase 2 negotiation would be
*       resumed after the SA has been established.
*
* INPUTS
*
*       *group                  IKE Group to be used.
*       *policy                 Policy which applies to the
*                               Phase 2 exchange.
*       *request                The initiate request.
*       *sa                     Pointer to the Phase 1 SA.
*       *ret_msg_id             On return, this contains the
*                               Phase 2 exchange's message ID.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         IPsec group or policy not found.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_UNALLOWED_XCHG2     Exchange not allowed by IPsec
*                               policy because security protocol
*                               does not match.
*       Exchange Status         Status of Phase 2 state machine
*                               is returned.
*
*************************************************************************/
STATIC STATUS IKE_Initiate_Phase2(IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy,
                                  IKE_INITIATE_REQ *request,
                                  IKE_SA *sa, UINT32 *ret_msg_id)
{
    STATUS              status;
    IKE_STATE_PARAMS    params;
    IKE_PHASE2_HANDLE   *phase2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((group      == NU_NULL) || (policy == NU_NULL) ||
       (request    == NU_NULL) || (sa     == NU_NULL) ||
       (ret_msg_id == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Phase 1 SA found - initiating Phase 2");

    /* Initialize state machine parameters to zero. */
    UTL_Zero(&params, sizeof(IKE_STATE_PARAMS));

    /* Set available state machine parameters. */
    params.ike_group  = group;
    params.ike_policy = policy;

    /* Generate Phase 2 Handle. */
    status = IKE_New_Phase2(&IKE_Large_Data.ike_phase2, sa, &params);

    if(status == NU_SUCCESS)
    {
        /* Set Message ID to be returned to the caller. */
        *ret_msg_id = IKE_Large_Data.ike_phase2.ike_msg_id;

        /* Add security protocols to the Phase 2 Handle. */
        status = IKE_Set_Phase2_Security(&IKE_Large_Data.ike_phase2,
                                         request);

        if(status == NU_SUCCESS)
        {
            /* Add Phase 2 Handle to the database in the SA. */
            status = IKE_Add_Phase2(&sa->ike_phase2_db,
                                    &IKE_Large_Data.ike_phase2, &phase2);

            /* If the Handle was added to IKE SA and if the
             * IKE SA has already been established.
             */
            if((status == NU_SUCCESS) &&
               (sa->ike_state == IKE_SA_ESTABLISHED))
            {
                /* Initiate a Phase 2 exchange. Note that if
                 * the above condition fails and the IKE SA
                 * is currently being established, then this
                 * Phase 2 exchange would be initiated
                 * by the dispatch function after the IKE SA
                 * negotiation is complete.
                 */
                status = IKE_Process_Quick_Mode(phase2);
            }

            else
            {
                NLOG_Error_Log("Failed to add Phase 2 Handle to database",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to set IPsec security in Handle",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to create local Phase 2 Handle",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Initiate_Phase2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Dispatch
*
* DESCRIPTION
*
*       This function is responsible for handling incoming packets
*       and dispatching them to the appropriate exchange mode state
*       machine.
*
* INPUTS
*
*       *pkt                    Packet received from network.
*       *group                  Group to which the receiving
*                               interface belongs to.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           No matching policy was found for
*                               the sender's address.
*       IKE_UNALLOWED_MODE      Exchange mode of ISAKMP is not
*                               allowed by the IKE policy.
*       IKE_LENGTH_IS_SHORT     Not enough data in packet.
*       IKE_UNEQUAL_PLOAD_LEN   Length in header and actual length
*                               do not match.
*       IKE_INVALID_COOKIE      Cookie is not valid.
*       IKE_INVALID_PLOAD_TYPE  Invalid payload type in message.
*       IKE_INVALID_MJR_VER     Major version not supported.
*       IKE_INVALID_MNR_VER     Minor version not supported.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_SA_NOT_FOUND        No SA found to decrypt message.
*       IKE_INVALID_XCHG_TYPE   Mode of exchange is invalid.
*       IKE_ADDR_MISMATCH       Remote address does not match
*                               the one specified in the SA.
*       Exchange Status         If the state machine was called,
*                               status of the current state
*                               processing is returned.
*
*************************************************************************/
STATUS IKE_Dispatch(IKE_PACKET *pkt, IKE_POLICY_GROUP *group)
{
    STATUS              status;
    IKE_SA              *sa = NU_NULL;
    IKE_POLICY          *policy = NU_NULL;
    IKE_DEC_HDR         hdr;
    IKE_POLICY_SELECTOR select;

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
    UINT8               version = 0;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the pointers are not NULL. */
    if((pkt == NU_NULL) || (group == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set type of the selector. */
    select.ike_type = IKE_FAMILY_TO_FLAGS(pkt->ike_remote_addr.family);

    /* Initialize selector IP to sender's IP address. */
    NU_BLOCK_COPY(select.ike_addr.ike_ip.ike_addr1,
                  pkt->ike_remote_addr.id.is_ip_addrs,
                  MAX_ADDRESS_SIZE);

    /* Get IKE Policy which applies to this exchange. */
    status = IKE_Get_Policy_By_Selector(group->ike_group_name,
                                        &select, NU_NULL, &policy,
                                        IKE_MATCH_SELECTORS);

    /* If a valid policy was found. */
    if(status == NU_SUCCESS)
    {
        IKE_DEBUG_LOG("IKE policy found for incoming message");

#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
        /* IKEv2 is enabled. Check which version this packet has been
         * sent under.
         */
        if(pkt->ike_data_len > IKE_HDR_LEN)
        {
            version = GET8(pkt->ike_data, IKE_HDR_VERSION_OFFSET);
            version = version >> IKE_MAJOR_VERSION_SHL;
        }
        if(version == IKE_VERSION_2)
        {
            IKE2_Dispatch(pkt, policy);
        }

        else if (version == IKE_VERSION_1)
        {
#endif
            /* Decode the ISAKMP header. */
            status = IKE_Decode_Header(pkt->ike_data, pkt->ike_data_len,
                &hdr);

            if(status == NU_SUCCESS)
            {
                /* Check if the length of the message is valid. */
                if(hdr.ike_length > (UINT32)pkt->ike_data_len)
                {
                    NLOG_Error_Log("Invalid IKE packet length",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_UNEQUAL_PLOAD_LEN;
                }

                /* Make sure the Initiator cookie is set. */
                else if(!IKE_COOKIE_IS_SET(hdr.ike_icookie))
                {
                    NLOG_Error_Log("Invalid cookie in ISAKMP header",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_COOKIE;
                }

                /* Make sure first payload type is valid. */
                else if(!IKE_VALID_FIRST_PAYLOAD(hdr.ike_first_payload))
                {
                    NLOG_Error_Log("IKE message contains invalid payload",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_PLOAD_TYPE;
                }

                /* Make sure ISAKMP major version is supported. */
                else if((hdr.ike_version >> IKE_MAJOR_VERSION_SHL) >
                    IKE_MAJOR_VERSION)
                {
                    NLOG_Error_Log("IKE major version incompatible",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_MJR_VER;
                }

                /* Make sure ISAKMP minor version is supported. */
                else if(((hdr.ike_version >> IKE_MAJOR_VERSION_SHL) ==
                    IKE_MAJOR_VERSION) &&
                    ((hdr.ike_version & IKE_MINOR_VERSION_MASK) >
                    IKE_MINOR_VERSION))
                {
                    NLOG_Error_Log("IKE minor version incompatible",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_MNR_VER;
                }

                /* Unused flag bits MUST be zero. */
                else if((hdr.ike_flags & (UINT8)(~(IKE_HDR_ENC_MASK  |
                    IKE_HDR_AUTH_MASK |
                    IKE_HDR_COMMIT_MASK))) != 0)
                {
                    NLOG_Error_Log("Invalid flag set in ISAKMP header",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_FLAGS;
                }

                /* Message ID must be non-zero if commit bit is set. */
                else if(((hdr.ike_flags & IKE_HDR_COMMIT_MASK) != 0) &&
                    (hdr.ike_msg_id == 0))
                {
                    NLOG_Error_Log("Commit flag in Phase 1 not supported",
                        NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_FLAGS;
                }

                else
                {
                    /* Search for an SA using the Initiator cookie. */
                    if(IKE_Get_SA(&policy->ike_sa_list,
                        hdr.ike_icookie,
                        IKE_MATCH_SA_PARTIAL_COOKIE,
                        &sa) != NU_SUCCESS)
                    {
                        /* Continue normal execution if an SA is not
                        * found. This might be a new negotiation.
                        * However, make sure that the packet is
                        * not encrypted because an SA is required
                        * for decrypting it.
                        */
                        if((hdr.ike_flags & IKE_HDR_ENC_MASK) != 0)
                        {
                            NLOG_Error_Log("Encryption used but SA not found",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

                            status = IKE_SA_NOT_FOUND;
                        }
                    }

                    else
                    {
                        /* Make sure packet was received from the same
                         * remote node with which the SA is established.
                         * This check must be performed to avoid replay
                         * attacks.
                         */
                        if(memcmp(sa->ike_node_addr.id.is_ip_addrs,
                            pkt->ike_remote_addr.id.is_ip_addrs,
                            IKE_IP_LEN(pkt->ike_remote_addr.family))
                            != 0)
                        {
                            NLOG_Error_Log("Remote address mismatch",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

                            status = IKE_ADDR_MISMATCH;
                        }
                    }
                }

                if(status == NU_SUCCESS)
                {
                    /* If this is a Phase 1 exchange. */
                    if(IKE_IS_PHASE2_MODE(hdr.ike_exchange_type) == NU_FALSE)
                    {
                        /* Dispatch Phase 1 message. */
                        status = IKE_Dispatch_Phase1(pkt, &hdr, sa,
                            group, policy);
                    }

                    /* Otherwise this is either a Phase 2 message, or an
                     * Informational exchange message. Informational
                     * exchange is handled similar to a Phase 2 exchange.
                     */
                    else
                    {
                        /* Dispatch Phase 2 message. */
                        status = IKE_Dispatch_Phase2(pkt, &hdr, sa,
                            group, policy);
                    }
                }

                else
                {
                    IKE_DEBUG_LOG(
                        "Error in incoming message - message ignored");

                    /* If an SA is looked-up before the error checking,
                     * then an Informational message could be sent here.
                     * This has been disabled here due to risk of a DoS
                     * attack.
                     */
                }
            }
            else
            {
                NLOG_Error_Log("Unable to decode IKE message header",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
#if(IKE_INCLUDE_VERSION_2 == NU_TRUE)
        }
        else
        {
            NLOG_Error_Log("IKE version not recognized",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
#endif

    }

    /* If no valid policy found. */
    else
    {
        NLOG_Error_Log("No IKE policy found for incoming message",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Dispatch */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Dispatch_Phase1
*
* DESCRIPTION
*
*       This function is called by IKE_Dispatch if a Phase 1
*       message is received. It forwards the message to the
*       IKE state machine.
*
* INPUTS
*
*       *pkt                    Incoming raw packet.
*       *hdr                    ISAKMP Header of the message.
*       *sa                     IKE SA of the exchange. This
*                               can also be NULL.
*       *group                  Pointer to the IKE group.
*       *policy                 Pointer to the IKE policy.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         IPsec group or policy not found.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Valid handle not found.
*       IKE_UNALLOWED_MODE      Exchange mode not allowed.
*       IKE_INVALID_XCHG_TYPE   Exchange mode type is invalid.
*       Exchange Status         Status of Phase 1 state machine
*                               is returned.
*
*************************************************************************/
STATIC STATUS IKE_Dispatch_Phase1(IKE_PACKET *pkt, IKE_DEC_HDR *hdr,
                                  IKE_SA *sa, IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy)
{
    STATUS              status = NU_SUCCESS;
    IKE_PHASE1_HANDLE   *phase1 = NU_NULL;
    IKE_STATE_PARAMS    params;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((pkt   == NU_NULL) || (hdr    == NU_NULL) ||
       (group == NU_NULL) || (policy == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Dispatching Phase 1 message");

    /* Initialize state machine parameters to zero. */
    UTL_Zero(&params, sizeof(IKE_STATE_PARAMS));

    /* Set available state machine parameters. */
    params.ike_group      = group;
    params.ike_policy     = policy;
    params.ike_packet     = pkt;
    params.ike_in.ike_hdr = hdr;

    /* If incoming message is the first message of an exchange. */
    if(sa == NU_NULL)
    {
        /* IKE SA and Phase 1 Handle were not found.
         * Create temporary ones locally. These would be
         * added to the database by the state machine.
         */
        sa     = &IKE_Large_Data.ike_sa;
        phase1 = &IKE_Large_Data.ike_phase1;

        /* Set fields of the Handle for a new exchange. */
        status = IKE_New_Phase1(phase1, sa, &pkt->ike_remote_addr,
                                &params);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to create local Phase 1 Handle",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* If SA state set to established. */
    else if(sa->ike_state == IKE_SA_ESTABLISHED)
    {
        /* SA seems to be established already. */
        status = IKE_INVALID_PARAMS;
    }

    /* If the Handle is valid. */
    else if(sa->ike_phase1 != NU_NULL)
    {
        /* Make sure this Handle has not been queued for deletion. */
        if((sa->ike_phase1->ike_flags & IKE_DELETE_FLAG) != 0)
        {
            status = IKE_NOT_FOUND;
        }

        else
        {
            /* Set Phase 1 Handle pointer from the SA. */
            phase1 = sa->ike_phase1;

            /* Update state parameters in the Handle. */
            phase1->ike_params = &params;
        }
    }

    else
    {
        /* SA seems to be established already. No handle found. */
        status = IKE_INVALID_PARAMS;
    }

    if(status == NU_SUCCESS)
    {
        /* Determine exchange mode. */
        switch(phase1->ike_xchg_mode)
        {
#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)
        case IKE_XCHG_MAIN:
            /* Check if policy allows this exchange mode. */
            if((policy->ike_phase1_xchg & IKE_XCHG_MAIN_FLAG) != 0)
            {
                /* Dispatch message to Main Mode state machine. */
                status = IKE_Process_Main_Mode(phase1);

                if(status == NU_SUCCESS)
                {
                    /* If the Phase 1 SA has been established. */
                    if(phase1->ike_xchg_state == IKE_COMPLETE_STATE)
                    {
                        /* Log debug message. */
                        IKE_DEBUG_LOG("Main mode exchange completed");

                        /* Finalize the Handle and IKE SA. */
                        status = IKE_Finalize_Phase1(phase1);

                        if(status == NU_SUCCESS)
                        {
#if ((IKE_INCLUDE_INFO_MODE == NU_TRUE) && \
     (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE))
                            /* Send an INITIAL-CONTACT if this is the
                             * first SA established with the remote
                             * node.
                             */
                            if(IKE_Check_Initial_Contact(phase1) ==
                               NU_SUCCESS)
                            {
                                IKE_DEBUG_LOG("INITIAL-CONTACT sent");
                            }
#endif

                            /* Initiate Phase 2 exchanges if
                             * any are pending.
                             */
                            status = IKE_Resume_Phase2(sa, &params);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to resume Phase 2 exchanges",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to finalize Phase 1",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
                }

                else
                {
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
                    /* If Informational exchange is allowed. */
                    if((policy->ike_phase1_xchg & IKE_XCHG_INFO_FLAG) != 0)
                    {
                        /* Send a notification if possible. */
                        if(IKE_Notify_Error(phase1->ike_sa, status) ==
                           NU_SUCCESS)
                        {
                            IKE_DEBUG_LOG("Error notification sent");
                        }
                    }
#endif
                }
            }

            else
            {
                NLOG_Error_Log("Main Mode not allowed by policy",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_UNALLOWED_MODE;
            }
            break;
#endif /* (IKE_INCLUDE_MAIN_MODE == NU_TRUE) */

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
        case IKE_XCHG_AGGR:
            /* Check if policy allows this exchange mode. */
            if((policy->ike_phase1_xchg & IKE_XCHG_AGGR_FLAG) != 0)
            {
                /* Dispatch message to Aggressive Mode state machine. */
                status = IKE_Process_Aggr_Mode(phase1);

                if(status == NU_SUCCESS)
                {
                    /* If the Phase 1 SA has been established. */
                    if(phase1->ike_xchg_state == IKE_COMPLETE_STATE)
                    {
                        /* Log debug message. */
                        IKE_DEBUG_LOG("Aggr. mode exchange completed");

                        /* Finalize the Handle and IKE SA. Return
                         * value of this function can safely be
                         * ignored.
                         */
                        status = IKE_Finalize_Phase1(phase1);

                        if(status == NU_SUCCESS)
                        {
#if ((IKE_INCLUDE_INFO_MODE == NU_TRUE) && \
     (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE))
                            /* Send an INITIAL-CONTACT if this is the
                             * first SA established with the remote
                             * node.
                             */
                            if(IKE_Check_Initial_Contact(phase1) ==
                               NU_SUCCESS)
                            {
                                IKE_DEBUG_LOG("INITIAL-CONTACT sent");
                            }
#endif

                            /* Initiate Phase 2 exchanges if
                             * any are pending.
                             */
                            status = IKE_Resume_Phase2(sa, &params);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to resume Phase 2 exchanges",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                    }
                }

                else
                {
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
                    /* If Informational exchange is allowed. */
                    if((policy->ike_phase1_xchg & IKE_XCHG_INFO_FLAG) != 0)
                    {
                        /* Send a notification if possible. */
                        if(IKE_Notify_Error(phase1->ike_sa, status) ==
                           NU_SUCCESS)
                        {
                            IKE_DEBUG_LOG("Error notification sent");
                        }
                    }
#endif
                }
            }

            else
            {
                NLOG_Error_Log("Aggressive Mode not allowed by policy",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_UNALLOWED_MODE;
            }
            break;
#endif /* (IKE_INCLUDE_AGGR_MODE == NU_TRUE) */

        default:
            /* Unsupported exchange mode specified. */
            NLOG_Error_Log("Unsupported ISAKMP mode of exchange",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            status = IKE_INVALID_XCHG_TYPE;
            break;
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Dispatch_Phase1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Dispatch_Phase2
*
* DESCRIPTION
*
*       This function is called by IKE_Dispatch if a Phase 2
*       message is received. It forwards the message to the
*       IKE state machine.
*
* INPUTS
*
*       *pkt                    Incoming raw packet.
*       *hdr                    ISAKMP Header of the message.
*       *sa                     IKE SA of the exchange. This
*                               can also be NULL.
*       *group                  Pointer to the IKE group.
*       *policy                 Pointer to the IKE policy.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         IPsec group or policy not found.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_INVALID_COOKIE      Cookie is invalid.
*       IKE_NOT_FOUND           No valid Handle found.
*       IKE_MISSING_PAYLOAD     A required payload is missing.
*       IKE_UNALLOWED_MODE      Exchange mode not allowed.
*       IKE_INVALID_XCHG_TYPE   Exchange mode type is invalid.
*       Exchange Status         Status of Phase 2 state machine
*                               is returned.
*
*************************************************************************/
STATIC STATUS IKE_Dispatch_Phase2(IKE_PACKET *pkt, IKE_DEC_HDR *hdr,
                                  IKE_SA *sa, IKE_POLICY_GROUP *group,
                                  IKE_POLICY *policy)
{
    STATUS              status;
    IKE_STATE_PARAMS    params;
    IKE_PHASE2_HANDLE   *phase2;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((pkt   == NU_NULL) || (hdr    == NU_NULL) ||
       (group == NU_NULL) || (policy == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Dispatching Phase 2 message");

    /* Make sure an SA is present. */
    if(sa == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Initialize state machine parameters to zero. */
        UTL_Zero(&params, sizeof(IKE_STATE_PARAMS));

        /* Set available state machine parameters. */
        params.ike_group      = group;
        params.ike_policy     = policy;
        params.ike_packet     = pkt;
        params.ike_in.ike_hdr = hdr;

        /* Search for the Phase 2 Handle using the Message ID. */
        status = IKE_Get_Phase2(&sa->ike_phase2_db, hdr->ike_msg_id,
                                &phase2);

        if(status != NU_SUCCESS)
        {
            /* If a Phase 2 Handle was not found, create a
             * temporary one locally. This would be added
             * to the database by the state machine.
             */
            phase2 = &IKE_Large_Data.ike_phase2;

            /* Set fields of the Handle for a new exchange. */
            status = IKE_New_Phase2(phase2, sa, &params);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to create local Phase 2 Handle",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* If the matching Handle has been queued for deletion. */
        else if((phase2->ike_flags & IKE_DELETE_FLAG) != 0)
        {
            status = IKE_NOT_FOUND;
        }

        else
        {
            /* Update state parameters in the Handle. */
            phase2->ike_params = &params;
        }

        if(status == NU_SUCCESS)
        {
            /* Determine exchange mode. */
            switch(hdr->ike_exchange_type)
            {
            case IKE_XCHG_QUICK:
                /* Check if policy allows this exchange mode. */
                if((policy->ike_phase2_xchg & IKE_XCHG_QUICK_FLAG) != 0)
                {
                    /* Dispatch message to Quick Mode state machine. */
                    status = IKE_Process_Quick_Mode(phase2);

                    if(status == NU_SUCCESS)
                    {
                        /* If the Phase 2 exchange is complete. */
                        if(phase2->ike_xchg_state == IKE_COMPLETE_STATE)
                        {
                            /* Log debug message. */
                            IKE_DEBUG_LOG("Quick mode exchange completed");

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
                            /* If any process is waiting for this exchange,
                             * then resume it with status success. This
                             * function returns an error if no process is
                             * waiting.
                             */
                            if(IKE_Resume_Waiting_Process(phase2,
                               NU_SUCCESS) == NU_SUCCESS)
                            {
                                IKE_DEBUG_LOG("Waiting processes resumed");
                            }
#endif

                            /* Finalize the Handle. */
                            status = IKE_Finalize_Phase2(phase2);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to finalize Phase 2",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                    }

                    else
                    {
#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
                        /* If informational exchange is allowed. */
                        if((policy->ike_phase2_xchg & IKE_XCHG_INFO_FLAG)
                           != 0)
                        {
                            /* Send a notification if possible. */
                            if(IKE_Notify_Error(phase2->ike_sa, status) ==
                               NU_SUCCESS)
                            {
                                IKE_DEBUG_LOG("Error notification sent");
                            }
                        }
#endif
                    }
                }

                else
                {
                    NLOG_Error_Log("Quick Mode not allowed by policy",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_UNALLOWED_MODE;
                }
                break;

#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
            case IKE_XCHG_INFO:
                /* Check if policy allows this exchange mode. */
                if(((phase2->ike_msg_id == 0) &&
                    ((policy->ike_phase1_xchg & IKE_XCHG_INFO_FLAG) != 0))
                   ||
                   ((phase2->ike_msg_id != 0) &&
                    ((policy->ike_phase2_xchg & IKE_XCHG_INFO_FLAG) != 0)))
                {
                    /* Dispatch message to Info. Mode state machine. */
                    status = IKE_Process_Info_Mode(phase2);
                }

                else
                {
                    NLOG_Error_Log("Info. Mode not allowed by policy",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_UNALLOWED_MODE;
                }
                break;
#endif /* (IKE_INCLUDE_INFO_MODE == NU_TRUE) */

            default:
                /* Unsupported exchange mode specified. */
                NLOG_Error_Log("Unsupported ISAKMP mode of exchange",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                status = IKE_INVALID_XCHG_TYPE;
                break;
            }
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Dispatch_Phase2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Resume_Phase2
*
* DESCRIPTION
*
*       This function resumes Phase 2 exchanges for IPsec
*       outbound SAs, if any are pending under the specified
*       Phase 1 SA. This function must only be called for
*       newly established IKE SAs.
*
*
* INPUTS
*
*       *sa                     Phase 1 IKE SA.
*       *params                 State parameters of the last
*                               Phase 1 message.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
*************************************************************************/
STATIC STATUS IKE_Resume_Phase2(IKE_SA *sa, IKE_STATE_PARAMS *params)
{
    IKE_PHASE2_HANDLE   *phase2;

#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
    INT                 xchg_index;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((sa == NU_NULL) || (params == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Initiating Phase 2 exchange(s)");

    /* Re-initialize the packet pointer in the state parameters. */
    params->ike_packet = NU_NULL;

    /* Re-initialize the payload pointers in the state parameters. */
    UTL_Zero(&params->ike_in, sizeof(IKE_DEC_MESSAGE));
    UTL_Zero(&params->ike_out, sizeof(IKE_ENC_MESSAGE));

    /* Set pointer to the first item in the database. */
    phase2 = sa->ike_phase2_db.ike_flink;

    /* Process all Handles in the database. */
    while(phase2 != NU_NULL)
    {
#if (IKE_ENABLE_BLOCKING_REQUEST == NU_TRUE)
        /* Get the exchange index. This would only be found
         * if the caller is waiting for this exchange and
         * otherwise this call would fail.
         */
        if(IKE_Get_Exchange_Index(phase2->ike_msg_id, &xchg_index) ==
           NU_SUCCESS)
        {
            /* Update the exchange status in the array. */
            IKE_Data.ike_status[xchg_index] = IKE_PHASE2_INCOMPLETE;
        }
#endif

        /* Use the same state parameters which were used in the
         * last message of the Phase 1 Exchange, processed right
         * before this function was called. Note that it is NOT
         * safe to access these from the Phase 1 Handle pointer
         * in the SA since the Phase 1 Handle might have been
         * deallocated by now.
         */
        phase2->ike_params = params;

        /* Initiate the Phase 2 exchange. Even if this call fails,
         * continue processing all pending Phase 2 Handles.
         */
        if(IKE_Process_Quick_Mode(phase2) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to resume Phase 2 exchange",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Move to the next item in the database. */
        phase2 = phase2->ike_flink;
    }

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Resume_Phase2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_New_Phase1
*
* DESCRIPTION
*
*       This is a utility function used to generate the
*       Phase 1 Handle when a new Phase 1 negotiation is being
*       started. It is called by both the Initiator and the
*       Responder.
*
* INPUTS
*
*       *phase1                 On return, this contains the
*                               initialized Phase 1 Handle.
*       *sa                     On return, this contains the
*                               initialized IKE SA.
*       *remote_addr            Address of remote node with
*                               which to perform exchange.
*       *params                 State parameters to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
*************************************************************************/
STATUS IKE_New_Phase1(IKE_PHASE1_HANDLE *phase1, IKE_SA *sa,
                      struct addr_struct *remote_addr,
                      IKE_STATE_PARAMS *params)
{
    STATUS          status;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((phase1      == NU_NULL) || (sa     == NU_NULL) ||
       (remote_addr == NU_NULL) || (params == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Generating new Phase 1 Handle and SA");

    /* Set all fields of the Handle and SA to zero. */
    UTL_Zero(phase1, sizeof(IKE_PHASE1_HANDLE));
    UTL_Zero(sa, sizeof(IKE_SA));

    /* Set pointer to the IKE SA. */
    phase1->ike_sa = sa;

    /* Set pointer to state parameters. */
    phase1->ike_params = params;

    /* Set the remote node's address in the SA. */
    NU_BLOCK_COPY(&sa->ike_node_addr, remote_addr,
                  sizeof(struct addr_struct));

    /* Set the current SA state. */
    sa->ike_state = IKE_SA_INCOMPLETE;

    /* Set pointer to the Phase 1 Handle in the SA. */
    sa->ike_phase1 = phase1;

    /* If we are the Initiator. */
    if(params->ike_packet == NU_NULL)
    {
        /* Set Initiator flag. */
        phase1->ike_flags = IKE_INITIATOR;

        /* Set the initial exchange state. */
        phase1->ike_xchg_state = IKE_INITIATOR_START_STATE;

        /* If more than one exchange mode allowed by policy. */
        if(((params->ike_policy->ike_phase1_xchg & IKE_XCHG_MAIN_FLAG)
            != 0) &&
           ((params->ike_policy->ike_phase1_xchg & IKE_XCHG_AGGR_FLAG)
            != 0))
        {
            /* Set exchange mode to default. */
            phase1->ike_xchg_mode = IKE_PHASE1_DEFAULT_XCHG;
        }

        else if((params->ike_policy->ike_phase1_xchg & IKE_XCHG_AGGR_FLAG)
                != 0)
        {
            /* Set exchange mode to Aggressive mode. */
            phase1->ike_xchg_mode = IKE_XCHG_AGGR;
        }

        else if((params->ike_policy->ike_phase1_xchg & IKE_XCHG_MAIN_FLAG)
                != 0)
        {
            /* Set exchange mode to Main mode. */
            phase1->ike_xchg_mode = IKE_XCHG_MAIN;
        }

        /* Generate the Initiator cookie. */
        status = IKE_Generate_Cookie(sa->ike_cookies, &sa->ike_node_addr);
    }

    else
    {
        /* Set Responder flag. */
        phase1->ike_flags = IKE_RESPONDER;

        /* Set the initial exchange state. */
        phase1->ike_xchg_state = IKE_RESPONDER_START_STATE;

        /* Set exchange mode to the one used by the Initiator. */
        phase1->ike_xchg_mode = params->ike_in.ike_hdr->ike_exchange_type;

        /* Copy Initiator cookie from the header. */
        NU_BLOCK_COPY(sa->ike_cookies, params->ike_in.ike_hdr->ike_icookie,
                      IKE_COOKIE_LEN);

        /* Generate the Responder cookie. */
        status = IKE_Generate_Cookie(&sa->ike_cookies[IKE_COOKIE_LEN],
                                     &sa->ike_node_addr);
    }

    /* Return the status. */
    return (status);

} /* IKE_New_Phase1 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_New_Phase2
*
* DESCRIPTION
*
*       This is a utility function used to generate the
*       Phase 2 Handle when a new Phase 2 negotiation is being
*       started. It is called by both the Initiator and the
*       Responder.
*
* INPUTS
*
*       *phase2                 On return, this contains a new
*                               Handle.
*       *sa                     Pointer to Phase 1 SA.
*       *params                 State parameters to be used.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
*************************************************************************/
STATUS IKE_New_Phase2(IKE_PHASE2_HANDLE *phase2, IKE_SA *sa,
                      IKE_STATE_PARAMS *params)
{
    STATUS          status = NU_SUCCESS;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((phase2 == NU_NULL) || (sa == NU_NULL) || (params == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Generating new Phase 2 Handle");

    /* Set all fields of the Handle to zero. */
    UTL_Zero(phase2, sizeof(IKE_PHASE2_HANDLE));

    /* Set the remote node's address. */
    NU_BLOCK_COPY(&phase2->ike_node_addr, &sa->ike_node_addr,
                  sizeof(struct addr_struct));

    /* Set pointer to the IKE SA. */
    phase2->ike_sa = sa;

    /* Set pointer to state parameters. */
    phase2->ike_params = params;

    /* If we are the Initiator. */
    if(params->ike_packet == NU_NULL)
    {
        /* Generate a new Phase 2 message ID for this Handle. */
        status = IKE_Generate_Message_ID(&phase2->ike_msg_id);

        if(status == NU_SUCCESS)
        {
            /* Set Initiator flag. */
            phase2->ike_flags = IKE_INITIATOR;

            /* Set the initial exchange state. */
            phase2->ike_xchg_state = IKE_INITIATOR_START_STATE;
        }

        else
        {
            NLOG_Error_Log("Failed to generate new IKE Message ID",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        /* Set Message ID from the ISAKMP header. */
        phase2->ike_msg_id = params->ike_in.ike_hdr->ike_msg_id;

        /* Set Responder flag. */
        phase2->ike_flags = IKE_RESPONDER;

        /* Set the initial exchange state. */
        phase2->ike_xchg_state = IKE_RESPONDER_START_STATE;
    }

    /* Return the status. */
    return (status);

} /* IKE_New_Phase2 */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Select_Phase2_Security
*
* DESCRIPTION
*
*       This is a utility function used to select IPsec security
*       protocols specified in the initiate request. The
*       requested security protocol is negotiated, but in addition
*       to that, other protocols might be part of the security
*       protocol suite. Therefore, all security protocols which
*       are to be negotiated are added to an SA2 list, after
*       conversion to SA2 items.
*
* INPUTS
*
*       *req_security           Requested security protocol.
*       *pol_security           Complete list of security protocols
*                               specified in the IPsec policy.
*       pol_security_no         Number of items in above array.
*       *sa2db                  Database to which the selected
*                               security protocols are to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_UNALLOWED_XCHG2     Exchange not allowed by IPsec
*                               policy because security protocol
*                               does not match.
*
*************************************************************************/
STATUS IKE_Select_Phase2_Security(IPSEC_SECURITY_PROTOCOL *req_security,
                                  IPSEC_SECURITY_PROTOCOL *pol_security,
                                  UINT8 pol_security_no,
                                  IKE_SA2_DB *sa2db)
{
    STATUS                  status;
    IKE_SA2                 sa2;
    UINT8                   is_match = NU_FALSE;
    IPSEC_SECURITY_PROTOCOL local_req_security;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((req_security    == NU_NULL) || (pol_security == NU_NULL) ||
       (pol_security_no == 0)       || (sa2db        == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Copy the requested security protocol to a local variable. */
    NU_BLOCK_COPY(&local_req_security, req_security,
                  sizeof(IPSEC_SECURITY_PROTOCOL));

    /* Normalize the security protocol. */
    status = IPSEC_Normalize_Security(&local_req_security);

    /* Loop for each IPsec security protocol in the policy.
     *
     * The IPsec security protocols are provided in the same
     * order as specified in the IPsec policy. However, the
     * IKE proposal must contain these in the reverse order,
     * so run this loop in the reverse direction.
     */
    while((pol_security_no > 0) && (status == NU_SUCCESS))
    {
        /* Decrement count of security protocols. */
        pol_security_no--;

        /* If security protocol specified in the request
         * matches that in the IPsec policy.
         */
        if(memcmp(&local_req_security, &pol_security[pol_security_no],
                  sizeof(IPSEC_SECURITY_PROTOCOL)) == 0)
        {
            /* Set match flag to true. */
            is_match = NU_TRUE;
        }

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If no matching security protocol found till now. */
        if(is_match == NU_FALSE)
        {
            /* If the current security protocol is for tunnel mode. */
            if(pol_security[pol_security_no].ipsec_security_mode ==
               IPSEC_TUNNEL_MODE)
            {
                /* Flush the SA2 database, since all security
                 * protocols added till now were with some other
                 * remote host and not the one with which the
                 * exchange was requested.
                 */
                IKE_Flush_SA2(sa2db);

                /* Check the next security protocol, if any. */
                continue;
            }
        }
#endif

        /* Zero out the SA2 structure. */
        UTL_Zero(&sa2, sizeof(sa2));

        /* Set the local SPI of the inbound SA. Outbound
         * SA's SPI would be returned by the remote node.
         */
        sa2.ike_local_spi = IKE_Data.ike_spi_index;

        /* Increment the SPI counter for next use. */
        IKE_Data.ike_spi_index++;

        /* If the SPI variable has overflowed. */
        if(IKE_Data.ike_spi_index == 0)
        {
            NLOG_Error_Log("SPI variable wrapped to initial value",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            /* Reset to start of the SPI range assigned to IKE. */
            IKE_Data.ike_spi_index = IPSEC_SPI_END + 1;
        }

        /* Set the IPsec security protocol in SA2. */
        NU_BLOCK_COPY(&sa2.ike_ips_security,
                      &pol_security[pol_security_no],
                      sizeof(IPSEC_SECURITY_PROTOCOL));

        /* Add SA2 to the database. */
        status = IKE_Add_SA2(sa2db, &sa2);

#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
        /* If a matching security protocol was found. */
        if(is_match == NU_TRUE)
        {
            /* If the current security protocol is for tunnel mode. */
            if(pol_security[pol_security_no].ipsec_security_mode ==
               IPSEC_TUNNEL_MODE)
            {
                /* Do not check any more security protocols as
                 * they might be with some other remote host.
                 */
                break;
            }
        }
#endif
    }

    /* If none of the requested security protocols matched. */
    if(is_match == NU_FALSE)
    {
        NLOG_Error_Log(
            "Requested Security Protocol not allowed by IPsec policy",
            NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Set status to error. */
        status = IKE_UNALLOWED_XCHG2;
    }

    /* Return the status. */
    return (status);

} /* IKE_Select_Phase2_Security */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Set_Phase2_Security
*
* DESCRIPTION
*
*       This is a utility function used to set IPsec security
*       protocols specified in the initiate request, in the
*       Phase 2 Handle. If any IPsec security protocols are
*       missing from the request, then those are also added
*       to the Phase 2 Handle.
*
* INPUTS
*
*       *phase2                 On return, this contains a new
*                               Phase 2 Handle.
*       *request                The IKE Initiate request.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_NOT_FOUND         IPsec group or policy not found.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_UNALLOWED_XCHG2     Exchange not allowed by IPsec
*                               policy because security protocol
*                               does not match.
*
*************************************************************************/
STATIC STATUS IKE_Set_Phase2_Security(IKE_PHASE2_HANDLE *phase2,
                                      IKE_INITIATE_REQ *request)
{
    STATUS                  status;
    UINT32                  buffer_len;
    IPSEC_SECURITY_PROTOCOL *security;
    UINT8                   security_size;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameters are valid. */
    if((phase2 == NU_NULL) || (request == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Adding IPsec Security Protocols to Phase 2 Handle");

    /* Set length of the buffer. */
    buffer_len = sizeof(phase2->ike_ips_group_name);

    /* Get the IPsec group name. */
    status = IKE_IPS_Group_Name_By_Device(request->ike_dev_index,
                                          phase2->ike_ips_group_name,
                                          &buffer_len);

    if(status == NU_SUCCESS)
    {
        /* Look-up IPsec policy by selector. */
        status = IPSEC_Get_Policy_Index(phase2->ike_ips_group_name,
                                        &request->ike_ips_select,
                                        IPSEC_OUTBOUND,
                                        &phase2->ike_ips_policy_index);

        if(status == NU_SUCCESS)
        {
            /* Copy the IPsec selector to the Handle. */
            NU_BLOCK_COPY(&phase2->ike_ips_select,
                          &request->ike_ips_select,
                          sizeof(IPSEC_SELECTOR));

            /* Set the SA lifetime and PFS group description in
             * the Handle. Also get the security protocol. All
             * these parameters are obtained from the IPsec policy.
             */
            status = IKE_IPS_Get_Policy_Parameters(phase2, &security,
                                                   &security_size);

            if(status == NU_SUCCESS)
            {
                /* Select the IPsec security protocols to be negotiated
                 * by this exchange. The list of security protocols
                 * provided in the request may be incomplete, so make
                 * sure a complete security suite is negotiated.
                 */
                status =
                    IKE_Select_Phase2_Security(&request->ike_ips_security,
                                               security, security_size,
                                               &phase2->ike_sa2_db);

                /* Security protocols are no longer needed so
                 * free dynamically allocated buffer.
                 */
                if(NU_Deallocate_Memory(security) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Unable to get IPsec policy parameters",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to get IPsec policy by index",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("No IPsec policy corresponding to IKE exchange",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Set_Phase2_Security */

#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Send_Delete_Notification
*
* DESCRIPTION
*
*       This function looks up an IKE SA and if found, uses it
*       to send an IPsec inbound SA delete notification to the
*       remote node. This allows synchronization between the two
*       nodes.
*
* INPUTS
*
*       *request                An IKE exchange request for the
*                               IPsec SA to be deleted. The
*                               ipsec_tunnel_source member of
*                               security actually contains the
*                               SPI of the SA being deleted.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Group, Policy or IKE SA not found.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*
*************************************************************************/
STATUS IKE_Send_Delete_Notification(IKE_INITIATE_REQ *request)
{
    STATUS                  status;
    struct addr_struct      remote_addr;
    IKE_POLICY_SELECTOR     select;
    IKE_POLICY_GROUP        *group;
    IKE_POLICY              *policy;
    IKE_SA                  *sa;
    UINT8                   proto;
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
    IKE2_EXCHANGE_HANDLE    *handle = NU_NULL;
    IKE2_STATE_PARAMS       params;
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure the parameter is valid. */
    if(request == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    IKE_DEBUG_LOG("Processing IPsec SA deletion notification request");

    /* First grab the semaphore. */
    status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Get the IKE Group for this exchange. */
        status = IKE_Get_Group_Entry_By_Device(request->ike_dev_index,
                                               &group);

        if(status == NU_SUCCESS)
        {
            /* Convert IPsec selector to IKE selector. */
            IKE_IPS_Selector_To_IKE(&request->ike_ips_select, &select);

            /* Get IKE Policy which applies to this exchange. */
            status = IKE_Get_Policy_By_Selector(group->ike_group_name,
                                                &select, NU_NULL, &policy,
                                                IKE_MATCH_SELECTORS);

            if(status == NU_SUCCESS)
            {
                /* Make sure policy allows Informational exchange. */
                if((policy->ike_phase2_xchg & IKE_XCHG_INFO_FLAG) != 0)
                {
#if (IPSEC_INCLUDE_TUNNEL_MODE == NU_TRUE)
                    /* If tunnel mode is being used. */
                    if(request->ike_ips_security.ipsec_security_mode ==
                       IPSEC_TUNNEL_MODE)
                    {
                        /* Convert IPsec security to IKE address. */
                        IKE_IPS_Security_To_Remote_IP(
                            &request->ike_ips_security, &remote_addr);
                    }

                    else
#endif
                    {
                        /* Convert IPsec selector to IKE address. */
                        IKE_IPS_Selector_To_Remote_IP(
                            &request->ike_ips_select, &remote_addr);
                    }

                    /* Search for an IKE SA. */
                    status = IKE_Get_SA(&policy->ike_sa_list, &remote_addr,
                                        IKE_MATCH_SA_IP, &sa);

                    if(status == NU_SUCCESS)
                    {
                        /* Convert IPsec protocol to the IKE identifier. */
                        proto = IKE_Protocol_ID_IPS_To_IKE(
                            request->ike_ips_security.ipsec_protocol);

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                        if(policy->ike_version == IKE_VERSION_1)
                        {
#endif
                            /* Send the notification. The tunnel source
                             * address actually contains the SPI of the
                             * SA being deleted.
                             */
                            status = IKE_Delete_Notification(sa, proto,
                                request->ike_ips_security.ipsec_tunnel_source);

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                        }
                        else
                        {
                            /* Get the exchange handle */
                            status = IKE2_Exchange_Lookup(sa,
                                        &policy->ike_select, &handle);
                            if(status == NU_SUCCESS)
                            {
                                UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));
                                params.ike2_group = group;
                                params.ike2_policy = policy;
                                handle->ike2_params = &params;

                                IKE2_Send_Info_Delete(handle, proto,
                                    IKE_IPS_SPI_LEN, 1,
                                     request->ike_ips_security.ipsec_tunnel_source);

                                handle->ike2_params = NU_NULL;
                            }
                            else
                            {
                                NLOG_Error_Log("Exchange handle not found",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
#endif
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to find IKE SA",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Unable to get IKE policy by selector",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to get IKE group by device",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the semaphore. */
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

    /* Return the status. */
    return (status);

} /* IKE_Send_Delete_Notification */
#endif

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
*       ike2_ctrl.c
*
* COMPONENT
*
*       Exchange Control
*
* DESCRIPTION
*
*       Exchange initiation and handling
*
* FUNCTIONS
*
*       IKE2_IPS_Get_Policy_Parameters
*       IKE2_Initiate_IKE2_Exchange
*       IKE2_Initiate_IKE2_Child_Exchange
*       IKE2_New_Exchange
*       IKE2_Set_CREATE_CHILD_SA_Security
*       IKE2_Dispatch
*       IKE2_Dispatch_INIT_SA
*       IKE2_Dispatch_CREATE_CHILD
*       IKE2_Cleanup_Exchange_Parameters
*       IKE2_Resume_Child_Exchanges
*       IKE2_IPS_Get_Policy_Parameters
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_api.h
*       ike2_pload.h
*       ike_ips.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_api.h"
#include "networking/ike_ips.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

#include "networking/ike2_pload.h"

extern NU_SEMAPHORE        IPSEC_Resource;

/* Local function declarations. */
STATIC STATUS IKE2_IPS_Get_Policy_Parameters(IKE2_EXCHANGE_HANDLE *handle,
                                             IPSEC_SECURITY_PROTOCOL **security,
                                             UINT8 *security_size);

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Initiate_IKE2_Exchange
*
* DESCRIPTION
*
*       This function is called to start a new IKEv2 exchange. It is
*       called by IKE_Initiate if the request is for version 2 exchange.
*
* INPUTS
*
*       *group              The policy group the policy belongs to
*       *policy             The policy structure for this exchange
*       *request            IKE request, contains information about IPsec
*                           SA to be negotiated
*       *remote_addr        Address of the peer that SA needs to be
*                           established with
*       *ret_msg_id         ID to be returned for this message
*
* OUTPUTS
*
*       NU_SUCCESS          On successful initiation of IKEv2 exchange.
*       IKE2_INVALID_PARAMS Parameters provided are not correct.
*
*************************************************************************/
STATUS IKE2_Initiate_IKE2_Exchange(IKE2_POLICY_GROUP *group,
                                   IKE2_POLICY *policy,
                                   IKE2_INITIATE_REQ *request,
                                   struct addr_struct *remote_addr,
                                   UINT32 *ret_msg_id)
{
    STATUS                  status;
    IKE2_EXCHANGE_HANDLE    *xchg_handle = NU_NULL;
    IKE2_STATE_PARAMS       params;
    IKE2_SA                 *sa = NU_NULL;

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if((group   == NU_NULL) || (policy      == NU_NULL) ||
       (request == NU_NULL) || (remote_addr == NU_NULL) ||
       (ret_msg_id == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Initiating IKE_SA_INIT exchange.");

    UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));

    /* Allocate memory for new exchange handle. */
    status = IKE2_New_Exchange(&xchg_handle, &sa, &params);

    if(status == NU_SUCCESS)
    {
        params.ike2_group = group;
        params.ike2_policy = policy;

        /* Each IKEv2 message is identified by a unique ID. Keep track of
         * what ID to use next.
         */
        xchg_handle->ike2_next_id_nu = 0;
        xchg_handle->ike2_next_id_peer = 0;

        /* Set remote node's address in exchange handle. */
        NU_BLOCK_COPY(&sa->ike_node_addr, remote_addr, sizeof(struct addr_struct));

        if(policy->ike_peers_id.ike_type == IKE2_ID_TYPE_FQDN ||
                policy->ike_peers_id.ike_type == IKE2_ID_TYPE_RFC822_ADDR)
        {
            sa->ike_node_addr.name = policy->ike_peers_id.ike_addr.ike_domain;
        }

        /* Add the SA structure in the list of SA's against this policy. */
        status = IKE2_Add_IKE_SA(&policy->ike_sa_list, sa);

        if(status == NU_SUCCESS)
        {
            /* Add the exchange handle to the SA's database of exchange
             * handles associated with this SA. There may be more than
             * one handles if more child SAs are negotiated under this SA.
             */
            status = IKE2_Add_Exchange_Handle(&sa->xchg_db, xchg_handle);

            if(status == NU_SUCCESS)
            {
                /* Set security parameters for the IPsec SA to be
                 * established under this IKE SA. This information will
                 * be used after IKE SA has been established.
                 */
                status = IKE2_Set_CREATE_CHILD_SA_Security(xchg_handle,
                                                           request);
                if(status == NU_SUCCESS)
                {
                    xchg_handle->ike2_socket = IKE_Data.ike_socket;

                    /* All done, enter the IKEv2 state machine. */
                    status = IKE2_Process_SA_INIT(xchg_handle);
                }
                else
                {
                    NLOG_Error_Log("Unable to add IPsec security",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Unable to add exchange handle to exchange DB",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

        }

        else
        {
            NLOG_Error_Log("Unable to add SA to IKEv2 SADB",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for IKEv2 state parameters",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Initiate_IKE2_Exchange */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Initiate_IKE2_Child_Exchange
*
* DESCRIPTION
*
*       This function starts an exchange to establish a child SA
*
* INPUTS
*
*       *group              Group this policy belongs to
*       *policy             Policy for current exchange
*       *request            The request structure containing info about
*                           IPsec security parameters
*       *sa                 IKE SA under which to establish child SA
*       *ret_msg_id         Message ID to be returned for this message
*
* OUTPUTS
*
*       NU_SUCCESS          On successful execution
*
*************************************************************************/
STATUS IKE2_Initiate_IKE2_Child_Exchange(IKE2_POLICY_GROUP *group,
                                         IKE2_POLICY *policy,
                                         IKE2_INITIATE_REQ *request,
                                         IKE2_SA *sa, UINT32 *ret_msg_id)
{
    STATUS                  status;
    IKE2_EXCHANGE_HANDLE    *handle;
    IKE2_STATE_PARAMS       params;

#if (IKE2_DEBUG == NU_TRUE)
    /* Validate parameters. */
    if((policy == NU_NULL) || (sa == NU_NULL) || (ret_msg_id == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("IKE2 SA found. Starting IKE_CHILD exchange");

    UNUSED_PARAMETER(group);

    UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));

    /* First look up the exchange handle in Exchange handle database. */
    status = IKE2_Exchange_Lookup(sa, &policy->ike_select, &handle);

    if(status == NU_SUCCESS)
    {
        /* Make sure we found the correct handle that has the same
         * IKE SA set in it.
         */
        if(handle->ike2_sa == sa)
        {
            handle->ike2_state = IKE2_STATE_CREATE_CHILD_SA_I;
            if(request == NU_NULL)
            {
                /* This is an IKE SA re-keying request. */
                handle->ike2_flags |= IKE2_SA_REKEY;
                params.ike2_policy = policy;
                handle->ike2_params = &params;
            }

            else
            {
                /* This is an IPsec SA request. */

                params.ike2_policy = policy;
                handle->ike2_params = &params;

                /* Set create_child_sa phase security parameters */
                status = IKE2_Set_CREATE_CHILD_SA_Security(handle, request);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to extract security parameters",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            if(status == NU_SUCCESS)
            {
                status = IKE2_Process_CREATE_CHILD_SA(handle);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to create child SA",
                        NERR_RECOVERABLE, __FILE__,
                        __LINE__);
                }
            }
        }

        *ret_msg_id = handle->ike2_next_id_nu;

        handle->ike2_params = NU_NULL;
    }

    else
    {
        NLOG_Error_Log("No exchange handle exists, cannot continue exchange",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Initiate_IKE2_Child_Exchange */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_New_Exchange
*
* DESCRIPTION
*
*       Allocates new Exchange Handle structure and sets up memory and
*       pointer in this new handle.
*
* INPUTS
*
*       **xchg_handle       Pointer to the structure to be allocated
*       **sa                SA structure to be allocated
*       *params             State params structure to be used
*
* OUTPUTS
*
*       NU_SUCCESS          On success
*       IKE2_INVALID_PARAMS Parameters are invalid
*
*************************************************************************/
STATUS IKE2_New_Exchange(IKE2_EXCHANGE_HANDLE **xchg_handle,
                         IKE2_SA **sa, IKE2_STATE_PARAMS *params)
{
    STATUS  status;

#if (IKE2_DEBUG == NU_TRUE)
    /* Validate parameters. */
    if ((xchg_handle == NU_NULL) || (sa == NU_NULL) || (params == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Generating IKEv2 Exchange Handle and IKE SA.");

    status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID **)xchg_handle,
                                sizeof(IKE2_EXCHANGE_HANDLE), NU_NO_SUSPEND);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for Exchange handle",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    if(status == NU_SUCCESS)
    {
        /* Zero out fields in new structures. */
        UTL_Zero(*xchg_handle, sizeof(IKE2_EXCHANGE_HANDLE));

        /* If there is no packet in the state parameters, we are the
         * initiator however, if there is a packet here already, this is
         * the one that came from the peer and we are now the responder.
         */
        if(params->ike2_packet == NU_NULL)
        {
            /* We are initiator of IKE2 exchange. */
            (*xchg_handle)->ike2_flags = IKE2_INITIATOR;
            (*xchg_handle)->ike2_state = IKE2_INITIATOR_START_STATE;
        }
        else
        {
            /* We are responder here since we already have a packet. */
            (*xchg_handle)->ike2_flags = IKE2_RESPONDER;
            (*xchg_handle)->ike2_state = IKE2_RESPONDER_START_STATE;
        }

        /* If no SA has been passed, allocate a new one. This must be
         * a new exchange. Otherwise, just use the passed in SA.
         */
        if(*sa == NU_NULL)
        {
            /* If there is no SA for this exchange, allocate a new one. */
            status = NU_Allocate_Memory(IKE_Data.ike_memory, (VOID **)sa,
                                        sizeof(IKE2_SA), NU_NO_SUSPEND);
            if(status == NU_SUCCESS)
            {
                UTL_Zero(*sa, sizeof(IKE2_SA));

                /* Set the status to incomplete. It will be updated as
                 * the exchange progresses.
                 */
                (*sa)->ike_state = IKE2_SA_INIT_INCOMPLETE;
            }

            else
            {
                NLOG_Error_Log("Failed to allocate memory for SA",
                    NERR_RECOVERABLE, __FILE__, __LINE__);

            }
        }

        else
        {
            (*sa)->ike_state = IKE2_SA_ESTABLISHED;
        }

        if(status == NU_SUCCESS)
        {
            /* Initialize fields in the exchange handle. */
            (*xchg_handle)->ike2_sa = *sa;
            (*xchg_handle)->ike2_params = params;
            (*xchg_handle)->ike2_lifetime = IKE2_SA_TIMEOUT;
            (*xchg_handle)->ike2_next_id_nu = 0;
            (*xchg_handle)->ike2_next_id_peer = 0;
            (*xchg_handle)->ike2_resend_count = IKE_RESEND_COUNT;

            /* Set a timeout for this Exchange. */
            status = IKE_Set_Timer(IKE2_INIT_AUTH_Timeout,
                                   (UNSIGNED)(*sa),
                                   (UNSIGNED)(*xchg_handle),
                                   IKE2_EXCHANGE_TIMEOUT);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Exchange timeout event could not be added",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        if(status != NU_SUCCESS)
        {
            if(NU_Deallocate_Memory(*xchg_handle) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate memory for Handle",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    return (status);
} /* IKE2_New_Exchange */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Set_CREATE_CHILD_SA_Security
*
* DESCRIPTION
*
*       Converts IPsec security parameters into a form that IKEv2 code
*       understands. This information is used during the IPsec SA
*       negotiation.
*
* INPUTS
*
*       *handle             Exchange handle for this SA request
*       *request            Request structure with the needed information
*                           for IPsec SA
*
* OUTPUTS
*
*       NU_SUCCESS          On success
*       IKE2_INVALID_PARAMS Parameters are invalid
*
*************************************************************************/
STATUS IKE2_Set_CREATE_CHILD_SA_Security(IKE2_EXCHANGE_HANDLE *handle,
                                         IKE2_INITIATE_REQ *request)
{
    STATUS                  status;
    UINT32                  buffer_len;
    IPSEC_SECURITY_PROTOCOL *security;
    UINT8                   security_size = 0;

#if (IKE2_DEBUG == NU_TRUE)
    /* Validate the parameters. */
    if((handle == NU_NULL) || (handle->ike2_params == NU_NULL) ||
       (request == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    buffer_len = sizeof(handle->ike2_ips_group);

    /* Get the IPsec group name. */
    status = IKE_IPS_Group_Name_By_Device(request->ike_dev_index,
                                          handle->ike2_ips_group,
                                          &buffer_len);
    if(status == NU_SUCCESS)
    {
        /* Copy the IPsec selector to the Handle. */
        NU_BLOCK_COPY(&handle->ike2_ips_selector, &request->ike_ips_select,
                      sizeof(IPSEC_SELECTOR));

        /* Get the IPsec Policy using the IPsec Selector. */
        status = IPSEC_Get_Policy_Index(handle->ike2_ips_group,
                                        &(handle->ike2_ips_selector),
                                        IPSEC_OUTBOUND,
                                        &(handle->ike2_ips_policy_index));

        if(status == NU_SUCCESS)
        {
            status = IKE2_IPS_Get_Policy_Parameters(handle, &security,
                                                    &security_size);

            if(status == NU_SUCCESS)
            {
                /* If we have to use transport mode, we need to tell
                 * IKEv2 to negotiate it using an informational payload.
                 * Check to see if the transport mode is set in request.
                 */
                if(request->ike_ips_security.ipsec_security_mode ==
                   IPSEC_TRANSPORT_MODE)
                {
                    handle->ike2_flags |= IKE2_USE_TRANSPORT_MODE;
                }

                /* Select the IPsec security protocols to be negotiated
                 * by this exchange. The list of security protocols
                 * provided in the request may be incomplete, so make
                 * sure a complete security suite is negotiated.
                 */
                status = IKE_Select_Phase2_Security(
                                                &request->ike_ips_security,
                                                security, security_size,
                                                &handle->ike2_sa2_db);
                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set IPsec security parameters",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Deallocate the "security" since it is dynamically
                 * allocated. */
                if(NU_Deallocate_Memory(security) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate Security memory",
                        NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to set IPsec policy parameters",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to get IPsec policy index",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to get group name", NERR_RECOVERABLE,
                        __FILE__, __LINE__);
    }

    return (status);
} /* IKE2_Set_CREATE_CHILD_SA_Security */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Dispatch
*
* DESCRIPTION
*
*       This function is the entry point for main IKEv2 processing. It
*       dispatches IKEv2 exchanges to appropriate handlers based on their
*       types.
*
* INPUTS
*
*       *pkt                    The IKEv2 message
*       *policy                 Policy to be applied to this exchange
*
* OUTPUTS
*
*       NU_SUCCESS              On successful execution.
*       IKE2_INVALID_PARAMS     Parameters are incorrect.
*       IKE_INVALID_XCHG_TYPE   Exchange type specified in incoming packet
*                               is not valid.
*
*************************************************************************/
STATUS IKE2_Dispatch(IKE2_PACKET *pkt, IKE2_POLICY *policy)
{
    STATUS          status;
    IKE2_HDR        ike2_hdr;
    IKE2_SA         *sa = NU_NULL;
    UINT8           *spi_local;
    UINT8           *spi_peer;

#if(IKE2_DEBUG == NU_SUCCESS)
    if((pkt == NU_NULL) || (policy == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif
    /* Policy has already been found and passed to us.*/
    status = IKE2_Decode_IKE_Header(pkt->ike_data, pkt->ike_data_len,
                                    &ike2_hdr);
    if(status == NU_SUCCESS)
    {
        /* We have already checked the major version number of this
         * message. Minor version number should be ignored on receipt
         * as per RFC 4306, and so should be the reserved bits. We,
         * therefore, don't check those fields here.
         */

        /* Check if message length is valid. */
        if(ike2_hdr.ike2_length > (UINT32)pkt->ike_data_len)
        {
            NLOG_Error_Log("Invalid IKEv2 packet length",
                NERR_RECOVERABLE, __FILE__, __LINE__);
            status = IKE_INVALID_LENGTH;
        }

        else
        {
            /* All checks have been successful. Proceed with packet
             * processing.
             */
            if((ike2_hdr.ike2_flags & IKE2_HDR_INITIATOR_FLAG) != 0)
            {
                /* We were responders. */
                spi_local = ike2_hdr.ike2_sa_spi_r;
                spi_peer = ike2_hdr.ike2_sa_spi_i;
            }
            else
            {
                /* The packet we received came with responder flag set.
                 * In this case, we were the initiators.
                 */
                spi_local = ike2_hdr.ike2_sa_spi_i;
                spi_peer = ike2_hdr.ike2_sa_spi_r;
            }

            if(IKE2_Find_SA(&policy->ike_sa_list, spi_local, spi_peer, &sa)
                    != NU_SUCCESS)
            {
                /* SA not found. This may be first packet in exchange.
                 * However, check to see if the packet is encrypted,
                 * in which case, we cannot proceed further without an SA
                 */
                if(ike2_hdr.ike2_next_payload == IKE2_ENCRYPT_PAYLOAD_ID)
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
            if((ike2_hdr.ike2_exchange_type == IKE2_SA_INIT) ||
                ike2_hdr.ike2_exchange_type == IKE2_AUTH)
            {
                status = IKE2_Dispatch_INIT_SA(pkt, &ike2_hdr, sa, policy);
            }
            else if(ike2_hdr.ike2_exchange_type == IKE2_CREATE_CHILD_SA)
            {
                status = IKE2_Dispatch_CREATE_CHILD(pkt, &ike2_hdr, sa, policy);
            }
            else if(ike2_hdr.ike2_exchange_type == IKE2_INFORMATIONAL)
            {
                status = IKE2_Dispatch_INFORMATIONAL(pkt, &ike2_hdr, sa, policy);
            }
            else
            {
                status = IKE_INVALID_XCHG_TYPE;

                NLOG_Error_Log("Invalid exchange type received.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            IKE2_DEBUG_LOG(
                "Error encountered in incoming message, packet discarded");
        }
    }
    else
    {
        NLOG_Error_Log("Unable to decode IKEv2 message header",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Dispatch */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Dispatch_INIT_SA
*
* DESCRIPTION
*
*       Handler and dispatcher for INIT_SA exchange type.
*
* INPUTS
*
*       *pkt                    Current packet to be processed
*       *hdr                    IKEv2 header for this packet
*       *sa                     SA to be used for this message
*       *policy                 Policy to be applied to this exchange
*
* OUTPUTS
*
*       NU_SUCCESS              Success.
*       IKE_INVALID_PARAMS      Parameters are incorrect.
*
*************************************************************************/
STATUS IKE2_Dispatch_INIT_SA(IKE2_PACKET *pkt, IKE2_HDR *hdr,
                             IKE2_SA *sa, IKE2_POLICY *policy)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_STATE_PARAMS       params;
    IKE2_EXCHANGE_HANDLE    *handle = NU_NULL;
    UINT32                  group_len;

#if(IKE2_DEBUG == NU_TRUE)
    if((pkt == NU_NULL) || (hdr == NU_NULL) || (policy == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Dispatching IKE_SA_INIT exchange");

    UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));

    params.ike2_packet = pkt;
    params.ike2_policy = policy;
    params.ike2_in.ike2_hdr = hdr;

    /* If SA is not present, this is a new exchange. */
    if(sa == NU_NULL)
    {
        status = IKE2_New_Exchange(&handle, &sa, &params);

        if(status == NU_SUCCESS)
        {
            group_len = IKE_MAX_GROUP_NAME_LEN;

            status = IKE_IPS_Group_Name_By_Device(pkt->ike_if_index,
                                                  handle->ike2_ips_group,
                                                  &group_len);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to get IPsec group name",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to create new exchange",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    else if(sa->ike_state == IKE_SA_ESTABLISHED)
    {
        /* SA already established. This packet is useless. */
        status = IKE2_INVALID_PARAMS;
		
        /* If it is an AUTH packet from PEER(Initiator), it may be the
         * peer did not receive the response AUTH we sent.
         * Retransmit the saved copy of AUTH message from handle. */
        if((hdr->ike2_exchange_type == IKE2_AUTH) &&
                        (hdr->ike2_flags & IKE2_HDR_INITIATOR_FLAG))
        {
            NLOG_Error_Log("Received an unexpected IKE AUTH message", 
                                NERR_INFORMATIONAL, __FILE__, __LINE__);

            if(IKE2_Exchange_Lookup(sa, &policy->ike_select, &handle) == NU_SUCCESS &&
                                        (hdr->ike2_msg_id == handle->ike2_next_id_peer))
            {
                if(IKE2_Resend_Packet(handle) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to re-send IKE AUTH message.",
                                         NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Exchange handle not found",
                                         NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }		
    }

    else
    {
        /* SA structure already exists but SA is not established yet.
         * We should have an exchange handle in this situation.
         */
        status = IKE2_Exchange_Lookup(sa, &policy->ike_select, &handle);

        if(status == NU_SUCCESS)
        {
            handle->ike2_params = &params;
        }

        else
        {
            NLOG_Error_Log("Exchange handle not found",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Copy the peer's address in the SA structure. */
        NU_BLOCK_COPY(&sa->ike_node_addr, &pkt->ike_remote_addr,
            sizeof(struct addr_struct));

        if(policy->ike_peers_id.ike_type == IKE2_ID_TYPE_FQDN ||
                policy->ike_peers_id.ike_type == IKE2_ID_TYPE_RFC822_ADDR)
        {
            sa->ike_node_addr.name = policy->ike_peers_id.ike_addr.ike_domain;
        }
        handle->ike2_socket = pkt->ike2_socket;
        handle->ike2_policy = policy;

        if((handle->ike2_flags & IKE2_RESPONDER) != 0)
        {
            /* If we are responder, pick up the next message ID from
             * packet as it has to be same as the ID we got in the
             * request packet.
             */
            handle->ike2_next_id_peer = hdr->ike2_msg_id;
        }

        status = IKE2_Process_SA_INIT(handle);

        if(status == NU_SUCCESS)
        {
            if(handle->ike2_sa->ike_state == IKE2_SA_ESTABLISHED)
            {
                /* INITIAL_CONTACT not supported in this version. Refer to
                 * the RFC 4306 section 3.10.1 (Notify Message Types).
                 * See the INITIAL_CONTACT sub-section.
                 */

                /* SA establishment has been completed. */
                IKE2_DEBUG_LOG("IKEv2 INIT_SA exchange complete.");

                /* SA established successfully. Remove all other events
                 * on this SA and add a lifetime expiry event.
                 */
                IKE_Unset_Matching_Events((UNSIGNED)handle->ike2_sa,
                                          0, TQ_CLEAR_ALL);

                status = IKE_Set_Timer(IKE2_SA_Timeout_Event,
                                       (UNSIGNED)handle->ike2_sa,
                                       (UNSIGNED)&params.ike2_policy->ike_sa_list,
                                       handle->ike2_sa->ike_attributes.
                                                ike_sa_lifetime.ike_no_of_secs);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("SA timeout event could not be added",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
    }

    IKE2_Cleanup_Exchange_Parameters(&params);

    return (status);

} /* IKE2_Dispatch_INIT_SA */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Dispatch_CREATE_CHILD
*
* DESCRIPTION
*
*       Dispatches CREATE_CHILD exchange to appropriate handlers.
*
* INPUTS
*
*       *pkt                    Current packet to be processed
*       *hdr                    IKEv2 header for this packet
*       *sa                     SA to be used for this message
*       *policy                 Policy to be applied to this exchange
*
* OUTPUTS
*
*       NU_SUCCESS              Success.
*       IKE_INVALID_PARAMS      Parameters are incorrect.
*
*************************************************************************/
STATUS IKE2_Dispatch_CREATE_CHILD(IKE2_PACKET *pkt, IKE2_HDR *hdr,
                                  IKE2_SA *sa, IKE2_POLICY *policy)
{
    STATUS                  status;
    IKE2_STATE_PARAMS       params;
    IKE2_EXCHANGE_HANDLE    *handle;
    UINT32                  group_len;

#if(IKE2_DEBUG == NU_TRUE)
    if((pkt == NU_NULL) || (hdr == NU_NULL) || (sa == NU_NULL) ||
        (policy == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif
    IKE2_DEBUG_LOG("Dispatching IKEv2 CREATE_CHILD_SA exchange");

    UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));

    params.ike2_packet = pkt;
    params.ike2_policy = policy;
    params.ike2_in.ike2_hdr = hdr;

    if(sa == NU_NULL)
    {
        status = IKE2_INVALID_PARAMS;
        NLOG_Error_Log("SA required here but not found", NERR_RECOVERABLE,
                        __FILE__, __LINE__);
    }

    else
    {
        status = IKE2_Exchange_Lookup(sa, &policy->ike_select, &handle);

        if(status != NU_SUCCESS)
        {
            /* If no handle is found, this can be a new CHILD exchange
             * existing IKE SA or a new CHILD exchange altogether! So,
             * create a new handle.
             */
            status = IKE2_New_Exchange(&handle, &sa, &params);

            if(status == NU_SUCCESS)
            {
                handle->ike2_state = IKE2_STATE_CREATE_CHILD_SA_R;

                group_len = IKE_MAX_GROUP_NAME_LEN;

                status = IKE_IPS_Group_Name_By_Device(
                            pkt->ike_if_index, handle->ike2_ips_group,
                            &group_len);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to get IPsec group name",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("New exchange handle could not be created",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        else
        {
            handle->ike2_params = &params;
        }

        if(status == NU_SUCCESS)
        {
            handle->ike2_socket = pkt->ike2_socket;

            if((handle->ike2_flags & IKE2_RESPONDER) != 0 &&
                (hdr->ike2_flags & IKE2_HDR_RESPONSE_FLAG) == 0)
            {
                /* If we are responder, pick up the next message ID from
                 * packet as it has to be same as the ID we got in the
                 * request packet.
                 */
                handle->ike2_next_id_peer = hdr->ike2_msg_id;
            }

            status = IKE2_Process_CREATE_CHILD_SA(handle);
        }

        handle->ike2_params = NU_NULL;
    }

    IKE2_Cleanup_Exchange_Parameters(&params);

    return (status);

} /* IKE2_Dispatch_CREATE_CHILD */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Cleanup_Exchange_Parameters
*
* DESCRIPTION
*
*       This is a utility function which cleans up the parameters
*       after processing of every 'incoming packet'.
*
* INPUTS
*
*       *params                 Pointer to the State parameters.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Cleanup_Exchange_Parameters(IKE2_STATE_PARAMS *params)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_NOTIFY_PAYLOAD     *notify_pload;
    IKE2_NOTIFY_PAYLOAD     *next_notify_pload;
    IKE2_DELETE_PAYLOAD     *del_pload;
    IKE2_DELETE_PAYLOAD     *next_del_pload;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Some payloads in the incoming packet are dynamically allocated
     * so we deallocate them here.
     */
    if(params->ike2_in.ike2_notify != NU_NULL)
    {
        notify_pload = params->ike2_in.ike2_notify;

        /* Traverse the list of all Notify payloads. */
        while(notify_pload != NU_NULL)
        {
            next_notify_pload = notify_pload->ike2_next;

            /* The "notify data" member is also dynamically allocated. */
            if(notify_pload->ike2_notify_data != NU_NULL)
            {
                if(NU_Deallocate_Memory(notify_pload->ike2_notify_data)
                        != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate N payload data",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
            if(NU_Deallocate_Memory(notify_pload) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate N payload memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            notify_pload = next_notify_pload;
        }

        params->ike2_in.ike2_notify = NU_NULL;
    }

    if(params->ike2_in.ike2_del != NU_NULL)
    {
        del_pload = params->ike2_in.ike2_del;

        /* Traverse the list of all Delete payloads. */
        while(del_pload != NU_NULL)
        {
            next_del_pload = del_pload->ike2_next;
            if(NU_Deallocate_Memory(del_pload) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate D payload memory",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            del_pload = next_del_pload;
        }

        params->ike2_in.ike2_del = NU_NULL;
    }

    return (status);

} /* IKE2_Cleanup_Exchange_Parameters */

/*************************************************************************
*
* FUNCTION
*
*       IKE2_IPS_Get_Policy_Parameters
*
* DESCRIPTION
*
*       This is a utility function which looks up the IPsec
*       security policy using the specified index and then
*       allocates a buffer and returns the Policy's array
*       of security protocols.
*
* INPUTS
*
*       *handle                 Exchange information. Contains IPsec group
*                               name and policy index.
*       **security              On return, this contains a
*                               pointer to the buffer containing
*                               the security protocols.
*       *security_size          On return, this contains the
*                               number of items in the array.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*
*************************************************************************/
STATIC STATUS IKE2_IPS_Get_Policy_Parameters(IKE2_EXCHANGE_HANDLE *handle,
                                             IPSEC_SECURITY_PROTOCOL **security,
                                             UINT8 *security_size)
{
    STATUS                  status;
    IPSEC_POLICY_GROUP      *group;
    IPSEC_POLICY            *policy;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((handle == NU_NULL) || (security == NU_NULL) ||
        (security_size == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* First grab the IPsec semaphore. */
    status = NU_Obtain_Semaphore(&IPSEC_Resource, IKE_TIMEOUT);

    /* Check the status value. */
    if(status == NU_SUCCESS)
    {
        /* Get the group entry pointer. */
        status = IPSEC_Get_Group_Entry(handle->ike2_ips_group, &group);

        /* Check the status value. */
        if(status == NU_SUCCESS)
        {
            /* Get the IPsec policy entry. */
            status = IPSEC_Get_Policy_Entry(group,
                                            handle->ike2_ips_policy_index,
                                            &policy);

            if(status == NU_SUCCESS)
            {
                /* Make sure action of this policy is apply. */
                if((policy->ipsec_flags & IPSEC_APPLY) == 0)
                {
                    NLOG_Error_Log("IPsec policy has bypass action instead\
                                   of apply - exchange not possible",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    status = IKE_INVALID_PARAMS;
                }

                else
                {
                    /* Get selector from the IPsec policy. */
                    NU_BLOCK_COPY(&handle->ike2_ips_pol_selector,
                                  &policy->ipsec_select,
                                  sizeof(IPSEC_SELECTOR));

                    /* Get lifetime from the IPsec policy. */
                    NU_BLOCK_COPY(&handle->ike2_ips_lifetime,
                                  &policy->ipsec_sa_max_lifetime,
                                  sizeof(IPSEC_SA_LIFETIME));

                    /* Get PFS group description from the IPsec policy. */
                    /*handle = policy->ipsec_pfs_group_desc;*/

                    /* Get IPsec policy flags. */
                    handle->ike2_sa2_db.ike_ips_flags = policy->ipsec_flags;

                    /* Allocate memory for the security protocol array. */
                    status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID**)security,
                                    (policy->ipsec_security_size *
                                    sizeof(IPSEC_SECURITY_PROTOCOL)),
                                    NU_NO_SUSPEND);

                    if(status == NU_SUCCESS)
                    {
                        /* Normalize the pointer. */
                        *security = TLS_Normalize_Ptr(*security);

                        /* Set number of items in security array. */
                        *security_size = policy->ipsec_security_size;

                        /* Get security protocols from the IPsec Policy. */
                        NU_BLOCK_COPY(*security, policy->ipsec_security,
                                      (policy->ipsec_security_size *
                                      sizeof(IPSEC_SECURITY_PROTOCOL)));
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }
        }

        /* Now everything is done, release the semaphore too. */
        if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain IPsec semaphore",
            NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE2_IPS_Get_Policy_Parameters */

#endif

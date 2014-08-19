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
*       ike2_info.c
*
* COMPONENT
*
*       IKE - Informational Messages
*
* DESCRIPTION
*
*       This component is responsible for dispatching and processing IKEv2
*       Informational messages.
*
* FUNCTIONS
*
*       IKE2_Dispatch_INFORMATIONAL
*       IKE2_Process_Notify
*       IKE2_Generate_Notify
*       IKE2_Process_Delete
*       IKE2_Generate_Delete
*       IKE2_Send_Info_Notification
*       IKE2_Send_Info_Delete
*       IKE2_Process_Info_Xchg
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_api.h
*       ike_db.h
*       ike_ips.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_db.h"
#include "networking/ike_ips.h"

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)

/*************************************************************************
*
* FUNCTION
*
*       IKE2_Dispatch_INFORMATIONAL
*
* DESCRIPTION
*
*       Dispatches informational messages.
*
* INPUTS
*
*       *pkt                    Packet being processed
*       *hdr                    IKEv2 header for current packet
*       *sa                     SA to be used for this packet
*       *policy                 Policy to be applied to this packet
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*
*************************************************************************/
STATUS IKE2_Dispatch_INFORMATIONAL(IKE2_PACKET *pkt, IKE2_HDR *hdr,
                                   IKE2_SA *sa, IKE2_POLICY *policy)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_STATE_PARAMS       params;
    IKE2_EXCHANGE_HANDLE    *handle;

#if (IKE2_DEBUG == NU_TRUE)
    /* Parameter sanity check. */
    if((pkt == NU_NULL) || (hdr == NU_NULL) || (sa == NU_NULL) ||
       (policy == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Dispatching IKEv2 Informational packet");

    /* Clear the memory used by state params structure. */
    UTL_Zero(&params, sizeof(IKE2_STATE_PARAMS));

    /* Set pointers in state params to appropriate data structures to be
     * used by other functions.
     */
    params.ike2_packet = pkt;
    params.ike2_policy = policy;
    params.ike2_in.ike2_hdr = hdr;

    /* Find the exchange handle under which this SA was negotiated. */
    status = IKE2_Exchange_Lookup(sa, &policy->ike_select, &handle);

    if(status == NU_SUCCESS)
    {
        handle->ike2_params = &params;

        if((hdr->ike2_flags & IKE2_HDR_RESPONSE_FLAG) == 0)
        {
            /* If we are responder, pick up the next message ID from
             * packet as it has to be same as the ID we got in the
             * request packet.
             */
            handle->ike2_next_id_peer = hdr->ike2_msg_id;
        }
        /* We have all the information needed to process this packet.
         * Now call the processing function to process the informational
         * message.
         */
        status = IKE2_Process_Info_Xchg(handle);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Informational message could not be processed",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* "handle->ike2_params" should preferably be set to NULL here
         * because it points to a local variable and will no longer be
         * a valid pointer when this function returns. Not setting it to
         * NULL does not have any side affects but may cause confusion
         * to someone not aware of this. However, we are not setting it
         * to NULL here because the "handle" may no longer be valid if
         * it was removed as a result of processing the Delete SA payload.
         */
    }

    else
    {
        NLOG_Error_Log("Exchange handle could not be found",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    IKE2_Cleanup_Exchange_Parameters(&params);

    return (status);

} /* IKE2_Dispatch_INFORMATIONAL */

/************************************************************************
*
* FUNCTION
*
*       IKE2_Process_Notify
*
* DESCRIPTION
*
*       This function is called to process the received notify payload.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*       IKE2_INVALID_MSG_TYPE   Message type in notify message is invalid.
*       IKE_INVALID_PROTOCOL    Protocol specified is not valid.
*
************************************************************************/
STATUS IKE2_Process_Notify(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                  status = NU_SUCCESS, loop_status = NU_SUCCESS;
    IKE2_MESSAGE            *in;
    IKE2_STATE_PARAMS       *params;
    IKE2_NOTIFY_PAYLOAD     *temp_notify = NU_NULL;
    IKE2_NOTIFY_PAYLOAD     *del_notify = NU_NULL;

#if (IKE2_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if ((handle == NU_NULL) || (handle->ike2_params == NU_NULL))
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Processing Notify payload");

    /* Set local pointers to commonly used data in the Handle. */
    params = handle->ike2_params;
    in = &params->ike2_in;

    /* Save the first notify so the head can be restored later. */
    temp_notify = in->ike2_notify;

    while ( (in->ike2_notify) && (loop_status == NU_SUCCESS) )
    {
        /* Determine protocol ID in the Notify payload. */
        switch (in->ike2_notify->ike2_protocol_id)
        {
        case IKE2_PROTO_ID_RESERVED:
            break;
        case IKE2_PROTO_ID_IKE:
            if(in->ike2_notify->ike2_spi_size != 0)
            {
                NLOG_Error_Log("Invalid SPI size - Notify payload ignored",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
                status = IKE_INVALID_PROTOCOL;

                /* Break out of this switch only if the condition above failed.
                 * Otherwise fall through and in to the next switch.
                 */
                break;
            }

        case IKE2_PROTO_ID_AH:
        case IKE2_PROTO_ID_ESP:
            /* Nothing to do for these protocol types. */
            break;

        default:

            NLOG_Error_Log(
                "Notify payload has invalid protocol ID - ignored",
                NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Update the status. */
            status = IKE_INVALID_PROTOCOL;
            break;
        }

        /*
         * Check the message type and take appropriate action
         */
        switch(in->ike2_notify->ike2_notify_message_type)
        {
            /* If an error occurred, we need to stop further processing
             * and have to start a new exchange.
             */
        case IKE2_NOTIFY_INVALID_IKE_SPI:
        case IKE2_NOTIFY_INVALID_MAJOR_VERSION:
        case IKE2_NOTIFY_INVALID_SYNTAX:
        case IKE2_NOTIFY_INVALID_MESSAGE_ID:
        case IKE2_NOTIFY_INVALID_SPI:
        case IKE2_NOTIFY_NO_PROPOSAL_CHOSEN:
        case IKE2_NOTIFY_INVALID_KE_PAYLOAD:
        case IKE2_NOTIFY_AUTHENTICATION_FAILED:
        case IKE2_NOTIFY_SINGLE_PAIR_REQUIRED:
        case IKE2_NOTIFY_NO_ADDITIONAL_SAS:
        case IKE2_NOTIFY_INTERNAL_ADDRESS_FAILURE:
        case IKE2_NOTIFY_FAILED_CP_REQUIRED:
        case IKE2_NOTIFY_TS_UNACCEPTABLE:
        case IKE2_NOTIFY_INVALID_SELECTORS:

            loop_status = status = in->ike2_notify->ike2_notify_message_type;

            break;

        case IKE2_NOTIFY_USE_TRANSPORT_MODE:
            if((in->ike2_hdr->ike2_flags & IKE2_HDR_RESPONSE_FLAG) == 0)
            {
                /* We have to use transport mode for the SA negotiation under
                 * processing. We need to send a similar notify message back to
                 * the peer.
                 */
                status = IKE2_Generate_Notify(handle, IKE2_PROTO_ID_RESERVED,
                                              IKE2_NOTIFY_USE_TRANSPORT_MODE,
                                              NU_NULL,  /* No notify data */
                                              0,        /* No notify length */
                                              0,        /* No SPI, zero size */
                                              NU_NULL   /* No SPI */
                                              );
                if(status != NU_SUCCESS)
                {
                	/* Stop processing. */
                	loop_status = status;

                    NLOG_Error_Log("Failed to generate the Notify payload",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            break;

        case IKE2_NOTIFY_REKEY_SA:

            handle->ike2_flags |= IKE2_SA_REKEY;

            break;

        case IKE2_NOTIFY_COOKIE:
    
            /* Save the incoming cookie information to be reflected. */
            status = IKE2_Save_Cookie(handle, in->ike2_notify);
    
            if (status == NU_SUCCESS)
            {
                handle->ike2_flags |= IKE2_USE_INIT_COOKIE;
            }

            else
            {
            	/* Stop processing. */
            	loop_status = status;
            }
    
            break;

        default:
            /* Notify message type is either unsupported/invalid
             * status or error.
             * According to RFC-4306 3.10.1 On receiving unsupported or
             * invalid error that it does not recognize in a response
             * MUST assume that the corresponding request has failed
             * entirely.
             */
            if((in->ike2_notify->ike2_notify_message_type < 16384) &&
                              (in->ike2_hdr->ike2_flags & IKE2_HDR_RESPONSE_FLAG))
            {
                loop_status = status = in->ike2_notify->ike2_notify_message_type;
            }
            break;
        }

        /* Get the next structure. */
        in->ike2_notify = in->ike2_notify->ike2_next;
    }

    /* Restore the original notify. */
    in->ike2_notify = temp_notify;

    /* Notify messages for this packet have been processed. Free any
     * memory used.
     */
    del_notify = in->ike2_notify;

    /* There can be more than one "Delete" messages chained. Loop through
     * all and process all the delete payloads.
     */
    while(del_notify != NU_NULL)
    {
        temp_notify = del_notify->ike2_next;

        if((del_notify->ike2_notify_data != NU_NULL) &&
            (NU_Deallocate_Memory(del_notify->ike2_notify_data)
            != NU_SUCCESS))
        {
            NLOG_Error_Log("Failed to deallocate memory for notify data",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        if(NU_Deallocate_Memory(del_notify) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for notify payload",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        del_notify = temp_notify;
    }

    /* This memory has been freed. Set it to NULL to prevent writes to
     * this location again.
     */
    in->ike2_notify = NU_NULL;

   /* Return the status. */
   return (status);
}

/************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_Notify
*
* DESCRIPTION
*
*       This function is used to send notify payloads to a peer.
*
* INPUTS
*
*       *handle                 Exchange information.
*       protocol_id             Protocol for which notification is being
*                               sent.
*       notify_type             Type of notification.
*       *notify_data            Data to be sent.
*       spi_size                Size of SPI being sent, zero if no SPI.
*       *spi                    SPI for this notification message. SPI is
*                               only specified for IPsec SA and not for
*                               IKE SA.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*       IKE2_INVALID_SPI_SIZE   If size of SPI is greater than maximum size
*
************************************************************************/
STATUS IKE2_Generate_Notify(IKE2_EXCHANGE_HANDLE *handle, UINT8 protocol_id,
                            UINT16 notify_type, VOID* notify_data,
                            UINT16 notify_data_len,
                            UINT8 spi_size, UINT8 *spi)
{
    STATUS                status = NU_SUCCESS;
    IKE2_MESSAGE          *enc_msg;
    IKE2_NOTIFY_PAYLOAD   **notify_payload;

#if (IKE2_DEBUG == NU_TRUE)
    /* Parameter sanity checks. */
    if ( handle == NU_NULL )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Adding Notify payload");

    /* Get a handle to the outgoing data. */
    enc_msg = &handle->ike2_params->ike2_out;
    notify_payload = &enc_msg->ike2_notify;

    /* If first payload then simply allocated memory for it. */
    if(*notify_payload == NU_NULL)
    {
        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                    (VOID **)notify_payload, sizeof(IKE2_NOTIFY_PAYLOAD),
                    NU_NO_SUSPEND);
    }

    else
    {
        /* Otherwise, traverse the Notify payload chain to reach the point
         * to insert new payload.
         */
        while ((*notify_payload)->ike2_next != NU_NULL)
        {
            *notify_payload = (*notify_payload)->ike2_next;
        }

        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                    (VOID **)&((*notify_payload)->ike2_next),
                    sizeof(IKE2_NOTIFY_PAYLOAD), NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Move pointer ahead to point to the newly allocated payload. */
            *notify_payload = (*notify_payload)->ike2_next;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Zero out the freshly allocated buffer. */
        UTL_Zero((VOID*)*notify_payload, sizeof(IKE2_NOTIFY_PAYLOAD));

        /* All done, now copy the values and prepare the payload structure
         * to be added to the chain.
         */
        (*notify_payload)->ike2_notify_message_type = notify_type;
        (*notify_payload)->ike2_protocol_id = protocol_id;
        (*notify_payload)->ike2_spi_size = spi_size;

        if((*notify_payload)->ike2_spi_size > 0)
        {
            NU_BLOCK_COPY((*notify_payload)->ike2_spi, spi, spi_size);
        }

        if(notify_data != NU_NULL)
        {
            (*notify_payload)->ike2_notify_data = notify_data;
            (*notify_payload)->ike2_notify_data_len = notify_data_len;
        }

        /* Fill the IKEv2 Generic header. */
        (*notify_payload)->ike2_gen.ike2_payload_length =
            IKE2_MIN_NOTIFY_PAYLOAD_LEN + (*notify_payload)->ike2_spi_size
            + (*notify_payload)->ike2_notify_data_len;

        (*notify_payload)->ike2_gen.ike2_payload_type =
                                                    IKE2_NOTIFY_PAYLOAD_ID;

        /* Add the Notify Payload to the chain. */
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
                          *notify_payload, IKE2_NOTIFY_PAYLOAD_ID);
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for notify payload",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);
} /* IKE2_Generate_Notify */

/************************************************************************
*
* FUNCTION
*
*       IKE2_Process_Delete
*
* DESCRIPTION
*
*       This function processes the delete payloads. Each exchange handle
*       has an IKE SA and a database of IPsec SA indexes negotiated under
*       it. Any of these SAs (IKE or IPsec) can be deleted through this
*       payload. There is a array of type IPSEC_OUTBOUND_INDEX_REAL that
*       can be used to find and delete IPsec SAs identified by the SPI.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*
************************************************************************/
STATUS IKE2_Process_Delete(IKE2_EXCHANGE_HANDLE *handle)
{
    STATUS                    status = NU_SUCCESS;
    IKE2_MESSAGE              *in;
    IKE2_STATE_PARAMS         *params;
    UINT32                    spi;
    UINT32                    buffer_len;
    INT                       i;
    CHAR                      ips_group_name[IKE_MAX_GROUP_NAME_LEN];
    IPSEC_OUTBOUND_INDEX_REAL ips_index;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if ( handle == NU_NULL )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("Processing Delete payload");

    /* Set local pointers to commonly used data in the Handle. */
    params = handle->ike2_params;
    in = &params->ike2_in;

    /* Determine protocol ID in the delete payload. */
    switch(in->ike2_del->ike2_protocol_id)
    {
    case IKE2_PROTO_ID_IKE:

        /* Make sure there was no SPI specified. */
        if(in->ike2_del->ike2_no_of_spis != 0)
        {
            status = IKE2_INVALID_SPI;

            NLOG_Error_Log("Delete IKE SA payload, no SPI should be specified",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* We have a delete payload for IKE SA deletion. Delete the
             * SA (or replace it if the action is set to replace and new
             * SA is present). Then, send a reply to informational message.
             */
            if(((handle->ike2_sa->ike2_flags & IKE2_SA_REKEY) != 0) &&
                (handle->ike2_new_sa != NU_NULL))
            {
                status = IKE2_Send_Info_Notification(handle,
                            IKE2_PROTO_ID_IKE, 0, IKE2_NOTIFY_RESERVED,
                            NU_NULL, NU_NULL, 0, IKE2_INFORMATIONAL);

                if(status == NU_SUCCESS)
                {
                    /* We have a new SA and the action on old SA is to re-key
                     * it, replace the old SA with the new one.
                     */
                    status = IKE2_Replace_SA(handle, handle->ike2_new_sa);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Re-keyed SA could not be added",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }

                }

                else
                {
                    NLOG_Error_Log("Failed to send informational message \
                                   to peer", NERR_RECOVERABLE, __FILE__,
                                   __LINE__);
                }
            }

            else
            {
                status = IKE2_Send_Info_Notification(handle,
                            IKE2_PROTO_ID_IKE, 0, IKE2_NOTIFY_RESERVED,
                            NU_NULL, NU_NULL, 0, IKE2_INFORMATIONAL);

                if(status == NU_SUCCESS)
                {
                    if(IKE2_Unset_Matching_Timers((UNSIGNED)handle->ike2_sa,
                                                        0,
                                                        TQ_CLEAR_ALL_EXTRA)
                                                        != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to unset IKE2 timers",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                    }


                    /* Similarly, remove all matching events from the
                     * IKE timer list.
                     */
                    IKE_Unset_Matching_Events((UNSIGNED)handle->ike2_sa, 0,
                                                  TQ_CLEAR_ALL_EXTRA);

                    /* Delete the IKE SA and all associated data. */
                    status = IKE2_Delete_IKE_SA(
                                        &params->ike2_policy->ike_sa_list,
                                        handle->ike2_sa);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to delete IKE SA",
                                       NERR_RECOVERABLE, __FILE__,
                                       __LINE__);
                    }
                }
            }
        }
        break;

    case IKE2_PROTO_ID_AH:
    case IKE2_PROTO_ID_ESP:

        /* Make sure SPI length is valid. */
        if( in->ike2_del->ike2_spi_size != IKE_IPS_SPI_LEN )
        {
            status = IKE_INVALID_SPI;

            NLOG_Error_Log("Invalid SPI in incoming message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else if(handle->ike2_params->ike2_in.ike2_hdr->ike2_flags
                    & IKE2_HDR_RESPONSE_FLAG)
        {
            /* We would not do any thing as we have already deleted
             * the SA pair when hard-life timer expired */
            IKE2_DEBUG_LOG("Received Delete payload reply");
        }
        else
        {
            status = IKE2_Send_Info_Delete(handle,
                                           in->ike2_del->ike2_protocol_id,
                                           in->ike2_del->ike2_spi_size,
                                           in->ike2_del->ike2_no_of_spis,
                                           in->ike2_del->ike2_spi_data);

            if(status != NU_NULL)
            {
                NLOG_Error_Log("Failed to send notification to peer",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Set length of the group name buffer. */
            buffer_len = sizeof(ips_group_name);

            /* Lookup the IPsec group name. */
            status = IKE_IPS_Group_Name_By_Packet(params->ike2_packet,
                                                  ips_group_name,
                                                  &buffer_len);

            if(status == NU_SUCCESS)
            {
                /* Process each SPI in delete payload. */
                for(i = 0; i < (INT)in->ike2_del->ike2_no_of_spis; i++)
                {
                    spi = GET32(in->ike2_del->ike2_spi_data, 0);

                    /* Set the outbound SA index. */
                    ips_index.ipsec_group     = ips_group_name;
                    ips_index.ipsec_spi       = spi;
                    ips_index.ipsec_protocol  =
                        IKE_Protocol_ID_IKE_To_IPS(
                            in->ike2_del->ike2_protocol_id);

                    /* Set destination address in SA index. */
                    ips_index.ipsec_dest      =
                        params->ike2_packet->ike_remote_addr.id.is_ip_addrs;

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                    ips_index.ipsec_dest_type =
                        (IPSEC_SINGLE_IP |
                         IKE_IPS_FAMILY_TO_FLAGS(
                         params->ike2_packet->ike_local_addr.
                                 family));
#elif (INCLUDE_IPV4 == NU_TRUE)
                    ips_index.ipsec_dest_type = IPSEC_SINGLE_IP |
                                                IPSEC_IPV4;
#elif (INCLUDE_IPV6 == NU_TRUE)
                    ips_index.ipsec_dest_type = IPSEC_SINGLE_IP |
                                                IPSEC_IPV6;
#endif

                    /* Remove IPsec SA pair based on outbound SA's
                     * SPI.
                     *
                     * NOTE: Most implementations do not send delete
                     * notifications for (our side) inbound SAs. And
                     * not deleting inbound SAs does not cause any
                     * synchronization problems so only outbound SA
                     * deletion requests are serviced. However,
                     * the corresponding inbound SA is also deleted
                     * because IKE assumes that all negotiated SAs
                     * must always exist in pairs.
                     */
                    status = IPSEC_Remove_SA_Pair(&ips_index);

                    if(status == NU_SUCCESS)
                    {
                        IKE2_DEBUG_LOG("Deleted IPsec SA pair on remote request");
                    }

                    else
                    {
                        /* This is not an error so only log it as
                         * a debug message.
                         */
                        IKE2_DEBUG_LOG("Failed to delete IPsec SA on request");
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Unable to get IPsec group by packet",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
        break;

    default:
        NLOG_Error_Log(
            "Delete payload has invalid protocol ID - ignored",
            NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Update the status. */
        status = IKE_INVALID_PROTOCOL;
        break;
    }

    /* Return the status. */
    return (status);
} /* IKE_Process_Delete */

/************************************************************************
*
* FUNCTION
*
*       IKE2_Generate_Delete
*
* DESCRIPTION
*
*       Used to send delete payloads to peer to delete them.
*
* INPUTS
*
*       *handle                 Exchange information.
*       *spi                    SPI of the IPsec SA to delete.
*       spi_len                 Length of the SPI.
*       spi_count               Number of SPI's that we are sending.
*       proto_id                Protocol ID for SA to be deleted.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*
************************************************************************/
STATUS IKE2_Generate_Delete(IKE2_EXCHANGE_HANDLE *handle, UINT8 *spi,
                            UINT8 spi_len, UINT16 spi_count, UINT8 proto_id)
{
    STATUS                status = NU_SUCCESS;
    IKE2_MESSAGE          *enc_msg;
    IKE2_DELETE_PAYLOAD   **del_payload;

#if (IKE2_DEBUG == NU_TRUE)
    /* Parameter sanity checks. */
    if ( handle == NU_NULL )
    {
        return (IKE2_INVALID_PARAMS);
    }
#endif

    IKE2_DEBUG_LOG("Adding Delete payload");

    /* Get a handle to the outgoing data. */
    enc_msg = &handle->ike2_params->ike2_out;
    del_payload = &enc_msg->ike2_del;

    /* If first payload then simply allocated memory for it. */
    if(*del_payload == NU_NULL)
    {
        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
            (VOID **)del_payload, sizeof(IKE2_DELETE_PAYLOAD),
            NU_NO_SUSPEND);
    }

    else
    {
        /* Otherwise, traverse the Delete payload chain to reach the point
         * to insert new payload.
         */
        while ((*del_payload)->ike2_next != NU_NULL)
        {
            *del_payload = (*del_payload)->ike2_next;
        }

        /* Now allocated Memory for the current payload. */
        status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                    (VOID **)&((*del_payload)->ike2_next),
                                    sizeof(IKE2_DELETE_PAYLOAD),
                                    NU_NO_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* Move pointer ahead to point to the newly allocated payload. */
            *del_payload = (*del_payload)->ike2_next;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Zero out the freshly allocated buffer. */
        UTL_Zero((VOID*)*del_payload, sizeof(IKE2_DELETE_PAYLOAD));

        /* All done, now copy the values and prepare the payload structure
         * to be added to the chain.
         */
        (*del_payload)->ike2_no_of_spis = spi_count;
        (*del_payload)->ike2_protocol_id = proto_id;
        (*del_payload)->ike2_spi_size = spi_len;

        if((*del_payload)->ike2_spi_size > 0)
        {
            NU_BLOCK_COPY((*del_payload)->ike2_spi_data, spi, spi_len);
        }

        /* Fill the IKEv2 Generic header. */
        (*del_payload)->ike2_gen.ike2_payload_length =
            IKE2_MIN_DELETE_PAYLOAD_LEN + (*del_payload)->ike2_spi_size;

        (*del_payload)->ike2_gen.ike2_payload_type = IKE2_DELETE_PAYLOAD_ID;

        /* Add the Notify Payload to the chain. */
        IKE2_ADD_TO_CHAIN(handle->ike2_params->ike2_out.ike2_last,
            *del_payload, IKE2_DELETE_PAYLOAD_ID);
    }

    else
    {
        NLOG_Error_Log("Failed to allocate memory for delete payload",
            NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* IKE2_Generate_Delete */

/************************************************************************
*
* FUNCTION
*
*       IKE2_Send_Info_Notification
*
* DESCRIPTION
*
*       This function is used to send an informational packet to the peer.
*
* INPUTS
*
*       *handle                 Exchange information.
*       proto_id                Protocol to which the informational
*                               message applies to.
*       spi_size                Size of SPI to be sent.
*       type_count              Notify type or count of SPI's in delete
*                               message.
*       *spi                    SPI data.
*       *data                   Additional notification data.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*
************************************************************************/
STATUS IKE2_Send_Info_Notification(IKE2_EXCHANGE_HANDLE *handle,
                                   UINT8 proto_id, UINT8 spi_size,
                                   UINT16 type, UINT8 *spi, UINT8 *data,
                                   UINT16 data_len, UINT8 xchg)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_HDR                hdr;

    IKE2_STATE_PARAMS       *params;
    IKE2_MESSAGE            *out_msg;

    params  = handle->ike2_params;
    out_msg = &params->ike2_out;

    /* Initialize hdr to remove benign KW warning. */
    memset(&hdr, 0, sizeof(IKE2_HDR));
    IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr);

    IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr,
                    IKE2_NONE_PAYLOAD_ID);

    switch(type)
    {
    case IKE2_NOTIFY_RESERVED:
        break;
    default:

        status = IKE2_Generate_Notify(handle, proto_id, type, data,
                                      data_len, spi_size, spi);

        if(status != NU_NULL)
        {
            NLOG_Error_Log("Failed to generate notify payload",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        break;
    }

    if(status == NU_SUCCESS)
    {
        IKE2_END_CHAIN(out_msg->ike2_last);

        status = IKE2_Construct_Header(handle, xchg,
                    ((handle->ike2_flags & IKE2_INITIATOR) != 0)?
                    (IKE2_HDR_INITIATOR_FLAG | IKE2_HDR_RESPONSE_FLAG) :
                    IKE2_HDR_RESPONSE_FLAG);

        if(status == NU_SUCCESS)
        {
            if((hdr.ike2_flags & (IKE2_HDR_INITIATOR_FLAG |
                IKE2_HDR_RESPONSE_FLAG)) != 0)
            {
                /* We are initiator but we are sending a response to a
                 * request from responder. Use the same msg ID as peer
                 * did.
                 */
                hdr.ike2_msg_id = params->ike2_in.ike2_hdr->ike2_msg_id;
            }

            /* We will need to encrypt the packet before sending. Set
             * the key to appropriate value.
             */
            if((handle->ike2_flags & IKE2_INITIATOR) == 0)
            {
                /* We are the responder. */
                handle->ike2_sa->ike_encryption_key =
                    handle->ike2_sa->ike2_sk_er;
            }

            else
            {
                /* We are the initiator. */
                handle->ike2_sa->ike_encryption_key =
                    handle->ike2_sa->ike2_sk_ei;
            }

            /* Send the packet to the peer. */
            status = IKE2_Send_Packet(handle);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to send the notify message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

    }

    return (NU_SUCCESS);

} /* IKE2_Send_Info_Notification */

/************************************************************************
*
* FUNCTION
*
*       IKE2_Send_Info_Delete
*
* DESCRIPTION
*
*       This function is used to send an informational packet to the peer.
*
* INPUTS
*
*       *handle                 Exchange information.
*       proto_id                Protocol to which the informational
*                               message applies to.
*       spi_size                Size of SPI to be sent.
*       type_count              Notify type or count of SPI's in delete
*                               message.
*       *spi                    SPI data.
*       *data                   Additional notification data.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*
************************************************************************/
STATUS IKE2_Send_Info_Delete(IKE2_EXCHANGE_HANDLE *handle, UINT8 proto_id,
                             UINT8 spi_size, UINT16 spi_count, UINT8 *spi)
{
    STATUS                  status = NU_SUCCESS;
    IKE2_HDR                hdr;

    IKE2_STATE_PARAMS       *params;
    IKE2_MESSAGE            *out_msg;

    UINT8                   flags = 0;

    if(handle->ike2_params == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }

    params  = handle->ike2_params;
    out_msg = &params->ike2_out;

    IKE2_SET_OUTBOUND(out_msg->ike2_hdr, &hdr);

    IKE2_INIT_CHAIN(out_msg->ike2_last, out_msg->ike2_hdr,
        IKE2_NONE_PAYLOAD_ID);

    switch(proto_id)
    {
    case IKE2_PROTO_ID_IKE:

        status = IKE2_Generate_Delete(handle, NU_NULL, 0, 0, proto_id);

        if(status != NU_NULL)
        {
            NLOG_Error_Log("Failed to generate notify payload",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        break;

    case IKE2_PROTO_ID_AH:
    case IKE2_PROTO_ID_ESP:

        status = IKE2_Generate_Delete(handle, spi, spi_size, spi_count,
                                      proto_id);

        if(status != NU_NULL)
        {
            NLOG_Error_Log("Failed to generate notify payload",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        break;

    default:
        break;
    }

    if(status == NU_SUCCESS)
    {
        IKE2_END_CHAIN(out_msg->ike2_last);

        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            flags |= IKE2_HDR_INITIATOR_FLAG;
        }

        if((handle->ike2_params->ike2_in.ike2_hdr) &&
            (handle->ike2_params->ike2_in.ike2_hdr->ike2_flags &
            IKE2_HDR_RESPONSE_FLAG) == 0)
        {
            flags |= IKE2_HDR_RESPONSE_FLAG;
        }

        status = IKE2_Construct_Header(handle, IKE2_INFORMATIONAL,flags);

        if(status == NU_SUCCESS)
        {

            /* We will need to encrypt the packet before sending. Set
             * the key to appropriate value.
             */
            if((handle->ike2_flags & IKE2_INITIATOR) == 0)
            {
                /* We are the responder. */
                handle->ike2_sa->ike_encryption_key =
                    handle->ike2_sa->ike2_sk_er;
            }

            else
            {
                /* We are the initiator. */
                handle->ike2_sa->ike_encryption_key =
                    handle->ike2_sa->ike2_sk_ei;
            }

            /* Send the packet to the peer. */
            status = IKE2_Send_Packet(handle);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to send the notify message",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

    }

    return (NU_SUCCESS);

} /* IKE2_Send_Info_Delete */

/************************************************************************
*
* FUNCTION
*
*       IKE2_Process_Info_Xchg
*
* DESCRIPTION
*
*       The processing of Informational exchange is offloaded to this
*       function. It is responsible for parsing and responding to the
*       informational messages received.
*
* INPUTS
*
*       *handle                 Exchange information.
*
* OUTPUTS
*
*       NU_SUCCESS              Processing was successful.
*       IKE2_INVALID_PARAMS     Parameters are invalid.
*       IKE_SA_NOT_FOUND        SA not present in the exchange handle.
*       IKE2_UNEXPECTED_PAYLOAD Encrypted payload not found, illegal in
*                               informational exchange.
*
************************************************************************/
STATUS IKE2_Process_Info_Xchg(IKE2_EXCHANGE_HANDLE *handle)
{
    IKE2_STATE_PARAMS       *params;
    IKE2_MESSAGE            *in_msg;
    IKE2_VENDOR_ID_PAYLOAD  vid_peer;
    STATUS                  status = NU_SUCCESS;


#if(IKE2_DEBUG == NU_TRUE)

    if(handle == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_params == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }
    else if(handle->ike2_sa == NU_NULL)
    {
        return (IKE_SA_NOT_FOUND);
    }
    else if(handle->ike2_params->ike2_policy == NU_NULL)
    {
        return (IKE2_INVALID_PARAMS);
    }

#endif

    /* Log debug message. */
    IKE2_DEBUG_LOG("INFORMATIONAL MODE");

    params = handle->ike2_params;
    in_msg = &params->ike2_in;

    /* If we have an encrypted payload and an SA has been established,
     * decrypt the payload.
     */
    if (( params->ike2_in.ike2_hdr->ike2_next_payload ==
         IKE2_ENCRYPT_PAYLOAD_ID ) && ( handle->ike2_sa ))
    {
        /* Set which key to use to decrypt the packet. */
        if((handle->ike2_flags & IKE2_INITIATOR) != 0)
        {
            /* We are the initiator. Use responder key to decrypt. */
            handle->ike2_sa->ike_encryption_key =
                handle->ike2_sa->ike2_sk_er;
        }

        else
        {
            /* We are the responder. Use initiator key to decrypt. */
            handle->ike2_sa->ike_encryption_key =
                handle->ike2_sa->ike2_sk_ei;
        }

        status = IKE2_Decode_IKE_Encrypted_Payload(
                    params->ike2_packet->ike_data,
                    params->ike2_packet->ike_data_len, handle->ike2_sa);

        if(status == NU_SUCCESS)
        {
            /* The total length and next payload ID have been updated
             * in the buffer, update them in the IKEv2 header as well.
             * (This header was already decoded and needs to be updated
             * separately)
             */
            in_msg->ike2_hdr->ike2_length = GET32(params->ike2_packet->ike_data,
                                                  IKE2_HDR_LEN_OFST);

            in_msg->ike2_hdr->ike2_next_payload = GET8(params->ike2_packet->ike_data,
                                                       IKE2_HDR_NXT_PYLD_OFST);

            /* Check if incoming Informaional packet is a Liveness check.
             * Its payload should be of zero length. Subtract header
             * length from total length to get the total payload length
             */
            if(((in_msg->ike2_hdr->ike2_length - IKE2_HDR_TOTAL_LEN) == 0) &&
                (in_msg->ike2_hdr->ike2_next_payload == IKE2_NONE_PAYLOAD_ID))
            {
                /* Yes it is, so let's reply back Liveness check to the peer
                 * node saying we are alive.
                 */
                status = IKE2_Send_Info_Notification(handle, IKE2_PROTO_ID_RESERVED,
                                                   0,       /* No SPI, zero size */
                                                   IKE2_NONE_PAYLOAD_ID,
                                                   NU_NULL, /* No SPI */
                                                   NU_NULL, /* No notify data */
                                                   0,       /* No notify data length */
                                                   IKE2_INFORMATIONAL);
               if(status != NU_SUCCESS)
               {
                   NLOG_Error_Log("Failed to send Liveness information packet",
                                 NERR_RECOVERABLE, __FILE__, __LINE__);
               }
            }
        }
    }

    /* We cannot do unencrypted INFORMATIONAL exchanges.
     */
    else
    {
        status = IKE2_UNEXPECTED_PAYLOAD;
    }

    /* If the informational.
     */
    if(status == NU_SUCCESS)
    {
        IKE2_SET_INBOUND_OPTIONAL(params->ike2_in.ike2_vid, &vid_peer);

        /* Messages has be decrypted. Now decode the payloads out of it. */
        status = IKE2_Decode_Message(params->ike2_packet->ike_data,
                                     &params->ike2_in );

        /* All payloads have been successfully decoded. */
        if(status == NU_SUCCESS)
        {
            if(params->ike2_in.ike2_hdr->ike2_next_payload ==
                IKE2_NONE_PAYLOAD_ID)
            {
                /* SA was waiting to be deleted or replaced. */
                if((handle->ike2_sa->ike2_flags & IKE2_WAIT_DELETE) != 0)
                {
                    /* Clear the wait deletion flag. */
                    handle->ike2_sa->ike2_flags &= ~IKE2_WAIT_DELETE;

                    if((handle->ike2_sa->ike2_flags & IKE2_SA_REKEY) != 0)
                    {
                        status = IKE2_Replace_SA(handle, handle->ike2_new_sa);

                        if(status == NU_SUCCESS)
                        {
                            /* We re-keyed IKE SA. Set the timer for this
                             * new SA to be re-keyed as well.
                             */
                            status = IKE_Set_Timer(IKE2_SA_Timeout_Event,
                                        (UNSIGNED)handle->ike2_sa,
                                        (UNSIGNED)&params->ike2_policy->ike_sa_list,
                                        handle->ike2_sa->ike_attributes.
                                                ike_sa_lifetime.ike_no_of_secs);

                            if(status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to reset SA timeout timer",
                                               NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to replace SA",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        status = IKE2_Delete_IKE_SA(
                            &params->ike2_policy->ike_sa_list,
                            handle->ike2_sa);

                        if(status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to replace SA",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }
                    }
                }
            }

            else
            {
                /* Process the notify payloads. */
                if(params->ike2_in.ike2_notify)
                {
                    status = IKE2_Process_Notify( handle );
                }

                /* Process the delete payloads. */
                if (( status == NU_SUCCESS ) && params->ike2_in.ike2_del)
                {
                    status = IKE2_Process_Delete(handle);
                }
            }
        }
        else
        {
            NLOG_Error_Log("Failed to decode IKE Informational message",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to decrypt IKE Informational message",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE2_Process_Info_Xchg */

#endif

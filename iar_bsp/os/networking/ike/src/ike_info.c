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
*       ike_info.c
*
* COMPONENT
*
*       IKE - Informational Mode
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE
*       Informational Mode Exchange.
*
* DATA STRUCTURES
*
*       IKE_Notify_Errors       Mapping of internal status codes
*                               to IKE notification error types.
*
* FUNCTIONS
*
*       IKE_Send_Informational
*       IKE_Notify_Error
*       IKE_Delete_Notification
*       IKE_Check_Initial_Contact
*       IKE_Process_Initial_Contact
*       IKE_Process_Delete
*       IKE_Process_Notify
*       IKE_Process_Info_Mode
*
* DEPENDENCIES
*
*       nu_net.h
*       md4.h
*       md5.h
*       sha.h
*       des.h
*       blowfish.h
*       cast.h
*       aes.h
*       evp.h
*       ips_api.h
*       ike_api.h
*       ike_pkt.h
*       ike_enc.h
*       ike_oak.h
*       ike_ips.h
*       ike_evt.h
*       ike_crypto_wrappers.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "openssl/md4.h"
#include "openssl/md5.h"
#include "openssl/sha.h"
#include "openssl/des.h"
#include "openssl/blowfish.h"
#include "openssl/cast.h"
#include "openssl/aes.h"
#include "openssl/evp.h"
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_pkt.h"
#include "networking/ike_enc.h"
#include "networking/ike_oak.h"
#include "networking/ike_ips.h"
#include "networking/ike_evt.h"
#include "networking/ike_crypto_wrappers.h"

#if (IKE_INCLUDE_INFO_MODE == NU_TRUE)

/* Mapping internal status codes to the IKE Notification
 * error types. This mapping starts from the internal
 * error equal to the value of IKE_NOTIFY_ERROR_MAX.
 */
static UINT16 IKE_Notify_Errors[IKE_TOTAL_NOTIFY_ERRORS] =
{
    IKE_NOTIFY_UNEQUAL_PLOAD_LEN,           /* IKE_UNEQUAL_PLOAD_LEN */
    IKE_NOTIFY_INVALID_COOKIE,              /* IKE_INVALID_COOKIE */
    IKE_NOTIFY_INVALID_MJR_VER,             /* IKE_INVALID_MJR_VER */
    IKE_NOTIFY_INVALID_MNR_VER,             /* IKE_INVALID_MNR_VER */
    IKE_NOTIFY_INVALID_PLOAD_TYPE,          /* IKE_INVALID_PLOAD_TYPE */
    IKE_NOTIFY_INVALID_XCHG_TYPE,           /* IKE_INVALID_XCHG_TYPE */
    IKE_NOTIFY_INVALID_FLAGS,               /* IKE_INVALID_FLAGS */
    IKE_NOTIFY_INVALID_MSGID,               /* IKE_INVALID_MSGID */
    IKE_NOTIFY_PLOAD_MALFORMED,             /* IKE_INVALID_PAYLOAD */
    IKE_NOTIFY_PLOAD_MALFORMED,             /* IKE_DUPLICATE_PAYLOAD */
    IKE_NOTIFY_PLOAD_MALFORMED,             /* IKE_TOO_MANY_TRANSFORMS */
    IKE_NOTIFY_PLOAD_MALFORMED,             /* IKE_TOO_MANY_PROPOSALS */
    IKE_NOTIFY_INVALID_COOKIE,              /* IKE_SA_NOT_FOUND */
    IKE_NOTIFY_ATTRIB_UNSUPPORTED,          /* IKE_ATTRIB_TOO_LONG */
    IKE_NOTIFY_INVALID_SPI,                 /* IKE_INVALID_SPI */
    IKE_NOTIFY_INVALID_PROTID,              /* IKE_INVALID_PROTOCOL */
    IKE_NOTIFY_INVALID_TRANSID,             /* IKE_INVALID_TRANSFORM */
    IKE_NOTIFY_NO_PROP_CHOSEN,              /* IKE_INVALID_KEYLEN */
    IKE_NOTIFY_BAD_PROP_SYNTAX,             /* IKE_INVALID_PROPOSAL */
    IKE_NOTIFY_DOI_UNSUPPORTED,             /* IKE_UNSUPPORTED_DOI */
    IKE_NOTIFY_NO_PROP_CHOSEN,              /* IKE_UNSUPPORTED_ALGO */
    IKE_NOTIFY_ATTRIB_UNSUPPORTED,          /* IKE_UNSUPPORTED_ATTRIB */
    IKE_NOTIFY_BAD_PROP_SYNTAX,             /* IKE_MISSING_ATTRIB */
    IKE_NOTIFY_INVALID_PLOAD_TYPE,          /* IKE_MISSING_PAYLOAD */
    IKE_NOTIFY_INVALID_PLOAD_TYPE,          /* IKE_UNEXPECTED_PAYLOAD */
    IKE_NOTIFY_NO_PROP_CHOSEN,              /* IKE_NOT_NEGOTIABLE */
    IKE_NOTIFY_PLOAD_MALFORMED,             /* IKE_PROPOSAL_TAMPERED */
    IKE_NOTIFY_AUTH_FAILED,                 /* IKE_AUTH_FAILED */
    IKE_NOTIFY_AUTH_FAILED,                 /* IKE_VERIFY_FAILED */
    IKE_NOTIFY_AUTH_FAILED,                 /* IKE_ID_MISMATCH */
    IKE_NOTIFY_SITU_UNSUPPORTED,            /* IKE_UNSUPPORTED_SITU */
    IKE_NOTIFY_INVALID_ID_INFO,             /* IKE_INVALID_ID */
};

/*************************************************************************
*
* FUNCTION
*
*       IKE_Send_Informational
*
* DESCRIPTION
*
*       This function sends an Informational mode message to
*       the destination (specified in the IKE SA). The message
*       is encrypted if key material is available, otherwise
*       an un-encrypted message is sent. The payload provided
*       to this function must specify the payload type in the
*       generic payload header.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       *enc_pload              Either a Delete or a
*                               Notification payload.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_INVALID_COOKIE      Initiator cookie not set in SA.
*       IKE_CRYPTO_ERROR        Crypto related error.
*
*************************************************************************/
STATUS IKE_Send_Informational(IKE_SA *sa, IKE_GEN_HDR *enc_pload)
{
    STATUS                  status;
    IKE_ENC_HDR             enc_hdr;
    IKE_HASH_ENC_PAYLOAD    enc_hash;
    IKE_STATE_PARAMS        params;
    UINT8                   enc_pload_type;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if((sa == NU_NULL) || (enc_pload == NU_NULL))
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Make sure SA contains Initiator's cookie. */
    if(!IKE_COOKIE_IS_SET(sa->ike_cookies))
    {
        /* Cookie is invalid. */
        status = IKE_INVALID_COOKIE;
    }

    else
    {
        /* Get user payload type. */
        enc_pload_type = enc_pload->ike_type;

        /* Zero out the state parameters. */
        UTL_Zero(&params, sizeof(IKE_STATE_PARAMS));

        /* Set outbound payloads used in this state. Assume
         * that passed payload is a Notification. It would
         * not matter if its a Delete payload because this
         * payload is already initialized and need not
         * be accessed.
         */
        IKE_SET_OUTBOUND(params.ike_out.ike_hdr, &enc_hdr);
        IKE_SET_OUTBOUND(params.ike_out.ike_hash, &enc_hash);
        IKE_SET_OUTBOUND(params.ike_out.ike_notify,
                         ((IKE_NOTIFY_ENC_PAYLOAD*)&enc_pload));

        /* Initialize chain of outbound payloads. */
        IKE_INIT_CHAIN(params.ike_out.ike_last, &enc_hdr,
                       IKE_NONE_PAYLOAD_ID);


        /* Initialize the Phase 2 Handle. Note that this sets
         * the current state equivalent to IKE_SEND_HASH1_STATE
         * so the correct hash would be calculated by the packet
         * send routine.
         */
        status = IKE_New_Phase2(&IKE_Large_Data.ike_phase2, sa, &params);

        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to create local Phase 2 Handle",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* If the IKE SA has been established. */
        else if(sa->ike_state == IKE_SA_ESTABLISHED)
        {
            /* Generate Phase 2 IV for encryption. */
            status = IKE_Phase2_IV(&IKE_Large_Data.ike_phase2);

            if(status == NU_SUCCESS)
            {
                /* Get hash digest length. */
                status = IKE_Crypto_Digest_Len(IKE_Hash_Algos[sa->ike_attributes.ike_hash_algo].crypto_algo_id,
                                               &(enc_hash.ike_hash_data_len));

                if(status == NU_SUCCESS)
                {
                    /* Add Hash payload to the payloads chain. */
                    IKE_ADD_TO_CHAIN(params.ike_out.ike_last,
                                     &enc_hash, IKE_HASH_PAYLOAD_ID);

                    /* Add user specified payload to the payloads chain. */
                    IKE_ADD_TO_CHAIN(params.ike_out.ike_last,
                                     enc_pload, enc_pload_type);

                    /* Terminate chain of payloads. */
                    IKE_END_CHAIN(params.ike_out.ike_last);

                    /* Initialize ISAKMP header. */
                    IKE_Set_Header(&enc_hdr,
                                   IKE_Large_Data.ike_phase2.ike_msg_id,
                                   sa->ike_cookies, IKE_XCHG_INFO,
                                   IKE_HDR_ENC_MASK);

                    /* Send the message. */
                    status = IKE_Send_Phase2_Packet(
                                 &IKE_Large_Data.ike_phase2);
                }

                else
                {
                    NLOG_Error_Log("Failed to find hash digest length",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to generate Phase 2 encryption IV",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Otherwise do the exchange in the open without
         * an accompanying HASH payload.
         */
        else
        {
            /* Add user specified payload to the payloads chain. */
            IKE_ADD_TO_CHAIN(params.ike_out.ike_last,
                             enc_pload, enc_pload_type);

            /* Terminate chain of payloads. */
            IKE_END_CHAIN(params.ike_out.ike_last);

            /* Initialize ISAKMP header. */
            IKE_Set_Header(&enc_hdr, 0, sa->ike_cookies,
                           IKE_XCHG_INFO, 0);

            /* Send the message. */
            status = IKE_Send_Phase2_Packet(&IKE_Large_Data.ike_phase2);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Send_Informational */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Notify_Error
*
* DESCRIPTION
*
*       This function notifies the remote node if an error
*       occurs during the exchange. Notifications are sent
*       only for IKE errors which map to notification error
*       types.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA.
*       error_status            The error status.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_INVALID_COOKIE      Initiator cookie not set in SA.
*       IKE_INTERNAL_ERROR      Error is internal so cannot be
*                               notified.
*
*************************************************************************/
STATUS IKE_Notify_Error(IKE_SA *sa, STATUS error_status)
{
    STATUS                  status;
    INT                     error_index;
    IKE_NOTIFY_ENC_PAYLOAD  enc_notify;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Check whether the error status maps to
     * a notification error type.
     */
    if(IKE_IS_NOTIFY_ERROR(error_status))
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Sending error notification using Info. mode");

        /* Calculate index to the error mapping array. */
        error_index = (INT)(-error_status) + IKE_NOTIFY_ERROR_MAX;

        /* Set Notification payload type. */
        IKE_SET_PAYLOAD_TYPE(&enc_notify, IKE_NOTIFY_PAYLOAD_ID);

        /* Create the notify payload. */
        enc_notify.ike_doi              = IKE_DOI_IPSEC;
        enc_notify.ike_notify_data_len  = 0;
        enc_notify.ike_protocol_id      = IKE_PROTO_ISAKMP;
        enc_notify.ike_spi_len          = 0;
        enc_notify.ike_notify_type      = IKE_Notify_Errors[error_index];

        /* Send the notification message. */
        status = IKE_Send_Informational(sa, (IKE_GEN_HDR*)&enc_notify);
    }

    else
    {
        /* Internal error so cannot be notified. */
        status = IKE_INTERNAL_ERROR;
    }

    /* Return the status. */
    return (status);

} /* IKE_Notify_Error */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Delete_Notification
*
* DESCRIPTION
*
*       This function notifies the remote node that an SA
*       has been deleted. This allows synchronization between
*       the two nodes. The SA being deleted could be either an
*       IKE SA or an IPsec SA.
*
* INPUTS
*
*       *sa                     Pointer to the IKE SA which is
*                               used to protect the informational
*                               notification.
*       proto                   Protocol of the SA being deleted.
*                               If this is IKE_PROTO_ISAKMP, then
*                               the notification is sent for the
*                               IKE SA passed in the 'sa' parameter.
*       *spi                    IPsec SA SPI. This is unused when
*                               an IKE SA is deleted.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_INVALID_COOKIE      Initiator cookie not set in SA.
*
*************************************************************************/
STATUS IKE_Delete_Notification(IKE_SA *sa, UINT8 proto, UINT8 *spi)
{
    STATUS                  status;
    IKE_DELETE_ENC_PAYLOAD  enc_del;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(sa == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set Delete payload type. */
    IKE_SET_PAYLOAD_TYPE(&enc_del, IKE_DELETE_PAYLOAD_ID);

    /* Create the delete payload. */
    enc_del.ike_doi              = IKE_DOI_IPSEC;
    enc_del.ike_num_spi          = 1;
    enc_del.ike_protocol_id      = proto;

    /* If notification is for an IKE SA. */
    if(proto == IKE_PROTO_ISAKMP)
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Sending delete notification for an IKE SA");

        /* Set SPI length. */
        enc_del.ike_spi_len = IKE_COOKIE_LEN * 2;

        /* Copy the cookies to the SPI field. */
        NU_BLOCK_COPY(enc_del.ike_spis[0], sa->ike_cookies,
                      IKE_COOKIE_LEN * 2);
    }

    else
    {
        /* Log debug message. */
        IKE_DEBUG_LOG("Sending delete notification for an IPsec SA");

        /* Set SPI length. */
        enc_del.ike_spi_len = IKE_IPS_SPI_LEN;

        /* Set SPI in the payload. */
        NU_BLOCK_COPY(enc_del.ike_spis[0], spi, IKE_IPS_SPI_LEN);
    }

    /* Send the notification message. */
    status = IKE_Send_Informational(sa, (IKE_GEN_HDR*)&enc_del);

    /* Return the status. */
    return (status);

} /* IKE_Delete_Notification */

#if (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Check_Initial_Contact
*
* DESCRIPTION
*
*       This function checks whether the specified IKE SA is
*       the first SA being established with the remote node.
*       If so, it sends an INITIAL-CONTACT notification to
*       the remote node.
*
* INPUTS
*
*       *ph1                    Pointer to the Phase 1 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IPSEC_ALREADY_EXISTS    If not an initial contact.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NO_MEMORY           Not enough memory to send packet.
*       IKE_INVALID_PAYLOAD     One or more payloads are invalid.
*       IKE_LENGTH_IS_SHORT     Message exceeds maximum size
*                               expected.
*       IKE_INVALID_COOKIE      Initiator cookie not set in SA.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*
*************************************************************************/
STATUS IKE_Check_Initial_Contact(IKE_PHASE1_HANDLE *ph1)
{
    STATUS                  status;
    IKE_STATE_PARAMS        *params;
    IKE_NOTIFY_ENC_PAYLOAD  enc_notify;
    IPSEC_SINGLE_IP_ADDR    remote_addr;
    CHAR                    group_name[IKE_MAX_GROUP_NAME_LEN];
    UINT32                  group_name_len = IKE_MAX_GROUP_NAME_LEN;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all parameters are valid. */
    if(ph1 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set pointer to state parameters. */
    params = ph1->ike_params;

    /* Set remote address in IPsec address structure. */
    remote_addr.ipsec_addr = ph1->ike_sa->ike_node_addr.id.is_ip_addrs;
    remote_addr.ipsec_type =
        IKE_IPS_FAMILY_TO_FLAGS(ph1->ike_sa->ike_node_addr.family);

    /* Look-up the IPsec group name. */
    status = IKE_IPS_Group_Name_By_Packet(params->ike_packet, group_name,
                                          &group_name_len);

    if(status == NU_SUCCESS)
    {
        /* Determine whether there are any IPsec SAs with
         * the specified remote node.
         */
        status = IPSEC_Check_Initial_Contact(group_name, &remote_addr);

        /* If no SA found with the remote node. */
        if(status == NU_SUCCESS)
        {
            /* Set Notification payload type. */
            IKE_SET_PAYLOAD_TYPE(&enc_notify, IKE_NOTIFY_PAYLOAD_ID);

            /* Create the notification payload. */
            enc_notify.ike_doi         = IKE_DOI_IPSEC;
            enc_notify.ike_notify_data_len = 0;
            enc_notify.ike_protocol_id = IKE_PROTO_ISAKMP;
            enc_notify.ike_spi_len     = IKE_COOKIE_LEN * IKE_NUM_ENTITIES;
            enc_notify.ike_notify_type = IKE_IPS_NOTIFY_INIT_CONTACT;

            /* Copy SPI to the notification payload. */
            NU_BLOCK_COPY(enc_notify.ike_spi, ph1->ike_sa->ike_cookies,
                          IKE_COOKIE_LEN * IKE_NUM_ENTITIES);

            /* Send the notification message. */
            status = IKE_Send_Informational(ph1->ike_sa,
                                            (IKE_GEN_HDR*)&enc_notify);
        }
    }

    else
    {
        NLOG_Error_Log("Unable to get IPsec group name based on packet",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Check_Initial_Contact */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Initial_Contact
*
* DESCRIPTION
*
*       This function processes the INITIAL-CONTACT notification.
*       It deletes all established IKE and IPsec SAs which have
*       the same remote node address as that of the specified
*       IKE SA.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         No previous SAs are established
*                               with the remote node.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_STATE       IKE SA is not established.
*       IKE_INVALID_PROTOCOL    Protocol is not ISAKMP.
*       IKE_INVALID_SPI         SPI length is not valid.
*       IKE_INVALID_COOKIE      Cookies do not match.
*
*************************************************************************/
STATUS IKE_Process_Initial_Contact(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status;
    IKE_STATE_PARAMS        *params;
    IKE_NOTIFY_DEC_PAYLOAD  *notify;
    IKE_POLICY              *policy;
    IPSEC_SINGLE_IP_ADDR    remote_addr;
    IPSEC_SINGLE_IP_ADDR    local_addr;
    CHAR                    group_name[IKE_MAX_GROUP_NAME_LEN];
    UINT32                  group_name_len = IKE_MAX_GROUP_NAME_LEN;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Set local pointers to commonly used data in the Handle. */
    params = ph2->ike_params;
    notify = ph2->ike_params->ike_in.ike_notify;

    /* Make sure this message was sent after phase 1 completion. */
    if(ph2->ike_sa->ike_state != IKE_SA_ESTABLISHED)
    {
        status = IKE_INVALID_STATE;
    }

    /* Make sure protocol is ISAKMP. */
    else if(notify->ike_protocol_id != IKE_PROTO_ISAKMP)
    {
        status = IKE_INVALID_PROTOCOL;
    }

    /* Make sure SPI length is valid. */
    else if(notify->ike_spi_len != IKE_COOKIE_LEN * IKE_NUM_ENTITIES)
    {
        /* SPI length is not valid. */
        status = IKE_INVALID_SPI;
    }

    /* Make sure the SA protecting this exchange is the
     * one specified in the notify payload.
     */
    else if(memcmp(ph2->ike_sa->ike_cookies, notify->ike_spi,
                   IKE_COOKIE_LEN * IKE_NUM_ENTITIES) != 0)
    {
        /* Cookies do not match. */
        status = IKE_INVALID_COOKIE;
    }

    else
    {
        /* Log the event. */
        IKE_DEBUG_LOG("INITIAL-CONTACT received");

        /*
         * Delete all established IKE SAs with remote node.
         * The only IKE SAs searched are those negotiated
         * under the IKE policy used by the current
         * IKE exchange. There are little chances of finding
         * SAs with the remote node under other policies.
         */

        /* Set policy to the first policy in the IKE group. */
        policy = params->ike_group->ike_policy_list.ike_flink;

        /* Loop for all policies in this group. */
        while(policy != NU_NULL)
        {
            /* Delete all SAs which match the remote address
             * of the sending node. Return value of this
             * function can be ignored.
             */
            if(IKE_Sync_Remove_SAs(&policy->ike_sa_list,
                                   &ph2->ike_sa->ike_node_addr,
                                   IKE_MATCH_SA_IP, ph2->ike_sa)
                                   == NU_SUCCESS)
            {
                IKE_DEBUG_LOG("Removed IKE SAs due to INITIAL-CONTACT");
            }

            /* Move to the next policy in the list. */
            policy = policy->ike_flink;
        }

        /*
         * Delete all established IPsec SAs with remote node.
         * Both end points of the SA are compared, to avoid
         * deletion of valid SAs if multiple interfaces
         * exist on the local machine.
         */

        /* Set local address in IPsec address structure. */
        local_addr.ipsec_addr =
            params->ike_packet->ike_local_addr.id.is_ip_addrs;
        local_addr.ipsec_type =
            IKE_IPS_FAMILY_TO_FLAGS(
                params->ike_packet->ike_local_addr.family);

        /* Set remote address in IPsec address structure. */
        remote_addr.ipsec_addr =
            params->ike_packet->ike_remote_addr.id.is_ip_addrs;
        remote_addr.ipsec_type =
            IKE_IPS_FAMILY_TO_FLAGS(
                params->ike_packet->ike_remote_addr.family);

        /* Look-up the IPsec group name. */
        status = IKE_IPS_Group_Name_By_Packet(params->ike_packet,
                                              group_name,
                                              &group_name_len);

        if(status == NU_SUCCESS)
        {
            /* Delete all SAs with the specified local and remote
             * addresses.
             */
            status = IPSEC_Remove_SAs_By_Addr(group_name, &local_addr,
                                              &remote_addr);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_Initial_Contact */
#endif /* (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Delete
*
* DESCRIPTION
*
*       This function handles incoming Delete payloads.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         IPsec group was not found or
*                               SA to be deleted was not found.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_SPI         SPI length is not valid.
*       IKE_NOT_FOUND           Device not found or it does
*                               not contain an IPsec group.
*
*************************************************************************/
STATUS IKE_Process_Delete(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                    status = NU_SUCCESS;
    IKE_DEC_MESSAGE           *in;
    IKE_STATE_PARAMS          *params;
    IKE_SA                    *del_sa;
    UINT32                    spi;
    UINT32                    buffer_len;
    INT                       i;
    CHAR                      ips_group_name[IKE_MAX_GROUP_NAME_LEN];
    IPSEC_OUTBOUND_INDEX_REAL ips_index;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Processing Delete payload");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    params = ph2->ike_params;

    /* Make sure the deletion request is encrypted. */
    if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) == 0)
    {
        /* Report invalid flags. */
        status = IKE_INVALID_FLAGS;
    }

    else
    {
        /* Determine protocol ID in the delete payload. */
        switch(in->ike_del->ike_protocol_id)
        {
        case IKE_PROTO_ISAKMP:
            /* ISAKMP SPI must be Initiator and Responder cookies. */
            if(in->ike_del->ike_spi_len !=
               (IKE_COOKIE_LEN * IKE_NUM_ENTITIES))
            {
                status = IKE_INVALID_SPI;

                NLOG_Error_Log("Invalid SPI in incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Process each SPI in delete payload. */
                for(i = 0; i < (INT)in->ike_del->ike_num_spi; i++)
                {
                    /* Look for an SA with this SPI. */
                    if(IKE_Get_SA(&params->ike_policy->ike_sa_list,
                                  in->ike_del->ike_spis[i],
                                  IKE_MATCH_SA_COOKIE,
                                  &del_sa) == NU_SUCCESS)
                    {
                        /* If the Handle pointer is valid. */
                        if(del_sa->ike_phase1 != NU_NULL)
                        {
                            /* Mark the Handle as deleted. */
                            del_sa->ike_phase1->ike_flags |=
                                IKE_DELETE_FLAG;
                        }

                        /* Enqueue event to remove the IKE SA. The SA
                         * is removed through the event queue to keep
                         * all events synchronized.
                         */
                        IKE_SYNC_REMOVE_SA(
                            &params->ike_policy->ike_sa_list, del_sa);

                        /* Log the event. */
                        IKE_DEBUG_LOG(
                            "Forced IKE SA timeout on remote request");
                    }
                }
            }
            break;

        case IKE_PROTO_AH:
        case IKE_PROTO_ESP:
            /* Make sure SPI length is valid. */
            if(in->ike_del->ike_spi_len != IKE_IPS_SPI_LEN)
            {
                status = IKE_INVALID_SPI;

                NLOG_Error_Log("Invalid SPI in incoming message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            else
            {
                /* Set length of the group name buffer. */
                buffer_len = sizeof(ips_group_name);

                /* Lookup the IPsec group name. */
                status = IKE_IPS_Group_Name_By_Packet(params->ike_packet,
                                                      ips_group_name,
                                                      &buffer_len);

                if(status == NU_SUCCESS)
                {
                    /* Process each SPI in delete payload. */
                    for(i = 0; i < (INT)in->ike_del->ike_num_spi; i++)
                    {
                        /* Copy SPI to a UINT32 variable. */
                        spi = GET32(in->ike_del->ike_spis[i], 0);

                        /* Set the outbound SA index. */
                        ips_index.ipsec_group     = ips_group_name;
                        ips_index.ipsec_spi       = spi;
                        ips_index.ipsec_protocol  =
                            IKE_Protocol_ID_IKE_To_IPS(
                                in->ike_del->ike_protocol_id);

                        /* Set destination address in SA index. */
                        ips_index.ipsec_dest      =
                            params->ike_packet->ike_remote_addr.
                                id.is_ip_addrs;

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                        ips_index.ipsec_dest_type =
                            (IPSEC_SINGLE_IP |
                             IKE_IPS_FAMILY_TO_FLAGS(
                                 params->ike_packet->ike_local_addr.
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
                            IKE_DEBUG_LOG(
                                "Deleted IPsec SA pair on remote request");
                        }

                        else
                        {
                            /* This is not an error so only log it as
                             * a debug message.
                             */
                            IKE_DEBUG_LOG(
                                "Failed to delete IPsec SA on request");
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
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_Delete */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Notify
*
* DESCRIPTION
*
*       This function handles incoming Notification payloads.
*
* INPUTS
*
*       *ph2                    Pointer to the Phase 2 Handle.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IPSEC_NOT_FOUND         No previous SAs are established
*                               with the remote node.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_UNSUPPORTED_NOTIFY  Unsupported notification type.
*       IKE_INVALID_STATE       IKE SA is not established.
*       IKE_INVALID_PROTOCOL    Protocol is not ISAKMP.
*       IKE_INVALID_SPI         SPI length is not valid.
*       IKE_INVALID_COOKIE      Cookies do not match.
*       IKE_NOT_FOUND           No SAs matched operation.
*
*************************************************************************/
STATUS IKE_Process_Notify(IKE_PHASE2_HANDLE *ph2)
{
    STATUS              status;

#if (IKE_ENABLE_INITIAL_CONTACT == NU_FALSE)
    UNUSED_PARAMETER(ph2);
#endif

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure all pointers are valid. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("Processing Notification payload");

#if (IKE_ENABLE_INITIAL_CONTACT == NU_TRUE)
    /* If notification is an INITIAL-CONTACT. */
    if(ph2->ike_params->ike_in.ike_notify->ike_notify_type ==
       IKE_IPS_NOTIFY_INIT_CONTACT)
    {
        /* Process the initial contact. */
        status = IKE_Process_Initial_Contact(ph2);
    }

    /* Otherwise, unrecognized notification type. */
    else
#endif
    {
        /* Unsupported notification message. */
        IKE_DEBUG_LOG("Unsupported notification message - ignored");

        /* Set the status. */
        status = IKE_UNSUPPORTED_NOTIFY;
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_Notify */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Process_Info_Mode
*
* DESCRIPTION
*
*       This function handles incoming Informational mode
*       messages.
*
* INPUTS
*
*       *ph2                    Pointer to Phase 2 Handle
*                               temporarily created for this
*                               Informational exchange.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      If parameters are invalid.
*       IKE_INVALID_MSGID       Message ID is invalid.
*       IKE_INVALID_FLAGS       ISAKMP header flags are invalid.
*       IKE_INVALID_XCHG_TYPE   SA Exchange type is unrecognized.
*       IKE_INVALID_PAYLOAD     Payload in message is invalid.
*       IKE_INVALID_STATE       IKE SA has invalid state.
*       IKE_INVALID_PROTOCOL    Invalid protocol specified.
*       IKE_INVALID_SPI         SPI length is not valid.
*       IKE_INVALID_COOKIE      Cookies do not match.
*       IKE_NO_KEYMAT           No key material in IKE SA.
*       IKE_VERIFY_FAILED       Hash verification failed.
*       IKE_SA_NOT_FOUND        IKE SA was not passed.
*       IKE_LENGTH_IS_SHORT     Incomplete data in message.
*       IKE_NOT_FOUND           No SAs matched operation.
*       IKE_UNEXPECTED_PAYLOAD  Unexpected payload in message.
*       IKE_DUPLICATE_PAYLOAD   Duplicate payloads in message.
*       IKE_UNSUPPORTED_DOI     DOI is not supported.
*       IKE_UNSUPPORTED_NOTIFY  Unsupported notification type.
*
*************************************************************************/
STATUS IKE_Process_Info_Mode(IKE_PHASE2_HANDLE *ph2)
{
    STATUS                  status = NU_SUCCESS;
    IKE_DEC_MESSAGE         *in;
    IKE_STATE_PARAMS        *params;
    IKE_PHASE1_HANDLE       *ph1;
    IKE_HASH_DEC_PAYLOAD    dec_hash;
    IKE_NOTIFY_DEC_PAYLOAD  dec_notify;
    IKE_DELETE_DEC_PAYLOAD  dec_del;
    UINT16                  real_len;

#if (IKE_DEBUG == NU_TRUE)
    /* Check that the Handle pointer is not NULL. */
    if(ph2 == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* State parameters must be passed to this function. */
    else if(ph2->ike_params == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the policy pointer is valid. */
    else if(ph2->ike_params->ike_policy == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure the SA pointer is valid. */
    else if(ph2->ike_sa == NU_NULL)
    {
        /* Set SA missing error code. */
        return (IKE_SA_NOT_FOUND);
    }
#endif

    /* Log debug message. */
    IKE_DEBUG_LOG("INFORMATIONAL MODE");

    /* Set local pointers to commonly used data in the Handle. */
    in     = &ph2->ike_params->ike_in;
    params = ph2->ike_params;

    /* Make sure packet pointer is not NULL. */
    if(params->ike_packet == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* If the message is encrypted. */
    else if((in->ike_hdr->ike_flags & IKE_HDR_ENC_MASK) != 0)
    {
        /* Make sure SA key material is available. */
        if(ph2->ike_sa->ike_skeyid == NU_NULL)
        {
            NLOG_Error_Log("Message encrypted but SA has no key material",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* No key material found. */
            status = IKE_NO_KEYMAT;
        }

        else if(in->ike_hdr->ike_msg_id == 0)
        {
            NLOG_Error_Log("Message encrypted but Message ID is non-zero",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Message ID is invalid. */
            status = IKE_INVALID_MSGID;
        }

        else
        {
            /* Generate phase 2 IV for decryption. */
            status = IKE_Phase2_IV(ph2);

            if(status == NU_SUCCESS)
            {
                /* Decrypt the message. */
                status = IKE_Encrypt(ph2->ike_sa,
                            params->ike_packet->ike_data + IKE_HDR_LEN,
                            params->ike_packet->ike_data_len - IKE_HDR_LEN,
                            NU_NULL, ph2->ike_decryption_iv,
                            ph2->ike_encryption_iv, IKE_DECRYPT);

                if(status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to decrypt IKE message",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to generate Phase 2 encryption IV",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
    }

    /* Otherwise the message is not encrypted. */
    else
    {
        /* If the Phase 1 Handle is still present. */
        if(ph2->ike_sa->ike_phase1 != NU_NULL)
        {
            /* Get the Phase 1 pointer from the SA. */
            ph1 = ph2->ike_sa->ike_phase1;

            /* Determine the IKE SA's exchange mode. */
            switch(ph1->ike_xchg_mode)
            {
#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)
            case IKE_XCHG_MAIN:
                /* Make sure current state does not require
                 * encryption of Informational exchange.
                 */
                if(ph1->ike_xchg_state > IKE_MAIN_ENCRYPT_INFO_STATE)
                {
                    NLOG_Error_Log("Message must be encrypted but isn't",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Encryption flag must be set. */
                    status = IKE_INVALID_FLAGS;
                }
                break;
#endif

#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
            case IKE_XCHG_AGGR:
                /* Make sure current state does not require
                 * encryption of Informational exchange.
                 */
                if(ph1->ike_xchg_state > IKE_AGGR_ENCRYPT_INFO_STATE)
                {
                    NLOG_Error_Log("Message must be encrypted but isn't",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);

                    /* Encryption flag must be set. */
                    status = IKE_INVALID_FLAGS;
                }
                break;
#endif

            default:
                NLOG_Error_Log("Unrecognized IKE exchange mode in message",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Unrecognized exchange mode. */
                status = IKE_INVALID_XCHG_TYPE;
                break;
            }
        }
    }

    /* Make sure no error occurred. */
    if(status == NU_SUCCESS)
    {
        /* Set inbound payloads used in this state. */
        IKE_SET_INBOUND_OPTIONAL(in->ike_hash, &dec_hash);
        IKE_SET_INBOUND_OPTIONAL(in->ike_notify, &dec_notify);
        IKE_SET_INBOUND_OPTIONAL(in->ike_del, &dec_del);

        /* Decode all payloads in the packet. */
        status = IKE_Decode_Message(params->ike_packet->ike_data, in);

        if(status == NU_SUCCESS)
        {
            /* If a hash payload is present. */
            if(IKE_PAYLOAD_IS_PRESENT(&dec_hash) == NU_TRUE)
            {
                /* Log debug message. */
                IKE_DEBUG_LOG("Authenticating Informational message");

                /* Get length of message data without padding. */
                status = IKE_Get_Message_Length(
                             params->ike_packet->ike_data,
                             params->ike_packet->ike_data_len, &real_len);

                if(status == NU_SUCCESS)
                {
                    /* Verify Hash of the message. */
                    status = IKE_Verify_Hash_x(ph2,
                                 params->ike_packet->ike_data,
                                 real_len, dec_hash.ike_hash_data,
                                 dec_hash.ike_hash_data_len);
                }

                else
                {
                    NLOG_Error_Log("Failed to get message length",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* If a Notification payload is present. */
            if((status == NU_SUCCESS) &&
               (IKE_PAYLOAD_IS_PRESENT(&dec_notify)))
            {
                /* Process the notification payload. */
                status = IKE_Process_Notify(ph2);
            }

            /* If a Delete payload is present. */
            if((status == NU_SUCCESS) &&
               (IKE_PAYLOAD_IS_PRESENT(&dec_del)))
            {
                /* Process the Delete payload. */
                status = IKE_Process_Delete(ph2);
            }
        }

        else
        {
            NLOG_Error_Log("Unable to decode incoming IKE message",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Process_Info_Mode */

#endif /* (IKE_INCLUDE_INFO_MODE == NU_TRUE) */

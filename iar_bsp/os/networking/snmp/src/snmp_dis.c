/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_dis.c                                               
*
*   DESCRIPTION
*
*       This file contains definitions of all the functions
*       required by the SNMP Dispatcher.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_Send_Response
*       SNMP_Determine_Ver
*       SNMP_Execute_Request
*       SNMP_Send_Notification
*
*   DEPENDENCIES
*
*       snmp_cr.h
*       snmp_mp.h
*       udp.h
*
************************************************************************/

#include "networking/snmp_cr.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_udp.h"

extern snmp_stat_t              SnmpStat;
extern udp_stat_t               Snmp_Udp_Stat;
extern INT                      Snmp_Socket;
extern SNMP_ENGINE_STRUCT       Snmp_Engine;

SNMP_MPD_MIB_STRUCT             Snmp_Mpd_Mib;
/************************************************************************
*
*   FUNCTION
*
*       SNMP_Send_Response
*
*   DESCRIPTION
*
*       This function sends the response.
*
*   INPUTS
*
*       *snmp_response        The response message to be sent to the
*                             manager.
*       *snmp_session         The session structure containing information
*                             about the response which is to be sent.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Send_Response(SNMP_MESSAGE_STRUCT *snmp_response,
                          SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status;
    INT16       unused_param;

    /* Assigning a value to get rid of warning. */
    unused_param = 0;

    status = SNMP_Encode(snmp_response, snmp_session);

    if(status == NU_SUCCESS)
    {
        SnmpStat.OutPkts++;

        /* Sending the PDU back to the client */
        if(NU_Send_To(Snmp_Socket ,
                     (CHAR *)&(snmp_response->snmp_buffer[0]),
                     (UINT16)snmp_response->snmp_buffer_len, 0,
                     &(snmp_response->snmp_transport_address),
                     unused_param) <= 0)
        {
            status = SNMP_ERROR;
            Snmp_Udp_Stat.outErrors++;
        }
        else
            Snmp_Udp_Stat.outDatagrams++;
    }

    return (status);

} /* SNMP_Send_Response */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Determine_Ver
*
*   DESCRIPTION
*
*       This function determines the message version.
*
*   INPUTS
*
*       *snmp_request        The request message.
*       *mp_model            The Message Processing Model for the given
*                            request Message.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Determine_Ver(const SNMP_MESSAGE_STRUCT *snmp_request,
                          UINT32 *mp_model)
{
    STATUS      status = NU_SUCCESS;
    UINT8       *Eoc, *End;
    UINT32      Cls, Con, Tag;
    asn1_sck_t  asn1;

    Asn1Opn(&asn1, snmp_request->snmp_buffer,
            snmp_request->snmp_buffer_len, ASN1_DEC);

    /* Get the first tag field. */
    if (!Asn1HdrDec (&asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /* This should be a sequence field. */
    else if (Eoc == NU_NULL || Cls != ASN1_UNI || Con != ASN1_CON ||
             Tag != ASN1_SEQ)
        status = SNMP_ERROR;

    /* Decode the header for the version. */
    else if (!Asn1HdrDec (&asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /* Check if this is the the correct format */
    else if (End == NU_NULL || Cls != ASN1_UNI || Con != ASN1_PRI ||
             Tag != ASN1_INT)
        status = SNMP_ERROR;

    /* Get the model. */
    else if (!Asn1IntDecUns (&asn1, End, mp_model))
        status = SNMP_ERROR;

    return (status);

} /* SNMP_Determine_Ver */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Execute_Request
*
*   DESCRIPTION
*
*       This function determines the message processing model version and
*       invokes the Message Processing subsystem to decode the SNMP
*       Message.
*
*   INPUTS
*
*       *snmp_request    The request message to be processed.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Execute_Request(SNMP_MESSAGE_STRUCT *snmp_request)
{
    STATUS                     status;
    SNMP_SESSION_STRUCT        *snmp_session;
    UINT32                     snmp_unknown_pduhandlers_oid[] =
                                {1, 3, 6, 1, 6, 3, 11, 2, 1, 3, 0};
    UINT8                      snmp_unknown_pduhandlers_oid_len = 11;
    UINT8                      *temp_ptr;

    /* Get an empty SNMP Session structure. */
    if(SNMP_Get_Request_Ptr(&snmp_session) != NU_SUCCESS)
        return (SNMP_ERROR);

    /* Clear snmp_session structure. */
    UTL_Zero(snmp_session,(UINT32)(sizeof(SNMP_SESSION_STRUCT)));

    /* Increment the snmpInPkts counter */
    SnmpStat.InPkts++;

    /* Determine the version of the SNMP message. */
    if((status = SNMP_Determine_Ver(snmp_request,
                                 &(snmp_session->snmp_mp))) != NU_SUCCESS)
    {
        SnmpStat.InASNParseErrs++;  /* Version cannot be determined */
        return (status);
    }

    /* Decode theRequest and put the values in to the Session
     * structure.
     */
    status = SNMP_Decode(snmp_request, snmp_session);

    if(status == NU_SUCCESS)
    {
        /* Was there an error while decoding for which we need to send a
         * report?
         */
        if(snmp_session->snmp_status_info.snmp_oid_len != 0)
        {
            /* Save the current SNMP Buffer pointer. */
            temp_ptr = snmp_request->snmp_buffer;

            /* Send the report. */
            status = SNMP_Send_Response(snmp_request, snmp_session);

            /* Reset the SNMP Buffer pointer. */
            snmp_request->snmp_buffer = temp_ptr;
        }

        /* Otherwise, if we have an application which processes the given
         * request, invoke it.
         */
        else if((Snmp_Engine.snmp_engine_id_len ==
                            snmp_session->snmp_context_engine_id_len) &&
                (memcmp(Snmp_Engine.snmp_engine_id,
                        snmp_session->snmp_context_engine_id,
                        Snmp_Engine.snmp_engine_id_len) == 0) &&
                (snmp_session->snmp_pdu.Type == SNMP_PDU_GET  ||
                 snmp_session->snmp_pdu.Type == SNMP_PDU_NEXT ||
                 snmp_session->snmp_pdu.Type == SNMP_PDU_SET  ||
                 snmp_session->snmp_pdu.Type == SNMP_PDU_BULK))
        {
            /* The request is ready to be processed. */
            snmp_session->snmp_in_use = SNMP_READY;
            status = SNMP_Request_Ready();
        }
        else
        {
#if (INCLUDE_MIB_MPD == NU_TRUE)
            /* There is no application to process the request. */
            Snmp_Mpd_Mib.snmp_unknown_pduhandlers++;
#endif
            /* Create an error report. */
            NU_BLOCK_COPY(snmp_session->snmp_status_info.snmp_oid,
                          snmp_unknown_pduhandlers_oid,
                      sizeof(UINT32) * snmp_unknown_pduhandlers_oid_len);

            snmp_session->snmp_status_info.snmp_oid_len =
                                    snmp_unknown_pduhandlers_oid_len;
            snmp_session->snmp_status_info.snmp_value =
                                    Snmp_Mpd_Mib.snmp_unknown_pduhandlers;

            /* Save the current SNMP Buffer pointer. */
            temp_ptr = snmp_request->snmp_buffer;

            /* Send the report. */
            status = SNMP_Send_Response(snmp_request, snmp_session);

            /* Reset the SNMP Buffer pointer. */
            snmp_request->snmp_buffer = temp_ptr;
        }

    }

    return (status);

} /* SNMP_Execute_Request */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Send_Notification
*
*   DESCRIPTION
*
*       This function sends the notification.
*
*   INPUTS
*
*       *snmp_notification    The notification message to be sent to the
*                             manager.
*       *snmp_session         The session structure containing information
*                             which is to be used to encode the
*                             notification.
*       socket                Socket descriptor to be used for sending
*                             notification.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_Send_Notification(SNMP_MESSAGE_STRUCT *snmp_notification,
                            SNMP_SESSION_STRUCT *snmp_session, INT socket)
{
    STATUS      status;
    INT16       unused_param;

    /* Assigning a value to get rid of warning. */
    unused_param = 0;

    status = SNMP_Notify(snmp_notification, snmp_session);

    if(status == NU_SUCCESS)
    {
        SnmpStat.OutPkts++;

        /* Sending the PDU back to the client */
        if(NU_Send_To(socket,
                      (CHAR *)&(snmp_notification->snmp_buffer[0]),
                      (UINT16)snmp_notification->snmp_buffer_len, 0,
                      &(snmp_notification->snmp_transport_address),
                      unused_param) <= 0)
        {
            status = SNMP_ERROR;
            Snmp_Udp_Stat.outErrors++;
        }
        else
            Snmp_Udp_Stat.outDatagrams++;
    }

    return (status);

} /* SNMP_Send_Notification */




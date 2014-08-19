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
*       snmp_v2.c                                                
*
*   DESCRIPTION
*
*       This file contains functions for the SNMP Version 2
*       Processing Model.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_V2_Init
*       SNMP_V2_Dec_Request
*       SNMP_V2_Enc_Respond
*       SNMP_V2_Enc_Error
*       SNMP_V2_Enc
*       SNMP_V2_Dec
*       SNMP_V2_Notification_Enc
*
*   DEPENDENCIES
*
*       target.h
*       snmp.h
*       snmp_mp.h
*       snmp_v2.h
*       snmp_pdu.h
*       snmp_no.h
*       snmp_ss.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/snmp.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_v2.h"
#include "networking/snmp_pdu.h"
#include "networking/snmp_no.h"
#include "networking/snmp_ss.h"

#if( INCLUDE_SNMPv2 )
extern SNMP_ENGINE_STRUCT      Snmp_Engine;
extern snmp_stat_t             SnmpStat;
extern asn1_sck_t              Snmp_V2_ErrAsn1;
STATIC SNMP_REQLIST_STRUCT     Snmp_V2_ReqList;

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Init
*
*   DESCRIPTION
*
*       This function initializes SNMPv1 Message Processing Model.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_NO_MEMORY
*
*************************************************************************/
STATUS SNMP_V2_Init(VOID)
{
    STATUS                  status;

    /* Initialize the ReqList */
    status = SNMP_Init_ReqList(&Snmp_V2_ReqList, SNMP_V2_BUFFER_SIZE);

    return (status);

} /* SNMP_V2_Init */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Dec_Request
*
*   DESCRIPTION
*
*       Decodes a request from SNMPv2 Message Format. The snmp_request is
*       stored in a list for future use.
*
*   INPUTS
*
*       *snmp_request       This structures contains the SNMP Message to
*                           be decoded.
*       *snmp_session       This structure stores the information decoded
*                           from the message.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_Dec_Request(SNMP_MESSAGE_STRUCT *snmp_request,
                           SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status = SNMP_ERROR;

    /* OID of InASNParseErrs */
    UINT32      parse_err_oid[] = {1,3,6,1,2,1,11,6,0};


    /* Add the snmp_request struct to the snmpReqList
     * and set the added request struct ref. to the
     * snmp_session stateReference variable.
     */
    snmp_session->snmp_state_ref =
                        SNMP_Add_ReqList(&Snmp_V2_ReqList, snmp_request);

    /* Call function for decoding request if the message was placed in the
     * buffer.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {

        /* Set the parameters required by the security subsystem. */
        snmp_session->snmp_sm = 2;
        snmp_session->snmp_security_level = SNMP_SECURITY_NOAUTHNOPRIV;

        status = SNMP_V2_Dec(snmp_session->snmp_state_ref, snmp_session);

        /* If an error was encountered while decoding, the packet is
         * discarded.
         */
        if(status != NU_SUCCESS)
        {
            SnmpStat.InASNParseErrs++;

            /* Generate a report. */
            NU_BLOCK_COPY(snmp_session->snmp_status_info.snmp_oid,
                        parse_err_oid, sizeof(UINT32) * 9);
            snmp_session->snmp_status_info.snmp_oid_len   = 9;
            snmp_session->snmp_status_info.snmp_value     =
                                                 SnmpStat.InASNParseErrs;

            status = NU_SUCCESS;
        }
        else
        {
            NU_BLOCK_COPY(&snmp_session->snmp_err_asn1, &Snmp_V2_ErrAsn1,
                          sizeof(asn1_sck_t));
        }
    }

    return (status);

} /* SNMP_V2_Dec_Request */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Enc_Respond
*
*   DESCRIPTION
*
*       Encodes the response for SNMPv2 Message Formats.
*
*   INPUTS
*
*       *snmp_response      This structures contains the SNMP Message to
*                           be sent to the Manager.
*       *snmp_session       This structure contains information which is
*                           to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_Enc_Respond(SNMP_MESSAGE_STRUCT *snmp_response,
                           SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status = NU_SUCCESS;
    asn1_sck_t  asn1;
    UINT8       *eoc;
    UINT32      cls, con, tag;
    UINT32      temp;
    UINT32      length = 0;

    /* Get the community string and place it in to the security name. */
    /* snmp_session->snmp_state_ref will NU_NULL in case of notifications.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {
        /* Open the ASN1 connection. */
        Asn1Opn(&asn1, snmp_session->snmp_state_ref->snmp_buffer,
                snmp_session->snmp_state_ref->snmp_buffer_len, ASN1_DEC);

        /* Decoding the Message. */
        if (!Asn1HdrDec (&asn1, &eoc, &cls, &con, &tag))
            status = SNMP_ERROR;

        else if (!Asn1HdrDec (&asn1, &eoc, &cls, &con, &tag))
            status = SNMP_ERROR;

        else if (!Asn1IntDecUns (&asn1, eoc, &temp))
            status = SNMP_ERROR;

        else if (!Asn1HdrDec (&asn1, &eoc, &cls, &con, &tag))
            status = SNMP_ERROR;

        else if (!Asn1OtsDec (&asn1, eoc,
                    snmp_session->snmp_security_name,
                    SNMP_SIZE_BUFCHR - 1, &length))
        {
            status = SNMP_ERROR;
        }

        /* Put the null terminator in to the securityName. */
        snmp_session->snmp_security_name[length] = NU_NULL;

        if(status != NU_SUCCESS)
            return (status);
    }   /* end if */

    /* if no error was reported while decoding the incoming request. */
    if((snmp_session->snmp_status_info.snmp_oid_len == 0) &&
		(snmp_session->snmp_state_ref != NU_NULL))
    {
        /* Check whether we are sending an error response. */
        if(snmp_session->snmp_pdu.Request.ErrorStatus != SNMP_NOERROR)
		{
            /* Encode an error response. */
            status = SNMP_V2_Enc_Error(snmp_session);

            /* Copy the error message to the Response buffer. */
            NU_BLOCK_COPY(snmp_response->snmp_buffer,
                          snmp_session->snmp_state_ref->snmp_buffer,
             (unsigned int)snmp_session->snmp_state_ref->snmp_buffer_len);

           /* Set the buffer length. */
            snmp_response->snmp_buffer_len =
                            snmp_session->snmp_state_ref->snmp_buffer_len;

        }
        else
        {
            /* Otherwise, do the normal encoding. */
            status = SNMP_V2_Enc(snmp_response, snmp_session);

            /* If while encoding, the message size exceeded the buffer
             * size, we need to send an error response.
             */
            if(status == SNMP_TOOBIG)
            {
                status = SNMP_V2_Enc_Error(snmp_session);

                /* Copy the error message to the Response buffer. */
                NU_BLOCK_COPY(snmp_response->snmp_buffer,
                              snmp_session->snmp_state_ref->snmp_buffer,
             (unsigned int)snmp_session->snmp_state_ref->snmp_buffer_len);

                snmp_response->snmp_buffer_len = 
                    snmp_session->snmp_state_ref->snmp_buffer_len;
            }
        }
    }   /* end if */

    /* since agent will receive only confirmed class PDUs (only requests),
     * therefore we will generate report when ever security model returns
     * an error.
     */
    else
    {
        /* if we could not decode contextEngineID*/
        if(snmp_session->snmp_context_engine_id_len == 0)
        {
            NU_BLOCK_COPY(snmp_session->snmp_context_engine_id,
                          Snmp_Engine.snmp_engine_id,
                          (unsigned int)Snmp_Engine.snmp_engine_id_len);

            snmp_session->snmp_context_engine_id_len =
                                        Snmp_Engine.snmp_engine_id_len;
        }

        NU_BLOCK_COPY(snmp_session->snmp_object_list[0].Id,
                      snmp_session->snmp_status_info.snmp_oid,
                     (unsigned int)(sizeof(UINT32) *
                            snmp_session->snmp_status_info.snmp_oid_len));

        snmp_session->snmp_object_list[0].IdLen =
                            snmp_session->snmp_status_info.snmp_oid_len;
        snmp_session->snmp_object_list[0].Syntax.LngUns =
                            snmp_session->snmp_status_info.snmp_value;
        snmp_session->snmp_object_list[0].Type = SNMP_COUNTER;
        snmp_session->snmp_object_list[0].Request = SNMP_PDU_REPORT;
        /* snmp_session->snmp_status_info has only one object in it */
        snmp_session->snmp_object_list_len = 1;

        snmp_session->snmp_pdu.Request.ErrorIndex = 0;
        snmp_session->snmp_pdu.Request.ErrorStatus = 0;
        snmp_session->snmp_pdu.Request.Id = 0;
        snmp_session->snmp_pdu.Request.Type = SNMP_PDU_REPORT;

        /* If security level is zero, make it NOAUTHNOPRIV. */
        if(snmp_session->snmp_security_level == 0)
        {
            snmp_session->snmp_security_level =
                                    SNMP_SECURITY_NOAUTHNOPRIV;
        }

        /* If security model is zero, make it 2. */
        if(snmp_session->snmp_sm == 0)
            snmp_session->snmp_sm = 2;

        status = SNMP_V2_Enc(snmp_response, snmp_session);

    } /* end else */

    /* snmp_session->snmp_state_ref is NU_NULL in case of notifications.
     * In this case SNMP_Notification_Task_Entry copies values
     * of transport address and transport domain in snmp_response.
     */
    if((status == NU_SUCCESS) &&
       (snmp_session->snmp_state_ref != NU_NULL))
    {
        /* Storing the snmp_transport_address */
        NU_BLOCK_COPY(&snmp_response->snmp_transport_address,
                    &snmp_session->snmp_state_ref->snmp_transport_address,
                    sizeof(struct addr_struct));

        /* Storing the snmp_transport_domain. */
        snmp_response->snmp_transport_domain =
                    snmp_session->snmp_state_ref->snmp_transport_domain;
    }

    /* Removing request from the snmpReqList */
    /* snmp_session->snmp_state_ref will be NU_NULL in case of a
     * notification.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {
        SNMP_Remove_ReqList(&Snmp_V2_ReqList,
                            snmp_session->snmp_state_ref);
    }

    return (status);

} /* SNMP_V2_Enc_Respond */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Enc_Error
*
*   DESCRIPTION
*
*       Encodes an error response for SNMPv2 Message Formats.
*
*   INPUTS
*
*       *snmp_session       This structure contains information which is
*                           to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_Enc_Error(SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status = NU_SUCCESS;
    UINT8       *End;

    /* Since we are sending back the same message, we just need
       to re-encode the headers. */

    /* Encode the error index. */
    if (!Asn1IntEncUns(&(snmp_session->snmp_err_asn1), &End,
                       snmp_session->snmp_pdu.Request.ErrorIndex))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc(&(snmp_session->snmp_err_asn1), End, ASN1_UNI,
                         ASN1_PRI, ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /* Encode the error status. */
    else if (!Asn1IntEncUns(&(snmp_session->snmp_err_asn1), &End,
                            snmp_session->snmp_pdu.Request.ErrorStatus))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc(&(snmp_session->snmp_err_asn1), End, ASN1_UNI,
                         ASN1_PRI, ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /* Encode the request ID. */
    else if (!Asn1IntEnc(&(snmp_session->snmp_err_asn1), &End,
                        (INT32)(snmp_session->snmp_pdu.Request.Id)))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc(&(snmp_session->snmp_err_asn1), End, ASN1_UNI,
                         ASN1_PRI, ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /* Encode the PDU header and set type to SNMP_Response. */
    else if (!Asn1HdrEnc(&(snmp_session->snmp_err_asn1),
                         snmp_session->snmp_err_asn1.End, ASN1_CTX,
                         ASN1_CON, snmp_session->snmp_pdu.Request.Type))
    {
        status = SNMP_ERROR;
    }

    /* Encode the community string. */
    else if (!Asn1OtsEnc (&(snmp_session->snmp_err_asn1), &End,
                          snmp_session->snmp_security_name,
                          (UINT32)(strlen((CHAR *)snmp_session->
                                                snmp_security_name))))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc (&(snmp_session->snmp_err_asn1), End, ASN1_UNI,
                          ASN1_PRI, ASN1_OTS))
    {
        status = SNMP_ERROR;
    }

    /* Encode the version. */
    else if (!Asn1IntEncUns (&(snmp_session->snmp_err_asn1), &End,
                             SNMP_VERSION_V2))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc (&(snmp_session->snmp_err_asn1), End, ASN1_UNI,
                          ASN1_PRI, ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /* Encode SNMP message header. */
    else if (!Asn1HdrEnc (&(snmp_session->snmp_err_asn1),
                          snmp_session->snmp_err_asn1.End,
                          ASN1_UNI, ASN1_CON, ASN1_SEQ))
    {
        status = SNMP_ERROR;
    }

    else
    {
        /* Update the SNMP buffer and length in snmp_session. */
        Asn1Cls(&(snmp_session->snmp_err_asn1),
                &(snmp_session->snmp_state_ref->snmp_buffer),
                &(snmp_session->snmp_state_ref->snmp_buffer_len));

        /* Update the statistics for sending back the response. */
        SnmpStat.OutGetResponses++;
    }

    return (status);

} /* SNMP_V2_Enc_Error */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Enc
*
*   DESCRIPTION
*
*       This function encodes and checks the syntax of an outgoing
*       response. Encoding is done in reverse order first variable binding
*       list(in fact starting with last variable) then appending fields
*       for PDU(1-ErrorIndex,2-ErrorStatus,3-RequestId) then append the
*       field of community then append the field of request id for
*       outgoing message.
*
*   INPUTS
*       *snmp_session   Pointer to data structure which contain the data
*                       structures in which information is placed and is
*                       to be encoded in this function.
*
*       *snmp_response  snmp_response version two message.
*
*   OUTPUTS
*
*       NU_SUCCESS      The outgoing SNMP-message is syntactically
*                       correct.
*       SNMP_ERROR      The outgoing SNMP-message is not syntactically
*                       correct.
*
*************************************************************************/
STATUS SNMP_V2_Enc(SNMP_MESSAGE_STRUCT *snmp_response,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS        status = NU_SUCCESS;
    UINT8         *Eoc, *End;
    asn1_sck_t    snmpEncAsn1;

    Asn1Opn (&snmpEncAsn1, snmp_response->snmp_buffer, SNMP_BUFSIZE,
             ASN1_ENC);

    if (!Asn1EocEnc (&snmpEncAsn1, &Eoc))
        status = SNMP_ERROR;

    /*decode PDU first*/
    else if (SNMP_V2_PduEnc(&snmpEncAsn1, &(snmp_session->snmp_pdu),
                            snmp_session->snmp_object_list,
                            snmp_session->snmp_object_list_len)
                                                            != NU_SUCCESS)
    {
        status = SNMP_ERROR;
    }

    /* Append community name. */
    else if (!Asn1OtsEnc (&snmpEncAsn1, &End,
                          snmp_session->snmp_security_name,
                          (UINT32)strlen((CHAR *)snmp_session->
                                                    snmp_security_name)))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc (&snmpEncAsn1, End, ASN1_UNI, ASN1_PRI,
                          ASN1_OTS))
    {
        status = SNMP_ERROR;
    }

    /*append SNMP version*/
    else if (!Asn1IntEncUns (&snmpEncAsn1, &End, SNMP_VERSION_V2))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (&snmpEncAsn1, End, ASN1_UNI, ASN1_PRI,
                          ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc (&snmpEncAsn1, Eoc, ASN1_UNI, ASN1_CON,
                         ASN1_SEQ))
    {
        status = SNMP_ERROR;
    }

    else
    {
        Asn1Cls(&snmpEncAsn1, &(snmp_response->snmp_buffer),
                &(snmp_response->snmp_buffer_len));

        switch (snmp_session->snmp_pdu.Type)
        {
        case SNMP_PDU_GET:
            SnmpStat.OutGetRequests++;
            break;
        case SNMP_PDU_NEXT:
            SnmpStat.OutGetNexts++;
            break;
        case SNMP_PDU_BULK:
            SnmpStat.OutGetBulks++;
            break;
        case SNMP_PDU_RESPONSE:
            SnmpStat.OutGetResponses++;
            break;
        case SNMP_PDU_SET:
            SnmpStat.OutSetRequests++;
            break;
        case SNMP_PDU_TRAP_V2:
            SnmpStat.OutTraps++;
            break;
        }
    }

    /* If an error occurred while encoding, did it occur because we had
       exceeded the Buffer? */
    if(status == SNMP_ERROR && snmpEncAsn1.Pointer <= snmpEncAsn1.Begin)
    {
        /* If so, we need to send an error response. */
        SnmpStat.OutTooBigs++;
        snmp_session->snmp_pdu.Request.ErrorStatus = SNMP_TOOBIG;
        status = SNMP_TOOBIG;
    }

    return (status);

} /* SNMP_V2_Enc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Dec
*
*   DESCRIPTION
*
*       This function decodes and checks the syntax of an incoming
*       request.
*
*   INPUTS
*
*       *snmp_session   Pointer to data structure which contain the data
*                       structures to which decoded information will be
*                       placed.
*       *snmp_request   Pointer to data structure contains the buffer
*                       which have message and related information.
*
*   OUTPUTS
*
*       NU_SUCCESS      The incoming SNMP-message is syntactically
*                       correct.
*       SNMP_ERROR      The incoming SNMP-message is not syntactically
*                       correct.
*
*************************************************************************/
STATUS SNMP_V2_Dec(SNMP_MESSAGE_STRUCT *snmp_request,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS     status;
    UINT8      *Eoc, *End;
    UINT32     Cls, Con, Tag;
    UINT32     length;
    asn1_sck_t    SnmpDecAsn1;

#if (SNMP_SIZE_BUFCHR > (SNMP_SIZE_SMALLOBJECTID * 2))
    UINT8      security_param[SNMP_SIZE_BUFCHR + SNMP_MAX_IP_ADDRS + 5];

    /* Clear the security parameters. */
    UTL_Zero(security_param, SNMP_SIZE_BUFCHR + SNMP_MAX_IP_ADDRS + 5);

#else
    UINT8      security_param[(SNMP_SIZE_SMALLOBJECTID * 2) + 5 +
                                                    SNMP_MAX_IP_ADDRS];

    /* Clear the security parameters. */
    UTL_Zero(security_param, (SNMP_SIZE_SMALLOBJECTID * 2) + 5 +
                                                    SNMP_MAX_IP_ADDRS);
#endif

    /* Setting the 3 pointers (Pointer, Begin, End) from Asn1 Data
     * Structures to the i incoming message. */

    Asn1Opn(&SnmpDecAsn1, snmp_request->snmp_buffer,
            snmp_request->snmp_buffer_len, ASN1_DEC);

    /* Decoding the Message. */
    if (!Asn1HdrDec (&SnmpDecAsn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrDec (&SnmpDecAsn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /* Get the message processing model. */
    else if (!Asn1IntDecUns (&SnmpDecAsn1, End, &(snmp_session->snmp_mp)))
        status = SNMP_ERROR;

    /*check for message processing model*/
    else if (snmp_session->snmp_mp != SNMP_VERSION_V2)
        status = SNMP_ERROR;

    else if (!Asn1HdrDec (&SnmpDecAsn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OTS))
    {
        status = SNMP_ERROR;
    }

    /*decoding the community name*/
    else if (!Asn1OtsDec (&SnmpDecAsn1, End, security_param,
                          SNMP_SIZE_BUFCHR - 1, &length))
    {
        status = SNMP_ERROR;
    }

    else
    {
        /* Create the security parameters to pass to the security
         * subsystem.
         */

        /* Put in the domain */
        security_param[length] = ':';
        security_param[length + 1] = snmp_request->snmp_transport_domain;
        security_param[length + 2] = ':';
        security_param[length + 3] = (UINT8)(snmp_request->
                                    snmp_transport_address.family & 0xff);
        security_param[length + 4] = (UINT8)((snmp_request->
                               snmp_transport_address.family >> 8) &0xff);
        length = length + 5;

        /* Put in the source address. */
        NU_BLOCK_COPY(&(security_param[length]),
                      snmp_request->snmp_transport_address.id.is_ip_addrs,
                      SNMP_MAX_IP_ADDRS);

        /* Do the security check. */
        status = SNMP_Verify(snmp_session->snmp_mp, SNMP_BUFSIZE,
                             security_param, snmp_session->snmp_sm,
                             &(snmp_session->snmp_security_level),
                             &(snmp_request->snmp_buffer),
                             &(snmp_request->snmp_buffer_len),
                             snmp_session->snmp_context_engine_id,
                             &(snmp_session->snmp_context_engine_id_len),
                             snmp_session->snmp_security_name,
                             &(snmp_session->snmp_maxsize_response_pdu),
                             &(snmp_session->snmp_security_state_ref),
                             &(snmp_session->snmp_status_info));

        /* Does the security check pass? */
        if(status == NU_SUCCESS)
        {
            /* Get the context engine id and the context name. */
            snmp_session->snmp_context_engine_id_len =
                                        GET32(security_param, 0);

            NU_BLOCK_COPY(snmp_session->snmp_context_engine_id,
                          &security_param[4],
                          (unsigned int)snmp_session->
                                              snmp_context_engine_id_len);
            strncpy((CHAR*)snmp_session->snmp_context_name,
                    (CHAR*)&security_param[4 +
                               snmp_session->snmp_context_engine_id_len],
				 	SNMP_SIZE_BUFCHR);

            snmp_session->snmp_pdu_version = SNMP_PDU_V2;

            /* The Message has been properly authenticated. Now decode the
             * PDU.
             */
            if (SNMP_V2_PduDec(&SnmpDecAsn1, &(snmp_session->snmp_pdu),
                               snmp_session->snmp_object_list,
                               AGENT_LIST_SIZE,
                               &snmp_session->snmp_object_list_len)
                                                            != NU_SUCCESS)
            {
                status = SNMP_ERROR;
            }
            else if (!Asn1EocDec (&SnmpDecAsn1, Eoc))
            {
                status = SNMP_ERROR;
            }
            else
            {

                switch (snmp_session->snmp_pdu.Type)
                {
                case SNMP_PDU_GET:
                    SnmpStat.InGetRequests++;
                    break;
                case SNMP_PDU_NEXT:
                    SnmpStat.InGetNexts++;
                    break;
                case SNMP_PDU_BULK:
                    SnmpStat.InGetBulks++;
                    break;
                case SNMP_PDU_RESPONSE:
                    SnmpStat.InGetResponses++;
                    break;
                case SNMP_PDU_SET:
                    SnmpStat.InSetRequests++;
                    break;
                case SNMP_PDU_TRAP_V2:
                    SnmpStat.InTraps++;
                    break;
                }
            }

        }

    }

    return (status);

} /* SNMP_V2_Dec */


/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_Notification_Enc
*
*   DESCRIPTION
*
*       This function calls SNMP_V2_Enc_Respond.
*
*   INPUTS
*
*
*       *snmp_session           Pointer to SNMP_SESSION_STRUCT, which
*                               contains information about the
*                               notification being generated.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V2_Notification_Enc(SNMP_MESSAGE_STRUCT *snmp_notification,
                                SNMP_SESSION_STRUCT* snmp_session)
{
    snmp_session->snmp_pdu_version = SNMP_PDU_V2;
    snmp_session->snmp_pdu.Request.Type = SNMP_PDU_TRAP_V2;

    return SNMP_V2_Enc_Respond(snmp_notification, snmp_session);

} /* SNMP_V2_Notification_Enc */
#endif



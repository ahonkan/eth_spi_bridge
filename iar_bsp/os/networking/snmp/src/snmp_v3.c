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
*       snmp_v3.c                                                
*
*   DESCRIPTION
*
*       This file contains functions for the SNMP Version 3
*       Processing Model.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_V3_Init
*       SNMP_V3_Dec_Request
*       SNMP_V3_Dec
*       SNMP_Msg_V3_Dec
*       SNMP_Scoped_Pdu_Dec
*       SNMP_V3_Enc_Respond
*       SNMP_V3_Encode
*       SNMP_V3_Message_Encode
*       SNMP_V3_Extract_MsgId
*       SNMP_V3_Notification_Enc
*
*   DEPENDENCIES
*
*       target.h
*       snmp_mp.h
*       snmp_v3.h
*       snmp_pdu.h
*       snmp_no.h
*       snmp_ss.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_v3.h"
#include "networking/snmp_pdu.h"
#include "networking/snmp_no.h"
#include "networking/snmp_ss.h"

#if( INCLUDE_SNMPv3 )
/* SNMP_V3_Message_ID is msgID field in SNMPv3 message */
STATIC UINT32                       SNMP_V3_Message_ID = 0;
extern snmp_stat_t                  SnmpStat;
extern SNMP_ENGINE_STRUCT           Snmp_Engine;
extern SNMP_MPD_MIB_STRUCT          Snmp_Mpd_Mib;
STATIC SNMP_REQLIST_STRUCT          Snmp_V3_ReqList;
STATIC UINT8                        Snmp_V3_Status;

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Init
*
*   DESCRIPTION
*
*       This function initializes SNMPv1 Message Processing Model.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_NO_MEMORY
*
*
*************************************************************************/
STATUS SNMP_V3_Init(VOID)
{
    STATUS                  status;

    /* Initialize the ReqList */
    if((status = SNMP_Init_ReqList(&Snmp_V3_ReqList,
                                   SNMP_V3_BUFFER_SIZE)) == NU_SUCCESS)
    {
        Snmp_V3_Status = SNMP_MODULE_INITIALIZED;
    }
    else
    {
        Snmp_V3_Status = SNMP_MODULE_NOTINITIALIZED;
    }

    return (status);

} /* SNMP_V3_Init */


/************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Dec_Request
*
*   DESCRIPTION
*
*       Decodes a request from SNMPv1 Message Format. The Request is
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
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V3_Dec_Request(SNMP_MESSAGE_STRUCT *snmp_request,
                           SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status = SNMP_ERROR;
    /* OID of InASNParseErrs */
    UINT32      parseErrOID[] = {1,3,6,1,2,1,11,6,0};

    /* Add the Request struct to the snmpReqList
     * and set the added request struct ref. to the
     * Session stateReference variable.
     */
    snmp_session->snmp_state_ref =
                        SNMP_Add_ReqList(&Snmp_V3_ReqList, snmp_request);

    /* Call function for decoding request if the message was placed in the
     * buffer.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {
        status = SNMP_V3_Dec(snmp_session->snmp_state_ref, snmp_session);

        /* If an error was encountered while decoding */
        if(status != NU_SUCCESS)
        {
            /* if the error was caused by SNMPv3 message processing model
             * (and not by user based security model).
             */
            if(status == SNMP_ERROR)
            {
                SnmpStat.InASNParseErrs++;
                NU_BLOCK_COPY(snmp_session->snmp_status_info.snmp_oid,
                              parseErrOID, sizeof(parseErrOID));
                snmp_session->snmp_status_info.snmp_oid_len = 9;
                snmp_session->snmp_status_info.snmp_value   =
                                                SnmpStat.InASNParseErrs;
            }

            /* status has been changed to NU_SUCCESS so that dispatcher
             * would call SNMPv3 message processing model to generate a
             * report PDU.
             */
            status = NU_SUCCESS;
        }
    }

    return (status);

} /* SNMP_V3_Dec_Request */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Dec
*
*   DESCRIPTION
*
*       This function initializes Asn1 pointer and calls SNMP_Msg_V3_Dec.
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
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V3_Dec(SNMP_MESSAGE_STRUCT* snmp_request,
                   SNMP_SESSION_STRUCT* snmp_session)
{

    STATUS      status;
    asn1_sck_t  Asn1;

    Asn1Opn(&(Asn1), snmp_request->snmp_buffer,
            snmp_request->snmp_buffer_len, ASN1_DEC);

    snmp_session->snmp_pdu_version = SNMP_PDU_V2;

    status = SNMP_Msg_V3_Dec(&Asn1, &(snmp_session->snmp_sm),
                             snmp_session->snmp_security_name,
                             &(snmp_session->snmp_security_level),
                             snmp_session->snmp_context_engine_id,
                             snmp_session->snmp_context_name,
                             snmp_session->snmp_object_list,
                             &(snmp_session->snmp_object_list_len),
                             &(snmp_session->snmp_pdu),
                             &(snmp_request->snmp_buffer_len),
                             &(snmp_session->snmp_context_engine_id_len),
                             &(snmp_session->snmp_maxsize_response_pdu),
                             &(snmp_session->snmp_security_state_ref),
                             &(snmp_session->snmp_status_info));

    if( status == NU_SUCCESS )
    {
        switch (snmp_session->snmp_pdu.Type)
        {
        case SNMP_PDU_GET:
            SnmpStat.InGetRequests++;
            break;
        case SNMP_PDU_NEXT:
            SnmpStat.InGetNexts++;
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

    return status;
} /* SNMP_V3_Dec */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_Msg_V3_Dec
*
*   DESCRIPTION
*
*       This function decodes snmpv3 message from its start to the point
*       where security parameters start.
*
*   INPUTS
*
*       *asn1                   The ASN1 connection used to decode.
*       *snmp_sm                The security model to be used.
*       *snmp_security_name     The security name for the principal.
*       *snmp_security_level    The security level required.
*       *snmp_engine_id         The context engine ID.
*       *snmp_context_name      The Context.
*       *snmp_object_list       The object list on which the request is to
*                               be processed.
*       *snmp_object_list_len   The object list length.
*       *snmp_pdu               The SNMP PDU.
*       *buff_len               The message length.
*       *snmp_context_engine_id_len     The length of the context engine
*                                       ID.
*       *snmp_max_size_response_pdu     The maximum size of the response
*                                       PDU.
*       **snmp_security_state_ref       Reference to the security
*                                       information.
*       *error_indication               Information about the error
*                                       (if any occurred).
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*
*************************************************************************/
STATUS SNMP_Msg_V3_Dec(asn1_sck_t* asn1, UINT32* snmp_sm,
                       UINT8* snmp_security_name,
                       UINT8* snmp_security_level, UINT8* snmp_engine_id,
                       UINT8* snmp_context_name,
                       snmp_object_t* snmp_object_list,
                       UINT32* snmp_object_list_len, snmp_pdu_t* snmp_pdu,
                       UINT32* buff_len,
                       UINT32* snmp_context_engine_id_len,
                       UINT32* snmp_max_size_response_pdu,
                       VOID** snmp_security_state_ref,
                       SNMP_ERROR_STRUCT*  error_indication)
{
    STATUS  status = SNMP_ERROR;
    UINT8   *Eoc, *End;
    UINT32  Cls, Con, Tag, msgVersion, msgID, msgMaxSize, tempLen;
    UINT8   msgFlags;

    if (!Asn1HdrDec (asn1, &Eoc, &Cls, &Con, &Tag))     /* Sequence */
    {
        return status;
    }

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
    {
        return status;
    }

    /* msgVersion */
    else if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag))
    {
        return status;
    }

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        return status;
    }

    else if (!Asn1IntDecUns (asn1, End, &msgVersion))
    {
        return status;
    }

    else if(msgVersion != SNMP_VERSION_V3)
        return status;

    if (!Asn1HdrDec (asn1, &Eoc, &Cls, &Con, &Tag))     /* Sequence */
    {
        return status;
    }

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
    {
        return status;
    }

    else if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag)) /* msgID */
    {
        return status;
    }

    else if ((Cls != ASN1_UNI) || (Con != ASN1_PRI) || (Tag != ASN1_INT))
    {
        return status;
    }

    else if (!Asn1IntDecUns (asn1, End, &msgID))
    {
        return status;
    }

    else if(msgID > 0x7FFFFFFF)
    {
        SnmpStat.InASNParseErrs++;
        return (ASN1_ERR_DEC_BADVALUE);
    }

    else if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag)) /* msgMaxSize */
    {
        return status;
    }

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        return status;
    }

    else if (!Asn1IntDecUns (asn1, End, &msgMaxSize))
    {
        return status;
    }

    else if((msgMaxSize <= 300) || (msgMaxSize > 0x7FFFFFFF))
    {
        SnmpStat.InASNParseErrs++;
        return (ASN1_ERR_DEC_BADVALUE);
    }

    else if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag)) /* msgFlags */
    {
        return status;
    }

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OTS))
    {
        return status;
    }

    else if (!Asn1OctDec (asn1, &msgFlags))
    {
        return status;
    }

    else if ((msgFlags == 2) || (msgFlags == 6)) 
    {
        Snmp_Mpd_Mib.snmp_invalid_msg++;
        return (ASN1_ERR_DEC_BADVALUE);
    }

    /* msgSecurityModel */
    else if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag))
    {
        return status;
    }

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        return status;
    }

    else if (!Asn1IntDecLng (asn1, End, snmp_sm))
    {
        return status;
    }
    else if (((INT32)*snmp_sm) <= 0) 
    {
        SnmpStat.InASNParseErrs++;

        /* Convert to SNMPv3 in case of sending back a report. */
        *snmp_sm = 3; 

        return (ASN1_ERR_DEC_BADVALUE);
    }

    *snmp_security_level = (UINT8)(msgFlags & SNMP_SECURITY_LEVEL_MASK);

    if ((*snmp_security_level) == 2)
        (*snmp_security_level) = 4;

    else if( (*snmp_security_level == 0) || (*snmp_security_level == 1) )
        (*snmp_security_level)++;

    /* save length of snmp message from start up to scoped PDU*/
    tempLen = (UINT32)(asn1->Pointer - asn1->Begin);

    /* Call security model and pass it asn1->Pointer which is pointing
     * to security parameters at this point.
     */
    status = SNMP_Verify(SNMP_VERSION_V3, AGENT_BUFFER_SIZE,
                         asn1->Pointer, *snmp_sm, snmp_security_level,
                         &(asn1->Begin), buff_len, snmp_engine_id,
                         snmp_context_engine_id_len, snmp_security_name,
                         snmp_max_size_response_pdu,
                         snmp_security_state_ref, error_indication);

    if(status != NU_SUCCESS)
        return status;

    /* again open asn1, now it is pointing to decrypted scoped snmp_pdu */
    Asn1Opn (asn1, asn1->Begin, *buff_len, ASN1_DEC);

    /* save the length of whole snmp message with decrypted PDU*/
    *buff_len = (*buff_len) + tempLen;

    status = SNMP_Scoped_Pdu_Dec(asn1, snmp_engine_id, snmp_context_name,
                                 snmp_object_list, snmp_object_list_len,
                                 snmp_pdu, snmp_context_engine_id_len);

    return (status);

} /* SNMP_Msg_V3_Dec */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_Scoped_Pdu_Dec
*
*   DESCRIPTION
*
*       This function decodes decrypted scoped PDU.
*
*   INPUTS
*
*       *asn1                   ASN1 connection for decoding.
*       *snmp_engine_id         SNMP Context Engine Id.
*       *snmp_context_name      The Context.
*       *snmp_object_list       List of objects on which the request is to
*                               be processed.
*       *snmp_object_list_len   Object List length.
*       *snmp_pdu               The SNMP PDU
*       *snmp_engine_id_len     Context Engine ID length.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*
*************************************************************************/
STATUS SNMP_Scoped_Pdu_Dec(asn1_sck_t* asn1, UINT8* snmp_engine_id,
                           UINT8* snmp_context_name,
                           snmp_object_t* snmp_object_list,
                           UINT32* snmp_object_list_len,
                           snmp_pdu_t* snmp_pdu,
                           UINT32* snmp_engine_id_len)
{
    UINT8   *Eoc, *End;
    UINT32  Cls, Con, Tag;
    UINT32  contextLen;
    STATUS  status;

    if (!Asn1HdrDec (asn1, &Eoc, &Cls, &Con, &Tag))     /* Sequence */
    {
        return SNMP_ERROR;
    }

    if (Cls != ASN1_UNI || Con != ASN1_CON || Tag != ASN1_SEQ)
    {

        return SNMP_ERROR;
    }

    if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag)) /* snmp_engine_id */
    {
        return SNMP_ERROR;
    }

    if ( (Cls != ASN1_UNI) || (Con != ASN1_PRI) || (Tag != ASN1_OTS) )
    {
        return SNMP_ERROR;
    }

    if (!Asn1OtsDec (asn1, End, snmp_engine_id,
                     (SNMP_SIZE_SMALLOBJECTID - 1), snmp_engine_id_len))
    {
        return SNMP_ERROR;
    }

    /* snmp_context_name */
    if (!Asn1HdrDec (asn1, &End, &Cls, &Con, &Tag))
    {
        return SNMP_ERROR;
    }

    if ( (Cls != ASN1_UNI) || (Con != ASN1_PRI) || (Tag != ASN1_OTS) )
    {
        return SNMP_ERROR;
    }

    if (!Asn1OtsDec (asn1, End, snmp_context_name, (SNMP_SIZE_BUFCHR - 1),
                     &contextLen))
    {
        return SNMP_ERROR;
    }

    /* Terminate snmp_context_name with a NU_NULL character */
    *(snmp_context_name + contextLen + 1) = '\0';

    status = SNMP_V2_PduDec(asn1, snmp_pdu, snmp_object_list,
                            AGENT_LIST_SIZE, snmp_object_list_len);

    if (status == ASN1_ERR_DEC_EOC_MISMATCH) 
    {
        SnmpStat.InASNParseErrs++;
    }

    return (status);

} /* SNMP_Scoped_Pdu_Dec */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Enc_Respond
*
*   DESCRIPTION
*
*       This function calls functions which encode a response PDU. If an
*       error was returned by security model while decoding the message,
*       this function generates a report PDU.
*
*   INPUTS
*
*       *snmp_response      The response message
*       *snmp_session       The information which will be used to encode
*                           the response.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*
*************************************************************************/
STATUS SNMP_V3_Enc_Respond(SNMP_MESSAGE_STRUCT *snmp_response,
                           SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status;

    /* Has the message processing model been initialized? */
    if(Snmp_V3_Status != SNMP_MODULE_INITIALIZED)
        /* The Model is not initialized! */
        return (SNMP_ERROR);

    /* if no error was reported while decoding the incoming request */
    if (snmp_session->snmp_status_info.snmp_oid_len == 0)
    {

        status = SNMP_V3_Encode(snmp_response, snmp_session);

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
            (unsigned int)(sizeof(UINT32) * snmp_session->
                                          snmp_status_info.snmp_oid_len));
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

        status = SNMP_V3_Encode(snmp_response, snmp_session);

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
    /* following call to SNMP_Remove_ReqList should not modify "status"*/
    /* snmp_session->snmp_state_ref will be NU_NULL in case of a
     * notification.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {
        SNMP_Remove_ReqList(&Snmp_V3_ReqList,
                            snmp_session->snmp_state_ref);
    }

    return (status);

} /* SNMP_V3_Enc_Respond */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Encode
*
*   DESCRIPTION
*
*       This function initializes Asn1 pointer and calls
*       SNMP_V3_Message_Encode.
*
*   INPUTS
*
*       *snmp_response      The response message.
*       *snmp_session       The information required to encode the
*                           response message.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*
*************************************************************************/
STATUS SNMP_V3_Encode (SNMP_MESSAGE_STRUCT * snmp_response,
                       SNMP_SESSION_STRUCT* snmp_session)
{
    STATUS              status;
    static asn1_sck_t   SnmpEncAsn1;
    UINT8*              scopedPdu = NU_NULL;
    UINT32              msgID = 0;

    /*setting the pointers (Pointer, Begin, End) to the response Buffer
     * area.
     */
    /* SnmpEncAsn1->Pointer will be set to end of the Buffer for
     * encoding.
     */
    /* AGENT_BUFFER_SIZE defined in Agent.h */
    Asn1Opn(&SnmpEncAsn1, snmp_response->snmp_buffer, SNMP_BUFSIZE,
            ASN1_ENC);

    /* snmp_session->snmp_state_ref will be NU_NULL in case of
     * notification.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {
        if(SNMP_V3_Extract_MsgId(
                            snmp_session->snmp_state_ref->snmp_buffer,
                            snmp_session->snmp_state_ref->snmp_buffer_len,
                            &msgID) != NU_TRUE)
        {
            return SNMP_ERROR;
        }
    }


    if (SNMP_V3_Message_Encode(&SnmpEncAsn1, &(snmp_session->snmp_pdu),
                               snmp_session->snmp_object_list,
                               snmp_session->snmp_object_list_len,
                               snmp_session->snmp_context_engine_id,
                               snmp_session->snmp_context_engine_id_len,
                               snmp_session->snmp_context_name,
                               &snmp_session->snmp_sm,
                               snmp_session->snmp_mp,
                               snmp_session->snmp_security_name,
                               snmp_session->snmp_security_level,
                               snmp_session->snmp_security_state_ref,
                               &scopedPdu, msgID) != NU_SUCCESS)
    {
        status = SNMP_ERROR;
    }
    else
    {

        /*updating SNMP and SnmpLength with Asn1 data structures*/
        Asn1Cls(&SnmpEncAsn1, &(snmp_response->snmp_buffer),
                &(snmp_response->snmp_buffer_len));

        if( (status = SNMP_Secure(snmp_session->snmp_mp,
                                &(snmp_response->snmp_buffer),
                                &(snmp_response->snmp_buffer_len),
                                AGENT_BUFFER_SIZE,
                                snmp_session->snmp_sm,
                                snmp_session->snmp_context_engine_id,
                                snmp_session->snmp_context_engine_id_len,
                                snmp_session->snmp_security_name,
                                snmp_session->snmp_security_level,
                                scopedPdu,
                                snmp_session->snmp_security_state_ref))
                                                           != NU_SUCCESS )
        {
            return status;
        }

        /*updating snmp statistics*/
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
        case SNMP_PDU_INFORM:
            SnmpStat.OutInforms++;
            break;
        case SNMP_PDU_TRAP_V2:
            SnmpStat.OutTraps++;
            break;
        case SNMP_PDU_REPORT:
            SnmpStat.OutReports++;
            break;
        }
    }

    return (status);

} /* SNMP_V3_Encode */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Message_Encode
*
*   DESCRIPTION
*
*       This function encodes snmpv3 message.
*
*   INPUTS
*
*       *asn1                   ASN1 connection for encoding.
*       *snmp_pdu               SNMP PDU.
*       *snmp_object_list       Object List on which the request was
*                               processed.
*       snmp_object_list_len    Object List length.
*       *snmp_engine_id         Context Engine ID.
*       snmp_engine_id_len      Engine ID Length.
*       *snmp_context_name      The Context.
*       *snmp_sm                Security Model to be used.
*       *original_msg           The original SNMP Message.
*       original_msg_size       Original Message length.
*       snmp_mp                 Message processing model.
*       *snmp_security_name     Security Name of the principal.
*       snmp_security_level     Security level requested.
*       *security_state_ref     Security Information for the session.
*       **scoped_pdu            Scoped PDU
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*
*************************************************************************/
STATUS SNMP_V3_Message_Encode(asn1_sck_t* asn1, snmp_pdu_t* snmp_pdu,
                              snmp_object_t* snmp_object_list,
                              UINT32 snmp_object_list_len,
                              const UINT8* snmp_engine_id,
                              UINT32 snmp_engine_id_len,
                              const UINT8* snmp_context_name,
                              UINT32 *snmp_sm, UINT32 snmp_mp,
                              UINT8* snmp_security_name,
                              UINT8 snmp_security_level,
                              VOID* security_state_ref,
                              UINT8** scoped_pdu, UINT32 msgID)
{
    STATUS  status = NU_SUCCESS;
    UINT8   *Eoc, *End;
    UINT8   msgFlags = 0;

    UNUSED_PARAMETER(snmp_mp);
    UNUSED_PARAMETER(snmp_security_name);
    UNUSED_PARAMETER(security_state_ref);

    if( (snmp_security_level == 1) || (snmp_security_level == 2) )
        snmp_security_level--;

    /* in case of notifications, if snmp_sm is 0 (i.e Any Security Model),
     * change it to 3 (i.e user based security model).
     */
    if(*snmp_sm == 0)
        *snmp_sm = 3;

    msgFlags = snmp_security_level;

    if((snmp_pdu->Type == SNMP_PDU_RESPONSE) ||
       (snmp_pdu->Type == SNMP_PDU_TRAP_V2) ||
       (snmp_pdu->Type == SNMP_PDU_REPORT))
    {
        msgFlags = (UINT8)(msgFlags & SNMP_REPORTABLE_FLAG_0);
    }
    else
    {
        msgFlags = (UINT8)(msgFlags | SNMP_REPORTABLE_FLAG_1);
    }

    /*marking the pointer Eoc in the end of response Buffer*/
    if (!Asn1EocEnc (asn1, &Eoc))
    {
        return SNMP_ERROR;
    }

    /*calling the function which encodes the snmp_pdu*/
    if (SNMP_V2_PduEnc(asn1, snmp_pdu, snmp_object_list,
                       snmp_object_list_len) != NU_SUCCESS)
    {
        return SNMP_ERROR;
    }

    /* Calling the function which appends the snmp_context_name in front
     * of PDU.
     */
    if (!Asn1OtsEnc (asn1, &End, snmp_context_name,
                     (UINT32)strlen((char *)snmp_context_name)))
    {
        return SNMP_ERROR;
    }

    /* Append the header for snmp_context_name with properties
     * (ASN1_UNI, ASN1_PRI, ASN1_OTS).
     */
    if (!Asn1HdrEnc (asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OTS))
    {
        return SNMP_ERROR;
    }

    /* Calling the function which appends the snmp_engine_id in front of
     * snmp_context_name.
     */
    if (!Asn1OtsEnc (asn1, &End, snmp_engine_id, snmp_engine_id_len))
    {
        return SNMP_ERROR;
    }

    /* Append the header for snmp_engine_id with properties
     * (ASN1_UNI, ASN1_PRI, ASN1_OTS).
     */
    if (!Asn1HdrEnc (asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OTS))
    {
        return SNMP_ERROR;
    }


    /********************* append SEQUENCE for ScopedPDU *************/
    if (!Asn1HdrEnc (asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
    {
        return SNMP_ERROR;
    }

    /* save pointer to start of scoped_pdu */
    *scoped_pdu = asn1->Pointer;

    /*append the msgSecurityModel */
    if (!Asn1IntEncUns (asn1, &Eoc, *snmp_sm))
    {
        return SNMP_ERROR;
    }

    /*append the header for msgSecurityModel*/
    if (!Asn1HdrEnc (asn1, Eoc, ASN1_UNI, ASN1_PRI, ASN1_INT))
    {
        return SNMP_ERROR;
    }

    /* Calling the function which appends the msgFlags in front of
     * msgSecurityModel.
     */
    if (!Asn1OtsEnc (asn1, &End, &msgFlags, 1))
    {
        return SNMP_ERROR;
    }

    /* Append the header for msgFlags with properties
     * (ASN1_UNI, ASN1_PRI, ASN1_OTS).
     */
    if (!Asn1HdrEnc (asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OTS))
    {
        return SNMP_ERROR;
    }

    /* append the msgMaxSize in front of msgFlags */
    /* AGENT_BUFFER_SIZE defined in agent.h */
    if (!Asn1IntEncUns (asn1, &End, AGENT_BUFFER_SIZE))
    {
        return SNMP_ERROR;
    }

    /*append the header for msgMaxSize*/
    if (!Asn1HdrEnc (asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
    {
        return SNMP_ERROR;
    }

    if( (snmp_pdu->Type == SNMP_PDU_TRAP_V2) ||
        (snmp_pdu->Type == SNMP_PDU_INFORM) ) /* if this is a trap */
    {
        SNMP_V3_Message_ID++;

        if (!Asn1IntEncUns (asn1, &End, SNMP_V3_Message_ID))
            return SNMP_ERROR;
    }

    /*append the msgID */
    else
    {

        if (!Asn1IntEncUns (asn1, &End, msgID))
            return SNMP_ERROR;
    }

    /*append the header for msgID*/
    if (!Asn1HdrEnc (asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
    {
        return SNMP_ERROR;
    }


    /*************** append SEQUENCE for HeaderData ***********/
    if (!Asn1HdrEnc (asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
    {
        return SNMP_ERROR;
    }

    return status;

} /* SNMP_V3_Message_Encode */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Extract_MsgId
*
*   DESCRIPTION
*
*       This function extracts msg_id from the original snmpv3 message.
*
*   INPUTS
*
*       *snmp_buffer     buffer containing original snmp message.
*       snmp_buffer_size size of original snmp message.
*       *msg_id          extracted msg_id will be stored in the variable
*                        pointed by msg_id
*
*   OUTPUTS
*
*       NU_TRUE
*       NU_FALSE
*
*************************************************************************/
BOOLEAN SNMP_V3_Extract_MsgId(UINT8* snmp_buffer, UINT32 snmp_buffer_size,
                           UINT32* msg_id)
{
    UINT8*      End;
    UINT32      Cls, Con, Tag;
    asn1_sck_t  Asn1;

    Asn1Opn(&Asn1, snmp_buffer, snmp_buffer_size, ASN1_DEC);

    if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))        /* Sequence */
        return NU_FALSE;

    else if (Cls != ASN1_UNI || Con != ASN1_CON || Tag != ASN1_SEQ)
        return NU_FALSE;

    /* msgVersion */
    else if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))
        return NU_FALSE;

    else if (Cls != ASN1_UNI || Con != ASN1_PRI || Tag != ASN1_INT)
        return NU_FALSE;

    /* msgVersion has been decoded just to increment the pointers of
     * Asn1.
     */
    else if (!Asn1IntDecUns (&Asn1, End, msg_id))
        return NU_FALSE;

    if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag))        /* Sequence */
        return NU_FALSE;

    else if (Cls != ASN1_UNI || Con != ASN1_CON || Tag != ASN1_SEQ)
        return NU_FALSE;

    else if (!Asn1HdrDec (&Asn1, &End, &Cls, &Con, &Tag)) /* msg_id */
        return NU_FALSE;

    else if ((Cls != ASN1_UNI) || (Con != ASN1_PRI) || (Tag != ASN1_INT))
        return NU_FALSE;

    else if (!Asn1IntDecUns (&Asn1, End, msg_id))
        return NU_FALSE;

    return NU_TRUE;

} /* SNMP_V3_Extract_MsgId */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V3_Notification_Enc
*
*   DESCRIPTION
*
*       This function calls SNMP_V3_Enc_Respond.
*
*   INPUTS
*
*
*       *snmp_notification      The notification message.
*       *snmp_session           Information used to create the
*                               notification message.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V3_Notification_Enc(SNMP_MESSAGE_STRUCT *snmp_notification,
                                SNMP_SESSION_STRUCT* snmp_session)
{
    snmp_session->snmp_pdu_version = SNMP_PDU_V2;
    snmp_session->snmp_pdu.Request.Type = SNMP_PDU_TRAP_V2;

    return SNMP_V3_Enc_Respond(snmp_notification, snmp_session);

} /* SNMP_V3_Notification_Enc */

#endif

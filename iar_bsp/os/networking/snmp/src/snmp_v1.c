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
*       snmp_v1.c                                                
*
*   DESCRIPTION
*
*       This file contains functions for the SNMP Version 1
*       Processing Model.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_V1_Init
*       SNMP_V1_Enc_Respond
*       SNMP_V1_Enc_Error
*       SNMP_V1_Dec_Request
*       SNMP_V1_ObjEnc
*       SNMP_V1_ObjDec
*       SNMP_V1_LstEnc
*       SNMP_V1_LstDec
*       SNMP_V1_RqsEnc
*       SNMP_V1_RqsDec
*       SNMP_V1_TrpEnc
*       SNMP_V1_PduEnc
*       SNMP_V1_PduDec
*       SNMP_V1_Enc
*       SNMP_V1_Dec
*       SNMP_V1_Notification_Enc
*
*   DEPENDENCIES
*
*       target.h
*       snmp.h
*       snmp_mp.h
*       snmp_v1.h
*       snmp_no.h
*       snmp_ss.h
*       mib.h
*
************************************************************************/

#include "networking/target.h"
#include "networking/snmp.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_v1.h"
#include "networking/snmp_no.h"
#include "networking/snmp_ss.h"
#include "networking/mib.h"

#if (INCLUDE_SNMPv1 == NU_TRUE)
extern snmp_stat_t                  SnmpStat;
extern UINT8                        cfig_hostid[SNMP_MAX_IP_ADDRS];
extern UINT8                        cfig_hosttype;
STATIC asn1_sck_t                   Snmp_V1_ErrAsn1;
STATIC SNMP_REQLIST_STRUCT          Snmp_V1_ReqList;

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Init
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
STATUS SNMP_V1_Init(VOID)
{
    STATUS                  status;

    /* Initialize the ReqList */
    status = SNMP_Init_ReqList(&Snmp_V1_ReqList, SNMP_V1_BUFFER_SIZE);    

    return (status);

} /* SNMP_V1_Init*/

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Enc_Respond
*
*   DESCRIPTION
*
*       Encodes response in SNMPv1 Message Format.
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
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_Enc_Respond(SNMP_MESSAGE_STRUCT *snmp_response,
                           SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status = NU_SUCCESS;
    asn1_sck_t  asn1;
    UINT8       *eoc;
    UINT32      cls, con, tag;
    UINT32      temp;
    UINT32      length = 0;

    /* If a report is to be sent, then since this is a version 1 model, we
       cannot send the report. */
    if(snmp_session->snmp_status_info.snmp_oid_len != 0)
    {
        /* Removing request from the snmpReqList */
        SNMP_Remove_ReqList(&Snmp_V1_ReqList,
                            snmp_session->snmp_state_ref);

        return (SNMP_ERROR);
    }

    /* Get the community string and place it in to the security name. */
    /* snmp_session->snmp_state_ref will NU_NULL in case of
     * notifications.
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

        if(status != NU_SUCCESS)
            return (status);

        /* Put the null terminator in to the securityName. */
        snmp_session->snmp_security_name[length] = NU_NULL;

    }   /* end if */

    /* Check whether we are sending an error response. */
    if((snmp_session->snmp_pdu.Request.ErrorStatus != SNMP_NOERROR) &&
       (snmp_session->snmp_state_ref != NU_NULL))
    {
        /* Encode an error response. */
        status = SNMP_V1_Enc_Error(snmp_session);

        /* Copy the error message to snmp_response' buffer. */
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
        status = SNMP_V1_Enc(snmp_response, snmp_session);

        /* If while encoding, the message size exceeded the buffer size,
           we need to send an error response. */
        /* snmp_session->snmp_state_ref will be NU_NULL in case of
         * notifications.
         */
        if((status == SNMP_TOOBIG) &&
           (snmp_session->snmp_state_ref != NU_NULL))
        {
            status = SNMP_V1_Enc_Error(snmp_session);

            /* Copy the error message to snmp_response' buffer. */
            NU_BLOCK_COPY(snmp_response->snmp_buffer,
                          snmp_session->snmp_state_ref->snmp_buffer,
             (unsigned int)snmp_session->snmp_state_ref->snmp_buffer_len);

            /* Set the buffer length. */
            snmp_response->snmp_buffer_len =
                            snmp_session->snmp_state_ref->snmp_buffer_len;


        }
    }

    /* snmp_session->snmp_state_ref is NU_NULL in case of notifications.
     * In this case SNMP_Notification_Task_Entry copies
     * values of transport address and transport domain in snmp_response.
     */
    if((status == NU_SUCCESS) &&
       (snmp_session->snmp_state_ref != NU_NULL))
    {
        /* Storing the snmp_transport_address */
        NU_BLOCK_COPY(&snmp_response->snmp_transport_address,
                    &snmp_session->snmp_state_ref->snmp_transport_address,
                    sizeof(struct addr_struct));

        /* Storing the transport domain. */
        snmp_response->snmp_transport_domain =
                      snmp_session->snmp_state_ref->snmp_transport_domain;
    }

    /* Removing request from the snmpReqList */
    /* snmp_session->snmp_state_ref will be NU_NULL in case of
     * notifications.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {
        SNMP_Remove_ReqList(&Snmp_V1_ReqList,
                            snmp_session->snmp_state_ref);
    }

    return (status);

} /* SNMP_V1_Enc_Respond */


/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Enc_Error
*
*   DESCRIPTION
*
*       Encodes an error response for SNMPv1 Message Formats.
*
*   INPUTS
*
*       *snmp_session       This structure contains information which is
*                           to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_Enc_Error(SNMP_SESSION_STRUCT *snmp_session)
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
              (UINT32)(strlen((CHAR *)snmp_session->snmp_security_name))))
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
                             SNMP_VERSION_V1))
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
                          snmp_session->snmp_err_asn1.End, ASN1_UNI,
                          ASN1_CON, ASN1_SEQ))
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

} /* SNMP_V1_Enc_Error */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Dec_Request
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
STATUS SNMP_V1_Dec_Request(SNMP_MESSAGE_STRUCT *snmp_request,
                           SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS      status = SNMP_ERROR;

    /* Add the Request structure to the snmpReqList
     * and set the added request structure ref. to the
     * Session stateReference variable.
     */
    snmp_session->snmp_state_ref = SNMP_Add_ReqList(&Snmp_V1_ReqList,
                                                    snmp_request);

    /* Call function for decoding request if the message was placed in the
     * buffer.
     */
    if(snmp_session->snmp_state_ref != NU_NULL)
    {

        /* Set the parameters required by the security subsystem. */
        snmp_session->snmp_sm = 1;
        snmp_session->snmp_security_level = SNMP_SECURITY_NOAUTHNOPRIV;

        status = SNMP_V1_Dec(snmp_session->snmp_state_ref, snmp_session);

        /* If an error was encountered while decoding, the packet is
         * discarded.
         */
        if(status != NU_SUCCESS)
        {
            SnmpStat.InASNParseErrs++;
            SNMP_Remove_ReqList(&Snmp_V1_ReqList,
                                snmp_session->snmp_state_ref);
        }
        else
        {
            NU_BLOCK_COPY(&snmp_session->snmp_err_asn1, &Snmp_V1_ErrAsn1,
                          sizeof(asn1_sck_t));
        }
    }
    else
    {
        SnmpStat.InASNParseErrs++;
    }

    return (status);

} /* SNMP_V1_Dec_Request */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_ObjEnc
*
*   DESCRIPTION
*
*       This function encodes a single variable binding.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure used to
*                   encode the data.
*       *Obj        A pointer to the data to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_ObjEnc(asn1_sck_t *Asn1, const snmp_object_t *Obj)
{
    STATUS        status = NU_SUCCESS;
    UINT32        Cls, Tag;
    UINT8         *Eoc, *End = NU_NULL;

    if (Asn1EocEnc (Asn1, &Eoc))
    {
        switch (Obj->Type)
        {
            case SNMP_INTEGER:
                if (!Asn1IntEnc (Asn1, &End, Obj->Syntax.LngInt))
                    status = SNMP_ERROR;
                break;
            case SNMP_OCTETSTR:
            case SNMP_OPAQUE:
                if (!Asn1OtsEnc (Asn1, &End, Obj->Syntax.BufChr,
                                 Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                break;
            case SNMP_NULL:
                if (!Asn1NulEnc (Asn1, &End))
                    status = SNMP_ERROR;
                break;
            case SNMP_OBJECTID:
                if (!Asn1OjiEnc (Asn1, &End, Obj->Syntax.BufInt,
                                 Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                break;
            case SNMP_IPADDR:
                if (!Asn1OtsEnc (Asn1, &End, (UINT8 *)&Obj->Syntax.LngUns,
                                 4))
                {
                    status = SNMP_ERROR;
                }

                break;
            case SNMP_COUNTER:
            case SNMP_GAUGE:
            case SNMP_TIMETICKS:
                if (!Asn1IntEncLngUns (Asn1, &End, Obj->Syntax.LngUns))
                    status = SNMP_ERROR;
                break;
            default:
                SnmpStat.OutBadValues++;
                status = SNMP_ERROR;
        }
    }
    else
        status = SNMP_ERROR;

    if (status == NU_SUCCESS)
    {
        if (!SNMP_Syn2TagCls (&Tag, &Cls, Obj->Type))
            status = SNMP_ERROR;

        else if (!Asn1HdrEnc (Asn1, End, Cls, ASN1_PRI, Tag))
            status = SNMP_ERROR;

        else if (!Asn1OjiEnc (Asn1, &End, Obj->Id, Obj->IdLen))
            status = SNMP_ERROR;

        else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OJI))
            status = SNMP_ERROR;

        else if (!Asn1HdrEnc (Asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
            status = SNMP_ERROR;
    }

    return (status);

} /* SNMP_V1_ObjEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_ObjDec
*
*   DESCRIPTION
*
*       This function decodes a single Var-bind.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Obj        A pointer to the data to be decoded.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_ObjDec (asn1_sck_t *Asn1, snmp_object_t *Obj)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *Eoc, *End;
    UINT32  Cls, Con, Tag;

    if (!Asn1HdrDec (Asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OJI))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1OjiDec (Asn1, End, Obj->Id, SNMP_SIZE_OBJECTID,
                          &Obj->IdLen))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || Con != ASN1_PRI)
        status = SNMP_ERROR;

    /* Determine the Obj->Type of the packet */
    else if (!SNMP_TagCls2Syn (Tag, Cls, &Obj->Type))
        status = SNMP_ERROR;

    else
    {
        /* Process the Obj->Type accordingly */
        switch (Obj->Type)
        {
            case SNMP_INTEGER:
                if (!Asn1IntDecLng (Asn1, End,
                                    (UINT32 *)&Obj->Syntax.LngInt))
                {
                    status = SNMP_ERROR;
                }

                break;

            case SNMP_OCTETSTR:
            case SNMP_OPAQUE:
                if (!Asn1OtsDec (Asn1, End, Obj->Syntax.BufChr,
                                 SNMP_SIZE_BUFCHR, &Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                break;
            case SNMP_NULL:
                if (!Asn1NulDec (Asn1, End))
                    status = SNMP_ERROR;
                break;
            case SNMP_OBJECTID:
                if (!Asn1OjiDec (Asn1, End, Obj->Syntax.BufInt,
                                 SNMP_SIZE_BUFINT, &Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                break;
            case SNMP_IPADDR:
                if (!Asn1OtsDec (Asn1, End, (UINT8 *)&Obj->Syntax.LngUns,
                                 4, &Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                if (Obj->SyntaxLen != 4)
                    status = SNMP_ERROR;

                break;
            case SNMP_COUNTER:
            case SNMP_GAUGE:
            case SNMP_TIMETICKS:
                if (!Asn1IntDecLngUns (Asn1, End, &Obj->Syntax.LngUns))
                    status = SNMP_ERROR;
                break;
            default:
                status = SNMP_ERROR;
        }

        if ((status == NU_SUCCESS) && (!Asn1EocDec (Asn1, Eoc)) )
            status = SNMP_ERROR;

    }

    return (status);

} /* SNMP_V1_ObjDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_LstEnc
*
*   DESCRIPTION
*
*       This function encodes the variable binding list.
*
*   INPUTS
*
*       *Asn1       The asn1_sck_t data structure used to encode the
*                   data.
*       *Lst        The data to be encoded.
*       LstLen      The length of the data.
*
*   OUTPUTS
*
*       NU_SUCCESS     The data is syntactically correct.
*       SNMP_ERROR    The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V1_LstEnc (asn1_sck_t *Asn1, snmp_object_t *Lst,
                       UINT32 LstLen)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *Eoc;

    if (!Asn1EocEnc (Asn1, &Eoc))
        status = SNMP_ERROR;

    else
    {
        Lst += LstLen;

        while ( (LstLen-- > 0) && (status == NU_SUCCESS) )
        {
            status = SNMP_V1_ObjEnc(Asn1, --Lst);
        }

        if (!Asn1HdrEnc (Asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
            status = SNMP_ERROR;
    }

    return (status);

} /* SNMP_V1_LstEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_LstDec
*
*   DESCRIPTION
*
*       This function decodes the variable bindings.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Lst        A pointer to the data to be decoded.
*       LstSze      The size of the data to be decoded.
*       *LstLen     The size of the decoded data.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_LstDec (asn1_sck_t *Asn1, snmp_object_t *Lst,
                       UINT32 LstSze, UINT32 *LstLen)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *Eoc;
    UINT32  Cls, Con, Tag;
    UINT32  listLength = 0;

    if (!Asn1HdrDec (Asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
    {
        status = SNMP_ERROR;
    }

    else
    {

        while (!Asn1Eoc (Asn1, Eoc))
        {
            if (++listLength > LstSze)
            {
                status = SNMP_TOOBIG;
                break;
            }
            if ((status = SNMP_V1_ObjDec (Asn1, Lst++)) != NU_SUCCESS)
                break;
        }

        if ( (status == NU_SUCCESS) && (!Asn1EocDec (Asn1, Eoc)) )
            status = SNMP_ERROR;
    }

    *LstLen = listLength;
    return (status);

} /* SNMP_V1_LstDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_RqsEnc
*
*   DESCRIPTION
*
*       This function checks the syntax of an outgoing snmp_request_t
*       variable.
*
*   INPUTS
*
*       *Asn1       The asn1_sck_t data structure used to encode the
*                   data.
*       *Rqs        The data to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS     The data is syntactically correct.
*       SNMP_ERROR    The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V1_RqsEnc(asn1_sck_t *Asn1, const snmp_request_t *Rqs)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *End;

    if (!Asn1IntEncUns (Asn1, &End, Rqs->ErrorIndex))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    else if (!Asn1IntEncUns (Asn1, &End, Rqs->ErrorStatus))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    else if (!Asn1IntEnc (Asn1, &End, (INT32)Rqs->Id))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    return (status);

} /* SNMP_V1_RqsEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_RqsDec
*
*   DESCRIPTION
*
*       This function decodes Get-Request PDUs.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Rqs        A pointer to the data to be decoded.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_RqsDec (asn1_sck_t *Asn1, snmp_request_t *Rqs)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *End;
    UINT32  Cls, Con, Tag;

    if (!Asn1HdrDec(Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1IntDec(Asn1, End, (INT32*)&Rqs->Id))
        status = SNMP_ERROR;

    else if (!Asn1HdrDec(Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1IntDecUns(Asn1, End, &Rqs->ErrorStatus))
        status = SNMP_ERROR;

    else if (!Asn1HdrDec(Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1IntDecUns(Asn1, End, &Rqs->ErrorIndex))
        status = SNMP_ERROR;


    /*should not have any value in ErrorStatus and ErrorIndex fields*/
    Rqs->ErrorStatus = SNMP_NOERROR;
    Rqs->ErrorIndex = SNMP_NOERROR;
    NU_BLOCK_COPY(&Snmp_V1_ErrAsn1, Asn1, sizeof(asn1_sck_t));

    return (status);

} /* SNMP_V1_RqsDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_TrpEnc
*
*   DESCRIPTION
*
*       This function checks the syntax of an outgoing snmp_trap_t
*       variable.
*
*   INPUTS
*
*       *Asn1       The asn1_sck_t data structure used to encode the
*                   data.
*       *Trp        The data to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS     The data is syntactically correct.
*       SNMP_ERROR    The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V1_TrpEnc(asn1_sck_t *Asn1, const snmp_trap_t *Trp)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *End;

    if (!Asn1IntEncLngUns (Asn1, &End, Trp->Time))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_APL, ASN1_PRI, SNMP_TIT))
        status = SNMP_ERROR;

    else if (!Asn1IntEncUns (Asn1, &End, Trp->Specific))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    else if (!Asn1IntEncUns (Asn1, &End, Trp->General))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    else if (!Asn1OtsEnc (Asn1, &End, Trp->IpAddress, Trp->IpAddressLen))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_APL, ASN1_PRI, SNMP_IPA))
        status = SNMP_ERROR;

    else if (!Asn1OjiEnc (Asn1, &End, Trp->Id, Trp->IdLen))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OJI))
        status = SNMP_ERROR;

    return (status);

} /* SNMP_V1_TrpEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_PduEnc
*
*   DESCRIPTION
*
*       This function checks the syntax of an outgoing snmp_object_t
*       variable.
*
*   INPUTS
*
*       *Asn1       The asn1_sck_t data structure used to encode the
*                   data.
*       *Pdu        The data to be encoded.
*       *Lst        The data to be encoded.
*       LstLen      The length of the data.
*
*   OUTPUTS
*
*       NU_SUCCESS     The data is syntactically correct.
*       SNMP_ERROR    The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V1_PduEnc (asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstLen)
{
    STATUS    status;
    UINT8   *Eoc;

    /* If this is a Trap PDU, then ignore the first two objects as they
     * are not sent with this PDU.
     */
    if(Pdu->Type == SNMP_PDU_TRAP_V1)
    {
        Lst += 2;
        LstLen -=2;
    }

    if (!Asn1EocEnc (Asn1, &Eoc))
        status = SNMP_ERROR;

    else if ((status = SNMP_V1_LstEnc (Asn1, Lst, LstLen)) != NU_SUCCESS);

    else
    {
        switch (Pdu->Type)
        {
        case SNMP_PDU_GET:
        case SNMP_PDU_NEXT:
        case SNMP_PDU_RESPONSE:
        case SNMP_PDU_SET:
            status = SNMP_V1_RqsEnc (Asn1, &Pdu->Request);
            break;
        case SNMP_PDU_TRAP_V1:
            status = SNMP_V1_TrpEnc (Asn1, &Pdu->Trap);
            break;
        default:
            status = SNMP_ERROR;
        }

        if ( (status == NU_SUCCESS) &&
             (!Asn1HdrEnc (Asn1, Eoc, ASN1_CTX, ASN1_CON, Pdu->Type)) )
            status = SNMP_ERROR;
    }

    return (status);

} /* SNMP_V1_PduEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_PduDec
*
*   DESCRIPTION
*
*       This function decodes the PDU.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Lst        A pointer to the data to be decoded.
*       LstSze      The size of the data to be decoded.
*       *LstLen     The size of the decoded data.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_PduDec (asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstSze, UINT32 *LstLen)
{
    STATUS  status;
    UINT8   *Eoc;
    UINT32  Cls, Con;

    if (!Asn1HdrDec (Asn1, &Eoc, &Cls, &Con, &Pdu->Type))
    {
        status = SNMP_ERROR;
    }

    else if ((Eoc == NU_NULL) || Con != ASN1_CON || Cls != ASN1_CTX )
    {
        status = SNMP_ERROR;
    }

    else
    {
        switch (Pdu->Type)
        {
        case SNMP_PDU_GET:
        case SNMP_PDU_NEXT:
        case SNMP_PDU_RESPONSE:
        case SNMP_PDU_SET:
            status = SNMP_V1_RqsDec (Asn1, &Pdu->Request);
            break;
        default:
            SnmpStat.InBadTypes++;
            status = SNMP_ERROR;
        }

        if (status == NU_SUCCESS)
        {
            status = SNMP_V1_LstDec (Asn1, Lst, LstSze, LstLen);

            if (status == NU_SUCCESS)
            {
                if (!Asn1EocDec (Asn1, Eoc))
                    status = SNMP_ERROR;
            }
        }
    }

    return (status);

} /* SNMP_V1_PduDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Enc
*
*   DESCRIPTION
*
*       This function Encodes an outgoing SNMPv1 Response Message.
*
*   INPUTS
*
*       *snmp_response         Contains the buffer where the response will
*                              be placed.
*       *snmp_session          Contains information to be used for
*                              encoding.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_Enc(SNMP_MESSAGE_STRUCT *snmp_response,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS        status;
    UINT8         *Eoc, *End;
    asn1_sck_t    Asn1;

    Asn1Opn (&Asn1, snmp_response->snmp_buffer, SNMP_BUFSIZE, ASN1_ENC);

    if (!Asn1EocEnc (&Asn1, &Eoc))
        status = SNMP_ERROR;

    else if ((status = SNMP_V1_PduEnc(&Asn1, &(snmp_session->snmp_pdu),
                                snmp_session->snmp_object_list,
                                snmp_session->snmp_object_list_len))
                                                           != NU_SUCCESS);

    else if (!Asn1OtsEnc (&Asn1, &End, snmp_session->snmp_security_name,
                (UINT32)strlen((CHAR *)snmp_session->snmp_security_name)))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrEnc (&Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OTS))
        status = SNMP_ERROR;

    else if (!Asn1IntEncUns (&Asn1, &End, SNMP_VERSION_V1))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (&Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    else if (!Asn1HdrEnc (&Asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
        status = SNMP_ERROR;

    else
    {
        Asn1Cls(&Asn1, &(snmp_response->snmp_buffer),
                &(snmp_response->snmp_buffer_len));

        switch (snmp_session->snmp_pdu.Type)
        {
        case SNMP_PDU_GET:
            SnmpStat.OutGetRequests++;
            break;
        case SNMP_PDU_NEXT:
            SnmpStat.OutGetNexts++;
            break;
        case SNMP_PDU_RESPONSE:
            SnmpStat.OutGetResponses++;
            break;
        case SNMP_PDU_SET:
            SnmpStat.OutSetRequests++;
            break;
        case SNMP_PDU_TRAP_V1:
            SnmpStat.OutTraps++;
            break;
        }
    }

    /* If an error occurred while encoding, did it occur because we had
       exceeded the Buffer? */
    if(status == SNMP_ERROR && Asn1.Pointer <= Asn1.Begin)
    {
        /* If so, we need to send an error response. */
        SnmpStat.OutTooBigs++;
        snmp_session->snmp_pdu.Request.ErrorStatus = SNMP_TOOBIG;
        status = SNMP_TOOBIG;
    }


    return (status);
} /* SNMP_V1_Enc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Dec
*
*   DESCRIPTION
*
*       This function checks the decodes an incoming request.
*
*   INPUTS
*
*       *snmp_request           Pointer to SNMP_MESSAGE_STRUCT, which
*                               contains the SNMP Message.
*       *snmp_session           Pointer to SNMP_SESSION_STRUCT, which
*                               contains information decoded from the
*                               SNMP Message.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_Dec(SNMP_MESSAGE_STRUCT *snmp_request,
                   SNMP_SESSION_STRUCT *snmp_session)
{
    STATUS     status;
    asn1_sck_t asn1;
    UINT8      *Eoc, *End;
    UINT32     Cls, Con, Tag;
    UINT32     length;

#if (SNMP_SIZE_BUFCHR > (SNMP_SIZE_SMALLOBJECTID * 2))
    UINT8      security_param[SNMP_SIZE_BUFCHR + SNMP_MAX_IP_ADDRS + 7];

    /* Clear the security parameters. */
    UTL_Zero(security_param, SNMP_SIZE_BUFCHR + SNMP_MAX_IP_ADDRS + 7);

#else
    UINT8      security_param[(SNMP_SIZE_SMALLOBJECTID * 2) + 7 +
                                                       SNMP_MAX_IP_ADDRS];

    /* Clear the security parameters. */
    UTL_Zero(security_param, (SNMP_SIZE_SMALLOBJECTID * 2) + 7 +
                                                       SNMP_MAX_IP_ADDRS);
#endif

    Asn1Opn(&asn1, snmp_request->snmp_buffer,
            snmp_request->snmp_buffer_len, ASN1_DEC);

    /* Decoding the Message. */
    if (!Asn1HdrDec (&asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1HdrDec (&asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /* Get the message processing model. */
    else if (!Asn1IntDecUns (&asn1, End, &(snmp_session->snmp_mp)))
        status = SNMP_ERROR;

    else if (snmp_session->snmp_mp != SNMP_VERSION_V1)
        status = SNMP_ERROR;

    else if (!Asn1HdrDec (&asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OTS))
    {
        status = SNMP_ERROR;
    }

    else if (!Asn1OtsDec (&asn1, End, security_param,
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
        security_param[length + 3] =
              (UINT8)(snmp_request->snmp_transport_address.family & 0xff);
        security_param[length + 4] =
                (UINT8)((snmp_request->snmp_transport_address.family >>
                                                               8) & 0xff);
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

        /* Does the security check pass ? */
        if(status == NU_SUCCESS)
        {
            /* Get the context engine id and the context name. */
            snmp_session->snmp_context_engine_id_len =
                                                GET32(security_param, 0);
            NU_BLOCK_COPY(snmp_session->snmp_context_engine_id,
                          &security_param[4],
                  (unsigned int)snmp_session->snmp_context_engine_id_len);

            strncpy((CHAR*)snmp_session->snmp_context_name,
                    (CHAR*)&security_param[4 +
                               snmp_session->snmp_context_engine_id_len],
					SNMP_SIZE_BUFCHR);

            /* Set the PDU version to 1. */
            snmp_session->snmp_pdu_version = SNMP_PDU_V1;

            /* The Message has been properly authenticated. Now decode the
             * PDU.
             */
            status = SNMP_V1_PduDec (&asn1, &snmp_session->snmp_pdu,
                                     snmp_session->snmp_object_list,
                                     AGENT_LIST_SIZE,
                                     &snmp_session->snmp_object_list_len);

            if ((status == NU_SUCCESS) && (Asn1EocDec (&asn1, Eoc)))
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
                case SNMP_PDU_TRAP_V1:
                    SnmpStat.InTraps++;
                    break;
                }
            }

        }

    }

    return (status);

} /* SNMP_V1_Dec */


/************************************************************************
*
*   FUNCTION
*
*       SNMP_V1_Notification_Enc
*
*   DESCRIPTION
*
*       This function first translates a notification from SNMPv2 format
*       to SNMPv1 format, it calls SNMP_V1_Enc_Respond.
*
*   INPUTS
*
*       *snmp_notification  The notification message.
*       *snmp_session       Pointer to SNMP_SESSION_STRUCT, which contains
*                           information about the notification being
*                           generated.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS SNMP_V1_Notification_Enc(SNMP_MESSAGE_STRUCT *snmp_notification,
                                SNMP_SESSION_STRUCT* snmp_session)
{
    NOTIFICATION_OID    standardTraps[] = {
                                            /* coldStart */
                                            { {1,3,6,1,6,3,1,1,5,1}, 10 },
                                            /* warmStart */
                                            { {1,3,6,1,6,3,1,1,5,2}, 10 },
                                            /* linkDown */
                                            { {1,3,6,1,6,3,1,1,5,3}, 10 },
                                            /* linkUp */
                                            { {1,3,6,1,6,3,1,1,5,4}, 10 },
                                            /* authenticationFailure */
                                            { {1,3,6,1,6,3,1,1,5,5}, 10 },
                                            /* egpNeighborLoss */
                                            { {1,3,6,1,6,3,1,1,5,6}, 10 },
                                          };
    /* defined in RFC1907 */
    NOTIFICATION_OID   snmpTrapEnterprise = {{1,3,6,1,6,3,1,1,4,3,0}, 11};
     /* defined in RFC1907 */
    NOTIFICATION_OID   snmpTraps         = {{1,3,6,1,6,3,1,1,5},      9};

    BOOLEAN          trapEnterpriseFlag = NU_FALSE;

    UINT32             i, j;

    snmp_session->snmp_pdu.Trap.Type = SNMP_PDU_TRAP_V1;

    for(i = 0; i < SNMP_TOTAL_STANDARD_TRAPS; i++)
    {
        /* If snmpTrapOID is one of the standard traps */
        if(MibCmpObjId(standardTraps[i].notification_oid,
                      standardTraps[i].oid_len,
                      snmp_session->snmp_object_list[1].Syntax.BufInt,
                      snmp_session->snmp_object_list[1].SyntaxLen) == 0)
        {
            for( j = 0; j < snmp_session->snmp_object_list_len; j++)
            {
                /* If snmpTrapEnterprise.0 exists in variable bindings of
                 * snmpv2 notification.
                 */
                if (MibCmpObjId(snmpTrapEnterprise.notification_oid,
                                snmpTrapEnterprise.oid_len,
                                snmp_session->snmp_object_list[j].Id,
                                snmp_session->snmp_object_list[j].IdLen)
                                                                     == 0)
                {
                    snmp_session->snmp_pdu.Trap.IdLen =
                              snmp_session->snmp_object_list[j].SyntaxLen;
                    NU_BLOCK_COPY(snmp_session->snmp_pdu.Trap.Id,
                          snmp_session->snmp_object_list[j].Syntax.BufInt,
                          (unsigned int)(sizeof(UINT32) *
                            snmp_session->snmp_object_list[j].SyntaxLen));
                    trapEnterpriseFlag = NU_TRUE;
                    break;  /* break inner for loop */

                } /* end if*/

            } /* end for */

            /* If snmpTrapEnterprise does not exist in variable bindings
             * list.
             */
            if (trapEnterpriseFlag == NU_FALSE)
            {
                snmp_session->snmp_pdu.Trap.IdLen = snmpTraps.oid_len;
                NU_BLOCK_COPY(snmp_session->snmp_pdu.Trap.Id,
                              snmpTraps.notification_oid,
                      (unsigned int)(sizeof(UINT32) * snmpTraps.oid_len));
            } /* end if*/

            snmp_session->snmp_pdu.Trap.General = i;
            snmp_session->snmp_pdu.Trap.Specific = 0;
            break;  /* break outer for loop */

        } /* end if */

    } /* end for */

    /* If snmpTrapOID is not one of the standard traps. */
    if( i >= SNMP_TOTAL_STANDARD_TRAPS)
    {
        if(snmp_session->snmp_object_list[1].Syntax.BufInt[
                    snmp_session->snmp_object_list[1].SyntaxLen - 2] == 0)
        {
            snmp_session->snmp_pdu.Trap.IdLen =
                        snmp_session->snmp_object_list[1].SyntaxLen - 2;
            NU_BLOCK_COPY(snmp_session->snmp_pdu.Trap.Id,
                          snmp_session->snmp_object_list[1].Syntax.BufInt,
                         (unsigned int)(sizeof(UINT32) *
                                      snmp_session->snmp_pdu.Trap.IdLen));
        } /* end if */

        else
        {
            snmp_session->snmp_pdu.Trap.IdLen =
                          snmp_session->snmp_object_list[1].SyntaxLen - 1;
            NU_BLOCK_COPY(snmp_session->snmp_pdu.Trap.Id,
                          snmp_session->snmp_object_list[1].Syntax.BufInt,
                          (unsigned int)(sizeof(UINT32) *
                                      snmp_session->snmp_pdu.Trap.IdLen));
        } /* end else */

        snmp_session->snmp_pdu.Trap.General = SNMP_TRAP_ENTSPECIFIC;
        snmp_session->snmp_pdu.Trap.Specific = 
            snmp_session->snmp_object_list[1].Syntax.
            BufInt[snmp_session->snmp_object_list[1].SyntaxLen - 1];

    } /* end if */

    NU_BLOCK_COPY(snmp_session->snmp_pdu.Trap.IpAddress, cfig_hostid,
                  SNMP_MAX_IP_ADDRS);

    if (cfig_hosttype == 1)
        snmp_session->snmp_pdu.Trap.IpAddressLen = IP_ADDR_LEN;

#if (INCLUDE_IPV6 == NU_TRUE)
    else
        snmp_session->snmp_pdu.Trap.IpAddressLen = IP6_ADDR_LEN;
#endif

    snmp_session->snmp_pdu.Trap.Time =
                snmp_session->snmp_object_list[0].Syntax.LngUns;

    return SNMP_V1_Enc_Respond(snmp_notification, snmp_session);

} /* SNMP_V1_Notification_Enc */

#endif /* (INCLUDE_SNMPv1 == NU_TRUE) */




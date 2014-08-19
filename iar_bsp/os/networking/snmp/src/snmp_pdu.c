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
*       snmp_pdu.c                                               
*
*   DESCRIPTION
*
*       This file contains the implementation of functions specific to
*       encoding / decoding / of version 2 PDU.
*
*
*   DATA STRUCTURES
*
*       Snmp_V2_ErrAsn1
*
*   FUNCTIONS
*
*       SNMP_V2_ObjEnc
*       SNMP_V2_ObjDec
*       SNMP_V2_LstEnc
*       SNMP_V2_LstDec
*       SNMP_V2_BulkEnc
*       SNMP_V2_BulkDec
*       SNMP_V2_RqsEnc
*       SNMP_V2_RqsDec
*       SNMP_V2_PduEnc
*       SNMP_V2_PduDec
*
*   DEPENDENCIES
*
*       target.h
*       snmp.h
*       asn1.h
*       snmp_mp.h
*       snmp_pdu.h
*       snmp_g.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/snmp.h"
#include "networking/asn1.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_pdu.h"
#include "networking/snmp_g.h"

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )

extern snmp_stat_t                  SnmpStat;
asn1_sck_t                          Snmp_V2_ErrAsn1;

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_ObjEnc
*
*   DESCRIPTION
*
*       This function checks the syntax of an outgoing snmp_object_t
*       variable.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure used to
*                   encode the data.
*       *Obj        A pointer to the data to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_ObjEnc(asn1_sck_t *Asn1, snmp_object_t *Obj)
{
    STATUS        status = NU_SUCCESS;
    UINT32      Cls, Tag;
    UINT8       *Eoc, *End = NU_NULL;

    if (Asn1EocEnc (Asn1, &Eoc))
    {

    /*first encode the value depending upon its type*/
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

            case SNMP_NOSUCHOBJECT:
            case SNMP_NOSUCHINSTANCE:
            case SNMP_ENDOFMIBVIEW:
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

            case SNMP_COUNTER64:
                if (!Asn1EncCounter64 (Asn1, &End, Obj->Syntax.BigInt))
                    status = SNMP_ERROR;
                break;

            default:
                SnmpStat.OutBadValues++;
                status = SNMP_ERROR;
        }
    }
    else
        status = SNMP_ERROR;




    /* If value part is properly encoded then append the header of value
     * and append the encode the Object Identifier and append the header
     * for Object Identifier.
     */
    if (status == NU_SUCCESS)
    {
        /* On providing the Object Type retrieve the Tag and Class for
         * appending the header.
         */
        if (!SNMP_Syn2TagCls (&Tag, &Cls, Obj->Type))
            status = SNMP_ERROR;

        /* Append the header for value of object identifier with Class,
         * Primitive and Tag.
         */
        else if (!Asn1HdrEnc (Asn1, End, Cls, ASN1_PRI, Tag))
            status = SNMP_ERROR;

        /* By providing Object Identifier and its length encode the Object
         * Identifier.
         */
        else if (!Asn1OjiEnc (Asn1, &End, Obj->Id, Obj->IdLen))
            status = SNMP_ERROR;

        /*append the header for Object Identifier by providing its
          attributes(ASN1_UNI, ASN1_PRI, ASN1_OJI)*/
        else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_OJI))
            status = SNMP_ERROR;

        /*append the header for variable binding list*/
        else if (!Asn1HdrEnc (Asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
            status = SNMP_ERROR;
    }

    return (status);

} /* SNMP_V2_ObjEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_ObjDec
*
*   DESCRIPTION
*
*       This function checks the syntax of an incoming snmp_object_t
*       variable.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Obj        A pointer to the data to be decoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_ObjDec(asn1_sck_t *Asn1, snmp_object_t *Obj)
{
    STATUS    status = NU_SUCCESS;

    UINT8   *Eoc;   /* Represents the end of Object Identifier and its
                     * value both.
                     */

    UINT8   *End;   /*frequently used*/
                    /* Either represents the end of Object Identifier or
                     * the end of the value part of the corresponding
                     * Object Identifier.
                     */

    UINT32  Cls;    /*to store the value of a Class*/

    UINT32  Con;    /*to check whether it is primitive or constructive*/

    UINT32  Tag;    /*to store the value of Tag*/


    /*decode the header expecting the tag for sequence*/
    if (!Asn1HdrDec (Asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes for constructive,sequence and universal*/
    else if ((Eoc == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_CON) ||
             (Tag != ASN1_SEQ))
        status = SNMP_ERROR;

    /*decode the header expecting the tag for object identifier*/
    else if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes for primitive and tag as an object identifier*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_OJI))
    {
        status = SNMP_ERROR;
    }

    /*decode the object identifier*/
    else if (!Asn1OjiDec (Asn1, End, Obj->Id, SNMP_SIZE_OBJECTID,
                          &Obj->IdLen))
    {
        status = ASN1_ERR_DEC_EOC_MISMATCH;
    }

    /*decode the header*/
    else if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*at this stage it is expecting for value so Con must be primitive */
    /*if not primitive then update the SNMP statistics*/
    else if ((End == NU_NULL) || Con)
    {
        status = ASN1_ERR_DEC_EOC_MISMATCH;
    }

    /*depending upon the tag and class its syntax will be determined and
    stored in Obj->Type (e.g. INTEGER,OCTETSTR,OPAQUE,IPADD,COUNTER etc)
    used later on to find the value of the Object Identifier*/
    else if (!SNMP_TagCls2Syn (Tag, Cls, &Obj->Type))
    {
        status = SNMP_ERROR;
    }

    else
    {
        /* Process the Obj->Type accordingly */
        switch (Obj->Type)
        {
            case SNMP_INTEGER:
                /*passing in the Buffer of 4 bytes to store value*/
                if (!Asn1IntDecLng (Asn1, End,
                                    (UINT32 *)&Obj->Syntax.LngInt))
                {
                    status = SNMP_ERROR;
                }

                break;

            case SNMP_OCTETSTR:
            case SNMP_OPAQUE:
                /* Passing in the buffer of size SNMP_SIZE_BUFCHR for
                 * Octet String or opaque data.
                 */
                if (!Asn1OtsDec (Asn1, End, Obj->Syntax.BufChr,
                                 SNMP_SIZE_BUFCHR, &Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                break;
            case SNMP_NULL:
                /*decodes null*/
                if (!Asn1NulDec (Asn1, End))
                    status = SNMP_ERROR;
                break;
            case SNMP_OBJECTID:
                /* Passing in the buffer of size SNMP_SIZE_BUFINT for
                 * value of Object Identifier. */
                if (!Asn1OjiDec (Asn1, End, Obj->Syntax.BufInt,
                                 SNMP_SIZE_BUFINT, &Obj->SyntaxLen))
                {
                    status = SNMP_ERROR;
                }

                break;

            case SNMP_IPADDR:
                /*passing in the buffer for IP address*/
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
                /* Passing in the buffer to store the value for Counter,
                 * Gauge or TimeTicks.
                 */
                if (!Asn1IntDecLngUns (Asn1, End, &Obj->Syntax.LngUns))
                    status = SNMP_ERROR;
                break;

            case SNMP_COUNTER64:
                /* Calling the routine from Asn1 for decoding value of
                 * Counter64.
                 */

                /* Passing in the buffer to store the value for
                 * Counter64.
                 */
                if (!Asn1DecCounter64(Asn1, End, &(Obj->Syntax.BigInt)))
                    status = SNMP_ERROR;
                break;
            default:
                /*for none of above update the SNMP statistics*/
                status = SNMP_ERROR;
        }

        if ( (status == NU_SUCCESS) && (!Asn1EocDec (Asn1, Eoc)) )
            status = SNMP_ERROR;
    }

    return (status);

} /* SNMP_V2_ObjDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_LstEnc
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
*       *Lst        The data to be encoded.
*       LstLen      The length of the data.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_LstEnc(asn1_sck_t *Asn1, snmp_object_t *Lst, UINT32 LstLen)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *Eoc;

    /*mark the pointer Eoc to the end of the variable binding list*/
    if (!Asn1EocEnc (Asn1, &Eoc))
        status = SNMP_ERROR;

    else
    {
        /*moving the pointer to the end of the Object Identifier's list*/
        Lst += LstLen;

        /*run the loop until all Objects are finished in the list*/
        while ( (LstLen-- > 0) && (status == NU_SUCCESS) )
        {
            /*calling the function to encode the object*/
            if (SNMP_V2_ObjEnc(Asn1, --Lst) != NU_SUCCESS)
                status = SNMP_ERROR;
        }

        if (status == NU_SUCCESS) 
        {
            /*when the complete list of Object Identifiers is encoded add the
            header of the Seq (format prescribed by RFC 1905)*/
            if (!Asn1HdrEnc (Asn1, Eoc, ASN1_UNI, ASN1_CON, ASN1_SEQ))
                status = SNMP_ERROR;
        }
    }

    return (status);

} /* SNMP_V2_LstEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_LstDec
*
*   DESCRIPTION
*
*       This function checks the syntax of an incoming snmp_object_t
*       variable.
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
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_LstDec(asn1_sck_t *Asn1, snmp_object_t *Lst, UINT32 LstSze,
                 UINT32 *LstLen)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *Eoc;   /* A Pointer to the end of the buffer(end of variable
                     * bindings list).
                     */

    UINT32  Cls;    /*to store the value of a Class*/
    UINT32  Con;    /*to check whether it is primitive or constructive*/
    UINT32  Tag;    /*to store the value of Tag*/

    UINT32 listLength = 0;


    /*decode the header and extracts the tag*/
    if (!Asn1HdrDec (Asn1, &Eoc, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*condition to check if it is universal, constructive and sequence*/
    else if ((Eoc == NU_NULL) || (Tag != ASN1_SEQ) || (Cls != ASN1_UNI) ||
             (Con != ASN1_CON) )
        status = SNMP_ERROR;

    else
    {
        listLength = 0;

        while (!Asn1Eoc (Asn1, Eoc))
        {
            if (++listLength > LstSze)
            {
                status = SNMP_ERROR;
                break;
            }
            if ((status = SNMP_V2_ObjDec (Asn1, Lst++)) != NU_SUCCESS)
                break;
        }

        if ( (status == NU_SUCCESS) && (!Asn1EocDec (Asn1, Eoc)) )
            status = SNMP_ERROR;
    }


    *LstLen = listLength;
    return (status);

} /* SNMP_V2_LstDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_BulkEnc
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
*       *Blk        The data to be encoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_BulkEnc(asn1_sck_t *Asn1, const SNMP_BULK_STRUCT *Blk)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *End;

    /*encode the unsigned number for max repetitions*/
    if (!Asn1IntEnc (Asn1, &End, Blk->snmp_max_repetitions))
        status = SNMP_ERROR;

    /* Append the header for max repetitions with its attributes
     * (ASN1_UNI, ASN1_PRI, ASN1_INT).
     */
    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    /*encode the unsigned number for non repeaters*/
    else if (!Asn1IntEnc (Asn1, &End, Blk->snmp_non_repeaters))
        status = SNMP_ERROR;

    /* Append the header for non repeaters with its attributes
     * (ASN1_UNI, ASN1_PRI, ASN1_INT).
     */
    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    /*encode the signed number for Request Id*/
    else if (!Asn1IntEnc (Asn1, &End, (INT32)Blk->snmp_id))
        status = SNMP_ERROR;

    /* Append the header for Request Id with its attributes
     * (ASN1_UNI, ASN1_PRI, ASN1_INT).
     */
    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    return (status);

} /* SNMP_V2_BulkEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_BulkDec
*
*   DESCRIPTION
*
*       This function decodes and checks the syntax of an incoming
*       snmp_request_t variable.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Blk        A pointer to the data to be decoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_BulkDec(asn1_sck_t *Asn1, SNMP_BULK_STRUCT *Blk)
{
    STATUS    status = NU_SUCCESS;

    UINT8   *End;   /*pointer used to place the end of the each field*/

    UINT32  Cls;    /*to store the value of a Class*/
    UINT32  Con;    /*to check whether it is primitive or constructive*/
    UINT32  Tag;    /*to store the value of Tag*/


    /*decode the header expecting tag for integer here for request id*/
    if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes if it is primitive,integer and universal*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /*decode and place the value of "bulk id" in Blk->snmp_id*/
    else if (!Asn1IntDec(Asn1, End, (INT32*)&Blk->snmp_id))
        status = SNMP_ERROR;

    /*decode the header expecting tag for integer here*/
    else if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check if it is primitive,integer and universal*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /*store the value of "non repeaters" in Blk->snmp_non_repeaters*/
    else if (!Asn1IntDec (Asn1, End, &Blk->snmp_non_repeaters))
        status = SNMP_ERROR;

    /*decode the header expecting tag for integer here*/
    else if (!Asn1HdrDec (Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes if it is primitive,integer and universal*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /*store the value of "max repetitions" in Blk->snmp_max_repetitions*/
    else if (!Asn1IntDec (Asn1, End, &Blk->snmp_max_repetitions))
        status = SNMP_ERROR;

    NU_BLOCK_COPY(&Snmp_V2_ErrAsn1, Asn1, sizeof(asn1_sck_t));

    return (status);

} /* SNMP_V2_BulkDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_RqsEnc
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
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_RqsEnc(asn1_sck_t *Asn1, const snmp_request_t *Rqs)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *End;

    if (Rqs->Type == SNMP_PDU_TRAP_V2)
    {
    /*encode the unsigned number for Error Index*/
        if (!Asn1IntEncUns (Asn1, &End, 0))
            status = SNMP_ERROR;
    }
    else
    {
    if (!Asn1IntEncUns (Asn1, &End, Rqs->ErrorIndex))
        status = SNMP_ERROR;
    }

    if (status != NU_SUCCESS);
    /* Append the header for Error Index with its attributes
     * (ASN1_UNI, ASN1_PRI, ASN1_INT).
     */
    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    else
    {
        if (Rqs->Type == SNMP_PDU_TRAP_V2)
        {
    /*encode the unsigned number for Error status*/
            if (!Asn1IntEncUns (Asn1, &End, 0))
                status = SNMP_ERROR;
        }
        else
        {
            if (!Asn1IntEncUns (Asn1, &End, Rqs->ErrorStatus))
                status = SNMP_ERROR;
        }
    }    
    if (status != NU_SUCCESS);

    /* Append the header for Error status with its attributes
     * (ASN1_UNI, ASN1_PRI, ASN1_INT).
     */
    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    /*encode the signed number for Request Id*/
    else if (!Asn1IntEnc (Asn1, &End, (INT32)Rqs->Id))
        status = SNMP_ERROR;

    /* Append the header for Request Id with its attributes
     * (ASN1_UNI, ASN1_PRI, ASN1_INT).
     */
    else if (!Asn1HdrEnc (Asn1, End, ASN1_UNI, ASN1_PRI, ASN1_INT))
        status = SNMP_ERROR;

    return (status);

} /* SNMP_V2_RqsEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_RqsDec
*
*   DESCRIPTION
*
*       This function decodes and checks the syntax of an incoming
*       snmp_request_t variable.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Rqs        A pointer to the data to be decoded.
*
*   OUTPUTS
*
*       NU_SUCCESS      The data is syntactically correct.
*       SNMP_ERROR      The data is not syntactically correct.
*
*************************************************************************/
STATUS SNMP_V2_RqsDec(asn1_sck_t *Asn1, snmp_request_t *Rqs)
{
    STATUS   status = NU_SUCCESS;

    UINT8   *End;   /* Pointer used to place in the end of different PDU
                     * fields.
                     */

    UINT32  Cls;    /*to store the value of a Class*/

    UINT32  Con;    /*to check whether it is primitive or constructive*/

    UINT32  Tag;    /*to store the value of Tag*/


    /*decode the header expecting tag for integer here for request id*/
    if (!Asn1HdrDec(Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes if it is primitive, integer and universal*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /*decoding and placing the value of "request id" in Rqs->Id*/
    else if (!Asn1IntDec(Asn1, End,(INT32*) &Rqs->Id))
        status = SNMP_ERROR;

    /*decode the header expecting tag for integer here for error status*/
    else if (!Asn1HdrDec(Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes if it is primitive ,integer and universal*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /*store the value of "Error Status" in Rqs->ErrorStatus*/
    else if (!Asn1IntDecUns(Asn1, End, &Rqs->ErrorStatus))
        status = SNMP_ERROR;

    /*decode the header expecting tag for integer here for error index*/
    else if (!Asn1HdrDec(Asn1, &End, &Cls, &Con, &Tag))
        status = SNMP_ERROR;

    /*check the attributes if it is primitive,integer and universal*/
    else if ((End == NU_NULL) || (Cls != ASN1_UNI) || (Con != ASN1_PRI) ||
             (Tag != ASN1_INT))
    {
        status = SNMP_ERROR;
    }

    /*store the value of "Error index" in Rqs->ErrorIndex*/
    else if (!Asn1IntDecUns(Asn1, End, &Rqs->ErrorIndex))
        status = SNMP_ERROR;

    /*should not have any value in ErrorStatus and ErrorIndex fields*/
    Rqs->ErrorStatus = SNMP_NOERROR;
    Rqs->ErrorIndex = SNMP_NOERROR;
    NU_BLOCK_COPY(&Snmp_V2_ErrAsn1, Asn1, sizeof(asn1_sck_t));

    return (status);

} /* SNMP_V2_RqsDec */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_PduEnc
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
*       NU_SUCCESS      The incoming SNMP-message is syntactically
*                       correct.
*       SNMP_ERROR      The incoming SNMP-message is not syntactically
*                       correct.
*************************************************************************/
STATUS SNMP_V2_PduEnc (asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstLen)
{
    STATUS    status = NU_SUCCESS;
    UINT8   *Eoc;

    /*mark the pointer Eoc to the end of the PDU*/
    if (!Asn1EocEnc (Asn1, &Eoc))
        status = SNMP_ERROR;

    /* Call the function to encode the variable binding list in reverse
     * order (start from last variable in variable binding list.
     */
    else if (SNMP_V2_LstEnc(Asn1, Lst, LstLen) != NU_SUCCESS)
        status = SNMP_ERROR;

    else
    {
        /* Append the attributes (request id,error status,error index) if
         * it is one of (GET, GETNEXT, RESPONSE, SET, INFORM, TRAP,
         * REPORT).
         */
        switch (Pdu->Type)
        {
        case SNMP_PDU_GET:
        case SNMP_PDU_NEXT:
        case SNMP_PDU_RESPONSE:
        case SNMP_PDU_SET:
        case SNMP_PDU_INFORM:
        case SNMP_PDU_TRAP_V2:
        case SNMP_PDU_REPORT:
            if (SNMP_V2_RqsEnc(Asn1, &Pdu->Request) != NU_SUCCESS)
            {
                status = SNMP_ERROR;
            }
            break;

        /* Append the attributes (request id,non repeaters ,max repeaters)
         * if it is GETBULK.
         */
        case SNMP_PDU_BULK:
            if (SNMP_V2_BulkEnc(Asn1, &Pdu->Bulk) != NU_SUCCESS)
            {
                status = SNMP_ERROR;
            }
            break;
        default:
            status = SNMP_ERROR;
        }


        /*after successful encoding of all append the header for PDU with
        attributes (ASN1_CTX, ASN1_CON, PDU->Type)*/
        if ( (status == NU_SUCCESS) &&
             (!Asn1HdrEnc (Asn1, Eoc, ASN1_CTX, ASN1_CON, Pdu->Type)) )

            status = SNMP_ERROR;
    }

    return (status);

} /* SNMP_V2_PduEnc */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_V2_PduDec
*
*   DESCRIPTION
*
*       This function decodes and checks the syntax of an incoming
*       snmp_object_t variable.
*
*   INPUTS
*
*       *Asn1       A pointer to the asn1_sck_t data structure that will
*                   be used to decode the data.
*       *Lst        Pointer to the data structure which will store
*                   (Object Identifiers,values)
*       LstSze      max number of pairs (Object Identifiers, Value)
*       *LstLen     number of pairs (Object Identifiers, Value) present in
*                   message.
*
*   OUTPUTS
*
*       NU_SUCCESS      The incoming SNMP-message is syntactically
*                       correct.
*       SNMP_ERROR      The incoming SNMP-message is not syntactically
*                       correct.
*************************************************************************/
STATUS SNMP_V2_PduDec (asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstSze, UINT32 *LstLen)
{
    STATUS    status = NU_SUCCESS;

    UINT8   *Eoc;   /*pointer used to place in the end of PDU field*/

    UINT32  Cls;    /*variable used for Class(universal /application)*/

    UINT32  Con;    /*used to check whether this particular section is */
                    /*constructive or primitive*/



    /* Decode the tag which will tell the type of the PDU. is it get,
     * getnext? etc.
     */
    if (!Asn1HdrDec (Asn1, &Eoc, &Cls, &Con, &Pdu->Type))
        status = SNMP_ERROR;

    /* Check for context (if Cls is not context then condition is false.
     */

    /* If Eoc is Null that means there is no length for PDU therefore
     * return false.
     */
    else if ((Eoc == NU_NULL) || (Cls != ASN1_CTX) || (Con != ASN1_CON) )
        status = SNMP_ERROR;

    else
    {

        /*calling the respective function depending upon PDU Type*/
        switch (Pdu->Type)
        {

        /* For all of the following cases "Snmp_V2_Request_Decode(..)"
         * function will be called for proper scanning of request-id,
         * error-status,error-index.
         */
        case SNMP_PDU_GET:
        case SNMP_PDU_NEXT:
        case SNMP_PDU_RESPONSE:
        case SNMP_PDU_SET:
        case SNMP_PDU_INFORM:
        case SNMP_PDU_TRAP_V2:
        case SNMP_PDU_REPORT:
            if (SNMP_V2_RqsDec(Asn1, &Pdu->Request) != NU_SUCCESS)
            {
                status = SNMP_ERROR;
            }
            break;

        /* In case of get bulk "SNMP_V2_BulkDec(..)" function will be
         * called for proper scanning of request-id,non-repeaters,
         * max-repetitions.
         */
        case SNMP_PDU_BULK:
            if (SNMP_V2_BulkDec(Asn1, &Pdu->Bulk) != NU_SUCCESS)
            {
                status = SNMP_ERROR;
            }
            break;
        /*if type is not one of the defined types*/
        default:
            SnmpStat.InBadTypes++;
            status = SNMP_ERROR;
        }


        /* If the incoming request is properly scanned up to variable
         * binding list then call the function to decode the variable
         * binding list.
         */
        if (status == NU_SUCCESS)
        {
            status = SNMP_V2_LstDec(Asn1, Lst, LstSze, LstLen);

            /*check whether the PDU is completely scanned*/
            if ((status == NU_SUCCESS) && (!Asn1EocDec (Asn1, Eoc)))
                status = SNMP_ERROR;
        }
    }

    return (status);

} /* SNMP_V2_PduDec */

#endif

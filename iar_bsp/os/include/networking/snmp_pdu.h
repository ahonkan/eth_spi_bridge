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
*       snmp_pdu.h                                               
*
*   DESCRIPTION
*
*       This file contains definitions required to decode the version 2
*       PDU.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       snmp.h
*
************************************************************************/
#include "networking/snmp.h"

#ifndef SNMP_PDU_H
#define SNMP_PDU_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/*Encodes the single variable in variable binding list*/
STATUS SNMP_V2_ObjEnc(asn1_sck_t *Asn1, snmp_object_t *Obj);

/*Decodes the single variable in variable binding list*/
STATUS SNMP_V2_ObjDec(asn1_sck_t *Asn1, snmp_object_t *Obj);

/*Encodes the complete variable binding list*/
STATUS SNMP_V2_LstEnc(asn1_sck_t *Asn1, snmp_object_t *Lst,
                      UINT32 LstLen);

/*Decodes the complete variable binding list*/
STATUS SNMP_V2_LstDec(asn1_sck_t *Asn1, snmp_object_t *Lst, UINT32 LstSze,
                      UINT32 *LstLen);

/*Firstly appends Error-Index field then appends Error-Status field
and finally append the Request-Id*/
STATUS SNMP_V2_RqsEnc (asn1_sck_t *Asn1, const snmp_request_t *Rqs);

/*First scans the Request-Id then Error-Status and finally Error-Index */
STATUS SNMP_V2_RqsDec(asn1_sck_t *Asn1, snmp_request_t *Rqs);

/* Append MaxRepetitions field then appends NonRepeaters field and finally
 * append the Request-Id.
 */
STATUS SNMP_V2_BulkEnc(asn1_sck_t *Asn1, const SNMP_BULK_STRUCT *Blk);

/* First scans the Request-Id then NonRepeaters and finally MaxRepetitions
 */
STATUS SNMP_V2_BulkDec(asn1_sck_t *Asn1, SNMP_BULK_STRUCT *Blk);

/* First call the Snmp_List_Encode(..)to encode the variable binding list
 * then call Snmp_V2_Bulk_Encode(..) or Snmp_V2_Request_Encode(..) to
 * append respective attributes(fields)
 */
STATUS SNMP_V2_PduEnc (asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstLen);

/* Firstly callSnmp_V2_Bulk_Decode(..) or Snmp_V2_Request_Decode(..) to
 * extract the respective attributes(fields) then call
 * Snmp_List_Decode(..) to decode variable binding list.
 */
STATUS SNMP_V2_PduDec (asn1_sck_t *Asn1, snmp_pdu_t *Pdu,
                       snmp_object_t *Lst, UINT32 LstSze, UINT32 *LstLen);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_PDU_H */


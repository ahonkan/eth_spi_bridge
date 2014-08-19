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
*       asn1.h                                                   
*
*   DESCRIPTION
*
*       This file contains the macros, data structures and function
*       declarations specific to parsing an ASN.1 object.
*
*   DATA STRUCTURES
*
*       asn1_sck_s
*
*   DEPENDENCIES
*
*       snmp.h
*
************************************************************************/

#ifndef ASN1_H
#define ASN1_H

#include "networking/snmp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/*------------- Class ----------------------*/
#define ASN1_UNI       0     /* Universal   */
#define ASN1_APL       1     /* Application */
#define ASN1_CTX       2     /* Context     */

/*------------- Tag -------------------------------*/
#define ASN1_INT       2     /* Integer            */
#define ASN1_OTS       4     /* Octet String       */
#define ASN1_NUL       5     /* Null               */
#define ASN1_OJI       6     /* Object Identifier  */
#define ASN1_SEQ       16    /* Sequence           */

/*Tag number for for exceptional encoding*/
#define ASN1_NSO       0     /* no such object  (80h = 1000 0000b)*/
#define ASN1_NSI       1     /* no such instance(80h = 1000 0001b)*/
#define ASN1_EMV       2     /* end of MIB view (80h = 1000 0002b)*/

/*------------- Primitive/Constructed -----------------*/
#define ASN1_PRI     0       /* Primitive              */
#define ASN1_CON     1       /* Constructed            */

/*------------- Mode to open ASN11 ---------------*/
#define ASN1_ENC     0       /* Encoding          */
#define ASN1_DEC     1       /* Decoding          */


#define ASN1_ERR_NOERROR                0
#define ASN1_ERR_ENC_FULL               1
#define ASN1_ERR_DEC_EMPTY              2
#define ASN1_ERR_DEC_EOC_MISMATCH       3
#define ASN1_ERR_DEC_LENGTH_MISMATCH    4
#define ASN1_ERR_DEC_BADVALUE           5
#define ASN1_ERR_ENC_BADVALUE           6

typedef struct asn1_sck_s asn1_sck_t;

struct asn1_sck_s
{
    UINT8       *Pointer;           /* Octet just encoded or to be decoded
                                     */
    UINT8       *Begin;             /* First octet                      */
    UINT8       *End;               /* Octet after last octet           */
};

VOID Asn1Opn(asn1_sck_t *, UINT8 *, UINT32, UINT32);
VOID Asn1Cls(const asn1_sck_t *, UINT8 **, UINT32 *);
BOOLEAN Asn1OctEnc(asn1_sck_t *, UINT8);
BOOLEAN Asn1OctDec(asn1_sck_t *, UINT8 *);
BOOLEAN Asn1TagEnc(asn1_sck_t *, UINT32);
BOOLEAN Asn1TagDec(asn1_sck_t *, UINT32 *);
BOOLEAN Asn1IdrEnc(asn1_sck_t *, UINT32, UINT32, UINT32);
BOOLEAN Asn1IdrDec(asn1_sck_t *, UINT32 *, UINT32 *, UINT32 *);
BOOLEAN Asn1LenEnc(asn1_sck_t *, UINT32, UINT32);
BOOLEAN Asn1LenDec(asn1_sck_t *, UINT32 *, UINT32 *);
BOOLEAN Asn1HdrEnc(asn1_sck_t *, const UINT8 *, UINT32, UINT32, UINT32);
BOOLEAN Asn1HdrDec(asn1_sck_t *, UINT8 **, UINT32 *, UINT32 *, UINT32 *);
BOOLEAN Asn1Eoc(const asn1_sck_t *, const UINT8 *);
BOOLEAN Asn1EocEnc(asn1_sck_t *, UINT8 **);
BOOLEAN Asn1EocDec(asn1_sck_t *, const UINT8 *);
BOOLEAN Asn1NulEnc(const asn1_sck_t *, UINT8 **);
BOOLEAN Asn1NulDec(asn1_sck_t *, UINT8 *);
BOOLEAN Asn1BolEnc(asn1_sck_t *, UINT8 **, BOOLEAN);
BOOLEAN Asn1BolDec(asn1_sck_t *, const UINT8 *, BOOLEAN *);
BOOLEAN Asn1IntEnc(asn1_sck_t *, UINT8 **, INT32);
BOOLEAN Asn1IntDec(asn1_sck_t *, const UINT8 *, INT32 *);
BOOLEAN Asn1IntEncLng(asn1_sck_t *, UINT8 **, UINT32);
BOOLEAN Asn1IntDecLng(asn1_sck_t *, const UINT8 *, UINT32 *);
BOOLEAN Asn1IntEncUns(asn1_sck_t *, UINT8 **, UINT32);
BOOLEAN Asn1IntDecUns(asn1_sck_t *, const UINT8 *, UINT32 *);
BOOLEAN Asn1IntEncLngUns(asn1_sck_t *, UINT8 **, UINT32);
BOOLEAN Asn1IntDecLngUns(asn1_sck_t *, const UINT8 *, UINT32 *);
BOOLEAN Asn1BtsEnc(asn1_sck_t *, UINT8 **, const UINT8 *, UINT32, UINT8);
BOOLEAN Asn1BtsDec(asn1_sck_t *, const UINT8 *, UINT8 *, UINT32, UINT32 *,
                UINT8 *);
BOOLEAN Asn1OtsEnc(asn1_sck_t *, UINT8 **, const UINT8 *, UINT32);
BOOLEAN Asn1OtsDec(asn1_sck_t *, const UINT8 *, UINT8 *, UINT32, UINT32 *);
BOOLEAN Asn1SbiEnc(asn1_sck_t *, UINT32);
BOOLEAN Asn1SbiDec(asn1_sck_t *, UINT32 *);
BOOLEAN Asn1OjiEnc(asn1_sck_t *, UINT8 **, const UINT32 *, UINT32);
BOOLEAN Asn1OjiDec(asn1_sck_t *, const UINT8 *, UINT32 *, UINT32, UINT32 *);
BOOLEAN Asn1EncCounter64(asn1_sck_t *Asn1, UINT8 **Eoc, COUNTER64 Int);
BOOLEAN Asn1DecCounter64 (asn1_sck_t *Asn1, const UINT8 *Eoc, COUNTER64 *Int);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

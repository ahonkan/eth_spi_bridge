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
*       1213icmp.h                                               
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the function declarations for operations
*       on ICMP objects.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*
*************************************************************************/
#include "networking/target.h"

#if (RFC1213_ICMP_INCLUDE == NU_TRUE)

#ifndef _1213_ICMP_H_
#define _1213_ICMP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 icmpInMsgs              (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInErrors            (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInDestUnreachs      (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInTimeExcds         (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInParmProbs         (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInSrcQuenchs        (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInRedirects         (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInEchos             (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInEchoReps          (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInTimestamps        (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInTimestampReps     (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInAddrMasks         (snmp_object_t *, UINT16, VOID *);
UINT16 icmpInAddrMaskReps      (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutMsgs             (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutErrors           (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutDestUnreachs     (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutTimeExcds        (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutParmProbs        (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutSrcQuenchs       (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutRedirects        (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutEchos            (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutEchoReps         (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutTimestamps       (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutTimestampReps    (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutAddrMasks        (snmp_object_t *, UINT16, VOID *);
UINT16 icmpOutAddrMaskReps     (snmp_object_t *, UINT16, VOID *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
#endif

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
*       1213snmp.h                                               
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the function declarations for operations
*       on SNMP objects.
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

#if (RFC1213_SNMP_INCLUDE == NU_TRUE)

#ifndef _1213_SNMP_H_
#define _1213_SNMP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 snmpInPkts                 (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutPkts                (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInBadVersions          (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInBadCommunityNames    (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInBadCommunityUses     (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInASNParseErrs         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInTooBigs              (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInNoSuchNames          (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInBadValues            (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInReadOnlys            (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInGenErrs              (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInTotalReqVars         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInTotalSetVars         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInGetRequests          (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInGetNexts             (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInSetRequests          (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInGetResponses         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpInTraps                (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutTooBigs             (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutNoSuchNames         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutBadValues           (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutGenErrs             (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutGetRequests         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutGetNexts            (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutSetRequests         (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutGetResponses        (snmp_object_t *, UINT16, VOID *);
UINT16 snmpOutTraps               (snmp_object_t *, UINT16, VOID *);
UINT16 snmpEnableAuthenTraps      (snmp_object_t *, UINT16, VOID *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
#endif

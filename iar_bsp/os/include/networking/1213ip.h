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
*       1213ip.h                                                 
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the function declarations for operations
*       on IP objects.
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

#if (RFC1213_IP_INCLUDE == NU_TRUE)

#ifndef _1213_IP_H_
#define _1213_IP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (RFC1213_IP_INCLUDE == NU_TRUE)

UINT16 ipForwarding       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipDefaultTTL       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipInReceives       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipInHdrErrors      (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipInAddrErrors     (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipForwDatagrams    (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipInUnknownProtos  (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipInDiscards       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipInDelivers       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipOutRequests      (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipOutDiscards      (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipOutNoRoutes      (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipReasmTimeout     (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipReasmReqds       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipReasmOKs         (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipReasmFails       (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipFragOKs          (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipFragFails        (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipFragCreates      (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipRoutingDiscards  (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipAddrEntry        (snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipRouteEntry       (snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif  /* RFC1213_IP_INCLUDE == NU_TRUE */

UINT16 ipNetToMediaEntry  (snmp_object_t *obj, UINT16 idlen, VOID *param);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
#endif

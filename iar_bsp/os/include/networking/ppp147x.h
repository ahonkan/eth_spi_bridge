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
*       ppp147x.h                                                
*
*   DESCRIPTION
*
*       This file contains object access definitions and function
*       declarations for the PPP MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       net/inc/os.h
*       networking/snmp_prt.h
*       networking/snmp.h
*
************************************************************************/
#ifndef SNMP_RFC147X_INC_PPP147X_H
#define SNMP_RFC147X_INC_PPP147X_H

#include "networking/snmp_prt.h"
#include "networking/snmp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++   */
#endif /* _cplusplus */

/* Object ID definitions. */
#define OID_PPP                 1,3,6,1,2,1,10,23
#define OID_PPP_LCP             OID_PPP,1,1
#define OID_PPP_SEC             OID_PPP,2
#define OID_PPP_NCP             OID_PPP,3

#define OID_LCP_STATUS          OID_PPP_LCP,1,1
#define OID_LCP_CONFIG          OID_PPP_LCP,2,1

#define OID_SEC_PROTOCOL        OID_PPP_SEC,1
#define OID_SEC_CONFIG          OID_PPP_SEC,2,1
#define OID_SEC_SECRETS         OID_PPP_SEC,3,1

#define OID_SEC_PAP             OID_SEC_PROTOCOL,1
#define OID_SEC_CHAP            OID_SEC_PROTOCOL,2

#define OID_NCP_STATUS          OID_PPP_NCP,1,1
#define OID_NCP_CONFIG          OID_PPP_NCP,2,1

#define PROTOCOL_OBJ_SIZE       11
#define PROTOCOL_OBJ_BYTES      (PROTOCOL_OBJ_SIZE * sizeof(INT32))

UINT16 pppLinkStatusEntry      (snmp_object_t *, UINT16, VOID *);
UINT16 pppLinkConfigEntry      (snmp_object_t *, UINT16, VOID *);
UINT16 pppSecurityConfigEntry  (snmp_object_t *, UINT16, VOID *);
UINT16 pppSecuritySecretsEntry (snmp_object_t *, UINT16, VOID *);
UINT16 pppIpEntry              (snmp_object_t *, UINT16, VOID *);
UINT16 pppIpConfigEntry        (snmp_object_t *, UINT16, VOID *);

extern INT32 ChapObjectId[PROTOCOL_OBJ_SIZE];
extern INT32 PapObjectId[PROTOCOL_OBJ_SIZE];

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* SNMP_RFC147X_INC_PPP147X_H */


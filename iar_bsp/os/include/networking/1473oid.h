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
*       1473oid.h                                                
*
*   DESCRIPTION
*
*       Object identifiers to be included in the SNMP mib table.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef _1473_OID_H
#define _1473_OID_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Status */
{ {OID_NCP_STATUS,1}, 12, pppIpEntry, SNMP_INTEGER, MIB_READ},
{ {OID_NCP_STATUS,2}, 12, pppIpEntry, SNMP_INTEGER, MIB_READ},
{ {OID_NCP_STATUS,3}, 12, pppIpEntry, SNMP_INTEGER, MIB_READ},
{ {OID_NCP_STATUS,4}, 12, pppIpEntry, SNMP_INTEGER, MIB_READ},
{ {OID_NCP_STATUS,5}, 12, pppIpEntry, SNMP_INTEGER, MIB_READ},

/* Config */
{ {OID_NCP_CONFIG,1}, 12, pppIpConfigEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},
{ {OID_NCP_CONFIG,2}, 12, pppIpConfigEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


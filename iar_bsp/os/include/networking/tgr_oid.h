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
*       tgr_oid.h                                                
*
*   DESCRIPTION
*
*       This file contains OID definitions for the Target Address Table
*       and Target params table.
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

#ifndef TGR_OID_H
#define TGR_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

{ {1,3,6,1,6,3,12,1,1},      9, snmpTargetSpinLock, SNMP_INTEGER , MIB_READ | MIB_WRITE },

/*target address table*/
{ {1,3,6,1,6,3,12,1,2,1,2}, 11, snmpTargetAddrEntry, SNMP_OBJECTID,   MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,3}, 11, snmpTargetAddrEntry, SNMP_DISPLAYSTR, MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,4}, 11, snmpTargetAddrEntry, SNMP_INTEGER,    MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,5}, 11, snmpTargetAddrEntry, SNMP_INTEGER,    MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,6}, 11, snmpTargetAddrEntry, SNMP_DISPLAYSTR, MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,7}, 11, snmpTargetAddrEntry, SNMP_DISPLAYSTR, MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,8}, 11, snmpTargetAddrEntry, SNMP_INTEGER,    MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,2,1,9}, 11, snmpTargetAddrEntry, SNMP_INTEGER,    MIB_READ |
MIB_CREATE},

/*params table*/

{ {1,3,6,1,6,3,12,1,3,1,2}, 11, snmpTargetParamsEntry, SNMP_INTEGER,    MIB_READ|
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,3,1,3}, 11, snmpTargetParamsEntry, SNMP_INTEGER,    MIB_READ|
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,3,1,4}, 11, snmpTargetParamsEntry, SNMP_DISPLAYSTR, MIB_READ|
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,3,1,5}, 11, snmpTargetParamsEntry, SNMP_INTEGER,    MIB_READ|
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,3,1,6}, 11, snmpTargetParamsEntry, SNMP_INTEGER,    MIB_READ|
MIB_CREATE},
{ {1,3,6,1,6,3,12,1,3,1,7}, 11, snmpTargetParamsEntry, SNMP_INTEGER,    MIB_READ|
MIB_CREATE},


{ {1,3,6,1,6,3,12,1,4},      9, snmpUnavailableContexts, SNMP_COUNTER, MIB_READ},
{ {1,3,6,1,6,3,12,1,5},      9, snmpUnknownContexts,     SNMP_COUNTER, MIB_READ},

#ifdef          __cplusplus
}
#endif /* __cplusplus */
#endif



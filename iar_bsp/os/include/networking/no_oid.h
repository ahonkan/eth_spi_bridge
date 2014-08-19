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
*       no_oid.h                                                 
*
*   DESCRIPTION
*
*       This file contains OID declarations for Notification MIBs.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef NO_OID_H
#define NO_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/*snmp notify table*/
{ {1,3,6,1,6,3,13,1,1,1,2}, 11, snmpNotifyEntry, SNMP_DISPLAYSTR, MIB_READ},
{ {1,3,6,1,6,3,13,1,1,1,3}, 11, snmpNotifyEntry, SNMP_INTEGER,    MIB_READ},
{ {1,3,6,1,6,3,13,1,1,1,4}, 11, snmpNotifyEntry, SNMP_INTEGER,    MIB_READ},
{ {1,3,6,1,6,3,13,1,1,1,5}, 11, snmpNotifyEntry, SNMP_INTEGER,    MIB_READ},

/*snmp notify filter profile table*/
{ {1,3,6,1,6,3,13,1,2,1,1}, 11, snmpNotifyFiltProfEntry, SNMP_DISPLAYSTR, MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,13,1,2,1,2}, 11, snmpNotifyFiltProfEntry, SNMP_INTEGER,    MIB_READ |
MIB_CREATE},
{ {1,3,6,1,6,3,13,1,2,1,3}, 11, snmpNotifyFiltProfEntry, SNMP_INTEGER,    MIB_READ |
MIB_CREATE},

/*snmp notify filter table*/
{ {1,3,6,1,6,3,13,1,3,1,2}, 11, snmpNotifyFilterEntry, SNMP_DISPLAYSTR, MIB_READ},
{ {1,3,6,1,6,3,13,1,3,1,3}, 11, snmpNotifyFilterEntry, SNMP_INTEGER,    MIB_READ},
{ {1,3,6,1,6,3,13,1,3,1,4}, 11, snmpNotifyFilterEntry, SNMP_INTEGER,    MIB_READ},
{ {1,3,6,1,6,3,13,1,3,1,5}, 11, snmpNotifyFilterEntry, SNMP_INTEGER,    MIB_READ},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


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
*       engine_oid.h                                             
*
*   DESCRIPTION
*
*       This file contains the OID declarations for the SNMP Engine MIB.
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

#ifndef ENGINE_OID_H
#define ENGINE_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

{ {1,3,6,1,6,3,10,2,1,1}, 10, snmpEngineID,             SNMP_DISPLAYSTR, MIB_READ},
{ {1,3,6,1,6,3,10,2,1,2}, 10, snmpEngineBoots,          SNMP_INTEGER,    MIB_READ},
{ {1,3,6,1,6,3,10,2,1,3}, 10, snmpEngineTime,           SNMP_INTEGER,    MIB_READ},
{ {1,3,6,1,6,3,10,2,1,4}, 10, snmpEngineMaxMessageSize, SNMP_INTEGER,    MIB_READ},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif



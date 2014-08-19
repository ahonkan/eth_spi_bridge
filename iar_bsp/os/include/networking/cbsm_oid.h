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
*       cbsm_oid.h                                               
*
*   DESCRIPTION
*
*       This file contains declarations for initialization of
*       MibRoot->Table.
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
#ifndef CBSM_OID_H
#define CBSM_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (INCLUDE_MIB_CBSM == NU_TRUE)

{ {1,3,6,1,6,3,18,1,1,1,2} , 11 , snmpCommunityEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE | MIB_WRITE },
{ {1,3,6,1,6,3,18,1,1,1,3} , 11 , snmpCommunityEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE | MIB_WRITE },
{ {1,3,6,1,6,3,18,1,1,1,4} , 11 , snmpCommunityEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE | MIB_WRITE },
{ {1,3,6,1,6,3,18,1,1,1,5} , 11 , snmpCommunityEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE | MIB_WRITE },
{ {1,3,6,1,6,3,18,1,1,1,6} , 11 , snmpCommunityEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE | MIB_WRITE },
{ {1,3,6,1,6,3,18,1,1,1,7} , 11 , snmpCommunityEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE | MIB_WRITE },
{ {1,3,6,1,6,3,18,1,1,1,8} , 11 , snmpCommunityEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE | MIB_WRITE },

#endif /* (INCLUDE_MIB_CBSM == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* CBSM_OID_H */

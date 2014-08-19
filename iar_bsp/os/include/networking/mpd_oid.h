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
*       mpd_oid.h                                                
*
*   DESCRIPTION
*
*       This file contains OID declarations for SNMP-MPD-MIB.
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

#ifndef MPD_OID_H
#define MPD_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

{ {1,3,6,1,6,3,11,2,1,1}, 10, snmpUnknownSecurityModels, SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,11,2,1,2}, 10, snmpInvalidMsgs,           SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,11,2,1,3}, 10, snmpUnknownPDUHandlers,    SNMP_COUNTER , MIB_READ},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif



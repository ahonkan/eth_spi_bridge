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
*       1472oid.h                                                
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
#ifndef _1472_OID_H
#define _1472_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Config */
{ {OID_SEC_CONFIG,1}, 12, pppSecurityConfigEntry, SNMP_INTEGER,  MIB_READ | MIB_WRITE},
{ {OID_SEC_CONFIG,2}, 12, pppSecurityConfigEntry, SNMP_INTEGER,  MIB_READ | MIB_WRITE},
{ {OID_SEC_CONFIG,3}, 12, pppSecurityConfigEntry, SNMP_OBJECTID, MIB_READ | MIB_WRITE},
{ {OID_SEC_CONFIG,4}, 12, pppSecurityConfigEntry, SNMP_INTEGER,  MIB_READ | MIB_WRITE},

/* Secret */
{ {OID_SEC_SECRETS,1}, 12, pppSecuritySecretsEntry, SNMP_INTEGER,  MIB_READ},
{ {OID_SEC_SECRETS,2}, 12, pppSecuritySecretsEntry, SNMP_INTEGER,  MIB_READ},
{ {OID_SEC_SECRETS,3}, 12, pppSecuritySecretsEntry, SNMP_INTEGER,  MIB_READ | MIB_WRITE},
{ {OID_SEC_SECRETS,4}, 12, pppSecuritySecretsEntry, SNMP_OBJECTID, MIB_READ | MIB_WRITE},
{ {OID_SEC_SECRETS,5}, 12, pppSecuritySecretsEntry, SNMP_OCTETSTR, MIB_READ | MIB_WRITE},
{ {OID_SEC_SECRETS,6}, 12, pppSecuritySecretsEntry, SNMP_OCTETSTR, MIB_READ | MIB_WRITE},
{ {OID_SEC_SECRETS,7}, 12, pppSecuritySecretsEntry, SNMP_INTEGER,  MIB_READ | MIB_WRITE},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


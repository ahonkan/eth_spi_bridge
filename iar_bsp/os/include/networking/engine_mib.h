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
*       engine_mib.h                                             
*
*   DESCRIPTION
*
*       This file contains declarations of functions for SNMP Engine MIB.
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


#ifndef ENGINE_MIB_H
#define ENGINE_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 snmpEngineID(snmp_object_t *Obj, UINT16 IdLen, VOID *param);
UINT16 snmpEngineBoots(snmp_object_t *Obj, UINT16 IdLen, VOID *param);
UINT16 snmpEngineTime(snmp_object_t *Obj, UINT16 IdLen, VOID *param);
UINT16 snmpEngineMaxMessageSize(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


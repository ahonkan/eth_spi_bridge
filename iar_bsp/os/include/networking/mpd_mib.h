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
*       mpd_mib.h                                                
*
*   DESCRIPTION
*
*       This file contains declarations of functions for Message
*       Processing and Dispatching(MPD) MIB.
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

#ifndef MPD_MIB_H
#define MPD_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 snmpUnknownSecurityModels(snmp_object_t *Obj, UINT16 IdLen,
                                 VOID *param);

UINT16 snmpInvalidMsgs(snmp_object_t *Obj, UINT16 IdLen, VOID *param);
UINT16 snmpUnknownPDUHandlers(snmp_object_t *Obj, UINT16 IdLen,
                              VOID *param);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


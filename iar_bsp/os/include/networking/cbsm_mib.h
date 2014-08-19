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
*       cbsm_mib.h                                               
*
*   DESCRIPTION
*
*       This component provides functionality for CBSM MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       cbsm.h
*
************************************************************************/
#ifndef CBSM_MIB_H
#define CBSM_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#include "networking/cbsm.h"

UINT16 snmpCommunityEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* CBSM_MIB_H*/

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
*       vacm_mib.h                                               
*
*   DESCRIPTION
*
*       This file contains declaration of the functions to honor the
*       different requests of view based access mode(VACM) [rfc2575].
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       vacm.h
*
************************************************************************/

#ifndef VACM_MIB_H
#define VACM_MIB_H

#include "networking/vacm.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 vacmContextEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

UINT16 vacmSec2GroupEntry(snmp_object_t *obj, UINT16 idlen,
                                VOID *param);

UINT16 vacmAccessEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

UINT16 vacmViewSpinLock(snmp_object_t *obj, UINT16 idlen, VOID *param);

UINT16 vacmViewTreeEntry(snmp_object_t *obj, UINT16 idlen,
                               VOID *param);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


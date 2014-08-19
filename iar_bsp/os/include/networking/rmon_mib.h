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
*       rmon_mib.h                                               
*
*   DESCRIPTION
*
*       This file contains function declarations used by the RMON
*
*   DATA STRUCTURES
*
*
*   DEPENDENCIES
*
*       target.h
*
*************************************************************************/

#ifndef RMON_MIB_H
#define RMON_MIB_H

#include "net/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

/* Following two functions will be used by RMON for initialization
 * purposes.
 */
UINT16       RMON_MibRequest(UINT32, snmp_object_t *, UINT16 *);
STATUS       RMON_Request   (snmp_object_t *, INT32 *);

/*This function checks the syntax of an RMON MIB*/
mib_local_t* RMON_MibRmon(snmp_object_t *, mib_local_t *, UINT16, UINT16);

mib_local_t* RMON_MibInsert(snmp_object_t *, mib_local_t **, UINT16,
                            UINT16);
BOOL         RMON_MibRemove(snmp_object_t *, mib_local_t **, UINT16,
                            UINT16);

#endif /* (INCLUDE_MIB_RMON1 == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


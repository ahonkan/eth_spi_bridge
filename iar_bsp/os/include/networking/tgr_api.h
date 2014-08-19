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
*       tgr_api.h                                                
*
*   DESCRIPTION
*
*       This file contains API declarations for the SNMP TARGET Table.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*       tgr_mib.h
*
************************************************************************/

#ifndef TGR_API_H
#define TGR_API_H

#include "networking/target.h"
#include "networking/tgr_mib.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */
SNMP_TARGET_ADDRESS_TABLE* SNMP_Find_Target(const UINT8 *target_name);
SNMP_TARGET_PARAMS_TABLE* SNMP_Find_Params(const UINT8 *params_name);
STATUS SNMP_Remove_From_Tgr_Table(SNMP_TARGET_ADDRESS_TABLE *node);
STATUS SNMP_Remove_From_Params_Table(SNMP_TARGET_PARAMS_TABLE *node);
#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

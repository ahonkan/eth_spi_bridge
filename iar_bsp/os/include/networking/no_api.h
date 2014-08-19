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
*       no_api.h                                                 
*
*   DESCRIPTION
*
*       Contains API for SNMP Notifications.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*       snmp_no.h
*
************************************************************************/
#ifndef NO_API_H
#define NO_API_H

#include "networking/target.h"
#include "networking/snmp_no.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

STATUS SNMP_Add_To_Notify_Tbl(SNMP_NOTIFY_TABLE *node);
STATUS SNMP_Add_To_Profile_Tbl(SNMP_NOTIFY_FILTER_PROFILE_TABLE *node);
STATUS SNMP_Add_To_Filter_Tbl(SNMP_NOTIFY_FILTER_TABLE *node);
SNMP_NOTIFY_TABLE* SNMP_Find_Notify_Entry(const UINT8 *notify_name);
SNMP_NOTIFY_FILTER_PROFILE_TABLE* SNMP_Find_Profile_Entry(
                                            const UINT8 *params_name);
SNMP_NOTIFY_FILTER_TABLE* SNMP_Find_Filter_Entry(const UINT8 *filter_name);
STATUS SNMP_Remove_From_Notify_Table(SNMP_NOTIFY_TABLE *node);
STATUS SNMP_Remove_From_Profile_Table(SNMP_NOTIFY_FILTER_PROFILE_TABLE *node);
STATUS SNMP_Remove_From_Filter_Table(SNMP_NOTIFY_FILTER_TABLE *node);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif



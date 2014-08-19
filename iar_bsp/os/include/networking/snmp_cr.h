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
*       snmp_cr.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used by the Command Responder Application.
*
*   DATA STRUCTURES
*
*       SNMP_QUEUE_STRUCT
*
*   DEPENDENCIES
*
*       snmp_cfg.h
*       snmp_dis.h
*
************************************************************************/
#ifndef SNMP_CR_H
#define SNMP_CR_H

#include "networking/snmp_cfg.h"
#include "networking/snmp_dis.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define SNMP_REQUEST_ARRIVED    0x00000001

/* Functions */
VOID SNMP_AppResponder_Task_Entry(UNSIGNED argc, VOID *argv);
STATUS SNMP_Get_Request_Ptr(SNMP_SESSION_STRUCT **request);
STATUS SNMP_Request_Ready(VOID);
STATUS SNMP_Retrieve_Request (SNMP_SESSION_STRUCT **snmp_session);
STATUS SNMP_Process_Request(SNMP_SESSION_STRUCT *snmp_session);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_CR_H */

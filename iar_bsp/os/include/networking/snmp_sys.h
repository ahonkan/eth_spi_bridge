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
*       snmp_sys.h                                               
*
*   DESCRIPTION
*
*       This file contains function declarations for the functions
*       defined in snmp_sys.c.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       1213xxxx.h
*
************************************************************************/

#ifndef SNMP_SYS_H
#define SNMP_SYS_H

#include "networking/1213xxxx.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define UNICAST             0
#define BCAST               1
#define MCAST               2

#define STATISTICS_ERROR    1000L
#define EVENT_ERROR         9000L
#define ALARM_ERROR         8000L
#define CHANNEL_ERROR       4000L
#define FILTER_ERROR        3000L
#define CAPTURE_ERROR       5000L
#define HOST_ERROR          11000L
#define MATRIX_ERROR        13000L
#define TOPN_ERROR          12000L
#define HISTORY_ERROR       14000L

STATUS    snmp_init(VOID);
STATUS  SNMP_Mib_Init(VOID);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

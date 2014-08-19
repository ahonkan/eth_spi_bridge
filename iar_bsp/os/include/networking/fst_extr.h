/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE                                             VERSION
*
*       fst_extr.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server Tasks
*
*   DESCRIPTION
*
*       This file contains the function prototypes for the server-level
*       tasks and general application initialization utilities.  All
*       correlating function definitions are located in fst.c.
*
*   DATA STRUCTURES
*
*       FST_ACTIVE_LIST     List of tasks and data sockets that have
*                           been created.
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       ftps_def.h
*
*************************************************************************/

#ifndef FST_EXTR
#define FST_EXTR
#include "networking/ftps_def.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT32  NU_FTP_Server_Init(VOID);
STATUS NU_FTP_Server_Uninit(UINT8 urgency);
VOID   Master_Task(UNSIGNED argc, VOID *argv);
VOID   Cleaner_Task(UNSIGNED argc, VOID *argv);
VOID   Control_Task(UNSIGNED argc, VOID *argv);
VOID   FST_Data_Task_Entry(UNSIGNED argc, VOID *svr);
INT    FST_Client_Connect(FTP_SERVER *server);
VOID   FTP_Printf(char *fmt);
VOID   FST_Cleanup(TQ_EVENT event, UNSIGNED dat0, UNSIGNED dat1);

typedef struct Active_Task_List {
    NU_TASK *active_task;
    INT     active_sckt;
} FST_ACTIVE_LIST;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

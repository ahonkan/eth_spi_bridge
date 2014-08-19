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
*       ftps_ext.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server API
*       Functions
*
*   DESCRIPTION
*
*       This file contains the function prototypes for the server-level
*       API and general utilities.  All correlating function definitions
*       are located in ftps.c.
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
*       pcdisk.h
*
*************************************************************************/

#ifndef FTPS_EXT
#define FTPS_EXT
#include "storage/pcdisk.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT     FTPS_Server_Session_Init(FTP_SERVER*, UNSIGNED, INT, INT16);
INT     FTPS_Server_Structure_Init(FTP_SERVER*, UNSIGNED, INT, INT16);
INT     FTPS_UserInit(FTP_SERVER *server);
INT     FTPS_Create_Data_Task(FTP_SERVER *server);
INT     FTPS_Server_Get(FTP_SERVER *server, UINT32 gtype);
INT     FTPS_Server_Put(FTP_SERVER *server);
INT     FTPS_Server_ChDir(FTP_SERVER *server);
INT     FTPS_Server_Dir(FTP_SERVER *server, UINT32 dirtype);
INT     FTPS_Server_MkDir(FTP_SERVER *server);
INT     FTPS_Server_RmDir(FTP_SERVER *server);
INT     FTPS_Server_PrintDir(FTP_SERVER *server);
INT     FTPS_Server_Rename_File(FTP_SERVER *server);
INT     FTPS_Server_Delete_File(FTP_SERVER *server);
INT     FTPS_Server_Find_File_Size(FTP_SERVER *server);
INT     Str_Equal(UINT8 *str1, CHAR *str2);
VOID    FTPS_Server_File_Error(FTP_SERVER *server);
STATUS  FTPS_Authenticate (CHAR *id, CHAR *pw);
CHAR   *FTPS_File_Time(DSTAT *file, CHAR *string, UINT16 size);
INT     FTPS_Move_File(CHAR *source, CHAR *destpath, CHAR *dest);
VOID    FTPS_Get_Rename(CHAR *string);
INT     FTP_Is_Dir(CHAR *path);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif


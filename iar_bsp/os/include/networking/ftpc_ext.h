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
*       ftpc_ext.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client API-Level
*       Function Definitions
*
*   DESCRIPTION
*
*       This file contains the client-level API function prototypes.  All
*       correlating functions are found in ftpc.c.
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
*       None
*
*************************************************************************/

#ifndef FTPC_EXT
#define FTPC_EXT

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT     FTPC_Client_Open(FTP_CLIENT *client, IP_ADDR *host_ip);
INT     FTPC_Client_Open2(FTP_CLIENT *client, IP_ADDR *host_ip);
INT     FTPC_Client_Close(FTP_CLIENT *client);
INT     FTPC_Client_Login(FTP_CLIENT *client, CHAR *username,
          CHAR *password);
INT     FTPC_Client_Get(FTP_CLIENT *client, CHAR *rpath, CHAR *lpath);
INT     FTPC_Client_Put(FTP_CLIENT *client, CHAR *rpath, CHAR *lpath);
INT     FTPC_Client_Append_To_File(FTP_CLIENT *client, CHAR *rpath, CHAR *lpath);
INT     FTPC_Client_ChDir(FTP_CLIENT *client, CHAR *path);
INT     FTPC_Client_MkDir(FTP_CLIENT *client, CHAR *path);
INT     FTPC_Client_RmDir(FTP_CLIENT *client, CHAR *path);
INT     FTPC_Client_Dir(FTP_CLIENT *client, CHAR *buffer, INT buff_size,
             CHAR *filespec);
INT     FTPC_Client_Nlist(FTP_CLIENT *client, CHAR *buffer, INT buff_size,
             CHAR *filespec);
INT     FTPC_Client_Status(FTP_CLIENT *client, CHAR *path, CHAR *buffer,
           INT buff_size);
INT     FTPC_Client_Rename_File(FTP_CLIENT *client, CHAR *old_file_name,
                CHAR *new_file_name);
INT     FTPC_Client_Tran_Type(FTP_CLIENT *client, INT type);
INT     FTPC_Client_Tran_Mode(FTP_CLIENT *client, INT mode);
INT     FTPC_Client_Restart(FTP_CLIENT *client, INT32 restartpt);
INT     FTPC_Client_Size(FTP_CLIENT *client, CHAR *path, INT32 *size);
INT     FTPC_Client_Set_PASV(FTP_CLIENT *client);
INT     FTPC_Client_Set_ACTV(FTP_CLIENT *client);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

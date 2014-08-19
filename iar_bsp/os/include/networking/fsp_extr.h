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
*       fsp_extr.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Server Primitives
*
*   DESCRIPTION
*
*       This file contains function prototypes for the Nucleus FTP server
*       command primitives. All correlating function definitions are
*       located in fsp.c.
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

#ifndef FSP_EXTR
#define FSP_EXTR

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT     FSP_Command_Read(FTP_SERVER*, CHAR*, INT, UINT32);
INT     FSP_Filename_Parser(CHAR*, CHAR*, CHAR **);
INT     FSP_Server_TYPE(FTP_SERVER *server);
INT     FSP_Server_MODE(FTP_SERVER *server);
INT     FSP_Server_STRU(FTP_SERVER *server);
INT     FSP_Server_PORT(FTP_SERVER *server);
INT     FSP_Server_EPRT(FTP_SERVER *server);
INT     FSP_Server_LIST(FTP_SERVER *server);
INT     FSP_Server_RETR(FTP_SERVER *server);
INT     FSP_Server_STOR(FTP_SERVER *server);
INT     FSP_Server_APPE(FTP_SERVER *server);
INT     FSP_Server_NLST(FTP_SERVER *server);
INT     FSP_Server_USER(FTP_SERVER *server);
INT     FSP_Server_PASS(FTP_SERVER *server);
INT     FSP_Server_ACCT(FTP_SERVER *server);
INT     FSP_Server_CWD(FTP_SERVER *server);
INT     FSP_Server_MKD(FTP_SERVER *server);
INT     FSP_Server_RMD(FTP_SERVER *server);
INT     FSP_Server_XCWD(FTP_SERVER *server);
INT     FSP_Server_XMKD(FTP_SERVER *server);
INT     FSP_Server_XRMD(FTP_SERVER *server);
INT     FSP_Server_PASV(FTP_SERVER *server);
INT     FSP_Server_EPSV(FTP_SERVER *server);
INT     FSP_Server_QUIT(FTP_SERVER *server);
INT     FSP_Server_SYST(FTP_SERVER *server);
INT     FSP_Server_STAT(FTP_SERVER *server);
INT     FSP_Server_HELP(FTP_SERVER *server);
INT     FSP_Server_PWD(FTP_SERVER *server);
INT     FSP_Server_XPWD(FTP_SERVER *server);
INT     FSP_Server_RNFR(FTP_SERVER *server);
INT     FSP_Server_RNTO(FTP_SERVER *server);
INT     FSP_Server_ABOR(FTP_SERVER *server);
INT     FSP_Server_DELE(FTP_SERVER *server);
INT     FSP_Server_SIZE(FTP_SERVER *server);
INT     FSP_Server_REST(FTP_SERVER *server);
INT     FSP_Server_FEAT(FTP_SERVER *server);
INT     FSP_Server_NOOP(FTP_SERVER *server);
INT     FSP_Server_UNKNOWN(FTP_SERVER *server);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif



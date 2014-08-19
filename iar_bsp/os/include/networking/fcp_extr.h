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
*       fcp_extr.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client Primitives
*
*   DESCRIPTION
*
*       This file contains function prototypes and defined constants for
*       the Nucleus FTP command primitives.  All correlating function
*       definitions are located in fcp.c.
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

#ifndef FCP_EXTR
#define FCP_EXTR

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT     FCP_Reply_Read(FTP_CLIENT *client, UINT8 *buffer,
             INT buffsize, UINT32 timeout);
INT     FCP_Client_Verify_Caller(FTP_CLIENT *client);
INT     FCP_Client_TYPE(FTP_CLIENT *client, INT type);
INT     FCP_Client_MODE(FTP_CLIENT *client, INT mode);
INT     FCP_Client_STRU(FTP_CLIENT *client, INT structure);
INT     FCP_Client_PORT(FTP_CLIENT *client, struct addr_struct *servaddr);
INT     FCP_Client_EPRT(FTP_CLIENT *client, struct addr_struct *servaddr);
INT     FCP_Client_LIST(FTP_CLIENT *client, CHAR *filespec);
INT     FCP_Client_RETR(FTP_CLIENT *client, CHAR *filespec);
INT     FCP_Client_STOR(FTP_CLIENT *client, CHAR *filespec);
INT     FCP_Client_APPE(FTP_CLIENT *client, CHAR *filespec);
INT     FCP_Client_NLST(FTP_CLIENT *client, CHAR *filespec);
INT     FCP_Client_Tran_Ack(FTP_CLIENT *client, INT ignore_flag);
INT     FCP_Client_USER(FTP_CLIENT *client, CHAR *username);
INT     FCP_Client_PASS(FTP_CLIENT *client, CHAR *password);
INT     FCP_Client_ACCT(FTP_CLIENT *client, CHAR *account);
INT     FCP_Client_CWD(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_XCWD(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_MKD(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_XMKD(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_RMD(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_XRMD(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_CDUP(FTP_CLIENT *client);
INT     FCP_Client_XCUP(FTP_CLIENT *client);
INT     FCP_Client_PASV(FTP_CLIENT *client, struct addr_struct *servaddr);
INT     FCP_Client_QUIT(FTP_CLIENT *client);
INT     FCP_Client_SYST(FTP_CLIENT *client, CHAR *buffer, INT buffsize);
INT     FCP_Client_STAT(FTP_CLIENT *client, CHAR *path, CHAR *buffer,
         INT buffsize);
INT     FCP_Client_HELP(FTP_CLIENT *client, CHAR *topic, CHAR *buffer,
         INT buffsize);
INT     FCP_Client_PWD(FTP_CLIENT *client, CHAR *buffer, INT buffsize);
INT     FCP_Client_XPWD(FTP_CLIENT *client, CHAR *buffer, INT buffsize);
INT     FCP_Client_NOOP(FTP_CLIENT *client);
INT     FCP_Client_RNFR(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_RNTO(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_DELE(FTP_CLIENT *client, CHAR *path);
INT     FCP_Client_FEAT(FTP_CLIENT *client, CHAR *buffer, INT buffsize);
INT     FCP_Client_REST(FTP_CLIENT *client, INT32 restartpt);
INT     FCP_Client_SIZE(FTP_CLIENT *client, CHAR *path, INT32 *size);
INT 	FCP_Client_EPSV(FTP_CLIENT *client, struct addr_struct *servaddr);

VOID    FCP_Client_LIST_Response(FTP_CLIENT *client, UINT8 *buffer,
         INT buffsize);
VOID    FCP_Client_MKD_Response(FTP_CLIENT *client, UINT8 *buffer,
         INT buffsize);
VOID    FCP_Client_TYPE_Response(FTP_CLIENT *client, UINT8 *buffer,
         INT buffsize);
VOID    FCP_Client_PWD_Response(FTP_CLIENT *client, CHAR *buffer,
         INT buffsize);

/* FCP_Reply_Read() state constants */
#define RR_START        0
#define RR_DIGIT2       1
#define RR_DIGIT3       2
#define RR_SPACE        3
#define RR_AFTER_SPACE  4
#define RR_FIND_LF      5
#define RR_HYPHEN_ALT   6
#define RR_HYPHEN2      7
#define RR_HYPHEN3      8
#define RR_BAD_REP      9
#define RR_BAD_FIND_LF  10

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

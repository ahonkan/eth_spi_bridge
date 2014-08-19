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
*       ftpc_def.h                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client Definitions
*
*   DESCRIPTION
*
*       This file contains the definitions for the Nucleus FTP Client API.
*       It includes the API constant defines as well as definitions of all
*       API specific types.
*
*   DATA STRUCTURES
*
*       FTP_CLIENT              Stores client parameters.
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       fc_defs.h
*       ftp_cfg.h
*
*************************************************************************/

#ifndef FTPC_DEF
#define FTPC_DEF

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/fc_defs.h"
#include "networking/ftp_cfg.h"

#define FTPC_MODE_STREAM                 0

#define FTPC_STRU_FILE                   0

#define NU_FCP_Reply_Read              FCP_Reply_Read
#define NU_FCP_Client_Verify_Caller    FCP_Client_Verify_Caller
#define NU_FCP_Client_TYPE             FCP_Client_TYPE
#define NU_FCP_Client_MODE             FCP_Client_MODE
#define NU_FCP_Client_STRU             FCP_Client_STRU
#define NU_FCP_Client_PORT             FCP_Client_PORT
#define NU_FCP_Client_EPRT             FCP_Client_EPRT
#define NU_FCP_Client_LIST             FCP_Client_LIST
#define NU_FCP_Client_RETR             FCP_Client_RETR
#define NU_FCP_Client_STOR             FCP_Client_STOR
#define NU_FCP_Client_APPE             FCP_Client_APPE
#define NU_FCP_Client_NLST             FCP_Client_NLST
#define NU_FCP_Client_Tran_Ack         FCP_Client_Tran_Ack
#define NU_FCP_Client_USER             FCP_Client_USER
#define NU_FCP_Client_PASS             FCP_Client_PASS
#define NU_FCP_Client_ACCT             FCP_Client_ACCT
#define NU_FCP_Client_CWD              FCP_Client_CWD
#define NU_FCP_Client_XCWD             FCP_Client_XCWD
#define NU_FCP_Client_MKD              FCP_Client_MKD
#define NU_FCP_Client_XMKD             FCP_Client_XMKD
#define NU_FCP_Client_RMD              FCP_Client_RMD
#define NU_FCP_Client_XRMD             FCP_Client_XRMD
#define NU_FCP_Client_CDUP             FCP_Client_CDUP
#define NU_FCP_Client_XCUP             FCP_Client_XCUP
#define NU_FCP_Client_PASV             FCP_Client_PASV
#define NU_FCP_Client_EPSV             FCP_Client_EPSV
#define NU_FCP_Client_QUIT             FCP_Client_QUIT
#define NU_FCP_Client_SYST             FCP_Client_SYST
#define NU_FCP_Client_STAT             FCP_Client_STAT
#define NU_FCP_Client_SIZE             FCP_Client_SIZE
#define NU_FCP_Client_HELP             FCP_Client_HELP
#define NU_FCP_Client_PWD              FCP_Client_PWD
#define NU_FCP_Client_XPWD             FCP_Client_XPWD
#define NU_FCP_Client_NOOP             FCP_Client_NOOP
#define NU_FCP_Client_RNFR             FCP_Client_RNFR
#define NU_FCP_Client_RNTO             FCP_Client_RNTO
#define NU_FCP_Client_DELE             FCP_Client_DELE
#define NU_FCP_Client_REST             FCP_Client_REST
#define NU_FCP_Client_FEAT             FCP_Client_FEAT
#define NU_FTPC_Client_Open            FTPC_Client_Open
#define NU_FTPC_Client_Open2           FTPC_Client_Open2
#define NU_FTPC_Client_Close           FTPC_Client_Close
#define NU_FTPC_Client_Login           FTPC_Client_Login
#define NU_FTPC_Client_Get             FTPC_Client_Get
#define NU_FTPC_Client_Put             FTPC_Client_Put
#define NU_FTPC_Client_Append_To_File  FTPC_Client_Append_To_File
#define NU_FTPC_Client_ChDir           FTPC_Client_ChDir
#define NU_FTPC_Client_MkDir           FTPC_Client_MkDir
#define NU_FTPC_Client_RmDir           FTPC_Client_RmDir
#define NU_FTPC_Client_Dir             FTPC_Client_Dir
#define NU_FTPC_Client_Nlist           FTPC_Client_Nlist
#define NU_FTPC_Client_Size            FTPC_Client_Size
#define NU_FTPC_Client_Status          FTPC_Client_Status
#define NU_FTPC_Client_Rename_File     FTPC_Client_Rename_File
#define NU_FTPC_Client_Tran_Type       FTPC_Client_Tran_Type
#define NU_FTPC_Client_Tran_Mode       FTPC_Client_Tran_Mode
#define NU_FTPC_Client_Restart         FTPC_Client_Restart
#define NU_FTPC_Client_Set_PASV        FTPC_Client_Set_PASV
#define NU_FTPC_Client_Set_ACTV        FTPC_Client_Set_ACTV

typedef struct FTP_CLIENT_STRUCT
{
     UNSIGNED   valid_pattern;
     UNSIGNED   task_id;
     IP_ADDR    *host_addr,
                *local_data_addr,
                *host_data_addr;
     INT32      restart;
     INT16      ftpc_family;
     UINT8      padN[2];
     BOOLEAN    mode;
     INT        socketd;
     INT        transfer_type;
     INT        last_error;
     INT        stack_error;
     INT        reply_idx, reply_tail;
     CHAR       reply_buff[FTPC_REPLY_BUFFER_SIZE];
} FTP_CLIENT;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

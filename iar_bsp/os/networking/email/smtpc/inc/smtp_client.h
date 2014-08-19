/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************/

/************************************************************************
*
* FILE NAME
*
*       smtp_client.h
*
* COMPONENT
*
*       SMTP Client.
*
* DESCRIPTION
*
*       This file contains internal function prototypes and macros used
*       in SMTP client API.
*
* DATA STRUCTURES
*
*       SMTP_Memory_Pool
*
* DEPENDENCIES
*
*       smtp_client_api.h
*       smtp_ssl.h
*       rfc_4954.h
*       rfc_2487.h
*       smtp_message.h
*       smtp_line.h
*
*************************************************************************/
#ifndef _SMTP_CLIENT_H
#define _SMTP_CLIENT_H

#include "networking/smtp_client_api.h"
#include "os/networking/email/smtpc/inc/smtp_ssl.h"
#include "os/networking/email/smtpc/inc/rfc_4954.h"
#include "os/networking/email/smtpc/inc/rfc_2487.h"
#include "os/networking/email/smtpc/inc/smtp_message.h"
#include "os/networking/email/smtpc/inc/smtp_line.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Memory pool used by SMTP API. */
extern NU_MEMORY_POOL  *SMTP_Memory_Pool;

/* SMTP COM state. */
#define SMTP_NO_CONNECTION      0
#define SMTP_OPEN_CONNECTED     1
#define SMTP_SSL_CONNECTED      2

/* Internal error definitions. */
#define SMTP_COMMNAD_FAILED     1
#define SMTP_NOT_CONNECTED      2
#define SMTP_CONNECT_FAILED     3
#define SMTP_SEND_FAILED        4
#define SMTP_RECEIVE_FAILED     5
#define SMTP_UNKNOWN_COMMAND    6
#define SMTP_ENCODE_ERROR       7
#define SMTP_SETTING_ERROR      8
#define SMTP_SSL_INIT_ERROR     9
#define SMTP_NO_MEMORY          10
#define SMTP_TLS_ERROR          11
#define SMTP_AUTH_ERROR         12

/* Extension settings. */
/* If extension is enabled on server corresponding bit will */
/* be set in SMTP_SESSION->smtp_extensions. */

/* BIT-0    RFC-4954 Authentication plain.  */
/* BIT-1    RFC-4954 Authentication login.  */
/* BIT-2    RFC-2487 Start-TLS.             */
/* BIT-3    RFC-2920 Pipelining.            */
/* BIT-4    RFC-3030 Chucking.              */
/* BIT-5    RFC-3030 Binary MIME.           */
/* BIT-6    RFC-6152 8-bit MIME.            */
/* BIT-7    RFC-1870 Size.                  */
/* BIT-8    RFC-3461 DSN.                   */
/* BIT-9    RFC-2852 Deliver by.            */
/* BIT-10   RFC-3885 MTRK.                  */
/* BIT-11   RFC-4141 CONPERM.               */
/* BIT-12   RFC-4141 CONNEG.                */
/* BIT-13   RFC-2034 Enhanced status codes. */
/* BIT-14   RFC-3865 N0 soliciting.         */
/* BIT-15   RFC-1845 Checkpoint.            */

#define SMTP_EXT_AUTH_PLAIN 0
#define SMTP_EXT_AUTH_LOGIN 1
#define SMTP_EXT_TLS        2
#define SMTP_EXT_PIPELINE   3
#define SMTP_EXT_CHUNKING   4
#define SMTP_EXT_BIN_MIME   5
#define SMTP_EXT_8_BIT_MIME 6
#define SMTP_EXT_SIZE       7
#define SMTP_EXT_DSN        8
#define SMTP_EXT_DELIVERBY  9
#define SMTP_EXT_MTRK       10
#define SMTP_EXT_CONPERM    11
#define SMTP_EXT_CONNEG     12
#define SMTP_EXT_ENH_STATUS 13
#define SMTP_EXT_NO_SOLIC   14
#define SMTP_EXT_CHECKPOINT 15

/* Internal API. */
STATUS SMTP_Get_Settings(SMTP_SESSION *session, CHAR *response);

/* Helper functions. */
STATUS SMTP_Send_Bytes(SMTP_SESSION *session, CHAR *str, INT32 len);
STATUS SMTP_Data_Available(SMTP_SESSION *session);
INT32 SMTP_Recv_Bytes(SMTP_SESSION *session, CHAR *dest, INT32 len);
STATUS SMTP_Recv(SMTP_SESSION *session);
VOID SMTP_Error_Resp(SMTP_SESSION *session, UINT16 err);
VOID SMTP_Check_Errors(SMTP_SESSION *session, STATUS *status, INT16 *ok_rep);
CHAR *SMTP_Ultoa_Zeropad(CHAR num, CHAR *buf, INT16 width);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_CLIENT_H */

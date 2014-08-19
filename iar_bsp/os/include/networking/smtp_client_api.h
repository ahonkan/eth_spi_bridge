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
*       smtp_client_api.h
*
* COMPONENT
*
*       SMTP Client.
*
* DESCRIPTION
*
*       This file contains SMTP Client API function prototypes.
*
* DATA STRUCTURES
*
*       NONE
*
* DEPENDENCIES
*
*       NONE
*
*************************************************************************/
#ifndef _SMTP_CLIENT_API_H
#define _SMTP_CLIENT_API_H

#include "networking/smtp_client_defs.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* High level API. */

/* Session functions. */
SMTP_SESSION *NU_SMTP_Session_Init( CHAR *address, UINT16 port,
        INT16 connection_mode);
STATUS NU_SMTP_Session_Set_Login(SMTP_SESSION *session,
        CHAR *username, CHAR *password);
STATUS NU_SMTP_Session_Connect(SMTP_SESSION *session);
STATUS NU_SMTP_Session_Disconnect(SMTP_SESSION *session);

/* Message functions. */
SMTP_MSG *NU_SMTP_Msg_Init(VOID);
STATUS NU_SMTP_Msg_Set_Subject(SMTP_MSG *message, CHAR *subject);
STATUS NU_SMTP_Msg_Set_Sender(SMTP_MSG *message, CHAR *sender);
STATUS NU_SMTP_Msg_Add_Recipient(SMTP_MSG *message, CHAR *recpt,
        INT16 recpt_type);
STATUS NU_SMTP_Msg_Add_Body(SMTP_MSG *message, CHAR *body_txt);
STATUS NU_SMTP_Msg_Add_Attachment(SMTP_MSG *message,
        CHAR *attachment, CHAR *name, INT32 len, INT16 type);

STATUS NU_SMTP_Send_Message(SMTP_SESSION *session, SMTP_MSG * message);

/* Delete functions. */
STATUS NU_SMTP_Session_Delete(SMTP_SESSION *session);
STATUS NU_SMTP_Msg_Delete(SMTP_MSG *message);

/* Low Level API */
STATUS SMTP_Client_Init(SMTP_SESSION *session);
STATUS NU_SMTP_Data(SMTP_SESSION *session);
STATUS NU_SMTP_Ehlo(SMTP_SESSION *session);
STATUS NU_SMTP_Helo(SMTP_SESSION *session);
STATUS NU_SMTP_Mail(SMTP_SESSION *session, CHAR *from);
STATUS NU_SMTP_Rcpt(SMTP_SESSION *session, CHAR *to);
STATUS NU_SMTP_Msg(SMTP_SESSION *session, CHAR *data, INT32 length);
STATUS NU_SMTP_Rset(SMTP_SESSION *session);
STATUS NU_SMTP_Close(SMTP_SESSION *session);
STATUS NU_SMTP_Quit(SMTP_SESSION *session);
STATUS NU_SMTP_Base_Cmd(SMTP_SESSION *session, INT16 command, CHAR *argument,
        INT16 *ok_rep, CHAR *reply);

STATUS NU_SMTP_StartTLS(SMTP_SESSION *session);
STATUS NU_SMTP_Auth(SMTP_SESSION *session, CHAR *username, CHAR *password);

STATUS SMTP_Set_Extension_Settings(SMTP_SESSION *session,
        INT16 extension, INT16 setting);
STATUS SMTP_Base64_Encode(CHAR *string, CHAR **result, UINT32 length);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_CLIENT_API_H */

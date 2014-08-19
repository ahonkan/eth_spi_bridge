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
*       smtp_message.h
*
* COMPONENT
*
*       SMTP Client - Message MIME Support.
*
* DESCRIPTION
*
*       This file contains function prototypes and macros used in message
*       MIME support for SMTP client API.
*
* DATA STRUCTURES
*
*       NONE
*
* DEPENDENCIES
*
*       smtp_client_api.h
*
*************************************************************************/
#ifndef _SMTP_MESSAGE_H
#define _SMTP_MESSAGE_H

#include "networking/smtp_client_api.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Header definitions. */
#define SMTP_MIME_HEADER    "MIME-Version: 1.0"

#define SMTP_MIME_CNT_TYPE  "Content-Type: "
#define SMTP_MIME_CNT_TXT   "text/plain"
#define SMTP_MIME_CNT_BIN   "binary"
#define SMTP_MIME_CNT_MULTI "multipart/mixed"

#define SMTP_MIME_CNT_TRANS "Content-Transfer-Encoding: "
#define SMTP_MIME_CNT_B64   "base64"

#define SMTP_MIME_CNT_DIS   "Content-Disposition: "
#define SMTP_MIME_CNT_ATT   "attachment"
#define SMTP_MIME_CNT_FL_NM "filename="
#define SMTP_MIME_BOUND     "boundary="
#define SMTP_MIME_BOUNDRY   "__NU_BOUNDRY__"

#define SMTP_MIME_TAB       "    "
#define SMTP_MSG_END        "\r\n.\r\n"

/* RFC-5321 4.5.3.1.3. */
#define SMTP_MAX_ADDR   254                     /* Maximum address size. */
#define SMTP_MAX_SUB    SMTP_MAX_CMD_LINE - 15  /* Maximum subject size. */

/* Function prototypes. */
STATUS SMTP_Send_Txt(SMTP_SESSION *session, SMTP_PART * msg_part);
STATUS SMTP_Send_Bin(SMTP_SESSION *session, SMTP_PART * msg_part);
STATUS SMTP_Send_Message_Data(SMTP_SESSION *session, SMTP_MSG *message);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_MESSAGE_H */

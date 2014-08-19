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
*       smtp_client_defs.h
*
* COMPONENT
*
*       SMTP Client.
*
* DESCRIPTION
*
*       This file contains data structures used in SMTP Client API.
*
* DATA STRUCTURES
*
*       SMTP_LINE
*       SMTP_SESSION
*       SMTP_PART
*       SMTP_PART_DB
*       SMTP_RECPT
*       SMTP_RECPT_DB
*       SMTP_MSG
*
* DEPENDENCIES
*
*       smtp_client_cfg.h
*       ssl.h
*
*************************************************************************/
#ifndef _SMTP_CLIENT_DEFS_H
#define _SMTP_CLIENT_DEFS_H

#include "networking/smtp_client_cfg.h"
#include "openssl/ssl.h"

#ifdef     __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Message part types. */
#define SMTP_MSG_TXT    0
#define SMTP_ATT_TXT    1
#define SMTP_ATT_BIN    2

/* Forward address types. */
#define SMTP_MSG_TO     0
#define SMTP_MSG_CC     1
#define SMTP_MSG_BCC    2

/* Identifier for RFC-4954. */
#define SMTP_AUTH_EXTEN     4954

/* RFC-5954 status. */
#define SMTP_AUTH_DISABLED  0
#define SMTP_AUTH_ENABLED   1

/* RFC-5954 settings. */
#define SMTP_AUTH_PLAIN     2
#define SMTP_AUTH_LOGIN     3

/* SMTP connection modes. */
#define SMTP_OPEN           0
#define SMTP_SSL            1
#define SMTP_TLS            2

/* Identifier for RFC-2487. */
#define SMTP_TLS_EXTEN      2487

/* RFC-2487 status and settings. */
#define SMTP_TLS_DISABLED   4
#define SMTP_TLS_ENABLED    5

/* SMTP commands. */
#define SMTP_CONNECTING     0
#define SMTP_HELO_CMD       1
#define SMTP_EHLO_CMD       2
#define SMTP_DATA_CMD       3
#define SMTP_RSET_CMD       4
#define SMTP_VRFY_CMD       5
#define SMTP_EXPN_CMD       6
#define SMTP_HELP_CMD       7
#define SMTP_NOOP_CMD       8
#define SMTP_QUIT_CMD       9

/* Commands affected by Extensions. */
#define SMTP_MAIL_CMD       10
#define SMTP_RCPT_CMD       11
#define SMTP_STARTTLS_CMD   12

/* Must be at last. */
#define SMTP_DISCONNECTING  13

/* Line object structure. */
typedef struct smtp_line_struct
{
    CHAR    *smtp_line;
    CHAR    *smtp_data;
    CHAR    *smtp_last_line;
    CHAR    *smtp_delim;
    UINT32  smtp_maxline;
    UINT32  smtp_data_len;
} SMTP_LINE;

/* Data structure used for SMTP session. */
typedef struct smtp_session_struct
{
    struct  addr_struct smtp_serv;              /* Address of server. */

    SSL     *smtp_ssl_struct;                   /* SSL socket. */
    SSL_CTX *smtp_ssl_ctx;                      /* SMTP SSL context. */
    CHAR    *smtp_username;                     /* Username for login. */
    CHAR    *smtp_password;                     /* Password for login. */

    UINT32  smtp_extensions;                    /* Extension info. */

    INT     smtp_sock;                          /* Listen socket. */
    INT16   smtp_cmd;                           /* Current command. */
    INT16   smtp_com_mode;                      /* Communication method. */
    INT16   smtp_com_state;                     /* Communication state. */
    INT16   smtp_error;
    UINT16  smtp_rfc_4954_setting;              /* Authentication setting. */
    UINT16  smtp_rfc_4954_state;                /* Authentication state. */
    UINT16  smtp_rfc_2487_setting;              /* Start-TLS setting. */
    UINT16  smtp_rfc_2487_state;                /* Start-TLS state. */

    CHAR    smtp_resp[SMTP_MAX_CMD_LINE];       /* Response of server. */
    CHAR    smtp_cmd_sent[SMTP_MAX_CMD_LINE];   /* Last command sent. */

    UINT8   smtp_pad[2];                        /* Padding variable. */
} SMTP_SESSION;


/* Message part object structure. */
typedef struct smtp_part_struct
{
    struct smtp_part_struct *smtp_next;

    CHAR                    *smtp_data;     /* Data for that part. */
    CHAR                    *smtp_att_name; /* Attachment name. */
    INT32                   smtp_data_len;  /* Data length for that part. */
    INT16                   smtp_type;      /* Type of message portion. */

    UINT8                   smtp_pad[2];
} SMTP_PART;

/* Part list's root node. */
typedef struct smtp_part_db_struct
{
    SMTP_PART *smtp_front;
    SMTP_PART *smtp_last;

} SMTP_PART_DB;

/* Message recipient address object structure. */
typedef struct smtp_recpt_struct
{
    struct smtp_recpt_struct    *smtp_next;

    CHAR                        *smtp_addr;         /* Recipient address. */
    INT16                       smtp_recpt_type;    /* Address type. */

    UINT8                       smtp_pad[2];
} SMTP_RECPT;

/* Recipient list's root node. */
typedef struct smtp_rcpt_db_struct
{
    SMTP_RECPT *smtp_front;
    SMTP_RECPT *smtp_last;

} SMTP_RECPT_DB;

/* Message object structure. */
typedef struct smtp_msg_struct
{
    SMTP_PART_DB    smtp_part_list;     /* Message part list. */
    SMTP_RECPT_DB   smtp_recpt_list;    /* Recipient address list. */
    CHAR            *smtp_from_addr;    /* Sender's address. */
    CHAR            *smtp_subject;      /* Subject of message. */
} SMTP_MSG;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _SMTP_CLIENT_DEFS_H */

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

/*************************************************************************
*
*   FILE
*
*       smtp_client.c
*
*   COMPONENT
*
*       SMTP Client.
*
*   DESCRIPTION
*
*       This file contains SMTP client API functions.
*
*   DATA STRUCTURES
*
*       SMTP_Memory_Pool
*       SMTP_Error_Messages
*       SMTP_Commands
*       SMTP_EHLO_Keywords
*
*   FUNCTIONS
*
*       SMTP_Client_Init
*       SMTP_Get_Settings
*       NU_SMTP_Ehlo
*       NU_SMTP_Helo
*       NU_SMTP_Mail
*       NU_SMTP_Rcpt
*       NU_SMTP_Data
*       NU_SMTP_Msg
*       NU_SMTP_Rset
*       NU_SMTP_Close
*       NU_SMTP_Quit
*       SMTP_Set_Extension_Settings
*       SMTP_Base_Cmd
*       SMTP_Send_Bytes
*       SMTP_Data_Available
*       SMTP_Is_End_Response
*       SMTP_Recv_Bytes
*       SMTP_Recv
*       SMTP_Base64_Encode
*       SMTP_Error_Resp
*       SMTP_Check_Errors
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       smtp_client_api.h
*       smtp_client.h
*       smtp_ssl.h
*
*************************************************************************/
#include "nucleus.h"
#include "networking/nu_net.h"
#include "networking/smtp_client_api.h"
#include "os/networking/email/smtpc/inc/smtp_client.h"
#include "os/networking/email/smtpc/inc/smtp_ssl.h"

/* SMTP Client API memory pool. */
NU_MEMORY_POOL  *SMTP_Memory_Pool = &System_Memory;

/* Error messages returned in the response field. */
CHAR *SMTP_Error_Messages[] = {
    "SMTP: No error.",                              /* 0 */
    "SMTP: Command failed.",                        /* 1 */
    "SMTP: Not connected to server.",               /* 2 */
    "SMTP: Connect to remote SMTP server failed.",  /* 3 */
    "SMTP: Unable to send data.",                   /* 4 */
    "SMTP: Unable to receive data.",                /* 5 */
    "SMTP: Command not listed.",                    /* 6 */
    "SMTP: Base64 encoder failed.",                 /* 7 */
    "SMTP: Setting was not successful.",            /* 8 */
    "SMTP: SSL Initialization failed.",             /* 9 */
    "SMTP: Unable to allocate memory.",             /* 10 */
    "SMTP: Start-TLS not supported by server.",     /* 11 */
    "SMTP: User must specify login information."    /* 12 */
};

/* SMTP Commands. */
CHAR *SMTP_Commands[] = {
    "CONNECTING",
    "HELO",
    "EHLO",
    "DATA",
    "RSET",
    "VRFY",
    "EXPN",
    "HELP",
    "NOOP",
    "QUIT",
    "MAIL",
    "RCPT",
    "STARTTLS",
    "DISCONNECTING"
};

/* EHLO keywords. */
CHAR *SMTP_EHLO_Keywords[] = {
    "AUTH",
    "AUTH",
    "STARTTLS",
    "PIPELINING",
    "CHUNKING",
    "BINARYMIME",
    "8BITMIME",
    "SIZE",
    "DSN",
    "DELIVERBY",
    "MTRK",
    "CONPERM",
    "CONNEG",
    "ENHANCEDSTATUSCODES",
    "NO-SOLICITING",
    "CHECKPOINT",
    "\0"
};

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Client
*
* DESCRIPTION
*
*       Startup an SMTP session. Must be called before client can start
*       transferring mail. This is for the client side only.
*
* INPUTS
*
*       *session                Pointer to the SMTP structure that
*                               contains information about the server that
*                               we will attempt to connect.
*
* OUTPUTS
*
*       NU_SUCCESS              If client was successfully connected.
*       NU_INVALID_PARM         If session pointer is NU_NULL.
*       SMTP_CONNECT_FAILED     If we were unable to connect to server.
*       SMTP_SSL_INIT_ERROR     SSL was unable to initialized.
*       SMTP_RECEIVE_FAILED     if we were unable to receive banner from
*                               server.
*
*************************************************************************/
STATUS SMTP_Client_Init(SMTP_SESSION *session)
{
    STATUS status = NU_SUCCESS;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Initialize structure variables. */
    session->smtp_error             = 0;
    session->smtp_cmd               = SMTP_CONNECTING;
    session->smtp_com_state         = SMTP_NO_CONNECTION;
    session->smtp_rfc_4954_state    = SMTP_AUTH_NOT_DONE;
    session->smtp_rfc_2487_state    = SMTP_TLS_NOT_DONE;

    /* Create a socket. */
    session->smtp_sock = NU_Socket(session->smtp_serv.family,
            NU_TYPE_STREAM, NU_NONE);

    if (session->smtp_sock > 0)
    {
        /* Set it as blocking socket. */
        NU_Fcntl(session->smtp_sock, NU_SETFLAG, NU_BLOCK);

        /* Connect to the SMTP server. */
        if (NU_Connect(session->smtp_sock,
                &(session->smtp_serv), 0) >= 0)
        {
            /* Socket opened. */
            session->smtp_com_state = SMTP_OPEN_CONNECTED;

            if (status == NU_SUCCESS)
            {
                /* If we will use SSL to connect. */
                if ((session->smtp_com_mode == SMTP_SSL) ||
                        (session->smtp_com_mode == SMTP_TLS))
                {
                    /* Call SSL initialization routine. */
                    status = SMTP_SSL_Init(&session->smtp_ssl_ctx);
                }
            }

            if (status == NU_SUCCESS)
            {
                if (session->smtp_com_mode == SMTP_SSL)
                {
                    /* Open SSL connection. */
                    if (SMTP_SSLD_Connect(&(session->smtp_ssl_struct),
                            session->smtp_sock,
                            &session->smtp_ssl_ctx) == NU_SUCCESS)
                    {
                        session->smtp_com_state = SMTP_SSL_CONNECTED;
                        status = NU_SUCCESS;
                    }
                    else
                    {
                        /* Close SMTP socket. */
                        NU_SMTP_Close(session);

                        status = SMTP_CONNECT_FAILED;
                        SMTP_Error_Resp(session, status);
                    }
                }
            }
            else
            {
                status = SMTP_SSL_INIT_ERROR;
                SMTP_Error_Resp(session, status);

                /* Close SMTP socket. */
                NU_SMTP_Close(session);
            }

            if (status == NU_SUCCESS)
            {
                /* Receive welcome message from server. */
                if (SMTP_Recv_Bytes(session,
                        session->smtp_resp,
                        SMTP_MAX_CMD_LINE) > 0)
                {
                    if (strncmp(session->smtp_resp, "220", 3) != 0)
                    {
                        /* Send QUIT and close socket. */
                        NU_SMTP_Quit(session);

                        status = SMTP_CONNECT_FAILED;
                        SMTP_Error_Resp(session, status);
                    }
                    else
                    {
                        /* We are successfully connected. */
                        status = NU_SUCCESS;
                    }
                }
                else
                {
                    /* Close SMTP socket. */
                    NU_SMTP_Close(session);

                    status = SMTP_RECEIVE_FAILED;
                    SMTP_Error_Resp(session, status);
                }
            }

        } /* NU_Connect. */
        else
        {
            /* Close socket. */
            NU_Close_Socket(session->smtp_sock);

            status = SMTP_CONNECT_FAILED;
            SMTP_Error_Resp(session, status);
        }
    } /* NU_Socket. */
    else
    {
        status = SMTP_CONNECT_FAILED;
        SMTP_Error_Resp(session, status);
        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Client */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Get_Settings
*
* DESCRIPTION
*
*       This function will parse EHLO response and set the corresponding
*       bits in SMTP_SESSION->smtp_extensions.
*
* INPUTS
*
*       *session                Pointer to session object.
*       *response               EHLO response from server.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*
*************************************************************************/
STATUS SMTP_Get_Settings(SMTP_SESSION *session, CHAR *response)
{
    STATUS      status;
    SMTP_LINE   lineobj;
    CHAR        *line;
    CHAR        *keyword;
    INT         n = 0;

    /* Server's EHLO reply will contain STARTTLS if it supports RFC-2487. */
    status = SMTP_Init_Line_Obj(&lineobj, response, "\r\n",
            strlen(response), SMTP_MAX_CMD_LINE);

    if (status == NU_SUCCESS)
    {
        session->smtp_extensions = 0x0;

        /* Get a line from reply. */
        line = SMTP_Get_Line(&lineobj);

        while(line != NU_NULL)
        {
            n = 0;
            keyword = SMTP_EHLO_Keywords[n];

            while (keyword[0] != '\0')
            {
                /* Check if we have this keyword. */
                if (strstr(line, keyword) != NU_NULL)
                {
                    if ((n != SMTP_EXT_AUTH_PLAIN) &&
                            (n != SMTP_EXT_AUTH_LOGIN))
                    {
                        session->smtp_extensions |= (0x01 << n);
                    }
                    else
                    if ((n == SMTP_EXT_AUTH_PLAIN) ||
                            (n == SMTP_EXT_AUTH_LOGIN))
                    {
                        if (strstr(line, "PLAIN") != NU_NULL)
                        {
                            session->smtp_extensions |=
                                    (0x01 << SMTP_EXT_AUTH_PLAIN);
                        }

                        if (strstr(line, "LOGIN") != NU_NULL)
                        {
                            session->smtp_extensions |=
                                    (0x01 << SMTP_EXT_AUTH_LOGIN);
                        }
                    }

                    break;
                }

                /* Try next keyword */
                n++;
                keyword = SMTP_EHLO_Keywords[n];
            }

            /* Get next line. */
            line = SMTP_Get_Line(&lineobj);
        }

        /* Delete line object. */
        SMTP_Delete_Line_Obj(&lineobj);
    }

    return(status);
} /* SMTP_Get_Settings */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Ehlo
*
* DESCRIPTION
*
*       Must be the first command sent by client before any transfers
*       can be done.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Ehlo(SMTP_SESSION *session)
{
    STATUS  status;
    INT16   ok_rep[2] = {250, 0};
    struct  sockaddr_struct local_address;
    INT16   addr_len;
#if (INCLUDE_IPV6 == NU_TRUE)||(INCLUDE_IPV4 == NU_TRUE)
    CHAR    addr_str[50];
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* We will send our ip-address as domain name. */
    NU_Get_Sock_Name(session->smtp_sock, &local_address, &addr_len);
    NU_Inet_NTOP(session->smtp_serv.family,
            local_address.ip_num.is_ip_addrs,
            addr_str,
            sizeof(addr_str));

    /* Send command to server. */
    status = NU_SMTP_Base_Cmd(session, SMTP_EHLO_CMD, addr_str,
            ok_rep, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* After EHLO we lose login state. */
        session->smtp_rfc_4954_state = SMTP_AUTH_NOT_DONE;

        SMTP_Get_Settings(session, session->smtp_resp);
    }
    /* Check if server don't supports EHLO we should send HELO. */
    else if (status == SMTP_COMMNAD_FAILED)
    {
        /* Send HELO. */
        status = NU_SMTP_Helo(session);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Ehlo */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Helo
*
* DESCRIPTION
*
*       Must be the first command sent by client before any transfers
*       can be done.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Helo(SMTP_SESSION *session)
{
    STATUS  status;
    INT16   ok_rep[2] = {250, 0};
    struct  sockaddr_struct local_address;
    INT16   addr_len;
#if (INCLUDE_IPV6 == NU_TRUE)||(INCLUDE_IPV4 == NU_TRUE)
    CHAR    addr_str[50];
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* We will send our ip-address as domain name. */
    NU_Get_Sock_Name(session->smtp_sock, &local_address, &addr_len);
    NU_Inet_NTOP(session->smtp_serv.family,
            local_address.ip_num.is_ip_addrs,
            addr_str,
            sizeof(addr_str));

    /* Send command to server. */
    status = NU_SMTP_Base_Cmd(session, SMTP_HELO_CMD, addr_str,
            ok_rep, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* After HELO we lose login state. */
        session->smtp_rfc_4954_state = SMTP_AUTH_NOT_DONE;
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Hello */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Mail
*
* DESCRIPTION
*
*       This will send the MAIL command with extra arguments if enabled
*       by extensions.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       *from                   Sender's email address.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Mail(SMTP_SESSION *session, CHAR *from)
{
    STATUS  status = -1;
    INT16   ok_rep[2] = {250, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((session == NU_NULL) || (from == NU_NULL)
            || (strlen(from) == 0))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Make command that we will be sending. */
    sprintf(session->smtp_cmd_sent, "%s FROM:<%s>\r\n",
            SMTP_Commands[SMTP_MAIL_CMD], from);

    /* Send command to server. */
    status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
            strlen(session->smtp_cmd_sent));

    /* Receive reply from server. */
    if (status == NU_SUCCESS)
    {
        status = SMTP_Recv(session);
    }

    /* Check for errors. */
    SMTP_Check_Errors(session, &status, ok_rep);

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Mail */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Rcpt
*
* DESCRIPTION
*
*       This will send the RCPT command with extra arguments if enabled
*       by extensions.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       *to                     Recepiant's email address.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Rcpt(SMTP_SESSION *session, CHAR *to)
{
    STATUS  status = -1;
    INT16   ok_rep[3] = {250, 251, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((session == NU_NULL) || (to == NU_NULL)
            || (strlen(to) == 0))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Make command that we will be sending. */
    sprintf(session->smtp_cmd_sent, "%s TO:<%s>\r\n",
            SMTP_Commands[SMTP_RCPT_CMD], to);

    /* Send command to server. */
    status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
            strlen(session->smtp_cmd_sent));

    /* Receive reply from server. */
    if (status == NU_SUCCESS)
    {
        status = SMTP_Recv(session);
    }

    /* Check for errors. */
    SMTP_Check_Errors(session, &status, ok_rep);

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Rcpt */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Data
*
* DESCRIPTION
*
*       Tells SMTP server that client is about to send a mail message.
*       All mail messages must be terminated with the string '.\r\n'
*       in the first column.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
************************************************************************/
STATUS NU_SMTP_Data(SMTP_SESSION *session)
{
    STATUS  status;
    INT16   ok_rep[2] = {354, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Send command to server. */
    status = NU_SMTP_Base_Cmd(session, SMTP_DATA_CMD, NU_NULL,
            ok_rep, NU_NULL);

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Data */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg
*
* DESCRIPTION
*
*       This function will send message data to server with addition of
*       <CRLF>.<CRLF> at the end.
*
* INPUTS
*
*       *session                Pointer to session object.
*       *data                   Pointer to message data.
*       length                  Size of data to send.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
************************************************************************/
STATUS NU_SMTP_Msg(SMTP_SESSION *session, CHAR *data, INT32 length)
{
    STATUS  status;
    INT16   ok_rep[2] = {250, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((session == NU_NULL) || (data == NU_NULL)
            || (length <= 0) || (strlen(data) == 0))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Send data to server. */
    status = SMTP_Send_Bytes(session, data, length);

    /* Send message terminator to server. */
    if (status == NU_SUCCESS)
    {
        status = SMTP_Send_Bytes(session, "\r\n.\r\n", 5);
    }

    /* Receive reply from server. */
    if (status == NU_SUCCESS)
    {
        status = SMTP_Recv(session);
    }

    /* Check for errors. */
    SMTP_Check_Errors(session, &status, ok_rep);

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Msg */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Rset
*
* DESCRIPTION
*
*       This will reset SMTP server.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Rset(SMTP_SESSION *session)
{
    STATUS status;
    INT16  ok_rep[2] = {250, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Send command to server. */
    status = NU_SMTP_Base_Cmd(session, SMTP_RSET_CMD, NU_NULL,
            ok_rep, NU_NULL);

    if (status == NU_SUCCESS)
    {
        /* After RSET we lose login state. */
        session->smtp_rfc_4954_state = SMTP_AUTH_NOT_DONE;
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Rset */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Close
*
* DESCRIPTION
*
*       Close the socket connected to the SMTP server.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If the connection was successfully
*                               closed.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Close(SMTP_SESSION *session)
{
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (session->smtp_com_state == SMTP_OPEN_CONNECTED)
    {
        /* Call NU_Close_Socket if we are using open socket. */
        if (session->smtp_sock >= 0)
            NU_Close_Socket(session->smtp_sock);
    }
    else if (session->smtp_com_state == SMTP_SSL_CONNECTED)
    {
        /* Call SSL routines if we are using SSL socket. */
        SSL_shutdown(session->smtp_ssl_struct);
        SSL_free(session->smtp_ssl_struct);
        NU_Close_Socket(session->smtp_sock);
    }

    /* Resetting connections. */
    session->smtp_sock              = -1;
    session->smtp_com_state         = SMTP_NO_CONNECTION;
    session->smtp_rfc_4954_state    = SMTP_AUTH_NOT_DONE;
    session->smtp_rfc_2487_state    = SMTP_TLS_NOT_DONE;

    /* Switch back to user mode */
    NU_USER_MODE();

    return(NU_SUCCESS);
} /* NU_SMTP_Close */

/***********************************************************************
*
* FUNCTION
*
*       NU_SMTP_Quit
*
* DESCRIPTION
*
*       Log of form a SMTP server.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Quit(SMTP_SESSION *session)
{
    STATUS status;
    INT16  ok_rep[2] = {221, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set state of server. */
    session->smtp_cmd = SMTP_DISCONNECTING;

    if (session->smtp_sock < 0)
    {
        /* check if we have an error. */
        SMTP_Error_Resp(session, SMTP_NOT_CONNECTED);

        /* Switch back to user mode */
        NU_USER_MODE();

        return(-1);
    }

    /* Send command to server. */
    status = NU_SMTP_Base_Cmd(session, SMTP_QUIT_CMD, NU_NULL,
            ok_rep, NU_NULL);

    /* Close the socket. */
    NU_SMTP_Close(session);

    session->smtp_error = status;

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Quit   */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Set_Extension_Settings
*
* DESCRIPTION
*
*       This will set extension setting.
*
* INPUTS
*
*       *session                Pointer to session object.
*       extension               Extension to set.
*       setting                 Setting that should be set.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       SMTP_SETTING_ERROR      If we got an error while changing
*                               extension setting, or provided extension
*                               don't exist.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS SMTP_Set_Extension_Settings(SMTP_SESSION *session,
        INT16 extension, INT16 setting)
{
    STATUS status = -1;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* All extensions descriptors are defined in their respective headers. */
    switch (extension)
    {
        /* RFC-4954 */
        case SMTP_AUTH_EXTEN:
            status = SMTP_AUTH_Setting(session, setting);
            break;

        /* RFC-2487 */
        case SMTP_TLS_EXTEN:
            status = SMTP_TLS_Setting(session, setting);
            break;

        default:
            break;
    }

    /* Check if we have successfully changed the settings. */
    if (status == -1)
    {
        status = SMTP_SETTING_ERROR;
        SMTP_Error_Resp(session, status);
    }
    else
    {
        status = NU_SUCCESS;
        SMTP_Error_Resp(session, NU_SUCCESS);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* SMTP_Set_Extension_Settings */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Base_Cmd
*
* DESCRIPTION
*
*       Builds command and sends command to the SMTP server.
*       Used INTernally to the SMTP protocol.
*
* INPUTS
*
*       *session                Pointer to session object.
*       command                 Command code.
*       *argument               Extra arguments if any.
*       *ok_rep                 Null terminating array of status codes
*                               returned by server if command was
*                               successful.
*       *reply                  If not NU_NULL server reply will be copied
*                               to this array.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       SMTP_UNKNOWN_COMMAND    If provided command is unknown.
*       NU_INVALID_PARM         If an invalid parameter is passed.
*
************************************************************************/
STATUS NU_SMTP_Base_Cmd(SMTP_SESSION *session, INT16 command, CHAR *argument,
        INT16 *ok_rep, CHAR *reply)
{
    STATUS  status;
    
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((session == NU_NULL) || (ok_rep == NU_NULL))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    session->smtp_cmd = command;

    /* Check if we have a valid command. */
    if ((command > SMTP_CONNECTING) && (command < SMTP_DISCONNECTING))
    {
        /* Build the command that we will be sending. */
        if (argument != NU_NULL)
            sprintf(session->smtp_cmd_sent, "%s %s\r\n",
                    SMTP_Commands[command], argument);
        else
            sprintf(session->smtp_cmd_sent, "%s\r\n",
                    SMTP_Commands[command]);

        /* Send command to the SMTP server. */
        status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                strlen(session->smtp_cmd_sent));

        if (status == NU_SUCCESS)
        {
            /* Receive reply from server. */
            status = SMTP_Recv(session);
        }
    }
    else
    {
        status = SMTP_UNKNOWN_COMMAND;
    }

    /* Check for errors. */
    SMTP_Check_Errors(session, &status, ok_rep);

    /* Copy reply string. */
    if (reply != NU_NULL)
    {
        strcpy(reply, session->smtp_resp);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_Base_Cmd */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Send_Bytes
*
* DESCRIPTION
*
*       This function will actually send data on the socket according to
*       session settings.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       *str                    Pointer to data to send.
*       len                     length of data to send.
*
* OUTPUTS
*
*       NU_SUCCESS              If the command was successful.
*       SMTP_NOT_CONNECTED      If there is no connection to server.
*       SMTP_SEND_FAILED        If there was some error while sending
*                               data.
*
************************************************************************/
STATUS SMTP_Send_Bytes(SMTP_SESSION *session, CHAR *str, INT32 len)
{
    STATUS  status = 0;
    INT32  sent = 0;
    INT32  sent_part = 0;

    if (session->smtp_com_state != SMTP_NO_CONNECTION)
    {
        for(;;)
        {
            sent_part = -1;

            if (session->smtp_com_state == SMTP_OPEN_CONNECTED)
            {
                /* Use "NU_Send" if we are on an open socket. */
                sent_part = NU_Send(session->smtp_sock,
                        &str[sent], len, 0);
            }
            else if (session->smtp_com_state == SMTP_SSL_CONNECTED)
            {
                /* Use "SSL_write" if we are using SSL socket. */
                sent_part = SSL_write(session->smtp_ssl_struct,
                        &str[sent], len);
            }

            /* Send remaining bytes if not all data has been send. */
            if (sent_part > 0)
            {
                sent += sent_part;
                if (sent == len)
                {
                    /* Return "NU_SUCCESS" if all the bytes are sent. */
                    status = NU_SUCCESS;
                    break;
                }
            }
            else
            {
                /* Return error if sending was unsuccessful. */
                status = SMTP_SEND_FAILED;
                break;
            }
        }
    }
    else
    {
        status = SMTP_NOT_CONNECTED;
    }

    return status;
} /* SMTP_Send_Bytes */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Data_Available
*
* DESCRIPTION
*
*       This function returns NU_SUCCESS when data is available to read
*       on SMTP socket. This function will only work for open socket as
*       for SSL socket underlying SSL API will handle it.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If there is some data available to read
*                               on socket.
*
*************************************************************************/
STATUS SMTP_Data_Available(SMTP_SESSION *session)
{
    STATUS  status;
    FD_SET  readfs;

    /* Set file descriptors. */
    NU_FD_Init(&readfs);
    NU_FD_Set(session->smtp_sock, &readfs);

    /* Wait for data. */
    status = NU_Select(session->smtp_sock + 1, &readfs,
            NU_NULL, NU_NULL, (SMTP_TIMEOUT));

    if (status == NU_SUCCESS)
    {
        /* Check if it is same file that we want to read from. */
        if (NU_FD_Check(session->smtp_sock, &readfs) != NU_TRUE)
            status = -1;
    }

    return status;
} /* SMTP_Data_Available */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Is_End_Response
*
* DESCRIPTION
*
*       This function will return NU_SUCCESS if we are have done receiving
*       response of a command from SMTP server.
*       End response is when SMTP server sends
*                   <3 digits> <sp> <string/+extended-code> <CRLF>.
*
*
* INPUTS
*
*       *str                    String containing the response from server.
*
* OUTPUTS
*
*       NU_SUCCESS              If we have end of response.
*       -1                      If this is not end of response.
*
*************************************************************************/
STATUS SMTP_Is_End_Response(CHAR *str)
{
    CHAR    *ptr = str + strlen(str);
    STATUS  status;

    /* Move two locations back. */
    ptr = ptr - 2;
    if (ptr >= str)
    {
        /* Check for <CRLF> */
        if ((ptr[0] == '\r') && (ptr[1] == '\n'))
        {
            /* Move a location back. */
            ptr --;

            /* Skip the <string/+extended-code> */
            while (ptr > str)
            {
                if ((*ptr == '\n') || (*ptr == '\r'))
                {
                    ptr++;
                    break;
                }
                else
                {
                    ptr--;
                }
            }

            /* Check for <3 digits> <sp>. */
            if (isdigit(ptr[0]) && isdigit(ptr[1]) &&
                    isdigit(ptr[2]) && (ptr[3] == ' '))
            {
                status = NU_SUCCESS;
            }
            else
                status = -1;
        }
        else
        {
            status = -1;
        }

    }
    else
    {
        status = -1;
    }

    return status;
} /* SMTP_Is_End_Response */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Recv_Bytes
*
* DESCRIPTION
*
*       This function will receive data from the SMTP socket until it
*       receives an end response.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       *dest                   Copy the received bytes in the
*                               location pointed by "str".
*       len                     Max data to receive.
*
* OUTPUTS
*
*       No of bytes received from server.
*       0                       If we have an error or connection is
*                               not opened.
*
*************************************************************************/
INT32 SMTP_Recv_Bytes(SMTP_SESSION *session, CHAR *dest, INT32 len)
{
    INT32   received = 0;
    CHAR    *ptr = dest;
    INT32   got;

    if (session->smtp_com_state != SMTP_NO_CONNECTION)
    {
        for (;;)
        {
            got = -1;

            /* Check for communication state and use */
            /* "NU_Recv" or "SSL_read" accordingly. */
            if (session->smtp_com_state == SMTP_OPEN_CONNECTED)
            {
                /* Wait for data if we are on an open socket. */
                if (SMTP_Data_Available(session) == NU_SUCCESS)
                {
                    /* Receive data from server. */
                    got = NU_Recv(session->smtp_sock, ptr,
                            (len - received), 0);
                }
                else
                {
                    break;
                }
            }
            else if (session->smtp_com_state == SMTP_SSL_CONNECTED)
            {
                /* Receive data from server. */
                got = SSL_read(session->smtp_ssl_struct, ptr,
                        (len - received));

            }

            if (got > 0)
            {
                received += got;
            }
            else
            {
                /* Return 0. */
                dest[0] = '\0';
                received = 0;
                break;
            }

            /* Set ptr to the end of received data. */
            ptr = dest + received;
            *ptr = '\0';

            /* Check if we at end of response. */
            if (SMTP_Is_End_Response(dest) == NU_SUCCESS)
            {
                break;
            }

            /* Check overflow condition. */
            if (received >= len)
            {
                break;
            }
        }
    }
    else
    {
        /* Zero data will specify that we got an error */
        /* during receiving data. */
        received = 0;
        dest[0] = '\0';
    }

    return received;
} /* SMTP_Recv_Bytes */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Recv
*
* DESCRIPTION
*
*       Does the real work of receiving data from the SMTP server.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If we have successfully received
*                               some data.
*       SMTP_NOT_CONNECTED      If there is no connection to server.
*       SMTP_RECEIVE_FAILED     If we were unable to receive some data.
*
*************************************************************************/
STATUS SMTP_Recv(SMTP_SESSION *session)
{
    STATUS  status;
    INT32   received = 0;

    /* Set first character to '\0' */
    session->smtp_resp[0] = '\0';

    /* Check if we are actually connected. */
    if (session->smtp_com_state != SMTP_NO_CONNECTION)
    {
        /* Receive data. */
        received = SMTP_Recv_Bytes(session, session->smtp_resp,
                SMTP_MAX_CMD_LINE);

        /* If data was successfully received. */
        if (received > 0)
        {
            session->smtp_resp[received] = 0;
            status = 0;
        }
        else
        {
            status = SMTP_RECEIVE_FAILED;
            session->smtp_error = SMTP_RECEIVE_FAILED;
        }
    }
    else
    {
        received = 0;
        status = SMTP_NOT_CONNECTED;
    }

    return(status);
} /* SMTP_Recv */

/***********************************************************************
*
* FUNCTION
*
*       SMTP_Base64_Encode
*
* DESCRIPTION
*
*       This function uses OpenSSL to encode data/information according to
*       Base64 standard.
*
*
* INPUTS
*
*       *string                 Data to be encoded.
*       **result                Return buffer.
*       length                  Size of data.
*
* OUTPUTS
*
*       NU_SUCCESS              If data was successfully encoded.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS SMTP_Base64_Encode(CHAR *string, CHAR **result, UINT32 length)
{
    BIO     *bmem=NULL, *b64=NULL;
    BUF_MEM *bptr=NULL;
    STATUS  status = -1;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((string == NU_NULL) || (length == 0))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if (length <= 0)
    {
        return NU_INVALID_PARM;
    }

    /* Initialize a base64 encoder. */
    b64 = BIO_new(BIO_f_base64());

    /* Initialize memory for base64 encoder. */
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    /* Send data to encoder. */
    BIO_write(b64, string, length);
    BIO_flush(b64);

    /* Get return pointer of data. */
    BIO_get_mem_ptr(b64, &bptr);

    /* Allocate memory to store data. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
            (VOID **)result,
            bptr->length + 3,
            NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Copy data from encoder. */
        memcpy(*result, bptr->data, bptr->length-1);
        (*result)[bptr->length-1] = '\0';

        /* Free memory and structures used by encoder. */
        BIO_free_all(b64);
    }

    if (status != NU_SUCCESS)
    {
        status = SMTP_ENCODE_ERROR;
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* SMTP_Base64_Encode */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Error_Resp
*
* DESCRIPTION
*
*       Sets up the error message pointer and length of error message.
*       Used internally to the SMTP protocol.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       error                   Error code that will be used to return
*                               the information about specific error.
*
* OUTPUTS
*
*       NONE
*
************************************************************************/
VOID SMTP_Error_Resp(SMTP_SESSION *session, UINT16 error)
{
    /* Set response string according to error. */
    if ((session->smtp_resp != NU_NULL) && (error != SMTP_COMMNAD_FAILED))
    {
        sprintf(session->smtp_resp, "%s\r\n", SMTP_Error_Messages[error]);
    }
    session->smtp_error = error;
} /* SMTP_Error_Resp */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Check_Errors
*
* DESCRIPTION
*
*       This function must be called after every command so that we can
*       check weather a command was successful or not.
*
* INPUTS
*
*       *session                Pointer to session object.
*       *status                 Return code of previous commands.
*       *ok_rep                 Null terminating array of status codes that
*                               a command might return if command was
*                               successful.
*
* OUTPUTS
*
*       NONE
*
************************************************************************/
VOID SMTP_Check_Errors(SMTP_SESSION *session, STATUS *status, INT16 *ok_rep)
{
    CHAR num[4];
    INT16 got_rep;

    /* First check if we have an error. */
    if (*status != NU_SUCCESS)
    {
        /* Set response accordingly. */
        SMTP_Error_Resp(session, *status);
        session->smtp_error = *status;
    }
    else
    {
        /* Now check for reply from server. */
        memcpy(num, session->smtp_resp, 3);
        num[3] = '\0';
        got_rep = atoi(num);

        /* Check if there is an error returned from server. */
        *status = SMTP_COMMNAD_FAILED;

        while (*ok_rep != 0)
        {
            if (got_rep == *ok_rep)
            {
                /* Status code matched. */
                *status = NU_SUCCESS;
                break;
            }
            ok_rep++;
        }

        if (*status != NU_SUCCESS)
        {
            /* There was an error. */
            session->smtp_error = SMTP_COMMNAD_FAILED;
        }
    }
} /* SMTP_Check_Errors */


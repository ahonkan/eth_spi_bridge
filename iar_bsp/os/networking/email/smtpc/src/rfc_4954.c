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
*       rfc_4954.c
*
*   COMPONENT
*
*       SMTP Client - RFC-4954.
*
*   DESCRIPTION
*
*       This file contains the RFC-4954 implementation for SMTP Client
*       API.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SMTP_AUTH_Is_Enabled
*       SMTP_AUTH_Setting
*       NU_SMTP_Auth
*
*   DEPENDENCIES
*
*       smtp_client.h
*       rfc_4954.h
*
*************************************************************************/
#include "os/networking/email/smtpc/inc/smtp_client.h"
#include "os/networking/email/smtpc/inc/rfc_4954.h"

/************************************************************************
*
* FUNCTION
*
*       SMTP_AUTH_Is_Enabled
*
* DESCRIPTION
*
*       This function will tell if authentication is enabled on server.
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       SMTP_AUTH_DISABLED      If authentication is disabled on server.
*       SMTP_AUTH_ENABLED       If authentication is enabled on server.
*
*************************************************************************/
INT16 SMTP_AUTH_Is_Enabled(SMTP_SESSION *session)
{
    INT16 ret;

    /* Set our setting according to request. */
    if ((((session->smtp_extensions >> SMTP_EXT_AUTH_PLAIN)
            & 0x01) == 1) ||
            (((session->smtp_extensions >> SMTP_EXT_AUTH_LOGIN)
                    & 0x01) == 1))
    {
        ret = SMTP_AUTH_ENABLED;
    }
    else
    {
        ret = SMTP_AUTH_DISABLED;
    }

    return (ret);
} /* SMTP_AUTH_Is_Enabled */

/************************************************************************
*
* FUNCTION
*
*       SMTP_AUTH_Setting
*
* DESCRIPTION
*
*       This function will set a specific option for AUTH according to
*       setting if that option was found in EHLO reply of server.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       setting                 Setting that is asked to be enabled.
*
* OUTPUTS
*
*       NU_SUCCESS              If setting was successful.
*
*************************************************************************/
STATUS SMTP_AUTH_Setting(SMTP_SESSION *session, INT16 setting)
{
    STATUS      status = -1;

    if (SMTP_AUTH_Is_Enabled(session) == SMTP_AUTH_ENABLED)
    {
        if (setting == SMTP_AUTH_PLAIN)
        {
            if (((session->smtp_extensions >> SMTP_EXT_AUTH_PLAIN)
                    & 0x01) == 1)
            {
                session->smtp_rfc_4954_setting = SMTP_AUTH_PLAIN;
                status = NU_SUCCESS;
            }
        }
        else if (setting == SMTP_AUTH_LOGIN)
        {
            if (((session->smtp_extensions >> SMTP_EXT_AUTH_LOGIN)
                    & 0x01) == 1)
            {
                session->smtp_rfc_4954_setting = SMTP_AUTH_LOGIN;
                status = NU_SUCCESS;
            }
        }
    }
    else if (setting == SMTP_AUTH_DISABLED)
    {
        session->smtp_rfc_4954_setting = SMTP_AUTH_DISABLED;
        status = NU_SUCCESS;
    }
    else
    {
        session->smtp_rfc_4954_setting = SMTP_AUTH_DISABLED;
    }

    return (status);
} /* SMTP_AUTH_Setting */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Auth
*
* DESCRIPTION
*
*       This function will send username and password to the server
*       according to AUTH settings.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*       *username               User name.
*       *password               Password linked to the user name.
*
* OUTPUTS
*
*       NU_SUCCESS              If server accepted our login.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_Auth(SMTP_SESSION *session, CHAR *username, CHAR *password)
{
    STATUS  status = NU_SUCCESS;
    CHAR    *ptr;
    CHAR    *encode_user = NU_NULL;
    CHAR    *encode_pass = NU_NULL;
    CHAR    *to_encode = NU_NULL;
    INT16   ok_rep[2] = {0, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((session == NU_NULL) || (username == NU_NULL)
            || (password == NU_NULL) || (strlen(username) == 0)
            || (strlen(password) == 0))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check current setting of AUTH. */
    switch (session->smtp_rfc_4954_setting)
    {
        case SMTP_AUTH_PLAIN:
            /* Allocate memory for encoding buffer. */
            status = NU_Allocate_Memory(SMTP_Memory_Pool,
                    (VOID **)&to_encode,
                    SMTP_MAX_CMD_LINE,
                    NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* We will send username and password in PLAIN format. */
                ptr = to_encode;

                /* Copy username and password in the format of */
                /* "\0username\0password". */
                *ptr++ = '\0';
                strcpy(ptr, username);
                ptr += strlen(username);
                *ptr++ = '\0';
                strcpy(ptr, password);

                /* Build command string. */
                strcpy(session->smtp_cmd_sent, "AUTH PLAIN ");

                /* Encode username and password in Base64 format. */
                status = SMTP_Base64_Encode(to_encode, &encode_user,
                        strlen(username) + strlen(password) + 2);
            }
            else
            {
                to_encode = NU_NULL;
            }

            /* Send encoded login to server. */
            if (status == NU_SUCCESS)
            {
                if (strlen(encode_user) < (SMTP_MAX_CMD_LINE -
                        strlen(session->smtp_cmd_sent) - 3))
                {
                    strcat(session->smtp_cmd_sent, encode_user);
                    strcat(session->smtp_cmd_sent, "\r\n");


                    /* Send command to server. */
                    status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));
                }
                else
                {
                    status = SMTP_ENCODE_ERROR;
                }

                NU_Deallocate_Memory(encode_user);
            }

            if (status == NU_SUCCESS)
            {
                /* Receive reply from server. */
                status = SMTP_Recv(session);
            }

            /* Check if user accepted the password. */
            ok_rep[0] = 235;
            SMTP_Check_Errors(session, &status, ok_rep);

            /* Set RFC-4954 state. */
            if (status == NU_SUCCESS)
            {
                session->smtp_rfc_4954_state = SMTP_AUTH_DONE;
            }

            break;

        case SMTP_AUTH_LOGIN:
            /* Allocate memory for encoding buffer. */
            status = NU_Allocate_Memory(SMTP_Memory_Pool,
                    (VOID **)&to_encode,
                    SMTP_MAX_CMD_LINE,
                    NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Build data to encode. */
                strcpy(to_encode, username);
                strcpy(session->smtp_cmd_sent, "AUTH LOGIN ");

                /* Encode username. */
                status = SMTP_Base64_Encode(to_encode,
                        &encode_user, strlen(username));
            }
            else
            {
                to_encode = NU_NULL;
            }

            /* Send our username. */
            if (status == NU_SUCCESS)
            {
                if (strlen(encode_user) < (SMTP_MAX_CMD_LINE -
                        strlen(session->smtp_cmd_sent) - 3))
                {
                    strcat(session->smtp_cmd_sent, encode_user);
                    strcat(session->smtp_cmd_sent, "\r\n");

                    /* Send "AUTH LOGIN <username in base64>". */
                    status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                            strlen(session->smtp_cmd_sent));
                }
                else
                {
                    status = SMTP_ENCODE_ERROR;
                }

                /* Deallocate memory. */
                NU_Deallocate_Memory(encode_user);
            }

            if (status == NU_SUCCESS)
            {
                /* Receive reply from server. */
                status = SMTP_Recv(session);
            }

            /* Check if user accepted the username. */
            ok_rep[0] = 334;
            SMTP_Check_Errors(session, &status, ok_rep);

            /* Now encode password and send it. */
            if ((status == NU_SUCCESS) && (to_encode != NU_NULL))
            {
                /* Encode password. */
                strcpy(to_encode, password);
                status = SMTP_Base64_Encode(to_encode,
                        &encode_pass, strlen(password));
            }

            /* Now send password. */
            if (status == NU_SUCCESS && (encode_pass != NU_NULL))
            {
                if (strlen(encode_pass) <
                        (SMTP_MAX_CMD_LINE - strlen(session->smtp_cmd_sent)))
                {
                    strcpy(session->smtp_cmd_sent, encode_pass);
                    strcat(session->smtp_cmd_sent, "\r\n");

                    /* Send password. */
                    status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                            strlen(session->smtp_cmd_sent));
                }
                else
                {
                    status = SMTP_ENCODE_ERROR;
                }

                /* Deallocate memory. */
                NU_Deallocate_Memory(encode_pass);
            }

            /* Check if user accepted our login. */
            if (status == NU_SUCCESS)
            {
                /* Receive server reply. */
                status = SMTP_Recv(session);

                /* Check if user accepted the password. */
                ok_rep[0] = 235;
                SMTP_Check_Errors(session, &status, ok_rep);

                /* Set RFC-4954 state. */
                if (status == NU_SUCCESS)
                {
                    session->smtp_rfc_4954_state = SMTP_AUTH_DONE;
                }

            }

            break;

        case SMTP_AUTH_DISABLED:

            /* We will send NU_SUCCESS as this command is not enabled. */
            status = NU_SUCCESS;
            sprintf(session->smtp_resp,
                    "SMTP: Authentication disabled\r\n");
            break;

        default:
            status = SMTP_COMMNAD_FAILED;
            break;
    }

    if (to_encode != NU_NULL)
    {
        /* Deallocate memory. */
        NU_Deallocate_Memory(to_encode);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Auth */

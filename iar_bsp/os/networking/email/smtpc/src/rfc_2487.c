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
*       rfc_2487.c
*
*   COMPONENT
*
*       SMTP Client - RFC-2487.
*
*   DESCRIPTION
*
*       This file contains the RFC-2487 implementation for SMTP Client
*       API.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SMTP_TLS_Is_Enabled
*       SMTP_TLS_Setting
*       NU_SMTP_StartTLS
*
*   DEPENDENCIES
*
*       smtp_client.h
*       rfc_2487.h
*
*************************************************************************/
#include "os/networking/email/smtpc/inc/smtp_client.h"
#include "os/networking/email/smtpc/inc/rfc_2487.h"

/************************************************************************
*
* FUNCTION
*
*       SMTP_TLS_Is_Enabled
*
* DESCRIPTION
*
*       This function will tell if Start-TLS is enabled on server.
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       SMTP_TLS_ENABLED        If Start-TLS is disabled on server.
*       SMTP_TLS_DISABLED       If Start-TLS is enabled on server.
*
*************************************************************************/
INT16 SMTP_TLS_Is_Enabled(SMTP_SESSION *session)
{
    INT16 ret;

    /* Set our setting according to request. */
    if (((session->smtp_extensions >> SMTP_EXT_TLS)
            & 0x01) == 1)
    {
        ret = SMTP_TLS_ENABLED;
    }
    else
    {
        ret = SMTP_TLS_DISABLED;
    }

    return (ret);
} /* SMTP_TLS_Is_Enabled */

/************************************************************************
*
* FUNCTION
*
*       SMTP_TLS_Setting
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
STATUS SMTP_TLS_Setting(SMTP_SESSION *session, INT16 setting)
{
    STATUS      status = -1;

    /* Set our setting according to request. */
    if (SMTP_TLS_Is_Enabled(session) == SMTP_TLS_ENABLED)
    {
        if (setting == SMTP_TLS_ENABLED)
        {
            session->smtp_rfc_2487_setting = SMTP_TLS_ENABLED;
            status = NU_SUCCESS;
        }
        else if (setting == SMTP_TLS_DISABLED)
        {
            session->smtp_rfc_2487_setting = SMTP_TLS_DISABLED;
            status = NU_SUCCESS;
        }
    }
    else
    {
        session->smtp_rfc_2487_setting = SMTP_TLS_DISABLED;
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
*       This function will encrypt connection by sending STARTTLS and
*       starting SSL negotiation.
*
*
* INPUTS
*
*       *session                Pointer to session object.
*
* OUTPUTS
*
*       NU_SUCCESS              If we Start-TLS was successful.
*       NU_INVALID_PARM         If an invalid argument is passed.
*
*************************************************************************/
STATUS NU_SMTP_StartTLS(SMTP_SESSION *session)
{
    STATUS status;
    INT16  ok_rep[2] = {220, 0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Send STARTTLS to server. */
    if (session->smtp_rfc_2487_setting == SMTP_TLS_ENABLED)
    {
        status = NU_SMTP_Base_Cmd(session, SMTP_STARTTLS_CMD, NU_NULL,
                ok_rep, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Command was successful now encrypt our connection. */
            status = SMTP_SSLD_Connect(&(session->smtp_ssl_struct),
                    (INT)session->smtp_sock, &session->smtp_ssl_ctx);
        }

        if (status == NU_SUCCESS)
        {
            /* Set socket state as we are now encrypted. */
            session->smtp_com_state = SMTP_SSL_CONNECTED;

            /* Set RFC-2487 state. */
            session->smtp_rfc_2487_state = SMTP_TLS_DONE;
        }
    }
    else
    {
        status = NU_SUCCESS;
        sprintf(session->smtp_resp, "SMTP: Start-TLS disabled.\r\n");
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return(status);
} /* NU_SMTP_StartTLS */

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
*       smtp_message.c
*
*   COMPONENT
*
*       SMTP Client - Message MIME Support.
*
*   DESCRIPTION
*
*       This file contains the message MIME support for SMTP Client
*       API.
*
*   DATA STRUCTURES
*
*       SMTP_To_Types
*
*   FUNCTIONS
*
*       SMTP_Send_Txt
*       SMTP_Send_Bin
*       SMTP_Send_Message_Data
*
*   DEPENDENCIES
*
*       smtp_client_api.h
*       smtp_client.h
*       smtp_message.h
*
*************************************************************************/
#include "networking/smtp_client_api.h"
#include "os/networking/email/smtpc/inc/smtp_client.h"
#include "os/networking/email/smtpc/inc/smtp_message.h"

/* Forward address types. */
CHAR *SMTP_To_Types[] =
{
    "TO",
    "CC",
    "BCC"
};

/************************************************************************
*
* FUNCTION
*
*       SMTP_Send_Txt
*
* DESCRIPTION
*
*       This function will send text part of message.
*
*
* INPUTS
*
*       *session                SMTP session structure.
*       *msg_part               Message part to send.
*
* OUTPUTS
*
*       NU_SUCCESS              If data has been successfully sent.
*
*************************************************************************/
STATUS SMTP_Send_Txt(SMTP_SESSION *session, SMTP_PART *msg_part)
{
    STATUS      status = NU_SUCCESS;
    SMTP_LINE   lineobj;
    CHAR        *line_ptr;

    status = SMTP_Init_Line_Obj(&lineobj, msg_part->smtp_data, "\r\n",
            msg_part->smtp_data_len, SMTP_MAX_TXT_LINE);

    if (status == NU_SUCCESS)
    {
        line_ptr = SMTP_Get_Line(&lineobj);

        while ((line_ptr != NU_NULL)
                && (status == NU_SUCCESS))
        {
            /* If there is a period in the first column we will send */
            /* an extra period, server will remove it. */
            if (line_ptr[0] == '.')
            {
                status = SMTP_Send_Bytes(session, ".", 1);
            }

            if ((line_ptr[0] != '\0') && (status == NU_SUCCESS))
            {
                status = SMTP_Send_Bytes(session, line_ptr, strlen(line_ptr));
            }

            if (status == NU_SUCCESS)
            {
                status = SMTP_Send_Bytes(session, "\r\n", 2);

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s\r\n", line_ptr);
#endif
                line_ptr = SMTP_Get_Line(&lineobj);
            }
        }

        /* Delete line object. */
        SMTP_Delete_Line_Obj(&lineobj);
    }

    return status;
} /* SMTP_Send_Txt */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Send_Bin
*
* DESCRIPTION
*
*       This function will send binary part of message.
*
*
* INPUTS
*
*       *session                SMTP session structure.
*       *part                   Message part to send.
*
* OUTPUTS
*
*       NU_SUCCESS              If data has been successfully sent.
*
*************************************************************************/
STATUS SMTP_Send_Bin(SMTP_SESSION *session, SMTP_PART *part)
{
    STATUS      status = NU_SUCCESS;
    CHAR        *encode_result;
    SMTP_LINE   lineobj;
    CHAR        *line_ptr;

    status = SMTP_Base64_Encode(part->smtp_data,
            &encode_result,
            part->smtp_data_len);

    if (status == NU_SUCCESS)
    {
        status = SMTP_Init_Line_Obj(&lineobj, encode_result, "\n",
                strlen(encode_result), SMTP_MAX_TXT_LINE);

        if (status == NU_SUCCESS)
        {
            line_ptr = SMTP_Get_Line(&lineobj);

            while ((line_ptr != NU_NULL)
                    && (status == NU_SUCCESS))
            {
                if (line_ptr[0] != '\0')
                {
                    status = SMTP_Send_Bytes(session, line_ptr, strlen(line_ptr));
                }

                if (status == NU_SUCCESS)
                {
                    status = SMTP_Send_Bytes(session, "\r\n", 2);

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                    printf("%s\r\n", line_ptr);
#endif
                    line_ptr = SMTP_Get_Line(&lineobj);
                }
            }

            /* Delete line object. */
            SMTP_Delete_Line_Obj(&lineobj);
        }

        NU_Deallocate_Memory(encode_result);
    }

    return status;
} /* SMTP_Send_Bin */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Send_Message_Data
*
* DESCRIPTION
*
*       This function will send a message that was initialized earlier.
*
*
* INPUTS
*
*       *session                Session object.
*       *message                Message object.
*
* OUTPUTS
*
*       NU_SUCCESS              If message was successfully sent.
*
*************************************************************************/
STATUS SMTP_Send_Message_Data(SMTP_SESSION *session, SMTP_MSG *message)
{
    STATUS      status = -1;
    SMTP_RECPT  *to_ptr;
    SMTP_PART   *part_ptr;
    UINT8       bound_en = 0;
    INT16       ok_rep[2] = {250, 0};

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Send FROM header. */
    sprintf(session->smtp_cmd_sent, "FROM: <%s>\r\n", message->smtp_from_addr);
    status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
            strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
    printf("%s", session->smtp_cmd_sent);
#endif

    /* Send TO headers. */
    if (status == NU_SUCCESS)
    {
        to_ptr = message->smtp_recpt_list.smtp_front;

        while ((to_ptr != NU_NULL) && (status == NU_SUCCESS))
        {
            sprintf(session->smtp_cmd_sent, "%s: <%s>\r\n",
                    SMTP_To_Types[to_ptr->smtp_recpt_type],
                    to_ptr->smtp_addr);
            status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                    strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("%s", session->smtp_cmd_sent);
#endif
            to_ptr = to_ptr->smtp_next;
        }
    }

    /* Send SUBJECT header. */
    if (status == NU_SUCCESS)
    {
        sprintf(session->smtp_cmd_sent, "SUBJECT: %s\r\n",
                message->smtp_subject);
        status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("%s", session->smtp_cmd_sent);
#endif
    }

    /* Send MIME header. */
    if (status == NU_SUCCESS)
    {
        sprintf(session->smtp_cmd_sent, "%s\r\n", SMTP_MIME_HEADER);
        status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("%s", session->smtp_cmd_sent);
#endif
    }

    /* Send boundary if we have a multipart message. */
    if (status == NU_SUCCESS)
    {
        if (message->smtp_part_list.smtp_front->smtp_next != NU_NULL)
        {
            sprintf(session->smtp_cmd_sent, "%s%s;\r\n",
                    SMTP_MIME_CNT_TYPE, SMTP_MIME_CNT_MULTI);
            status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                    strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("%s", session->smtp_cmd_sent);
#endif
            if (status == NU_SUCCESS)
            {
                sprintf(session->smtp_cmd_sent, "%s%s\"%s\"\r\n",
                        SMTP_MIME_TAB, SMTP_MIME_BOUND, SMTP_MIME_BOUNDRY);
                status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s", session->smtp_cmd_sent);
#endif
            }
            bound_en = 1;
        }
    }

    /* Get first part. */
    part_ptr = message->smtp_part_list.smtp_front;

    /* Send all parts. */
    while((part_ptr != NU_NULL) && (status == NU_SUCCESS))
    {
        /* If we have a multipart message better send a start of boundary. */
        if ((bound_en == 1) && (status == NU_SUCCESS))
        {
            sprintf(session->smtp_cmd_sent, "--%s\r\n", SMTP_MIME_BOUNDRY);
            status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                    strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("%s", session->smtp_cmd_sent);
#endif
        }

        /* Send the content according to it's type. */
        if ((part_ptr->smtp_type == SMTP_MSG_TXT) && (status == NU_SUCCESS))
        {
            /* Tell this is text content. */
            sprintf(session->smtp_cmd_sent, "%s%s\r\n\r\n",
                    SMTP_MIME_CNT_TYPE, SMTP_MIME_CNT_TXT);
            status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                    strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("%s", session->smtp_cmd_sent);
#endif
            if (status == NU_SUCCESS)
            {
                /* Send text content. */
                status = SMTP_Send_Txt(session, part_ptr);
            }
        }
        else if ((part_ptr->smtp_type == SMTP_ATT_TXT) &&
                (status == NU_SUCCESS))
        {
            /* Tell this is text content. */
            sprintf(session->smtp_cmd_sent, "%s%s\r\n", SMTP_MIME_CNT_TYPE,
                    SMTP_MIME_CNT_TXT);
            status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                    strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("%s", session->smtp_cmd_sent);
#endif
            if (status == NU_SUCCESS)
            {
                /* Tell this will be an attachment. */
                sprintf(session->smtp_cmd_sent, "%s%s;\r\n",
                        SMTP_MIME_CNT_DIS,
                        SMTP_MIME_CNT_ATT);
                status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s", session->smtp_cmd_sent);
#endif
            }

            if (status == NU_SUCCESS)
            {
                /* Tell the attachment name. */
                sprintf(session->smtp_cmd_sent, "%s%s\"%s\"\r\n\r\n",
                        SMTP_MIME_TAB,
                        SMTP_MIME_CNT_FL_NM,
                        part_ptr->smtp_att_name);
                status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s", session->smtp_cmd_sent);
#endif
            }

            if (status == NU_SUCCESS)
            {
                /* Send text content. */
                status = SMTP_Send_Txt(session, part_ptr);
            }
        }
        else if ((part_ptr->smtp_type == SMTP_ATT_BIN) &&
                (status == NU_SUCCESS))
        {
            /* Tell this is binary content. */
            sprintf(session->smtp_cmd_sent, "%s%s\r\n", SMTP_MIME_CNT_TYPE,
                    SMTP_MIME_CNT_BIN);
            status = SMTP_Send_Bytes(session,
                    session->smtp_cmd_sent,
                    strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("%s", session->smtp_cmd_sent);
#endif
            if (status == NU_SUCCESS)
            {
                /* Tell this will be an attachment. */
                sprintf(session->smtp_cmd_sent, "%s%s;\r\n",
                        SMTP_MIME_CNT_DIS,
                        SMTP_MIME_CNT_ATT);
                status = SMTP_Send_Bytes(session,
                        session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s", session->smtp_cmd_sent);
#endif
            }

            if (status == NU_SUCCESS)
            {
                /* Tell the attachment name. */
                sprintf(session->smtp_cmd_sent, "%s%s\"%s\"\r\n", SMTP_MIME_TAB,
                        SMTP_MIME_CNT_FL_NM,
                        part_ptr->smtp_att_name);
                status = SMTP_Send_Bytes(session,
                        session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s", session->smtp_cmd_sent);
#endif
            }

            if (status == NU_SUCCESS)
            {
                /* Tell that content will be transfered in base64 encoding. */
                sprintf(session->smtp_cmd_sent, "%s%s\r\n\r\n",
                        SMTP_MIME_CNT_TRANS,
                        SMTP_MIME_CNT_B64);
                status = SMTP_Send_Bytes(session,
                        session->smtp_cmd_sent,
                        strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("%s", session->smtp_cmd_sent);
#endif
            }

            if (status == NU_SUCCESS)
            {
                /* Send binary content. */
                status = SMTP_Send_Bin(session, part_ptr);
            }
        }

        part_ptr = part_ptr->smtp_next;
    }

    /* If we have a multipart message batter send end boundary. */
    if ((bound_en == 1) && (status == NU_SUCCESS))
    {
        sprintf(session->smtp_cmd_sent, "--%s--\r\n", SMTP_MIME_BOUNDRY);
        status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("%s", session->smtp_cmd_sent);
#endif
    }

    /* Terminate message by sending end characters. */
    if (status == NU_SUCCESS)
    {
        sprintf(session->smtp_cmd_sent, "%s", SMTP_MSG_END);
        status = SMTP_Send_Bytes(session, session->smtp_cmd_sent,
                strlen(session->smtp_cmd_sent));

#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("%s", session->smtp_cmd_sent);
#endif
    }

    /* Receive reply from server. */
    if (status == NU_SUCCESS)
    {
        status = SMTP_Recv(session);
    }

    SMTP_Check_Errors(session, &status, ok_rep);

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* SMTP_Send_Message_Data */

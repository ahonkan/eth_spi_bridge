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
*       smtp_api.c
*
*   COMPONENT
*
*       SMTP Client.
*
*   DESCRIPTION
*
*       This file contains SMTP high level API implementation.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       NU_SMTP_Session_Init
*       NU_SMTP_Session_Set_Login
*       NU_SMTP_Msg_Init
*       NU_SMTP_Msg_Set_Subject
*       NU_SMTP_Msg_Set_Sender
*       NU_SMTP_Msg_Add_Recipient
*       NU_SMTP_Msg_Add_Body
*       NU_SMTP_Msg_Add_Attachment
*       SMTP_Message_OK
*       NU_SMTP_Session_Connect
*       NU_SMTP_Send_Message
*       NU_SMTP_Session_Disconnect
*       NU_SMTP_Session_Delete
*       NU_SMTP_Msg_Delete
*
*   DEPENDENCIES
*
*       smtp_client.h
*       smtp_client_api.h
*       sll.h
*
*************************************************************************/
#include "os/networking/email/smtpc/inc/smtp_client.h"
#include "networking/smtp_client_api.h"
#include "networking/sll.h"

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Session_Init
*
* DESCRIPTION
*
*       This function will initialize a session object and return
*       it's pointer.
*
*       Connection modes can be
*           SMTP_OPEN           No encryption.
*           SMTP_SSL            SSL socket.
*           SMTP_TLS            Start-TLS based security.
*
* INPUTS
*
*       *address                String containing server address.
*       port                    Port of server we will use to connect.
*       connection_mode         Connection mode we will be using when
*                               connecting to the server.
*
* OUTPUTS
*
*       Pointer to session object if it successfully initialized.
*       NU_NULL                 If we were unable to allocate memory.
*
*************************************************************************/
SMTP_SESSION *NU_SMTP_Session_Init(CHAR *address, UINT16 port,
        INT16 connection_mode)
{
    STATUS          status;
    SMTP_SESSION    *session;
    NU_HOSTENT      *hentry;
    CHAR            address_ip[MAX_ADDRESS_SIZE] = {0};

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (((connection_mode != SMTP_OPEN) && (connection_mode != SMTP_SSL)
            && (connection_mode != SMTP_TLS)) ||
            (address == NU_NULL) || (port == 0) )
        return NU_NULL;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory for this object. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&session,
                sizeof(SMTP_SESSION),
                NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Initialize this object. */
        session->smtp_sock              = 0;
        session->smtp_error             = 0;
        session->smtp_com_mode          = connection_mode;
        session->smtp_com_state         = SMTP_NO_CONNECTION;
        session->smtp_username          = NU_NULL;
        session->smtp_password          = NU_NULL;
        session->smtp_rfc_4954_setting  = SMTP_AUTH_DISABLED;
        session->smtp_rfc_2487_setting  = SMTP_TLS_DISABLED;
        session->smtp_rfc_4954_state    = SMTP_AUTH_NOT_DONE;
        session->smtp_rfc_2487_state    = SMTP_TLS_NOT_DONE;

        /* Set server connection parameters. */
        session->smtp_serv.port = port;
        session->smtp_serv.name = "SMTPC";


#if (INCLUDE_IPV6 == NU_TRUE)
        if (strchr(address, ':') != NU_NULL)
        {
            /* Search for : in "address" as an IPv6 address will have one. */
            session->smtp_serv.family = NU_FAMILY_IP6;
            status = NU_Inet_PTON(NU_FAMILY_IP6, address, address_ip);
        }
        else
#if (INCLUDE_IPV4 == NU_FALSE)
        {
            /* An IPv6 address was not passed into the routine. */
            status = -1;
        }
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            /* Try to convert "address" into an array for IPv4 format. */
            session->smtp_serv.family = NU_FAMILY_IP;
            status = NU_Inet_PTON(NU_FAMILY_IP, address, address_ip);
        }
#endif

        /* If the address contains an IP address, copy it into the server structure. */
        if (status == NU_SUCCESS)
        {
            memcpy(&session->smtp_serv.id.is_ip_addrs, address_ip, MAX_ADDRESS_SIZE);
        }
        else
        {
            /* If "address" is not an ip-address try to resolve it */
            session->smtp_serv.family = NU_FAMILY_IP;

            /* Try getting host info by name. */
            hentry = NU_Get_IP_Node_By_Name(address, session->smtp_serv.family, 0, &status);

            if ((status == NU_SUCCESS) && (hentry != NU_NULL))
            {
                /* Copy the "hentry" data into the server structure. */
                memcpy(&session->smtp_serv.id.is_ip_addrs, *hentry->h_addr_list, hentry->h_length);
                session->smtp_serv.family = hentry->h_addrtype;

                /* Free the memory associated with the host entry returned. */
                NU_Free_Host_Entry(hentry);
            }
            else
            {
                /* We were unable to parse the address. */
                status = -1;
            }
        }
    }
    else
    {
        /* We were unable to allocate memory return NU_NULL. */
        session = NU_NULL;
    }

    /* Deallocate memory for object if we have an error.  */
    if ((status != NU_SUCCESS) && (session != NU_NULL))
    {
        NU_Deallocate_Memory(session);
        session = NU_NULL;
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return session;
} /* NU_SMTP_Session_Init */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Session_Set_Login
*
* DESCRIPTION
*
*       This function will set username and password used to
*       login in the server.
*
* INPUTS
*
*       *session                Session object.
*       *username               Username to use.
*       *password               Password to use.
*
* OUTPUTS
*
*       NU_SUCCESS              If username and password were
*                               successfully set.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Session_Set_Login(SMTP_SESSION *session,
        CHAR *username, CHAR *password)
{
    STATUS status = NU_SUCCESS;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((session == NU_NULL) || (username == NU_NULL) ||
            (password == NU_NULL))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory to store username and password. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&(session->smtp_username),
                strlen(username) + strlen(password) + 2,
                NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Set username and password. */
        session->smtp_password =
                ((CHAR *)session->smtp_username) + strlen(username) + 1;

        strcpy(session->smtp_username, username);
        strcpy(session->smtp_password, password);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Session_Set_Login */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Init
*
* DESCRIPTION
*
*       This function will initialize a message object and return
*       it's pointer.
*
* INPUTS
*
*       NONE
*
* OUTPUTS
*
*       Pointer to message object if it successfully initialized.
*       NU_NULL                 If we were unable to allocate memory.
*
*************************************************************************/
SMTP_MSG *NU_SMTP_Msg_Init(VOID)
{
    STATUS status;
    SMTP_MSG *message;

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory for this object. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&message,
                sizeof(SMTP_MSG),
                NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Initialize object. */
        message->smtp_part_list.smtp_front = NU_NULL;
        message->smtp_part_list.smtp_last = NU_NULL;

        message->smtp_recpt_list.smtp_front = NU_NULL;
        message->smtp_recpt_list.smtp_last = NU_NULL;

        message->smtp_subject = NU_NULL;
        message->smtp_from_addr = NU_NULL;
    }
    else
    {
        /* We were unable to allocate memory for this object return NU_NULL. */
        message = NU_NULL;
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return message;
} /* NU_SMTP_Msg_Init */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Set_Subject
*
* DESCRIPTION
*
*       This function will set subject of message.
*
* INPUTS
*
*       *message                Message object pointer.
*       *subject                Subject of message.
*
* OUTPUTS
*
*       NU_SUCCESS              If subject was successfully set.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Msg_Set_Subject(SMTP_MSG *message, CHAR *subject)
{
    STATUS status = NU_SUCCESS;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((subject == NU_NULL) || (message == NU_NULL) ||
            (strlen(subject) >= SMTP_MAX_SUB))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If subject was already set remove it. */
    if (message->smtp_subject != NU_NULL)
    {
        status = NU_Deallocate_Memory(message->smtp_subject);
    }

    /* Allocate memory to store subject. */
    if (status == NU_SUCCESS)
    {
        status = NU_Allocate_Memory(SMTP_Memory_Pool,
                    (VOID **)&message->smtp_subject,
                    strlen(subject) + 1,
                    NU_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        /* Set message subject. */
        strcpy(message->smtp_subject, subject);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Msg_Set_Subject */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Set_Sender
*
* DESCRIPTION
*
*       This function will set sender address of message.
*
* INPUTS
*
*       *message                Message object pointer.
*       *sender                 Sender address.
*
* OUTPUTS
*
*       NU_SUCCESS              If address was successfully set.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Msg_Set_Sender(SMTP_MSG *message, CHAR *sender)
{
    STATUS status = NU_SUCCESS;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((sender == NU_NULL) || (message == NU_NULL) ||
            (strlen(sender) >= SMTP_MAX_ADDR))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If sender's address was already set delete it. */
    if (message->smtp_from_addr != NU_NULL)
    {
        status = NU_Deallocate_Memory(message->smtp_from_addr);
    }

    /* Allocate memory to store sender's address. */
    if (status == NU_SUCCESS)
    {
        status = NU_Allocate_Memory(SMTP_Memory_Pool,
                    (VOID **)&message->smtp_from_addr,
                    strlen(sender) + 1,
                    NU_SUSPEND);
    }

    if (status == NU_SUCCESS)
    {
        /* Set sender's address. */
        strcpy(message->smtp_from_addr, sender);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Msg_Set_Sender */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Add_Recipient
*
* DESCRIPTION
*
*       This function will add a recipient address to the message.
*
*       Recipient types can be
*           SMTP_MSG_TO         "TO"
*           SMTP_MSG_CC         "CC"
*           SMTP_MSG_BCC        "BCC"
*
* INPUTS
*
*       *message                Message object pointer.
*       *recpt                  Recipient address.
*       recpt_type              This will be sent in header of message
*
* OUTPUTS
*
*       NU_SUCCESS              If recipient was successfully added.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Msg_Add_Recipient(SMTP_MSG *message, CHAR *recpt, INT16 recpt_type)
{
    STATUS status = NU_SUCCESS;
    SMTP_RECPT *recpt_node;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((recpt == NU_NULL) || (message == NU_NULL) ||
            (strlen(recpt) >= SMTP_MAX_ADDR) ||
            ((recpt_type != SMTP_MSG_TO) &&
            (recpt_type != SMTP_MSG_CC) &&
            (recpt_type != SMTP_MSG_BCC)))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory to store recipient address. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&recpt_node,
                sizeof(SMTP_RECPT) + strlen(recpt) + 1,
                NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Set recipient list object. */
        recpt_node->smtp_addr = ((CHAR *)recpt_node) + sizeof(SMTP_RECPT);
        strcpy(recpt_node->smtp_addr, recpt);
        recpt_node->smtp_recpt_type = recpt_type;

        /* Enqueue this object in the recipient's list. */
        SLL_Enqueue((VOID *)&message->smtp_recpt_list, (VOID *)recpt_node);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Msg_Add_Recipient */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Add_Body
*
* DESCRIPTION
*
*       This function will add text body to the message.
*
* INPUTS
*
*       *message                Message object pointer.
*       *body_txt               Text content of body.
*
* OUTPUTS
*
*       NU_SUCCESS              If body was successfully added.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Msg_Add_Body(SMTP_MSG *message, CHAR *body_txt)
{
    STATUS status = NU_SUCCESS;
    SMTP_PART *part_node;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((body_txt == NU_NULL) || (message == NU_NULL))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory to store body. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&part_node,
                sizeof(SMTP_PART) + strlen(body_txt) + 1,
                NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Set part object according to our body. */
        part_node->smtp_data = ((CHAR *)part_node) + sizeof(SMTP_PART);
        strcpy(part_node->smtp_data, body_txt);

        part_node->smtp_att_name = NU_NULL;
        part_node->smtp_data_len = strlen(body_txt);
        part_node->smtp_type = SMTP_MSG_TXT;

        /* Enqueue part object in our part list. */
        SLL_Enqueue((VOID *)&message->smtp_part_list, (VOID *)part_node);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Msg_Add_Body */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Add_Attachment
*
* DESCRIPTION
*
*       This function will add an attachment to the message object.
*       Types of attachments can be
*           SMTP_ATT_TXT        Attachment will be sent as text.
*           SMTP_ATT_BIN        Attachment will be sent in base64 format.
*
* INPUTS
*
*       *message                Message object pointer.
*       *attachment             Pointer to data of attachment.
*       *name                   Name of file that will be used while
*                               sending an attachment.
*       len                     Length of attachment data in bytes.
*       type                    Type of attachment.
*
* OUTPUTS
*
*       NU_SUCCESS              If attachment was successfully added.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Msg_Add_Attachment(SMTP_MSG *message,
        CHAR *attachment, CHAR *name, INT32 len, INT16 type)
{
    STATUS status = NU_SUCCESS;
    SMTP_PART *part_node;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((attachment == NU_NULL) || (message == NU_NULL) || (len == 0) ||
            ((type != SMTP_ATT_TXT) && (type != SMTP_ATT_BIN)))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory to store this attachment. */
    status = NU_Allocate_Memory(SMTP_Memory_Pool,
                (VOID **)&part_node,
                sizeof(SMTP_PART) + strlen(name) + len + 1,
                NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Set part object according to our attachment. */
        part_node->smtp_att_name = ((CHAR *)part_node) + sizeof(SMTP_PART);
        part_node->smtp_data = ((CHAR *)part_node) + sizeof(SMTP_PART) + strlen(name) + 1;

        /* Copy Attachment name. */
        strcpy(part_node->smtp_att_name, name);

        /* Copy attachment data. */
        memcpy(part_node->smtp_data, attachment, len);

        part_node->smtp_data_len = len;
        part_node->smtp_type = type;

        /* Enqueue this part in our part list. */
        SLL_Enqueue((VOID *)&message->smtp_part_list, (VOID *)part_node);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Msg_Add_Attachment */

/************************************************************************
*
* FUNCTION
*
*       SMTP_Message_OK
*
* DESCRIPTION
*
*       This function will return 1 if message object is populated
*       enough to be sent.
*
* INPUTS
*
*       *message                Message object pointer.
*
* OUTPUTS
*
*       1                       If message can be sent.
*       0                       Something is missing in the message.
*
*************************************************************************/
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
STATUS SMTP_Message_OK(SMTP_MSG * message)
{
    if (message->smtp_from_addr != NU_NULL &&
            message->smtp_part_list.smtp_front != NU_NULL &&
            message->smtp_recpt_list.smtp_front != NU_NULL &&
            message->smtp_subject != NU_NULL)
        return 1;
    else
        return 0;
} /* SMTP_Message_OK */
#endif

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Session_Connect
*
* DESCRIPTION
*
*       This function will start a session to SMTP server. If already
*       connected it will re-login the server.
*
* INPUTS
*
*       *session                Session object pointer.
*
* OUTPUTS
*
*       NU_SUCCESS              If connection was successful.
*       SMTP_TLS_ERROR          If Start-TLS is disabled on server.
*       SMTP_AUTH_ERROR         If authentication is enabled on server and
*                               user did not provide any login information.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Session_Connect(SMTP_SESSION *session)
{
    STATUS status = NU_SUCCESS;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* check if really need to connect to server. */
    if (session->smtp_com_state == SMTP_NO_CONNECTION)
    {
        /* Open connection to the server. */
        status = SMTP_Client_Init(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("REPLY: %s", session->smtp_resp);
#endif
    }

    /* Send EHLO. */
    if (status == NU_SUCCESS)
    {
        status = NU_SMTP_Ehlo(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("COMMAND: %s", session->smtp_cmd_sent);
        printf("REPLY: %s", session->smtp_resp);
#endif
    }

    /* If EHLO was not successful then we need to reconnect. */
    if ((status != NU_SUCCESS) &&
            (session->smtp_com_state != SMTP_NO_CONNECTION))
    {
        /* Close socket and deallocate SSL structure. */
        NU_SMTP_Close(session);

        /* Reconnect the connection must be lost. */
        /* Open connection to the server. */
        status = SMTP_Client_Init(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("REPLY: %s", session->smtp_resp);
#endif
        /* Send EHLO. */
        if (status == NU_SUCCESS)
        {
            status = NU_SMTP_Ehlo(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("COMMAND: %s", session->smtp_cmd_sent);
            printf("REPLY: %s", session->smtp_resp);
#endif
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Do STARTTLS if we are using it. */
        if (session->smtp_com_mode == SMTP_TLS)
        {
            /* Check if Start-TLS is enabled on server. */
            if (SMTP_TLS_Is_Enabled(session) == SMTP_TLS_ENABLED)
            {
                /* Check if we have to encrypt our connection. */
                if (session->smtp_rfc_2487_state == SMTP_TLS_NOT_DONE)
                {
                    /* Set extension settings if not done already. */
                    if (session->smtp_rfc_2487_setting != SMTP_TLS_ENABLED)
                    {
                        status = SMTP_Set_Extension_Settings(session,
                                SMTP_TLS_EXTEN,
                                SMTP_TLS_ENABLED);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                        printf("COMMAND: StartTLS Settings\r\n");
                        printf("REPLY: %s", session->smtp_resp);
#endif
                    }

                    /* Send STARTTLS and encrypt our connection. */
                    if (status == NU_SUCCESS)
                    {
                        status = NU_SMTP_StartTLS(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                        printf("COMMAND: %s", session->smtp_cmd_sent);
                        printf("REPLY: %s", session->smtp_resp);
#endif
                    }

                    /* EHLO response changes after we do STARTTLS. */
                    if (status == NU_SUCCESS)
                    {
                        status = NU_SMTP_Ehlo(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                        printf("COMMAND: %s", session->smtp_cmd_sent);
                        printf("REPLY: %s", session->smtp_resp);
#endif
                    }
                }
            }
            else
            {
                status = SMTP_TLS_ERROR;
                SMTP_Error_Resp(session, status);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("ERROR: %s", session->smtp_resp);
#endif
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        if (SMTP_AUTH_Is_Enabled(session) == SMTP_AUTH_ENABLED)
        {
            /* Login to server if user has set username and password. */
            if ((session->smtp_username != NU_NULL) &&
                    (session->smtp_password != NU_NULL))
            {
                /* Check if we have to login the server. */
                if (session->smtp_rfc_4954_state == SMTP_AUTH_NOT_DONE)
                {
                    /* Do extension settings. */
                    if ((session->smtp_rfc_4954_setting != SMTP_AUTH_PLAIN) &&
                            (session->smtp_rfc_4954_setting != SMTP_AUTH_LOGIN))
                    {
                        /* Try to check and enable AUTH LOGIN method. */
                        status = SMTP_Set_Extension_Settings(session,
                                SMTP_AUTH_EXTEN, SMTP_AUTH_LOGIN);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                        printf("COMMAND: AUTH Settings\r\n");
                        printf("REPLY: %s", session->smtp_resp);
#endif

                        if (status != NU_SUCCESS)
                        {
                            /* Try to check and enable AUTH PLAIN method. */
                            status = SMTP_Set_Extension_Settings(session,
                                    SMTP_AUTH_EXTEN, SMTP_AUTH_PLAIN);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                            printf("COMMAND: AUTH Settings\r\n");
                            printf("REPLY: %s", session->smtp_resp);
#endif
                        }
                    }

                    /* Send AUTH according to our settings. */
                    if (status == NU_SUCCESS)
                    {
                        status = NU_SMTP_Auth(session,
                                session->smtp_username,
                                session->smtp_password);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                        printf("COMMAND: %s", session->smtp_cmd_sent);
                        printf("REPLY: %s", session->smtp_resp);
#endif
                    }
                }
            }
            else
            {
                status = SMTP_AUTH_ERROR;
                SMTP_Error_Resp(session, status);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
                printf("ERROR: %s", session->smtp_resp);
#endif
            }
        }
    }

    if ((status != NU_SUCCESS) &&
            (session->smtp_com_state != SMTP_NO_CONNECTION))
    {
        /* Connect failed but connection is still open so close it. */
        NU_SMTP_Quit(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("COMMAND: %s", session->smtp_cmd_sent);
        printf("REPLY: %s", session->smtp_resp);
#endif
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Session_Connect */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Send_Message
*
* DESCRIPTION
*
*       This function will send a message using session connected
*       earlier. If connection was not established or dropped due
*       to timeout or connection error, it will connect to
*       server.
*
* INPUTS
*
*       *session                Session object pointer.
*       *message                Message object pointer.
*
* OUTPUTS
*
*       NU_SUCCESS              If message was successfully sent.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Send_Message(SMTP_SESSION *session, SMTP_MSG *message)
{
    STATUS status = NU_SUCCESS;
    SMTP_RECPT *rcpt;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((message == NU_NULL) || (session == NU_NULL) ||
            (SMTP_Message_OK(message) == 0))
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if (session->smtp_com_state == SMTP_NO_CONNECTION)
    {
        /* Connect to server if not connected. */
        status = NU_SMTP_Session_Connect(session);
    }

    if (status == NU_SUCCESS)
    {
        /* Send MAIL. */
        status = NU_SMTP_Mail(session, message->smtp_from_addr);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("COMMAND: %s", session->smtp_cmd_sent);
        printf("REPLY: %s", session->smtp_resp);
#endif
    }

    if ((status != NU_SUCCESS) &&
            (session->smtp_com_state != SMTP_NO_CONNECTION) &&
            (status != SMTP_COMMNAD_FAILED))
    {
        /* Connection is dropped. */
        /* Close socket and deallocate SSL structure. */
        NU_SMTP_Close(session);

        /* Reconnect to server. */
        status = NU_SMTP_Session_Connect(session);

        /* Send MAIL. */
        if (status == NU_SUCCESS)
        {
            status = NU_SMTP_Mail(session, message->smtp_from_addr);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("COMMAND: %s", session->smtp_cmd_sent);
            printf("REPLY: %s", session->smtp_resp);
#endif
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Send all the recipient addresses through RCPT. */
        rcpt = message->smtp_recpt_list.smtp_front;
        while ((status == NU_SUCCESS) && (rcpt != NU_NULL))
        {
            status = NU_SMTP_Rcpt(session, rcpt->smtp_addr);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("COMMAND: %s", session->smtp_cmd_sent);
            printf("REPLY: %s", session->smtp_resp);
#endif
            rcpt = rcpt->smtp_next;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Tell server that we will be sending message now. */
        status = NU_SMTP_Data(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("COMMAND: %s", session->smtp_cmd_sent);
        printf("REPLY: %s", session->smtp_resp);
#endif
    }

    if (status == NU_SUCCESS)
    {
        /* Send message according to our message object. */
        status = SMTP_Send_Message_Data(session, message);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
        printf("REPLY: %s", session->smtp_resp);
#endif
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Send_Message */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Session_Disconnect
*
* DESCRIPTION
*
*       This function will disconnect session.
*
* INPUTS
*
*       *session                Session object pointer.
*
* OUTPUTS
*
*       NU_SUCCESS              If connection was successfully closed.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Session_Disconnect(SMTP_SESSION *session)
{
    STATUS status = NU_SUCCESS;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if we are connected to server. */
    if (session->smtp_com_state != SMTP_NO_CONNECTION)
    {
        if (status == NU_SUCCESS)
        {
            /* Close connection to server. */
            status = NU_SMTP_Quit(session);
#if (SMTP_PRINT_RESPONSE == NU_TRUE)
            printf("COMMAND: %s", session->smtp_cmd_sent);
            printf("REPLY: %s", session->smtp_resp);
#endif
        }
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Session_Disconnect */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Session_Delete
*
* DESCRIPTION
*
*       This function will de-allocate session object form memory.
*
*
* INPUTS
*
*       *session                Pointer to session object pointer.
*
* OUTPUTS
*
*       NU_SUCCESS              If session was successfully deleted.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Session_Delete(SMTP_SESSION *session)
{
    STATUS status = -1;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (session == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Delete username and password if they were set. */
    if (session->smtp_username != NU_NULL)
    {
        status = NU_Deallocate_Memory(session->smtp_username);
    }

    /* Now we will delete our structure. */
    if (status == NU_SUCCESS)
    {
        status = NU_Deallocate_Memory(session);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Session_Delete */

/************************************************************************
*
* FUNCTION
*
*       NU_SMTP_Msg_Delete
*
* DESCRIPTION
*
*       This function will de-allocate message object form memory.
*
*
* INPUTS
*
*       *message                Pointer to message object pointer.
*
* OUTPUTS
*
*       NU_SUCCESS              If message was successfully deleted.
*       NU_INVALID_PARM         If user has provided wrong argument.
*
*************************************************************************/
STATUS NU_SMTP_Msg_Delete(SMTP_MSG *message)
{
    STATUS      status = -1;
    SMTP_RECPT  *recpt;
    SMTP_PART   *part;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if (message == NU_NULL)
        return NU_INVALID_PARM;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Delete sender's address if it was set. */
    if (message->smtp_from_addr != NU_NULL)
    {
        status = NU_Deallocate_Memory(message->smtp_from_addr);
    }

    /* Delete subject if it was set. */
    if (status == NU_SUCCESS)
    {
        if (message->smtp_subject != NU_NULL)
        {
            status = NU_Deallocate_Memory(message->smtp_subject);
        }
    }

    /* Delete part list object. */
    if (status == NU_SUCCESS)
    {
        if (message->smtp_part_list.smtp_front != NU_NULL)
        {
            part = (SMTP_PART *)SLL_Dequeue(
                    (VOID *)&(message->smtp_part_list));

            while ((status == NU_SUCCESS) && (part != NU_NULL))
            {
                status = NU_Deallocate_Memory(part);
                part = (SMTP_PART *)SLL_Dequeue(
                        (VOID *)&(message->smtp_part_list));
            }

        }
    }

    /* Delete recipient list object. */
    if (status == NU_SUCCESS)
    {
        if (message->smtp_recpt_list.smtp_front != NU_NULL)
        {
            recpt = (SMTP_RECPT *)SLL_Dequeue(
                    (VOID *)&(message->smtp_recpt_list));

            while ((status == NU_SUCCESS) && (recpt != NU_NULL))
            {
                status = NU_Deallocate_Memory(recpt);
                recpt = (SMTP_RECPT *)SLL_Dequeue(
                        (VOID *)&(message->smtp_recpt_list));
            }

        }
    }

    /* Now delete our object. */
    if (status == NU_SUCCESS)
    {
        status = NU_Deallocate_Memory(message);
    }

    /* Switch back to user mode */
    NU_USER_MODE();

    return status;
} /* NU_SMTP_Msg_Delete */

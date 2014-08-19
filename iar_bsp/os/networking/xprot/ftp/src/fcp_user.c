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
*       fcp_user.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of USER.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_USER
*
*   DEPENDENCIES
*
*       nucleus.h
*       externs.h
*       ncl.h
*       fc_defs.h
*       ftpc_defs.h
*       fcp_extr.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/externs.h"
#include "networking/ncl.h"
#include "networking/fc_defs.h"
#include "networking/ftpc_def.h"
#include "networking/fcp_extr.h"
#include "networking/fc_extr.h"

/******************************************************************************
*
*   FUNCTION
*
*       FCP_Client_USER
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP USER message to the
*       server. Parameters include a pointer to a valid FTP client structure
*       and a pointer to a character string containing the user name to send
*       to the server.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       username                pointer to character string containing user
*                               name
*
*   OUTPUTS
*
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_NEED_PASSWORD       The FTP Server has not received password infor-
*                               mation needed to process the requested action.
*       FTP_NEED_ACCOUNT        FTP Server has not received account information
*                               needed to process the requested action.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_INVALID_USER        The FTP Server did not recognize user account.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_USER(FTP_CLIENT *client, CHAR *username)
{
    INT     status,
            index,
            bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (username)
    {
        buffer[0] = 'U';
        buffer[1] = 'S';
        buffer[2] = 'E';
        buffer[3] = 'R';
        buffer[4] = ' ';

        index = 5;

        /* Accumulate username into buffer */
        while (*username)
        {
            buffer[index] = (UINT8)*username;
            username++;
            index++;
            /* Check for possible buffer overflow */
            if (index >= (FTPC_GENERIC_BUFF_SIZE-3))
                break;
        }

        buffer[index] = '\r';
        buffer[index+1] = '\n';
        buffer[index+2] = 0;

        /* Send FTP User Command */
        status = (INT)NU_Send(client->socketd, (CHAR *)buffer,(UINT16)((index+2)),0);
    }
    else
    {
        /* Anonymous FTP connection */
        status = (INT)NU_Send(client->socketd,"USER anonymous\r\n",16,0);
    }

    if (status < 0)
    {
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }
    else
    {
        /*  Go get the server's response.  */
        bytes_received = FCP_Reply_Read(client, buffer, FTPC_GENERIC_BUFF_SIZE,
                                        FTPC_INACT_TIMEOUT);

        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 230:
                client->last_error = NU_SUCCESS;
                break;

            case 331:
                client->last_error = FTP_NEED_PASSWORD;
                break;

            case 332:
                client->last_error = FTP_NEED_ACCOUNT;
                break;

            case 421:
                client->last_error = FTP_SERVICE_UNAVAILABLE;
                break;

            case 500:
                client->last_error = FTP_CMD_UNRECOGNIZED;
                break;

            case 501:
                client->last_error = FTP_BAD_CMD_FORMAT;
                break;

            case 530:
                client->last_error = FTP_INVALID_USER;
                break;

            default:
                client->last_error = FTP_BAD_RESPONSE;
                break;
            }
        }
        else
        {
            /* Error in call to NU_Recv(). */
            client->last_error = FTP_STACK_ERROR;
            client->stack_error = bytes_received;
            NU_Close_Socket(client->socketd);
        }
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_USER */

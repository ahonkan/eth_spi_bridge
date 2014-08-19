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
*       fcp_acct.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of ACCT.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_ACCT
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
*       FCP_Client_ACCT
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP ACCT message to the
*       server. Parameters include a pointer to a valid FTP client structure
*       and a pointer to a character string contain an account name to send to
*       the server.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       account                 pointer to character string containing account
*                               name
*
*   OUTPUTS
*
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_INVALID_ACCOUNT     The account pointer should not be null.
*       FTP_CMD_NOT_IMPLEMENTED The FTP Server replied that the requested
*                               command has not been implemented.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_BAD_CMD_SEQUENCE    The FTP Server returned a 'Bad command
*                               sequence' error.
*       FTP_INVALID_USER        FTP Server did not recognize user account.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_ACCT(FTP_CLIENT *client, CHAR *account)
{
    INT     status,
            index,
            bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (account)
    {
        buffer[0] = 'A';
        buffer[1] = 'C';
        buffer[2] = 'C';
        buffer[3] = 'T';
        buffer[4] = ' ';

        index = 5;

        /* Accumulate account details into buffer */
        while (*account)
        {
            buffer[index] = (UINT8)*account;
            account++;
            index++;
            /* Check for possible buffer overflow */
            if (index >= (FTPC_GENERIC_BUFF_SIZE-3))
                break;
        }

        buffer[index] = '\r';
        buffer[index+1] = '\n';
        buffer[index+2] = 0;

        status = (INT)NU_Send(client->socketd, (CHAR *)buffer,(UINT16)((index+2)),0);

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
                case 202:
                    client->last_error = FTP_CMD_NOT_IMPLEMENTED;
                    break;

                case 230:
                    client->last_error = NU_SUCCESS;
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

                case 503:
                    client->last_error = FTP_BAD_CMD_SEQUENCE;
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
    }
    else
    {
        client->last_error = FTP_INVALID_ACCOUNT;
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_ACCT */

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
*       fcp_retr.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of RETR.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_RETR
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
*       FCP_Client_RETR
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP RETR message to the
*       server. Parameters include a pointer to a valid FTP client structure,
*       and a pointer to a character string containing the path and/or name of
*       the file to be retrieved.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       filespec                pointer to string containing name of file to
*                               be retrieved
*
*   OUTPUTS
*
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_FILE_UNAVAILABLE    The file on the FTP Server was either not found
*                               or the user does not have access to the same.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_INVALID_USER        The FTP Server did not recognize user account.
*       FTP_FILE_NOT_FOUND      The FTP Server could not find requested file.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_RETR(FTP_CLIENT *client, CHAR *filespec)
{
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];
    INT     status,
            index,
            bytes_received;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (filespec == NU_NULL) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* build message */
    buffer[0] = 'R';
    buffer[1] = 'E';
    buffer[2] = 'T';
    buffer[3] = 'R';
    buffer[4] = ' ';

    index = 5;

    /* Accumulate file details into buffer */
    while (*filespec)
    {
        buffer[index] = (UINT8)*filespec;
        filespec++;
        index++;
        /* Check for possible buffer overflow */
        if (index >= (FTPC_GENERIC_BUFF_SIZE-3))
            break;
    }

    buffer[index] = '\r';
    buffer[index+1] = '\n';
    buffer[index+2] = 0;

    /* send message */
    status = (INT)NU_Send(client->socketd, (CHAR *)buffer,(UINT16)((index+2)),0);

    if (status < 0)
    {
        /* Error encountered while sending command to host. */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }

    else
    {
        /* get port response */
        /*  Go get the server's response.  */
        bytes_received = FCP_Reply_Read(client, buffer, FTPC_GENERIC_BUFF_SIZE,
                                        FTPC_INACT_TIMEOUT);

        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 125:
                client->last_error = NU_SUCCESS;
                break;

            case 150:
                client->last_error = NU_SUCCESS;
                break;

            case 421:
                client->last_error = FTP_SERVICE_UNAVAILABLE;
                break;

            case 450:
                client->last_error = FTP_FILE_UNAVAILABLE;
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

            case 550:
                client->last_error = FTP_FILE_NOT_FOUND;
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

} /* FCP_Client_RETR */

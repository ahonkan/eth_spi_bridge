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
*       fcp_help.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of HELP.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_HELP
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
*       FCP_Client_HELP
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP HELP message to the
*       server. Parameters include a pointer to a valid FTP client structure,
*       a pointer to a character string containing the requested help topic,
*       a pointer to a character buffer to store the help text, and the size
*       of the buffer.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       topic                   pointer to character string containing help
*                               topic
*       buffer                  storage location for help information
*       buffsize                size of buffer
*
*   OUTPUTS
*
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_CMD_NOT_IMPLEMENTED The FTP Server replied that the requested
*                               command has not been implemented.
*       FTP_FILE_UNAVAILABLE    The file on FTP Server was either not found
*                               or the user does not have access to the same.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_HELP(FTP_CLIENT *client, CHAR *topic, CHAR *buffer, INT buffsize)
{
    INT     status,
            index,
            bytes_received;
    UINT8   buff[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (buffer == NU_NULL) || (buffsize <= 0) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Build command buffer */
    buff[0] = 'H';
    buff[1] = 'E';
    buff[2] = 'L';
    buff[3] = 'P';
    index = 4;

    if ( (topic != NU_NULL) && (*topic) )
    {
        buff[4] = ' ';
        index = 5;

        /* Accumulate help topic details into buffer */
        while (*topic)
        {
            buff[index] = (UINT8)*topic;
            topic++;
            index++;
            /* Check for possible buffer overflow */
            if (index >= (FTPC_GENERIC_BUFF_SIZE-3))
                break;
        }
    }

    buff[index] = '\r';
    buff[index+1] = '\n';
    buff[index+2] = 0;

    /* Send FTP Command */
    status = (INT)NU_Send(client->socketd, (CHAR *)buff,(UINT16)((index+2)),0);

    if (status < 0)
    {
        /* Error encountered while sending command to host. */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }
    else
    {
        /*  Go get the server's response.  */
        bytes_received = FCP_Reply_Read(client, (UINT8 *)buffer, buffsize,
                                        FTPC_INACT_TIMEOUT);

        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 211:
                client->last_error = NU_SUCCESS;
                break;

            case 214:
                client->last_error = NU_SUCCESS;
                break;

            case 421:
                client->last_error = FTP_SERVICE_UNAVAILABLE;
                break;

            case 500:
            case 501:
            case 502:
                FCP_Client_XPWD(client, buffer, buffsize);
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

} /* FCP_Client_HELP */
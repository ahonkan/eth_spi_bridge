/*************************************************************************
*
*             Copyright 1998-2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE                                            
*
*       fcp_rest.c                                 
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of REST.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_REST
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
*       FCP_Client_REST
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP REST message to the
*       server. Parameters include a pointer to a valid FTP client structure,
*       and an integer specifying the restart point.
*
*   INPUTS
*
*       client                  Pointer to valid FTP client structure
*       restartpt               Integer specifying the restart point
*
*   OUTPUTS
*
*   NU_SUCCESS                  The operation was successful.
*   FTP_STACK_ERROR             A send or receive command returned an error
*                               while communicating with the FTP Server.
*   FTP_SERVICE_UNAVAILABLE     The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*   FTP_FILE_UNAVAILABLE        FTP Server returned 'File Unavailable' message.
*   FTP_MEMORY                  A memory allocation error occurred.
*   FTP_CMD_UNRECOGNIZED        The FTP Server did not recognize the command.
*   FTP_BAD_CMD_FORMAT          FTP Server did not recognize command format.
*   FTP_CMD_NOT_IMPLEMENTED     The FTP Server replied that the requested
*                               command has not been implemented.
*   FTP_INVALID_USER            The FTP Server did not recognize user account.
*   FTP_NEED_ACCOUNT            FTP Server has not received account information
*                               that is needed to process the requested action.
*   FTP_INVALID_FILE_NAME       FTP Server returned 'Invalid filename' message.
*   FTP_BAD_RESPONSE            The FTP Server returned an unknown error code.
*   FTP_INVALID_PARM            A required parameter is null.
*
******************************************************************************/
INT FCP_Client_REST(FTP_CLIENT *client, INT32 restartpt)
{
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];
    INT     status,
            index,
            bytes_received;

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* build list message */
    buffer[0] = 'R';
    buffer[1] = 'E';
    buffer[2] = 'S';
    buffer[3] = 'T';
    buffer[4] = ' ';

    index = 5;

    /* Accumulate restart point info into buffer */
    NU_ITOA((INT)(restartpt), (CHAR*)&buffer[index], 10);

    while (buffer[index])
        index++;

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
        /*  Go get the server's response.  */
        bytes_received = FCP_Reply_Read(client, buffer, FTPC_GENERIC_BUFF_SIZE,
                                        FTPC_INACT_TIMEOUT);

        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 350:
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

            case 502:
                client->last_error = FTP_CMD_NOT_IMPLEMENTED;
                break;

            case 530:
                client->last_error = FTP_INVALID_USER;
                break;

            case 532:
                client->last_error = FTP_NEED_ACCOUNT;
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

} /* FCP_Client_REST */

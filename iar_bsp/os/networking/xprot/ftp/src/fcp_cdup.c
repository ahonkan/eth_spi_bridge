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
*       fcp_cdup.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of CDUP.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_CDUP
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
*       FCP_Client_CDUP
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP CDUP (change
*       directory to parent) message to the server. The sole parameter is a
*       pointer to a valid FTP client structure.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*
*   OUTPUTS
*
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service UnUnavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_CMD_NOT_IMPLEMENTED The FTP Server replied that the requested
*                               command has not been implemented.
*       FTP_INVALID_USER        FTP Server did not recognize the user account.
*       FTP_FILE_NOT_FOUND      The FTP Server could not find specified file.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_CDUP(FTP_CLIENT *client)
{
    INT     status,
            bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Send FTP Command */
    status = (INT)NU_Send(client->socketd,"CDUP\r\n",6,0);

    if (status < 0)
    {
        /* Error encountered while sending command to host. */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }
    else
    {
        /* Go get the server's response.  */
        bytes_received = FCP_Reply_Read(client, buffer, FTPC_GENERIC_BUFF_SIZE,
                                        FTPC_INACT_TIMEOUT);

        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 250:
            case 200:
                client->last_error = NU_SUCCESS;
                break;

            case 421:
                client->last_error = FTP_SERVICE_UNAVAILABLE;
                break;

            case 500:
                FCP_Client_XCUP(client);
                break;

            case 501:
                FCP_Client_XCUP(client);
                break;

            case 502:
                FCP_Client_XCUP(client);
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

} /* FCP_Client_CDUP */

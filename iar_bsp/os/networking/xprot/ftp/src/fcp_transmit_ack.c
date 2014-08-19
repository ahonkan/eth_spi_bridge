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
*       fcp_transmit_ack.c                           1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client
*
*   DESCRIPTION
*
*       This file contains support for Client Transmit Acknowledge.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_Tran_Ack
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
*       FCP_Client_Tran_Ack
*
*   DESCRIPTION
*
*       This function is one of the three general purpose functions included
*       with the primitives. It does the followup processing of a data
*       transfer connection, including fielding the end of transmit (or
*       receive) FTP codes.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure.
*       ignore_flag             flag to determine whether or not to use the
*                               incoming message
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
*       FTP_TRANSFER_ABORT      The server returned a 'Transfer Abort' message.
*       FTP_SYNTAX_ERROR        The server returned a 'Syntax Error' message.
*       FTP_FILE_NOT_FOUND      The FTP Server did not find the requested file.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_Tran_Ack(FTP_CLIENT *client, INT ignore_flag)
{
    INT     bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    bytes_received = FCP_Reply_Read(client, buffer, FTPC_GENERIC_BUFF_SIZE,
                                    FTPC_INACT_TIMEOUT);

    /* We want to read the response off the command socket, but we do not
       want to update the client structure, regardless of the response from
       the call to FTP_Reply_Read(). */
    if (!ignore_flag)
    {
        if (bytes_received >= 0)
        {
            switch (NU_ATOI((CHAR*)buffer))
            {
            case 226:
                client->last_error = NU_SUCCESS;
                break;

            case 250:
                client->last_error = NU_SUCCESS;
                break;

            case 421:
                client->last_error = FTP_SERVICE_UNAVAILABLE;
                break;

            case 426:
                client->last_error = FTP_TRANSFER_ABORT;
                break;

            case 450:
                client->last_error = FTP_FILE_UNAVAILABLE;
                break;

            case 451:
                client->last_error = FTP_TRANSFER_ABORT;
                break;

            case 501:
                client->last_error = FTP_SYNTAX_ERROR;
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

} /* FCP_Client_Tran_Ack */

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
*       fcp_epsv.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of EPSV.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_EPSV
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
*       FCP_Client_EPSV
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP EPSV (e-passive
*       connect) message to the server. Parameters include a pointer to a
*       valid FTP client structure and a pointer to an addr_struct which will
*       be filled with the data returned by the server.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       servaddr                pointer to structure containing address to
*                               retrieve from the server
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
*       FTP_INVALID_USER        The FTP Server did not recognize user account.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_EPSV(FTP_CLIENT *client, struct addr_struct *servaddr)
{
    INT     status,
            index,
            bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];
    CHAR    *family, *ip_addr, *port;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (servaddr == NU_NULL) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Send FTP Command */
    status = (INT)NU_Send(client->socketd,"EPSV\r\n", 6, 0);

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
            case 229:

                index = 0;
                /* Get to the open parenthesis for the EPSV command */
                while (buffer[index] != '(')
                    index ++;

                /* Parse out the parameters of the response */
                client->last_error = FC_Parse_Extended_Command(
                                        (CHAR*)&buffer[index + 1],
                                        &family, &ip_addr, &port,
                                        (CHAR)(buffer[index + 1]),
                                        (INT)strlen((CHAR*)&buffer[index + 1]));

                if (client->last_error != NU_SUCCESS)
                    client->last_error = FTP_BAD_RESPONSE;

                else
                {
                    /* Extract the server's data port number from the string. */
                    servaddr->port = (UINT16)NU_ATOI(port);
                    client->last_error = NU_SUCCESS;
                }

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

} /* FCP_Client_EPSV */

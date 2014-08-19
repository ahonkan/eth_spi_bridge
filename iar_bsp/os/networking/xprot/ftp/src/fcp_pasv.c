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
*       fcp_pasv.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of PASV.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_PASV
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
*       FCP_Client_PASV
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP PASV (passive
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
*       FTP_CMD_NOT_IMPLEMENTED The FTP Server replied that requested command
*                               has not been implemented.
*       FTP_INVALID_USER        The FTP Server did not recognize user account.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_PASV(FTP_CLIENT *client, struct addr_struct *servaddr)
{
    INT     status,
            index,
            out,
            buff_index,
            bytes_received;
    UINT8   temp_int;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (servaddr == NU_NULL) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Send FTP Command */
    status = (INT)NU_Send(client->socketd,"PASV\r\n",6,0);

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
            case 227:
			    /* The message received is of the form:
				 *       227 Entering Passive Mode (192,168,0,10,15,149)
				 */

                index = 3;
                out = 0;
				
                /* Scan return buffer for IP address */
                while (!out)
                {
					/* Skip until a numeric character indicating the begining of an IP address is found */
                    if ( ((buffer[index] >= '0') && (buffer[index] <= '9')) ||
                         (buffer[index] == 0) )
                        out++;
                    else
                        index++;
                    if (index >= FTPC_GENERIC_BUFF_SIZE)
                    {
                        index = FTPC_GENERIC_BUFF_SIZE - 1;
                        break;
                    }
                }

                if (buffer[index] == 0)
                    client->last_error = FTP_BAD_RESPONSE;
                else
                {
                    /* Extract IP address and port number from buffer. */
                    out = 0;
                    buff_index = 0;

                    while (!out)
                    {
                        temp_int = 0;

                        for (;;)
                        {
							/* Build each octet of the IP address */
                            if ( (buffer[index] >= '0') &&
                                 (buffer[index] <= '9') )
                                temp_int = (UINT8)((temp_int) * 10
                                                   + (buffer[index] - 48));
                            else
                                break;

                            index++;
                        }

                        if (buff_index < 4)
						    /* Store each decoded octet into the respective IP address byte */
                            servaddr->id.is_ip_addrs[buff_index] = temp_int;
                        else if (buff_index == 4) 
						    /* First byte of the port */
                            servaddr->port = (UINT16)temp_int;
                        else
                        {
							/* (actual port) = ((fifth octet) * (256)) + (sixth octet) */
                            servaddr->port <<= 8;
                            servaddr->port = (UINT16)(temp_int +
                                                      servaddr->port);
                        }

                        if (buff_index == 5)
                            out++;
                        else
						{
							/* The currect character in the buffer is a ',' separator, so skip it */
                            index++;
							buff_index++;
						}
                    }

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

} /* FCP_Client_PASV */

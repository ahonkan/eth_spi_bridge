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
*       fcp_size.c
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of SIZE.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_SIZE
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

STATIC INT32 FCP_Read_Size(UINT8 *buffer);

/******************************************************************************
*
*   FUNCTION
*
*       FCP_Client_SIZE
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP SIZE message to the
*       server. Parameters include a pointer to a valid FTP client structure
*       and a pointer to a character string containing the name of the file
*       for which the size is to be determined.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       path                    pointer to character string containing the file
*                               name of which the size needs to be determined
*       size                    pointer to the size of the file. On a
*                               successful return this contains the size of
*                               the requested file.
*
*   OUTPUTS
*
*       NU_SUCCESS              On a successful return, SIZE of the file is
*                               returned.
*       FTP_STACK_ERROR         A send or receive command returned an error
*                               while communicating with the FTP Server.
*       FTP_SERVICE_UNAVAILABLE The FTP Server returned a 'Service Unavailable'
*                               message. This may be a reply to any command if
*                               the ftp service knows it must shut down.
*       FTP_FILE_UNAVAILABLE    The file on the FTP Server was either not found
*                               or the user does not have access to the same.
*       FTP_CMD_UNRECOGNIZED    The FTP Server did not recognize the command.
*       FTP_BAD_CMD_FORMAT      FTP Server did not recognize command format.
*       FTP_CMD_NOT_IMPLEMENTED The FTP Server replied that the requested
*                               command has not been implemented.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_SIZE(FTP_CLIENT *client, CHAR *path, INT32 *size)
{
    INT     status,
            index,
            bytes_received;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    if ( (client == NU_NULL) || (path == NU_NULL) ||
    	 (size == NU_NULL) )
        return (FTP_INVALID_PARM);

    /* Build command buffer */
    buffer[0] = 'S';
    buffer[1] = 'I';
    buffer[2] = 'Z';
    buffer[3] = 'E';
    buffer[4] = ' ';

    index = 5;

    /* Accumulate path details into buffer */
    while (*path)
    {
        buffer[index] = (UINT8)*path;
        path++;
        index++;
        /* Check for possible buffer overflow */
        if (index >= (FTPC_GENERIC_BUFF_SIZE-3))
            break;
    }

    buffer[index] = '\r';
    buffer[index+1] = '\n';
    buffer[index+2] = 0;

    /* Send FTP Command */
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
            case 213:
                *size = FCP_Read_Size(buffer);

                if (*size >= 0)
                	client->last_error = NU_SUCCESS;
                else
                	client->last_error = FTP_BAD_RESPONSE;
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

            case 502:
                client->last_error = FTP_CMD_NOT_IMPLEMENTED;
                break;

            case 550:
                client->last_error = FTP_FILE_UNAVAILABLE;
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

    return (client->last_error);

} /* FCP_Client_SIZE */

/******************************************************************************
*
*   FUNCTION
*
*       FCP_Read_Size
*
*   DESCRIPTION
*
*       This is a utility function to read the size of the file received and
*       is contained in the replyBuffer.
*
*   INPUTS
*
*       buffer                  pointer to the buffer
*
*   OUTPUTS
*
*       Size of the file contained in the buffer.
*
******************************************************************************/
STATIC INT32 FCP_Read_Size(UINT8 *buffer)
{
    INT     index = 0;
    CHAR    sizeBuff[10];

    /* Scan for a '\r' after FTP status number in buffer,
     * copy buffer to sizeBuff
     */
    while ((index < (FTPC_GENERIC_BUFF_SIZE - 4)) &&
           (buffer[index + 4]!= '\r'))
    {
        sizeBuff[index] = buffer[index+4];
        index++;
    }

    /* Terminate the sizeBuff string */
    sizeBuff[index] = '\0';

    /* Return the ASCII size converted to integer */
    return (NU_ATOI(sizeBuff));
} /* FCP_Read_Size */

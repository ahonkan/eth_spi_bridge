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
*       fcp_mkd.c                                    1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of MKD.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_MKD
*
*   DEPENDENCIES
*
*       nucleus.h
*       externs.h
*       fc_defs.h
*       ftpc_defs.h
*       fcp_extr.h
*       fc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/externs.h"
#include "networking/fc_defs.h"
#include "networking/ftpc_def.h"
#include "networking/fcp_extr.h"
#include "networking/fc_extr.h"

/******************************************************************************
*
*   FUNCTION
*
*       FCP_Client_MKD
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP MKD (make directory)
*       message to the server. Parameters include a pointer to a valid FTP
*       client structure and a pointer to a character string containing the
*       new directory name.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       path                    pointer to character string containing new
*                               directory
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
*       FTP_FILE_UNAVAILABLE    The file on FTP Server was either not found
*                               or the user does not have access to the same.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_MKD(FTP_CLIENT *client, CHAR *path)
{
    INT     status, index;
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (path == NU_NULL) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Build MkDir command buffer */
    buffer[0] = 'M';
    buffer[1] = 'K';
    buffer[2] = 'D';
    buffer[3] = ' ';

    index = 4;

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

    /* Send FTP MkDir Command */
    status = (INT)NU_Send(client->socketd, (CHAR *)buffer,(UINT16)((index+2)),0);

    if (status < 0)
    {
        /* Error encountered while sending command to host. */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }
    else
        FCP_Client_MKD_Response(client, buffer, FTPC_GENERIC_BUFF_SIZE);

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_MKD */

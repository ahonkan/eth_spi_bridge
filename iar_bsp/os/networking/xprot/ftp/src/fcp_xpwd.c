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
*       fcp_xpwd.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of XPWD.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_XPWD
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
*       FCP_Client_XPWD
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP XPWD (print working
*       directory, experimental) message to the server. Parameters include a
*       pointer to a valid FTP client structure, a pointer to a character
*       buffer to store the directory name and the size of the buffer. This
*       function is called by FTP_Client_CWD() if a command not recognized
*       error is returned. Some servers still use the older experimental
*       versions of this command.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       buffer                  pointer to storage location for dir name
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
*       FTP_CMD_NOT_IMPLEMENTED The FTP Server replied that requested command
*                               has not been implemented.
*       FTP_FILE_UNAVAILABLE    The file on the FTP Server was either not found
*                               or the user does not have access to the same.
*       FTP_BAD_RESPONSE        The FTP Server returned an unknown error code.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_XPWD(FTP_CLIENT *client, CHAR *buffer, INT buffsize)
{
    INT status;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (buffer == NU_NULL) || (buffsize <= 0) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Send FTP Command */
    status = (INT)NU_Send(client->socketd,"XPWD\r\n",6,0);

    if (status < 0)
    {
        /* Error encountered while sending command to host. */
        client->last_error = FTP_STACK_ERROR;
        client->stack_error = status;
    }
    else
        FCP_Client_PWD_Response(client, buffer, buffsize);

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_XPWD */

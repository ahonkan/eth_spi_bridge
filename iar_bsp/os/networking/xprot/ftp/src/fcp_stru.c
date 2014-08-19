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
*       fcp_stru.c                                   1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client commands
*
*   DESCRIPTION
*
*       This file contains the FTP Client implementation of STRU.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_STRU
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
*       FCP_Client_STRU
*
*   DESCRIPTION
*
*       This function provides a primitive to send an FTP STRU message to the
*       server. Parameters include a pointer to a valid FTP client structure,
*       and an integer storing a code representing a valid transfer structure
*       type.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       structure               code for transfer structure type
*
*   OUTPUTS
*
*       FTP_INVALID_STRU_CODE   The structure parameter is invalid.
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
INT FCP_Client_STRU(FTP_CLIENT *client, INT structure)
{
    UINT8   buffer[FTPC_GENERIC_BUFF_SIZE];

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    client->last_error = NU_SUCCESS;

    /* Send FTP Command */
    if (structure == FTPC_STRU_FILE)
    {
        client->stack_error = (INT)NU_Send(client->socketd,"STRU F\r\n",8,0);
        if ( client->stack_error >= 0 )
            FCP_Client_TYPE_Response(client, buffer, FTPC_GENERIC_BUFF_SIZE);
        else
            client->last_error = FTP_STACK_ERROR;
    }
    else
        client->last_error = FTP_INVALID_STRU_CODE;

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_STRU */

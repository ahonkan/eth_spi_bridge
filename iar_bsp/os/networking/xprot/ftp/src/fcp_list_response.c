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
*       fcp_list_response.c                          1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client
*
*   DESCRIPTION
*
*       This file contains support for Client LIST Response.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_LIST_Response
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
*       FCP_Client_LIST_Response
*
*   DESCRIPTION
*
*       This function reads the reply from the server for the LIST and NLST
*       commands and updates the client structure with the error code.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       buffer                  pointer to the input buffer
*       buffsize                size of the input buffer
*
*   OUTPUTS
*
*       NONE
*
******************************************************************************/
VOID FCP_Client_LIST_Response(FTP_CLIENT *client, UINT8 *buffer,
                              INT buffsize)
{
    INT bytes_received;

    /*  Go get the server's response.  */
    bytes_received = FCP_Reply_Read(client, buffer, buffsize,
                                    FTPC_INACT_TIMEOUT);

    if (bytes_received >= 0)
    {
        switch (NU_ATOI((CHAR*)buffer))
        {
        case 125:
            client->last_error = NU_SUCCESS;
            break;

        case 150:
            client->last_error = NU_SUCCESS;
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

        case 530:
            client->last_error = FTP_INVALID_USER;
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

} /* FCP_Client_LIST_Response */

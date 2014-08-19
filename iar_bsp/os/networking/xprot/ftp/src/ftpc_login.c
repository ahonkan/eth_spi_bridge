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
*   FILE NAME                                              
*
*       ftpc_login.c                                   
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client-Level API
*       Functions
*
*   DESCRIPTION
*
*       This file contains the client-level API functions.  These
*       functions provide a basic FTP client implementation, also serving
*       as an example for building custom clients.  This function set is
*       built on a single-task model, in that all connections, control and
*       data, are managed by a single task.  This single task usage is
*       strictly enforced by the API.  The API also expects a file system
*       for retrieval and storage of files.  Possible alternatives would be
*       to use memory space to store retrieved files, or to send memory
*       images to store at the server.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FTPC_Client_Login
*
*   DEPENDENCIES
*
*       nucleus.h
*       target.h
*       externs.h
*       fc_defs.h
*       ftpc_defs.h
*       ftpc_extr.h
*       fcp_extr.h
*       pcdisk.h
*       ftp_zc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/target.h"
#include "networking/externs.h"
#include "storage/pcdisk.h"
#include "networking/fc_defs.h"
#include "networking/ftpc_def.h"
#include "networking/ftpc_ext.h"
#include "networking/fcp_extr.h"

#ifdef NET_5_1
#include "networking/ftp_zc_extr.h"
#endif

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Login
*
*   DESCRIPTION
*
*       This function performs an FTP login sequence, including user name and
*       password, but not account.  If account is necessary, an appropriate
*       error code is returned.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       username                pointer to character string containing
*                               user's name
*       password                pointer to character string containing
*                               user's password
*
*   OUTPUTS
*
*       The last error code generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Login(FTP_CLIENT *client, CHAR *username, CHAR *password)
{
    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
    {
        NU_USER_MODE();
        return (FTP_INVALID_PARM);
    }

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (FCP_Client_Verify_Caller(client) == NU_SUCCESS)
    {
        if (FCP_Client_USER(client, username) == FTP_NEED_PASSWORD)
        {
            FCP_Client_PASS(client, password);
        }
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Login */

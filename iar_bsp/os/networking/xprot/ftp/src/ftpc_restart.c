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
*   FILE NAME                                        
*
*       ftpc_restart.c                                 
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
*       FTPC_Client_Restart
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
*       ftp_zc_extr.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/target.h"
#include "networking/externs.h"
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
*       FTPC_Client_Restart
*
*   DESCRIPTION
*
*       This function sets restart marker for the subsequent file transfer.
*       The function primitive FCP_Client_REST() must be called after this
*       function in order to send the restart marker to the other side.
*       The sole purpose of this function is to set the restart marker which
*       should be sent by calling FCP_Client_REST() subsequently.
*
*   INPUTS
*
*       client                  Pointer to valid FTP client structure.
*       restartpt               Integer specifying the restart point.
*
*   OUTPUTS
*
*       The last error generated.
*       FTP_INVALID_PARM        A required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Restart(FTP_CLIENT *client, INT32 restartpt)
{
    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (FCP_Client_Verify_Caller(client) == NU_SUCCESS)
    {
        client->restart = restartpt;
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Restart */

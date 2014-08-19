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
*       ftpc_rename.c                                  
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
*       FTPC_Client_Rename_File
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
*       FTPC_Client_Rename_File
*
*   DESCRIPTION
*
*       This function renames a file on the remote host.  The name of the
*       file is contained in old_file_name, while the new name is contained
*       in new_file_name.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       old_file_name           string containing old name of file to be
*                               renamed
*       new_file_name           string containing new name of file to rename
*
*   OUTPUTS
*
*       The last error code generated.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Rename_File(FTP_CLIENT *client, CHAR *old_file_name,
                            CHAR *new_file_name)
{
    INT status;
    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (old_file_name == NU_NULL) ||
         (new_file_name == NU_NULL) )
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (FCP_Client_Verify_Caller(client) == NU_SUCCESS)
    {
        status = FCP_Client_RNFR(client, old_file_name);

        if (status == NU_SUCCESS)
        {
            FCP_Client_RNTO(client, new_file_name);
        }
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FTPC_Client_Rename_File */

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
*       ftpc_size.c
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
*       FTPC_Client_Size
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
*       FTPC_Client_Size
*
*   DESCRIPTION
*
*       This function returns the size of a file on the remote host. This
*       function is a client-level wrapper for the FCP_Client_SIZE() function.
*       Its sole purpose is to add a caller verification wrapper to the
*       primitive.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*       path                    pointer to character string defining object
*                               to be checked
*       size                    pointer to the size of the file. On a
*                               successful return this contains the size of
*                               the requested file.
*
*   OUTPUTS
*
*       NU_SUCCESS              On a successful return, SIZE of the file is
*                               returned.
*       FTP_INVALID_PARM        An required input parameter is NULL.
*       Otherwise               The last error code generated is returned.
*
******************************************************************************/
INT FTPC_Client_Size(FTP_CLIENT *client, CHAR *path, INT32 *size)
{
	STATUS   status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    if ( (client == NU_NULL) || (path == NU_NULL) ||
    	 (size == NU_NULL) )
    	status = FTP_INVALID_PARM;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (status >= NU_SUCCESS)
    {
    	status = FCP_Client_Verify_Caller(client);

    	if (status >= NU_SUCCESS)
    	{
    		/* Invoke the SIZE command primitive */
    		status = FCP_Client_SIZE(client, path, size);
    	}
    }

    NU_USER_MODE();    /* return to user mode */

    return (status);

} /* FTPC_Client_Size */

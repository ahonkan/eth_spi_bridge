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
*       ftpc_actv_pasv.c
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
*       FTPC_Client_Set_PASV
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

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Set_PASV
*
*   DESCRIPTION
*
*       This API sets a FTP PASV flag for the forthcoming data connections.
*       Parameters include a pointer to a valid FTP client structure.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*
*   OUTPUTS
*
*       NU_SUCCESS              If the Flag was successfully set.
*       FTP_INVALID_PARM        A required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Set_PASV(FTP_CLIENT *client)
{
    STATUS     status;

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Verify that all the client parameters are valid */
    status = FCP_Client_Verify_Caller(client);

    if (status == NU_SUCCESS)
    {
        /* Set the Passive mode ON */
        client->mode = FTP_PASSIVE_MODE_ON;
    }

    NU_USER_MODE();    /* return to user mode */

    return (status);
} /* FTPC_Client_Set_PASV */

/******************************************************************************
*
*   FUNCTION
*
*       FTPC_Client_Set_ACTV
*
*   DESCRIPTION
*
*       This API sets a FTP ACTV flag for the forthcoming data connections.
*       Parameters include a pointer to a valid FTP client structure.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*
*   OUTPUTS
*
*       NU_SUCCESS              If the Flag was successfully set.
*       FTP_INVALID_PARM        A required input parameter is NULL.
*
******************************************************************************/
INT FTPC_Client_Set_ACTV(FTP_CLIENT *client)
{
    STATUS     status;

    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Verify that all the client parameters are valid */
    status = FCP_Client_Verify_Caller(client);

    if (status == NU_SUCCESS)
    {
        /* Set the Passive mode ON */
        client->mode = FTP_PASSIVE_MODE_OFF;
    }

    NU_USER_MODE();    /* return to user mode */

    return (status);
} /* FTPC_Client_Set_ACTV */

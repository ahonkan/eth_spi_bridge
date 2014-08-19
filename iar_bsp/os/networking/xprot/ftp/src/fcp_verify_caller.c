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
*       fcp_verify_caller.c                          1.7
*
*   COMPONENT
*
*       Nucleus Extended Protocol Package - Nucleus FTP Client
*
*   DESCRIPTION
*
*       This file contains support for Client Caller Verification.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       FCP_Client_Verify_Caller
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
*       FCP_Client_Verify_Caller
*
*   DESCRIPTION
*
*      This function is one of the three general purpose functions included
*      with the primitives. It verifies the client parameter and also
*      determines if the currently active Nucleus PLUS task is the 'owner'
*      of the FTP connection associated with client.
*
*   INPUTS
*
*       client                  pointer to valid FTP client structure
*
*   OUTPUTS
*
*       FTP_INVALID_CLIENT      The client did not receive a '220 Service
*                               Ready' or equivalent message from FTP Server.
*       FTP_INVALID_TASK        The task ID of the calling task does not match
*                               the task ID in the FTP_CLIENT structure.
*       FTP_INVALID_PARM        A required parameter is null.
*
******************************************************************************/
INT FCP_Client_Verify_Caller(FTP_CLIENT *client)
{
    NU_SUPERV_USER_VARIABLES

    if (client == NU_NULL)
        return (FTP_INVALID_PARM);

    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* First, check that we have a valid FTP_CLIENT structure by verifying
       the contents of the valid_pattern field. */
    if (client->valid_pattern == FTP_VALID_PATTERN)
    {
        /* Now check and see if the calling task "owns" this client instance */
        if (client->task_id == (UNSIGNED)(NU_Current_Task_Pointer()))
        {
            client->last_error = NU_SUCCESS;
        }
        else
        {
            client->last_error = FTP_INVALID_TASK;
        }
    }
    else
    {
        client->last_error = FTP_INVALID_CLIENT;
    }

    NU_USER_MODE();    /* return to user mode */

    return (client->last_error);

} /* FCP_Client_Verify_Caller */

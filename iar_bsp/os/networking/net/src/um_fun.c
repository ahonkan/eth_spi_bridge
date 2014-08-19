/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       um_fun.c
*
* DESCRIPTION
*
*       Retrieves next user record from UM database.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UM_Find_User_Next
*
* DEPENDENCIES
*
*       nu_net.h
*       um_defs.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/um_defs.h"

/*************************************************************************
*
*   FUNCTION
*
*       UM_Find_User_Next
*
*   DESCRIPTION
*
*       This function retrieves the next user record from the UM database.
*
*   INPUTS
*
*       name                    current user name (ASCII string between
*                               UM_MIN_NAME_SIZE and UM_MAX_NAME_SIZE)
*       dp                      pointer to pointer to UM_USER_TYPE
*                               struct, will contain the next user
*                               information
*       service                 User's service type.
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       UM_USER_UNKNOWN         user name is not in database
*
*************************************************************************/
STATUS UM_Find_User_Next(const CHAR *name, UM_USER *dp, UINT32 service)
{
    STATUS          status;
    UM_USER_ELEMENT *tp;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_Protect (&UM_Protect);

    tp = UM_Scan_User (name);    /* find current user record */

    /* Walk the list until a user is found for the requested service. */
    while((tp) && (tp->flink) && !(tp->flink->um_pv & service))
        tp = tp->flink;

    /* If tp->flink is not null, then we found a match. */
    if ((tp) && (tp->flink))
    {
        /* next record exists */
        tp = tp->flink;

        strcpy (dp->um_name, tp->um_name);
        strcpy (dp->um_pw, tp->um_pw);
        dp->um_pv = tp->um_pv;

        status = NU_SUCCESS;
    }
    else
    {
        status = UM_USER_UNKNOWN;   /* current user is not in the UM database */
    }

    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Find_User_Next */

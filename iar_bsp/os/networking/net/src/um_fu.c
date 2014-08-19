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
*       um_fu.c
*
* DESCRIPTION
*
*       Retrieves the User Management (UM) database record for
*       the specified user.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UM_Find_User
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
*       UM_Find_User
*
*   DESCRIPTION
*
*       This function retrieves the UM database record for the specified
*       user.
*
*   INPUTS
*
*       name                    Pointer to the user name between
*                               UM_MIN_NAME_SIZE and UM_MAX_NAME_SIZE
*                               bytes long.
*
*       dp                      Pointer to pointer to UM_USER struct
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       UM_USER_UNKNOWN         user name is not in UM database
*
*************************************************************************/
STATUS UM_Find_User(const CHAR *name, UM_USER *dp)
{
    STATUS              status;
    UM_USER_ELEMENT     *tp;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_Protect (&UM_Protect);

    /* find this user in the database */
    tp = UM_Scan_User (name);

    if (tp)
    {
        strcpy (dp->um_name, tp->um_name);
        strcpy (dp->um_pw, tp->um_pw);
        dp->um_pv = tp->um_pv;
        dp->um_id = tp->um_id;

        /* user name was found in the UM database */
        status = NU_SUCCESS;
    }
    else
    {
        status = UM_USER_UNKNOWN;
    }

    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Find_User */

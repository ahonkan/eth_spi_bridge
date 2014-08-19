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
*       um_mu.c
*
* DESCRIPTION
*
*       Modify user by ID
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UM_Modify_User
*
* DEPENDENCIES
*
*       nu_net.h
*       um_defs.h
*
************************************************************************/
#include "networking/nu_net.h"
#include "networking/um_defs.h"

/************************************************************************
*
*   FUNCTION
*
*       UM_Modify_User
*
*   DESCRIPTION
*
*       Modify an existing entry in the database, given the entry id and
*       a pointer to the user entry data structure. All fields of the
*       entry may be changed except the id, and the validity status of
*       the record is not changed in any way.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
************************************************************************/
STATUS UM_Modify_User(UINT32 id, const UM_USER *user)
{
    STATUS              status;
    UM_USER_ELEMENT     *entry;
    NU_SUPERV_USER_VARIABLES

    /* Validate parameters. */
    if ( (id == 0) || (user == NU_NULL) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Init */
    status = UM_USER_UNKNOWN;

    /* Exclude other threads while traversing the UM_List. */
    NU_Protect(&UM_Protect);

    /* Get a pointer to the start of the list. */
    entry = UM_List.flink;

    /* Cycle through the UM entries to check for an id that
       matches the parameter. */
    while (entry)
    {
        /* Is this the one we are searching for? */
        if (entry->um_id == id)
        {
            /* Copy user name and password strings to user. */
            strcpy(entry->um_name, user->um_name);
            strcpy(entry->um_pw, user->um_pw);

            /* Copy the id and permissions fields. Note that the id is
               read-only and cannot be modified. */
            entry->um_pv = user->um_pv;

            status = NU_SUCCESS;
            break;
        }

        /* No matching id was found. Move to the next entry. */
        entry = entry->flink;
    }

    /* Allow other threads to play with the list. */
    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Modify_User */

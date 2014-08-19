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
*       um_fui.c
*
* DESCRIPTION
*
*       Access functions to the User Management (UM) database via entry id.
*
* DATA STRUCTURES
*
*       UM_Next_ID
*
* FUNCTIONS
*
*       UM_FindUserByID
*       UM_Find_User_By_ID
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
*       UM_FindUserByID
*
*   DESCRIPTION
*
*       Worker function that searches for the entry associated with
*       a given id. If found, the user fields are copied to the location
*       specified by 'user'. This can be skipped by making 'user' a
*       null pointer, and thus, the function simply verifies the id.
*       Note: calling functions must use NU_Protect() before calling
*       this function.
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
STATUS UM_FindUserByID(UINT32 id, UM_USER *user)
{
    STATUS              status = UM_USER_UNKNOWN;
    UM_USER_ELEMENT     *entry = UM_List.flink;

    /* Validate parameters. Note that *user can be null. */
    if (id == 0)
        return NU_INVALID_PARM;

    /* Cycle through the UM entries to check for an id that
       matches the parameter. */
    while (entry)
    {
        /* The id comparison should ignore the 'valid' bit. */
        if (entry->um_id == id)
        {
            /* If a valid UM_USER structure was allocated, copy the contents
               of the entry into it. If it is null, then the contents aren't
               needed by the calling function. */
            if (user != NU_NULL)
            {
                /* Copy user name and password strings to user. */
                strcpy(user->um_name, entry->um_name);
                strcpy(user->um_pw, entry->um_pw);

                /* Copy the id and permissions fields. Note that the calling
                   function should never be aware of the 'valid' bit. */
                user->um_id = entry->um_id;
                user->um_pv = entry->um_pv;

            }

            status = NU_SUCCESS;

            break;

        }

        /* No matching id was found. Move to the next entry. */
        entry = entry->flink;
    }

    return (status);

} /* UM_FindUserByID */



/************************************************************************
*
*   FUNCTION
*
*       UM_Find_User_By_ID
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
STATUS UM_Find_User_By_ID(UINT32 id, UM_USER *user)
{
    STATUS              status;
    NU_SUPERV_USER_VARIABLES

    /* Validate parameters. */
    if (id == 0)
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Exclude other threads while traversing the UM_List. */
    NU_Protect(&UM_Protect);

    /* Call the internal function. */
    status = UM_FindUserByID(id, user);

    /* Allow other threads to play with the list. */
    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* end UM_Find_User_By_ID */

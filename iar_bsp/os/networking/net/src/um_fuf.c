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
*       um_fuf.c
*
* DESCRIPTION
*
*       Retrieve first user in User Management (UM) database
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UM_Find_User_First
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
*       UM_Find_User_First
*
*   DESCRIPTION
*
*       This function retrieves the first user in UM database
*
*   INPUTS
*
*       dp                      Pointer to UM_USER_TYPE structure
*       service                 service type requested
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       UM_USER_UNKNOWN         no users have been added to the UM
*                               database
*
*************************************************************************/
STATUS UM_Find_User_First(UM_USER *dp, UINT32 service)
{
    STATUS              status;
    UM_USER_ELEMENT     *user_ptr;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_Protect(&UM_Protect);

    /* Walk the list until a user is found for the requested service. */
    user_ptr = UM_List.flink;

    while (user_ptr && !(user_ptr->um_pv & service))
        user_ptr = user_ptr->flink;

    /* If dp is not null, then we found a match. */
    if (user_ptr)
    {
        strcpy (dp->um_name, user_ptr->um_name);
        strcpy (dp->um_pw, user_ptr->um_pw);
        dp->um_pv = user_ptr->um_pv;

        status = NU_SUCCESS;
    }
    else
        status = UM_USER_UNKNOWN;   /* no users have been added to the UM database */

    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Find_User_First */

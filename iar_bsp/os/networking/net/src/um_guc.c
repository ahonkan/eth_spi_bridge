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
*       um_guc.c
*
* DESCRIPTION
*
*       Count the number of users registered for the specified
*       NET services.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UM_Get_User_Count
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
*       UM_Get_User_Count
*
*   DESCRIPTION
*
*       This function counts the number of users that have registered
*       for the specified NET services.
*
*   INPUTS
*
*       pv                      permission value, NET application
*                               registrations defined in UM_Defs.h
*
*
*   OUTPUTS
*
*       The number of users registered for the specified NET services.
*
*************************************************************************/
UINT16 UM_Get_User_Count(UINT32 pv)
{
    UINT16          count;   /* count number of users who have the specified permissions */
    UM_USER_ELEMENT *entry;   /* user record entry */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    count = 0;

    NU_Protect(&UM_Protect);    /* lock out access by other functions */

    /* set the starting entry */
    entry = UM_List.flink;

    while (entry)
    {
        if ( (entry->um_pv & pv) == pv)
        {
            /* a match was found so increment the count */
            ++count;
        }

        /* point to the next record */
        entry = entry->flink;
    }

    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (count);

} /* UM_Get_User_Count */

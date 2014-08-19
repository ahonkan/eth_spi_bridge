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
*       um_du.c
*
* DESCRIPTION
*
*       Delete user from the User Management (UM) database
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       UM_Delete_User_Permissions
*
* DEPENDENCIES
*
*       nu_net.h
*       um_defs.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/um_defs.h"

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Memory already declared in um_a.c */
extern UM_USER_ELEMENT  NET_UM_User_Memory[];
extern UINT8            NET_UM_User_Memory_Flags[];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       UM_Delete_User_Permissions
*
*   DESCRIPTION
*
*       This functions deletes a user from the User Management (UM)
*       database and means this user is un-registering for all NET
*       services.
*
*   INPUTS
*
*       name                    Pointer to the user name between
*                               UM_MIN_NAME_SIZE and UM_MAX_NAME_SIZE
*                               bytes long.
*       pv                      The permission value, NET application
*                               registrations defined in UM_Defs.h
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       UM_USER_UNKNOWN         user name is not in the UM database
*       UM_PV_MISMATCH          Permission mismatch
*
*************************************************************************/
STATUS UM_Delete_User_Permissions(const CHAR *name, UINT32 pv)
{
    STATUS          status;
    UM_USER_ELEMENT *entry;   /* pointer to UM database entry */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_Protect (&UM_Protect);

    /* scan the UM database to find the selected user name */
    entry = UM_Scan_User(name);

    if (entry)
    {
        /* If this user has the specified privileges */
        if (entry->um_pv & pv)
        {
            /* Remove the privileges from the user */
            entry->um_pv &= ~pv;
            status = NU_SUCCESS;

            /* If removing the privileges leaves the user with no privileges,
             * delete the user from the database.
             */
            if (!(entry->um_pv))
            {
                /* the selected user record has been found */
                DLL_Remove(&UM_List, entry);

                NU_Unprotect();

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                /* deallocate the UM memory block*/
                status = NU_Deallocate_Memory(entry);

                if (status != NU_SUCCESS)
                    NLOG_Error_Log ("Error occurred while dealloc memory of a user management entry",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
#else
                /* Turn the memory flag off to indicate the memory is now unused */
                NET_UM_User_Memory_Flags[(UINT8)(entry - NET_UM_User_Memory)] = NU_FALSE;
                status = NU_SUCCESS;
#endif
            }

            else
                NU_Unprotect();
        }
        else
        {
            NU_Unprotect();
            status = UM_PV_MISMATCH;
        }

    }
    else
    {
        NU_Unprotect();
        status = UM_USER_UNKNOWN;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Delete_User_Permissions */

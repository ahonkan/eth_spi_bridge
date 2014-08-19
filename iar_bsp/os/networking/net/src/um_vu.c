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
*
*   FILE NAME
*
*       um_vu.c
*
*   DESCRIPTION
*
*       Validates user information in the UM database.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       UM_Validate_User
*
*   DEPENDENCIES
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
*       UM_Validate_User
*
*   DESCRIPTION
*
*       This function validates the current user information contained
*       in the UM database against the passed in parameters.
*
*   INPUTS
*
*       name                    user name (ASCII string between
*                               UM_MIN_NAME_SIZE and UM_MAX_NAME_SIZE)
*
*       pw                      password (ASCII string between
*                               UM_MIN_PW_SIZE and UM_MAX_PW_SIZE)
*
*       pv                      permission value, NET application
*                               registrations defined in UM_Defs.h
*
*
*
*   OUTPUTS
*
*       NU_SUCCESS              All parameters match the UM database
*                               record entry for specified user
*       UM_USER_UNKNOWN         no user name in the UM database matches
*                               the user name parameter
*       UM_PW_MISMATCH          user password in the UM database does
*                               not the password parameter
*       UM_PV_MISMATCH          user permissions in the UM database
*                               does not match the permissions parameter
*
*************************************************************************/
STATUS UM_Validate_User(const CHAR *name, const CHAR *pw, UINT32 pv)
{
    STATUS          status;
    UM_USER_ELEMENT *entry;
    INT             scv;            /* string compare value */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_Protect (&UM_Protect);

    /* find the specified user record in the UM database */
    entry = UM_Scan_User (name);

    if (entry)
    {

#if (UM_CASE_SENSITIVE == NU_TRUE)

        scv = strcmp (entry->um_pw, pw);       /* Do CASE SENSITIVE string compare */

#else

        scv = NU_STRICMP (entry->um_pw, pw);  /* Do CASE INSENSITIVE string compare */

#endif

        /* found the right user record, so validate the password and permissions */
        if (!scv)
        {
            /* password is valid, so check the permissions */
            if ((entry->um_pv & pv) == pv)
            {
                /* permissions are valid */
                status = NU_SUCCESS;
            }
            else
            {
                status = UM_PV_MISMATCH;
            }
        }
        else
        {
            status = UM_PW_MISMATCH;
        }
    }
    else
    {
        status = UM_USER_UNKNOWN;   /* the specified user is not in the UM database */
    }

    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Validate_User */

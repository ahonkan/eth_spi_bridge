/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       auth_au.c
*
*   COMPONENT
*
*       AUTH - User Authentication
*
*   DESCRIPTION
*
*       This file contains the function AUTH_Add_User, to which is
*       mapped the API function NU_Add_PPP_User. This file should only
*       be needed in a PPP server implementation.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       AUTH_Add_User
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

extern NU_SEMAPHORE PPP_Users;
extern STATUS UM_Modify_User(UINT32, UM_USER*);


#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*       AUTH_Add_User
*
* DESCRIPTION
*
*       Adds a new PPP user to User Management (UM) database. If the
*       SNMP security MIB is included in the build, then the user will
*       also be added to the security secrets table. This is intended
*       only for remote users dialing in.
*
* INPUTS
*
*       username                Pointer to the username string.
*       pw                      Pointer to the password string.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS AUTH_Add_User(CHAR *username, CHAR *pw)
{
    UM_USER     user;
    STATUS      status;
#if (INCLUDE_SEC_MIB == NU_TRUE)
    PMSS_ENTRY  *entry;
#endif
    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Protect list access to the UM database and optionally the
       PMSS list. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.",
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Return from function in user mode. */
        NU_USER_MODE();

        return status;
    }

    /* See if this user already exists in the UM database. */
    status = UM_Find_User(username, &user);
    if (status != NU_SUCCESS)
    {
        /* User does not exist. Add him. */
        UM_Add_User(username, pw, UM_PPP, 0);

        /* Verify that it was added, and fill in the user information. */
        status = UM_Find_User(username, &user);
    }

    if (status == NU_SUCCESS)
    {
        /* Make sure the user has PPP permissions. */
        if (!(user.um_pv & UM_PPP))
        {
            /* The user already existed with non-PPP permissions.
               Add them. */
            user.um_pv |= UM_PPP;
            status = UM_Modify_User(user.um_id, &user);
        }
    }

#if (INCLUDE_SEC_MIB == NU_TRUE)
    if (status == NU_SUCCESS)
    {
        /* See if there is already an entry in the PMSS table. */
        entry = PMSS_FindEntry(0, user.um_id);
        if (entry == NU_NULL)
        {
            /* Create a new entry in the table. */
            if (PMSS_NewEntry(0, user.um_id, 0, 2) == NU_NULL)
            {
                /* Log the error. */
                NLOG_Error_Log("Failed to add entry to PMSS table.",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }
#endif

    /* Allow access to the user lists. */
    NU_Release_Semaphore(&PPP_Users);

    /* Return from function in user mode. */
    NU_USER_MODE();

    return status;

} /* AUTH_Add_User */

#endif                                      /* PPP_ENABLE_UM_DATABASE */

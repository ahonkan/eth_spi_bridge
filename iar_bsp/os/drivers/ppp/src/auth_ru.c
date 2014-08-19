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
*       auth_ru.c
*
*   COMPONENT
*
*       AUTH - User Authentication
*
*   DESCRIPTION
*
*       This file contains the function AUTH_Remove_User, to which is
*       mapped the API function NU_Remove_PPP_User. This file should
*       only be needed in a PPP server implementation.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       AUTH_Remove_User
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
*       AUTH_Remove_User
*
* DESCRIPTION
*
*       Deletes a PPP user from User Management (UM) database. If the
*       SNMP security MIB is included in the build, then the user will
*       also be removed from the security secrets table.
*
* INPUTS
*
*       username                Pointer to the username string.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS AUTH_Remove_User(CHAR *username)
{
    STATUS      status;
    UM_USER     user;
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
        return status;
    }

    /* Make sure the user exists and get its information. */
    status = UM_Find_User(username, &user);
    if (status == NU_SUCCESS)
    {
        /* Make sure the user has PPP permissions. */
        if (!(user.um_pv & UM_PPP))
            status = UM_USER_UNKNOWN;
        else if (user.um_pv != UM_PPP)
        {
            /* The user has other non-PPP permissions, so just remove the
               PPP permissions and don't delete the user. */
            user.um_pv &= ~UM_PPP;
            status = UM_Modify_User(user.um_id, &user);
        }
        else
        {
            /* Delete the user from the UM database. */
            status = UM_Del_User(username);
        }
    }

#if (INCLUDE_SEC_MIB == NU_TRUE)
    if (status == NU_SUCCESS)
        status = PMSS_DeleteUserEntries(user.um_id);
#endif

    /* Allow access to the user lists. */
    NU_Release_Semaphore(&PPP_Users);

    /* Return from function in user mode. */
    NU_USER_MODE();

    return status;

} /* AUTH_Remove_User */

#endif                                      /* PPP_ENABLE_UM_DATABASE */

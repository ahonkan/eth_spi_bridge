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
*       auth.c
*
*   COMPONENT
*
*       AUTH - User Authentication
*
*   DESCRIPTION
*
*       This file contains functions and data structures that may be
*       used by both PAP and CHAP authentication protocols.
*
*   DATA STRUCTURES
*
*       PPP_Users
*       _passwordlist
*
*   FUNCTIONS
*
*       AUTH_Init
*       AUTH_Check_Login
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

NU_SEMAPHORE PPP_Users;
extern STATUS UM_Modify_User(UINT32, UM_USER*);


#if (PPP_ENABLE_UM_DATABASE == NU_FALSE)
/* This is the password list. When authenticating a user it is this list
   that will be used to compare the ID and PW. The length of the ID and PW
   pair is defined in ppp_cfg.h. The only restriction placed on this list
   is that the last entry MUST end with {'\0', '\0'}. */
PPP_USER _passwordlist[] =
{
    {"jon", "doe"},
    {"fred", "fish"},
    {"joe", "blow"},
    {"\0",   "\0"}
};
#endif


/*************************************************************************
* FUNCTION
*
*       AUTH_Init
*
* DESCRIPTION
*
*       Initializes PPP authentication structures and tables.
*
* INPUTS
*
*       dev_ptr                 Pointer to device structure.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS AUTH_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS       status = NU_SUCCESS;
#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
    static UINT8 semaphore_created = NU_FALSE;  
#endif

    UNUSED_PARAMETER(dev_ptr);
    UNUSED_PARAMETER(status);

#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
    /* if a semaphore for access to the PPP user tables is not already
       created, create one */
    if(semaphore_created == NU_FALSE)
    {
        /* Create semaphore for access to the PPP user tables.  */
        status = NU_Create_Semaphore(&PPP_Users, "PPP_UM", (UNSIGNED)1,
                                     NU_FIFO);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to create PPP Users semaphore.",
                           NERR_FATAL, __FILE__, __LINE__);
        }
        else
        {
            /* The semaphore has been created */
            semaphore_created = NU_TRUE;
        }
    }
#endif

    return status;

} /* AUTH_Init */



/*************************************************************************
* FUNCTION
*
*       AUTH_Check_Login
*
* DESCRIPTION
*
*       Searches the password list for a correct match.
*
* INPUTS
*
*     CHAR                      id[]        ID to check
*     CHAR                      pw[]        pw to check
*
* OUTPUTS
*
*     STATUS                    Was a correct match found
*
*************************************************************************/
STATUS AUTH_Check_Login(CHAR *name, CHAR *pw)
{
    STATUS  found_it;
#if (PPP_ENABLE_UM_DATABASE == NU_FALSE)
    STATUS  wrong_pw;
    UINT16  index;
#endif

#if (PPP_ENABLE_UM_DATABASE == NU_TRUE)
    if (UM_Validate_User(name, pw, UM_PPP) == NU_SUCCESS)
        found_it = NU_TRUE;
    else
        found_it = NU_FALSE;
#else
    /* Assume an invalid login */
    found_it = NU_FALSE;
    wrong_pw = NU_FALSE;

    /* All we need to do is loop through the password list and compare
       the ID and PW. */
    for (index = 0;(_passwordlist[index].id[0] != 0)
                    &&(found_it == NU_FALSE)
                    &&(wrong_pw == NU_FALSE); index++)
    {
        /* If the IDs match, check the PW */
        if ((strcmp(name, _passwordlist[index].id)) == 0)
        {
            if ((strcmp(pw, _passwordlist[index].pw)) == 0)

                /* If they both match then we found it. */
                found_it = NU_TRUE;
            else
            {
                /* The password must be wrong. */
                wrong_pw = NU_TRUE;
            }
        }
    }
#endif

    return found_it;

} /* AUTH_Check_Login */

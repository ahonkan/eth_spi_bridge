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
*       um_au.c
*
*   DESCRIPTION
*
*       Add new user to User Management (UM) database
*
*   DATA STRUCTURES
*
*       UM_List
*       UM_Protect
*
*   FUNCTIONS
*
*       UM_Check_User_Name
*       UM_Check_Pass_Word
*       UM_Scan_User
*       UM_Add_User
*       UM_Allocate_UserId
*
*   DEPENDENCIES
*
*       nu_net.h
*       um_defs.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/um_defs.h"

/* Private access to internal function. */
STATUS UM_Allocate_UserId(UINT32*);

/* Global variable to keep track of the next unused id. */
UINT32 UM_Next_ID = 1UL;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for UM Users */
UM_USER_ELEMENT NET_UM_User_Memory[NET_MAX_UM_USERS];

/* Declare memory flag array for the memory declared above */
UINT8           NET_UM_User_Memory_Flags[NET_MAX_UM_USERS] = {0};
#endif

/*************************************************************************
*
*   FUNCTION
*
*       UM_Check_User_Name
*
*   DESCRIPTION
*
*       This function performs validation checking on the user name
*       parameter.  Currently this function only checks that the user
*       name is at least UM_MIN_NAME_SIZE and is not longer than
*       UM_MAX_NAME_SIZE. This function may be changed by the customer
*       to perform any additional checks.
*
*   INPUTS
*
*       name                    Pointer to a user name between
*                               UM_MIN_NAME_SIZE and UM_MAX_NAME_SIZE
*                               bytes long.
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       UM_INVALID_NAME         user name is invalid
*
*************************************************************************/
STATUS UM_Check_User_Name(const CHAR *name)
{
    INT32 len;

    len = (INT32)strlen(name);   /* retrieve length of user name string */

    if ((len >= UM_MIN_NAME_SIZE) && (len <= UM_MAX_NAME_SIZE))
    {
        return (NU_SUCCESS);
    }
    else
    {
        return (UM_INVALID_NAME);
    }

} /* UM_Check_User_Name */

/*************************************************************************
*
*   FUNCTION
*
*       UM_Check_Pass_Word
*
*   DESCRIPTION
*
*       This function performs validation checking on the password
*       parameter. Currently this function only checks that the password
*       is at least UM_MIN_PW_SIZE and is not longer than UM_MAX_PW_SIZE.
*       This function may be changed by the customer to perform any
*       additional checks.
*
*   INPUTS
*
*       pw                      Pointer to a password between
*                               UM_MIN_PW_SIZE and UM_MAX_PW_SIZE bytes
*                               long.
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       UM_INVALID_PASSWORD     user name is invalid
*
*************************************************************************/
STATUS UM_Check_Pass_Word(const CHAR *pw)
{
    INT32 len;

    len = (INT32)strlen(pw);   /* retrieve length of pw string */

    if ((len >= UM_MIN_PW_SIZE) && (len <= UM_MAX_PW_SIZE))
    {
        return (NU_SUCCESS);
    }
    else
    {
        return (UM_INVALID_NAME);
    }

} /* UM_Check_Pass_Word */

/*************************************************************************
*
*   FUNCTION
*
*       UM_Scan_User
*
*   DESCRIPTION
*
*       This function starts at the head node of the UM database and
*       scans for the specified user name.  If the user name is found a
*       pointer to the selected node is returned.
*
*   INPUTS
*
*       name                    Pointer to the user name.
*
*   OUTPUTS
*
*       UM_USER_ELEMENT*        Pointer to UM database node.
*       NU_NULL                 User name does not exist in database.
*
*************************************************************************/
UM_USER_ELEMENT *UM_Scan_User(const CHAR *name)
{
    UM_USER_ELEMENT *node       = UM_List.flink;    /* point to first entry in UM_List */
    INT             scv;                            /* string compare value */

    while (node)
    {

#if (UM_CASE_SENSITIVE == NU_TRUE)

        scv = strcmp (node->um_name, name);   /* Do CASE SENSITIVE string compare */

#else

        scv = NU_STRICMP (node->um_name, name);  /* Do CASE INSENSITIVE string compare */

#endif
        if (!scv)
        {
            /* user name match has been found so stop the scan */
            break;
        }
        else
        {
            node = node->flink;   /* point to next node in UM database */
        }

    }

    return (node);   /* node is valid address or NU_NULL if error */

} /* UM_Scan_User */

/*************************************************************************
*
*   FUNCTION
*
*       UM_Add_User
*
*   DESCRIPTION
*
*       This function allows a new user to be added to the
*       User_Management (UM) database or an existing user to be updated.
*
*   INPUTS
*
*       name                    Pointer to the user name between
*                               UM_MIN_NAME_SIZE and UM_MAX_NAME_SIZE
*                               bytes long.
*
*       pw                      Pointer to the password between
*                               UM_MIN_PW_SIZE and UM_MAX_PW_SIZE
*                               bytes long.
*
*       pv                      The permission value, NET application
*                               registrations defined in UM_Defs.h
*
*       flags                   UM_ADD_MODE     add new user to UM database
*                               UM_UPDATE_MODE  update existing user in UM
*                                               database
*
*   OUTPUTS
*
*       NU_SUCCESS              operation was successful
*       NU_NO_MEMORY            memory allocation failure on new user
*                               data record
*       UM_INVALID_NAME         user name is invalid
*       UM_INVALID_PASSWORD     password is invalid
*       UM_USER_EXISTS          user name already exists in UM database
*                               and mode = UM_ADD_MODE
*       UM_USER_UNKNOWN         user name is not in UM database and
*                               mode = UM_UPDATE_MODE
*
*************************************************************************/
STATUS UM_Add_User(const CHAR *name, const CHAR *pw, UINT32 pv,
                   UINT16 flags)
{
    STATUS          status;
    UM_USER_ELEMENT *entry = NU_NULL, *entry_ptr, *new_entry = NU_NULL;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT             i;                      /* Counter to traverse an array */
#endif
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    entry_ptr = NU_NULL;
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Allocate a memory block for the new UM database record */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&entry,
                           sizeof(UM_USER_ELEMENT),
                           NU_NO_SUSPEND) != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_NO_MEMORY);
    }

#endif

    NU_Protect (&UM_Protect);

    /* Validate the NAME, PW parameters */
    status = UM_Check_User_Name (name);

    if (status == NU_SUCCESS)
    {
        status = UM_Check_Pass_Word (pw);
    }

    if (status == NU_SUCCESS)
    {
        /* All validation checks passed so do the work */

        entry_ptr = UM_Scan_User (name);

        /* If the user does not exist, fill in the new entry */
        if (!entry_ptr)
        {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

            /* Traverse the flag array to find free memory location */
            for (i=0; (NET_UM_User_Memory_Flags[i] != NU_FALSE) &&
                      (i != NET_MAX_UM_USERS); i++)
                ;

            if (i != NET_MAX_UM_USERS)
            {
                /* Assign memory to the new entry*/
                entry = &NET_UM_User_Memory[i];

                /* Turn the memory flag on */
                NET_UM_User_Memory_Flags[i] = NU_TRUE;
                status = NU_SUCCESS;
            }

            else
                status = NU_NO_MEMORY;

            if (status == NU_SUCCESS)
#endif

            {
                /* Clear the new entry structure. */
                UTL_Zero(entry, sizeof(UM_USER_ELEMENT));

                /* Make sure we can allocate an id for it. */
                status = UM_Allocate_UserId(&entry->um_id);

                if (status == NU_SUCCESS)
                {
                    new_entry = entry;
                    DLL_Enqueue (&UM_List, entry);
                }

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
                else
                {
                     /* Turn the memory flag off to indicate the memory is now unused */
                    NET_UM_User_Memory_Flags[(UINT8)(entry - NET_UM_User_Memory)] = NU_FALSE;
                }
#endif
            }
        }

        else
        {
            /* check for overwrite flag */
            if (!(flags & UM_UPDATE_MODE))
                status = UM_USER_EXISTS;
            else
                new_entry = entry_ptr;
        }

        /* If everything is ok up to this point, add the user data. */
        if ( (status == NU_SUCCESS) && (new_entry != NU_NULL) )
        {
            strcpy(new_entry->um_name, name);
            strcpy(new_entry->um_pw, pw);
            new_entry->um_pv |= pv;
        }
    }

    NU_Unprotect();

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Deallocate the memory allocated for the new entry since it
     * was not used.
     */
    if ( (entry_ptr) || (status != NU_SUCCESS) )
    {
        if (NU_Deallocate_Memory((VOID*)entry) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* UM_Add_User */

/************************************************************************
*
*   FUNCTION
*
*       UM_Allocate_UserId
*
*   DESCRIPTION
*
*       Assign a valid id to be used when building a new UM_USER entry.
*       This allows a function to create a local user entry, then add
*       that entry to the UM_List database at a later time.
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
STATUS UM_Allocate_UserId(UINT32 *id)
{
    UINT32          iid, first = UM_Next_ID;
    STATUS          status;

    /* Validate parameters. */
    if (id == NU_NULL)
        return NU_INVALID_PARM;

    /* Assign a new id for the calling function. Each id is checked to
       make sure it is not used. This could be possible if the global
       UM_Next_ID wraps. */
    for(;;)
    {
        /* Assign the next id to a temp variable. */
        iid = UM_Next_ID++;
        if (iid == 0)
            continue;

        /* See if the id is already in use. */
        status = UM_FindUserByID(iid, NU_NULL);

        if (status == UM_USER_UNKNOWN)
        {
            /* Id is not used, so give it to the calling function. */
            *id = iid;

            status = NU_SUCCESS;
            break;
        }

        /* If we actually cycle through the whole range of ids and can't find
           one to allocate, then the list must be full. Abort. */
        if (first == UM_Next_ID)
        {
            status = UM_LIST_FULL;
            break;
        }
    }

    return status;

} /* end UM_Allocate_UserId */

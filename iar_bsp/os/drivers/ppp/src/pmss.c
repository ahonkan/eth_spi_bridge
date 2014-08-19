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
*       pmss.c
*
*   COMPONENT
*
*       PMSS - PPP MIB access for Security Secrets Table
*
*   DESCRIPTION
*
*       Security Secrets MIB access functions for RFC 1472.
*
*   DATA STRUCTURES
*
*       PMSS_Table
*
*   FUNCTIONS
*
*       PMSS_NewEntry
*       PMSS_FindEntry
*       PMSS_DeleteUserEntries
*       PMSS_GetNextEntry
*       PMSS_SetClientLogin
*       PMSS_GetSecuritySecretsLink
*       PMSS_GetSecuritySecretsIdIndex
*       PMSS_GetSecuritySecretsDirection
*       PMSS_GetSecuritySecretsProtocol
*       PMSS_GetSecuritySecretsIdentity
*       PMSS_GetSecuritySecretsSecret
*       PMSS_GetSecuritySecretsStatus
*       PMSS_SetSecuritySecretsDirection
*       PMSS_SetSecuritySecretsProtocol
*       PMSS_SetSecuritySecretsIdentity
*       PMSS_SetSecuritySecretsSecret
*       PMSS_SetSecuritySecretsStatus
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

extern STATUS UM_Modify_User(UINT32, UM_USER*);

/* The security secrets table. */
struct
{
    PMSS_ENTRY *head;
    PMSS_ENTRY *tail;
} PMSS_Table;


/*************************************************************************
* FUNCTION
*
*       PMSS_NewEntry
*
* DESCRIPTION
*
*       Adds a new user entry into the security secrets table.
*       Note: The PPP user database must be protected (PPP_Users)
*       before entry into this function.
*
* INPUTS
*
*       dev_index               Device index.
*       um_id                   UM user id associated with this entry.
*       protocol                Authentication protocol to be used.
*       direction               PPP client or server.
*
* OUTPUTS
*
*       PMSS_ENTRY*             Pointer to the new entry.
*
*************************************************************************/
PMSS_ENTRY* PMSS_NewEntry(INT32 dev_index, INT32 um_id, UINT16 protocol,
                          UINT16 direction)
{
    PMSS_ENTRY *entry;
    STATUS status;

    /* Allocate memory for entry. */
    status = NU_Allocate_Memory(PPP_Memory, (VOID**)&entry, sizeof(PMSS_ENTRY), NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to allocate memory for PMSS entry.", NERR_SEVERE, __FILE__, __LINE__);
        return NU_NULL;
    }

    /* Clear the entry. */
    UTL_Zero(entry, sizeof(PMSS_ENTRY));
    entry->dev_index = dev_index;
    entry->um_id = um_id;

    /* Set the protocol type. If it is 0, then choose the best one. */
    if (protocol == 0)
#if (PPP_USE_CHAP == NU_TRUE)
        entry->auth = 1;
#else
        entry->auth = 2;
#endif
    if (protocol == PPP_CHAP_PROTOCOL)
        entry->auth = 1;
    else if (protocol == PPP_PAP_PROTOCOL)
        entry->auth = 2;

    /* Set the authentication direction, i.e. client or server. */
    entry->dir = direction;
    entry->status = 2;

    /* Link it into the database. */
    DLL_Enqueue(&PMSS_Table, entry);

    return entry;

} /* PMSS_NewEntry */



/*************************************************************************
* FUNCTION
*
*       PMSS_FindEntry
*
* DESCRIPTION
*
*       Returns a pointer to the security secrets entry associated
*       with the given index.
*       Note: The PPP user database must be protected (PPP_Users)
*       before entry into this function.
*
* INPUTS
*
*       link_id                 Link identifier.
*       uid                     The user ID.
*
* OUTPUTS
*
*       PMSS_ENTRY*
*
*************************************************************************/
PMSS_ENTRY* PMSS_FindEntry(INT32 link_id, INT32 uid)
{
    PMSS_ENTRY *entry;

    /* Find the entry in the database. */
    entry = PMSS_Table.head;

    while (entry != NU_NULL)
    {
        /* If the entry matches in both device index and user id,
           then return it. */
        if ((entry->dev_index == link_id || link_id == 0)
            && entry->um_id == uid)
            break;

        entry = entry->next;
    }

    return entry;

} /* PMSS_FindEntry */



/*************************************************************************
* FUNCTION
*
*       PMSS_DeleteUserEntries
*
* DESCRIPTION
*
*       Removes and deletes all entries associated with the given
*       UM user id from the security secrets table.
*       Note: The PPP user database must be protected (PPP_Users)
*       before entry into this function.
*
* INPUTS
*
*       um_id                   The UM user id to delete.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_DeleteUserEntries(INT32 um_id)
{
    PMSS_ENTRY *entry, *nextentry;
    STATUS status = UM_USER_UNKNOWN;

    /* Find and delete all entries for um_id in the database. */
    entry = PMSS_Table.head;
    while (entry != NU_NULL)
    {
        if (entry->um_id != um_id)
            entry = entry->next;
        else
        {
            /* An entry was found. Unlink it. */
            nextentry = DLL_Remove(&PMSS_Table, entry);

            /* Deallocate the memory. */
            NU_Deallocate_Memory(entry);

            entry = nextentry;
            status = NU_SUCCESS;
        }
    }

    /* Not found. */
    return status;

} /* PMSS_DeleteUserEntries */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetNextEntry
*
* DESCRIPTION
*
*       Supplies the next entry id in the security secrets table that
*       immediately follows the given entry id.
*
* INPUTS
*
*       *link_id                Pointer to the link index.
*       *uid                    Pointer to the user id.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetNextEntry(INT32 *link_id, INT32 *uid)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get the first entry index. */
    if ((*link_id == 0) && (*uid == 0))
    {
        if (PMSS_Table.head != NU_NULL)
        {
            *link_id = PMSS_Table.head->dev_index;
            *uid = PMSS_Table.head->um_id;
            status = NU_SUCCESS;
        }
        else
            status = UM_USER_UNKNOWN;
    }
    else
    {
        /* Find the entry in the database. */
        entry = PMSS_FindEntry(*link_id, *uid);
        if (entry != NU_NULL && entry->next != NU_NULL)
        {
            /* A next entry exists. Write the indices. */
            *link_id = entry->next->dev_index;
            *uid = entry->next->um_id;
            status = NU_SUCCESS;
        }
        else
            status = UM_USER_UNKNOWN;
    }

    /* Allow access to the user lists. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetNextEntry */



/*************************************************************************
* FUNCTION
*
*       PMSS_SetClientLogin
*
* DESCRIPTION
*
*       Find or add a client user to both the UM database and the
*       security secrets table.
*
* INPUTS
*
*       dev_index               Index of the device to set login
*                               information.
*       *username               Username string.
*       *password               Password string.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_SetClientLogin(INT32 dev_index, CHAR *username, CHAR *password)
{
    STATUS status;
    UM_USER user;
    PMSS_ENTRY *entry = PMSS_Table.head;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* See if the user already exists in the database. */
    status = UM_Find_User(username, &user);
    if (status != NU_SUCCESS)
    {
        /* User doesn't exist, so add it. */
        UM_Add_User(username, password, UM_PPP, 0);
        status = UM_Find_User(username, &user);
        if (status != NU_SUCCESS)
        {
            /* Release the user lists. */
            NU_Release_Semaphore(&PPP_Users);

            return status;
        }
    }

    /* See if there is an entry in the PMSS table that matches. */
    while (entry != NU_NULL)
    {
        if (entry->um_id == (INT32)user.um_id
            && (entry->dev_index == dev_index + 1 || entry->dev_index == 0)
            && entry->dir == 1
            && entry->status == 2)
        {
            break;
        }

        entry = entry->next;
    }

    if (entry == NU_NULL)
    {
        /* No entry in the table, so add it. */
        entry = PMSS_NewEntry(dev_index + 1, user.um_id, 0, 1);
        if (entry == NU_NULL)
            status = PM_ERROR;
    }
    else
    {
        /* Entry already exists in the table, so just change the information. */
        entry->dir = 1;
        entry->status = 2;

#if (PPP_USE_CHAP)
        entry->auth = 1;
#else
        entry->auth = 2;
#endif

        status = NU_SUCCESS;
    }

    /* Allow access to the user lists. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_SetClientLogin */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsLink
*
* DESCRIPTION
*
*       Provides the device index associated with the given entry index.
*
* INPUTS
*
*       link_id                 The link identifier.
*       id                      The index of the entry in the table.
*       *value                  Pointer to storage variable for the index.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsLink(INT32 link_id, INT32 uid, INT32 *value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        *value = entry->dev_index;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsLink */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsIdIndex
*
* DESCRIPTION
*
*       Provides the UM user id associated with the security secrets
*       entry id.
*
* INPUTS
*
*       link_id                 The link identifier.
*       uid                     The index of the entry in the table.
*       *value                  Pointer to storage variable for the id.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsIdIndex(INT32 link_id, INT32 uid,
                                      INT32 *value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        *value = entry->um_id;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsIdIndex */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsDirection
*
* DESCRIPTION
*
*       Provides the direction of the link, meaning PPP client
*       (local-to-remote) or PPP server (remote-to-local).
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to storage variable for direction.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsDirection(INT32 link_id, INT32 uid,
                                        INT32 *value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        *value = entry->dir;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsDirection */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsProtocol
*
* DESCRIPTION
*
*       Provides the authentication protocol that should be used
*       during negotiation of any link associated with this entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to storage variable for the protocol.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsProtocol(INT32 link_id, INT32 uid,
                                       INT32 *value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        *value = entry->auth;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsProtocol */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsIdentity
*
* DESCRIPTION
*
*       Provides the username string associated with the entry index.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to storage buffer for the string.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsIdentity(INT32 link_id, INT32 uid, UINT8 *value)
{
    UM_USER     user;
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        status = UM_Find_User_By_ID(entry->um_id, &user);
        if (status == NU_SUCCESS)
            strcpy((CHAR*)value, user.um_name);
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsIdentity */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsSecret
*
* DESCRIPTION
*
*       Provides the password string associated with the entry index.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to storage buffer for the string.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsSecret(INT32 link_id, INT32 uid,
                                     UINT8 *value)
{
    UM_USER user;
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        status = UM_Find_User_By_ID(entry->um_id, &user);
        if (status == NU_SUCCESS)
            strcpy((CHAR*)value, user.um_pw);
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsSecret */



/*************************************************************************
* FUNCTION
*
*       PMSS_GetSecuritySecretsStatus
*
* DESCRIPTION
*
*       Provides the status (valid or invalid) of the security secrets
*       entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to storage variable for the status.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_GetSecuritySecretsStatus(INT32 link_id, INT32 uid,
                                     INT32 *value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        *value = entry->status;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_GetSecuritySecretsStatus */



/*************************************************************************
* FUNCTION
*
*       PMSS_SetSecuritySecretsDirection
*
* DESCRIPTION
*
*       Sets the given direction (remote-to-local or local-to-remote)
*       into the security secrets entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       value                   Direction to set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_SetSecuritySecretsDirection(INT32 link_id, INT32 uid,
                                        INT32 value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        entry->dir = value;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_SetSecuritySecretsDirection */



/*************************************************************************
* FUNCTION
*
*       PMSS_SetSecuritySecretsProtocol
*
* DESCRIPTION
*
*       Sets the authentication protocol to be used for any link that
*       is associated with this entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       value                   Protocol to set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_SetSecuritySecretsProtocol(INT32 link_id, INT32 uid,
                                       INT32 value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        entry->auth = value;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_SetSecuritySecretsProtocol */



/*************************************************************************
* FUNCTION
*
*       PMSS_SetSecuritySecretsIdentity
*
* DESCRIPTION
*
*       Sets the username string into the security secrets entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to string that will be set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_SetSecuritySecretsIdentity(INT32 link_id, INT32 uid,
                                       UINT8 *value)
{
    UM_USER user;
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        status = UM_Find_User_By_ID(entry->um_id, &user);
        if (status == NU_SUCCESS)
        {
            strcpy(user.um_name, (CHAR*)value);
            status = UM_Modify_User(user.um_id, &user);
        }
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_SetSecuritySecretsIdentity */



/*************************************************************************
* FUNCTION
*
*       PMSS_SetSecuritySecretsSecret
*
* DESCRIPTION
*
*       Sets the password string into the security secrets entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       *value                  Pointer to string that will be set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_SetSecuritySecretsSecret(INT32 link_id, INT32 uid,
                                     UINT8 *value)
{
    UM_USER user;
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        status = UM_Find_User_By_ID(entry->um_id, &user);
        if (status == NU_SUCCESS)
        {
            strcpy(user.um_pw, (CHAR*)value);
            status = UM_Modify_User(user.um_id, &user);
        }
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_SetSecuritySecretsSecret */



/*************************************************************************
* FUNCTION
*
*       PMSS_SetSecuritySecretsStatus
*
* DESCRIPTION
*
*       Sets the status (valid or invalid) of the given entry.
*
* INPUTS
*
*       link_id                 The device index of the entry in the table.
*       uid                     The user index of the entry in the table.
*       value                   Status value to be set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSS_SetSecuritySecretsStatus(INT32 link_id, INT32 uid, INT32 value)
{
    PMSS_ENTRY  *entry;
    STATUS      status;

    /* Protect access to the security secrets table. */
    status = NU_Obtain_Semaphore(&PPP_Users, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log the error and return. */
        NLOG_Error_Log("Failed to obtain semaphore.", NERR_SEVERE, __FILE__, __LINE__);
        return status;
    }

    /* Get a pointer to the entry we need. */
    entry = PMSS_FindEntry(link_id, uid);
    if (entry != NU_NULL)
    {
        entry->status = value;
        status = NU_SUCCESS;
    }
    else
        status = PM_ERROR;

    /* Release access to the security secrets table. */
    NU_Release_Semaphore(&PPP_Users);

    return status;

} /* PMSS_SetSecuritySecretsStatus */

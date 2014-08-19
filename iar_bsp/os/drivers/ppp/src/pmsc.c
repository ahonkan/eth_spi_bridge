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
*       pmsc.c
*
*   COMPONENT
*
*       PMSC - PPP MIB access to Security Configuration Table
*
*   DESCRIPTION
*
*       Security Configuration MIB access functions for RFC 1472.
*
*   DATA STRUCTURES
*
*       PMSC_Table
*
*   FUNCTIONS
*
*       PMSC_NewEntry
*       PMSC_FindEntry
*       PMSC_GetNextEntry
*       PMSC_GetDefaultProtocol
*       PMSC_GetSecurityConfigLink
*       PMSC_GetSecurityConfigPreference
*       PMSC_GetSecurityConfigProtocol
*       PMSC_GetSecurityConfigStatus
*       PMSC_SetSecurityConfigLink
*       PMSC_SetSecurityConfigPreference
*       PMSC_SetSecurityConfigProtocol
*       PMSC_SetSecurityConfigStatus
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

/* The security configuration table. */
struct
{
    PMSC_ENTRY *head;
    PMSC_ENTRY *tail;
} PMSC_Table;



/*************************************************************************
* FUNCTION
*
*       PMSC_NewEntry
*
* DESCRIPTION
*
*       Adds a new entry into the security configuration table.
*
* INPUTS
*
*       dev_index               Device index associated with this entry.
*       protocol                Authentication protocol to be used.
*       pref                    The preference level for the protocol.
*
* OUTPUTS
*
*       PMSC_ENTRY*
*
*************************************************************************/
PMSC_ENTRY* PMSC_NewEntry(INT32 dev_index, UINT16 protocol, UINT16 pref)
{
    PMSC_ENTRY *entry;
    STATUS status;

    /* Allocate memory for entry. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&entry, sizeof(PMSC_ENTRY), NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Log error. */
        NLOG_Error_Log("Failed to allocate memory for PMSC entry.", NERR_SEVERE, __FILE__, __LINE__);
        return NU_NULL;
    }

    /* Clear the entry. */
    UTL_Zero(entry, sizeof(PMSC_ENTRY));
    entry->dev_index = dev_index + 1;

    if (protocol == PPP_CHAP_PROTOCOL)
        entry->auth = 1;
    else if (protocol == PPP_PAP_PROTOCOL)
        entry->auth = 2;

    entry->pref = pref;
    entry->status = 2;

    /* Link it into the database. */
    DLL_Enqueue(&PMSC_Table, entry);

    return entry;

} /* PMSC_NewEntry */



/*************************************************************************
* FUNCTION
*
*       PMSC_FindEntry
*
* DESCRIPTION
*
*       Returns a pointer to the security configuration entry associated
*       with the given index.
*
* INPUTS
*
*       dev_index               Device index associated with this entry.
*       pref                    The preference level for the protocol.
*
* OUTPUTS
*
*       PMSC_ENTRY*
*
*************************************************************************/
PMSC_ENTRY* PMSC_FindEntry(INT32 dev_index, INT32 pref)
{
    PMSC_ENTRY *entry;

    /* Find the entry in the database. */
    entry = PMSC_Table.head;

    while (entry != NU_NULL)
    {
        if (entry->dev_index == dev_index && entry->pref == pref)
            return entry;

        entry = entry->next;
    }

    return NU_NULL;

} /* PMSC_FindEntry */



/*************************************************************************
* FUNCTION
*
*       PMSC_GetNextEntry
*
* DESCRIPTION
*
*       Supplies the index of the entry in the security config table that
*       immediately follows the given entry id.
*
* INPUTS
*
*       *link_id                On return, contains link ID.
*       *pref                   On return, contains preference level.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_GetNextEntry(INT32 *link_id, INT32 *pref)
{
    PMSC_ENTRY *entry;

    /* Get the first entry index. */
    if (*link_id == 0)
    {
        if (PMSC_Table.head != NU_NULL)
        {
            *link_id = PMSC_Table.head->dev_index;
            *pref = PMSC_Table.head->pref;
            return NU_SUCCESS;
        }
    }
    else
    {
        /* Find the current entry in the database. */
        entry = PMSC_FindEntry(*link_id, *pref);
        if (entry != NU_NULL && entry->next != NU_NULL)
        {
            /* A next entry exists. Write the indices. */
            *link_id = entry->next->dev_index;
            *pref = entry->next->pref;
            return NU_SUCCESS;
        }
    }

    return PM_ERROR;

} /* PMSC_GetNextEntry */



/*************************************************************************
* FUNCTION
*
*       PMSC_GetDefaultProtocol
*
* DESCRIPTION
*
*       Provides the authentication protocol with the highest preference
*       level for the specified device index.
*
* INPUTS
*
*       dev_index
*       *protocol
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_GetDefaultProtocol(INT32 dev_index, UINT16 *protocol)
{
    INT32 temp = 0x7FFFFFFFL;
    UINT16 auth = 0;
    STATUS status = NU_NOT_FOUND;

    PMSC_ENTRY *entry = PMSC_Table.head;

    while (entry != NU_NULL)
    {
        if (entry->dev_index == dev_index + 1)
        {
            if (entry->pref >= 0 && entry->pref < temp)
            {
                temp = entry->pref;

                if (entry->auth == 1)
                    auth = PPP_CHAP_PROTOCOL;
                else if (entry->auth == 2)
                    auth = PPP_PAP_PROTOCOL;

                status = NU_SUCCESS;
            }
        }

        entry = entry->next;
    }

    if (status == NU_SUCCESS)
        *protocol = auth;

    return status;

} /* PMSC_GetDefaultProtocol */



/*************************************************************************
* FUNCTION
*
*       PMSC_GetSecurityConfigLink
*
* DESCRIPTION
*
*       Provides the device index that is associated with the given
*       security configuration entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       *value                  Pointer to storage variable for the link.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_GetSecurityConfigLink(INT32 id, INT32 pref, INT32 *value)
{
    /* Verify that the entry id is valid, and use it as an index
       into the security config table. */
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Write the dev_index to the arg location. */
        *value = entry->dev_index;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_GetSecurityConfigLink */



/*************************************************************************
* FUNCTION
*
*       PMSC_GetSecurityConfigPreference
*
* DESCRIPTION
*
*       Provides the preference level for the authentication protocol
*       associated with the given security configuration entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       *value                  Pointer to variable for the preference.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_GetSecurityConfigPreference(INT32 id, INT32 pref, INT32 *value)
{
    /* Verify that the entry id is valid, and use it as an index
       into the security config table. */
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Write the preference level to the arg location. */
        *value = entry->pref;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_GetSecurityConfigPreference */



/*************************************************************************
* FUNCTION
*
*       PMSC_GetSecurityConfigProtocol
*
* DESCRIPTION
*
*       Provides the authentication protocol associated with the given
*       security configuration entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       *value                  Pointer to variable for the protocol.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_GetSecurityConfigProtocol(INT32 id, INT32 pref, INT32 *value)
{
    /* Verify that the entry id is valid, and use it as an index
       into the security config table. */
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Write the protocol to the arg location. */
        *value = entry->auth;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_GetSecurityConfigProtocol */



/*************************************************************************
* FUNCTION
*
*       PMSC_GetSecurityConfigStatus
*
* DESCRIPTION
*
*       Provides the status (valid or invalid) of the given security
*       configuration entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       *value                  Pointer to variable for the status.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_GetSecurityConfigStatus(INT32 id, INT32 pref, INT32 *value)
{
    /* Verify that the entry id is valid, and use it as an index
       into the security config table. */
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Write the validity status to the arg location. */
        *value = entry->status;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_GetSecurityConfigStatus */



/*************************************************************************
* FUNCTION
*
*       PMSC_SetSecurityConfigLink
*
* DESCRIPTION
*
*       Sets the device index that is associated with the given
*       security configuration entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       value                   Value to set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_SetSecurityConfigLink(INT32 id, INT32 pref, INT32 value)
{
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Set dev_index into the structure. */
        entry->dev_index = value;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_SetSecurityConfigLink */



/*************************************************************************
* FUNCTION
*
*       PMSC_SetSecurityConfigPreference
*
* DESCRIPTION
*
*       Sets the preference level for the authentication protocol that
*       is associated with this entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       value                   Value to set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_SetSecurityConfigPreference(INT32 id, INT32 pref, INT32 value)
{
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Set preference into the structure. */
        entry->pref = value;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_SetSecurityConfigPreference */



/*************************************************************************
* FUNCTION
*
*       PMSC_SetSecurityConfigProtocol
*
* DESCRIPTION
*
*       Sets the authentication protocol to be used for any link that
*       is associated with this entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       value                   Protocol to set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_SetSecurityConfigProtocol(INT32 id, INT32 pref, INT32 value)
{
    /* Verify that the entry id is valid, and use it as an index
       into the security config table. */
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Write the protocol to the arg location. */
        if (value == 1)
            entry->auth = 1;
        else if (value == 2)
            entry->auth = 2;

        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_SetSecurityConfigProtocol */



/*************************************************************************
* FUNCTION
*
*       PMSC_SetSecurityConfigStatus
*
* DESCRIPTION
*
*       Sets the status (valid or invalid) of the given security
*       configuration entry.
*
* INPUTS
*
*       id                      The index of the entry in the table.
*       pref                    Preference level for the protocol.
*       value                   Status value to set.
*
* OUTPUTS
*
*       STATUS
*
*************************************************************************/
STATUS PMSC_SetSecurityConfigStatus(INT32 id, INT32 pref, INT32 value)
{
    PMSC_ENTRY *entry = PMSC_FindEntry(id, pref);

    if (entry != NU_NULL)
    {
        /* Set preference into the structure. */
        entry->status = value;
        return NU_SUCCESS;
    }

    return PM_ERROR;

} /* PMSC_SetSecurityConfigStatus */

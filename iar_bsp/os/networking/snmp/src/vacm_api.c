/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       vacm_api.c                                               
*
*   DESCRIPTION
*
*       This file contains API functions for VACM. These functions are
*       also required for VACM MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       VACM_Remove_Context
*       VACM_Remove_Group
*       VACM_Remove_AccessEntry
*       VACM_Remove_MibView
*
*   DEPENDENCIES
*
*       target.h
*       xtypes.h
*       vacm.h
*       snmp_cfg.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/xtypes.h"
#include "networking/vacm.h"
#include "networking/snmp_cfg.h"

extern VACM_MIB_STRUCT Vacm_Mib_Table;

/************************************************************************
*
*   FUNCTION
*
*       VACM_Remove_Context
*
*   DESCRIPTION
*
*       Removes a context name entry from the list.
*
*   INPUTS
*
*       *context_name       the pointer to a context_name
*
*   OUTPUTS
*
*       NU_SUCCESS          The entry is removed.
*       SNMP_ERROR          The entry was not found.
*       SNMP_BAD_PARAMETER  Null parameter passed in.
*
*************************************************************************/
STATUS VACM_Remove_Context(UINT8 *context_name)
{
    STATUS                   status;
    VACM_CONTEXT_STRUCT      *delete_ptr;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (context_name != NU_NULL)
    {
        /* Find the node for the context_name. */
        if((status = VACM_Search_Context(context_name, &delete_ptr))
                                                            == NU_SUCCESS)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Vacm_Mib_Table.vacm_context_table, delete_ptr);

            /* Deallocate the memory occupied by the node. */
            if (NU_Deallocate_Memory(delete_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }
    
    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Remove_Context */


/************************************************************************
*
*   FUNCTION
*
*       VACM_Remove_Group
*
*   DESCRIPTION
*
*       This function removes a group from the list.
*
*   INPUTS
*
*       *location_ptr       Pointer to a group structure.
*
*   OUTPUTS
*
*       NU_SUCCESS          The entry was successfully removed.
*       SNMP_ERROR          The entry was not found.
*       SNMP_BAD_PARAMETER  Null pointer was passed in. 
*
************************************************************************/
STATUS VACM_Remove_Group (VACM_SEC2GROUP_STRUCT *location_ptr)
{
    STATUS                      status = NU_SUCCESS;
    VACM_SEC2GROUP_STRUCT       *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (location_ptr != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Vacm_Mib_Table.vacm_security_to_group_tab.next;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == location_ptr)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->next;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Vacm_Mib_Table.vacm_security_to_group_tab,
                       location_ptr);

    #if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
            /* Set the group row Status to destroy so that
               the group is not added to the table if this call
               is made by the user. */
            location_ptr->vacm_status = SNMP_ROW_DESTROY;

            VACM_Save_Group(location_ptr);
    #endif

            /* Deallocate the memory occupied by the node. */
            if (NU_Deallocate_Memory(location_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }
        
        else
        {
            status = SNMP_ERROR;
        }
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Remove_Group */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Remove_AccessEntry
*
*   DESCRIPTION
*
*       This function removes an access entry from the list.
*
*   INPUTS
*
*       *location_ptr       Pointer to the access entry structure.
*
*   OUTPUTS
*
*       NU_SUCCESS          The entry was removed successfully removed.
*       SNMP_WARNING        The entry does not exist.
*       SNMP_BAD_PARAMETER  A nuill pointer was passed in. 
*
************************************************************************/
STATUS VACM_Remove_AccessEntry (VACM_ACCESS_STRUCT * location_ptr)
{
    STATUS                  status = NU_SUCCESS;
    VACM_ACCESS_STRUCT      *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (location_ptr != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Vacm_Mib_Table.vacm_access_tab.next;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == location_ptr)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->next;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Vacm_Mib_Table.vacm_access_tab, location_ptr);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
            /* Set the AccessEntry row Status to destroy so that
               the AccessEntry is not added to the table if this call
               is made by the user. */
            location_ptr->vacm_status = SNMP_ROW_DESTROY;

            VACM_Save_Access(location_ptr);
#endif

            /* Deallocate the memory occupied by the node. */
            if (NU_Deallocate_Memory(location_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }
        
        else
        {
            status = SNMP_WARNING;
        }
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* VACM_Remove_AccessEntry */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Remove_MibView
*
*   DESCRIPTION
*
*       This function removes a view from the list.
*
*   INPUTS
*
*       *location_ptr       Pointer to the view structure.
*
*     OUTPUTS
*
*       NU_SUCCESS          The entry was removed successfully.
*       SNMP_ERROR          The entry was not found.
*       SNMP_BAD_PARAMETER  Null pointer passed in. 
*
************************************************************************/
STATUS VACM_Remove_MibView  (VACM_VIEWTREE_STRUCT *location_ptr)
{
    STATUS                      status = NU_SUCCESS;
    VACM_VIEWTREE_STRUCT        *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (location_ptr != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Vacm_Mib_Table.vacm_view_tree_family_tab.next;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == location_ptr)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->next;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Vacm_Mib_Table.vacm_view_tree_family_tab,
                       location_ptr);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
            /* Set the MibView row Status to destroy so that
               the MibView is not added to the table if this call
               is made by the user. */
            location_ptr->vacm_status = SNMP_ROW_DESTROY;

            VACM_Save_View(location_ptr);
#endif

            /* Deallocate the memory occupied by the node. */
            if (NU_Deallocate_Memory(location_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }
        
        else
        {
            status = SNMP_ERROR;
        }
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Remove_MibView */








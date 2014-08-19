/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILENAME                                               
*
*       no_api.c                                                 
*
*   DESCRIPTION
*
*       This file contains API for SNMP Notifications.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_Find_Notify_Entry
*       SNMP_Find_Profile_Entry
*       SNMP_Find_Filter_Entry
*       SNMP_Remove_From_Notify_Table
*       SNMP_Remove_From_Filter_Table
*       SNMP_Remove_From_Profile_Table
*
*    DEPENDENCIES
*
*       snmp.h
*       no_api.h
*       snmp_utl.h
*
*************************************************************************/

#include "networking/snmp.h"
#include "networking/no_api.h"
#include "networking/snmp_utl.h"

extern SNMP_NOTIFY_TABLE_ROOT              Snmp_Notify_Table;
extern SNMP_PROFILE_TABLE_ROOT             Snmp_Profile_Table;
extern SNMP_FILTER_TABLE_ROOT              Snmp_Filter_Table;

/***********************************************************************
*
*   Function
*
*       SNMP_Find_Notify_Entry
*
*   DESCRIPTION
*
*       Finds an entry based on the name.
*
*   INPUTS
*
*       *notify_name            Name to be used to find the entry.
*
*   OUTPUTS
*
*       SNMP_NOTIFY_TABLE*      Successful.
*       NU_NULL                 Entry not found.
*
*************************************************************************/
SNMP_NOTIFY_TABLE* SNMP_Find_Notify_Entry(const UINT8 *notify_name)
{
    /* Handle to the notification table entry. */
    SNMP_NOTIFY_TABLE             *entry;

    /* Variable to hold the comparison results. */
    INT32                         cmp = 0;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (notify_name != NU_NULL)
    {
        /* Loop through all the entries to find the entry. */
        entry = Snmp_Notify_Table.flink;

        while(entry != NU_NULL)
        {
            /* Check whether the name of the current entry matches,
             * the name passed or if the current entry has a bigger
             * name. If it does, break out of the loop.
             */
            if((cmp = strcmp((CHAR *) entry->snmp_notify_name,
                             (CHAR *)notify_name)) >= 0)
            {
                break;
            }

            /* This is not the entry we required, go to the next entry. */
            entry = entry->flink;
        }

       /* If the name comparison had indicated that the current entry has
        * a greater name we need to assign null to the returned value.
        * This is because we broke out of the loop when we had reached an
        * entry which had a greater name than what we were looking. Since
        * we have a list in ascending order, there is no hope of finding
        * an entry which matches.
        */
        if(cmp > 0)
        {
            entry = NU_NULL;
        }
    }
    else
    {
        entry = NU_NULL;
    }
    
    NU_USER_MODE();    /* return to user mode */

    /* Return handle to the notification table entry if found, otherwise
     * return NU_NULL.
     */
    return (entry);

} /* SNMP_Find_Notify_Entry */

/***********************************************************************
*
*   Function
*
*       SNMP_Find_Profile_Entry
*
*   DESCRIPTION
*
*       Finds an entry based on the params name.
*
*   INPUTS
*
*       *params_name    Params Name
*
*   OUTPUTS
*
*       SNMP_NOTIFY_FILTER_PROFILE_TABLE *  Successful.
*       NU_NULL                             Entry not found.
*
*************************************************************************/
SNMP_NOTIFY_FILTER_PROFILE_TABLE* SNMP_Find_Profile_Entry(
                                                 const UINT8 *params_name)
{
    SNMP_NOTIFY_FILTER_PROFILE_TABLE    *entry;
    INT32                               cmp = 0;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (params_name != NU_NULL)
    {
        /* Loop through all the entries to find the entry. */
        entry = Snmp_Profile_Table.flink;

        while(entry != NU_NULL)
        {
            /* Check whether the name of the current entry matches, the
             * name passed or if the current entry has a bigger name. If
             * it does, break out of the loop.
             */
            if((cmp = strcmp((CHAR *) entry->snmp_target_params_name,
                             (CHAR *) params_name)) >= 0)
            {
                /* Break through the loop. */
                break;
            }

            /* This is not the entry we required, go to the next entry. */
            entry = entry->flink;
        }


       /* If the name comparison had indicated that the current entry has
        * a greater name we need to assign null to the returned value.
        * This is because we broke out of the loop when we had reached an
        * entry which had a greater name than what we were looking. Since
        * we have a list in ascending order, there is no hope of finding
        * an entry which matches.
        */
        if(cmp > 0)
        {
            entry = NU_NULL;
        }
    }
    
    else
    {
        entry = NU_NULL;
    }

    NU_USER_MODE();    /* return to user mode */
    return (entry);

} /* SNMP_Find_Profile_Entry */

/***********************************************************************
*
*   Function
*
*       SNMP_Find_Filter_Entry
*
*   DESCRIPTION
*
*       Finds an entry based on the name.
*
*   INPUTS
*
*       *filter_name   Name to be used to find the entry.
*
*   OUTPUTS
*
*       SNMP_NOTIFY_FILTER_TABLE *  Successful.
*       NU_NULL                     Entry not found.
*
*************************************************************************/
SNMP_NOTIFY_FILTER_TABLE* SNMP_Find_Filter_Entry(const UINT8 *filter_name)
{
    SNMP_NOTIFY_FILTER_TABLE    *entry;
    INT32                       cmp = 0;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (filter_name != NU_NULL)
    {
        /* Loop through all the entries to find the entry. */
        entry = Snmp_Filter_Table.flink;

        while(entry != NU_NULL)
        {
            /* Check whether the name of the current entry matches, the
             * name passed or if the current entry has a bigger name. If
             * it does, break out of the loop.
             */
            if((cmp = UTL_Admin_String_Cmp(
                        (CHAR *) entry->snmp_notify_filter_profile_name,
                        (CHAR *)filter_name)) >= 0)
            {
                /* Break through the loop. */
                break;
            }

            /* This is not the entry we required, go to the next entry. */
            entry = entry->flink;
        }

       /* If the name comparison had indicated that the current entry has
        * a greater name we need to assign null to the returned value.
        * This is because we broke out of the loop when we had reached an
        * entry which had a greater name than what we were looking. Since
        * we have a list in ascending order, there is no hope of finding
        * an entry which matches.
        */
        if(cmp > 0)
        {
            entry = NU_NULL;
        }
    }

    else
    {
        entry = NU_NULL;
    }

    NU_USER_MODE();    /* return to user mode */
    return (entry);

} /* SNMP_Find_Filter_Entry */

/***********************************************************************
*
*   Function
*
*       SNMP_Remove_From_Notify_Table
*
*   DESCRIPTION
*
*       Removes a node from one of the lists in the Notifications MIB.
*
*   INPUTS
*
*       *node           Node which will be removed
*
*   OUTPUTS
*
*       NU_SUCCESS             Node successfully removed
*       SNMP_ERROR             The node was not found.
*       SNMP_INVALID_POINTER   Could not deallocate memory
*       SNMP_INVALID_PARAMETER A null pointer was passed in.
*
*************************************************************************/
STATUS SNMP_Remove_From_Notify_Table(SNMP_NOTIFY_TABLE *node)
{
    STATUS              status = NU_SUCCESS;
    SNMP_NOTIFY_TABLE   *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (node != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Snmp_Notify_Table.flink;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == node)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->flink;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Snmp_Notify_Table, node);

            /* Deallocate the memory occupied by the node. */
            if(NU_Deallocate_Memory(node)!=NU_SUCCESS)
            {
                status = SNMP_BAD_POINTER;
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

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* SNMP_Remove_From_Notify_Table */

/***********************************************************************
*
*   Function
*
*       SNMP_Remove_From_Profile_Table
*
*   DESCRIPTION
*
*       Removes a node from one of the lists in the Notifications MIB.
*
*   INPUTS
*
*       *node           Node which will be removed.
*
*   OUTPUTS
*
*       NU_SUCCESS             Node successfully removed
*       SNMP_ERROR             The node was not found.
*       SNMP_INVALID_POINTER   Could not deallocate memory
*       SNMP_INVALID_PARAMETER A null pointer was passed in.
*
*************************************************************************/
STATUS SNMP_Remove_From_Profile_Table(SNMP_NOTIFY_FILTER_PROFILE_TABLE *node)
{
    STATUS                              status = NU_SUCCESS;
    SNMP_NOTIFY_FILTER_PROFILE_TABLE    *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (node != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Snmp_Profile_Table.flink;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == node)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->flink;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Snmp_Profile_Table, node);

            /* Deallocate the memory occupied by the node. */
            if(NU_Deallocate_Memory(node) != NU_SUCCESS)
            {
                status = SNMP_BAD_POINTER;
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

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* SNMP_Remove_From_Profile_Table */

/***********************************************************************
*
*   Function
*
*       SNMP_Remove_From_Filter_Table
*
*   DESCRIPTION
*
*       Removes a node from one of the lists in the Notifications MIB.
*
*   INPUTS
*
*       *node          Node which will be removed
*
*   OUTPUTS
*
*       NU_SUCCESS             Node successfully removed
*       SNMP_ERROR             The node was not found.
*       SNMP_INVALID_POINTER   Could not deallocate memory
*       SNMP_INVALID_PARAMETER A null pointer was passed in.
*
*************************************************************************/
STATUS SNMP_Remove_From_Filter_Table(SNMP_NOTIFY_FILTER_TABLE *node)
{
    STATUS                      status = NU_SUCCESS;
    SNMP_NOTIFY_FILTER_TABLE    *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */
    /* If we had valid pointer. */
    if (node != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Snmp_Filter_Table.flink;

        /* Traverse the list to find the entry. */
        while (dummy != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (dummy == node)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            dummy = dummy->flink;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Snmp_Filter_Table, node);

            /* Deallocate the memory occupied by the node. */
            if(NU_Deallocate_Memory(node)!= NU_SUCCESS)
            {
                status = SNMP_BAD_POINTER;
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

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* SNMP_Remove_From_Filter_Table */



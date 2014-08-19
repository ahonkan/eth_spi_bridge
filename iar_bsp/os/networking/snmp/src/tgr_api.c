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
*       trg_api.c
*
*   DESCRIPTION
*
*       This file contains API functions for the Target Address Table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SNMP_Find_Target
*       SNMP_Find_Params
*       SNMP_Remove_From_Tgr_Table
*       SNMP_Remove_From_Params_Table
*
*    DEPENDENCIES
*
*       tgr_api.h
*       snmp_no.h
*
*************************************************************************/

#include "networking/tgr_api.h"
#include "networking/snmp_no.h"

extern SNMP_TARGET_MIB                          Snmp_Target_Mib;

/***********************************************************************
*
*   Function
*
*       SNMP_Find_Target
*
*   DESCRIPTION
*
*       Find an entry in the target params table which matches the passed
*       target name.
*
*   INPUTS
*
*       *target_name   Name of the target.
*
*   OUTPUTS
*
*       Pointer to the target entry if one is found, NU_NULL otherwise.
*
*************************************************************************/
SNMP_TARGET_ADDRESS_TABLE* SNMP_Find_Target(const UINT8 *target_name)
{
    /* Handle to the target address table entry. */
    SNMP_TARGET_ADDRESS_TABLE       *target;

    /* Variable to hold comparison result. */
    INT                             cmp = 0;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we have valid parameters, get handle handle to the target
     * address table entry.
     */
    if (target_name)
    {
        /* Loop through all the target entries to find the entry. */
        target = Snmp_Target_Mib.target_addr_table.next;

        while(target != NU_NULL)
        {
            /* Check whether the target names of the current entry
             * matches, the target name passed or if the current entry
             * has a bigger target name. If it does, break out of the
             * loop.
             */
            if((cmp = strcmp(target->snmp_target_addr_name,
                             (CHAR *)target_name)) >= 0)
            {
                /* Break through the loop. */
                break;
            }

            /* This is not the entry we required, go to the next entry. */
            target = target->next;
        }


       /* If the name comparison had indicated that the current entry has
        * a greater target name we need to assign null to the returned
        * value. This is because we broke out of the loop when we had
        * reached a target which had a greater target name than what we
        * were looking. Since we have a list in ascending order, there
        * is no hope of finding an entry which matches.
        */
        if(cmp > 0)
        {
            target = NU_NULL;
        }
    }

    /* If we don't have valid parameters then return NU_NULL. */
    else
    {
        /* Return NU_NULL. */
        target = NU_NULL;
    }


    NU_USER_MODE();    /* return to user mode */

    /* Return handle to the target address table entry if found, otherwise
     * return error code.
     */
    return (target);

} /* SNMP_Find_Target */

/***********************************************************************
*
*   Function
*
*       SNMP_Find_Params
*
*   DESCRIPTION
*
*       Find an entry in the params table which matches the passed
*       name.
*
*   INPUTS
*
*       *params_name   Name of the params entry to be found.
*
*   OUTPUTS
*
*       Pointer to the entry if one is found, NU_NULL otherwise.
*
*************************************************************************/
SNMP_TARGET_PARAMS_TABLE* SNMP_Find_Params(const UINT8 *params_name)
{
    SNMP_TARGET_PARAMS_TABLE*    target;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (params_name != NU_NULL)
    {
        /* Make the actual call to find the target params entry. */
        target = SNMP_Find_Target_Params((CHAR *)params_name,
                                Snmp_Target_Mib.target_params_table.next);
    }
    
    else
    {
        target = NU_NULL;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (target);

} /* SNMP_Find_Params */

/***********************************************************************
*
*   Function
*
*       SNMP_Remove_From_Tgr_Table
*
*   DESCRIPTION
*
*       Removes a node from one of the lists in the Target/Params MIB.
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
STATUS SNMP_Remove_From_Tgr_Table(SNMP_TARGET_ADDRESS_TABLE *node)
{
    STATUS                      status = NU_SUCCESS;
    SNMP_TARGET_ADDRESS_TABLE   *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (node != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Snmp_Target_Mib.target_addr_table.next;

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
            dummy = dummy->next;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Snmp_Target_Mib.target_addr_table , node);

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

} /* SNMP_Remove_From_Tgr_Table */

/***********************************************************************
*
*   Function
*
*       SNMP_Remove_From_Params_Table
*
*   DESCRIPTION
*
*       Removes a node from one of the lists in the Target/Params MIB.
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
STATUS SNMP_Remove_From_Params_Table(SNMP_TARGET_PARAMS_TABLE *node)
{
    STATUS                      status = NU_SUCCESS;
    SNMP_TARGET_PARAMS_TABLE    *dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* If we had valid pointer. */
    if (node != NU_NULL)
    {
        /* Start traversing from start of the list. */
        dummy = Snmp_Target_Mib.target_params_table.next;

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
            dummy = dummy->next;
        }

        /* node found in the list. */
        if (dummy != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Snmp_Target_Mib.target_params_table , node);

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

} /* SNMP_Remove_From_Params_Table */




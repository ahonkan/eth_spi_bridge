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
*       cbsm_api.c                                               
*
*   DESCRIPTION
*
*       This file contains API functions. These functions are also
*       required by CBSM MIB.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       CBSM_Remove_Community
*       CBSM_Find_Community_Index
*
*   DEPENDENCIES
*
*       snmp.h
*       snmp_dis.h
*       cbsm.h
*
*************************************************************************/

#include "networking/snmp.h"
#include "networking/snmp_dis.h"
#include "networking/cbsm.h"

#if ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))

extern CBSM_COMMUNITY_TABLE             Cbsm_Mib_Root;

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Remove_Community
*
*   DESCRIPTION
*
*       This function removes a community from the table.
*
*   INPUTS
*
*       *community              Pointer to the community to be removed.
*
*   OUTPUTS
*
*       NU_SUCCESS              The community was successfully removed. 
*       SNMP_ERROR              The community was not found.
*       SNMP_BAD_PARAMETER      Invalid argument.
*
*************************************************************************/
STATUS CBSM_Remove_Community(CBSM_COMMUNITY_STRUCT *community)
{
    STATUS                  status = NU_SUCCESS;
    CBSM_COMMUNITY_STRUCT   *node;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (community)
    {
        /* Start traversing from start of the list. */
        node = Cbsm_Mib_Root.next;

        /* Traverse the list to find the entry. */
        while (node != NU_NULL)
        {
            /* If we found the entry then break through the loop. */
            if (node == community)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            node = node->next;
        }

        if (node != NU_NULL)
        {
            /* Remove the node from the table. */
            DLL_Remove(&Cbsm_Mib_Root, community);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

            /* Set the communities row Status to destroy so that
               the community is not added to the table if this call
               is made by the user. */
            community->cbsm_status = SNMP_ROW_DESTROY;

            /* Save the community table. */
            CBSM_Save_Community(community);
#endif

            /* Deallocate the memory occupied by the node. */
            if (NU_Deallocate_Memory(community) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory.",
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

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* CBSM_Remove_Community */

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Find_Community_Index
*
*   DESCRIPTION
*
*       This function gets an instance of Cbsm_Mib_Root.
*
*   INPUTS
*
*       *community_index        community's index
*       getflag                 flag for get or getnext/bulk
*
*
*   OUTPUTS
*
*       NU_SUCCESS              if particular instance is present in
*                               the table
*       SNMP_ERROR              if particular instance is not present
*                               in the table
*       SNMP_GENERROR           for failure
*
*************************************************************************/
CBSM_COMMUNITY_STRUCT *CBSM_Find_Community_Index(
              const UINT8 *community_index, UINT8 getflag, UINT16 *status)
{
    CBSM_COMMUNITY_STRUCT   *location_ptr;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ((community_index != NU_NULL) && (status != NU_NULL))
    {
        location_ptr = Cbsm_Mib_Root.next;
        *status = SNMP_ERROR;

        if (location_ptr)
        {
            switch (getflag)
            {
                /*next/bulk case*/
                case NU_FALSE:

                    while (location_ptr != NU_NULL)
                    {
                        if (strcmp((CHAR *)(location_ptr->
                                                cbsm_community_index),
                                   (CHAR *)community_index) > 0)
                        {
                            *status = NU_SUCCESS;
                            break;
                        }

                        location_ptr = location_ptr->next;
                    }
                    break;

                /*get case*/
                case NU_TRUE:

                    while (location_ptr != NU_NULL)
                    {
                        if (strcmp((CHAR *)(location_ptr->
                                                cbsm_community_index),
                                   (CHAR *)community_index) == 0)
                        {
                            *status = NU_SUCCESS;
                            break;
                        }

                        location_ptr = location_ptr->next;
                    }
                    break;

                default:

                    *status = SNMP_GENERROR;
                    location_ptr = NU_NULL;
            }
        }
    }

    else if (status != NU_NULL)
    {
        *status = SNMP_GENERROR;
        location_ptr = NU_NULL;
    }
    else
    {
        location_ptr = NU_NULL;
    }

    /* return to user mode */
    NU_USER_MODE();
    return (location_ptr);

} /* CBSM_Find_Community_Index */

#endif /* (INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE) */




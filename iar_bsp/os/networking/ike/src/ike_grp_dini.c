/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_grp_dini.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of the IKE Groups
*       de-initialization function.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Deinitialize_Groups
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/* External variables. */
extern IKE_POLICY_GROUP_DB IKE_Group_DB;

/************************************************************************
*
* FUNCTION
*
*       IKE_Deinitialize_Groups
*
* DESCRIPTION
*
*       This function de-initializes the IKE Groups Database.
*       It first deletes all policies in the group and then
*       de-allocates the group memory.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful de-initialization.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*
************************************************************************/
STATUS IKE_Deinitialize_Groups(VOID)
{
    STATUS              status;
    IKE_POLICY_GROUP    *group;
    IKE_POLICY_GROUP    *next_group;

    /* Log debug message. */
    IKE_DEBUG_LOG("De-initializing IKE groups");

    /* Un-register all interfaces from IKE groups because
     * all groups are being removed.
     */
    status = IKE_Empty_Group(NU_NULL);

    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to un-register interfaces from IKE groups",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    else
    {
        /* Start from the first group in the database. */
        group = IKE_Group_DB.ike_flink;

        /* Loop for all groups. */
        while(group != NU_NULL)
        {
            /* Save pointer to the next group. */
            next_group = group->ike_flink;

            /* Flush the group's SPDB. */
            status = IKE_Flush_Policies(&group->ike_policy_list);

            if(status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to flush group's SPDB",
                               NERR_SEVERE, __FILE__, __LINE__);
                break;
            }

            /* Deallocate the group memory. */
            if(NU_Deallocate_Memory(group) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate IKE memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Move to the next group. */
            group = next_group;
        }

        /* Set link pointers of the Group DB to NULL. */
        IKE_Group_DB.ike_flink = NU_NULL;
        IKE_Group_DB.ike_last  = NU_NULL;
    }

    /* Return the status. */
    return (status);

} /* IKE_Deinitialize_Groups */

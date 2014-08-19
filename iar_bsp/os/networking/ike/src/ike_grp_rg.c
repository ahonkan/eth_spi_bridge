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
*       ike_grp_rg.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Remove_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Remove_Group
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/* External variables. */
extern IKE_POLICY_GROUP_DB IKE_Group_DB;

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_Group
*
* DESCRIPTION
*
*       This function removes an IKE group from the Groups
*       database. All Policies and SAs belonging to the
*       specified Group are also removed.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Name of the group to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Group not found in database.
*
************************************************************************/
STATUS IKE_Remove_Group(CHAR *group_name)
{
    STATUS              status;
    IKE_POLICY_GROUP    *group_ptr;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Group name should not be NULL. */
    if(group_name == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get the group pointer. */
            status = IKE_Get_Group_Entry(group_name, &group_ptr);

            if(status == NU_SUCCESS)
            {
                /* Un-register all interfaces from the IKE group. */
                status = IKE_Empty_Group(group_ptr);

                if(status == NU_SUCCESS)
                {
                    /* Flush the group's SPDB. */
                    status = IKE_Flush_Policies(
                                 &group_ptr->ike_policy_list);

                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to flush group's SPDB",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log(
                        "Failed to remove interfaces from IKE group",
                        NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Remove the group node from the DB. */
                SLL_Remove(&IKE_Group_DB, group_ptr);
            }

            /* Release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            if(status == NU_SUCCESS)
            {
                /* Deallocate the group memory. */
                if(NU_Deallocate_Memory(group_ptr) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate IKE memory",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain IKE semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IKE_Remove_Group */

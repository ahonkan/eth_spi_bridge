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
*       ike_spdb_rp.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Remove_Policy.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Remove_Policy
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
************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_Policy
*
* DESCRIPTION
*
*       This function removes a policy from the IKE policy
*       database. The policy to be removed is identified
*       by the specified policy index. All SAs which have
*       been negotiated using the specified policy are also
*       removed.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Name of group from which to
*                               remove the policy.
*       index                   Index of the policy to be
*                               removed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters passed are invalid.
*       IKE_NOT_FOUND           The group was not found or
*                               there is no policy corresponding
*                               to the passed index.
*
************************************************************************/
STATUS IKE_Remove_Policy(CHAR *group_name, UINT32 index)
{
    STATUS              status;
    IKE_POLICY          *policy = NU_NULL;
    IKE_POLICY_GROUP    *group;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the group name pointer is valid. */
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
            /* Get the group. */
            status = IKE_Get_Group_Entry(group_name, &group);

            if(status == NU_SUCCESS)
            {
                /* Get the policy. */
                status = IKE_Get_Policy_Entry(group_name, &policy, index);

                if(status == NU_SUCCESS)
                {
                    /* Remove the policy from IKE SPDB. */
                    SLL_Remove(&group->ike_policy_list, policy);

                    /* Flush this policy's SADB. */
                    if(IKE_Flush_SAs(&policy->ike_sa_list) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to flush IKE SADB",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to find specified IKE policy",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to get specified IKE group",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* If policy successfully removed from the SPDB. */
            if(status == NU_SUCCESS)
            {
                /* Deallocate the policy memory. This need
                 * not be executed within the IKE semaphore.
                 */
                if(NU_Deallocate_Memory((VOID*)policy) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate the memory",
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

    /* Return the status. */
    return (status);

} /* IKE_Remove_Policy */

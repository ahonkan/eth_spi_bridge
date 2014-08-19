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
*       ike_spdb_gpi.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Get_Policy_Index.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Get_Policy_Index
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Policy_Index
*
* DESCRIPTION
*
*       This function searches the policy database for the
*       given selector. The selectors are matched logically
*       and not literally, by using the same procedure used
*       to match incoming packets to the policy selectors.
*       If a matching policy is found, its index is returned.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Name of the group to be searched.
*       *selector               Selector to be matched.
*       *return_index           On return, contains the index
*                               of the matching policy.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Specified selector does not
*                               match any of the policies in
*                               the database.
*
************************************************************************/
STATUS IKE_Get_Policy_Index(CHAR *group_name,
                            IKE_POLICY_SELECTOR *selector,
                            UINT32 *return_index)
{
    STATUS          status;
    IKE_POLICY      *policy;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure all pointers are valid. */
    if((group_name   == NU_NULL) || (selector == NU_NULL) ||
       (return_index == NU_NULL))
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Verify identifier type. Note that domain and user domain types
     * are not allowed in policies.
     */
    else if(
#if (INCLUDE_IPV4 == NU_TRUE)
            (selector->ike_type != IKE_IPV4)        &&
            (selector->ike_type != IKE_IPV4_SUBNET) &&
            (selector->ike_type != IKE_IPV4_RANGE)  &&
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            (selector->ike_type != IKE_IPV6)        &&
            (selector->ike_type != IKE_IPV6_SUBNET) &&
            (selector->ike_type != IKE_IPV6_RANGE)  &&
#endif
            (selector->ike_type != IKE_WILDCARD))
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get the policy by selector. */
            status = IKE_Get_Policy_By_Selector(group_name, selector,
                                                NU_NULL, &policy,
                                                IKE_MATCH_SELECTORS);

            if(status == NU_SUCCESS)
            {
                /* Return the index of the policy. */
                *return_index = policy->ike_index;
            }

            else
            {
                NLOG_Error_Log("Failed to get IKE policy by selector",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Release the semaphore. */
            if(NU_Release_Semaphore(&IKE_Data.ike_semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
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

} /* IKE_Get_Policy_Index */

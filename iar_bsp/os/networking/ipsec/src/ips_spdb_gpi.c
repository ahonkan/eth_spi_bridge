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
*       ips_spdb_gpi.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Get_Policy_Index.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Policy_Index
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*       ike_api.h
*       ike_evt.h
*
*************************************************************************/
/* Including the required header files. */
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)
#include "networking/ike_api.h"
#include "networking/ike_evt.h"
#endif

/************************************************************************
* FUNCTION
*
*       IPSEC_Get_Policy_Index
*
* DESCRIPTION
*
*       This function returns a policy index matching the passed
*       selector in the given group.
*
* INPUTS
*
*       *group_name             Name of the group.
*       *selector               Selector to be matched
*       selector_type           Type of selector IPSEC_INBOUND or
*                               IPSEC_OUTBOUND.
*       *return_index           Index to be found.
*
* OUTPUTS
*
*       NU_SUCCESS             In case of success.
*       NU_TIMEOUT             Timeout for obtaining semaphore.
*       IPSEC_NOT_FOUND        If a policy is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Policy_Index(CHAR *group_name, IPSEC_SELECTOR *selector,
                              UINT8 selector_type, UINT32 *return_index)
{
    STATUS              status;
    IPSEC_POLICY        *ret_policy_ptr;
    IPSEC_POLICY_GROUP  *group;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed first. */
    if((group_name == NU_NULL) || (selector == NU_NULL) ||
                                  (return_index == NU_NULL))
        status = IPSEC_INVALID_PARAMS;

    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status value. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* First get the group entry pointer. */
            status = IPSEC_Get_Group_Entry(group_name, &group);

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* Get the policy by selector. */
                status = IPSEC_Get_Policy_By_Selector(group, NU_NULL,
                                                selector, selector_type,
                                                &ret_policy_ptr);
                /* Check the status value. */
                if(status == NU_SUCCESS)
                {
                    /* Compare the policy found type and desired type. */
                    if(ret_policy_ptr->ipsec_flags & selector_type)
                    {
                        /* Now return the required index. */
                        *return_index = ret_policy_ptr->ipsec_index;
                    }
                    else
                    {
                        status = IPSEC_INVALID_PARAMS;
                    }
                }
            }

            /* Now everything is done, release the semaphore too. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release IPsec semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Policy_Index */

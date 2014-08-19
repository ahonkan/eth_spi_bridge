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
*       ips_spdb_rp.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Remove_Policy.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Remove_Policy
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
*
* FUNCTION
*
*       IPSEC_Remove_Policy
*
* DESCRIPTION
*
*       This function removes a policy from the list of policies.
*
* INPUTS
*
*       *group_name             Name of the group passed.
*       index                   Index of the policy to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS             The request was successfully executed.
*       NU_TIMEOUT             The operation timed out.
*       IPSEC_NOT_FOUND        The group was not found or there is no
*                              policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_Policy(CHAR *group_name, UINT32 index)
{
    STATUS              status;
    IPSEC_POLICY        *policy_ptr;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that the group name given is correct. */
    if(group_name == NU_NULL)
        status = IPSEC_INVALID_PARAMS;
    else
    {
        /* Now grab the IPsec semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* Get the group first. */
            status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* Get the policy. */
                status = IPSEC_Get_Policy_Entry(group_ptr, index,
                                                &policy_ptr);
                /* Check the status. */
                if(status == NU_SUCCESS)
                {
                    /* First grab the TCP semaphore. */
                    status = NU_Obtain_Semaphore(&TCP_Resource,
                                                 IPSEC_SEM_TIMEOUT);

                    /* Check the status. */
                    if(status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to obtain TCP semaphore",
                                       NERR_RECOVERABLE,
                                       __FILE__, __LINE__);
                    }
                    else
                    {

                        status = IPSEC_Remove_Policy_Real(group_ptr,
                                                          policy_ptr);

                        /* Release TCP semaphore also. */
                        if(NU_Release_Semaphore(&TCP_Resource) !=
                                                             NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to release the TCP \
                             semaphore", NERR_SEVERE, __FILE__, __LINE__);
                        }
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

} /* IPSEC_Remove_Policy */

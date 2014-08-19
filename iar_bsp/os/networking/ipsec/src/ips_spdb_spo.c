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
*       ips_spdb_spo.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Set_Policy_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Set_Policy_Opt
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
*       IPSEC_Set_Policy_Opt
*
* DESCRIPTION
*
*       This function sets a policy value as specified by optname.
*
* INPUTS
*
*       *group_name             Name of the group.
*       index                   Index of the policy.
*       optname                 Name of the options.
*       *optval                 Values to be set of optname.
*       optlen                  Length of the option value passed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_LENGTH_IS_SHORT   Required length is not available.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Set_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                            VOID *optval, INT optlen)
{
    STATUS              status;
    IPSEC_POLICY        *policy_ptr;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed. */
    if((group_name == NU_NULL) || (optval == NU_NULL))
    {
        /* Mark the status as bad. */
        status = IPSEC_INVALID_PARAMS;
    }
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
            status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* First get the policy entry. */
                status = IPSEC_Get_Policy_Entry(group_ptr, index,
                                                &policy_ptr);
                /* Check the status value. */
                if(status == NU_SUCCESS)
                {
                    /* Decode the request. */
                    switch(optname)
                    {
                    case IPSEC_SECURITY:
                    {
                       /* Validate the security protocol. */
                        status = IPSEC_Validate_Sec_Prot(
                                    (IPSEC_SECURITY_PROTOCOL *)optval);

                        if (status == NU_SUCCESS)
                        {

                            /* Make sure that there is room for the values
                             * to be set.
                             */
                            if(optlen !=
                                    (INT)(sizeof(IPSEC_SECURITY_PROTOCOL)
                                        *policy_ptr->ipsec_security_size))
                            {
                                /* Set the error code. */
                                status = IPSEC_INVALID_PARAMS;
                            }
                            else
                            {
                                /* Now copy the values passed to the
                                 * desired destination.
                                 */
                                NU_BLOCK_COPY(policy_ptr->ipsec_security,
                                              optval, optlen);
                            }
                        }
                    }

                    break;

/* Only if the IKE protocol is enabled. */
#if (INCLUDE_IKE == NU_TRUE)

                    case IPSEC_LIFETIME:
                    {
                        /* Validate the lifetime. */
                        if((policy_ptr->ipsec_sa_max_lifetime.
                               ipsec_expiry_action != IPSEC_REFRESH_SA) &&
                            (policy_ptr->ipsec_sa_max_lifetime.
                                ipsec_expiry_action != IPSEC_LOG_WARNING))
                        {
                            status = IPSEC_INVALID_PARAMS;
                        }
                        else
                        {
                            /* Make sure that there is room for the values
                               to be set. */
                            if(optlen != sizeof(IPSEC_SA_LIFETIME))
                            {
                                /* Set the error code. */
                                status = IPSEC_INVALID_PARAMS;
                            }
                            else
                            {
                                /* Now copy the values passed to the
                                   desired destination. */
                                NU_BLOCK_COPY(
                                    &(policy_ptr->ipsec_sa_max_lifetime),
                                    optval, optlen);
                            }
                        }
                    }
                    break;

                    case IPSEC_SA_BUNDLE_LIFETIME:
                    {
                        /* Make sure that there is room for the values to
                           be set. */
                        if(optlen != sizeof(policy_ptr->
                                            ipsec_bundles.ipsec_lifetime))
                        {
                            /* Set the error code. */
                            status = IPSEC_INVALID_PARAMS;
                        }
                        else
                        {
                            /* Now copy the values passed to the desired
                               destination. */
                            policy_ptr->ipsec_bundles.ipsec_lifetime =
                                                    *((UINT32*)optval);
                        }
                    }
                    break;

                    case IPSEC_PFS_GROUP_DESC:
                    {
                        /* Validate the PFS group. */
                        if((policy_ptr->ipsec_pfs_group_desc !=
                                                        IKE_GROUP_NONE) &&
                           (IKE_Oakley_Group_Prime(
                             policy_ptr->ipsec_pfs_group_desc) == NU_NULL))
                        {
                            status = IPSEC_INVALID_PARAMS;
                        }
                        else
                        {
                            /* Make sure that there is room for the values
                               to be set. */
                            if(optlen !=
                                sizeof(policy_ptr->ipsec_pfs_group_desc))
                            {
                                /* Set the error code. */
                                status = IPSEC_INVALID_PARAMS;
                            }
                            else
                            {
                                /* Now copy the values passed to the
                                   desired destination. */
                                policy_ptr->ipsec_pfs_group_desc =
                                                    *((UINT16*)optval);
                            }
                        }
                    }
                    break;
#endif

                    default:
                        /* Unable to decode the request. */
                        status = IPSEC_NOT_FOUND;
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

} /* IPSEC_Set_Policy_Opt */

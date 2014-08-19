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
*       ips_spdb_gpo.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Get_Policy_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Policy_Opt
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
*       IPSEC_Get_Policy_Opt
*
* DESCRIPTION
*
*       This function returns a policy's value as specified by optname.
*
* INPUTS
*
*       *group_name             Name of the group.
*       index                   Index of the policy.
*       optname                 Name of the options.
*       *optval                 Options which need to get.
*       *optlen                 Length of the option value passed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_LENGTH_IS_SHORT   Indicates that the value to be returned
*                               required more memory than was passed.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                            VOID *optval, INT *optlen)
{
    STATUS              status;
    INT                 req_len;
    IPSEC_POLICY        *policy_ptr;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed. */
    if(group_name == NU_NULL)
    {
        status = IPSEC_INVALID_PARAMS;
    }

    /* The optval and optlen parameters not required
     * for IPSEC_IS_POLICY, so check if otherwise.
     */
    else if((optname != IPSEC_IS_POLICY) &&
        ((optval  == NU_NULL) || (optlen == NU_NULL)))
    {
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
                    /* First decode the request. */
                    switch(optname)
                    {

                    case IPSEC_SECURITY:
                    {
                        /* Get the required length. */
                        req_len = (INT)(sizeof(IPSEC_SECURITY_PROTOCOL) *
                                        policy_ptr->ipsec_security_size);

                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < req_len)
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Place the desired values of this policy. */
                            NU_BLOCK_COPY(optval,
                                    policy_ptr->ipsec_security, req_len);
                        }

                        /* Return back the actual length. */
                        *optlen = req_len;
                    }

                    break;

                    case IPSEC_SELECT:
                    {
                        /* Get the required length. */
                        req_len = sizeof(IPSEC_SELECTOR);

                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < req_len)
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Place the desired values of this policy.*/
                            NU_BLOCK_COPY(optval,
                                    &policy_ptr->ipsec_select, req_len);
                        }

                        /* Return the size of the desired option. */
                        *optlen = req_len;
                    }

                    break;

                    case IPSEC_FLAGS:
                    {
                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < sizeof(policy_ptr->ipsec_flags))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now place the value of ipsec flags of this
                               policy. */
                            *((UINT8*)optval) = policy_ptr->ipsec_flags;
                        }

                        /* Return the size of values required. */
                        *optlen = sizeof(policy_ptr->ipsec_flags);
                    }

                    break;

                    case IPSEC_IS_POLICY:
                    {
                        /* If the control reaches here it means the given
                         * policy index is a valid one.
                         * Just break the case.
                         */
                        break;
                    }

                    case IPSEC_NEXT_POLICY:
                    {
                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < sizeof(policy_ptr->ipsec_index))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* If next policy is not null. */
                            if(policy_ptr->ipsec_flink != NU_NULL)
                            {
                                /* Now place the next policy index. */
                                *((UINT32 *)optval) =
                                    policy_ptr->ipsec_flink->ipsec_index;
                            }
                            else
                            {
                                /* Return the first policy index in the
                                   SPDB.*/
                                *((UINT32 *)optval) = group_ptr->
                                ipsec_policy_list.ipsec_head->ipsec_index;
                            }
                        }

                        /* Return the size of values required. */
                        *optlen = sizeof(policy_ptr->ipsec_index);
                    }
                    break;

#if (INCLUDE_IKE == NU_TRUE)

                    case IPSEC_LIFETIME:
                    {
                        /* Get the required length. */
                        req_len = sizeof(IPSEC_SA_LIFETIME);

                        /* Make sure that there is room for the values to
                        be returned. */
                        if(*optlen < req_len)
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Place the desired values of this policy.*/
                            NU_BLOCK_COPY(optval,
                                       &policy_ptr->ipsec_sa_max_lifetime,
                                        sizeof(IPSEC_SA_LIFETIME));
                        }

                        /* Return the size of values required. */
                        *optlen = req_len;
                    }
                    break;

                    case IPSEC_SA_BUNDLE_LIFETIME:
                    {
                        /* Get the required length. */
                        req_len = sizeof(policy_ptr->ipsec_bundles.
                                         ipsec_lifetime);

                        /* Make sure that there is room for the values to
                           be returned. */
                        if(*optlen < req_len)
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Place the desired values of this policy.*/
                            NU_BLOCK_COPY(optval,
                                &policy_ptr->ipsec_bundles.ipsec_lifetime,
                                          req_len);
                        }

                        /* Return the size of values required. */
                        *optlen = req_len;
                    }
                    break;

                    case IPSEC_PFS_GROUP_DESC:
                    {
                        /* Make sure that there is room for the values to
                           be returned. */
                        if((*optlen) <
                                sizeof(policy_ptr->ipsec_pfs_group_desc))
                        {
                            /* Set the error code. */
                            status = IPSEC_LENGTH_IS_SHORT;
                        }
                        else
                        {
                            /* Now place the value of ipsec flags of this
                               policy. */
                            *((UINT16*)optval) =
                                        policy_ptr->ipsec_pfs_group_desc;
                        }

                        /* Return the size of values required. */
                        *optlen =
                            sizeof(policy_ptr->ipsec_pfs_group_desc);
                    }
                    break;
#endif

                    default:
                        /* Invalid option. */
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

} /* IPSEC_Get_Policy_Opt */

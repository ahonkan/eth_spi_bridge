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
*       ike_spdb_gpo.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Get_Policy_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Get_Policy_Opt
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

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
#include "networking/ike2.h"
#endif

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Policy_Opt
*
* DESCRIPTION
*
*       This function returns a policy's attributes. The
*       attributes are specified in the 'optname' parameter. Valid
*       attributes for the 'optname' parameter are as follows:
*
*       Attribute           Type             Description
*       ----------------------------------------------------------
*       IKE_SELECTOR    IKE_POLICY_SELECTOR  Selector of policy.
*       IKE_IDS             IKE_IPS_ID[]     List of IDs.
*       IKE_PHASE1_XCHG     UINT8            Phase 1 exchange types.
*       IKE_PHASE2_XCHG     UINT8            Phase 2 exchange types.
*       IKE_XCHG1_ATTRIBS   IKE_ATTRIB[]     List Phase 1 attributes.
*       IKE_IS_POLICY       None             Success if policy exists.
*       IKE_NEXT_POLICY     UINT32           Index of next policy.
*       IKE_FLAGS           UINT8            Flags of the policy.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Name of the group to be searched.
*       index                   Index of the policy.
*       optname                 Name of the required attribute.
*       *optval                 Pointer to a buffer to store the
*                               attribute value. On return, this
*                               contains the value of the
*                               attribute.
*       *optlen                 Length of the attribute value
*                               buffer, in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters passed are invalid.
*       IKE_NOT_FOUND           The group was not found or
*                               there is no policy corresponding
*                               to the passed index.
*       IKE_LENGTH_IS_SHORT     Indicates that the value to be
*                               returned required more memory
*                               than was passed.
*
************************************************************************/
STATUS IKE_Get_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                          VOID *optval, INT *optlen)
{
    STATUS              status;
    IKE_POLICY          *policy;
    IKE_POLICY_GROUP    *group;
    INT                 reqlen;
    INT                 i;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that the group name given is correct. */
    if(group_name == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* The optval and optlen parameters not required
     * for IKE_IS_POLICY, so check if otherwise.
     */
    else if((optname != IKE_IS_POLICY) &&
            ((optval == NU_NULL) || (optlen == NU_NULL)))
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get the policy entry. */
            status = IKE_Get_Policy_Entry(group_name, &policy, index);

            if(status == NU_SUCCESS)
            {
                /* Decode the request. */
                switch(optname)
                {
                case IKE_SELECTOR:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(IKE_POLICY_SELECTOR);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Copy selector to the user buffer. */
                        NU_BLOCK_COPY(optval, &policy->ike_select, reqlen);
                    }
                    break;

                case IKE_IDS:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(IKE_IPS_ID) * policy->ike_ids_no;

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Copy IPsec IDs to user buffer. */
                        NU_BLOCK_COPY(optval, policy->ike_ids, reqlen);
                    }
                    break;

                case IKE_PHASE1_XCHG:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(UINT8);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Copy phase 1 exchange flags to user buffer. */
                        *(UINT8*)optval = policy->ike_phase1_xchg;
                    }
                    break;

                case IKE_PHASE2_XCHG:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(UINT8);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Copy phase 2 exchange flags to user buffer. */
                        *(UINT8*)optval = policy->ike_phase2_xchg;
                    }
                    break;

                case IKE_XCHG1_ATTRIBS:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(IKE_ATTRIB) *
                             policy->ike_xchg1_attribs_no;

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Copy phase 1 attributes to user buffer. */
                        NU_BLOCK_COPY(optval, policy->ike_xchg1_attribs,
                                      reqlen);

                        /* Loop for all attributes. */
                        for(i = 0; i < policy->ike_xchg1_attribs_no; i++)
                        {
                            /* Set remote key pointer to NULL. */
                            ((IKE_ATTRIB*)optval)->ike_remote_key_len = 0;
                            ((IKE_ATTRIB*)optval)->ike_remote_key     =
                                NU_NULL;

                            /* Move to next set of attributes. */
                            optval = ((IKE_ATTRIB*)optval) + 1;
                        }
                    }
                    break;

                case IKE_IS_POLICY:
                    /* If we've reached here, the policy must exist. */
                    reqlen = 0;
                    break;

                case IKE_NEXT_POLICY:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(UINT32);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Check if group has any policy entries. */
                        if(policy->ike_flink != NU_NULL)
                        {
                            /* Copy next policy's index to user buffer. */
                            *(UINT32*)optval =
                                policy->ike_flink->ike_index;
                        }

                        /* Otherwise, return index of first policy. */
                        else
                        {
                            /* Get group pointer. */
                            status = IKE_Get_Group_Entry(group_name,
                                                         &group);

                            if(status == NU_SUCCESS)
                            {
                                /* Copy index of first policy. */
                                *(UINT32*)optval =
                                    group->ike_policy_list.ike_flink->
                                        ike_index;
                            }

                            else
                            {
                                NLOG_Error_Log(
                                    "Failed to get specified IKE group",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }
                        }
                    }
                    break;

                case IKE_FLAGS:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(UINT8);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Copy policy flags to the user buffer. */
                        *(UINT8*)optval = policy->ike_flags;
                    }
                    break;

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                case IKE2_FLAGS_OPT:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(UINT8);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Now ensure if the correct IKE version is used */
                        if (policy->ike_version == IKE_VERSION_2)
                        {
                            /* Copy policy flags to the user buffer. */
                            *(UINT8*)optval = policy->ike2_flags;
                        }

                        else
                        {
                            /* Set the error code */
                            status = IKE_INVALID_VERSION;
                        }
                    }
                    break;

                case IKE2_SA_TIMEOUT_OPT:
                    /* Make sure there is room in optval */
                    reqlen = sizeof(UINT32);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Now ensure if the correct IKE version is used */
                        if (policy->ike_version == IKE_VERSION_2)
                        {
                            /* Copy SA timeout to the user buffer. */
                            *(UINT32*)optval = policy->ike2_sa_timeout;
                        }

                        else
                        {
                            /* Set the error code */
                            status = IKE_INVALID_VERSION;
                        }
                    }
                    break;

                case IKE2_VERSION_OPT:
                    /* Make sure there is room in optval. */
                    reqlen = sizeof(UINT8);

                    if(*optlen < reqlen)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Now ensure if the correct IKE version is used */
                        if (policy->ike_version == IKE_VERSION_2)
                        {
                            /* Copy IKE version to the user buffer. */
                            *(UINT8*)optval = policy->ike_version;
                        }

                        else
                        {
                            /* Set the error code */
                            status = IKE_INVALID_VERSION;
                        }
                    }
                    break;

#endif

                default:
                    /* Invalid option. */
                    status = IKE_INVALID_PARAMS;
                    reqlen = 0;
                    break;
                }

                /* Update the length if required. */
                if((status == NU_SUCCESS) ||
                   (status == IKE_LENGTH_IS_SHORT))
                {
                    if(optlen != NU_NULL)
                    {
                        *optlen = reqlen;
                    }
                }
            }

            else
            {
                NLOG_Error_Log("Failed to get specified IKE policy",
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

} /* IKE_Get_Policy_Opt */

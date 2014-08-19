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
*       ike_spdb_spo.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Set_Policy_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Set_Policy_Opt
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
*       IKE_Set_Policy_Opt
*
* DESCRIPTION
*
*       This function sets a policy value as specified by 'optname'.
*       Valid attributes for the 'optname' parameter are as follows:
*
*       Attribute           Type                Description
*       ----------------------------------------------------------
*       IKE_PHASE1_XCHG     UINT8               Phase 1 exchange types.
*       IKE_PHASE2_XCHG     UINT8               Phase 1 exchange types.
*       IKE_FLAGS           UINT8               Flags of the policy.
*       IKE2_FLAGS_OPT      UINT8               Flags used by IKEv2
*       IKE2_SA_TIMEOUT_OPT UINT8               Timeout for IKE SA when
*                                               IKEv2 is used
*
*       (Last two options are only available when IKEv2 is included
*        in the build.)
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Name of the group to be searched.
*       index                   Index of the required policy.
*       optname                 Name of the options.
*       *optval                 Values to be set of 'optname'.
*       optlen                  Exact length of the option being
*                               set.
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters passed are invalid.
*       IKE_INVALID_LENGTH      Option length is invalid.
*       IKE_NOT_FOUND           The group was not found or
*                               there is no policy corresponding
*                               to the passed index.
*
************************************************************************/
STATUS IKE_Set_Policy_Opt(CHAR *group_name, UINT32 index, INT optname,
                          VOID *optval, INT optlen)
{
    STATUS              status;
    IKE_POLICY          *policy;
    UINT8               xchg_flags;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure all parameters are valid. */
    if((group_name == NU_NULL) || (optval == NU_NULL) ||
       (optlen     == 0))
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
                case IKE_PHASE1_XCHG:
                    /* Make sure option length is valid. */
                    if(optlen != sizeof(UINT8))
                    {
                        /* Set the error code. */
                        status = IKE_INVALID_LENGTH;
                    }

                    else
                    {
                        /* Copy option value to local flags variable. */
                        xchg_flags = *(UINT8*)optval;

                        /* Make sure required flags are set. */
                        if((
#if (IKE_INCLUDE_MAIN_MODE == NU_TRUE)
                            (xchg_flags & IKE_XCHG_MAIN_FLAG)
#endif
#if ((IKE_INCLUDE_AGGR_MODE == NU_TRUE) && \
     (IKE_INCLUDE_MAIN_MODE == NU_TRUE))
                            |
#endif
#if (IKE_INCLUDE_AGGR_MODE == NU_TRUE)
                            (xchg_flags & IKE_XCHG_AGGR_FLAG)
#endif
                           ) == 0)
                        {
                            status = IKE_INVALID_PARAMS;
                        }

                        else
                        {
                            /* Update exchange flags in the policy. */
                            policy->ike_phase1_xchg = xchg_flags;
                        }
                    }
                    break;

                case IKE_PHASE2_XCHG:
                    /* Make sure option length is valid. */
                    if(optlen != sizeof(UINT8))
                    {
                        /* Set the error code. */
                        status = IKE_INVALID_LENGTH;
                    }

                    else
                    {
                        /* Copy option value to local flags variable. */
                        xchg_flags = *(UINT8*)optval;

                        /* Make sure required flags are set. */
                        if((xchg_flags & IKE_XCHG_QUICK_FLAG) == 0)
                        {
                            status = IKE_INVALID_PARAMS;
                        }

                        else
                        {
                            /* Update exchange flags in the policy. */
                            policy->ike_phase2_xchg = xchg_flags;
                        }
                    }
                    break;

                case IKE_FLAGS:
                    /* Make sure option length is valid. */
                    if(optlen != sizeof(UINT8))
                    {
                        /* Set the error code. */
                        status = IKE_INVALID_LENGTH;
                    }

                    else
                    {
                        /* Set flags in the policy. */
                        policy->ike_flags = *(UINT8*)optval;
                    }
                    break;

#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                case IKE2_FLAGS_OPT:
                    /* Make sure option length is valid. */
                    if(optlen != sizeof(UINT8))
                    {
                        /* Set the error code. */
                        status = IKE_INVALID_LENGTH;
                    }

                    else
                    {
                        /* Now ensure if the correct IKE version is used */
                        if (policy->ike_version == IKE_VERSION_2)
                        {
                           /* Set flags in the policy. */
                           policy->ike2_flags = *(UINT8*)optval;
                        }

                        else
                        {
                           /* Set the error code */
                            status = IKE_INVALID_VERSION;
                        }
                    }
                    break;

                case IKE2_SA_TIMEOUT_OPT:
                    /* Make sure option length is valid. */
                    if(optlen != sizeof(UINT32))
                    {
                        /* Set the error code. */
                        status = IKE_INVALID_LENGTH;
                    }

                    else
                    {
                        /* Now ensure if the correct IKE version is used */
                        if (policy->ike_version == IKE_VERSION_2)
                        {
                           /* Set flags in the policy. */
                           policy->ike2_sa_timeout = *(UINT32*)optval;
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
                    /* Unable to decode the option name. */
                    status = IKE_INVALID_PARAMS;
                    break;
                }
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

} /* IKE_Set_Policy_Opt */

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
*       ike_spdb_fs.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Flush_Policies.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Flush_Policies
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
*       IKE_Flush_Policies
*
* DESCRIPTION
*
*       This is an internal function which is called when
*       IKE is shutting down. It removes all policies from
*       the specified IKE Group. All SAs in each policy's
*       SADB are also removed. The caller is responsible
*       for obtaining the IKE semaphore before calling this
*       function.
*
* INPUTS
*
*       *spdb                   Policy database to be flushed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*
************************************************************************/
STATUS IKE_Flush_Policies(IKE_SPDB *spdb)
{
    IKE_POLICY      *policy;
    IKE_POLICY      *next_policy;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure that the SPDB pointer is valid. */
    if(spdb == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Start from the first policy. */
    policy = spdb->ike_flink;

    /* Loop for all policies in the database. */
    while(policy != NU_NULL)
    {
        /* Store pointer to the next policy. */
        next_policy = policy->ike_flink;

        /* Flush this policy's SADB. */
        if(IKE_Flush_SAs(&policy->ike_sa_list) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to flush IKE SADB",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Deallocate the policy memory. All dynamic fields of the
         * policy are allocated in this single memory block.
         */
        if(NU_Deallocate_Memory(policy) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate IKE memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Move to the next policy in the list. */
        policy = next_policy;
    }

    /* Set SPDB pointers to NULL. */
    spdb->ike_flink = NU_NULL;
    spdb->ike_last  = NU_NULL;

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Flush_Policies */

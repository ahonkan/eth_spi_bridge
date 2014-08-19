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
*       ips_sadb_ros.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Remove_Outbound_SA.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Remove_Outbound_SA
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
/* Including the required header files. */
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Remove_Outbound_SA
*
* DESCRIPTION
*
*       This function removes outbound SAs as specified by the index.
*
* INPUTS
*
*       *index                  Identifier to a unique outbound SA that
*                               has to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The SA was not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_Outbound_SA(IPSEC_OUTBOUND_INDEX *index)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group_ptr;
    IPSEC_OUTBOUND_SA   *sa_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the index pointer is not null. */
    if((index == NU_NULL) || (index->ipsec_group == NU_NULL) ||
                             (index->ipsec_index == 0))
    {
        /* Some invalid parameter has been passed. */
        status = IPSEC_INVALID_PARAMS;
    }
    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to obtain IPsec semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            /* First get the group pointer. */
            status = IPSEC_Get_Group_Entry(index->ipsec_group, &group_ptr);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* First get the required SA. */
                status = IPSEC_Get_Outbound_SA_By_Index(
                                                    index->ipsec_index,
                        group_ptr->ipsec_outbound_sa_list.ipsec_head,
                                                        &sa_ptr);
                /* Check the status. */
                if(status == NU_SUCCESS)
                {
#if (INCLUDE_IKE == NU_TRUE)

                    /* Make sure only those SAs can be removed
                     * which have been added by user not by IKE.
                     */
                    if(sa_ptr->ipsec_soft_lifetime == 0)
#endif
                    {
                        /* Remove the specified SA. */
                        SLL_Remove(&(group_ptr->ipsec_outbound_sa_list),
                                   sa_ptr);

                        /* Free the outbound SA. */
                        IPSEC_Free_Outbound_SA(sa_ptr);
                    }
                }
            }

            /* Now everything is done, release the semaphore too. */
            if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release the semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Remove_Outbound_SA */

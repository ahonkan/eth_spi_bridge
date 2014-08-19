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
*       ips_sadb_ris.c
*
* COMPONENT
*
*       DATABASE - Security Associations
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Remove_Inbound_SA.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Remove_Inbound_SA
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
* FUNCTION
*
*       IPSEC_Remove_Inbound_SA
*
* DESCRIPTION
*
*       This function removes inbound SAs as specified by the index.
*
* INPUTS
*
*       *group_name             Name of the desired group
*       *index                  Index of the SA to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              The operation timed out.
*       IPSEC_NOT_FOUND         The group was not found or there is no
*                               policy corresponding to the passed index.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_Inbound_SA(CHAR *group_name,
                               IPSEC_INBOUND_INDEX *index)
{
    STATUS              status;
    IPSEC_INBOUND_SA    *sa_ptr;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure of the parameters passed. */
    if((index == NU_NULL) || (group_name == NU_NULL) ||
                             (index->ipsec_spi == 0))
    {
        /* Some invalid parameter has been passed. */
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
            /* First get the group entry. */
            status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

            /* Check the status value. */
            if(status == NU_SUCCESS)
            {
                /* Find the SA entry from the index. */
                status = IPSEC_Get_Inbound_SA(group_ptr, index, &sa_ptr);

                /* Check the status. */
                if(status == NU_SUCCESS)
                {
#if (INCLUDE_IKE == NU_TRUE)
                    /* Make sure only that SA can be removed which
                     * has been added by the user and not by IKE.
                     */
                    if((sa_ptr->ipsec_soft_lifetime.ipsec_flags &
                                                    IPSEC_IKE_SA) == 0)
                    {
#endif
                        /* Remove the SA from the list. */
                        SLL_Remove(&(group_ptr->ipsec_inbound_sa_list),
                                    sa_ptr);

                        /* Free the inbound SA. */
                        IPSEC_Free_Inbound_SA(sa_ptr);

#if (INCLUDE_IKE == NU_TRUE)
                    }
#endif
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

} /* IPSEC_Remove_Inbound_SA */

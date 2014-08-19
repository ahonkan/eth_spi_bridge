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
*       ips_grp_sgo.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Set_Group_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Set_Group_Opt
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*       ips_api.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Set_Group_Opt
*
* DESCRIPTION
*
*       This function sets an IPsec Group value as specified.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       optname                 Name of the options.
*       *optval                 Value of the required options
*       optlen                  Length of the options given.
*
* OUTPUTS
*
*       NU_SUCCESS              The request was successfully executed.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       IPSEC_NOT_FOUND         The group or option was not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Set_Group_Opt(CHAR *group_name, INT optname,
                           VOID *optval, INT optlen)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the parameters passed. */
    if((group_name == NU_NULL) || (optval == NU_NULL) || (optlen == 0))
    {
        status = IPSEC_INVALID_PARAMS;
    }
    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* First get the group entry. */
            status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* Decode the request. */
                switch(optname)
                {
                case IPSEC_DF_PROCESSING:
                    if(optlen < sizeof(group_ptr->ipsec_df_processing))
                    {
                        /* Required length is not available. */
                        status = IPSEC_LENGTH_IS_SHORT;
                    }
                    else
                    {
                        /* Now copy the values passed to the desired
                           destination. */
                        group_ptr->ipsec_df_processing =
                                                        *((UINT8*)optval);
                    }

                    break;

                default:
                    /* Unable to decode the request. */
                    status = IPSEC_INVALID_PARAMS;
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

} /* IPSEC_Set_Group_Opt. */


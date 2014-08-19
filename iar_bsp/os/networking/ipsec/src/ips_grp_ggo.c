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
*       ips_grp_ggo.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Get_Group_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Group_Opt
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
*       IPSEC_Get_Group_Opt
*
* DESCRIPTION
*
*       This function returns an IPsec Group value as specified.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       optname                 Name of the required option.
*       *optval                 Value of the required option, to be
*                               returned.
*       *optlen                 Total memory length in bytes pointed by
*                               optval.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       IPSEC_NOT_FOUND         Group or option not found.
*       IPSEC_LENGTH_IS_SHORT   Supplied option space is less then
*                               required.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Group_Opt(CHAR *group_name, INT optname,
                           VOID *optval, INT *optlen)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that the parameters passed are not null. */
    if(group_name == NU_NULL)
        status = IPSEC_INVALID_PARAMS;

    else if((optname != IPSEC_IS_GROUP) && ((optval == NU_NULL) ||
        (optlen == NU_NULL)))
    {
        /* Passed parameter is incorrect. */
        status = IPSEC_INVALID_PARAMS;
    }

    else
    {
        /* First grab the semaphore. */
        status = NU_Obtain_Semaphore(&IPSEC_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* First decode the request. */
            switch(optname)
            {
            case IPSEC_IS_GROUP:
            {
                /* Just get the group entry, status value will show
                   whether it's a group or not. */
                status = IPSEC_Get_Group_Entry(group_name, &group_ptr);
            }

            break;

            case IPSEC_NEXT_GROUP:
            {
                /* Get the current group entry. */
                status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

                /* If we get the group entry. */
                if(status == NU_SUCCESS)
                {
                    /* Make sure that there is room for the values
                       to be returned. */
                    if(*optlen <
                           (INT)strlen(group_ptr->ipsec_group_name) + 1)
                    {
                        /* Set the error code. */
                        status = IPSEC_LENGTH_IS_SHORT;
                    }
                    else
                    {
                        /* If there is a next group. */
                        if(group_ptr->ipsec_flink != NU_NULL)
                        {
                            group_ptr = group_ptr->ipsec_flink;
                        }
                        /* Otherwise, set pointer to the first group. */
                        else
                        {
                            group_ptr = IPS_Group_DB.head;
                        }

                        /* Now place the desired values of this group.*/
                        strcpy((CHAR*)optval,
                               group_ptr->ipsec_group_name);
                    }

                    /* Return the size of values copied also. */
                    *optlen = (strlen(group_ptr->ipsec_group_name) + 1);
                }
                else
                {
                    /* Unable to get the group. */
                    *optlen = 0;
                }
            }

            break;

            case IPSEC_DF_PROCESSING:
            {
                /* Make sure that there is room for the values to
                   be returned. */
                if(*optlen < sizeof(group_ptr->ipsec_df_processing))
                {
                    /* Set the error code. */
                    status = IPSEC_LENGTH_IS_SHORT;
                }
                else
                {
                    /* First get the group entry. */
                    status = IPSEC_Get_Group_Entry(group_name, &group_ptr);

                    /* Check the status. */
                    if(status == NU_SUCCESS)
                    {
                        /* Now place the desired values of this group.*/
                        *((UINT8*)optval) =
                                    group_ptr->ipsec_df_processing;
                    }

                    /* Return the size of values copied also. */
                    *optlen = sizeof(group_ptr->ipsec_df_processing);
                }
            }

            break;

            default:

                /* Invalid option. */
                status = IPSEC_INVALID_PARAMS;
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

} /* IPSEC_Get_Group_Opt. */

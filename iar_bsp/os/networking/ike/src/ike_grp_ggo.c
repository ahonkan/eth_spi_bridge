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
*       ike_grp_ggo.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Get_Group_Opt.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Get_Group_Opt
*
* DEPENDENCIES
*
*       nu_net.h
*       sll.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/sll.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/* External variables. */
extern IKE_POLICY_GROUP_DB IKE_Group_DB;

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Group_Opt
*
* DESCRIPTION
*
*       This function returns the specified IKE Group attribute.
*       Supported attributes are as follows:
*
*       Attribute           Type      Description
*       ---------------------------------------------------------
*       IKE_TOTAL_POLICIES  UINT32    Total number of policies.
*       IKE_IS_GROUP        None      Returns success if group exists.
*       IKE_NEXT_GROUP      CHAR[]    Name of group returned in the
*                                     specified buffer. Buffer should
*                                     be large enough and its size
*                                     must be specified in 'optlen'.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       optname                 Name of the required option.
*       *optval                 Pointer to a buffer which would
*                               store the value of the required
*                               attribute.
*       *optlen                 Length of the optval buffer.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Group or option not found.
*       IKE_LENGTH_IS_SHORT     Supplied option buffer length
*                               is less then required.
*
************************************************************************/
STATUS IKE_Get_Group_Opt(CHAR *group_name, INT optname,
                         VOID *optval, INT *optlen)
{
    STATUS              status;
    IKE_POLICY_GROUP    *group_ptr;
    IKE_POLICY_GROUP    *req_group;
    INT                 req_len;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that the group name given is correct. */
    if(group_name == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* 'optlen' and 'optval' can be NULL if 'optname' if equal to
     * IKE_IS_GROUP.
     */
    else if((optname != IKE_IS_GROUP) &&
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
            /* Get the group entry. */
            status = IKE_Get_Group_Entry(group_name, &group_ptr);

            if(status == NU_SUCCESS)
            {
                /* Determine the requested attribute. */
                switch(optname)
                {
                case IKE_TOTAL_POLICIES:
                    /* Make sure there is room for the
                     * values to be returned.
                     */
                    if(*optlen < (INT)sizeof(UINT32))
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Save the value of this attribute. */
                        *((UINT32*)optval) =
                            SLL_Length(&(group_ptr->ike_policy_list));

                        /* Return the size of value copied. */
                        *optlen = sizeof(UINT32);
                    }

                    break;

                case IKE_IS_GROUP:
                    /* Group exists if control reaches here.
                     * 'status' is already NU_SUCCESS.
                     */
                    break;

                case IKE_NEXT_GROUP:
                    /* If next group is present. */
                    if(group_ptr->ike_flink != NU_NULL)
                    {
                        req_group = group_ptr->ike_flink;
                    }

                    else
                    {
                        req_group = IKE_Group_DB.ike_flink;
                    }

                    /* Make sure there is room for the
                     * values to be returned.
                     */
                    req_len = (INT)strlen(req_group->ike_group_name) + 1;

                    if(*optlen < req_len)
                    {
                        /* Set the error code. */
                        status = IKE_LENGTH_IS_SHORT;
                    }

                    else
                    {
                        /* Set the value of this attribute. */
                        strcpy((CHAR*)optval, req_group->ike_group_name);
                    }

                    /* Return the required length, even if group not
                     * found, to indicate the amount of space required.
                     */
                    *optlen = req_len;

                    break;

                default:
                    /* Invalid option. */
                    status = IKE_INVALID_PARAMS;
                    break;
                }
            }

            else
            {
                NLOG_Error_Log("Unable to find IKE group",
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

    /* Return the status value. */
    return (status);

} /* IKE_Get_Group_Opt */

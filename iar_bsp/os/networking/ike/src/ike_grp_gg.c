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
*       ike_grp_gg.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Get_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Get_Group
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Group
*
* DESCRIPTION
*
*       Returns the group name to which the specified interface
*       belongs.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *interface_name         Name of the interface being searched.
*       *return_group           Pointer to a buffer which would
*                               contain the group name on return.
*       *total_len              Length of the buffer for storing
*                               the returned group name, including
*                               the NULL terminator.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters not valid.
*       IKE_NOT_FOUND           Group or interface not found.
*       IKE_LENGTH_IS_SHORT     return_group buffer length too short.
*
************************************************************************/
STATUS IKE_Get_Group(CHAR *interface_name, CHAR *return_group,
                     UINT32 *total_len)
{
    STATUS              status;
    UINT32              req_len;
    DV_DEVICE_ENTRY     *dev_entry;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure all pointers are valid. */
    if((return_group == NU_NULL) || (interface_name == NU_NULL) ||
       (total_len    == NU_NULL))
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Grab the NET semaphore. */
            status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

            if(status == NU_SUCCESS)
            {
                /* Get the device entry. */
                dev_entry = DEV_Get_Dev_By_Name(interface_name);

                if(dev_entry != NU_NULL)
                {
                    /* Make sure this interface is registered
                     * with a group.
                     */
                    if(dev_entry->dev_physical->dev_phy_ike_group !=
                       NU_NULL)
                    {
                        /* Make sure that we have the room in the
                         * given group's name buffer.
                         */
                        req_len =
                            strlen(((IKE_POLICY_GROUP*)dev_entry->
                                    dev_physical->dev_phy_ike_group)->
                                        ike_group_name) + 1;

                        if((*total_len) < req_len)
                        {
                            /* Buffer length is not sufficient. */
                            status = IKE_LENGTH_IS_SHORT;
                        }

                        else
                        {
                            /* Copy the required group name. */
                            strcpy(return_group,
                                ((IKE_POLICY_GROUP*)dev_entry->
                                    dev_physical->dev_phy_ike_group)->
                                        ike_group_name);
                        }

                        /* Return the group name length. */
                        *total_len = req_len;
                    }

                    else
                    {
                        /* There is no group registered. */
                        status = IKE_NOT_FOUND;
                    }
                }

                else
                {
                    /* Failed to find the required interface. */
                    status = IKE_NOT_FOUND;
                }

                /* Release the NET semaphore. */
                if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release the semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                NLOG_Error_Log("Failed to obtain NET semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Release the IKE semaphore. */
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

} /* IKE_Get_Group */

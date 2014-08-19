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
*       ike_grp_rfg.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Remove_From_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Remove_From_Group
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*       ike_cfg.h
*       ike.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ips_api.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Remove_From_Group
*
* DESCRIPTION
*
*       This function removes an interface from an IKE group.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *interface_name         Name of the interface which is to
*                               be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Group or interface not found.
*
************************************************************************/
STATUS IKE_Remove_From_Group(CHAR *interface_name)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_entry;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that the interface name given is correct. */
    if(interface_name == NU_NULL)
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
                    /* Make sure that interface is registered with
                     * a policy group.
                     */
                    if(dev_entry->dev_physical->dev_phy_ike_group !=
                       NU_NULL)
                    {
                        /* Un-register the interface from the group. */
                        dev_entry->dev_physical->dev_phy_ike_group =
                            NU_NULL;
                    }

                    else
                    {
                        /* No group is registered with this interface. */
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

} /* IKE_Remove_From_Group */

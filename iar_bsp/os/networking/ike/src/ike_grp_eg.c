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
*       ike_grp_eg.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file contains the implementation of IKE_Empty_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Empty_Group
*
* DEPENDENCIES
*
*       nu_net.h
*       ike_cfg.h
*       ike.h
*       ike_db.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_db.h"

/************************************************************************
*
* FUNCTION
*
*       IKE_Empty_Group
*
* DESCRIPTION
*
*       This function removes interfaces from the specified
*       IKE group which they are a part of. If no group is
*       specified, then all IKE groups are emptied. This is
*       a utility function used internally by IKE.
*
* INPUTS
*
*       *group                  Pointer to group which is to
*                               be emptied. If this is NULL,
*                               then all IKE groups are emptied.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*
************************************************************************/
STATUS IKE_Empty_Group(IKE_POLICY_GROUP *group)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_entry;

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Start from the first device in the table. */
        dev_entry = DEV_Table.dv_head;

        /* Loop for all devices. */
        while(dev_entry != NU_NULL)
        {
            /* If interface is registered with an IKE group. */
            if(dev_entry->dev_physical->dev_phy_ike_group != NU_NULL)
            {
                /* If no specific group specified, or specified
                 * group is found.
                 */
                if((group == NU_NULL) ||
                   (dev_entry->dev_physical->dev_phy_ike_group == group))
                {
                    /* Un-register the interface from the group. */
                    dev_entry->dev_physical->dev_phy_ike_group = NU_NULL;
                }
            }

            /* Move to the next device. */
            dev_entry = dev_entry->dev_next;
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

    /* Return the status value. */
    return (status);

} /* IKE_Empty_Group */

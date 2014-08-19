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
*       ips_grp_rfg.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Remove_From_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Remove_From_Group.
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
*       IPSEC_Remove_From_Group
*
* DESCRIPTION
*
*       This function removes an interface from an IPsec group.
*
* INPUTS
*
*       *interface_name         Interface name which needs to be removed
*                               from IPsec group.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       IPSEC_NOT_FOUND         If required instance is not found.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Remove_From_Group(CHAR *interface_name)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_entry;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that the interface name given is correct. */
    if(interface_name == NU_NULL)
    {
        status = IPSEC_INVALID_PARAMS;
    }
    else
    {
        /* First grab the TCP semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

        /* Check the status. */
        if(status == NU_SUCCESS)
        {
            /* Get device by interface name. */
            dev_entry = DEV_Get_Dev_By_Name(interface_name);

            /* Make sure if got the device entry or not. */
            if(dev_entry == NU_NULL)
            {
                /* Mark the status as not found. */
                status = IPSEC_NOT_FOUND;
            }

            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* First grab the semaphore. */
                status = NU_Obtain_Semaphore(&IPSEC_Resource,
                                             IPSEC_SEM_TIMEOUT);

                /* Check the status. */
                if(status == NU_SUCCESS)
                {
                    /* First make sure that interface is register with
                       some policy group. */
                    if(dev_entry->dev_physical->dev_phy_ips_group !=
                                                                NU_NULL)
                    {
                        /* Unregister the interface from the group. */
                        dev_entry->dev_physical->dev_phy_ips_group =
                                                                NU_NULL;
                    }
                    else
                    {
                        /* No group is registered with this interface. */
                        status = IPSEC_NOT_FOUND;
                    }

                    /* Now everything is done, release the semaphore. */
                    if(NU_Release_Semaphore(&IPSEC_Resource)
                                                        != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release IPsec semaphore",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
                else
                {
                    NLOG_Error_Log("Failed to obtain the IPsec semaphore",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* Release the TCP semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release TCP semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
        else
        {
            NLOG_Error_Log("Failed to obtain the TCP semaphore",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Remove_From_Group */

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
*       ips_grp_atg.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Add_To_Group.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Add_To_Group
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
*       IPSEC_Add_To_Group
*
* DESCRIPTION
*
*       Registers an interface with the passed group.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       *interface_name         Interface name to be added
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       IPSEC_NOT_FOUND         Interface not found.
*       IPSEC_ALREADY_EXIST     Interface is already registered with
*                               other group.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Add_To_Group(CHAR *group_name, CHAR *interface_name)
{
    STATUS              status;
    IPSEC_POLICY_GROUP  *group;
    DV_DEVICE_ENTRY     *dev_entry;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_name == NU_NULL) || (interface_name == NU_NULL))
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First grab the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

    /* Check the status value. */
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain TCP semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    else
    {
        /* Get device by interface name. */
        dev_entry = DEV_Get_Dev_By_Name(interface_name);

        /* Make sure if got the device entry or not. */
        if(dev_entry == NU_NULL)
        {
            /* Mark the status as not found. */
            status = IPSEC_NOT_FOUND;
        }
        else
        {
            /* First grab the semaphore. */
            status = NU_Obtain_Semaphore(&IPSEC_Resource,
                                         IPSEC_SEM_TIMEOUT);
            /* Check the status. */
            if(status == NU_SUCCESS)
            {
                /* Get the group pointer first. */
                status = IPSEC_Get_Group_Entry(group_name,
                                               &group);
                /* Check the status. */
                if(status == NU_SUCCESS)
                {
                    /* Make sure that the group is not empty. */
                    if(dev_entry->dev_physical->dev_phy_ips_group
                                                    == NU_NULL)
                    {
                        /* Now set the policy group pointer of
                         * this interface with the policy group
                         * found.
                         */
                        dev_entry->dev_physical->
                                    dev_phy_ips_group = group;
                    }
                    else
                    {
                        /* Device is already registered with some
                           IPsec group. */
                        status = IPSEC_ALREADY_EXISTS;
                    }
                }

                /* Everything is done, release the semaphore. */
                if(NU_Release_Semaphore(&IPSEC_Resource)
                                                    != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release IPsecsemaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        /* Release the TCP semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the TCP semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Add_To_Group. */

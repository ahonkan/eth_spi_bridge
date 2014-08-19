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
*       ips_grp_gg.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Get_Group
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Get_Group
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
*       IPSEC_Get_Group
*
* DESCRIPTION
*
*       Returns the group name for the passed interface.
*
* INPUTS
*
*       *interface_name         Name of the interface.
*       *return_group           The required group name.
*       *total_len              The maximum size string that can be
*                               placed in return_group.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       NU_TIMEOUT              Timeout before semaphore is obtained.
*       IPSEC_NOT_FOUND         Group not found.
*       IPSEC_LENGTH_IS_SHORT   Given length is short.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
STATUS IPSEC_Get_Group(CHAR *interface_name, CHAR *return_group,
                       UINT32 *total_len)
{
    STATUS              status;
    UINT32              req_len;
    DV_DEVICE_ENTRY     *dev_entry;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((interface_name == NU_NULL) || (return_group == NU_NULL) ||
       (total_len      == NU_NULL) || (*total_len   == NU_NULL))
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* First grab the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IPSEC_SEM_TIMEOUT);

    /* Check the status. */
    if(status == NU_SUCCESS)
    {
        /* Get device by interface name. */
        dev_entry = DEV_Get_Dev_By_Name(interface_name);

        /* Make sure if we got the device entry or not. */
        if(dev_entry == NU_NULL)
        {
            /* Failed to find the required interface. */
            status = IPSEC_NOT_FOUND;
        }
        else
        {
            /* First grab the semaphore. */
            status = NU_Obtain_Semaphore(&IPSEC_Resource,
                                          IPSEC_SEM_TIMEOUT);
            /* Check the status . */
            if(status == NU_SUCCESS)
            {
                /* Make sure that this interface is registered with
                   some group which we are going to return. */
                if(dev_entry->dev_physical->dev_phy_ips_group !=
                                                            NU_NULL)
                {
                    /* Make sure that we have the room in the return
                       group name memory. */
                    req_len = strlen(dev_entry->dev_physical->
                        dev_phy_ips_group->ipsec_group_name) + 1;

                    if(*total_len < req_len)
                    {
                        /* No, the length is short. */
                        status = IPSEC_LENGTH_IS_SHORT;
                    }
                    else
                    {
                        /* Now copy the group name required. */
                        strcpy(return_group, dev_entry->dev_physical->
                               dev_phy_ips_group->ipsec_group_name);
                    }

                    /* Return the group name length. */
                    *total_len = req_len;
                }
                else
                {
                    /* Failed to find the group name. */
                    status = IPSEC_NOT_FOUND;
                }

                /* Now everything is done, release the semaphore. */
                if(NU_Release_Semaphore(&IPSEC_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release the semaphore",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
            else
            {
                NLOG_Error_Log("Failed to obtain IPsec semaphore",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Release the semaphore now. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release TCP semaphore",
                            NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to obtain TCP semaphore",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Group. */

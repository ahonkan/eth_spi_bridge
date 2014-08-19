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
*       ike_grp.c
*
* COMPONENT
*
*       IKE - Database
*
* DESCRIPTION
*
*       This file implements the IKE Groups database.
*
* DATA STRUCTURES
*
*       IKE_Group_DB            The IKE Groups database.
*
* FUNCTIONS
*
*       IKE_Initialize_Groups
*       IKE_Cmp_Groups
*       IKE_Add_Group
*       IKE_Add_To_Group
*       IKE_Get_Group_Entry
*       IKE_Get_Group_Entry_By_Device
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

/*** Global variables related to policy groups. ***/

/* The global policy group database. */
IKE_POLICY_GROUP_DB IKE_Group_DB;

/************************************************************************
*
* FUNCTION
*
*       IKE_Initialize_Groups
*
* DESCRIPTION
*
*       This function initializes the IKE Groups Database.
*       It sets the database to contain zero items.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful initialization.
*
************************************************************************/
STATUS IKE_Initialize_Groups(VOID)
{
    /* Log debug message. */
    IKE_DEBUG_LOG("Initializing IKE groups");

    /* Set link pointers of the Group DB to zero. */
    IKE_Group_DB.ike_flink = NU_NULL;
    IKE_Group_DB.ike_last = NU_NULL;

    /* Return the status. */
    return (NU_SUCCESS);

} /* IKE_Initialize_Groups */

/************************************************************************
*
* FUNCTION
*
*       IKE_Cmp_Groups
*
* DESCRIPTION
*
*       This is an internal utility function used for comparison
*       for adding sorted items to the Groups database. This
*       is a callback function and would never be called directly.
*       It is called by SLL_Insert_Sorted.
*
* INPUTS
*
*       *a                      Pointer to the first node of type
*                               IKE_POLICY_GROUP node.
*       *b                      Pointer to the second node of type
*                               IKE_POLICY_GROUP node.
*
* OUTPUTS
*
*       -1                      If a < b.
*       1                       If a > b.
*       0                       If a == b.
*
************************************************************************/
INT IKE_Cmp_Groups(VOID *a, VOID *b)
{
    INT                 result;
    IKE_POLICY_GROUP    *anode = a;
    IKE_POLICY_GROUP    *bnode = b;

    /* Compare the two strings. */
    result = strcmp(anode->ike_group_name, bnode->ike_group_name);

    /* Return the result of the comparison. */
    return (result);

} /* IKE_Cmp_Groups */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_Group
*
* DESCRIPTION
*
*       This function adds a new IKE group to the database.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Specifies the group name.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       NU_NO_MEMORY            Not enough memory available.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_ALREADY_EXISTS      Group already exists.
*       IKE_LENGTH_IS_LONG      Group name is too long.
*
************************************************************************/
STATUS IKE_Add_Group(CHAR *group_name)
{
    STATUS              status;
    UINT32              mem_size;
    IKE_POLICY_GROUP    *group_ptr;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the group name pointer is not NULL. */
    if(group_name == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Make sure group name length is not too long. The maximum
     * group name length macro includes the null-terminator.
     */
    else if(strlen(group_name) >= IKE_MAX_GROUP_NAME_LEN)
    {
        /* Group name length is too long. */
        status = IKE_LENGTH_IS_LONG;
    }

    else
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Check if group is already present. */
            status = IKE_Get_Group_Entry(group_name, &group_ptr);

            if(status == IKE_NOT_FOUND)
            {
                /* Calculate the memory required. */
                mem_size = (sizeof(IKE_POLICY_GROUP) +
                            strlen(group_name) + 1);

                /* Allocate the memory for the group entry along
                 * with the group name entry.
                 */
                status = NU_Allocate_Memory(IKE_Data.ike_memory,
                                            (VOID**)&group_ptr, mem_size,
                                            NU_NO_SUSPEND);

                if(status == NU_SUCCESS)
                {
                    /* Normalize the pointer. */
                    group_ptr = TLS_Normalize_Ptr(group_ptr);

                    /* Assign memory to the group name pointer
                     * of the structure.
                     */
                    group_ptr->ike_group_name =
                        ((CHAR *)group_ptr + sizeof(IKE_POLICY_GROUP));

                    /* Copy the given group name to the newly created
                     * group's name member.
                     */
                    strcpy(group_ptr->ike_group_name, group_name);

                    /* Zero out the SPDB in the group. */
                    UTL_Zero(&(group_ptr->ike_policy_list),
                             sizeof(IKE_SPDB));

                    /* Add newly created group to the list of groups. */
                    SLL_Insert_Sorted(&IKE_Group_DB, group_ptr,
                                      IKE_Cmp_Groups);
                }

                else
                {
                    NLOG_Error_Log("Failed to allocate memory",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                /* Group already exists, return error. */
                status = IKE_ALREADY_EXISTS;

                NLOG_Error_Log("IKE group already exists in the database",
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

} /* IKE_Add_Group */

/************************************************************************
*
* FUNCTION
*
*       IKE_Add_To_Group
*
* DESCRIPTION
*
*       This function associates an interface to the passed group.
*
*       This is an IKE API function.
*
* INPUTS
*
*       *group_name             Pointer to the group name.
*       *interface_name         Interface name to be added to group.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_ALREADY_EXISTS      Interface already present in a group.
*       IKE_NOT_FOUND           Group not found.
*
************************************************************************/
STATUS IKE_Add_To_Group(CHAR *group_name, CHAR *interface_name)
{
    STATUS              status;
    IKE_POLICY_GROUP    *group;
    DV_DEVICE_ENTRY     *dev_entry;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure the group name pointer is not NULL. */
    if(group_name == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    /* Make sure that the interface name given is correct. */
    else if(interface_name == NU_NULL)
    {
        status = IKE_INVALID_PARAMS;
    }

    else
    {
        /* Grab the IKE semaphore. */
        status = NU_Obtain_Semaphore(&IKE_Data.ike_semaphore, IKE_TIMEOUT);

        if(status == NU_SUCCESS)
        {
            /* Get the group pointer. */
            status = IKE_Get_Group_Entry(group_name, &group);

            if(status == NU_SUCCESS)
            {
                /* Grab the NET semaphore. */
                status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

                if(status == NU_SUCCESS)
                {
                    /* Get device by interface name. */
                    dev_entry = DEV_Get_Dev_By_Name(interface_name);

                    if(dev_entry != NU_NULL)
                    {
                        /* Make sure that the group is not empty. */
                        if(dev_entry->dev_physical->dev_phy_ike_group !=
                           NU_NULL)
                        {
                            /* The interface policy group is not empty.
                             * It should be removed from the current
                             * group first in order to register with
                             * another group.
                             */
                            status = IKE_ALREADY_EXISTS;
                        }

                        else
                        {
                            /* Set the policy group pointer of this
                             * interface to the policy group found above.
                             */
                            dev_entry->dev_physical->dev_phy_ike_group =
                                group;
                        }
                    }

                    else
                    {
                        /* Failed to find the interface. */
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
            }

            else
            {
                NLOG_Error_Log("Failed to get IKE group",
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

} /* IKE_Add_To_Group */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Group_Entry
*
* DESCRIPTION
*
*       Returns a pointer to the group corresponding to the
*       passed group name.
*
* INPUTS
*
*       *group_name             Name of the group.
*       **ret_group             Pointer to an IKE group pointer.
*                               On return, this would contain
*                               the pointer to the group being
*                               searched.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Group not found.
*
************************************************************************/
STATUS IKE_Get_Group_Entry(CHAR *group_name,
                           IKE_POLICY_GROUP **ret_group)
{
    STATUS              status;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure that the group name given is correct. */
    if(group_name == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }

    /* Make sure that return group pointer is not null. */
    if(ret_group == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Call the base IPsec function. */
    status = IPSEC_Get_Group_Entry_Real((IPSEC_GROUP_DB*)&IKE_Group_DB,
                                        group_name,
                                        (IPSEC_POLICY_GROUP**)ret_group);

    /* Return the status value. */
    return (status);

} /* IKE_Get_Group_Entry */

/************************************************************************
*
* FUNCTION
*
*       IKE_Get_Group_Entry_By_Device
*
* DESCRIPTION
*
*       Returns a pointer to the group corresponding to the
*       passed device index.
*
* INPUTS
*
*       dev_index               Index of the device.
*       **ret_group             Pointer to an IKE group pointer.
*                               On return, this would contain
*                               the pointer to the group being
*                               searched.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_INVALID_PARAMS      Parameters are invalid.
*       IKE_NOT_FOUND           Group not found.
*
************************************************************************/
STATUS IKE_Get_Group_Entry_By_Device(UINT32 dev_index,
                                     IKE_POLICY_GROUP **ret_group)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *device;

#if (IKE_DEBUG == NU_TRUE)
    /* Make sure that return group pointer is not null. */
    if(ret_group == NU_NULL)
    {
        return (IKE_INVALID_PARAMS);
    }
#endif

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Look-up the device using the index. */
        device = DEV_Get_Dev_By_Index(dev_index);

        if(device == NU_NULL)
        {
            /* No device found corresponding to specified index. */
            status = IKE_NOT_FOUND;

            NLOG_Error_Log("No device found corresponding to the index",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else if(device->dev_physical->dev_phy_ike_group == NU_NULL)
        {
            /* Specified device is not part of any IKE group. */
            status = IKE_NOT_FOUND;

            NLOG_Error_Log("Specified device not part of an IKE group",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        else
        {
            /* Retrieve IKE group pointer from the device. */
            *ret_group =
                (IKE_POLICY_GROUP*)device->dev_physical->dev_phy_ike_group;
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

} /* IKE_Get_Group_Entry_By_Device */

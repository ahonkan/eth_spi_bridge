/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       arp_up.c
*
* DESCRIPTION
*
*       This file contains the implementation of ARP_Update.
*
* DATA STRUCTURES
*
*       none
*
* FUNCTIONS
*
*       NU_ARP_Update
*       ARP_Update
*
* DEPENDENCIES
*
*       nu_net.h
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       NU_ARP_Update
*
*   DESCRIPTION
*
*       This function updates the parameters of the ARP Cache entry
*       associated with the IP address.
*
*   INPUTS
*
*       *arp_changes            A pointer to the data structure holding
*                               the updated parameters.
*       *target_ip              The IP address associated with the entry
*                               to be updated.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVAL                One of the parameters is invalid
*       NU_NO_MEMORY            Insufficient memory
*
*************************************************************************/
STATUS NU_ARP_Update(ARP_ENTRY *arp_changes, const UINT8 *target_ip)
{
    STATUS    status;
    NU_SUPERV_USER_VARIABLES

    arp_changes->arp_flags |= (INT32)RT_LOCAL;

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    status = ARP_Update(arp_changes, target_ip);

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_USER_MODE();

    return (status);

} /* NU_ARP_Update */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Update
*
*   DESCRIPTION
*
*       This function updates the parameters of the ARP Cache entry
*       associated with the IP address.
*
*   INPUTS
*
*       *arp_changes            A pointer to the data structure holding
*                               the updated parameters.
*       *target_ip              The IP address associated with the entry
*                               to update.
*   OUTPUTS
*
*       NU_SUCCESS
*       NU_INVAL
*       NU_NO_MEMORY
*
*************************************************************************/
STATUS ARP_Update(const ARP_ENTRY *arp_changes, const UINT8 *target_ip)
{
    SCK_SOCKADDR_IP     target;
    STATUS              status = NU_SUCCESS;
    ARP_ENTRY           *arp_target;
    UPDATED_ROUTE_NODE  updated_route;
    UINT8               default_hw_addr[6] = {0,0,0,0,0,0};
    UINT8               hw_addr[6];
    UINT8               gateway[IP_ADDR_LEN];
    RTAB4_ROUTE_ENTRY   *target_route;
    INT16               flags;
    DV_DEVICE_ENTRY     *dv_entry;

    target.sck_addr = IP_ADDR(target_ip);

    /* Find the ARP Cache entry */
    arp_target = ARP_Find_Entry(&target);

    /* If the entry already exists, update it according to arp_changes */
    if (arp_target)
    {
        /* Update the flags of the entry */
        if (arp_changes->arp_flags != -1)
        {
            /* Delete the entry */
            if ((arp_changes->arp_flags & ARP_UP) == 0)
            {
                if (ARP_Delete_Entry(&target) < 0)
                    NLOG_Error_Log("Failed to delete ARP entry", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }

            /* Otherwise, update the flags */
            else
                arp_target->arp_flags = arp_changes->arp_flags;
        }

        /* If we didn't just mark the entry as deleted */
        if ((arp_changes->arp_flags & ARP_UP) != 0)
        {
            /* Update the IP address of the entry */
            if (memcmp(arp_changes->ip_addr.ip_address, "\xff\xff\xff\xff", 4) != 0)
                arp_target->ip_addr.arp_ip_addr = IP_ADDR(arp_changes->ip_addr.ip_address);

            /* Update the hardware address of the entry */
            if (memcmp(arp_changes->arp_mac_addr, "\xff\xff\xff\xff\xff\xff", DADDLEN) != 0)
                memcpy(arp_target->arp_mac_addr, arp_changes->arp_mac_addr, DADDLEN);

            /* Update the arp_time to indicate when changes were last made */
            arp_target->arp_time = NU_Retrieve_Clock();

            /* Update the device index for the route associated with the entry */
            if ((INT)arp_changes->arp_dev_index != -1)
            {
                target.sck_addr = IP_ADDR(target_ip);

                /* Override the device state, because the device is going
                 * to change.
                 */
                target_route = RTAB4_Find_Route(&target, RT_HOST_MATCH |
                                                         RT_BEST_METRIC |
                                                         RT_OVERRIDE_METRIC |
                                                         RT_OVERRIDE_DV_STATE);

                if (target_route)
                {
                    RTAB_Free((ROUTE_ENTRY*)target_route, NU_FAMILY_IP);

                    /* Set the parameters of the updated_route data structure to NULL */
                    memset(&updated_route, -1, sizeof(updated_route));

                    /* Get a pointer to the device */
                    dv_entry = DEV_Get_Dev_By_Index((UINT32)arp_changes->arp_dev_index);

                    /* Assign the new name */
                    updated_route.urt_dev.urt_dev_name = dv_entry->dev_net_if_name;

                    if (arp_target->arp_flags & (INT32)RT_NETMGMT)
                        updated_route.urt_flags |= (INT32)RT_NETMGMT;

                    else if (arp_target->arp_flags & (INT32)RT_LOCAL)
                        updated_route.urt_flags |= (INT32)RT_LOCAL;

                    PUT32(gateway, 0, target_route->rt_gateway_v4.sck_addr);

                    status = RTAB4_Update_Route(target_ip, gateway, &updated_route);

                    /* Ensure the route was updated properly. */
                    if (status == NU_SUCCESS)
                    {
                        /* Update the ARP cache entry's interface index. */
                        arp_target->arp_dev_index = arp_changes->arp_dev_index;
                    }
                }
                else
                    status = NU_INVAL;
            }
        }
    }

    /* If the user is trying to create an entry by setting the flags to
     * invalid, exit.
     */
    else if ((arp_changes->arp_flags & ARP_UP) == 0)
    {
        status = NU_INVAL;
    }

    /* Otherwise, the entry does not already exist, so add the entry */
    else
    {
        /* If a valid hardware address was not passed in, use the default */
        if (memcmp(arp_changes->arp_mac_addr, "\xff\xff\xff\xff\xff\xff",
                   DADDLEN) != 0)
            memcpy(hw_addr, arp_changes->arp_mac_addr, DADDLEN);
        else
            memcpy(hw_addr, default_hw_addr, DADDLEN);

        /* If a valid flags parameter was not passed in, use the default */
        if (arp_changes->arp_flags != -1)
            flags = (INT16)(ARP_UP | arp_changes->arp_flags);
        else
            flags = ARP_UP;

        /* Create the new ARP Cache entry */
        status = ARP_Cache_Update(IP_ADDR(target_ip), hw_addr, flags,
                                  arp_changes->arp_dev_index);

        /* The entry was added */
        if (status >= 0)
            status = NU_SUCCESS;

        /* There is no room to add the entry */
        else
            status = NU_NO_MEMORY;
    }

    return (status);

} /* ARP_Update */

#endif

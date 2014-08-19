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
*
*   FILE NAME
*
*       dev4_difd.c
*
*   DESCRIPTION
*
*       This file contains the implementation of DEV4_Delete_IP_From_Device
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DEV4_Delete_IP_From_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
extern  DEV_IF_ADDR_ENTRY   NET_Device_Addr_Memory[];
extern  UINT8               NET_Device_Addr_Memory_Flags[];
#endif

extern ROUTE_NODE   *RTAB4_Default_Route;

/**************************************************************************
*
*   FUNCTION
*
*       DEV4_Delete_IP_From_Device
*
*   DESCRIPTION
*
*       Given a device name, this routine will remove the IPv4 number from
*       the network interface table for the device.  All routes using this
*       IP address as the next-hop will be removed.
*
*   INPUTS
*
*       *dev_ptr                A pointer to the device.
*       *ip_addr                The IP address to remove from the device.
*
*   OUTPUTS
*
*       NU_SUCCESS              Device was updated.
*       NU_INVALID_ADDRESS      The address was obtained via DHCP and
*                               must be released through DHCP.
*
****************************************************************************/
STATUS DEV4_Delete_IP_From_Device(DV_DEVICE_ENTRY *dev_ptr,
                                  DEV_IF_ADDR_ENTRY *ip_addr)
{
    UINT8               gw_ip[IP_ADDR_LEN];
    UINT8               current_address[] = {0, 0, 0, 0};
    UINT8               current_gw_address[IP_ADDR_LEN];
    RTAB4_ROUTE_ENTRY   *current_route, *next_route, *next_gw_route;
    STATUS              status;

#if (INCLUDE_LL_CONFIG == NU_TRUE)
    DEV_IF_ADDR_ENTRY   *current_addr;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT                 j;
#endif

#if (INCLUDE_DHCP == NU_TRUE)
    /* If this address was obtained via DHCP, return an error to the
     * application.  NU_DHCP_Release must be used to delete an address
     * obtained via DHCP from an interface.
     */
    if ( (dev_ptr->dev_addr.dev_dhcp_addr == 0) ||
         (ip_addr->dev_entry_ip_addr != dev_ptr->dev_addr.dev_dhcp_addr) )
#endif
    {
#if (INCLUDE_MDNS == NU_TRUE)

        /* Convert the 32-bit integer into a 4-byte array. */
        PUT32(current_address, 0, ip_addr->dev_entry_ip_addr);

        /* Update all local host records that are using this IP address. */
        MDNS_Remove_Local_Host(current_address, NU_FAMILY_IP);
#endif

        /* If this is the only IP address on the interface, mark the
         * interface as down for future processing below.
         */
        if ( (dev_ptr->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr ==
              ip_addr->dev_entry_ip_addr) &&
             (dev_ptr->dev_addr.dev_addr_list.dv_head->dev_entry_next == NU_NULL) )
        {
            dev_ptr->dev_flags &= ~DV_UP;

            /* Trace log */
            T_DEV_UP_STATUS(dev_ptr->dev_net_if_name, 1);

            /* If the default route uses this interface, delete the default route
             * now.
             */
            if (RTAB4_Default_Route)
            {
                /* Get a pointer to the route entry. */
                current_route = RTAB4_Default_Route->rt_list_head;

                /* Check if any of the route entries use this interface. */
                while (current_route)
                {
                    /* Save a pointer to the next route. */
                    next_route = current_route->rt_entry_next;

                    if (current_route->rt_device == dev_ptr)
                    {
                        PUT32(gw_ip, 0, current_route->rt_gateway_v4.sck_addr);

                        /* Delete this route. */
                        RTAB4_Delete_Route(RTAB4_Default_Route->rt_ip_addr, gw_ip);
                    }

                    current_route = next_route;
                }
            }

#if (MIB2_IF_INCLUDE == NU_TRUE)
            /* Set the desired state of the interface in MIB-2 to 2. */
            dev_ptr->dev_mibInterface.statusAdmin = NU_FALSE;

            MIB2_Set_IfStatusOper(dev_ptr, MIB2_IF_OPER_STAT_DOWN);
#endif
        }

        status = NU_SUCCESS;

        /* Get a pointer to the first route in the routing table */
        next_route = RTAB4_Find_Next_Route(current_address);

        /* Process each route in the Routing Table to determine if it needs
         * to be removed.  All routes using the interface passed in to the
         * routine will be removed from the routing table.
         */
        while (next_route)
        {
            /* Save a copy of the address to get the next route */
            memcpy(current_address, next_route->rt_route_node->rt_ip_addr,
                   IP_ADDR_LEN);

            next_route = next_route->rt_route_node->rt_list_head;

            /* Traverse the list of route entries for this route node */
            do
            {
                /* If this route uses the specified device, and there are no other IP
                 * addresses on the interface or the IP address being deleted is the
                 * gateway to the route, or this is the route to the loopback device
                 * through the specified address, delete it.
                 */
                if ( ((next_route->rt_entry_parms.rt_parm_device == dev_ptr) &&
                     ((!(dev_ptr->dev_flags & DV_UP)) ||
                      (next_route->rt_gateway_v4.sck_addr == ip_addr->dev_entry_ip_addr)))
#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)
                      ||
                     ((next_route->rt_entry_parms.rt_parm_device->dev_addr.dev_addr_list.
                       dv_head->dev_entry_ip_addr == 0x7F000001UL) &&
                      (IP_ADDR(next_route->rt_route_node->rt_ip_addr) == ip_addr->dev_entry_ip_addr))
#endif
                      )
                {
                    memset(current_gw_address, 0, IP_ADDR_LEN);

                    /* Get a pointer to the first route in the table */
                    next_gw_route = RTAB4_Find_Next_Route(current_gw_address);

                    /* Delete any routes that use this route as a next-hop */
                    while (next_gw_route)
                    {
                        /* Save a copy of the address to get the next route */
                        memcpy(current_gw_address, next_gw_route->rt_route_node->rt_ip_addr,
                               IP_ADDR_LEN);

                        /* Get a pointer to the first route in the list of routes */
                        next_gw_route = next_gw_route->rt_route_node->rt_list_head;

                        do
                        {
                            /* If this route uses the same device as the route to be
                             * deleted, delete the route.  This ensures that all
                             * routes added through a route added through the address
                             * to be removed are also removed.
                             */
                            if ( (next_gw_route->rt_entry_parms.rt_parm_device == dev_ptr) &&
                                 (next_gw_route->rt_gateway_v4.sck_addr == IP_ADDR(next_route->rt_route_node->rt_ip_addr)) &&
                                 (next_gw_route->rt_gateway_v4.sck_addr != ip_addr->dev_entry_ip_addr) )
                            {
                                /* Store the 32-bit value in an array */
                                PUT32(gw_ip, 0, next_gw_route->rt_gateway_v4.sck_addr);

                                /* Get a pointer to the next route entry before this one is
                                 * deleted.
                                 */
                                next_gw_route = next_gw_route->rt_entry_next;

                                /* Delete the route */
                                if (RTAB4_Delete_Route(current_gw_address, gw_ip) != NU_SUCCESS)
                                    NLOG_Error_Log("Failed to remove route for the IPv4 address",
                                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                            }

                            else
                                next_gw_route = next_gw_route->rt_entry_next;

                        } while (next_gw_route);

                        /* Get a pointer to the next route in the routing table */
                        next_gw_route = RTAB4_Find_Next_Route(current_gw_address);
                    }

                    /* Store the 32-bit value in an array */
                    PUT32(gw_ip, 0, next_route->rt_gateway_v4.sck_addr);

                    /* Get a pointer to the next route entry before this one is
                     * deleted.
                     */
                    next_route = next_route->rt_entry_next;

                    /* Delete the route */
                    if (RTAB4_Delete_Route(current_address, gw_ip) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to remove route for the IPv4 address",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Get a pointer to the next route entry */
                else
                    next_route = next_route->rt_entry_next;

            } while (next_route);

            /* Get a pointer to the next route in the routing table */
            next_route = RTAB4_Find_Next_Route(current_address);
        }

        /* Remove the address from the list of addresses for the interface */
        DLL_Remove(&dev_ptr->dev_addr.dev_addr_list, ip_addr);

#if (INCLUDE_SR_SNMP == NU_TRUE)

        /* Update the SNMP IP Address Translation Table. */
        SNMP_ipNetToMediaTableUpdate(SNMP_DELETE, dev_ptr->dev_index,
                                     dev_ptr->dev_mac_addr,
                                     (UINT8 *)ip_addr->dev_entry_ip_addr, 4);

        /* Update the address translation group. */
        SNMP_atTableUpdate(SNMP_DELETE, (INT)(dev_ptr->dev_index),
                           dev_ptr->dev_mac_addr,
                           (UINT8 *)ip_addr->dev_entry_ip_addr);

        SNMP_ipAdEntUpdate(SNMP_DELETE, (UINT32)(dev_ptr->dev_index),
                           (UINT8 *)ip_addr->dev_entry_ip_addr,
                           (UINT8 *)ip_addr->dev_entry_netmask, 1, 0);
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        /* Deallocate the memory being used by the address data structure */
        if (NU_Deallocate_Memory((VOID*)ip_addr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate the memory for the IPv4 address",
                           NERR_SEVERE, __FILE__, __LINE__);

#else

        /* Traverse the flag array to find the used memory location*/
        for (j = 0; j != (NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE); j++)
        {
            /* If this is the memory area being released */
            if (&NET_Device_Addr_Memory[j] == ip_addr)
            {
                /* Turn the memory flag off */
                NET_Device_Addr_Memory_Flags[j] = NU_FALSE;
            }
        }

#endif

    }

#if (INCLUDE_DHCP == NU_TRUE)
    else
        status = NU_INVALID_ADDRESS;
#endif

#if (INCLUDE_LL_CONFIG == NU_TRUE)

    /* If the device is enabled for link-local address configuration. */
    if (dev_ptr->dev_flags & DV_CFG_IPV4_LL_ADDR)
    {
        /* Convert the 32-bit integer into a 4-byte array. */
        PUT32(current_address, 0, ip_addr->dev_entry_ip_addr);

        /* If the address being deleted is not a link-local address. */
        if (!(IP_LL_ADDR(current_address)))
        {
            /* Ensure there is no routable address on the interface. */
            current_addr = dev_ptr->dev_addr.dev_addr_list.dv_head;

            while (current_addr)
            {
                /* If this is not an empty entry awaiting DHCP resolution. */
                if (current_addr->dev_entry_ip_addr != 0)
                {
                    break;
                }

                current_addr = current_addr->dev_entry_next;
            }

            /* If a routable address was not found on the interface. */
            if (!current_addr)
            {
                /* Initiate link-local address configuration. */
                DEV_Configure_Link_Local_Addr(dev_ptr, NU_NULL);
            }
        }

        else
        {
            /* Reset the state. */
            dev_ptr->dev_ll_state = 0;

            /* Clear out the link-local address. */
            dev_ptr->dev_addr.dev_link_local_addr = IP_ADDR_ANY;
        }
    }

#endif

    return (status);

} /* DEV4_Delete_IP_From_Device */

#endif

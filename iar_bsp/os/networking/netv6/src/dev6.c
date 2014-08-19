/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*       dev6.c
*
*   DESCRIPTION
*
*       This file contains those functions necessary to manage the
*       interfaces on a node.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DEV6_Init_Device
*       DEV6_AutoConfigure_Device
*       DEV6_Autoconfigure_Addrs
*       DEV6_Insert_Address
*       DEV6_Add_IP_To_Device
*       DEV6_Expire_Address
*       DEV6_Deprecate_Address
*       DEV6_Delete_IP_From_Device
*       DEV6_Attach_IP_To_Device
*       DEV6_Verify_Valid_Addr
*       DEV6_Get_Dev_By_Addr
*       DEV6_Find_Target_Address
*       DEV6_Get_Primary_Interface
*       DEV6_Create_Address_From_Prefix
*       DEV6_Update_Address_Entry
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       dad6.h
*       nu_net6.h
*       net6.h
*       nc6.h
*       nc6_eth.h
*       nd6radv.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/dad6.h"
#include "networking/nu_net6.h"
#include "networking/net6.h"
#include "networking/nc6.h"
#include "networking/nc6_eth.h"
#include "networking/dev6.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IP_MULTICASTING)
#include "networking/mld6.h"
#endif

#ifdef CFG_NU_OS_DRVR_PPP_ENABLE
#include "drivers/nu_ppp.h"
#endif

UINT32  DEV6_Addr_Struct_ID = 0;

extern UINT8    IP6_Link_Local_Prefix[];
extern UINT8    IP6_Solicited_Node_Multi[];
extern UINT8    IP6_All_Nodes_Multi[];
extern UINT8    IP6_All_Hosts_Multi[];
extern UINT8    IP6_Loopback_Address[];

extern TQ_EVENT ICMP6_RtrSol_Event;
extern TQ_EVENT IP6_Expire_Address_Event;
extern TQ_EVENT IP6_Deprecate_Address_Event;
extern TQ_EVENT IP6_Verify_Valid_Addr_Event;
extern TQ_EVENT IP6_Join_Solicited_Mcast_Event;

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Init_Device
*
*   DESCRIPTION
*
*       This function configured interface specific parameters of IPv6.
*
*   INPUTS
*
*       *dev_ptr                A pointer to the device entry.
*       *device                 A pointer to the device structure
*                               filled in by the user.
*
*   OUTPUTS
*
*       NU_SUCCESS              The device was successfully initialized.
*       NU_NO_MEMORY            Insufficient memory.
*       NU_INVALID_PARM         The device type is not supported by
*                               IPv6 or the prefix to add to the prefix
*                               list is invalid.
*       NU_INVAL                Router functionality is disabled on the
*                               node, and the user is attempting to
*                               configure the interface as an IPv6
*                               router.
*
*************************************************************************/
STATUS DEV6_Init_Device(DV_DEVICE_ENTRY *dev_ptr, const DEV_DEVICE *device)
{
    STATUS          status;

#ifdef NET_5_4

    /* Allocate memory for the new address structure */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&dev_ptr->dev6_ipv6_data,
                           sizeof(DEV6_IPV6_STRUCT), NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Could not allocate memory for the IPv6 device structure",
                       NERR_SEVERE, __FILE__, __LINE__);

        return (NU_NO_MEMORY);
    }

    /* Zero out the elements of the structure */
    UTL_Zero(dev_ptr->dev6_ipv6_data, sizeof(DEV6_IPV6_STRUCT));

    /* Copy the IPv6 flags set by the application. */
    dev_ptr->dev6_flags = device->dv6_flags;

#endif

    /* Ethernet Loopback */
    if (dev_ptr->dev_type == DVT_LOOP)
        status = ETH6_Init_IPv6_Device(dev_ptr,
                                       IP6_LDC_NEIGHBOR_CACHE_ENTRIES,
                                       0, IP6_LDC_INTERFACE_ID_LENGTH);

    /* Ethernet or VLAN */
    else if ( (dev_ptr->dev_type == DVT_ETHER) ||
              (dev_ptr->dev_type == DVT_VLAN) )
        status = ETH6_Init_IPv6_Device(dev_ptr,
                                       IP6_ETH_NEIGHBOR_CACHE_ENTRIES,
                                       DAD6_TRANS_COUNT,
                                       IP6_ETHERNET_ID_LENGTH);

#ifdef CFG_NU_OS_DRVR_PPP_ENABLE
    /* PPP or PPPoE */
    else if ( (dev_ptr->dev_type == DVT_PPP) ||
              (dev_ptr->dev_type == DVT_PPPOE) )
        status = PPP6_Init_IPv6_Device(dev_ptr);
#endif

    else
        status = NU_INVALID_PARM;

    /* If the initialization was successful and the interface is a routing
     * interface, set up the routing parameters.
     */
    if ( (dev_ptr->dev_flags & DV6_ISROUTER) && (status == NU_SUCCESS) )
    {
#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

        /* Set the configurable parameters on the route */
        DEV6_Configure_Router(dev_ptr, NU_NULL, -1, &device->dv6_rtr_opts);

        /* Add the prefix list entries */
        if (device->dv6_rtr_opts.rtr_AdvPrefixList)
        {
            status =
                DEV6_Configure_Prefix_List(dev_ptr,
                                           device->dv6_rtr_opts.
                                           rtr_AdvPrefixList);
        }
#else

        /* The user is trying to enable router functionality on an interface,
         * but router functionality is disabled in the system.
         */
        status = NU_INVAL;
#endif
    }

    return (status);

} /* DEV6_Init_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_AutoConfigure_Device
*
*   DESCRIPTION
*
*       This function invokes address autoconfiguration for the IPv6
*       enabled device.
*
*   INPUTS
*
*       *device                 A pointer to the device entry.
*
*   OUTPUTS
*
*       -1                      The device is invalid.
*       0                       The device was successfully configured.
*
*************************************************************************/
INT DEV6_AutoConfigure_Device(DV_DEVICE_ENTRY *device)
{
#if (INCLUDE_IP_MULTICASTING)

    /* If multicasting support is desired, join the relevant Multicast
     * groups.
     */
    if (device->dev_flags & DV_MULTICAST)
    {
        /* First, we must set the default MLD compatibility version for the
         * device.
         */
        device->dev6_mld_compat_mode = MLD6_DEFAULT_COMPATIBILTY_MODE;

        /* Set the query interval value to it's default values */
        device->dev6_mld_last_query_int = MLD6_QUERY_INTERVAL;

        /* Join the All-Nodes Multicast address group.  This group must be
         * joined before DAD is performed, because the Neighbor Advertisement
         * to a duplicate address will be sent to the All-Nodes multicast
         * address.
         */
        if (IP6_Add_Multi(IP6_All_Nodes_Multi, device, NU_NULL) == NU_NULL)
        {
            NLOG_Error_Log("Could not join All-Nodes Multicast Address group",
                           NERR_SEVERE, __FILE__, __LINE__);

            return (-1);
        }
    }

#endif /* INCLUDE_IP_MULTICASTING */

    /* Invoke stateless address autoconfiguration to assign a link-local
     * address and obtain a globally unique address from the on-link router.
     */
    if (DEV6_Autoconfigure_Addrs(device) != NU_SUCCESS)
    {
        NLOG_Error_Log("DEV6_Autoconfigure_Addrs() failed", NERR_SEVERE,
                       __FILE__, __LINE__);

        return (-1);
    }

    return (0);

} /* DEV6_AutoConfigure_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Autoconfigure_Addrs
*
*   DESCRIPTION
*
*       This function computes link-local address for the IPv6
*       enabled device.
*
*   INPUTS
*
*       *device                 A pointer to the device entry.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS DEV6_Autoconfigure_Addrs(DV_DEVICE_ENTRY* device)
{
    UINT8   addr[IP6_ADDR_LEN];

#if (INCLUDE_IP_MULTICASTING)
    UINT32  delay;
#endif

    /* RFC 4862 - section 5.3 - If the sum of the link-local prefix length
     * and N is larger than 128, autoconfiguration fails and manual
     * configuration is required.
     */
    if ( ((device->dev6_interface_id_length + LINK_LOCAL_PREFIX_LENGTH) << 3) > 128)
    {
        NLOG_Error_Log("Autoconfiguration Failed: interface id + link-local prefix > 128 bits",
                       NERR_SEVERE, __FILE__, __LINE__);

        return (-1);
    }

    /* Zero out the link-local address */
    UTL_Zero(addr, IP6_ADDR_LEN);

    /* Generate the Link-Local prefix */
    memcpy(addr, IP6_Link_Local_Prefix, LINK_LOCAL_PREFIX_LENGTH);

    /* Append the interface identifier onto the link-local address prefix. */
    if ( (device->dev6_interface_id_length > 0) &&
         (device->dev6_interface_id_length <= IP6_ADDR_LEN) &&
         (device->dev6_interface_id_length <= sizeof(device->dev6_interface_id)))
    {
        memcpy(&addr[IP6_ADDR_LEN - device->dev6_interface_id_length],
               device->dev6_interface_id, device->dev6_interface_id_length);
    }

    /* Attach the IP address to the device, delaying joining the solicited-node
     * multicast group since this is the first packet being sent from the interface.
     */
    if (DEV6_Attach_IP_To_Device(device->dev_net_if_name, addr,
                                 ADDR6_DELAY_MLD) != NU_SUCCESS)
    {
        NLOG_Error_Log("DEV6_Attach_IP_To_Device failed", NERR_SEVERE,
                       __FILE__, __LINE__);

        return (-1);
    }

#if (INCLUDE_IP_MULTICASTING)

    /* RFC 4861 - section 4.1 - Hosts send Router Solicitations in order to
     * prompt routers to generate Router Advertisements quickly.
     */
    if ( (device->dev_flags & DV_MULTICAST) && (!(device->dev_flags & DV6_ISROUTER)) )
    {
        /* RFC 4861 section 6.3.7 - Before a host sends an initial solicitation,
         * it SHOULD delay the transmission for a random amount of time between 0
         * and MAX_RTR_SOLICITATION_DELAY.
         */
        delay = ICMP6_Random_Delay(addr, IP6_Solicited_Node_Multi,
                                   IP6_MAX_RTR_SOLICITATION_DELAY);

        /* Set an event to send a Router Solicitation */
        if (TQ_Timerset(ICMP6_RtrSol_Event, device->dev_index, delay,
                         IP6_MAX_RTR_SOLICITATIONS) != NU_SUCCESS)
            NLOG_Error_Log("Could not set the event to transmit an initial Router Solicitation",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

#endif

    return (NU_SUCCESS);

} /* DEV6_Autoconfigure_Addrs */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Insert_Address
*
*   DESCRIPTION
*
*       This function adds the given address entry to the list of
*       addresses for the device.
*
*   INPUTS
*
*       *new_entry              A pointer to the address structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Insert_Address(DEV6_IF_ADDRESS *new_entry)
{
    DLL_Enqueue(&new_entry->dev6_device->dev6_addr_list,
                new_entry);

} /* DEV6_Insert_Address */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Add_IP_To_Device
*
*   DESCRIPTION
*
*       This function adds the given IP address to the list of IP
*       addresses for the respective device, and invokes Duplicate
*       Address Detection for the address to verify its uniqueness
*       on the link.
*
*   INPUTS
*
*       *device                 A pointer to the device.
*       *ip_addr                A pointer to the link-local IP address
*                               of the device.
*       prefix_length           The prefix length in bits.
*       preferred_lifetime      The preferred lifetime of the address.
*       valid_lifetime          The valid lifetime of the address.
*       flags                   Flags pertaining to the address.
*
*   OUTPUTS
*
*       -1                      There is not enough memory.
*       0                       The address was successfully added.
*
*************************************************************************/
INT DEV6_Add_IP_To_Device(DV_DEVICE_ENTRY *device, const UINT8 *ip_addr,
                          UINT8 prefix_length, UINT32 preferred_lifetime,
                          UINT32 valid_lifetime, UINT32 flags)
{
    DEV6_IF_ADDRESS     *new_entry;
    UINT8               prefix[IP6_ADDR_LEN];

#if (INCLUDE_DAD6 == NU_TRUE)
    UINT32              tick = 0;
#endif

#if (INCLUDE_IP_MULTICASTING)
    UINT32              delay;
#endif

    /* Allocate memory for the new address structure */
    if (NU_Allocate_Memory(MEM_Cached, (VOID**)&new_entry,
                           sizeof(DEV6_IF_ADDRESS), NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Could not allocate memory for new DEV6_IF_ADDRESS",
                       NERR_SEVERE, __FILE__, __LINE__);

        return (-1);
    }

    /* Zero out the new entry */
    UTL_Zero((VOID*)new_entry, sizeof(DEV6_IF_ADDRESS));

    new_entry->dev6_id = DEV6_Addr_Struct_ID ++;

    /* Copy the IP Address */
    NU_BLOCK_COPY(new_entry->dev6_ip_addr, ip_addr, IP6_ADDR_LEN);

    /* Set the Preferred and Valid Lifetime. */
    new_entry->dev6_preferred_lifetime = preferred_lifetime;
    new_entry->dev6_valid_lifetime = valid_lifetime;

#if (INCLUDE_DHCP6 == NU_TRUE)

    /* If this address was obtained via DHCP, set the appropriate flag. */
    if (flags & ADDR6_DHCP_ADDR)
        new_entry->dev6_addr_flags |= ADDR6_DHCP_ADDR;

#endif

    /* If the address is being created via stateless address autoconfiguration,
     * set the flag in the address structure.
     */
    if (flags & ADDR6_STATELESS_AUTO)
    {
        new_entry->dev6_addr_flags |= ADDR6_STATELESS_AUTO;
    }

    /* Set the timestamp indicating when this lifetime was set. */
    new_entry->dev6_timestamp = NU_Retrieve_Clock();

    /* Set a pointer back to the device */
    new_entry->dev6_device = device;

    /* If a prefix length was not specified, default to the interface ID
     * length of the device from 128.
     */
    if (prefix_length == 0)
        prefix_length = (UINT8)(128 - (device->dev6_interface_id_length << 3));

    /* Set the prefix length of the entry */
    new_entry->dev6_prefix_length = prefix_length;

    /* Insert the address into the list of addresses for the device */
    DEV6_Insert_Address(new_entry);

    /* If this is not a permanent address, set the timer to deprecate it. */
    if (preferred_lifetime != 0xffffffffUL)
    {
        if (TQ_Timerset(IP6_Deprecate_Address_Event, preferred_lifetime,
                        preferred_lifetime * TICKS_PER_SECOND,
                        (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set the timer to deprecate the new IPv6 address",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If this is not a permanent address, set the timer to expire it. */
    if (valid_lifetime != 0xffffffffUL)
    {
        if (TQ_Timerset(IP6_Expire_Address_Event, valid_lifetime,
                        valid_lifetime * TICKS_PER_SECOND,
                        (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to set the timer to expire the new IPv6 address",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Create the network IP address */
    UTL_Zero(prefix, IP6_ADDR_LEN);
    memcpy(prefix, ip_addr, prefix_length >> 3);

    /* Add a network route through the address of the interface if the prefix
     * length is not 128.
     */
    if (prefix_length != 128)
    {
        if (RTAB6_Add_Route(device, prefix, ip_addr, prefix_length,
                            (RT_UP | RT_STATIC | RT_LOCAL |
                            ((device->dev_type == DVT_LOOP) ?
                            RT_SILENT : 0))) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to add a route for the new address",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)

    /* If this is not the loopback device, add a route to the host through
     * the loopback device.
     */
    if (device->dev_type != DVT_LOOP)
    {
        if (RTAB6_Add_Route(DEV_Table.dv_head, ip_addr,
                            IP6_Loopback_Address, 128,
                            (RT_UP | RT_STATIC | RT_GATEWAY | RT_HOST |
                            RT_LOCAL)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to add a route for the new address",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

#endif

    /* If the device is not the Loopback device, invoke Duplicate Address
     * Detection for the new IP address.
     */
    if (device->dev_type != DVT_LOOP)
    {
#if (INCLUDE_IP_MULTICASTING)

        /* Otherwise, the address is not a duplicate.  Join the respective
         * multicast groups.
         */
        if (new_entry->dev6_device->dev_flags & DV_MULTICAST)
        {
            /* RFC 4862 section 5.4.2 - Even if the Neighbor Solicitation is
             * not going to be the first message sent, the node SHOULD delay
             * joining the solicited-node multicast address by a random delay
             * between 0 and MAX_RTR_SOLICITATION_DELAY if the address being
             * checked is configured by a router advertisement message sent
             * to a multicast address.
             */
            if (flags & ADDR6_DELAY_MLD)
            {
                /* Set the flag in the address so this address can be found
                 * when the timer expires.
                 */
                new_entry->dev6_addr_flags |= ADDR6_DELAY_MLD;

                delay = ICMP6_Random_Delay(new_entry->dev6_ip_addr,
                                           IP6_Solicited_Node_Multi,
                                           IP6_MAX_RTR_SOLICITATION_DELAY);

                /* Set an event to send the MLD report for joining the
                 * solicited-node multicast address.
                 */
                if (TQ_Timerset(IP6_Join_Solicited_Mcast_Event, device->dev_index,
                                delay, flags) != NU_SUCCESS)
                    NLOG_Error_Log("Could not set the event to join solicited-node multicast address.",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Join the solicited node multicast group for the address */
            IP6_Join_Solicited_Node_Multi(new_entry->dev6_device, new_entry->dev6_ip_addr,
                                          new_entry->dev6_addr_flags);
        }
#endif

#if (INCLUDE_DAD6 == NU_TRUE)

        /* If DAD is not disabled on the interface or for this address. */
        if ( (device->dev_flags & DV_MULTICAST) &&
             (!(device->dev_flags & DV6_NODAD)) && (!(flags & ADDR6_NO_DAD)) )
        {
            /* Set the state to Tentative until it is verified via Duplicate
             * Address Detection.
             */
            new_entry->dev6_addr_state |= DV6_TENTATIVE;

            /* DAD is invoked after joining the solicited-node multicast
             * group.
             */
            if (!(flags & ADDR6_DELAY_MLD))
            {
                /* Invoke DAD */
                nd6_dad_start(new_entry, device->dev6_dup_addr_detect_trans, &tick);
            }
        }

        else
        {
            /* Set the flag indicating that DAD should not be performed on this
             * address.
             */
            new_entry->dev6_addr_flags |= ADDR6_NO_DAD;
        }

#else
        /* Remove compiler warning */
        UNUSED_PARAMETER(flags);

        /* Set the flag for the address indicating that DAD has not been
         * performed for the address.
         */
        new_entry->dev6_addr_flags |= ADDR6_NO_DAD;
#endif

        if (!(flags & ADDR6_DELAY_MLD))
        {
            /* If DAD is enabled for the address. */
            if (!(new_entry->dev6_addr_flags & ADDR6_NO_DAD))
            {
                if (TQ_Timerset(IP6_Verify_Valid_Addr_Event, new_entry->dev6_id,
                                device->dev6_retrans_timer, 0) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to set the timer to verify the address",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Since DAD is disabled for the address, do not delay completing
             * autoconfiguration.
             */
            else
            {
                DEV6_Verify_Valid_Addr(IP6_Verify_Valid_Addr_Event,
                                       new_entry->dev6_id, 0);
            }
        }
    }

    /* Trace log */
    T_DEV_IPv6_IP(device->dev_net_if_name, (UINT8*)ip_addr, prefix_length, 0, 16);

    return (0);

} /* DEV6_Add_IP_To_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Expire_Address
*
*   DESCRIPTION
*
*       This function expires the IP address associated with the given
*       valid lifetime on the device associated with the given device
*       index.
*
*   INPUTS
*
*       event                   The event being handled.
*       valid_lifetime          The valid lifetime associated with
*                               the IP address entry to delete.
*       dev_index               The device index associated with the
*                               device on which the entry to delete
*                               exists.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Expire_Address(TQ_EVENT event, UNSIGNED valid_lifetime,
                         UNSIGNED dev_index)
{
    DV_DEVICE_ENTRY *device;
    DEV6_IF_ADDRESS *target_addr;

    UNUSED_PARAMETER(event);

    /* Get a pointer to the device associated with the device index */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {
        if (device->dev_flags & DV6_IPV6)
        {
            /* Get a pointer to the first IP address entry in the list */
            target_addr = device->dev6_addr_list.dv_head;

            /* Traverse the list of IP addresses searching for the IP address
             * with a matching valid lifetime.
             */
            while (target_addr)
            {
                /* If this is the target entry, delete it */
                if (target_addr->dev6_valid_lifetime == valid_lifetime)
                {
                    DEV6_Delete_IP_From_Device(target_addr);
                    break;
                }

                /* Get the next address in the list */
                target_addr = target_addr->dev6_next;
            }
        }
    }

} /* DEV6_Expire_Address */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Deprecate_Address
*
*   DESCRIPTION
*
*       This function deprecates the IP address associated with the given
*       valid lifetime on the device associated with the given device
*       index.
*
*   INPUTS
*
*       event                   The event being handled.
*       preferred_lifetime      The preferred lifetime associated with
*                               the IP address entry to deprecate.
*       dev_index               The device index associated with the
*                               device on which the entry to deprecate
*                               exists.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Deprecate_Address(TQ_EVENT event, UNSIGNED preferred_lifetime,
                            UNSIGNED dev_index)
{
    DV_DEVICE_ENTRY *device;
    DEV6_IF_ADDRESS *target_addr;

    UNUSED_PARAMETER(event);

    /* Get a pointer to the device associated with the device index */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {
        if (device->dev_flags & DV6_IPV6)
        {
            /* Get a pointer to the first IP address entry in the list */
            target_addr = device->dev6_addr_list.dv_head;

            /* Traverse the list of IP addresses searching for the IP address
             * with a matching preferred lifetime.
             */
            while (target_addr)
            {
                /* If this is the target entry, change its state to DEPRECATED */
                if (target_addr->dev6_preferred_lifetime == preferred_lifetime)
                {
                    target_addr->dev6_addr_state |= DV6_DEPRECATED;

                    break;
                }

                /* Get the next address in the list */
                target_addr = target_addr->dev6_next;
            }
        }
    }

} /* DEV6_Deprecate_Address */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Delete_IP_From_Device
*
*   DESCRIPTION
*
*       This function removes the given IPv6 address structure from the
*       list of addresses for the appropriate device.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address data
*                               structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Delete_IP_From_Device(DEV6_IF_ADDRESS *ip_addr)
{
    UINT8               gw_addr[IP6_ADDR_LEN];
    UINT8               current_address[] =
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    RTAB6_ROUTE_ENTRY   *next_route, *next_gw_route;
    UINT8               current_gw_address[IP6_ADDR_LEN];

#if (INCLUDE_DAD6 == NU_TRUE)

    /* If DAD is being performed on the address, halt DAD */
    nd6_dad_stop(ip_addr);

#endif

    /* Stop the timer that is running to deprecate the address. */
    if (ip_addr->dev6_preferred_lifetime != 0xffffffffUL)
    {
        TQ_Timerunset(IP6_Deprecate_Address_Event, TQ_CLEAR_EXACT,
                      ip_addr->dev6_preferred_lifetime,
                      ip_addr->dev6_device->dev_index);
    }

    /* Stop the timer that is running to expire the address. */
    if (ip_addr->dev6_valid_lifetime != 0xffffffffUL)
    {
        TQ_Timerunset(IP6_Expire_Address_Event, TQ_CLEAR_EXACT,
                      ip_addr->dev6_valid_lifetime,
                      ip_addr->dev6_device->dev_index);
    }

    /* Stop the timer of the event that is running to validate address. */
    TQ_Timerunset(IP6_Verify_Valid_Addr_Event, TQ_CLEAR_EXACT,
                  ip_addr->dev6_id, 0);

#if (INCLUDE_IP_MULTICASTING)

    /* Leave the solicited node multicast group for this address.  Do this
     * before deleting routes so the MLD packet can be sent out indicating
     * that the group membership is being dropped.
     */
    if ( (ip_addr->dev6_device->dev_type != DVT_LOOP) &&
         (ip_addr->dev6_device->dev_flags & DV_MULTICAST))
    {
        IP6_Leave_Solicited_Node_Multi(ip_addr->dev6_device, ip_addr->dev6_ip_addr);
    }

#endif

    /* Get a pointer to the first route in the routing table */
    next_route = RTAB6_Find_Next_Route(current_address);

    /* Process each route in the Routing Table to determine if it needs
     * to be removed.  All routes using the deleted IP address as the
     * gateway should be removed from the routing table.
     */
    while (next_route)
    {
        /* Save a copy of the address to get the next route */
        NU_BLOCK_COPY(current_address, next_route->rt_route_node->rt_ip_addr,
                      IP6_ADDR_LEN);

        RTAB_Free((ROUTE_ENTRY*)next_route, NU_FAMILY_IP6);

        next_route = next_route->rt_route_node->rt_list_head;

        /* Traverse the list of route entries for this route node */
        do
        {
            /* If this route uses the specified device and IP address as
             * the gateway or this is the route to the loopback device through
             * the specified address, delete it.
             */
            if ( ((next_route->rt_entry_parms.rt_parm_device == ip_addr->dev6_device) &&
                  (memcmp(next_route->rt_next_hop.sck_addr,
                          ip_addr->dev6_ip_addr, IP6_ADDR_LEN) == 0))
#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)
                  ||
                  ((next_route->rt_entry_parms.rt_parm_device->dev_type == DVT_LOOP) &&
                   (memcmp(next_route->rt_route_node->rt_ip_addr, ip_addr->dev6_ip_addr,
                           IP6_ADDR_LEN) == 0))
#endif
                  )
            {
                memset(current_gw_address, 0, IP6_ADDR_LEN);

                /* Get a pointer to the first route in the table */
                next_gw_route = RTAB6_Find_Next_Route(current_gw_address);

                /* Delete any routes that use this route as a next-hop */
                while (next_gw_route)
                {
                    /* Save a copy of the address to get the next route */
                    memcpy(current_gw_address, next_gw_route->rt_route_node->rt_ip_addr,
                           IP6_ADDR_LEN);

                    RTAB_Free((ROUTE_ENTRY*)next_gw_route, NU_FAMILY_IP6);

                    /* Get a pointer to the first route in the list of routes */
                    next_gw_route = next_gw_route->rt_route_node->rt_list_head;

                    do
                    {
                        /* If this route uses the same device as the route to be
                         * deleted, uses the route to be deleted as the next-hop,
                         * but does not use the address to be deleted as the
                         * next-hop, delete the route.  This ensures that all
                         * routes added through a route added through the address
                         * to be removed are also removed.
                         */
                        if ( (next_gw_route->rt_entry_parms.rt_parm_device == ip_addr->dev6_device) &&
                             (memcmp(next_gw_route->rt_next_hop.sck_addr, next_route->rt_route_node->rt_ip_addr, IP6_ADDR_LEN) == 0) &&
                             (memcmp(next_gw_route->rt_next_hop.sck_addr, ip_addr->dev6_ip_addr, IP6_ADDR_LEN) != 0) )
                        {
                            /* Store the gateway */
                            NU_BLOCK_COPY(gw_addr, next_gw_route->rt_next_hop.sck_addr,
                                          IP6_ADDR_LEN);

                            /* Get a pointer to the next route entry before this one is
                             * deleted.
                             */
                            next_gw_route = next_gw_route->rt_entry_next;

                            /* Delete the route */
                            if (RTAB6_Delete_Route(current_gw_address, gw_addr) != NU_SUCCESS)
                                NLOG_Error_Log("Failed to remove route for the IPv6 address",
                                               NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                        else
                            next_gw_route = next_gw_route->rt_entry_next;

                    } while (next_gw_route);

                    /* Get a pointer to the next route in the routing table */
                    next_gw_route = RTAB6_Find_Next_Route(current_gw_address);
                }

                /* Store the gateway */
                NU_BLOCK_COPY(gw_addr, next_route->rt_next_hop.sck_addr,
                              IP6_ADDR_LEN);

                /* Get a pointer to the next route entry before this one is
                 * deleted.
                 */
                next_route = next_route->rt_entry_next;

                /* Delete the route */
                if (RTAB6_Delete_Route(current_address, gw_addr) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to remove route for the IPv6 address",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Get a pointer to the next route entry */
            else
                next_route = next_route->rt_entry_next;

        } while (next_route);

        /* Get a pointer to the next route in the routing table */
        next_route = RTAB6_Find_Next_Route(current_address);
    }

#if (INCLUDE_MDNS == NU_TRUE)

    /* Update all host records that are using this IP address. */
    MDNS_Remove_Local_Host(ip_addr->dev6_ip_addr, NU_FAMILY_IP6);
#endif

#if (INCLUDE_DHCP6 == NU_TRUE)

    /* If this address was obtained via DHCPv6, transmit a Release message
     * so the server knows we no longer need this address.
     */
    if (ip_addr->dev6_addr_flags & ADDR6_DHCP_ADDR)
    {
        /* Set the state to DETACHED so this address will not be used
         * for future communications.
         */
        ip_addr->dev6_addr_state |= DV6_DETACHED;

        /* If the address is a duplicate, the DAD module is already
         * transmitting a Decline message, so do not release it.
         */
        if (!(ip_addr->dev6_addr_state & DV6_DUPLICATED))
        {
            /* Release the address with the DHCPv6 module. */
            DHCP6_Release_Address(ip_addr->dev6_ip_addr,
                                  ip_addr->dev6_device->dev_index);
        }
    }

    /* If the DHCP flag is not set or was removed above by DHCP6_Release_Address. */
    if (!(ip_addr->dev6_addr_flags & ADDR6_DHCP_ADDR))
    {
        /* Remove the address from the list of addresses for the interface */
        DLL_Remove(&ip_addr->dev6_device->dev6_addr_list, ip_addr);

        /* Deallocate the memory being used by the address data structure */
        if (NU_Deallocate_Memory((VOID*)ip_addr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate the memory for the IPv6 address",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

#else

    /* Remove the address from the list of addresses for the interface */
    DLL_Remove(&ip_addr->dev6_device->dev6_addr_list, ip_addr);

    /* Deallocate the memory being used by the address data structure */
    if (NU_Deallocate_Memory((VOID*)ip_addr) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate the memory for the IPv6 address",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

#endif

} /* DEV6_Delete_IP_From_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Attach_IP_To_Device
*
*   DESCRIPTION
*
*       Given a device name, this routine will assign the link-local
*       address to the device and flag the device as UP.
*
*   INPUTS
*
*       *name                   A pointer to the name of the device.
*       *ip_addr                A pointer to the link-local IP address
*                               of the device.
*       flags                   Flags associated with the operation.
*
*   OUTPUTS
*
*       -1                      The device does not exist.
*       0                       The device was successfully updated.
*
*************************************************************************/
INT DEV6_Attach_IP_To_Device(const CHAR *name, const UINT8 *ip_addr,
                             UINT32 flags)
{
    DV_DEVICE_ENTRY     *device;
    INT                 status = -1;

    /* Get a pointer to the device */
    device = DEV_Get_Dev_By_Name(name);

    /*  If the device exists in the system, set up the IP address. */
    if (device != NU_NULL)
    {
        if (device->dev_flags & DV6_IPV6)
        {
            /* Indicate that the device is Running. */
            device->dev_flags |= DV_UP;

            /* Trace log */
            T_DEV_UP_STATUS((char*)name, 0);

            /* Indicate that the device is able to receive IPv6 packets. */
            device->dev_flags2 |= DV6_UP;

            status = DEV6_Add_IP_To_Device(device, ip_addr, 0, 0xffffffffUL,
                                           0xffffffffUL, flags);
        }
    }

    return (status);

} /* DEV6_Attach_IP_To_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Verify_Valid_Addr
*
*   DESCRIPTION
*
*       This function verifies if DAD has completed on a given IP address.
*       If DAD has successfully completed, the function joins the
*       Solicited-Node Multicast group for that address.
*
*   INPUTS
*
*       event                   The event being handled.
*       addr_id                 The ID of the address being verified.
*       retry                   The number of times the address has
*                               been checked.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV6_Verify_Valid_Addr(TQ_EVENT event, UNSIGNED addr_id,
                            UNSIGNED retry)
{
    DEV6_IF_ADDRESS *addr_entry = NU_NULL;
    UINT8           len;
    DV_DEVICE_ENTRY *dev_ptr;

    UNUSED_PARAMETER(event);

    /* Get a pointer to the first interface in the list. */
    dev_ptr = DEV_Table.dv_head;

    /* Search for the device and address with the specified id. */
    while (dev_ptr)
    {
        /* Ensure this interface is IPv6-capable. */
        if (dev_ptr->dev6_ipv6_data)
        {
            /* Get a pointer to the first address in the list. */
            addr_entry = dev_ptr->dev6_addr_list.dv_head;

            /* Search the list for the address entry with the matching id. */
            while (addr_entry)
            {
                if (addr_entry->dev6_id == addr_id)
                    break;

                /* Get the next address in the list */
                addr_entry = addr_entry->dev6_next;
            }

            if (addr_entry)
                break;
        }

        dev_ptr = dev_ptr->dev_next;
    }

    /* If no matching address was found, the address has been deleted. */
    if (!addr_entry)
    {
        NLOG_Error_Log("IPv6 address deleted before DAD could be completed.",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return;
    }

    /* Extract the length of the interface ID. */
    len = addr_entry->dev6_device->dev6_interface_id_length;

#if (INCLUDE_DAD6 == NU_TRUE)

    /* If the state of the address is TENTATIVE and we have not exceeded
     * the maximum number of DAD transmissions, reset the timer to check
     * the state of the address.
     */
    if ( (addr_entry->dev6_addr_state & DV6_TENTATIVE) &&
         ((UINT8)retry <= addr_entry->dev6_device->dev6_dup_addr_detect_trans) )
    {
        if (TQ_Timerset(IP6_Verify_Valid_Addr_Event, addr_entry->dev6_id,
                        addr_entry->dev6_device->dev6_retrans_timer,
                        ++retry) != NU_SUCCESS)
            NLOG_Error_Log("Failed to reset the timer for DAD",
                           NERR_SEVERE, __FILE__, __LINE__);
    }
    else

#else
    UNUSED_PARAMETER(retry);
#endif

    /* If the address is a duplicate. */
    if (addr_entry->dev6_addr_state & DV6_DUPLICATED)
    {
        /* If the address is a link-local address */
        if (IPV6_IS_ADDR_LINKLOCAL(addr_entry->dev6_ip_addr))
        {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
            /* Leave the solicited-node multicast group for the IP address */
            IP6_Leave_Solicited_Node_Multi(addr_entry->dev6_device,
                                           addr_entry->dev6_ip_addr);
#endif

            /* RFC 2462 - section 5.4.5 - A tentative address that is
             * determined to be a duplicate as described above, MUST NOT
             * be assigned to an interface and the node SHOULD log a
             * system management error.  If the address is a link-local
             * address formed from an interface identifier, the interface
             * SHOULD be disabled.
             */
            if ( (len > 0) && (len <= 8) &&
                 (memcmp(&addr_entry->dev6_ip_addr[IP6_ADDR_LEN - len],
                         addr_entry->dev6_device->dev6_interface_id,
                         len) == 0) )
            {
                /* Indicate that the device is not able to receive IPv6 packets. */
                addr_entry->dev6_device->dev_flags2 &= ~DV6_UP;
            }
        }

        /* Delete the address from the interface */
        DEV6_Delete_IP_From_Device(addr_entry);

#if (INCLUDE_DHCP6 == NU_TRUE)

        /* If this address was obtained via DHCPv6, decline the address. */
        if (addr_entry->dev6_addr_flags & ADDR6_DHCP_ADDR)
        {
            /* Begin the decline process. */
            DHCP6_Decline_Address(addr_entry);
        }

#endif
    }

    else
    {

#if (INCLUDE_IP_MULTICASTING)

        /* Otherwise, the address is not a duplicate.  Join the
         * respective multicast groups.
         */
        if (addr_entry->dev6_device->dev_flags & DV_MULTICAST)
        {
            /* If this is the link-local address for the interface, join
             * the All-Nodes and All-Routers Multicast groups.  These
             * groups cannot be joined until the link-local address is
             * verified, because the link-local address is used as the
             * source address for MLD packets.
             */
            if (IPV6_IS_ADDR_LINKLOCAL(addr_entry->dev6_ip_addr))
            {
                /* Join the All-Hosts Multicast address group. */
                if (IP6_Add_Multi(IP6_All_Hosts_Multi,
                                  addr_entry->dev6_device,
                                  NU_NULL) == NU_NULL)
                {
                    NLOG_Error_Log("Failed to join the All-Hosts multicast address group",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

                /* If the device is acting as a Router, join the All-Routers
                 * Multicast group and begin transmitting Router Advertisement
                 * Messages.
                 */
                if (addr_entry->dev6_device->dev_flags & DV6_ISROUTER)
                    DEV6_Initialize_Router(addr_entry->dev6_device);
#endif
            }
        }
#endif

#if (INCLUDE_MDNS == NU_TRUE)

        /* Create a local host record for this IP address. */
        if (MDNS_Register_Local_Host(addr_entry->dev6_device,
                                     addr_entry->dev6_ip_addr,
                                     NU_FAMILY_IP6) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to add mDNS entry for IP address", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
#endif

    }

} /* DEV6_Verify_Valid_Addr */

/**************************************************************************
*
*   FUNCTION
*
*       DEV6_Get_Dev_By_Addr
*
*   DESCRIPTION
*
*       Find the device by IP address.
*
*   INPUTS
*
*       addr                    IP address to be associated with the device.
*
*   OUTPUTS
*
*       Pointer to the device structure or NU_NULL if the device does not
*       exist in the system.
*
****************************************************************************/
DV_DEVICE_ENTRY *DEV6_Get_Dev_By_Addr(const UINT8 *addr)
{
    DV_DEVICE_ENTRY   *dev;

#if (INCLUDE_IPV4 == NU_TRUE)

    /* If this is an IPv4-Mapped IPv6 address, check if the IPv4 address
     * is the IPv4 address of one of the devices.
     */
    if (IPV6_IS_ADDR_V4MAPPED(addr))
        dev = DEV_Get_Dev_By_Addr(&addr[12]);

    /* Otherwise, search through the list of devices for a device with
     * the IPv6 address.
     */
    else
#endif
    {
        /*  Look at the first in the list. */
        dev = DEV_Table.dv_head;

        /*  Search for a match.  */
        while (dev != NU_NULL)
        {
            /* Check if this device is configured for IPv6. */
            if (dev->dev_flags & DV6_IPV6)
            {
                /* Check if this address is one of the addresses on the interface */
                if (DEV6_Find_Target_Address(dev, addr) != NU_NULL)
                    break;
            }

            /* Get the next device in the list */
            dev = dev->dev_next;
        }
    }

    return (dev);

} /* DEV6_Get_Dev_By_Addr */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Find_Target_Address
*
*   DESCRIPTION
*
*       This function will find a matching valid address on the
*       given interface.
*
*   INPUTS
*
*       *device                 The device on which to search.
*       *target_addr            The address to match.
*
*   OUTPUTS
*
*       A pointer to the structure that holds the matching address or
*       NU_NULL if not found.
*
*************************************************************************/
DEV6_IF_ADDRESS *DEV6_Find_Target_Address(const DV_DEVICE_ENTRY *device,
                                          const UINT8 *target_addr)
{
    DEV6_IF_ADDRESS *target_address;

    /* Get a pointer to the first entry in the device's list of
     * addresses.
     */
    target_address = device->dev6_addr_list.dv_head;

    /* Search through the device's list of addresses for the address
     * that matches the Target Address in the packet.
     */
    while (target_address)
    {
        if ( (!(target_address->dev6_addr_state & DV6_DUPLICATED)) &&
             (memcmp(target_address->dev6_ip_addr, target_addr,
                     IP6_ADDR_LEN) == 0) )
            break;

        target_address = target_address->dev6_next;
    }

    return (target_address);

} /* DEV6_Find_Target_Address */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Get_Primary_Interface
*
*   DESCRIPTION
*
*       This function retrieves the primary interface on the node.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       A pointer to the primary interface or NU_NULL if a primary
*       interface was not specified at initialization.
*
*************************************************************************/
DV_DEVICE_ENTRY *DEV6_Get_Primary_Interface(VOID)
{
    DV_DEVICE_ENTRY *current_device;

    /* Search through the list of devices for the device
     * flagged as the primary device.
     */
    for (current_device = DEV_Table.dv_head;
         current_device != NU_NULL;
         current_device = current_device->dev_next)
    {
        if (current_device->dev_flags & DV6_PRIMARY_INT)
            break;
    }

    /* If the user did not specify a primary interface, use the first
     * real IPv6 interface on the node that is UP.
     */
    if (current_device == NU_NULL)
    {
        for (current_device = DEV_Table.dv_head;
             current_device != NU_NULL;
             current_device = current_device->dev_next)
        {
            if ( (current_device->dev_type != DVT_LOOP) &&
                 (current_device->dev_flags & DV6_IPV6) &&
                 (current_device->dev_flags & DV_UP) &&
                 (current_device->dev_flags2 & DV6_UP) )
                break;
        }
    }

    return (current_device);

} /* DEV6_Get_Primary_Interface */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Create_Address_From_Prefix
*
*   DESCRIPTION
*
*       This function assigns a new IPv6 address to an interface created
*       from the provided prefix and the interface's interface identifier.
*
*   INPUTS
*
*       *device                 A pointer to the interface.
*       *prefix                 A pointer to the prefix.
*       prefix_length           The length of the prefix.
*       preferred_lifetime      The preferred lifetime for the address.
*       valid_lifetime          The valid lifetime for the address.
*       flags                   Flags specific to the address.
*
*   OUTPUTS
*
*       NU_SUCCESS              The address was successfully added.
*       NU_NO_MEMORY            There is not enough memory.
*
*************************************************************************/
STATUS DEV6_Create_Address_From_Prefix(DV_DEVICE_ENTRY *device,
                                       const UINT8 *prefix, UINT8 prefix_length,
                                       UINT32 preferred_lifetime,
                                       UINT32 valid_lifetime, UINT32 flags)
{
    DEV6_IF_ADDRESS *link_local_addr;
    UINT8           new_addr[IP6_ADDR_LEN];
    STATUS          status;
    UINT8           len;

    /* Get a pointer to the link-local address on the device */
    link_local_addr = IP6_Find_Link_Local_Addr(device);

    len = device->dev6_interface_id_length;

    /* RFC 2462 section 5.4 - ... for a set of addresses formed
     * from the same interface identifier, it is sufficient to
     * check that the link-local address generated from the
     * identifier is unique on the link.
     */
    if ( (link_local_addr) && (len > 0) && (len <= 8) &&
         (memcmp(&link_local_addr->dev6_ip_addr[IP6_ADDR_LEN -
                 len], device->dev6_interface_id, len) == 0) )
    {
        /* If the address this is formed from is a duplicate, do not
         * create an address from this link-local address.
         */
        if (link_local_addr->dev6_addr_state & DV6_DUPLICATED)
            return (-1);
    }

    /* This check was added to satisfy static analysis tools.
     * This condition is not going to fail outside of unit
     * testing conditions, so no error is returned if it fails.
     */
    if ((prefix_length >> 3) < IP6_ADDR_LEN)
    {
        /* Form a new link-local address for the interface by appending
         * the Interface Identifier to the new prefix.
         */
        memcpy(new_addr, prefix, prefix_length >> 3);

        /* If the prefix length is a multiple of 8, copy the prefix
         * into the new address.
         */
        if ((prefix_length % 8) == 0)
        {
            memcpy(&new_addr[prefix_length >> 3], device->dev6_interface_id,
                   device->dev6_interface_id_length);
        }
        else
        {
            new_addr[prefix_length >> 3] = prefix[prefix_length >> 3];

            /* OR the first byte of the interface identifier in */
            new_addr[prefix_length >> 3] |= device->dev6_interface_id[0];

            /* Copy the rest of the interface identifier into the address */
            memcpy(&new_addr[(prefix_length >> 3) + 1],
                   &device->dev6_interface_id[1],
                   device->dev6_interface_id_length - 1);
        }
    }

    /* Add the address to the list of addresses assigned to the
     * interface.
     */
    status = DEV6_Add_IP_To_Device(device, new_addr, prefix_length,
                                   preferred_lifetime, valid_lifetime,
                                   flags);

    if (status != NU_SUCCESS)
        status = NU_NO_MEMORY;

    return (status);

} /* DEV6_Create_Address_From_Prefix */

/*************************************************************************
*
*   FUNCTION
*
*       DEV6_Update_Address_Entry
*
*   DESCRIPTION
*
*       This function currently updates the valid_lifetime of a configured
*       address on a device.  This function can be expanded in the future
*       to update other parameters of the address entry when/if needed.
*
*   INPUTS
*
*       *dev_ptr                A pointer to the device.
*       *target_address         A pointer to the address entry to update.
*       valid_lifetime          The new valid lifetime of the address
*                               entry.
*       preferred_lifetime      The new preferred lifetime of the address
*                               entry.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DEV6_Update_Address_Entry(DV_DEVICE_ENTRY *device,
                               DEV6_IF_ADDRESS *target_address,
                               UINT32 valid_lifetime,
                               UINT32 preferred_lifetime)
{
    /* If the caller wants to update the valid lifetime. */
    if (valid_lifetime != 0)
    {
        /* If this is not a permanent address, unset the expiration timer */
        if (target_address->dev6_valid_lifetime != 0xffffffffUL)
        {
            if (TQ_Timerunset(IP6_Expire_Address_Event, TQ_CLEAR_EXACT,
                              target_address->dev6_valid_lifetime,
                              (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to unset the timer to expire the IPv6 address",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        /* Assign the new value */
        target_address->dev6_valid_lifetime = valid_lifetime;

        /* Set the timestamp of when this lifetime was set. */
        target_address->dev6_timestamp = NU_Retrieve_Clock();

        /* If this is not a permanent address, set the timer to expire it. */
        if (valid_lifetime != 0xffffffffUL)
        {
            if (TQ_Timerset(IP6_Expire_Address_Event, valid_lifetime,
                            valid_lifetime * SCK_Ticks_Per_Second,
                            (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set the timer to expire the new IPv6 address",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* If the caller wants to update the preferred lifetime. */
    if (preferred_lifetime != 0)
    {
        /* If this is not a permanent address, unset the expiration timer */
        if (target_address->dev6_preferred_lifetime != 0xffffffffUL)
        {
            if (TQ_Timerunset(IP6_Deprecate_Address_Event, TQ_CLEAR_EXACT,
                              target_address->dev6_preferred_lifetime,
                              (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to unset the timer to deprecate the IPv6 address",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        /* Assign the new value */
        target_address->dev6_preferred_lifetime = preferred_lifetime;

        /* If this is not a permanent address, set the timer to expire it. */
        if (preferred_lifetime != 0xffffffffUL)
        {
            if (TQ_Timerset(IP6_Deprecate_Address_Event, preferred_lifetime,
                            preferred_lifetime * TICKS_PER_SECOND,
                            (UNSIGNED)(device->dev_index)) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to set the timer to deprecate the new IPv6 address",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

} /* DEV6_Update_Address_Entry */

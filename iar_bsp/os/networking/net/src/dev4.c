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
*   FILENAME
*
*       dev4.c
*
*   DESCRIPTION
*
*       This file is responsible for initializing all of the devices that
*       will be used with a specific instantiation of Nucleus NET.  This
*       code may be changed into a dynamic module in the future.
*
*   DATA STRUCTURES
*
*       DEV_Table
*
*   FUNCTIONS
*
*       DEV_Attach_IP_To_Device
*       DEV_Initialize_IP
*       DEV_Get_Dev_By_Addr
*       DEV_Init_Route
*       DEV_Find_Target_Address
*       DEV_Configure_Link_Local_Addr
*       DEV_Generate_LL_Addr
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_LL_CONFIG == NU_TRUE)
extern  TQ_EVENT            ARP_LL_Event;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

extern DEV_IF_ADDR_ENTRY   NET_Device_Addr_Memory[];
extern UINT8               NET_Device_Addr_Memory_Flags[];

#endif

STATIC STATUS  DEV_Init_Route(DV_DEVICE_ENTRY *, const DEV_IF_ADDR_ENTRY *);

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Attach_IP_To_Device
*
*   DESCRIPTION
*
*       Given a device name, this routine will set the IP number into the
*       network interface table for the device.
*
*   INPUTS
*
*       *name                   A pointer to an ASCII string representing
*                               the device
*       *ip_addr                A pointer to the IP address to be
*                               associated with the device
*       *subnet                 A pointer to the subnet mask to be
*                               associated with the device
*
*   OUTPUTS
*
*       NU_SUCCESS              Device was updated
*       NU_MEM_ALLOC            Memory allocation failure
*       NU_INVALID_PARM         Device does not exist or the user is
*                               adding a link-local address when a globally
*                               routable address already exists on the
*                               interface.
*
****************************************************************************/
INT DEV_Attach_IP_To_Device(const CHAR *name, const UINT8 *ip_addr,
                            const UINT8 *subnet)
{
    DV_DEVICE_ENTRY     *device;
    INT                 status;
    DEV_IF_ADDR_ENTRY   *new_entry = NU_NULL;

#if (INCLUDE_LL_CONFIG == NU_TRUE)
    DEV_IF_ADDR_ENTRY   *current_addr;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT                 j;
#endif

    /* Get a pointer to the device */
    device = DEV_Get_Dev_By_Name(name);

    /* If the device is valid, and this address has not already been added to the
     * device.
     */
    if ( (device != NU_NULL) &&
         (DEV_Find_Target_Address(device, IP_ADDR(ip_addr)) == NU_NULL) )
    {
#if (INCLUDE_LL_CONFIG == NU_TRUE)

        /* If the new address is link-local. */
        if (IP_LL_ADDR(ip_addr))
        {
            /* Ensure there is no routable address on the interface. */
            current_addr = device->dev_addr.dev_addr_list.dv_head;

            while (current_addr)
            {
                /* If this is not an empty entry awaiting DHCP resolution. */
                if (current_addr->dev_entry_ip_addr != 0)
                {
                    /* We cannot have a link-local and routable address on
                     * the same interface.
                     */
                    return (NU_INVALID_PARM);
                }

                current_addr = current_addr->dev_entry_next;
            }
        }

#endif

#if (MIB2_IF_INCLUDE == NU_TRUE)
        /* Set the desired state of the interface in MIB-2 to 1. */
        device->dev_mibInterface.statusAdmin = NU_TRUE;
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        /* Allocate memory for the address list entry */
        status = NU_Allocate_Memory(MEM_Cached,
                                    (VOID**)&new_entry,
                                    sizeof(DEV_IF_ADDR_ENTRY), NU_NO_SUSPEND);
#else
        status = NU_SUCCESS;

        /* Traverse the flag array to find the unused memory location*/
        for (j = 0; j != (NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE); j++)
        {
            /* If this memory is available, use it */
            if (NET_Device_Addr_Memory_Flags[j] != NU_TRUE)
            {
                /* Assign memory to this device */
                new_entry = &NET_Device_Addr_Memory[j];

                /* Turn the memory flag on */
                NET_Device_Addr_Memory_Flags[j] = NU_TRUE;
                break;
            }
        }

        /* If there are no available entries, set an error */
        if (j == (NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE))
            status = NU_NO_MEMORY;
#endif

        if (status != NU_SUCCESS)
            return (NU_NO_MEMORY);

        /* Zero out the memory */
        UTL_Zero(new_entry, sizeof(DEV_IF_ADDR_ENTRY));

        /* Add the entry to the head of the list.  This entry is added
         * to the head of the list to ensure that, if there is an entry
         * undergoing DHCP resolution, this entry is chosen before the
         * DHCP address entry.
         */
        DLL_Insert(&device->dev_addr.dev_addr_list, new_entry,
                   device->dev_addr.dev_addr_list.dv_head);

        /* If the IP address attached is not an empty address awaiting
         * dynamic resolution, initialize it.
         */
        if (IP_ADDR(ip_addr) != NULL_IP)
            status = DEV_Initialize_IP(device, ip_addr, subnet, new_entry);
    }

    else
        status = NU_INVALID_PARM;

    /* Trace log */
    T_DEV_IPv4_IP((char*)name, (UINT8*)ip_addr, (UINT8*)subnet, status, 4);

    return (status);

} /* DEV_Attach_IP_To_Device */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Initialize_IP
*
*   DESCRIPTION
*
*       This routine sets up the address entry structure according to
*       the information provided.
*
*   INPUTS
*
*       *device                 A pointer to the device to which this
*                               address structure belongs.
*       *ip_addr                A pointer to the IP address to be
*                               associated with the address structure.
*       *subnet                 A pointer to the subnet mask to be
*                               associated with the address structure.
*       *new_entry              A pointer to the address structure to
*                               set up.
*
*   OUTPUTS
*
*       NU_SUCCESS              Device was updated
*       NU_MEM_ALLOC            Memory allocation failure
*
****************************************************************************/
STATUS DEV_Initialize_IP(DV_DEVICE_ENTRY *device, const UINT8 *ip_addr,
                         const UINT8 *subnet, DEV_IF_ADDR_ENTRY *new_entry)
{
    STATUS  status;
    UINT8   dv_ip_addr[IP_ADDR_LEN];
    UINT8   dv_netmask[IP_ADDR_LEN];

#if (INCLUDE_LL_CONFIG == NU_TRUE)
    DEV_IF_ADDR_ENTRY   *current_addr;
#endif

#if (INCLUDE_LL_CONFIG == NU_TRUE)

    /* If this address is not a link-local address, ensure there
     * is not a link-local address on the interface also.  If there
     * is, delete the link-local address.
     *
     * RFC 3927 - section 1.4 - IPv4 Link-Local addresses should
     * therefore only be used where stable, routable addresses
     * are not available (such as on ad hoc or isolated networks)
     * or in controlled situations where these limitations and
     * their impact on applications are understood and accepted.
     * This document does not recommend that IPv4 Link-Local
     * addresses and routable addresses be configured
     * simultaneously on the same interface.
     */
    if (!(IP_LL_ADDR(ip_addr)))
    {
        /* Get a pointer to the first address in the list. */
        current_addr = device->dev_addr.dev_addr_list.dv_head;

        /* While there are addresses to process. */
        while (current_addr)
        {
            /* Convert the 32-bit integer into a 4-byte array. */
            PUT32(dv_ip_addr, 0, current_addr->dev_entry_ip_addr);

            /* If this address is link-local. */
            if (IP_LL_ADDR(dv_ip_addr))
            {
                /* Remove the address from the interface. */
                DEV4_Delete_IP_From_Device(device, current_addr);

                break;
            }

            /* Get a pointer to the next address in the list. */
            current_addr = current_addr->dev_entry_next;
        }
    }
#endif

    /*  Copy the IP Address into the interface table. */
    new_entry->dev_entry_ip_addr = IP_ADDR(ip_addr);

    /* If a network mask was not supplied, then choose one based on the
       class of the IP address. Else use the supplied mask. */
    if (IP_ADDR(subnet) == 0)
    {
        if (IP_CLASSC_ADDR(new_entry->dev_entry_ip_addr))
            new_entry->dev_entry_netmask = 0xffffff00UL;
        else if (IP_CLASSB_ADDR(device->dev_addr.dev_addr_list.
                                dv_head->dev_entry_ip_addr))
            new_entry->dev_entry_netmask = 0xffff0000UL;
        else if (IP_CLASSA_ADDR(device->dev_addr.dev_addr_list.
                                dv_head->dev_entry_ip_addr))
            new_entry->dev_entry_netmask = 0xff000000UL;
    }
    else
    {
        new_entry->dev_entry_netmask = IP_ADDR(subnet);
    }

    PUT32(dv_ip_addr, 0, new_entry->dev_entry_ip_addr);
    PUT32(dv_netmask, 0, new_entry->dev_entry_netmask);

    /* Fill in the network number. */
    new_entry->dev_entry_net = new_entry->dev_entry_ip_addr &
                               new_entry->dev_entry_netmask;

    /* Set the network broadcast address. */
    new_entry->dev_entry_net_brdcast =
        new_entry->dev_entry_ip_addr | (~new_entry->dev_entry_netmask);

#if (INCLUDE_SR_SNMP == NU_TRUE)

    /* Update the SNMP IP Address Translation Table. */
    SNMP_ipNetToMediaTableUpdate(SNMP_ADD, device->dev_index,
                                 device->dev_mac_addr, dv_ip_addr, 4);

    /* Update the address translation group. */
    SNMP_atTableUpdate(SNMP_ADD, (INT)(device->dev_index),
                       device->dev_mac_addr, dv_ip_addr);

    SNMP_ipAdEntUpdate(SNMP_ADD, (UINT32)(device->dev_index),
                       dv_ip_addr, dv_netmask, 1, 0);
#endif

    /* Set the status for MIB-II. */
#if (MIB2_IF_INCLUDE == NU_TRUE)
    MIB2_Set_IfStatusOper(device, MIB2_IF_OPER_STAT_UP);
#endif

    /* Init the route to the directly connected network. Not used
       for PPP links. */
    if (!(device->dev_flags & DV_POINTTOPOINT))
        status = DEV_Init_Route(device, new_entry);
    else
        status = NU_SUCCESS;

    /* Indicate that the device is Running. */
    device->dev_flags |= DV_UP;

    /* Trace log */
    T_DEV_UP_STATUS(device->dev_net_if_name, 0);

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    /* If this is the first IP address added to this device and
     * multicasting support is desired then join the all hosts group
     * (224.0.0.1) on all devices that support IP multicasting.
     */
    if ( (device->dev_addr.dev_addr_list.dv_head == new_entry) &&
         (device->dev_flags & DV_MULTICAST) )
    {
        if (IGMP_Initialize(device) != NU_SUCCESS)
            NLOG_Error_Log("Failed to initialized IGMP", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

#endif /* INCLUDE_IP_MULTICASTING */

#if (INCLUDE_ARP == NU_TRUE)

    /* Send a gratuitous ARP. */
    if ( (!((device->dev_flags & DV_NOARP) || (device->dev_flags & DV_POINTTOPOINT)))
#if (INCLUDE_LL_CONFIG == NU_TRUE)
         && (!(IP_LL_ADDR(dv_ip_addr))) )
#else
        )
#endif
    {
        if (ARP_Request(device, &new_entry->dev_entry_ip_addr,
                        (UINT8 *)"\0\0\0\0\0\0", EARP, ARPREQ) != NU_SUCCESS)
            NLOG_Error_Log("Failed to transmit gratuitous ARP", NERR_SEVERE,
                           __FILE__, __LINE__);
    }
#endif

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)

    /* Only add a new route if this is not the loopback device. */
    if (device->dev_type != DVT_LOOP)
    {
        /* Add a host route to the loopback device for this devices IP
           address. NOTE: since the loopback device is added first
           during NET initialization it will always be the first
           device in the list. */
        if (RTAB4_Add_Route(DEV_Table.dv_head, IP_ADDR(ip_addr), 0xFFFFFFFFUL,
                            0x7F000001UL, (INT32)(RT_UP | RT_STATIC |
                            RT_GATEWAY | RT_HOST | RT_LOCAL | RT_SILENT)) != NU_SUCCESS)
            NLOG_Error_Log("Failed to add route through loopback", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

#endif

#if (INCLUDE_MDNS == NU_TRUE)
    if ( (status == NU_SUCCESS) && (device->dev_type != DVT_LOOP) )
    {
        status = MDNS_Register_Local_Host(device, (UINT8*)ip_addr, NU_FAMILY_IP);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to add mDNS entry for IP address", NERR_SEVERE,
                           __FILE__, __LINE__);

            /* The IP initialization should not fail because a host record
             * could not be created.
             */
            status = NU_SUCCESS;
        }
    }
#endif

    return (status);

} /* DEV_Initialize_IP */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Init_Route
*
*   DESCRIPTION
*
*       Given a device structure, this routine will initialize the route
*       for the device.
*
*   INPUTS
*
*       *device                 Device to initialize
*       *new_entry              Pointer to the address structure
*                               to set up
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful.
*       -1                      If failure.
*
****************************************************************************/
STATIC STATUS DEV_Init_Route(DV_DEVICE_ENTRY *device,
                             const DEV_IF_ADDR_ENTRY *new_entry)
{
    UINT32      dest;
    STATUS      status;

    dest = new_entry->dev_entry_ip_addr & new_entry->dev_entry_netmask;

    /* The bitwise anding and checking of the destination address with
       the loopback address is done so that loopback routes will be
       marked as RT_SILENT routes and thus will not be part of
       routing advertisement messages. */
    status = RTAB4_Add_Route(device, dest, new_entry->dev_entry_netmask,
                             new_entry->dev_entry_ip_addr,
                             (UINT32)(RT_UP | RT_STATIC | RT_LOCAL |
                             (((dest & 0x7F000000UL) == 0x7F000000UL) ? RT_SILENT : 0)));

    /* If a route already exists, do not return an error. */
    if (status == NU_NO_ACTION)
        status = NU_SUCCESS;

    return (status);

} /* DEV_Init_Route */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Get_Dev_By_Addr
*
*   DESCRIPTION
*
*       Find the device by IP address
*
*   INPUTS
*
*       *addr                   A pointer to the IP address associated
*                               with the device.
*
*   OUTPUTS
*
*       DV_DEVICE_ENTRY*        Pointer to the device structure.
*       NU_NULL                 There is no device with that IP address.
*
****************************************************************************/
DV_DEVICE_ENTRY *DEV_Get_Dev_By_Addr(const UINT8 *addr)
{
    DV_DEVICE_ENTRY     *dev;
    DEV_IF_ADDR_ENTRY   *current_addr;

    /* Look at the first device in the list. */
    dev = DEV_Table.dv_head;

    /*  Search for a match.  */
    while (dev != NU_NULL)
    {
        /* Check the list of addresses on the device */
        current_addr = DEV_Find_Target_Address(dev, IP_ADDR(addr));

        /* If the address was not found, move on to the next device */
        if (current_addr == NU_NULL)
            dev = dev->dev_next;

        /* Otherwise, exit the loop */
        else
            break;
    }

    /* Return a pointer to the device or NULL if no matching address
     * was found.
     */
    return (dev);

} /* DEV_Get_Dev_By_Addr */

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Find_Target_Address
*
*   DESCRIPTION
*
*       This function will find a matching valid address on the
*       given interface.
*
*   INPUTS
*
*       *device                 The device on which to search.
*       target_addr             The address to match.
*
*   OUTPUTS
*
*       A pointer to the structure that holds the matching address or
*       NU_NULL if not found.
*
*************************************************************************/
DEV_IF_ADDR_ENTRY *DEV_Find_Target_Address(const DV_DEVICE_ENTRY *device,
                                           const UINT32 target_addr)
{
    DEV_IF_ADDR_ENTRY   *target_address;

    /* Get a pointer to the first entry in the device's list of
     * addresses.
     */
    target_address = device->dev_addr.dev_addr_list.dv_head;

    /* Search through the device's list of addresses for the address
     * that matches the Target Address in the packet.
     */
    while (target_address)
    {
        if (target_address->dev_entry_ip_addr == target_addr)
            break;

        target_address = target_address->dev_entry_next;
    }

    return (target_address);

} /* DEV_Find_Target_Address */

#if (INCLUDE_LL_CONFIG == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Configure_Link_Local_Addr
*
*   DESCRIPTION
*
*       This function invokes Dynamic Configuration of IPv4 Link-Local
*       Addresses, per RFC 3927, on an interface.
*
*   INPUTS
*
*       *device                 The device on which to invoke Dynamic
*                               Configuration of IPv4 Link-Local Addresses.
*       *ip_addr                A pointer to the preferred link-local
*                               address to use or NU_NULL if no address
*                               is preferred.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DEV_Configure_Link_Local_Addr(DV_DEVICE_ENTRY *device, UINT8 *ip_addr)
{
    UNSIGNED    timeout;

    /* Mark the device as UP and RUNNING. */
    device->dev_flags |= (DV_UP | DV_RUNNING);

    /* If an address was specified by the user. */
    if (ip_addr)
    {
        /* Copy the address. */
        device->dev_addr.dev_link_local_addr = IP_ADDR(ip_addr);
    }

    /* Otherwise, generate a random address. */
    else
    {
        device->dev_addr.dev_link_local_addr = DEV_Generate_LL_Addr();
    }

    /* Initialize the link-local parameters for the interface. */
    device->dev_ll_state = LL_STATE_PROBE_WAIT;
    device->dev_ll_probe_count = 0;
    device->dev_ll_conflict_count = 0;
    device->dev_ll_announce_count = 0;

    /* RFC 3927 - section 2.2.1 - When ready to begin probing, the host
     * should then wait for a random time interval selected uniformly in
     * the range zero to PROBE_WAIT seconds.
     */
    timeout = UTL_Rand() % LL_PROBE_WAIT_TIME;

    /* Set the timer. */
    if (TQ_Timerset(ARP_LL_Event, (UNSIGNED)device->dev_index,
                    timeout, 0) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to set timer for link-local address", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

} /* DEV_Configure_Link_Local_Addr */

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Generate_LL_Addr
*
*   DESCRIPTION
*
*       This function generates a random IPv4 link-local IP address
*       in the range 169.254.1.0 -> 169.254.254.255.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       A randomly generated 32-bit link-local IP address.
*
*************************************************************************/
UINT32 DEV_Generate_LL_Addr(VOID)
{
    UINT8   link_local_addr[IP_ADDR_LEN];

    /* RFC 3927 - section 2.1 - The IPv4 prefix 169.254/16 is
     * registered with the IANA for this purpose.
     */
    link_local_addr[0] = 169;
    link_local_addr[1] = 254;

    /* RFC 3927 - section 2.1 - The first 256 and last 256 addresses
     * in the 169.254/16 prefix are reserved for future use and
     * MUST NOT be selected by a host using this dynamic configuration
     * mechanism.
     */
    link_local_addr[2] = UTL_Rand() % 254;
    link_local_addr[3] = UTL_Rand() % 255;

    /* Ensure the value for the 3rd byte is valid. */
    while (link_local_addr[2] == 0)
    {
        link_local_addr[2] = UTL_Rand() % 254;
    }

    return (IP_ADDR(link_local_addr));

} /* DEV_Generate_LL_Addr */
#endif

#endif

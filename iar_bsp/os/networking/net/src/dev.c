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
*       dev.c
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
*       DEV_Next_Index
*
*   FUNCTIONS
*
*       NU_Init_Devices
*       DEV_Init_Devices
*       DEV_Get_Dev_By_Name
*       DEV_Get_Dev_By_Index
*       DEV_Get_Dev_For_Vector
*       DEV_Recover_TX_Buffers
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nud6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/*  List of devices. */
DV_DEVICE_LIST    DEV_Table = {NU_NULL, NU_NULL};

/*  Index counter for the devices. */
UINT32            DEV_Next_Index = 0;

#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )
UINT32            DHCP_IA_ID_Value = DHCP_IAID_START;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for the address structures for each device */
DEV_IF_ADDR_ENTRY   NET_Device_Addr_Memory[NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE];

/* Declare memory flags for the above declared array (memory) */
UINT8               NET_Device_Addr_Memory_Flags[NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE] = {0};

/* Declare Memory for Devices */
DV_DEVICE_ENTRY     NET_Device_Memory[NET_MAX_DEVICES];

/* Declare memory flags for the above declared array (memory) */
UINT8               NET_Device_Memory_Flags[NET_MAX_DEVICES] = {0};

/* Declare memory for physical devices. */
DV_PHY_DEVICE       NET_Phy_Device_Memory[NET_MAX_DEVICES];

#endif

/**************************************************************************
*
*   FUNCTION
*
*       NU_Init_Devices
*
*   DESCRIPTION
*
*       This function will initialize all of the devices that will be used
*       by Nucleus NET.  It will be hard coded to set up the devices based
*       on the developer's requirements.
*
*   INPUTS
*
*       devices                 Pointer to device to init.
*       dev_count               Number of such devices.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful init.
*       NU_INVALID_PARM         An input parameter is invalid.
*       -1                      Failure.
*
****************************************************************************/
STATUS NU_Init_Devices(DEV_DEVICE *devices, INT dev_count)
{
    STATUS      status;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input parameters */
    if ( (devices == NU_NULL) || (dev_count <= 0) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        status = DEV_Init_Devices(devices, dev_count);

        /* Release the semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("NU_Init_Devices could not release semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    else
        NLOG_Error_Log("NU_Init_Devices could not obtain semaphore",
                       NERR_FATAL, __FILE__, __LINE__);

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* NU_Init_Devices */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Init_Devices
*
*   DESCRIPTION
*
*       This function will initialize all of the devices that will be used
*       by Nucleus NET.  It will be hard coded to set up the devices based
*       on the developer's requirements.
*
*   INPUTS
*
*       devices                 Pointer to device to init.
*       dev_count               Number of such devices.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful init.
*       -1                      Failure.
*
****************************************************************************/
STATUS DEV_Init_Devices(const DEV_DEVICE *devices, INT dev_count)
{
    STATUS                  status = NU_SUCCESS;
    DV_DEVICE_ENTRY         *dev_ptr = NU_NULL;
    INT                     i;
    UINT16                  length;

#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )
    UINT8                   zero_addr[6] = {0, 0, 0, 0, 0, 0};
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    DEV_IF_ADDR_ENTRY       *new_entry = NU_NULL;
#endif

#if (INCLUDE_IF_STACK == NU_TRUE)
    MIB2_IF_STACK_STRUCT    *if_stack_entry;
    MIB2_IF_STACK_STRUCT    if_stack_node;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT                     j = 0;          /* Counter to traverse an array */
#endif

    /* Add each new interface to the system */
    for (i = 0; i < dev_count; i++)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        /* Get the size of the device structure. */
        length = sizeof(struct _DV_DEVICE_ENTRY);

        /* Check if the device is not a virtual device. */
        if (!(devices[i].dv_flags & DV_VIRTUAL_DEV))
        {
            /* Add the size of the physical device structure. */
            length += sizeof(struct _DV_PHY_DEVICE);
        }

        /* Allocate memory for this device entry. */
        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&dev_ptr,
                                    (UNSIGNED)length, (UNSIGNED)NU_NO_SUSPEND);

        /* If memory allocation has been successful. */
        if (status == NU_SUCCESS)
        {
            /* Clear out the structure. */
            UTL_Zero(dev_ptr, length);

            /* Check if the device is not virtual. */
            if (!(devices[i].dv_flags & DV_VIRTUAL_DEV))
            {
                /* Assign memory to the physical device. */
                dev_ptr->dev_physical =
                    (DV_PHY_DEVICE*)(((CHAR HUGE *)dev_ptr) +
                                     (sizeof(struct _DV_DEVICE_ENTRY)));
            }
        }

#else
        /* Traverse the flag array to find the unused memory location. */
        for (; (NET_Device_Memory_Flags[j] != NU_FALSE) && (j != NET_MAX_DEVICES); j++)
            ;

        if (j != NET_MAX_DEVICES)
        {
            /* Assign memory to this device. */
            dev_ptr = &NET_Device_Memory[j];

            /* Turn the memory flag on. */
            NET_Device_Memory_Flags[j] = NU_TRUE;

            /* Check if the device is not a virtual device. */
            if (!(devices[i].dv_flags & DV_VIRTUAL_DEV))
            {
                /* Also assign memory to the physical device. */
                dev_ptr->dev_physical = &NET_Phy_Device_Memory[j];
            }

            /* Set the status. */
            status = NU_SUCCESS;
        }

        /* If an unused memory could not be found. */
        else
            status = NU_NO_MEMORY;
#endif

        if (status == NU_SUCCESS)
        {
            /* Add the new device entry to the list of devices. */
            DLL_Enqueue(&DEV_Table, dev_ptr);

            /* Set the routing metric for this device. By default the metric will
               always be set to 1. Later if a user chooses to use RIP2 or OSPF
               with this device, the metric can be changed. */
            dev_ptr->dev_metric = DEV_DEFAULT_METRIC;

            /* Now initialize the fields that we can.  The rest will be initialized
               by the driver. */

            /* Set the unit number.  This number will be unique for all devices.
               The first will be numbered at 0 each succeeding device will be
               numbered contiguously. */
            dev_ptr->dev_index = DEV_Next_Index++;

            /* Initialize the device's name. If the supplied name is too long only
               copy that portion which will fit and still leave room for the null
               terminator. */
            length = (UINT16)(strlen(devices[i].dv_name));

            if (length > (DEV_NAME_LENGTH - 1))
                length = DEV_NAME_LENGTH - 1;

            memcpy(dev_ptr->dev_net_if_name, devices[i].dv_name,
                   (unsigned int)length);

            dev_ptr->dev_net_if_name[length] = 0;

            /* Get the flags set by the application */
            dev_ptr->dev_flags = devices[i].dv_flags;

            /* Set the driver options. This may or may not be used
               by the driver, it is driver specific. */
            dev_ptr->dev_driver_options = devices[i].dv_driver_options;

            /* Get dev handle */
            dev_ptr->dev_handle = devices[i].dev_handle;

            /* Initialize the outgoing ICMP error interval. */
            dev_ptr->dev_error_msg_rate_limit = ICMP_ERROR_MSG_RATE_LIMIT;
            dev_ptr->dev_max_error_msg = ICMP_ERROR_MSG_COUNT_RATE_LIMIT;

#if ( (INCLUDE_VLAN == NU_TRUE) && (USE_SW_VLAN_METHOD) )

            if (devices[i].dv_type == DVT_VLAN)
            {
                /* set VLAN device type */
                dev_ptr->dev_type = (UINT8) DVT_VLAN;

                /* reload device options field which points to VLAN */
                /* device initialization structure.                 */
                dev_ptr->dev_driver_options = devices[i].dv_driver_options;
            }
#endif
            if (!(devices[i].dv_flags & DV_VIRTUAL_DEV))
            {
                /* Is this a serial link? */
                if (devices[i].dv_flags & DV_POINTTOPOINT)
                {
                    /* Store the options that pertain only to a serial link.
                       All of these may not be used. It depends on the UART. */
                    dev_ptr->dev_com_port  = devices[i].dv_hw.uart.com_port;
                    dev_ptr->dev_baud_rate = devices[i].dv_hw.uart.baud_rate;
                    dev_ptr->dev_data_mode = devices[i].dv_hw.uart.data_mode;
                    dev_ptr->dev_parity    = devices[i].dv_hw.uart.parity;
                    dev_ptr->dev_stop_bits = devices[i].dv_hw.uart.stop_bits;
                    dev_ptr->dev_data_bits = devices[i].dv_hw.uart.data_bits;
                }
                else
                {
                    /* Store the options that pertain only to an ethernet link */
                    dev_ptr->dev_irq     = devices[i].dv_hw.ether.dv_irq;
                    dev_ptr->dev_sm_addr = devices[i].dv_hw.ether.dv_shared_addr;
                    dev_ptr->dev_io_addr = devices[i].dv_hw.ether.dv_io_addr;

#ifdef NU_SIMULATION
                    /* Nucleus SIM requires the MAC address to be passed from the
                     * application layer.
                     */
                    memcpy(dev_ptr->dev_mac_addr, devices[i].dv_hw.ether.dv_mac_addr,
                           DADDLEN);
#endif
                }
            }

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )
            /* Initialize the Interface. */
            if (MIB2_Interface_Init(dev_ptr) != NU_SUCCESS)
                NLOG_Error_Log("Could not initialize MIB-2 parameters for interface",
                               NERR_SEVERE, __FILE__, __LINE__);
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) && \
  (INCLUDE_IPV6_MIB == NU_TRUE) )

            if (devices[i].dv_flags & DV6_IPV6)
            {
                if (IP6_MIB_Init(dev_ptr) != NU_SUCCESS)
                    NLOG_Error_Log("Could not initialize IPv6 MIBs parameters for interface",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
#endif

            SNMP_ifCreate((INT32)dev_ptr->dev_index);

            /* Update the total number of interfaces that have been registered
               with SNMP. The index is 0 based, so add 1. */
            SNMP_ifTotalInterfaces((INT32)(dev_ptr->dev_index + 1));

            status = (devices[i].dv_init)(dev_ptr);

            if (status == NU_SUCCESS)
            {
#if (INCLUDE_IF_STACK == NU_TRUE)

                /* Getting stack entry to check if stack entry exists. */
                if_stack_entry =
                    MIB2_If_Stack_Get_HI_Entry(dev_ptr->dev_index + 1,
                                               NU_FALSE);

                /* If stack entry was not added by Device's
                 * initialization function then add the default stack
                 * entries.
                 */
                if (if_stack_entry == NU_NULL)
                {
                    /* Clearing out the stack entry node. */
                    UTL_Zero(&if_stack_node, sizeof(MIB2_IF_STACK_STRUCT));

                    /* Setting the stack entries as 'active'. */
                    MIB2_IF_STACK_ACTIVATE(&if_stack_node);

                    /* Setting higher layer as current device. */
                    if_stack_node.mib2_higher_layer = dev_ptr;

                    /* Adding a stack entry showing that no device is
                     * below this device.
                     */
                    MIB2_If_Stack_Add_Entry(&if_stack_node);

                    /* Setting higher layer as NU_NULL. */
                    if_stack_node.mib2_higher_layer = NU_NULL;

                    /* Setting lower layer as current device. */
                    if_stack_node.mib2_lower_layer = dev_ptr;

                    /* Adding a stack entry showing that no device is
                     * on top of this device.
                     */
                    MIB2_If_Stack_Add_Entry(&if_stack_node);
                }

#endif /* (INCLUDE_IF_STACK == NU_TRUE) */

                /* Indicate that the device is Running. */
                dev_ptr->dev_flags |= DV_RUNNING;

                /* RFC 1122 - section 3.3.2 - The largest datagram size that can
                 * be reassembled MUST be greater than or equal to 576 and SHOULD
                 * be greater than or equal to the MTU of the connected network(s).
                 */
                dev_ptr->dev_reasm_max_size = MAX_REASM_MAX_SIZE;

                if (dev_ptr->dev_reasm_max_size < MIN_REASM_MAX_SIZE)
                    dev_ptr->dev_reasm_max_size = MIN_REASM_MAX_SIZE;

                /* If an IP address was specified and this is not a PPP device then
                   go ahead and attach the IP address and the subnet mask.  Otherwise
                   it is assumed Bootp will be used to discover the IP address and
                   attach it at a later time. */
                if (!(devices[i].dv_flags & DV_POINTTOPOINT))
                {
#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )

                    /* If this is not the loopback interface, initialize the DHCP
                     * parameters.
                     */
                    if (dev_ptr->dev_type != DVT_LOOP)
                    {
                        /* Set the IAID for the interface. */
                        dev_ptr->dev_dhcp_iaid = DHCP_IA_ID_Value ++;

                        /* If a link-layer address has not been assigned for
                         * the DUID, use this interface's link-layer address.
                         */
                        if (memcmp(DHCP_Duid.duid_ll_addr, zero_addr,
                                   DADDLEN) == 0)
                        {
                            /* Copy the address. */
                            NU_BLOCK_COPY(DHCP_Duid.duid_ll_addr,
                                          dev_ptr->dev_mac_addr, DADDLEN);
                        }
                    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                    if (*(UINT32 *)devices[i].dv_ip_addr != 0)
                    {
                        status =
                            DEV_Attach_IP_To_Device(dev_ptr->dev_net_if_name,
                                                    (UINT8*)devices[i].dv_ip_addr,
                                                    (UINT8*)devices[i].dv_subnet_mask);
                    }

                    else
                    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        /* Allocate memory for the address list entry */
                        status = NU_Allocate_Memory(MEM_Cached,
                                                    (VOID**)&new_entry,
                                                    sizeof(DEV_IF_ADDR_ENTRY),
                                                    NU_NO_SUSPEND);
#else
                        /* Traverse the flag array to find the unused memory
                         * location
                         */
                        for (j = 0;
                             j != (NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE);
                             j++)
                        {
                            /* If this memory is available, use it */
                            if (NET_Device_Addr_Memory_Flags[j] != NU_TRUE)
                            {
                                /* Assign memory to this device */
                                new_entry = &NET_Device_Addr_Memory[j];

                                /* Turn the memory flag on */
                                NET_Device_Addr_Memory_Flags[j] = NU_TRUE;
                                status = NU_SUCCESS;

                                break;
                            }
                        }

                        /* If there are no available entries, set an error */
                        if (j == (NET_MAX_DEVICES * NET_MAX_ADDRS_PER_DEVICE))
                            status = NU_NO_MEMORY;
#endif
                        if (status == NU_SUCCESS)
                        {
                            /* Zero out the memory */
                            UTL_Zero(new_entry, sizeof(DEV_IF_ADDR_ENTRY));

                            /* Add the entry to the list */
                            DLL_Enqueue(&dev_ptr->dev_addr.dev_addr_list,
                                        new_entry);
                        }
                    }

#if (INCLUDE_LL_CONFIG == NU_TRUE)

                    /* If the user wants to invoke Dynamic Configuration of
                     * IPv4 Link-Local Addresses on the interface, do so now.
                     */
                    if (dev_ptr->dev_flags & DV_CFG_IPV4_LL_ADDR)
                    {
                        DEV_Configure_Link_Local_Addr(dev_ptr, NU_NULL);
                    }
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

                    /* Initialize the IPv6 enabled interface if it is not a virtual
                     * interface.
                     */
                    if (
#if (INCLUDE_IPV4 == NU_TRUE)
                         (status == NU_SUCCESS) &&
#endif
                         (dev_ptr->dev_flags & DV6_IPV6) &&
                         (!(dev_ptr->dev_flags & (DV6_VIRTUAL_DEV | DV_POINTTOPOINT))) )
                    {
                        status =
                            DEV6_Init_Device(dev_ptr, &devices[i]);

                        if (status == NU_SUCCESS)
                        {
                            /* If it is the Loopback device, then attach IP to it.
                             */
                            if (dev_ptr->dev_type == DVT_LOOP)
                            {
                                status =
                                    DEV6_Attach_IP_To_Device(dev_ptr->dev_net_if_name,
                                                             devices[i].dv6_ip_addr, 0);
                            }
                        }
                    }
#endif /* INCLUDE_IPV6 */

#if (INCLUDE_MDNS == NU_TRUE)

                    if (dev_ptr->dev_type == DVT_ETHER)
                    {
                        /* Create a host name for this interface. */
                        dev_ptr->dev_host_name =
                            MDNS_Initialize_Hostname(dev_ptr->dev_index);
                    }
#endif
                }
#if (INCLUDE_IPV6 == NU_TRUE)
                else
                {
                    if (dev_ptr->dev_flags & DV6_IPV6)
                    {
                        /* It is a PPP device, Initialize IPv6 on it */
                        status = DEV6_Init_Device(dev_ptr, &devices[i]);
                    }
                }
#endif /* (INCLUDE_IPV6 == NU_TRUE) */
            }

            /* If initialization was not successful. */
            if (status != NU_SUCCESS)
            {
                DEV_Remove_Device(dev_ptr, 0);
            }

            /* Trace log */
            T_DEV_LIST(dev_ptr->dev_net_if_name, dev_ptr->dev_mac_addr, dev_ptr->dev_type, dev_ptr->dev_flags, status, 6);
        }
    }

    return (status);

} /* DEV_Init_Devices */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Get_Dev_For_Vector
*
*   DESCRIPTION
*
*       Given a vector number, this routine will find the device that
*       resides there and return the device table address.
*
*   INPUTS
*
*       vector                  The vector for which the requester is
*                               finding a device
*
*   OUTPUTS
*
*       DV_DEVICE_ENTRY*        Address of the device table entry for the
*                               device found
*       NU_NULL                 The device was not found.
*
****************************************************************************/
DV_DEVICE_ENTRY *DEV_Get_Dev_For_Vector(INT vector)
{
    DV_DEVICE_ENTRY   *temp_dev_table;

    /*  Look at the first in the list. */
    temp_dev_table = DEV_Table.dv_head;

    /*  Search for a match.  */
    while (temp_dev_table != NU_NULL)
    {
        /* Check if the current device is not virtual and the vector
         * matches.
         */
        if ( (!(temp_dev_table->dev_flags & DV_VIRTUAL_DEV)) &&
             (temp_dev_table -> dev_vect == (UINT16)vector) )
            break;

        temp_dev_table = temp_dev_table->dev_next;
    }

    return (temp_dev_table);

}  /* DEV_Get_Dev_For_Vector */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Get_Device_By_Name
*
*   DESCRIPTION
*
*       Given a device name, this routine will return a pointer to the
*       device structure of the device that was named.
*
*   INPUTS
*
*       *name                   A pointer to an ASCII string representing
*                               the device
*
*   OUTPUTS
*
*       DV_DEVICE_ENTRY*        Pointer to the device structure of that name.
*       NU_NULL                 A device by that name does not exist.
*
****************************************************************************/
DV_DEVICE_ENTRY *DEV_Get_Dev_By_Name(const CHAR *name)
{
    DV_DEVICE_ENTRY   *dev;

    if (name == NU_NULL)
        return (NU_NULL);

    /*  Look at the first in the list. */
    dev = DEV_Table.dv_head;

    /*  Search for a match.  */
    while ( (dev != NU_NULL) &&
            (strcmp(dev->dev_net_if_name, name) != 0) )
        dev = dev->dev_next;

    return (dev);

} /* DEV_Get_Dev_By_Name */

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Get_Dev_By_Index
*
*   DESCRIPTION
*
*       Given a device index, this routine will return a pointer to the
*       device structure of the device.
*
*   INPUTS
*
*       device index            Index that it is looking for
*
*   OUTPUTS
*
*       DV_DEVICE_ENTRY*        Pointer to the device structure of that
*                               index.
*       NU_NULL                 A device with the index does not exist.
*
****************************************************************************/
DV_DEVICE_ENTRY *DEV_Get_Dev_By_Index(UINT32 device_index)
{
    DV_DEVICE_ENTRY   *dev;

    /*  Look at the first in the list. */
    dev = DEV_Table.dv_head;

    /*  Search for a match.  */
    while ( (dev != NU_NULL) && (dev->dev_index != device_index) )
        dev = dev->dev_next;

    return (dev);

} /* DEV_Get_Dev_By_Index */

/*****************************************************************************
*
*   FUNCTION
*
*       DEV_Recover_TX_Buffers
*
*   DESCRIPTION
*
*       This function "frees" the buffers for the first packet on a devices
*       transmit queue, depending on the packet the buffers will be placed on
*       the free buffer list or on the TCP ports retransmit list. This is done
*       after a driver has completed transmitting a packet and needs to
*       return the buffers to the stack.
*
*   INPUTS
*
*       *device                 Pointer to the device that has completed
*                               transmission of a packet.
*
*   OUTPUTS
*
*       None.
*
*****************************************************************************/
VOID DEV_Recover_TX_Buffers(DV_DEVICE_ENTRY *device)
{
    NET_BUFFER  *buf_ptr;

    /* If there is an item on the transmit list (there should be every time we
     * get here) then remove it because it has just been successfully
     * transmitted. */
    if ( (device) && (device->dev_transq.head) )
    {
        /* Pull the transmitted packet from the transmit queue. */
        buf_ptr = MEM_Buffer_Dequeue(&device->dev_transq);

        /* Free the buffers onto the appropriate free lists */
        MEM_Multiple_Buffer_Chain_Free(buf_ptr);

        /* Decrement the Q length for the device transmit Q.*/
        --(device->dev_transq_length);
        
        /* Trace log */
        T_DEV_TRANSQ_LEN(device->dev_net_if_name, device->dev_transq_length);
    }

} /* DEV_Recover_TX_Buffers */

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
*       ip_fcd.c
*
* DESCRIPTION
*
*       This file contains the implementation of IP_Find_Configured_Device.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Find_Configured_Device
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       IP_Find_Configured_Device
*
*   DESCRIPTION
*
*       This function returns a pointer to the first physical device in
*       the system that has an IP address.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       DV_DEVICE_ENTRY*        A pointer to a device with an IP address
*       NU_NULL                 There are no devices with an IP address
*
*************************************************************************/
DV_DEVICE_ENTRY *IP_Find_Configured_Device(VOID)
{
    DV_DEVICE_ENTRY     *current_dev;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;

    /* Get a pointer to the first device on the node */
    current_dev = DEV_Table.dv_head;

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)

    /* Get the first physical device in the system */
    current_dev = current_dev->dev_next;

#endif

    while (current_dev)
    {
        /* If there is an IPv4 address on the device. */
        dev_addr_entry = current_dev->dev_addr.dev_addr_list.dv_head;

        while (dev_addr_entry)
        {
            if (dev_addr_entry->dev_entry_ip_addr != 0)
                break;

            /* Get a pointer to the next address entry in the list of
             * addresses for the device.
             */
            dev_addr_entry = dev_addr_entry->dev_entry_next;
        }

        /* If an entry was not found on this device, get a pointer
         * to the next device in the system.
         */
        if (dev_addr_entry == NU_NULL)
            current_dev = current_dev->dev_next;
        else
            break;
    }

    return (current_dev);

} /* IP_Find_Configured_Device */

#endif

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
*       dev_sup.c
*
* DESCRIPTION
*
*       This file contains supplemental routines for dev.c.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       DEV_Get_Ether_Address
*       DEV_Get_Next_By_Addr
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Get_Ether_Address
*
*   DESCRIPTION
*
*       Given a device name, this routine will get the ethernet address of
*       the device.
*
*   INPUTS
*
*       *name                   Pointer to an ASCII string representing
*                               the device
*       *ether_addr             The container for the ethernet address
*                               to be returned.
*
*   OUTPUTS
*
*       NU_SUCCESS              Device was updated
*       -1                      Device was NOT updated
*
****************************************************************************/
INT DEV_Get_Ether_Address(const CHAR *name, UINT8 *ether_addr)
{
    DV_DEVICE_ENTRY   *temp_dev_table;
    INT         status = NU_SUCCESS;
    INT         i;

    /*  Look at the first in the list. */
    temp_dev_table = DEV_Table.dv_head;

    /*  Search for a match.  */
    while ((temp_dev_table != NU_NULL) &&
        (strcmp(temp_dev_table -> dev_net_if_name, name) != 0))
        temp_dev_table = temp_dev_table -> dev_next;

    /*  If we found one, then set up the IP address.  */
    if (temp_dev_table != NU_NULL)
    {
        /*  Copy the IP Address into the interface table. */
        for (i = 0; i < 6; i++)
            ether_addr[i] = temp_dev_table -> dev_mac_addr[i];
    }
    else
        status = -1;

    return (status);

} /* DEV_Get_Ether_Address */

#if (INCLUDE_IPV4 == NU_TRUE)

/**************************************************************************
*
*   FUNCTION
*
*       DEV_Get_Next_By_Addr
*
*   DESCRIPTION
*
*       Find the device with the next IP address
*
*   INPUTS
*
*       *addr                   A pointer to the IP address of the current
*                               device.
*
*   OUTPUTS
*
*       DEV_IF_ADDR_ENTRY*      Pointer to the next address structure.
*       NU_NULL                 There is no next device.
*
****************************************************************************/
DEV_IF_ADDR_ENTRY *DEV_Get_Next_By_Addr(UINT8 *addr)
{
    DV_DEVICE_ENTRY     *dev;
    DEV_IF_ADDR_ENTRY   *current_addr, *next_addr = NU_NULL;

    /*  Look at the first in the list. */
    dev = DEV_Table.dv_head;

    /* Look for the smallest IP which is "greater" than the one passed. */
    while (dev != NU_NULL)
    {
        /* Get a pointer to the first address entry in the list of
         * addresses for the device.
         */
        current_addr = dev->dev_addr.dev_addr_list.dv_head;

        /* Traverse the list of addresses for the device */
        while (current_addr)
        {
            /* If this address is greater than the address passed into
             * the function.
             */
            if (current_addr->dev_entry_ip_addr > IP_ADDR(addr))
            {
                /* If the next_addr has not been set or this address
                 * is less than the next_addr address.
                 */
                if ( (next_addr == NU_NULL) ||
                     (current_addr->dev_entry_ip_addr < next_addr->dev_entry_ip_addr) )
                    next_addr = current_addr;
            }

            /* Get a pointer to the next address in the list of
             * addresses for the device.
             */
            current_addr = current_addr->dev_entry_next;
        }

        /* Get a pointer to the next device in the list of devices */
        dev = dev->dev_next;
    }

    /* update addr with the device address. */
    if (next_addr != NU_NULL)
        PUT32(addr, 0, next_addr->dev_entry_ip_addr);

    return (next_addr);

} /* DEV_Get_Next_By_Addr */

#endif

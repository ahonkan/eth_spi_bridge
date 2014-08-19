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
*       dev_dafd.c
*
*   DESCRIPTION
*
*       This file contains the implementation of
*       DEV_Detach_Addrs_From_Device.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DEV_Detach_Addrs_From_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/prefix6.h"
#include "networking/nc6_eth.h"
#include "networking/nc6.h"
#include "networking/defrtr6.h"
#endif

#if ( (INCLUDE_ARP == NU_TRUE) || (INCLUDE_RARP == NU_TRUE) )
extern ARP_RESOLVE_LIST  ARP_Res_List;
extern ARP_ENTRY ARP_Cache[];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Detach_Addrs_From_Device
*
*   DESCRIPTION
*
*       This function removes all IP addresses from the device.
*
*   INPUTS
*
*       *dev                    A pointer to the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DEV_Detach_Addrs_From_Device(DV_DEVICE_ENTRY *dev)
{
#if (INCLUDE_IPV4 == NU_TRUE)
    DEV_IF_ADDR_ENTRY   *addr_entry, *next_entry;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )
    IP_MULTI            *ipm;
    IP_MULTI            *next_ipm;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && \
      ((INCLUDE_ARP == NU_TRUE) || (INCLUDE_RARP == NU_TRUE)) )
    ARP_RESOLVE_ENTRY               *ar_entry, *ar_prev = NU_NULL;
    INT16                           i;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    /* Leave all IPv4 multicast groups to which the interface
      belongs. */

    ipm = dev->dev_addr.dev_multiaddrs;

    while (ipm)
    {
        next_ipm =  ipm->ipm_next;

        /* Leave the group */
        IP_Delete_Multi(ipm);

        /* If the multicast group was not successfully deleted, get
          a pointer to the next multicast group for the interface. */
        ipm = next_ipm;
    }
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && \
      ((INCLUDE_ARP == NU_TRUE) || (INCLUDE_RARP == NU_TRUE)) )

    /* Get a pointer to the first pending ARP entry. */
    ar_entry = ARP_Res_List.ar_head;

    /* Stop transmitting ARP requests out this interface */
    while (ar_entry != NU_NULL)
    {
        /* If this entry is associated with this interface. */
        if (ar_entry->ar_device == dev)
        {
            /* Clear the timer event to resolve this entry. */
            TQ_Timerunset(ARPRESOLVE, TQ_CLEAR_EXACT, (UNSIGNED)ar_entry->ar_id, 0);

            /* Clean up the entry. */
            ARP_Cleanup_Entry(ar_entry);
        }

        /* Save this entry as the previous. */
        else
        {
            ar_prev = ar_entry;
        }

        /* If a previous was saved, get the next one. */
        if (ar_prev)
        {
            ar_entry = ar_prev->ar_next;
        }

        /* Otherwise, get the head of the list. */
        else
        {
            ar_entry = ARP_Res_List.ar_head;
        }
    }

    /* Invalidate all ARP cache entries for this interface. */
    for (i = 0; i < ARP_CACHE_LENGTH; i++)
    {
        if (ARP_Cache[i].arp_dev_index == dev->dev_index)
        {
            memset(&ARP_Cache[i], 0, sizeof(ARP_ENTRY));
        }
    }

#endif

    /* Get a pointer to the first address entry for the device */
    addr_entry = dev->dev_addr.dev_addr_list.dv_head;

    /* Remove each IP address from the device */
    while (addr_entry)
    {
        /* Get a pointer to the next entry before deleting the current */
        next_entry = addr_entry->dev_entry_next;

        /* Delete the IP and clean up the routes and socket */
        DEV4_Delete_IP_From_Device(dev, addr_entry);

        /* Get a pointer to the next entry */
        addr_entry = next_entry;
    }

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If this device is IPv6 enabled */
    if (dev->dev_flags & DV6_IPV6)
    {
        DEV6_Detach_Addrs_From_Device(dev);
    }
#endif

} /* DEV_Detach_Addrs_From_Device */

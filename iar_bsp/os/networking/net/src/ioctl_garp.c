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
*       ioctl_garp.c
*
*   DESCRIPTION
*
*       This file contains the routine to get an entry from
*       the arp cache
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_SIOCGARP
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       Ioctl_SIOCGARP
*
*   DESCRIPTION
*
*       This function gets an entry from the arp cache
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              The entry was successfully deleted.
*       NU_INVALID_PARM         A matching ARP cache entry does not
*                               exist.
*
*************************************************************************/
STATUS Ioctl_SIOCGARP(SCK_IOCTL_OPTION *option)
{
    SCK_SOCKADDR_IP     dest;
    ARP_ENTRY           *arp_entry;
    STATUS              status;

    /* Extract the destination address of the target ARP cache entry */
    dest.sck_addr = IP_ADDR((UINT8*)option->s_ret.arp_request.arp_pa.sck_data);

    /* Get a pointer to the ARP cache entry */
    arp_entry = ARP_Find_Entry(&dest);

    /* If the entry exists */
    if (arp_entry)
    {
        /* Set the MAC address */
        memcpy(&option->s_ret.arp_request.arp_ha.sck_data[0],
            &arp_entry->arp_mac_addr[0], DADDLEN);

        /* Get the flags */
        option->s_ret.arp_request.arp_flags =
            (INT16)arp_entry->arp_flags;

        status = NU_SUCCESS;
    }

    else
        status = NU_INVALID_PARM;

    return (status);

} /* Ioctl_SIOCGARP */



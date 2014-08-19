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
*       802_output.c
*
*   COMPONENT
*
*       802.x
*
*   DESCRIPTION
*
*       This file implements the functions used by the Ethernet and
*       Wireless LAN to transmit data.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       EightZeroTwo_Output
*
*   DEPENDENCIES
*
*       nu_net.h
*       resolve6.h
*       802.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/802.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/resolve6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/*************************************************************************
*
*   FUNCTION
*
*       EightZeroTwo_Output
*
*   DESCRIPTION
*
*       Given a packet to transmit this function resolves the hardware
*       layer address.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer.
*       *device                 Pointer to the device.
*       *dest                   Pointer to the destination to send to.
*       *ro                     Pointer to the routing information.
*       *mac_dest               The mac address of the destination.
*       *type                   Type of the packet to be send.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_HOST_UNREACHABLE     Host cannot be reached, wrong information
*       -1                      Invalid socket family
*
*************************************************************************/
STATUS EightZeroTwo_Output(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device,
                           VOID *dest, const VOID *ro, UINT8 *mac_dest,
                           UINT16 *type)
{
    STATUS              status;
    UINT8               dest_family;

#if (INCLUDE_IPV4 == NU_TRUE)
    RTAB4_ROUTE_ENTRY   *rt_v4 = NU_NULL;
    ARP_MAC_HEADER      *eh;                /* Ethernet header pointer */
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    RTAB6_ROUTE_ENTRY   *rt_v6 = NU_NULL;
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    if (buf_ptr->mem_flags & NET_IP6)
    {
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Extract the family type */
        dest_family = ((SCK6_SOCKADDR_IP*)dest)->sck_family;

        if (ro)
            rt_v6 = ((RTAB6_ROUTE*)ro)->rt_route;

        /* Verify the route is up. */
        if (rt_v6)
        {
            /* If the route is not up then try to locate an alternate route. */
            if ((rt_v6->rt_flags & RT_UP) == 0)
            {
                IP6_Find_Route((RTAB6_ROUTE*)ro);

                if (((RTAB6_ROUTE*)ro) && (((RTAB6_ROUTE*)ro)->rt_route))
                    RTAB_Free((ROUTE_ENTRY*)rt_v6, NU_FAMILY_IP6);
                else
                {
                    NLOG_Error_Log("A route does not exist to the destination",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    return (NU_HOST_UNREACHABLE);
                }
            }
        }
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    else

#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    {
        /* Extract the family type */
        dest_family = ((SCK_SOCKADDR_IP*)dest)->sck_family;

        if (ro)
            rt_v4 = ((RTAB_ROUTE*)ro)->rt_route;

        /* Verify the route is up. */
        if (rt_v4)
        {
            /* If the route is not up then try to locate an alternate route. */
            if ((rt_v4->rt_flags & RT_UP) == 0)
            {
                rt_v4 = RTAB4_Find_Route((SCK_SOCKADDR_IP*)dest,
                                         RT_BEST_METRIC);

                if (rt_v4 != NU_NULL)
                    RTAB_Free((ROUTE_ENTRY*)rt_v4, NU_FAMILY_IP);
                else
                    return (NU_HOST_UNREACHABLE);
            }
        }
    }

#endif

    switch (dest_family)
    {
#if (INCLUDE_IPV4 == NU_TRUE)

        case SK_FAM_IP:

            /* Resolve the MAC address. */
            status = ARP_Resolve(device, (SCK_SOCKADDR_IP*)dest, mac_dest,
                                 buf_ptr);

            if (status == NU_SUCCESS)
            {
                (*type) = EIP;
            }

            break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

        case SK_FAM_IP6:

            /* Resolve the address of the IPv6 next-hop */
            status = Resolve6_Get_Link_Addr(device, (SCK6_SOCKADDR_IP *)dest,
                                            mac_dest, buf_ptr);

            if (status != NU_SUCCESS)
            {
                /* If the address cannot be resolved due to memory
                 * restrictions.  Return an error.
                 */
                if (status != NU_UNRESOLVED_ADDR)
                {
                    NLOG_Error_Log("The address cannot be resolved",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            else
            {
                (*type) = EIP6;
            }

            break;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

        case SK_FAM_UNSPEC:

            /* Family is unspecified.  This should be an ARP packet. */

            /* Point to the ethernet header information, and extract the type
               and destination address. */
            eh = (ARP_MAC_HEADER *)dest;
            memcpy (mac_dest, eh->ar_mac.ar_mac_ether.dest, DADDLEN);
            (*type) = eh->ar_mac.ar_mac_ether.type;

            status = NU_SUCCESS;

            break;
#endif

        default:
            return (-1);

    }

    return (status);

} /* EightZeroTwo_Output */

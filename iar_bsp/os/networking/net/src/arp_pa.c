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
*       arp_pa.c
*
* DESCRIPTION
*
*       This file contains the implementation of Proxy ARP.
*
* DATA STRUCTURES
*
*       none
*
* FUNCTIONS
*
*       ARP_Proxy_ARP
*
* DEPENDENCIES
*
*       nu_net.h
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* default route used in ARP_Proxy_ARP */
extern ROUTE_NODE *RTAB4_Default_Route;

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Proxy_ARP
*
*   DESCRIPTION
*
*        Finds a route for the proxy ARP. Verifies if a proxy ARP can
*        be sent to the destination device. Makes sure that the route
*        is not the default route. Gets the source hardware address.
*        Then sends a reply to the request for the proxy ARP.
*
*   INPUTS
*
*       *dev_ptr                Pointer the device that RX the ARP Req
*       *a_pkt_ptr              Pointer to the ARP Req
*       src_ip                  The source IP number
*       dest_ip                 The destination IP number
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID ARP_Proxy_ARP(const ARP_LAYER *a_pkt_ptr, DV_DEVICE_ENTRY *dev_ptr,
                   UINT32 src_ip, UINT32 dest_ip)
{
    RTAB_ROUTE  route;
    UINT8       src_hrdwr[DADDLEN];

    /* If the destination is either off the broadcast addresses,
       then return.
     */
    if ( (dest_ip == 0) || (dest_ip == IP_ADDR_BROADCAST) )
    {
        return;
    }

    /* Setup the route structure. */
    route.rt_ip_dest.sck_family     = SK_FAM_IP;
    route.rt_ip_dest.sck_len        = sizeof (SCK_SOCKADDR_IP);
    route.rt_ip_dest.sck_addr       = dest_ip;
    route.rt_route                  = NU_NULL;

    /* Find a route for the proxy ARP */
    IP_Find_Route(&route);

    /* Was a route found. */
    if (route.rt_route)
    {
        /* Is proxy ARP enabled on the target device and is the route UP? */
        if (route.rt_route->rt_entry_parms.rt_parm_device->dev_flags &
            (DV_PROXYARP | DV_UP))
        {
            /* Do not send proxy ARPs out if the target interface is
               the same as the source interface? */
            if (route.rt_route->rt_entry_parms.rt_parm_device != dev_ptr)
            {
                /* Do not use the default interface either. */
                if ( (!RTAB4_Default_Route) ||
                     ((memcmp (route.rt_route->rt_route_node->rt_ip_addr,
                                "\0\0\0\0", IP_ADDR_LEN))) )
                {
                    /* Get the source hardware address */
                    GET_STRING(a_pkt_ptr, ARP_SHA_OFFSET, src_hrdwr, DADDLEN);

                    /* Send a reply to the request. */
                    if (ARP_Reply(src_hrdwr, src_ip, dest_ip, dev_ptr) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to transmit ARP reply",
                                       NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        } /* is proxy ARP enabled? */

        /* Free the route */
        RTAB_Free((ROUTE_ENTRY*)(route.rt_route), NU_FAMILY_IP);
    }

} /* ARP_Proxy_ARP */

#endif

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
*       rtab4_red.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for processing an incoming
*       ICMP Redirect.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Redirect
*       RTAB4_Update_Cached_Routes
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw4.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_UDP == NU_TRUE)
extern struct uport 		*UDP_Ports[UDP_MAX_PORTS];
#endif

#if (INCLUDE_TCP == NU_TRUE)
extern struct _TCP_Port    	*TCP_Ports[TCP_MAX_PORTS];
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
extern struct iport 		*IPR_Ports[IPR_MAX_PORTS];
#endif

STATIC VOID RTAB4_Update_Cached_Routes(ROUTE_ENTRY *, ROUTE_ENTRY *, UINT32);

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Redirect
*
*   DESCRIPTION
*
*       Process an ICMP redirect.
*
*   INPUTS
*
*       dst                     The destination to redirect to
*       gateway                 The gateway for the route to redirect
*       flags                   The flags on the route
*       src                     Source to redirect from
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID RTAB4_Redirect(UINT32 dst, UINT32 gateway, INT32 flags, UINT32 src)
{
    RTAB4_ROUTE_ENTRY   *rt_entry, *rtg_entry, *new_entry;
    DV_DEVICE_ENTRY     *temp_dev;
    SCK_SOCKADDR_IP     gw;
    UINT8               ip_dest[IP_ADDR_LEN];
    UINT8               gw_addr[IP_ADDR_LEN];
    DEV_IF_ADDR_ENTRY   *target_addr;
    STATUS              status;

    if (!IP_Localaddr(gateway))
    {
        NLOG_Error_Log("RTAB4_Redirect redirect gateway invalid",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        return;
    }

    PUT32(ip_dest, 0, dst);
    PUT32(gw_addr, 0, src);

    /* Find a route to the original IP address that caused the redirect. */
    rt_entry = (RTAB4_ROUTE_ENTRY*)RTAB4_Find_Route_By_Gateway(ip_dest,
                                                               gw_addr, 0);

    if (rt_entry != NU_NULL)
    {
        gw.sck_family = SK_FAM_IP;
        gw.sck_len = sizeof(gw);
        gw.sck_addr = gateway;

        rtg_entry = RTAB4_Find_Route(&gw, 0);

        if (rtg_entry != NU_NULL)
        {
            /* Validate the redirect.  There are several conditions that must be
               checked.
               - The RT_DONE flag must not be set.
               - The IP address of the router that sent the redirect must equal the
                 current rt_gateway_v4 for the destination.
               - The interface for the new gateway must equal the current interface for
                 the destination, i.e., the new gateway must be on the same network as
                 the current gateway.
             */
            if ( (!(flags & RT_DONE)) &&
                 (rt_entry->rt_entry_parms.rt_parm_device ==
                  rtg_entry->rt_entry_parms.rt_parm_device) )
            {
                /* Now make sure that the redirect was not to ourself.  That is that the new
                   gateway does not match one of our addresses. */
                for (temp_dev = DEV_Table.dv_head;
                     temp_dev != NU_NULL;
                     temp_dev = temp_dev->dev_next)
                {
                    /* Is there an exact match on the IP address. */
                    if (DEV_Find_Target_Address(temp_dev, gateway))
                        break;

                    /* Get a pointer to the first address in the list of addresses
                     * for the device.
                     */
                    target_addr = temp_dev->dev_addr.dev_addr_list.dv_head;

                    /* Traverse the list of addresses for a matching network
                     * address.
                     */
                    while (target_addr)
                    {
                        /* Is this a broadcast for our network. */
                        if (target_addr->dev_entry_net == gateway)
                            break;

                        /* Get a pointer to the next address in the list */
                        target_addr = target_addr->dev_entry_next;
                    }
                }

                /* If an interface was found with a matching IP address then this is not a
                   valid redirect. */
                if (!temp_dev)
                {
                    /* If the old route is to a gateway. */
                    if (rt_entry->rt_entry_parms.rt_parm_flags & RT_GATEWAY)
                    {
                        /* If the current route is a network route (HOST flag not set) and the
                         * redirect is for a host, then create a new route.
                         */
                        if ( ((rt_entry->rt_entry_parms.rt_parm_flags & RT_HOST) == 0) &&
                              (flags & RT_HOST) )
                        {
                            /* Create the new route. */
                            status = RTAB4_Add_Route(rt_entry->rt_entry_parms.rt_parm_device,
                                                     dst, 0xffffffffUL, gateway,
                                                     (UINT32)flags | RT_GATEWAY |
                                                     RT_DYNAMIC | RT_ICMP | RT_UP);

                            if (status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("RTAB4_Redirect unable to add route",
                                               NERR_SEVERE, __FILE__, __LINE__);

                                NET_DBG_Notify(status, __FILE__, __LINE__,
                                               NU_Current_Task_Pointer(), NU_NULL);
                            }

                            /* Store off the new gateway. */
                            PUT32(gw_addr, 0, gateway);

                            /* Find a route to the original IP address that caused the redirect. */
                            new_entry = (RTAB4_ROUTE_ENTRY*)RTAB4_Find_Route_By_Gateway(ip_dest,
                            															gw_addr, 0);

                            if (new_entry)
                            {
								/* Update any cached routes using the old route entry to
								 * use the new one.
								 */
                            	RTAB4_Update_Cached_Routes((ROUTE_ENTRY*)rt_entry,
														   (ROUTE_ENTRY*)new_entry, dst);

								/* Free the new entry. */
					            RTAB_Free((ROUTE_ENTRY*)new_entry, NU_FAMILY_IP);
                            }
                        }
                        else
                        {
                            /* Modify the existing route. */
                            rt_entry->rt_entry_parms.rt_parm_flags |= RT_MODIFIED;
                            rt_entry->rt_gateway_v4.sck_addr = gateway;
                        }
                    }

                } /* if (!temp_dev) */

                else
                    NLOG_Error_Log("RTAB4_Redirect redirect gateway invalid",
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);

            } /* if (!(flags & RT_DONE) ... */

            RTAB_Free((ROUTE_ENTRY*)rtg_entry, NU_FAMILY_IP);

        } /* if (RTAB4_Find_Route(...) */

        RTAB_Free((ROUTE_ENTRY*)rt_entry, NU_FAMILY_IP);

    } /* if (RTAB4_Find_Route_By_Gateway(...) */

} /* RTAB4_Redirect */

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Update_Cached_Routes
*
*   DESCRIPTION
*
*       Updates any cached routes using the destination IP address to use
*       the new route.
*
*   INPUTS
*
*       *old_route				Pointer to the old route - if any sockets
*                               are using the old route, and the destination
*                               IP address of the socket is the destination
*                               IP being updated, update the route.
*       *new_route				Pointer to the new route to use.
*      	dest					Destination IP address to update if using
*                               the old route.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RTAB4_Update_Cached_Routes(ROUTE_ENTRY *old_route, ROUTE_ENTRY *new_route,
							           UINT32 dest)
{
#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) || \
	  (INCLUDE_IP_RAW == NU_TRUE) )
	INT				i;
#endif

#if (INCLUDE_UDP == NU_TRUE)

	/* Check if any UDP ports are using the updated route. */
	for (i = 0; i < UDP_MAX_PORTS; i++)
	{
		/* If this port is using the updated cached route and the destinations
		 * match.
		 */
		if ( (UDP_Ports[i]) && (old_route == (ROUTE_ENTRY*)UDP_Ports[i]->up_route.rt_route) &&
			 (UDP_Ports[i]->up_faddr == dest) )
		{
			/* Point the old route to the new route. */
			UDP_Ports[i]->up_route.rt_route = (RTAB4_ROUTE_ENTRY*)new_route;

			/* Increase the count for the new route. */
			new_route->rt_entry_parms.rt_parm_refcnt ++;

			/* Free this instance of the old route. */
			RTAB_Free((ROUTE_ENTRY*)old_route, NU_FAMILY_IP);
		}
	}
#endif

#if (INCLUDE_TCP == NU_TRUE)

	/* Check if any UDP ports are using the updated route. */
	for (i = 0; i < TCP_MAX_PORTS; i++)
	{
		/* If this port is using the updated cached route and the destinations
		 * match.
		 */
		if ( (TCP_Ports[i]) && (old_route == (ROUTE_ENTRY*)TCP_Ports[i]->tp_route.rt_route) &&
			 (TCP_Ports[i]->tcp_faddrv4 == dest) )
		{
			/* Point the old route to the new route. */
			TCP_Ports[i]->tp_route.rt_route = (RTAB4_ROUTE_ENTRY*)new_route;

			/* Increase the count for the new route. */
			new_route->rt_entry_parms.rt_parm_refcnt ++;

			/* Free this instance of the old route. */
			RTAB_Free((ROUTE_ENTRY*)old_route, NU_FAMILY_IP);
		}
	}
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

	/* Check if any UDP ports are using the updated route. */
	for (i = 0; i < IPR_MAX_PORTS; i++)
	{
		/* If this port is using the updated cached route and the destinations
		 * match.
		 */
		if ( (IPR_Ports[i]) && (old_route == (ROUTE_ENTRY*)IPR_Ports[i]->ipraw_route.rt_route) &&
			 (IPR_Ports[i]->ip_faddr == dest) )
		{
			/* Point the old route to the new route. */
			IPR_Ports[i]->ipraw_route.rt_route = (RTAB4_ROUTE_ENTRY*)new_route;

			/* Increase the count for the new route. */
			new_route->rt_entry_parms.rt_parm_refcnt ++;

			/* Free this instance of the old route. */
			RTAB_Free((ROUTE_ENTRY*)old_route, NU_FAMILY_IP);
		}
	}
#endif

} /* RTAB4_Update_Cached_Routes */

#endif

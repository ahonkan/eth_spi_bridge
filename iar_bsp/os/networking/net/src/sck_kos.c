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
*       sck_kos.c
*
*   DESCRIPTION
*
*       This file contains the routine to kill all open sockets.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SCK_Kill_All_Open_Sockets
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
**************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Kill_All_Open_Sockets
*
*   DESCRIPTION
*
*       This function is called when an IP address is being remove from
*       a device. It loops through the port lists closing all ports that
*       are using the specified address. Any tasks that are suspended on
*       a port pending an event will be woke up.
*
*   INPUTS
*
*       *dev_ptr                Pointer to the device structure for the
*                               address that is being removed.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SCK_Kill_All_Open_Sockets(const DV_DEVICE_ENTRY *dev_ptr)
{

#if (INCLUDE_TCP == NU_TRUE)
    TCP_PORT    *prt;
#endif

#if (INCLUDE_UDP == NU_TRUE)
    UDP_PORT    *uprt;
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
    IPR_PORT    *iprt;
#endif

#if ((INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE))
    struct sock_struct  *sockptr;
#endif

#if ((INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) || \
     (INCLUDE_TCP == NU_TRUE))
    UINT16              index;
#else
    UNUSED_PARAMETER(dev_ptr);
#endif

#if (INCLUDE_UDP == NU_TRUE)
    /* Init the index. */
    index = 0;

    /* Go through the UDP ports first and check for an active port */
    do
    {
        if (UDP_Ports[index] != NU_NULL)
        {
            /* Get a pointer to the UDP port. */
            uprt = UDP_Ports[index];

            /* Make sure it is using this interface before we kill it. */
            if ( (SCK_Sockets[uprt->up_socketd]) && (
#if (INCLUDE_IPV4 == NU_TRUE)
                  ((uprt->up_faddr_struct.up_family == NU_FAMILY_IP) &&
                   (uprt->up_route.rt_route != NU_NULL) &&
                   (dev_ptr == uprt->up_route.rt_route->rt_device))
#if (INCLUDE_IPV6 == NU_TRUE)
                    ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                  ((uprt->up_faddr_struct.up_family == NU_FAMILY_IP6) &&
                   (uprt->up_routev6.rt_route != NU_NULL) &&
                   (dev_ptr == uprt->up_routev6.rt_route->rt_device))
#endif
                 ) )
            {
                sockptr = SCK_Sockets[uprt->up_socketd];

                /* Resume tasks pending on RX */
                SCK_Resume_All(&sockptr->s_RXTask_List, 0);

                /* Resume tasks pending on TX, remove from buffer suspension
                   list */
                SCK_Resume_All(&sockptr->s_TXTask_List, SCK_RES_BUFF);

                /* Indicate that this socket can no longer be used. */
                sockptr->s_state |= SS_DEVICEDOWN;
            }
        }

    } while (++index < UDP_MAX_PORTS);
#endif



#if (INCLUDE_TCP == NU_TRUE)
    /* Reset the index */
    index = 0;

    /* Look at the TCP ports. */
    do
    {
        /* See if there is an entry. */
        if (TCP_Ports[index] != NU_NULL)
        {
            /* Get a pointer to the TCP port. */
            prt = TCP_Ports[index];

            /* Make sure it is using this interface before we kill it. */
            if (
#if (INCLUDE_IPV4 == NU_TRUE)
                  ((prt->portFlags & TCP_FAMILY_IPV4) &&
                   (prt->tp_route.rt_route != NU_NULL) &&
                   (dev_ptr == prt->tp_route.rt_route->rt_device))
#if (INCLUDE_IPV6 == NU_TRUE)
                    ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                  ((prt->portFlags & TCP_FAMILY_IPV6) &&
                   (prt->tp_routev6.rt_route != NU_NULL) &&
                   (dev_ptr == prt->tp_routev6.rt_route->rt_device))
#endif
                 )
            {
                /* See if the port is already in the closed state */
                if ((prt->state != SCLOSED) && (prt->state != STWAIT))
                {
                    /* Mark the socket state as disconnecting. */
                    SCK_DISCONNECTING(prt->p_socketd);

                    /* Mark the port as closed */
                    prt->state = SCLOSED;

                    /* The connection is closed.  Cleanup. */
                    TCP_Cleanup(prt);
                }
            }
        }

    } while (++index < TCP_MAX_PORTS);
#endif

#if INCLUDE_IP_RAW
    /* Init the index. */
    index = 0;

    /* Now check the RAW IP ports. */
    do
    {
        if (IPR_Ports[index] != NU_NULL)
        {
            /* Get a pointer to the UDP port. */
            iprt = IPR_Ports[index];

            /* Make sure it is using this interface before we kill it. */
            if ( (SCK_Sockets[iprt->ip_socketd]) && (
#if (INCLUDE_IPV4 == NU_TRUE)
                  ((iprt->ipraw_faddr_struct.ipraw_family == NU_FAMILY_IP) &&
                   (iprt->ipraw_route.rt_route != NU_NULL) &&
                   (dev_ptr == iprt->ipraw_route.rt_route->rt_device))
#if (INCLUDE_IPV6 == NU_TRUE)
                    ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                  ((iprt->ipraw_faddr_struct.ipraw_family == NU_FAMILY_IP6) &&
                   (iprt->ipraw_routev6.rt_route != NU_NULL) &&
                   (dev_ptr == iprt->ipraw_routev6.rt_route->rt_device))
#endif
                 ) )
            {
                sockptr = SCK_Sockets[iprt->ip_socketd];

                /* Resume tasks pending on RX */
                SCK_Resume_All(&sockptr->s_RXTask_List, 0);

                /* Resume tasks pending on TX, remove from buffer suspension
                   list */
                SCK_Resume_All(&sockptr->s_TXTask_List, SCK_RES_BUFF);

                /* Indicate that this socket can no longer be used. */
                sockptr->s_state |= SS_DEVICEDOWN;
            }
        }

    } while (++index < IPR_MAX_PORTS);

#endif /* INCLUDE_IP_RAW */

} /* SCK_Kill_All_Open_Sockets */

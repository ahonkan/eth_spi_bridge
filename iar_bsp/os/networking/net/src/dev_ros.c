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
*       dev_ros.c
*
*   DESCRIPTION
*
*       This file contains the routine to resume all sockets using a
*       specific device.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DEV_Resume_All_Open_Sockets
*       DEV_Check_Socket
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
**************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) || \
      (INCLUDE_TCP == NU_TRUE) )
STATIC UINT8 DEV_Check_Socket(struct sock_struct *sockptr);
#endif

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Resume_All_Open_Sockets
*
*   DESCRIPTION
*
*       This function is called when an IP address is being remove from
*       a device. It loops through the socket list resuming all sockets
*       using an IP address that is no longer valid.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DEV_Resume_All_Open_Sockets(VOID)
{
#if (INCLUDE_TCP == NU_TRUE)
    TCP_PORT            *prt;
#endif

#if ((INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE))
    struct sock_struct  *sockptr;
#endif

#if ((INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) || \
     (INCLUDE_TCP == NU_TRUE))
    UINT16              index;
#endif

#if (INCLUDE_UDP == NU_TRUE)

    /* Traverse the UDP ports list, checking for UDP sockets using an
     * invalid IP address as their local address.
     */
    for (index = 0; index < UDP_MAX_PORTS; index++)
    {
        if (UDP_Ports[index])
        {
            if (SCK_Sockets[UDP_Ports[index]->up_socketd])
            {
                sockptr = SCK_Sockets[UDP_Ports[index]->up_socketd];

                /* If the socket is using an invalid address, resume all
                 * tasks using the socket.
                 */
                if (DEV_Check_Socket(sockptr) == NU_FALSE)
                {
                    /* Resume tasks pending on RX */
                    SCK_Resume_All(&sockptr->s_RXTask_List, 0);

                    /* Resume tasks pending on TX, remove from buffer suspension
                     * list.
                     */
                    SCK_Resume_All(&sockptr->s_TXTask_List, SCK_RES_BUFF);

                    /* Indicate that this socket can no longer be used. */
                    sockptr->s_state |= SS_DEVICEDOWN;
                }
            }
        }
    }
#endif

#if (INCLUDE_TCP == NU_TRUE)

    /* Traverse the TCP ports list, checking for TCP sockets using an
     * invalid IP address as their local address.
     */
    for (index = 0; index < TCP_MAX_PORTS; index++)
    {
        /* See if there is an entry. */
        if (TCP_Ports[index] != NU_NULL)
        {
            /* Get a pointer to the port structure. */
            prt = TCP_Ports[index];

            /* If the socket is valid. */
            if ( (prt->p_socketd >= 0) && (SCK_Sockets[prt->p_socketd]) )
            {
                /* If the socket is using an invalid address, resume all
                 * tasks using the socket.
                 */
                if (DEV_Check_Socket(SCK_Sockets[prt->p_socketd]) == NU_FALSE)
                {
                    /* See if the port is already in the closed state */
                    if ( (prt->state != SCLOSED) && (prt->state != STWAIT) )
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
        }
    }
#endif

#if INCLUDE_IP_RAW

    /* Traverse the RAW IP ports list, checking for RAW sockets using an
     * invalid IP address as their local address.
     */
    for (index = 0; index < IPR_MAX_PORTS; index++)
    {
        if (IPR_Ports[index])
        {
            if (SCK_Sockets[IPR_Ports[index]->ip_socketd])
            {
                sockptr = SCK_Sockets[IPR_Ports[index]->ip_socketd];

                /* If the socket is using an invalid address, resume all
                 * tasks using the socket.
                 */
                if (DEV_Check_Socket(sockptr) == NU_FALSE)
                {
                    /* Resume tasks pending on RX */
                    SCK_Resume_All(&sockptr->s_RXTask_List, 0);

                    /* Resume tasks pending on TX, remove from buffer suspension
                     * list
                     */
                    SCK_Resume_All(&sockptr->s_TXTask_List, SCK_RES_BUFF);

                    /* Indicate that this socket can no longer be used. */
                    sockptr->s_state |= SS_DEVICEDOWN;
                }
            }
        }
    }

#endif /* INCLUDE_IP_RAW */

} /* DEV_Resume_All_Open_Sockets */

#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) || \
      (INCLUDE_TCP == NU_TRUE) )

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Check_Socket
*
*   DESCRIPTION
*
*       This function checks whether a socket is using an IP address
*       that no longer exists in the system.
*
*   INPUTS
*
*       *sockptr                A pointer to the socket structure to
*                               check.
*
*   OUTPUTS
*
*       NU_FALSE                The socket is using an invalid IP address.
*       NU_TRUE                 The socket is using a valid IP address.
*
*************************************************************************/
STATIC UINT8 DEV_Check_Socket(struct sock_struct *sockptr)
{
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32  local_addr;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    /* Store the 32-bit IP address */
    if (sockptr->s_family == NU_FAMILY_IP)
    {
#if (INCLUDE_IPV6 == NU_TRUE)
        /* This socket could have been flagged IPv4 because it was an IPv6
         * listening socket that received an IPv4 connection.  In which
         * case, check that this isn't an IPv4-mapped IPv6 address.  If
         * so, extract the IPv4 address.
         */
        if (IPV6_IS_ADDR_V4MAPPED(sockptr->s_local_addr.ip_num.is_ip_addrs))
            local_addr = IP_ADDR(&sockptr->s_local_addr.ip_num.is_ip_addrs[12]);
        else
#endif
            local_addr = IP_ADDR(sockptr->s_local_addr.ip_num.is_ip_addrs);

        /* If this IP address does not exist on any interface. */
        if ( (local_addr != IP_ADDR_ANY) &&
             (!(DEV_Get_Dev_By_Addr(sockptr->s_local_addr.ip_num.is_ip_addrs))) )
        {
            return (NU_FALSE);
        }

        else
        {
            return (NU_TRUE);
        }
    }

#if (INCLUDE_IPV6 == NU_FALSE)

    else
    {
        return (NU_FALSE);
    }

#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If the socket is using an IP address that does not exist
     * on any interface.
     */
    if ( (sockptr->s_family == NU_FAMILY_IP6) &&
         (((!(IPV6_IS_ADDR_UNSPECIFIED(sockptr->s_local_addr.ip_num.is_ip_addrs))) &&
#if (INCLUDE_IPV4 == NU_TRUE)
          (!(IPV6_IS_ADDR_V4MAPPED(sockptr->s_local_addr.ip_num.is_ip_addrs))) &&
#endif
          (!(DEV6_Get_Dev_By_Addr(sockptr->s_local_addr.ip_num.is_ip_addrs))))
#if (INCLUDE_IPV4 == NU_TRUE)
         ||
         ((IPV6_IS_ADDR_V4MAPPED(sockptr->s_local_addr.ip_num.is_ip_addrs)) &&
          (!DEV_Get_Dev_By_Addr(&sockptr->s_local_addr.ip_num.is_ip_addrs[12])))
#endif
        ) )
    {
        return (NU_FALSE);
    }

    else
    {
        return (NU_TRUE);
    }

#endif

} /* DEV_Check_Socket */

#endif


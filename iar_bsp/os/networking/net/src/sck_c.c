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
*       sck_c.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Connect.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Connect
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/udp4.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/in6.h"
#include "networking/udp6.h"
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Connect
*
*   DESCRIPTION
*
*       This function is called by a client wishing to establish a
*       connection with a server.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *servaddr               Pointer to the server's protocol-specific
*                               address.
*       addrlen                 This parameter is reserved for future use.
*                               A value of zero should be used
*
*   OUTPUTS
*
*       > = 0                   Socket Descriptor
*       NU_INVALID_PARM         The server address parameter was not valid.
*       NU_NOT_CONNECTED        The connection attempt failed.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously allocated
*                               via the NU_Socket call.
*       NU_INVALID_ADDRESS      The address does not match a class A,
*                               class B or Class C address or the 4-tuple
*                               provided is not unique.
*       -1                      If the the MEM_Dequeue failed in TCPSS_Send_SYN_FIN
*       NU_IS_CONNECTING        The socket is non blocking and the connection is
*                               being established.
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for the socket:
*
*       NU_DEST_UNREACH_ADMIN
*       NU_DEST_UNREACH_ADDRESS
*       NU_DEST_UNREACH_PORT
*       NU_TIME_EXCEED_HOPLIMIT
*       NU_TIME_EXCEED_REASM
*       NU_PARM_PROB_HEADER
*       NU_PARM_PROB_NEXT_HDR
*       NU_PARM_PROB_OPTION
*       NU_DEST_UNREACH_NET
*       NU_DEST_UNREACH_HOST
*       NU_DEST_UNREACH_PROT
*       NU_DEST_UNREACH_FRAG
*       NU_DEST_UNREACH_SRCFAIL
*       NU_PARM_PROB
*       NU_SOURCE_QUENCH
*       NU_NO_ROUTE_TO_HOST
*
*************************************************************************/
STATUS NU_Connect(INT socketd, struct addr_struct *servaddr, INT16 addrlen)
{
    struct sock_struct      *sockptr;
    STATUS                  return_status;

#if (INCLUDE_TCP == NU_TRUE)
    TCP_PORT                *pprt;
    struct SCK_TASK_ENT     task_entry; /* task entry for list operations */
    UINT32                  socket_id;
    UINT8                   *server_address;
#endif
#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_TCP == NU_TRUE) )
    INT                     port_num;   /* client port number returned from netxopen */
#endif

#if (INCLUDE_UDP == NU_TRUE)
    UDP_PORT                *uprt;
#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8                   *dest_addr;
#endif
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (servaddr == NU_NULL) || (
#if (INCLUDE_IPV4 == NU_TRUE)
        (servaddr->family != SK_FAM_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
        &&
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        (servaddr->family != SK_FAM_IP6)
#endif
        ) )
        return (NU_INVALID_PARM);

#endif

    /* verify that a port number exists */
    if (servaddr->port)
    {
        /* Obtain the semaphore and validate the socket */
        return_status = SCK_Protect_Socket_Block(socketd);

        if (return_status != NU_SUCCESS)
            return (return_status);

        /* Clean up warnings.  This parameter is used for socket compatibility
         * but we are currently not making any use of it.
         */
        UNUSED_PARAMETER(addrlen);

        sockptr = SCK_Sockets[socketd];

        /* Verify the socket is not already in use. Connect can only be
         * called once.
         */
        if (sockptr->s_state != 0)
        {
#if (INCLUDE_UDP == NU_TRUE)

            /* If the address is the NULL address for the particular
             * family, reset the peer connection.
             */
            if ( (sockptr->s_protocol == NU_PROTO_UDP) && (
#if (INCLUDE_IPV4 == NU_TRUE)
                 ((servaddr->family == NU_FAMILY_IP) &&
                  (IP_ADDR(servaddr->id.is_ip_addrs) == IP_ADDR_ANY))
#if (INCLUDE_IPV6 == NU_TRUE)
                   ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                 ((servaddr->family == NU_FAMILY_IP6) &&
                  (IPV6_IS_ADDR_UNSPECIFIED(servaddr->id.is_ip_addrs)))
#endif
                  ) )
            {
                /* Set the state to not connected */
                sockptr->s_state = 0;

                uprt = UDP_Ports[sockptr->s_port_index];

                if (uprt != NU_NULL)
                {
                    /* Remove all stored peer information */
                    memset(&uprt->up_faddr_struct, 0, sizeof(uprt->up_faddr_struct));
                    memset(&uprt->up_laddr_struct, 0, sizeof(uprt->up_laddr_struct));
                    memset(&sockptr->s_local_addr, 0, sizeof(sockptr->s_local_addr));
                    memset(&sockptr->s_foreign_addr, 0, sizeof(sockptr->s_foreign_addr));
                }
            }

            else
#endif
                return_status = NU_INVALID_SOCKET;

            /* Release the semaphore */
            SCK_Release_Socket();

            return (return_status);
        }

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

        /* If the destination address is an IPv4-Mapped address, set a flag
         * in the socket indicating so.
         */
        if (IPV6_IS_ADDR_V4MAPPED(servaddr->id.is_ip_addrs))
        {
            /* If the socket option to transmit only IPv6 packets on this socket
             * has been set, return an error to the application.
             */
            if (sockptr->s_options & SO_IPV6_V6ONLY)
            {
                /* Release the semaphore */
                SCK_Release_Socket();

                return (NU_INVALID_ADDRESS);
            }

            SCK_Sockets[socketd]->s_flags |= SF_V4_MAPPED;

#if (INCLUDE_TCP == NU_TRUE)

            /* Point to the IPv4 portion of the IPv4-Mapped IPv6 address
             * and connect using IPv4.
             */
            server_address = &servaddr->id.is_ip_addrs[12];

#endif
        }
        else
#endif

        {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

            /* If the application is attempting to connect to an IPv4 address,
             * but the socket option to transmit only IPv6 packets on this socket
             * has been set, return an error to the application.
             */
            if ( (sockptr->s_options & SO_IPV6_V6ONLY) &&
                 (servaddr->family == NU_FAMILY_IP) )
            {
                /* Release the semaphore */
                SCK_Release_Socket();

                return (NU_INVALID_ADDRESS);
            }
#endif

#if (INCLUDE_TCP == NU_TRUE)
            server_address = servaddr->id.is_ip_addrs;
#endif
        }

#if (INCLUDE_TCP == NU_TRUE)

        /* Attempt to establish the connection if this is a TCP socket. */
        if (sockptr->s_protocol == NU_PROTO_TCP)
        {
            port_num = TCPSS_Net_Xopen(server_address, servaddr->family,
                                       (UINT16)servaddr->port, socketd);

            if (port_num >= 0)
            {
                /* Update the socket state */
                sockptr->s_state = SS_ISCONNECTING;

                /* Get a pointer to the TCP port structure. */
                pprt = TCP_Ports[port_num];

                /* Migrate the data from the port structure to the socket structure. */
                /* Copy the IP address from the port into the sockptr data
                 * structure if the caller didn't bind to a specific address.
                 */

                if (!(sockptr->s_flags & SF_BIND))
                {
#if (INCLUDE_IPV6 == NU_TRUE)
                    if (servaddr->family == SK_FAM_IP6)
                    {
#if (INCLUDE_IPV4 == NU_TRUE)
                        /* If this is a V4-Mapped socket, copy the V4-Mapped IPv6
                         * address into the socket structure.
                         */
                        if (sockptr->s_flags & SF_V4_MAPPED)
                        {
                            /* Map the IPv4 address to an IPv6 IPv4-Mapped address */
                            IP6_Create_IPv4_Mapped_Addr(sockptr->s_local_addr.ip_num.is_ip_addrs,
                                                        pprt->tcp_laddrv4);

                            /* Remove the V4-Mapped flag from the socket */
                            sockptr->s_flags &= ~SF_V4_MAPPED;
                        }

                        else
#endif
                            NU_BLOCK_COPY(sockptr->s_local_addr.ip_num.is_ip_addrs,
                                          pprt->tcp_laddrv6, IP6_ADDR_LEN);
                    }

#if (INCLUDE_IPV4 == NU_TRUE)
                    else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                        *(UINT32 *)sockptr->s_local_addr.ip_num.is_ip_addrs =
                            LONGSWAP(pprt->tcp_laddrv4);
#endif
                }

                /* Store the foreign IP address */
                memcpy(&(sockptr->s_foreign_addr.ip_num), &(servaddr->id),
                       MAX_ADDRESS_SIZE);

                /* Store the foreign (server's) port # and ip # in the socket
                 * descriptor.
                 */
                sockptr->s_foreign_addr.port_num = servaddr->port;

                /* This is a blocking TCP connect */
                if (sockptr->s_flags & SF_BLOCK)
                {
                    /* It is possible for the connection to be established by the time
                     * this point is reached.  This check guarantees that the task will
                     * not be suspended indefinitely if this occurs.
                     */
                    if ( (pprt->state == SSYNS) || (pprt->state == SSYNR) )
                    {
                        /* Initialize the list entry's task number */
                        task_entry.task = NU_Current_Task_Pointer();

                        /* Add it to the list of tasks pending on transmit */
                        DLL_Enqueue(&sockptr->s_TXTask_List, &task_entry);

                        /* Get the socket ID to verify the socket after suspension */
                        socket_id = sockptr->s_struct_id;

                        SCK_Suspend_Task(task_entry.task);

                        /* If this is a different socket, handle it appropriately */
                        if ( (!SCK_Sockets[socketd]) ||
                             (SCK_Sockets[socketd]->s_struct_id != socket_id) )
                        {
                            /* Release the semaphore */
                            SCK_Release_Socket();

                            return (NU_SOCKET_CLOSED);
                        }
                    }

                    /* If the port index is valid, check the port state and icmp errors */
                    if (sockptr->s_port_index != -1)
                    {
                        pprt = TCP_Ports[sockptr->s_port_index];

                        /* Should be in the SEST or SCWAIT state */
                        if ( !((pprt->state == SEST) || (pprt->state == SCWAIT)) )
                            return_status = NU_NOT_CONNECTED;

                        /* Socket errors pending */
                        else if (sockptr->s_error != 0)
                        {
                            return_status = (INT)sockptr->s_error;

                            /* Reset the socket error value */
                            sockptr->s_error = 0;
                        }

                        /* No errors pending, return the socket descriptor */
                        else
                            return_status = socketd;
                    } /* end valid port index */
                    else
                        return_status = NU_NOT_CONNECTED;

                } /* end blocking connect */
                /* Handle the non-blocking connect */
                else
                    return_status = NU_IS_CONNECTING;

            } /* end connection established */
            else
                /* This will return the port number error message, NU_NO_PORT_NUMBER */
                return_status = port_num;
        }

#if (INCLUDE_UDP == NU_TRUE)
        /* Otherwise, get the port index from the socket structure */
        else
#endif
#endif

#if (INCLUDE_UDP == NU_TRUE)
        {
            port_num = sockptr->s_port_index;

            if (port_num >= 0)
            {
                /* Get a pointer to the UDP port structure */
                uprt = UDP_Ports[port_num];

#if (INCLUDE_IPV6 == NU_TRUE)
                if (servaddr->family == SK_FAM_IP6)
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    /* If this is a V4-Mapped socket, copy the V4-Mapped IPv6
                     * address into the socket structure.
                     */
                    if (sockptr->s_flags & SF_V4_MAPPED)
                    {
                        /* If an address was not bound to the socket, select an
                         * IPv4 address based on the route found to the destination.
                         */
                        if (IPV6_IS_ADDR_UNSPECIFIED(uprt->up_laddrv6))
                        {
                            /* If a route was found, Create an IPv4-Mapped IPv6
                             * address from the IPv4 address associated with the
                             * device, and store it in the port structure.
                             */
                            if (UDP4_Cache_Route(uprt,
                                                 IP_ADDR(&servaddr->id.is_ip_addrs[12]))
                                                 == NU_SUCCESS)
                            {
                                /* Store the IPv4 address in the port structure */
                                uprt->up_laddr =
                                    uprt->up_cache_route.up_route_v4.rt_route->rt_device->
                                    dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr;

                                /* Create an IPv4-Mapped IPv6 address from the IPv4
                                 * address associated with the device and store it
                                 * in the socket structure.
                                 */
                                if (!(sockptr->s_flags & SF_BIND))
                                    IP6_Create_IPv4_Mapped_Addr(sockptr->s_local_addr.
                                                                ip_num.is_ip_addrs,
                                                                uprt->up_laddr);
                            }
                            else
                                return_status = NU_NOT_CONNECTED;
                        }
                        else
                        {
                            if (!(sockptr->s_flags & SF_BIND))
                                NU_BLOCK_COPY(sockptr->s_local_addr.ip_num.is_ip_addrs,
                                              uprt->up_laddrv6, IP6_ADDR_LEN);

                            /* Extract the IPv4 address from the IPv6 IPv4-Mapped address */
                            IP6_EXTRACT_IPV4_ADDR(uprt->up_laddr, uprt->up_laddrv6);
                        }

                        /* Extract the IPv4 address from the IPv6 IPv4-Mapped address */
                        IP6_EXTRACT_IPV4_ADDR(uprt->up_faddr, servaddr->id.is_ip_addrs);

                        uprt->up_faddr_struct.up_family = SK_FAM_IP;

                        /* Remove the V4-Mapped flag from the socket */
                        sockptr->s_flags &= ~SF_V4_MAPPED;
                    }

                    else
#endif
                    {
                        uprt->up_faddr_struct.up_family = SK_FAM_IP6;

                        /* If no local address was bound to the socket */
                        if (IPV6_IS_ADDR_UNSPECIFIED(uprt->up_laddrv6))
                        {
                            /* If a route to the destination exists, determine
                             * the local address to use based on the device
                             * out which the packet will be transmitted.
                             */
                            if (UDP6_Cache_Route(uprt,
                                                 servaddr->id.is_ip_addrs) == NU_SUCCESS)
                            {
                                /* Determine if there is an address of the same scope
                                 * as the destination address on this device.
                                 */
                                dest_addr =
                                    in6_ifawithifp(uprt->up_cache_route.up_route_v6.rt_route->rt_device,
                                                   servaddr->id.is_ip_addrs);

                                /* Copy the address into the port and socket
                                 * structures
                                 */
                                if (dest_addr)
                                {
                                    NU_BLOCK_COPY(uprt->up_laddrv6, dest_addr,
                                                  IP6_ADDR_LEN);

                                    if (!(sockptr->s_flags & SF_BIND))
                                        NU_BLOCK_COPY(sockptr->s_local_addr.ip_num.is_ip_addrs,
                                                      uprt->up_laddrv6, IP6_ADDR_LEN);
                                }
                                else
                                    return_status = NU_NOT_CONNECTED;
                            }
                            else
                                return_status = NU_NOT_CONNECTED;
                        }

                        /* Fill in the foreign address of the port structure */
                        NU_BLOCK_COPY(uprt->up_faddrv6, servaddr->id.is_ip_addrs,
                                      MAX_ADDRESS_SIZE);
                    }
                }

#if (INCLUDE_IPV4 == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                {
                    uprt->up_faddr_struct.up_family = SK_FAM_IP;

                    /* If an address was not bound to the socket, select an
                     * address based on the route found to the destination.
                     */
                    if (uprt->up_laddr == IP_ADDR_ANY)
                    {
                        /* If a route was found, use the IPv4 address of the
                         * device associated with the route.
                         */
                        if (UDP4_Cache_Route(uprt,
                                             IP_ADDR(servaddr->id.is_ip_addrs)) == NU_SUCCESS)
                        {
                            uprt->up_laddr =
                                uprt->up_cache_route.up_route_v4.rt_route->rt_device->
                                dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr;
                        }
                    }

                    /* If a source address was found */
                    if (uprt->up_laddr != IP_ADDR_ANY)
                    {
                        if (!(sockptr->s_flags & SF_BIND))
                            *(UINT32 *)sockptr->s_local_addr.ip_num.is_ip_addrs =
                                LONGSWAP(uprt->up_laddr);

                        uprt->up_faddr = IP_ADDR(servaddr->id.is_ip_addrs);
                    }
                    else
                        return_status = NU_NOT_CONNECTED;
                }
#endif

                /* If a source address was successfully found, finish
                 * filling in the socket structure.
                 */
                if (return_status == NU_SUCCESS)
                {
                    /* Store the foreign IP address */
                    memcpy(&(sockptr->s_foreign_addr.ip_num), &(servaddr->id),
                           MAX_ADDRESS_SIZE);

                    /* store the foreign (server's) port # and ip # in the socket
                       descriptor */
                    sockptr->s_foreign_addr.port_num = servaddr->port;

                    /* Fill in the foreign side's port number */
                    uprt->up_fport = servaddr->port;

                    /* Set the state of the socket to connected */
                    sockptr->s_state |= SS_ISCONNECTED;

                    return_status = socketd;
                }
            }
        }
#endif

        /* Trace log */
        T_SOCK_STATUS(sockptr->s_state, socketd, return_status);

        /* Release the semaphore */
        SCK_Release_Socket();

    }  /* end if destination port was not NU_NULL */
    else
        return_status = NU_NOT_CONNECTED;

    /* return to caller */
    return (return_status);

} /* NU_Connect */

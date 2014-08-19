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
*       sck_ac.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Accept.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Accept
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for ip multicast options for all sockets */
MULTI_SCK_OPTIONS NET_AC_Multi_Opt_Memory[NSOCKETS];

#if (INCLUDE_IPV6 == NU_TRUE)
MULTI_SCK_OPTIONS NET6_AC_Multi_Opt_Memory[NSOCKETS];
#endif
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Accept
*
*   DESCRIPTION
*
*       This function is responsible for establishing a new socket
*       descriptor containing info on both the server and a client
*       with whom a connection has been successfully established.
*
*   INPUTS
*
*       socketd                 Specifies the server's socket descriptor
*       *peer                   Pointer to the protocol-specific address
*                               of the client.
*       *addrlen                Reserved for future use. A value of zero
*                               should be used.
*
*   OUTPUTS
*
*       > = 0                   Socket Descriptor
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been
*                               previously allocated via the NU_Socket
*                               call.
*       NU_INVALID_PARM         The address of the client is invalid, if
*                               the sockets s_flag is not set to listen,
*                               if the task_entry is not valid, or the
*                               task_entry id is not valid
*       NU_NOT_CONNECTED        The socket is not connected
*       NU_WOULD_BLOCK          The user specified non-blocking and no
*                               resources are available
*
*************************************************************************/
STATUS NU_Accept(INT socketd, struct addr_struct *peer, INT16 *addrlen)
{
    TCP_PORT                    *pprt;          /* port pointer */
    struct TASK_TABLE_STRUCT    *task_entry;    /* structure of connections
                                                   for this server/task_id */
    struct SCK_TASK_ENT         ssp_task;       /* structure for task suspension */
    INT                         new_sock;       /* index of new socket in SCK_Sockets */
    STATUS                      return_status;
    INT                         index;
    UINT16                      total_entries;
    UINT32                      socket_id;

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) && (INCLUDE_STATIC_BUILD == NU_FALSE) )
    STATUS                      status;
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the peer pointer. */
    if (peer == NU_NULL)
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

    /* Handle compiler warnings. */
    UNUSED_PARAMETER(addrlen);

    /* Prior to the NU_Accept() service call, an application must call
     * NU_Listen().  NU_Listen() sets up a table to accept connection
     * requests.  This table must be ready before NU_Accept() begins
     * to wait for connection attempts.
     */

    /* If this is not a listening socket return an error. */
    if (!(SCK_Sockets[socketd]->s_flags & SF_LISTENER))
    {
        /* Release the semaphore */
        SCK_Release_Socket();

        return (NU_INVALID_PARM);
    }

    task_entry = SCK_Sockets[socketd]->s_accept_list;

    if (task_entry == NU_NULL)
    {
        /* Release the semaphore */
        SCK_Release_Socket();

        return (NU_INVALID_PARM);
    }

    /* Save off the total number of entries this task is willing to
     * accept.
     */
    total_entries = task_entry->total_entries;

    /* If the total number of entries we are willing to accept is zero,
     * we do not want to have any established connections waiting in
     * the queue.  Set total entries to 1 now to let the lower layer
     * know that we are ready to accept one connection.
     */
    if (task_entry->total_entries == 0)
        task_entry->total_entries = 1;

    /* Check for an established connection.  If there isn't one, then
     * suspend the calling task until the connection is made.
     */
    for (;;)
    {
        /* Search the task table for this port number/task id */
        index = SCK_SearchTaskList(task_entry, SEST, -1);

        /* Continue only if a match was found in the task table */
        if (index >= 0)
        {
            /* Grab the index of the socket that was created when the
             * connection was established.
             */
            new_sock = task_entry->socket_index[index];

            /* Clear the entry that was used to accept this connection
             * so that it can be reused.
             */
            SCK_Clear_Accept_Entry(task_entry, index);

            /* Make sure we got one.  */
            if (new_sock >= 0)
            {
                /* Get a pointer to the associated port. */
                pprt = TCP_Ports[SCK_Sockets[new_sock]->s_port_index];

                SCK_Sockets[new_sock]->s_protocol =
                    SCK_Sockets[socketd]->s_protocol;

                /* Copy the local port number */
                SCK_Sockets[new_sock]->s_local_addr.port_num =
                    SCK_Sockets[socketd]->s_local_addr.port_num;

                /* Fill the client portion of the new socket descriptor */

                /* Foreign side */
                SCK_Sockets[new_sock]->s_foreign_addr.port_num = pprt->out.port;

#if (INCLUDE_IPV6 == NU_TRUE)

                /* A new port will have been made for the incoming connection.  This
                 * port will either be for an IPv4 or IPv6 connection.
                 */
                if (pprt->portFlags & TCP_FAMILY_IPV6)
                {
                    NU_BLOCK_COPY(SCK_Sockets[new_sock]->s_foreign_addr.ip_num.is_ip_addrs,
                                  pprt->tcp_faddrv6, IP6_ADDR_LEN);

                    NU_BLOCK_COPY(SCK_Sockets[new_sock]->s_local_addr.ip_num.is_ip_addrs,
                                  pprt->tcp_laddrv6, IP6_ADDR_LEN);

                    NU_BLOCK_COPY(peer->id.is_ip_addrs, pprt->tcp_faddrv6, IP6_ADDR_LEN);

                    peer->family = NU_FAMILY_IP6;
                }

#if (INCLUDE_IPV4 == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                {

#if (INCLUDE_IPV6 == NU_TRUE)

                    /* If the foreign side is an IPv4 node and the original listening
                     * socket was an IPv6 socket, pass the user back IPv4-mapped
                     * addresses.
                     */
                    if (SCK_Sockets[new_sock]->s_flags & SF_V4_MAPPED)
                    {
                        /* Map the IPv4 addresses to IPv6 IPv4-Mapped addresses */
                        IP6_Create_IPv4_Mapped_Addr(SCK_Sockets[new_sock]->s_foreign_addr.ip_num.is_ip_addrs,
                                                    pprt->tcp_faddrv4);

                        IP6_Create_IPv4_Mapped_Addr(SCK_Sockets[new_sock]->s_local_addr.ip_num.is_ip_addrs,
                                                    pprt->tcp_laddrv4);

                        IP6_Create_IPv4_Mapped_Addr(peer->id.is_ip_addrs,
                                                    pprt->tcp_faddrv4);

                        peer->family = NU_FAMILY_IP6;
                    }

                    else
#endif
                    {
                        *(UINT32 *)SCK_Sockets[new_sock]->s_foreign_addr.ip_num.is_ip_addrs =
                              LONGSWAP(pprt->tcp_faddrv4);

                        *(UINT32 *)SCK_Sockets[new_sock]->s_local_addr.ip_num.is_ip_addrs =
                            LONGSWAP(pprt->tcp_laddrv4);

                        *(UINT32 *)peer->id.is_ip_addrs = LONGSWAP(pprt->tcp_faddrv4);

                        peer->family = NU_FAMILY_IP;
                    }
                }
#endif

                /* Fill in the client address structure that is returned */
                peer->port = SCK_Sockets[new_sock]->s_foreign_addr.port_num;

                /* If socket options were set on the listening socket, those
                 * values should be inherited by the new socket.
                 */

                /* Set the broadcast interface of the new socket */
                SCK_Sockets[new_sock]->s_bcast_if = SCK_Sockets[socketd]->s_bcast_if;

                /* Set the options */
                SCK_Sockets[new_sock]->s_options = SCK_Sockets[socketd]->s_options;

                /* Copy the linger options */
                SCK_Sockets[new_sock]->s_linger.linger_on =
                    SCK_Sockets[socketd]->s_linger.linger_on;

                SCK_Sockets[new_sock]->s_linger.linger_ticks =
                    SCK_Sockets[socketd]->s_linger.linger_ticks;

                /* If the parent socket is non-blocking, remove the blocking
                 * flag from the new child socket.  All sockets are created
                 * as blocking sockets by default.
                 */
                if (!(SCK_Sockets[socketd]->s_flags & SF_BLOCK))
                {
                    SCK_Sockets[new_sock]->s_flags &= ~SF_BLOCK;
                }

                /* Set the Time To Live */
                pprt->p_ttl = TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_ttl;

                /* Copy the value of the Nagle Algorithm */
                pprt->out.push =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->out.push;

                /* Copy the value of the first probe timeout for Zero Window
                 * probes.
                 */
                pprt->p_first_probe_to =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_first_probe_to;

                /* Copy the value of the maximum timeout for Zero Window probes. */
                pprt->p_max_probe_to =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_max_probe_to;

                /* Copy the value of the number of Zero Window probes to transmit. */
                pprt->p_max_probes =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_max_probes;

                /* Copy the MSL value of the socket. */
                pprt->p_msl = TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_msl;

                /* Copy the first RTO value for the socket. */
                pprt->p_first_rto =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_first_rto;

                /* Set up the RTO value for the socket. */
                pprt->p_rto =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_first_rto;

                /* Set up the max RTO value for the socket. */
                pprt->p_max_rto =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_max_rto;

                /* Set up the max number of retransmissions for the socket. */
                pprt->p_max_r2 =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_max_r2;

                /* Set up the delay ACK value for the socket. */
                pprt->p_delay_ack =
                    TCP_Ports[SCK_Sockets[socketd]->s_port_index]->p_delay_ack;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

                /* If Congestion Control is disabled on the original listening socket,
                 * disable Congestion Control on the new socket too.
                 */
                pprt->portFlags = (UINT32)(pprt->portFlags |
                    (TCP_Ports[SCK_Sockets[socketd]->s_port_index]->portFlags & TCP_DIS_CONGESTION));

#endif

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
#if (INCLUDE_IPV6 == NU_TRUE)

                /* Fill in the multicast options */
                if (peer->family == NU_FAMILY_IP6)
                {
                    if (SCK_Sockets[socketd]->s_moptions_v6)
                    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        /* Allocate a multicast option buffer. */
                        status = NU_Allocate_Memory(MEM_Cached,
                                               (VOID**)&SCK_Sockets[new_sock]->s_moptions_v6,
                                               sizeof(*SCK_Sockets[new_sock]->s_moptions_v6),
                                               (UNSIGNED)NU_NO_SUSPEND);

                        if (status != NU_SUCCESS)
                            NLOG_Error_Log("Failed to allocate memory for socket multicast options",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        else
#else
                        /* Assign memory to the multicast option buffer */
                        SCK_Sockets[new_sock]->s_moptions_v6 =
                            &NET6_AC_Multi_Opt_Memory[socketd];

#endif
                            memcpy(SCK_Sockets[new_sock]->s_moptions_v6,
                                   SCK_Sockets[socketd]->s_moptions_v6,
                                   sizeof(*SCK_Sockets[socketd]->s_moptions_v6));
                    }
                }

#if (INCLUDE_IPV4 == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                {
                    if (SCK_Sockets[socketd]->s_moptions_v4)
                    {
                        /* Allocate a multicast option buffer. */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                        status = NU_Allocate_Memory(MEM_Cached,
                                               (VOID**)&SCK_Sockets[new_sock]->s_moptions_v4,
                                               sizeof(*SCK_Sockets[new_sock]->s_moptions_v4),
                                               (UNSIGNED)NU_NO_SUSPEND);

                        if (status != NU_SUCCESS)
                            NLOG_Error_Log("Failed to allocate memory for socket multicast options",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        else
#else
                        /* Assign memory to multicast options */
                        SCK_Sockets[new_sock]->s_moptions_v4
                            = &NET_AC_Multi_Opt_Memory[new_sock];

#endif  /* INCLUDE_STATIC_BUILD */

                            memcpy(SCK_Sockets[new_sock]->s_moptions_v4,
                                   SCK_Sockets[socketd]->s_moptions_v4,
                                   sizeof(*SCK_Sockets[socketd]->s_moptions_v4));
                    }
                }
#endif
#endif
                /* Get the pointer */
                SCK_Sockets[new_sock]->s_accept_list = NU_NULL;

                /* A connection has been successfully accepted. */
                return_status = new_sock;

                break;
            }
        }

        /* There are no connections to accept so check to see if the application
         * wants to block pending a connection.
         */
        else if (!(SCK_Sockets[socketd]->s_flags & SF_BLOCK))
        {
            /* Blocking is not desired so indicate that no connection was made. */
            return_status = NU_WOULD_BLOCK;

            break;
        }

        /* No match in task table */
        else
        {
            /* Setup the suspended task entry */
            ssp_task.task = NU_Current_Task_Pointer();

            /* Put this on the suspended accept list */
            DLL_Enqueue(&task_entry->ssp_task_list, &ssp_task);

            /* Get the socket ID to verify the socket after suspension */
            socket_id = SCK_Sockets[socketd]->s_struct_id;

            /* Let others in while we are waiting for the connection.  */
            SCK_Suspend_Task(ssp_task.task);

            /* If this is a different socket, handle it appropriately */
            if ( (!SCK_Sockets[socketd]) ||
                 (SCK_Sockets[socketd]->s_struct_id != socket_id) )
            {
                return_status = NU_SOCKET_CLOSED;
                break;
            }
        }
    }

    /* Restore the total number of entries this task is willing to accept
     * to its original condition.
     */
    task_entry->total_entries = total_entries;

    /* Release the semaphore */
    SCK_Release_Socket();

    /* Return the new socket descriptor to the server */
    return (return_status);

} /* NU_Accept */

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
* FILE NAME
*
*       sck_tcp.c
*
* DESCRIPTION
*
*       This file contains socket routines related only to TCP
*       connections.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       SCK_Clear_Accept_Entry
*       SCK_TaskTable_Entry_Delete
*       SCK_SearchTaskList
*       SCK_Check_Listeners
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)
extern INT SCK_ReuseAddr_Set;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* array already declared in sck_li.c */
extern struct  TASK_TABLE_STRUCT    NET_Listen_Memory[];
extern UINT8     NET_Listen_Memory_Flags[];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Clear_Accept_Entry
*
*   DESCRIPTION
*
*       Clears an entry in a sockets list of accepted connections.
*
*   INPUTS
*
*       *task_entry             Pointer to the task entry to clear accept
*                               from
*       index                   Where in the task table
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SCK_Clear_Accept_Entry(const struct TASK_TABLE_STRUCT *task_entry,
                            INT index)
{
      /* clear the TCP_Ports number entries at the current index */
      task_entry->socket_index[index] = NU_IGNORE_VALUE;
      task_entry->stat_entry[index] = NU_IGNORE_VALUE;

} /* SCK_Clear_Accept_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       SCK_TaskTable_Entry_Delete
*
*   DESCRIPTION
*
*       This function is responsible for deleting a task table entry from
*       the task table.
*
*   INPUTS
*
*       socketd                 The socket descriptor
*
*   OUTPUTS
*
*       NU_SUCCESS              The entry was deleted.
*       -1                      The entry does not exist.
*
*************************************************************************/
STATUS SCK_TaskTable_Entry_Delete(INT socketd)
{
     INT    cleansock, cleanport;
     UINT16 t;

    if ((SCK_Sockets[socketd]->s_flags & SF_LISTENER) &&
        (SCK_Sockets[socketd]->s_accept_list))
    {
        /* See if there are any accepted connections */
        for (t=0; t < SCK_Sockets[socketd]->s_accept_list->total_entries; t++)
        {
            cleansock = SCK_Sockets[socketd]->s_accept_list->socket_index[t];
            if (cleansock >= 0)
            {
                cleanport = SCK_Sockets[cleansock]->s_port_index;
                TCP_Cleanup(TCP_Ports[cleanport]);
            }
        }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        if (NU_Deallocate_Memory((VOID *)SCK_Sockets[socketd]->s_accept_list)
            != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                           __FILE__, __LINE__);
#else
        NET_Listen_Memory_Flags[(UINT8)((struct TASK_TABLE_STRUCT *)(SCK_Sockets[socketd]->s_accept_list)
                        - NET_Listen_Memory)] = NU_FALSE;
#endif

        SCK_Sockets[socketd]->s_accept_list = 0;
        return(NU_SUCCESS);
    }
    else
        return(-1);


} /* SCK_TaskTable_Entry_Delete */

/*************************************************************************
*
*   FUNCTION
*
*       SCK_SearchTaskList
*
*   DESCRIPTION
*
*       This function is responsible for searching the task list table
*       structures until an entry with a match for the Task ID and server
*       port number specified by the caller is found.
*
*   INPUTS
*
*       *task_entry             Pointer to the task table
*       state                   What state the task is in
*       socketd                 The socket descriptor
*
*   OUTPUTS
*
*       > = 0                   The index
*       -1                      The search failed
*
*************************************************************************/
INT SCK_SearchTaskList(struct TASK_TABLE_STRUCT *task_entry, INT16 state,
                       INT socketd)
{
    UINT16                      index;
    INT16                       match = NU_FALSE;

    for(index = 0; index < task_entry->total_entries; index++)
    {
        /* verify that there is a TCP_Ports number entered.  If a state
           of zero is passed in we don't care about the state.  So look
           for a match on the port_entry alone.
        */
        if ( (task_entry->socket_index[index] >= 0) &&
             ((socketd == -1) || (task_entry->socket_index[index] == socketd)) &&
             ((state == -1) || (task_entry->stat_entry[index] == state)) )
        {
            task_entry->current_idx = index;
            match = NU_TRUE;
            break;
        } /* if */
    } /* for */

    if (match == NU_TRUE)
        return (task_entry->current_idx);
    else
        return (-1);

} /* SCK_SearchTaskList */

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Check_Listeners
*
*   DESCRIPTION
*
*       This function checks to see if there is a socket listening for
*       connection requests on the specified port. If there is then the
*       socket descriptor of the listening socket is returned.
*
*   INPUTS
*
*       port_num                TCP port number to check for listeners.
*       family                  The family - ipv4 or ipv6
*       tcp_chk                 Pointer to the pseudo TCP checksum
*
*   OUTPUTS
*
*       INT                     Socket descriptor of a listener.
*       -1                      There is no listener.
*
*************************************************************************/
INT SCK_Check_Listeners(UINT16 port_num, INT16 family, const VOID *tcp_chk)
{
    static INT      last_listener = -1;
    INT             socketd = -1, saved_listener = -1;
    INT             i;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32          dest_addr, last_listener_local_addr;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    dest_addr = LONGSWAP(((struct pseudotcp*)(tcp_chk))->dest);
#endif

    /* This is a minor optimization. When a connection request comes in check
       to see if it is for the same listener as the previous connection
       request before searching all sockets. */
    if ( (last_listener != -1) && (SCK_Sockets[last_listener]) )
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        last_listener_local_addr =
            IP_ADDR(SCK_Sockets[last_listener]->s_local_addr.ip_num.is_ip_addrs);
#endif

        /* Check the previous match. The socket must still be a listener, and
           it has to be listening on the correct port number. */
        if ( (SCK_Sockets[last_listener]->s_flags & SF_LISTENER) &&
             (SCK_Sockets[last_listener]->s_local_addr.port_num == port_num) &&

             /* If the family type of the socket is the same as the foreign
              * host or the family type of the socket is IPv6.  A listening IPv4
              * socket can accept incoming connections only from IPv4 clients.
              */
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
             ((SCK_Sockets[last_listener]->s_family == family) ||
              (SCK_Sockets[last_listener]->s_family == SK_FAM_IP6)) &&
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

            ((

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )

             /* A listening IPv4 socket can accept incoming connections only from
              * IPv4 clients.
              */
             (SCK_Sockets[last_listener]->s_family == SK_FAM_IP) &&
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
               (!(SCK_Sockets[last_listener]->s_options & SO_IPV6_V6ONLY)) &&
#endif
               ((last_listener_local_addr == IP_ADDR_ANY) ||
                (last_listener_local_addr == dest_addr)))

#if (INCLUDE_IPV6 == NU_TRUE)
               ||
#else
               )
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

             /* A listening IPv6 socket bound to the wildcard address can accept
              * incoming connections from IPv4 or IPv6 clients.  A listening IPv6 socket
              * bound to any address other than an IPv4-mapped address can accept
              * connections only from IPv6 clients.
              */
             ((SCK_Sockets[last_listener]->s_family == SK_FAM_IP6) &&
              (((IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[last_listener]->s_local_addr.ip_num.is_ip_addrs)) &&
               ((family == SK_FAM_IP6) || (!(SCK_Sockets[last_listener]->s_options & SO_IPV6_V6ONLY)))) ||
               ((family == SK_FAM_IP6) &&
                (memcmp(SCK_Sockets[last_listener]->s_local_addr.ip_num.is_ip_addrs,
                        (((struct pseudohdr*)(tcp_chk))->dest),
                        IP6_ADDR_LEN) == 0))
#if (INCLUDE_IPV4 == NU_TRUE)
               || ((family == SK_FAM_IP) &&
                   (!(SCK_Sockets[last_listener]->s_options & SO_IPV6_V6ONLY)) &&
                   (IPV6_IS_ADDR_V4MAPPED(SCK_Sockets[last_listener]->s_local_addr.ip_num.is_ip_addrs)) &&
                   (IP_ADDR(&(SCK_Sockets[last_listener]->s_local_addr.ip_num.is_ip_addrs[12])) == dest_addr)))
#endif
                ))
#endif

        )
        {
            /* If the SO_REUSEADDR socket option has been set on a socket,
             * do not use a WILDCARD match for last_listener.
             */
            if (
#if (INCLUDE_SO_REUSEADDR == NU_TRUE)
            	 (!SCK_ReuseAddr_Set) ||
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                 (
#if (INCLUDE_IPV6 == NU_TRUE)
                  (SCK_Sockets[last_listener]->s_family == SK_FAM_IP) &&
#endif
                  (last_listener_local_addr != IP_ADDR_ANY))
#if (INCLUDE_IPV6 == NU_TRUE)
                  ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                 (
#if (INCLUDE_IPV4 == NU_TRUE)
                  (SCK_Sockets[last_listener]->s_family == SK_FAM_IP6) &&
#endif
                  (!(IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[last_listener]->s_local_addr.ip_num.is_ip_addrs))))
#endif
               )
                socketd = last_listener;
        }
    }

    /* Either there was no previous listener or the previous one did not match.
       Search for a match. */
    if (socketd == -1)
    {
        for (i = 0; i < NSOCKETS; i++)
        {
            if ( (SCK_Sockets[i]) &&
                 (SCK_Sockets[i]->s_flags & SF_LISTENER) &&
                 (SCK_Sockets[i]->s_local_addr.port_num == port_num) &&

                 /* If the family type of the socket is the same as the foreign
                  * host or the family type of the socket is IPv6.  A listening IPv4
                  * socket can accept incoming connections only from IPv4 clients.
                  */
                 ((SCK_Sockets[i]->s_family == family)
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
                  || (SCK_Sockets[i]->s_family == SK_FAM_IP6)
#endif
                  ) &&

#if (INCLUDE_IPV4 == NU_TRUE)

                 ((

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
                 /* A listening IPv4 socket can accept incoming connections only from
                  * IPv4 clients.
                  */
                  (SCK_Sockets[i]->s_family == SK_FAM_IP) &&
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                  (!(SCK_Sockets[i]->s_options & SO_IPV6_V6ONLY)) &&
#endif
                  ((IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs) == IP_ADDR_ANY) ||
                   (IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs) == dest_addr)))

#if (INCLUDE_IPV6 == NU_TRUE)
                ||
#else
                )
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

                 /* A listening IPv6 socket bound to the wildcard address can accept
                  * incoming connections from IPv4 or IPv6 clients.  A listening IPv6 socket
                  * bound to any address other than an IPv4-mapped address can accept
                  * connections only from IPv6 clients.
                  */
                 ((SCK_Sockets[i]->s_family == SK_FAM_IP6) &&
                  (((IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs)) &&
                   ((family == SK_FAM_IP6) || (!(SCK_Sockets[i]->s_options & SO_IPV6_V6ONLY)))) ||
                   ((family == SK_FAM_IP6) &&
                    (memcmp(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs,
                            (((struct pseudohdr*)(tcp_chk))->dest),
                            IP6_ADDR_LEN) == 0))
#if (INCLUDE_IPV4 == NU_TRUE)

                   || ((family == SK_FAM_IP) &&
                       (!(SCK_Sockets[i]->s_options & SO_IPV6_V6ONLY)) &&
                       (IPV6_IS_ADDR_V4MAPPED(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs)) &&
                       (IP_ADDR(&(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs[12])) == dest_addr)))
#endif
                    ))
#endif
            )
            {
#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

                /* If the SO_REUSEADDR socket option has been set on a socket,
                 * check for a specific match before accepting a WILDCARD match.
                 */
                if ( (SCK_ReuseAddr_Set) && (
#if (INCLUDE_IPV4 == NU_TRUE)
                     (
#if (INCLUDE_IPV6 == NU_TRUE)
                      (SCK_Sockets[i]->s_family == SK_FAM_IP) &&
#endif
                      (IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs) == IP_ADDR_ANY))
#if (INCLUDE_IPV6 == NU_TRUE)
                      ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                     (
#if (INCLUDE_IPV4 == NU_TRUE)
                      (SCK_Sockets[i]->s_family == SK_FAM_IP6) &&
#endif
                      (IPV6_IS_ADDR_UNSPECIFIED(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs)))
#endif
                   ) )
                    saved_listener = i;

                /* An exact match was found. */
                else
#endif
                {
                    socketd = last_listener = i;
                    break;
                }
            }
        }
    }

    /* If an exact match was not found, use the WILDCARD match.  Do not save
     * this socket off as the last_listener, since a WILDCARD match is never
     * used for the last_listener if the SO_REUSEADDR option has been set.
     */
    if ( (socketd == -1) && (saved_listener != -1) )
        socketd = saved_listener;

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

    /* If the incoming client is IPv4 and the listening socket is IPv6,
     * flag the socket as IPv4-Mapped.
     */
    if ( (socketd != -1) && (family == SK_FAM_IP) &&
         (SCK_Sockets[socketd]->s_family == SK_FAM_IP6) )
        SCK_Sockets[socketd]->s_flags |= SF_V4_MAPPED;

#endif

    return (socketd);

} /* SCK_Check_Listeners */

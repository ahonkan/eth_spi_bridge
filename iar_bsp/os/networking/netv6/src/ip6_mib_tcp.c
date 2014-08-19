/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*        ip6_mib_tcp.c                               
*
*   COMPONENT
*
*        IP6- TCP MIB
*
*   DESCRIPTION
*
*        This file contains the functions to maintain 'ipv6TcpConnTable'
*        (IPv6 Connection Table).
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_MIB_TCP_Compare
*        IP6_MIB_TCP_Socket_List_Info
*        IP6_MIB_TCP_Get_State
*        IP6_MIB_TCP_Set_State
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IPV6_TCP_MIB == NU_TRUE)

STATIC INT IP6_MIB_TCP_Compare(UINT8 *, UINT16, UINT8 *, UINT16, UINT32, 
                               INT);
STATIC SOCKET_STRUCT *IP6_MIB_TCP_Socket_List_Info(UINT8, UINT8 *, 
                                                   UINT16 *, UINT8 *, 
                                                   UINT16 *, UINT32 *);

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_TCP_Compare
*
*   DESCRIPTION
*
*        This function is used to compare two TCP connection for indexes.
*
*   INPUTS
*
*        *local_addr            Local address.
*        local_port             Local port.
*        *remote_addr           Remote address.
*        remote_port            Remote port.
*        if_index               Interface index.
*        sock_index             Socket descriptor to compare with.
*
*   OUTPUTS
*
*        0                      Index passed in have the exact indexes of
*                               the socket descriptor.
*        > 0                    Indexes passed in are greater than that of
*                               socket.
*        < 0                    Indexes passed in are lesser than that of
*                               socket.
*
************************************************************************/
STATIC INT IP6_MIB_TCP_Compare(UINT8 *local_addr, UINT16 local_port,
                               UINT8 *remote_addr, UINT16 remote_port,
                               UINT32 if_index, INT sock_index)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Variable to hold the comparison result. */
    int                 cmp_result;

    /* Comparing local addresses. */
    cmp_result = memcmp(local_addr, SCK_Sockets[sock_index]->s_local_addr.
                        ip_num.is_ip_addrs, MAX_ADDRESS_SIZE);

    /* If local addresses are the equal. */
    if (cmp_result == 0)
    {
        /* Comparing local port numbers. */
        if (local_port > SCK_Sockets[sock_index]->s_local_addr.port_num)
            cmp_result = 1;

        else if (local_port < SCK_Sockets[sock_index]->s_local_addr.
                              port_num)
            cmp_result = -1;

        else /* If local port numbers are the same. */
        {
            /* Comparing remote addresses. */
            cmp_result = 
                memcmp(remote_addr, SCK_Sockets[sock_index]->
                       s_foreign_addr.ip_num.is_ip_addrs, 
                       MAX_ADDRESS_SIZE);

            /* If remote addresses are the same. */
            if (cmp_result == 0)
            {
                dev = DEV6_Get_Dev_By_Addr(local_addr);

                /* Comparing remote ports. */
                if (remote_port > SCK_Sockets[sock_index]->s_foreign_addr.
                                  port_num)
                    cmp_result = 1;

                else if (remote_port < SCK_Sockets[sock_index]->
                                       s_foreign_addr.port_num)
                    cmp_result = -1;

                /* If remote ports are the same. */
                else if ((if_index == 0) && (dev == NU_NULL))
                    cmp_result = 0;

                else if (dev != NU_NULL)
                {
                    if ((dev->dev_index + 1) < if_index)
                        cmp_result = 1;

                    else if ((dev->dev_index + 1) > if_index)
                        cmp_result = -1;
                }

                /* It is obvious here that if_index is 'zero' and dev is
                 * not NU_NULL. */
                else
                    cmp_result = -1;
            }
        }
    }

    /* Returning comparison result. */
    return (cmp_result);

} /* IP6_MIB_TCP_Compare */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_TCP_Socket_List_Info
*
*   DESCRIPTION
*
*        This used to get the handle to the socket structure.
*
*   INPUTS
*
*        getflag                Flag when 'true' represent the GET-REQUEST
*                               and when 'false' represent the
*                               GETNEXT-REQUEST.
*        *local_addr            Local address.
*        *local_port            Local port.
*        *remote_addr           Remote address.
*        *remote_port           Remote port.
*        *if_index              Interface index.
*
*   OUTPUTS
*
*        SOCKET_STRUCT*         Handle to the socket structure if found.
*        NU_NULL                If handle to the socket structure was not
*                               found.
*
************************************************************************/
STATIC SOCKET_STRUCT *IP6_MIB_TCP_Socket_List_Info(UINT8 getflag,
                                                   UINT8 *local_addr, 
                                                   UINT16 *local_port,
                                                   UINT8 *remote_addr, 
                                                   UINT16 *remote_port,
                                                   UINT32 *if_index)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY *dev;

    /* Variable to use in loop for searching socket structure, */
    INT             i;

    /* Variable to hold the index of logically next socket structure. */
    INT             next;

    /* Variable to hold the comparison result. */
    INT             cmp_result;

    /* Handle to socket structure to return. */
    SOCKET_STRUCT   *sock_info = NU_NULL;

    /* If it is a get request. */
    if (getflag)
    {
        /* Loop through the all socket structures. */
        for (i = 0; i < NSOCKETS; i++)
        {
            /* If we have reached a socket that have the same indexes as
             * passed in then get the handle to socket structure and
             * return that handle.
             */
            if ( (SCK_Sockets[i] != NU_NULL) &&
                 (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_TCP) &&
                 (SCK_Sockets[i]->s_family == NU_FAMILY_IP6) &&
                 (SCK_Sockets[i]->s_local_addr.port_num == (*local_port)) &&
                 (SCK_Sockets[i]->s_foreign_addr.port_num == (*remote_port)) &&
                 (memcmp(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs,
                         local_addr, MAX_ADDRESS_SIZE) == 0) &&
                 (memcmp(SCK_Sockets[i]->s_foreign_addr.ip_num.is_ip_addrs,
                        remote_addr, MAX_ADDRESS_SIZE) == 0) )
            {
                dev = DEV6_Get_Dev_By_Addr(local_addr);

                if ( ( (dev == NU_NULL) && ((*if_index) == 0) ) ||
                     ( (dev != NU_NULL) &&
                       ((dev->dev_index + 1) == (*if_index)) ) )
                {
                    sock_info = SCK_Sockets[i];

                    break;
                }
            }
        }
    }

    /* This is get-next request. */
    else
    {
        /* Currently we have not determined the next socket structure. */
        next = -1;

        /* Loop through all the socket structures to find the next entry.
         */
        for (i = 0; i < NSOCKETS; i++)
        {
            /* If this is a TCPv6 socket. */
            if ( (SCK_Sockets[i] != NU_NULL) &&
                 (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_TCP) &&
                 (SCK_Sockets[i]->s_family == NU_FAMILY_IP6) )
            {
                /* Check if the current socket is a candidate of being the
                 * next.
                 */
                cmp_result = IP6_MIB_TCP_Compare(local_addr,
                                                 (*local_port),
                                                 remote_addr,
                                                 (*remote_port),
                                                 (*if_index), i);

                /* If current socket is a candidate of being the next. */
                if (cmp_result < 0)
                {
                    /* If we already have the candidate. */
                    if (next >= 0)
                    {
                        /* Determine the better candidate. */
                        cmp_result = 
                            IP6_MIB_TCP_Compare(SCK_Sockets[i]->s_local_addr.
                                                ip_num.is_ip_addrs,
                                                SCK_Sockets[i]->s_local_addr.
                                                port_num, 
                                                SCK_Sockets[i]->s_foreign_addr.
                                                ip_num.is_ip_addrs,
                                                SCK_Sockets[i]->s_foreign_addr.
                                                port_num,
                                                ((DEV6_Get_Dev_By_Addr(SCK_Sockets[i]->
                                                  s_local_addr.ip_num.is_ip_addrs))->
                                                  dev_index + 1), 
                                                next);

                        /* If current socket is a better candidate then
                         * update the next socket index.
                         */
                        if (cmp_result < 0)
                            next = i;
                    }

                    /* If we don't have candidate then select current
                     * socket as candidate.
                     */
                    else
                        next = i;
                }
            }
        }

        /* If we found the next candidate then return handle to it. */
        if (next >= 0)
            sock_info = SCK_Sockets[next];
    }

    /* Return handle to the socket structure if one is found. */
    return (sock_info);

} /* IP6_MIB_TCP_Socket_List_Info */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_TCP_Get_State
*
*   DESCRIPTION
*
*        This function is used to get the value of 'ipv6TcpConnState'.
*
*   INPUTS
*
*        getflag                Flag when 'true' represent the GET-REQUEST
*                               and when 'false' represent the
*                               GETNEXT-REQUEST.
*        *local_addr            Local address.
*        *local_port            Local port.
*        *remote_addr           Remote address.
*        *remote_port           Remote port.
*        *if_index              Interface index.
*        *sock_state            Pointer to memory where socket state is to
*                               be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   We did not get the socket structure.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_TCP_Get_State(UINT8 getflag, UINT8 *local_addr,
                             UINT16 *local_port, UINT8 *remote_addr,
                             UINT16 *remote_port, UINT32 *if_index,
                             UINT32 *sock_state)
{
    /* Handle to the socket structure. */
    SOCKET_STRUCT   *sock_ptr;

    /* Handle to the interface device. */
    DV_DEVICE_ENTRY *dev;

    /* Status to return success or error code. */
    UINT16          status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Get the handle to the socket structure. */
        sock_ptr = IP6_MIB_TCP_Socket_List_Info(getflag, local_addr,
                                                local_port, remote_addr,
                                                remote_port, if_index);

        /* If we successfully got the handle to the socket structure then
         * get the value of connection state.
         */
        if (sock_ptr)
        {
            /* If socket is associated to a port then socket state in
             * socket structure truly represent the state. Otherwise
             * either this listener or connection is closed.
             */
            if (sock_ptr->s_port_index >= 0)
                (*sock_state) = sock_ptr->s_state;

            /* If it is listener. */
            else if (sock_ptr->s_flags & SF_LISTENER)
                (*sock_state) = SLISTEN;

            /* If connection is closed. */
            else
                (*sock_state) = SCLOSED;

            /* Return success code. */
            status = IP6_MIB_SUCCESS;

            /* If this was GET-NEXT request then update the indexes
             * passed in.
             */
            if (getflag == 0)
            {
                /* Update local address. */
                NU_BLOCK_COPY(local_addr, sock_ptr->s_local_addr.ip_num.
                                is_ip_addrs, MAX_ADDRESS_SIZE);

                /* Update remote address. */
                NU_BLOCK_COPY(remote_addr, sock_ptr->s_foreign_addr.
                                ip_num.is_ip_addrs, MAX_ADDRESS_SIZE);

                /* Update local port number. */
                (*local_port) = sock_ptr->s_local_addr.port_num;

                /* Update remote port number. */
                (*remote_port) = sock_ptr->s_foreign_addr.port_num;

                /* Get the handle to the interface device. */
                dev = DEV6_Get_Dev_By_Addr(sock_ptr->s_local_addr.ip_num.
                                           is_ip_addrs);

                /* Update the interface index passed in. */
                if (dev)
                    (*if_index) = dev->dev_index + 1;
                else
                    (*if_index) = 0;
            }
        }

        /* If we did not get the handle to the socket structure then
         * return error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_TCP_Get_State */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_TCP_Set_State
*
*   DESCRIPTION
*
*        This function is used to set the value of socket state.
*
*   INPUTS
*
*        *local_addr            Local address.
*        local_port             Local port.
*        *remote_addr           Remote address.
*        remote_port            Remote port.
*        if_index               Interface index.
*        sock_state             Socket state value to set.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_ERROR          General error.
*        IP6_MIB_NOSUCHOBJECT   Failed to get socket structure.
*        IP6_MIB_WRONGVALUE     Invalid to set.
*
************************************************************************/
UINT16 IP6_MIB_TCP_Set_State(UINT8 *local_addr, UINT16 local_port,
                             UINT8 *remote_addr, UINT16 remote_port,
                             UINT32 if_index, UINT32 sock_state)
{
    /* Handle to the socket structure. */
    SOCKET_STRUCT   *sock_ptr;

    /* Socket descriptor. */
    INT             socketd = -1;

    /* Status to return success or error code. */
    UINT16          status;

    /* If we have valid value 'deleteTCB(12)' then proceed. Otherwise
     * return error code.
     */
    if (sock_state == IP6_MIB_TCB_DELETE)
    {
        /* Grab the semaphore. */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
        {
            /* Get the handle to the socket structure. */
            sock_ptr = 
                IP6_MIB_TCP_Socket_List_Info((UINT8)NU_TRUE, local_addr,
                                             &local_port, remote_addr,
                                             &remote_port, &if_index);

            /* If successfully got the handle to the socket structure then
             * update the value socket descriptor.
             */
            if (sock_ptr)
                socketd = TCP_Ports[sock_ptr->s_port_index]->p_socketd;

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release the semaphore", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* If we have the handle to the socket structure then make
             * call to abort the TCP connection.
             */
            if (sock_ptr)
            {
                /* Abort TCP connection. If successful return success code. */
                if (NU_Abort(socketd) == NU_SUCCESS)
                    status = IP6_MIB_SUCCESS;

                /* If TCP connection not aborted properly then return
                 * error code.
                 */
                else
                    status = IP6_MIB_ERROR;
            }

            /* If we did not get the handle to the socket structure then
             * return error code.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* If we failed to grab the semaphore. */
        else
        {
            NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            /* Return error code. */
            status = IP6_MIB_ERROR;
        }
    }

    /* If socket state value is invalid then return error code. */
    else
        status = IP6_MIB_WRONGVALUE;

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_TCP_Set_State */

#endif /* (INCLUDE_IPV6_TCP_MIB == NU_TRUE) */

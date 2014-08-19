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
*        mib2_tcp.c
*
* COMPONENT
*
*        MIB II - TCP Group.
*
* DESCRIPTION
*
*        This file contain the functions that are responsible for
*        maintaining statistics for TCP group.
*
* DATA STRUCTURES
*
*        Mib2_Tcp_Data
*
* FUNCTIONS
*
*        MIB2_TcpEntry_GetNext
*        MIB2_Get_TcpConnState
*        MIB2_Set_TcpConnState
*        MIB2_Get_TcpConnLocalAddress
*        MIB2_Get_TcpConnLocalPort
*        MIB2_Get_TcpConnRemAddress
*        MIB2_Get_TcpConnRemPort
*        MIB2_SizeTcpTab
*        MIB2_Tcp_Compare
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)
#if ( (MIB2_IP_INCLUDE == NU_TRUE) || (MIB2_AT_INCLUDE == NU_TRUE) )
#if ( (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) )

MIB2_TCP_STRUCT              Mib2_Tcp_Data;

INT     MIB2_Tcp_Compare(UINT32, UINT32, UINT16, UINT16,
                         UINT32, UINT32, UINT16, UINT16);

STATUS  MIB2_TcpEntry_GetNext(UINT8 *, UINT32 *, UINT8 *, UINT32 *,
                              SOCKET_STRUCT *);

/************************************************************************
*
*   FUNCTION
*
*       MIB2_TcpEntry_GetNext
*
*   DESCRIPTION
*
*       This function gets a pointer to the next socket.
*
*   INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *remote_addr            The local remote address.
*       *remote_port            The local remote port.
*       *socket_info            Pointer to the data structure to
*                               fill in with the socket information.
*
*   OUTPUTS
*
*       NU_TRUE                 if information are found
*       NU_FALSE                if no information are found
*
*************************************************************************/
STATUS MIB2_TcpEntry_GetNext(UINT8 *local_addr, UINT32 *local_port,
                             UINT8 *remote_addr, UINT32 *remote_port,
                             SOCKET_STRUCT *socket_info)
{
    INT     i;
    INT     next = -1;
    INT     result;
    STATUS  status;

    /* Look for the smallest entry which is "greater" than the one passed. */
    for (i = 0; i < NSOCKETS; i++)
    {
        /* Check if the protocol is TCP and the family is IPv4 */
        if ( (SCK_Sockets[i] != NU_NULL) &&
             (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_TCP) &&
             (SCK_Sockets[i]->s_family == NU_FAMILY_IP) )
        {
            result =
                MIB2_Tcp_Compare(IP_ADDR(local_addr), IP_ADDR(remote_addr),
                                 ((UINT16)(*local_port)), ((UINT16)(*remote_port)),
                                 IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs),
                                 IP_ADDR(SCK_Sockets[i]->s_foreign_addr.ip_num.is_ip_addrs),
                                 SCK_Sockets[i]->s_local_addr.port_num,
                                 SCK_Sockets[i]->s_foreign_addr.port_num);

            if (result < 0)
            {
                /* Found an entry which is greater. */

                if (next < 0)
                {
                    /* This is the first such entry. */
                    next = i;
                }
                else
                {
                    /* Check to see if we have found a socket which is between our current next
                       and the original values passed to the function. */
                    result =
                        MIB2_Tcp_Compare(IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs),
                                         IP_ADDR(SCK_Sockets[i]->s_foreign_addr.ip_num.is_ip_addrs),
                                         SCK_Sockets[i]->s_local_addr.port_num,
                                         SCK_Sockets[i]->s_foreign_addr.port_num,
                                         IP_ADDR(SCK_Sockets[next]->s_local_addr.ip_num.is_ip_addrs),
                                         IP_ADDR(SCK_Sockets[next]->s_foreign_addr.ip_num.is_ip_addrs),
                                         SCK_Sockets[next]->s_local_addr.port_num,
                                         SCK_Sockets[next]->s_foreign_addr.port_num);

                    if (result < 0)
                        next = i;
                }
            }
        }
    }

    /* Check if there exists a value greater than the one passed. */
    if (next < 0)
        status = NU_FALSE;

    else
    {
        /* Update the addr variables and the ports. */
        memcpy(local_addr, SCK_Sockets[next]->s_local_addr.ip_num.is_ip_addrs,
               IP_ADDR_LEN);

        memcpy(remote_addr, SCK_Sockets[next]->s_foreign_addr.ip_num.is_ip_addrs,
               IP_ADDR_LEN);

        *local_port = SCK_Sockets[next]->s_local_addr.port_num;
        *remote_port = SCK_Sockets[next]->s_foreign_addr.port_num;

        /* copying the information from list to data-structures */
        memcpy(socket_info, SCK_Sockets[next], sizeof(SOCKET_STRUCT));

        /* Return the success representing next element is found */
        status = NU_TRUE;
    }

    return (status);

} /* MIB2_TcpEntry_GetNext */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_TcpConnState
*
* DESCRIPTION
*
*       This function gets the state of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *remote_addr            The local remote address.
*       *remote_port            The local remote port.
*       *state                  The memory location where the current
*                               state will be placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_TcpConnState(UINT8 *local_addr, UINT32 *local_port,
                            UINT8 *remote_addr, UINT32 *remote_port,
                            UINT32 *state, UINT8 getflag)
{
    SOCKET_STRUCT   socket_info;
    INT16           status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* call the function to get the required information from a
     * particular socket
     */
    if (MIB2_Socket_List_Information(NU_PROTO_TCP, getflag, local_addr,
                                     local_port, remote_addr, remote_port,
                                     &socket_info) == NU_TRUE)
    {
        if (socket_info.s_flags & SF_LISTENER)
        {
            *state = SLISTEN;
        }

        else if (socket_info.s_port_index >= 0)
        {
            *state = TCP_Ports[socket_info.s_port_index]->state;
        }

        else
        {
            *state = SCLOSED;
        }

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    return (status);

} /* MIB2_Get_TcpConnState */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Set_TcpConnState
*
* DESCRIPTION
*
*       This function sets the state of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       local_port              The local port.
*       *remote_addr            The local remote address.
*       remote_port             The local remote port.
*       state                   The new state.
*
* OUTPUTS
*
*       INT16                   1 if NU_SUCCESS
*                               2 if SNMP_GENERROR
*                               3 if SNMP_NOSUCHNAME
*                               4 if SNMP_BADVALUE
*
*************************************************************************/
INT16 MIB2_Set_TcpConnState(UINT8 *local_addr, UINT32 local_port,
                            UINT8 *remote_addr, UINT32 remote_port,
                            UINT32 state)
{
    SOCKET_STRUCT       socket_info;
    INT16               status;
    UINT8               getflag = 1;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* call the function to get the required information from a
     * particular socket
     */
    status = (INT16)MIB2_Socket_List_Information(NU_PROTO_TCP, getflag,
                                                 local_addr, &local_port,
                                                 remote_addr, &remote_port,
                                                 &socket_info);

    /* if exact match is found then update the connection state */
    if ( (status == NU_TRUE) && (state == 12) )
    {
        if (NU_Abort(TCP_Ports[socket_info.s_port_index]->p_socketd)
                                                            == NU_SUCCESS)
            status = 1;
        else
            status = 2;
    }

    else if (status != NU_TRUE)
        status = 4;

    else
        status = 3;

    return (status);

} /* MIB2_Set_TcpConnState */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_TcpConnLocalAddress
*
* DESCRIPTION
*
*       This function gets the local address of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *remote_addr            The local remote address.
*       *remote_port            The local remote port.
*       *addr                   The memory location where the local
*                               address will be placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_TcpConnLocalAddress(UINT8 *local_addr, UINT32 *local_port,
                                   UINT8 *remote_addr, UINT32 *remote_port,
                                   UINT8 *addr, UINT8 getflag)
{
    SOCKET_STRUCT   socket_info;
    INT16           status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* call the function to get the required information from a
     * particular socket
     */
    if (MIB2_Socket_List_Information(NU_PROTO_TCP, getflag, local_addr,
                                     local_port, remote_addr, remote_port,
                                     &socket_info) == NU_TRUE)
    {
        memcpy(addr, socket_info.s_local_addr.ip_num.is_ip_addrs,
               IP_ADDR_LEN);

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    return (status);

} /* MIB2_Get_TcpConnLocalAddress */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_TcpConnLocalPort
*
* DESCRIPTION
*
*       This function gets the local port of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *remote_addr            The local remote address.
*       *remote_port            The local remote port.
*       *port                   The memory location where the local port
*                               will be placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_TcpConnLocalPort(UINT8 *local_addr, UINT32 *local_port,
                                UINT8 *remote_addr, UINT32 *remote_port,
                                UINT32 *port, UINT8 getflag)
{
    SOCKET_STRUCT   socket_info;
    INT16           status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* call the function to get the required information from a
     * particular socket
     */
    if (MIB2_Socket_List_Information(NU_PROTO_TCP, getflag, local_addr,
                                     local_port, remote_addr, remote_port,
                                     &socket_info) == NU_TRUE)
    {
        *port = (UINT32)socket_info.s_local_addr.port_num;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    return (status);

} /* MIB2_Get_TcpConnLocalPort */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_TcpConnRemAddress
*
* DESCRIPTION
*
*       This function gets the remote address of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *remote_addr            The local remote address.
*       *remote_port            The local remote port.
*       *addr                   The memory location where the remote
*                               address will be placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_TcpConnRemAddress(UINT8 *local_addr, UINT32 *local_port,
                                 UINT8 *remote_addr, UINT32 *remote_port,
                                 UINT8 *addr, UINT8 getflag)
{
    SOCKET_STRUCT   socket_info;
    INT16           status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* call the function to get the required information from a
     * particular socket
     */
    if (MIB2_Socket_List_Information(NU_PROTO_TCP, getflag, local_addr,
                                     local_port, remote_addr, remote_port,
                                     &socket_info) == NU_TRUE)
    {
        memcpy(addr, socket_info.s_foreign_addr.ip_num.is_ip_addrs,
               IP_ADDR_LEN);

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    return (status);

} /* MIB2_Get_TcpConnRemAddress */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_TcpConnRemPort
*
* DESCRIPTION
*
*       This function gets the remote port of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *remote_addr            The local remote address.
*       *remote_port            The local remote port.
*       *port                   The memory location where the remote port
*                               will be placed.
*       getflag                 NU_TRUE if an exact match is required or
*                               NU_FALSE if the next element is required.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Get_TcpConnRemPort(UINT8 *local_addr, UINT32 *local_port,
                              UINT8 *remote_addr, UINT32 *remote_port,
                              UINT32 *port, UINT8 getflag)
{
    SOCKET_STRUCT   socket_info;
    INT16           status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* call the function to get the required information from a
     * particular socket
     */
    if (MIB2_Socket_List_Information(NU_PROTO_TCP, getflag, local_addr,
                                     local_port, remote_addr, remote_port,
                                     &socket_info) == NU_TRUE)
    {
        *port = (UINT32) socket_info.s_foreign_addr.port_num;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    return (status);

} /* MIB2_Get_TcpConnRemPort */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_SizeTcpTab
*
* DESCRIPTION
*
*       This function gets the size of the virtual TCP Table.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       UINT32                  Returns the number of TCP connections.
*
*************************************************************************/
UINT32 MIB2_SizeTcpTab(VOID)
{
    UINT32      size = 0;
    INT         i;
        STATUS      status;

    /* Grab the semaphore and traverse the socket list */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        for (i = 0; i < NSOCKETS; i++)
        {
            /* Check if the socket is not NULL and the protocol is TCP */
            if ( (SCK_Sockets[i]) &&
                 (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_TCP) &&
                 (SCK_Sockets[i]->s_family == NU_FAMILY_IP) )
            {
                /* Increment size. */
                size++;
            }
        }

        /* Release the semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }
    }

     /* The semaphore could not be obtained */
    else
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }

    return (size);

} /* MIB2_SizeTcpTab */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Tcp_Compare
*
* DESCRIPTION
*
*       This function compares two IP address / port number combinations
*       to determine which is larger.
*
* INPUTS
*
*       first_local_addr        The first local IP address.
*       first_remote_addr       The first remote IP address.
*       first_local_port        The corresponding local port number.
*       first_remote_port       The corresponding remote port number.
*       second_local_addr       The second local IP address.
*       second_remote_addr      The second remote IP address.
*       second_local_port       The corresponding local port number.
*       second_remote_port      The corresponding remote port number.
*
* OUTPUTS
*
*       0                       The two are the same.
*       1                       The 1st is larger.
*       -1                      The 2nd is larger.
*
*************************************************************************/
INT MIB2_Tcp_Compare(UINT32 first_local_addr, UINT32 first_remote_addr,
                     UINT16 first_local_port, UINT16 first_remote_port,
                     UINT32 second_local_addr, UINT32 second_remote_addr,
                     UINT16 second_local_port, UINT16 second_remote_port)
{
    INT result;

    /* Compare the local addresses of the two nodes.  If equal, go on to
     * compare more of the strings.
     */
    if (first_local_addr == second_local_addr)
    {
        /* Compare the local ports */
        if (first_local_port < second_local_port)
            result = -1;

        else if (first_local_port > second_local_port)
            result = 1;

        else
        {
            /* Compare the foreign addresses */
            if (first_remote_addr == second_remote_addr)
            {
                /* Compare the foreign ports */
                if (first_remote_port < second_remote_port)
                    result = -1;
                else if (first_remote_port > second_remote_port)
                    result = 1;
                else
                    result = 0;
            }
            else if (first_remote_addr > second_remote_addr)
                result = 1;
            else
                result = -1;
        }
    }

    /* If the first local address is less than the second, the first
     * string is smaller.
     */
    else if (first_local_addr < second_local_addr)
        result = -1;

    /* Otherwise, the first string is larger */
    else
        result = 1;

    return (result);

} /* MIB2_Tcp_Compare */

#endif /* ( (MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) ) */
#endif /* ( (MIB2_IP_INCLUDE == NU_TRUE) || (MIB2_AT_INCLUDE == NU_TRUE) ) */

#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */

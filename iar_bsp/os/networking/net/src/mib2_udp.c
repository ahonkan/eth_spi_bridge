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
*        mib2_udp.c
*
* COMPONENT
*
*        MIB II - UDP Group.
*
* DESCRIPTION
*
*        This file contain the functions that are responsible for
*        maintaining statistics for TCP group.
*
* DATA STRUCTURES
*
*        Mib2_Udp_Data
*
* FUNCTIONS
*
*        MIB2_UdpEntry_GetNext
*        MIB2_Get_UdpConnLocalAddress
*        MIB2_Get_UdpConnLocalPort
*        MIB2_SizeUdpTab
*        MIB2_Udp_Compare
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if(INCLUDE_MIB2_RFC1213 == NU_TRUE)

/*-----------------------------------------------------------------------
 * UDP GROUP
 *----------------------------------------------------------------------*/

#if ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) )

MIB2_UDP_STRUCT              Mib2_Udp_Data;

/* UDP Group. */
STATUS                  MIB2_UdpEntry_GetNext(UINT8 *, UINT32 *,
                                              SOCKET_STRUCT *);
INT                     MIB2_Udp_Compare(UINT32, UINT16, UINT32, UINT16);


#endif /* ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) ) */

#if ( (MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) )

/************************************************************************
*
*   FUNCTION
*
*       MIB2_UdpEntry_GetNext
*
*   DESCRIPTION
*
*       This function gets a pointer to the next socket.
*
*   INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
*       *socket_info            Pointer to the data structure into
*                               which to copy the socket information.
*
*   OUTPUTS
*
*       NU_FALSE                If a connection to socket with the
*                               specified addresses was not found.
*       NU_TRUE                 If a connection to socket with the
*                               specified addresses was found.
*
*************************************************************************/
STATUS MIB2_UdpEntry_GetNext(UINT8 *local_addr, UINT32 *local_port,
                             SOCKET_STRUCT *socket_info)
{
    INT     i;
    INT     next = -1;
    INT     result;
    STATUS  status;

    /* Look for an entry which is "greater" than the one passed. */
    for (i = 0; i < NSOCKETS; i++)
    {
        /* Check if the protocol is UDP and the family is IPv4 */
        if ( (SCK_Sockets[i] != NU_NULL) &&
             (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_UDP) &&
             (SCK_Sockets[i]->s_family == NU_FAMILY_IP) )
        {
            result =
                MIB2_Udp_Compare(IP_ADDR(local_addr), ((UINT16)(*local_port)),
                                 IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs),
                                 SCK_Sockets[i]->s_local_addr.port_num);
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
                    /* Check to see if we have found a socket which
                     * is between our current next and the original
                     * values passed to the function.
                     */
                    result =
                        MIB2_Udp_Compare(IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs),
                                         SCK_Sockets[i]->s_local_addr.port_num,
                                         IP_ADDR(SCK_Sockets[next]->s_local_addr.ip_num.is_ip_addrs),
                                         SCK_Sockets[next]->s_local_addr.port_num);

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
        /* Update the addr variable and the port. */
        memcpy(local_addr, SCK_Sockets[next]->s_local_addr.ip_num.is_ip_addrs,
               IP_ADDR_LEN);

        *local_port = SCK_Sockets[next]->s_local_addr.port_num;

        /* copying the information from list to data-structures */
        memcpy(socket_info, SCK_Sockets[next], sizeof(SOCKET_STRUCT));

        /* Return the success representing next element is found*/
        status = NU_TRUE;
    }

    return (status);

} /* MIB2_UdpEntry_GetNext */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_UdpConnLocalAddress
*
* DESCRIPTION
*
*       This function gets the local address of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
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
INT16 MIB2_Get_UdpConnLocalAddress(UINT8 *local_addr, UINT32 *local_port,
                                   UINT8 *addr, UINT8 getflag)
{
    SOCKET_STRUCT       socket_info;
    INT16               status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* if required information is found then copy the value at
     * appropriate place
     */
    if (MIB2_Socket_List_Information(NU_PROTO_UDP, getflag, local_addr,
                                     local_port, NU_NULL, 0,
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

} /* MIB2_Get_UdpConnLocalAddress */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_UdpConnLocalPort
*
* DESCRIPTION
*
*       This function gets the local port of a socket.
*
* INPUTS
*
*       *local_addr             The local address.
*       *local_port             The local port.
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
INT16 MIB2_Get_UdpConnLocalPort(UINT8 *local_addr, UINT32 *local_port,
                                UINT32 *port, UINT8 getflag)
{
    SOCKET_STRUCT   socket_info;
    INT16           status;

    /* zero out the data structure */
    UTL_Zero(&socket_info, sizeof(SOCKET_STRUCT));

    /* if required information is found then copy the value at
     * appropriate place
     */
    if (MIB2_Socket_List_Information(NU_PROTO_UDP, getflag, local_addr,
                                     local_port, NU_NULL, 0,
                                     &socket_info) == NU_TRUE)
    {
        *port = (UINT32) socket_info.s_local_addr.port_num;
        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    return (status);

} /* MIB2_Get_UdpConnLocalPort */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_SizeUdpTab
*
* DESCRIPTION
*
*       This function gets the size of the virtual UDP Table.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       UINT32                  Returns the number of UDP connections.
*
*************************************************************************/
UINT32 MIB2_SizeUdpTab(VOID)
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
                 (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_UDP) &&
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

} /* MIB2_SizeUdpTab */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Udp_Compare
*
* DESCRIPTION
*
*       This function compares two IP address / port number combinations
*       to determine which is larger.
*
* INPUTS
*
*       first_addr              The first IP address.
*       first_port              The corresponding port number.
*       second_addr             The second IP address.
*       second_port             The corresponding port number.
*
* OUTPUTS
*
*       0                       The two are the same.
*       1                       The 1st is larger.
*       -1                      The 2nd is larger.
*
*************************************************************************/
INT MIB2_Udp_Compare(UINT32 first_addr, UINT16 first_port,
                     UINT32 second_addr, UINT16 second_port)
{
    INT result;

    if (first_addr == second_addr)
    {
        if (first_port < second_port)
            result = -1;

        else if (first_port > second_port)
            result = 1;

        else
            result = 0;
    }

    else if (first_addr < second_addr)
        result = -1;

    else
        result = 1;

    return (result);

} /* MIB2_Udp_Compare */

#endif /* ( (MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) ) */


#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */

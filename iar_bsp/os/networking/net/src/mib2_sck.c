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
*        mib2_sck.c
*
* COMPONENT
*
*        MIB II - Socket Group.
*
* DESCRIPTION
*
*        This file contain the functions that are responsible for
*        maintaining statistics for Socket group.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        MIB2_Socket_List_Information
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

#if ( ((MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE)) || \
      ((MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE)) )

#if ( (MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) )

extern INT MIB2_Tcp_Compare(UINT32, UINT32, UINT16, UINT16,
                            UINT32, UINT32, UINT16, UINT16);

extern STATUS MIB2_TcpEntry_GetNext(UINT8 *, UINT32 *, UINT8 *, UINT32 *,
                                    SOCKET_STRUCT *);

#endif /* ( (MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) ) */

#if ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) )

STATUS                  MIB2_UdpEntry_GetNext(UINT8 *, UINT32 *,
                                              SOCKET_STRUCT *);
INT                     MIB2_Udp_Compare(UINT32, UINT16, UINT32, UINT16);

#endif /* ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) ) */

/*-----------------------------------------------------------------------
 * socket list information
 *----------------------------------------------------------------------*/
/*************************************************************************
*
* FUNCTION
*
*       MIB2_Socket_List_Information
*
* DESCRIPTION
*
*       This function gets a required information after traversing
*       socket list.
*
* INPUTS
*
*       protocol                protocol (TCP/UDP)
*       getflag                 type of request (get or getnext)
*       *local_addr             local address.
*       *local_port             local port
*       *remote_addr            remote address
*       *remote_port            remote port
*       *socket_info            the data-structure in which information
*                               will be returned
*
* OUTPUTS
*
*       NU_FALSE                if no information are found
*       NU_TRUE                 if approximate information are found
*
*************************************************************************/
STATUS MIB2_Socket_List_Information(INT protocol, UINT8 getflag,
                                    UINT8 *local_addr, UINT32 *local_port,
                                    UINT8 *remote_addr, UINT32 *remote_port,
                                    SOCKET_STRUCT *socket_info)
{
    STATUS  status;
    INT     i, result;

#if ( (MIB2_TCP_INCLUDE == NU_FALSE) || (INCLUDE_TCP == NU_FALSE) )
    UNUSED_PARAMETER(remote_addr);
    UNUSED_PARAMETER(remote_port);
#endif /* ( (MIB2_TCP_INCLUDE == NU_FALSE) || (INCLUDE_TCP == NU_FALSE) ) */

    /* Obtain Semaphore, so that changes are not made while looking through
     * the table
     */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);

        return (NU_NULL);
    }

    status = NU_FALSE;

    /* at least local address should be present in both cases [TCP/UDP] */
    if (local_addr)
    {
        switch (protocol)
        {
#if ( (MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) )

            /* if the incoming protocol is TCP */
            case NU_PROTO_TCP:

                /* condition is true if it is a get request */
                if (getflag)
                {
                    /* Look for an entry which is equal to the one passed. */
                    for (i = 0; i < NSOCKETS; i++)
                    {
                        /* Check if the protocol of socket list entry is
                         * TCP
                         */
                        if ( (SCK_Sockets[i] != NU_NULL) &&
                             (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_TCP) &&
                             (SCK_Sockets[i]->s_family == NU_FAMILY_IP) )
                        {
                            /* call the function to check whether incoming
                             * parameters matches with the current socket
                             */
                            result = MIB2_Tcp_Compare(IP_ADDR(local_addr),
                                                      IP_ADDR(remote_addr),
                                                      ((UINT16)(*local_port)), ((UINT16)(*remote_port)),
                                                      IP_ADDR((SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs)),
                                                      IP_ADDR((SCK_Sockets[i]->s_foreign_addr.ip_num.is_ip_addrs)),
                                                      SCK_Sockets[i]->s_local_addr.port_num,
                                                      SCK_Sockets[i]->s_foreign_addr.port_num);

                            /* if condition is satisfied then return the
                             * information
                             */
                            if (result == 0)
                            {
                                /* copying the information from socket list to
                                 * socket_info
                                 */
                                memcpy((VOID*)socket_info, (VOID*)SCK_Sockets[i],
                                       (UINT32)sizeof(struct sock_struct));

                                status = NU_TRUE;
                                break;
                            }
                        }
                    } /* end for loop which traverse socket list for all sockets */
                }

                /* else clause will be true in case of getnext or getbulk request */
                else
                {
                    /* call the function to get information of next entry in
                     * the socket list
                     */
                    status = MIB2_TcpEntry_GetNext(local_addr, local_port,
                                                   remote_addr, remote_port,
                                                   socket_info);
                }

                break;
#endif /* ( (MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) ) */

#if ( (MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) )

            /* if the incoming protocol is UDP */
            case NU_PROTO_UDP:

                /* condition is true if it is a get request */
                if (getflag)
                {
                    /* Look for an entry which is equal to the one passed. */
                    for (i = 0; i < NSOCKETS; i++)
                    {
                        /* Check if the protocol is UDP */
                        if ( (SCK_Sockets[i] != NU_NULL) &&
                             (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_UDP) &&
                             (SCK_Sockets[i]->s_family == NU_FAMILY_IP) )
                        {
                            /* call the function to check whether incoming
                             * parameters matches with the current socket
                             */
                            result = MIB2_Udp_Compare(IP_ADDR(local_addr), ((UINT16)(*local_port)),
                                                      IP_ADDR(SCK_Sockets[i]->s_local_addr.ip_num.is_ip_addrs),
                                                      SCK_Sockets[i]->s_local_addr.port_num);

                            /*if condition is satisfied then return the information */
                            if (result == 0)
                            {
                                /* copying the information from socket list to
                                 * socket_info
                                 */
                                memcpy((VOID*)socket_info, (VOID*)SCK_Sockets[i],
                                       (UINT32)sizeof(struct sock_struct));

                                status = NU_TRUE;
                                break;
                            }
                        }
                    } /* end for loop which traverse socket list for all sockets */
                }

                /* else clause will be true in case of getnext or getbulk request */
                else
                {
                    /* call the function to get the next in the socket list */
                    status = MIB2_UdpEntry_GetNext(local_addr, local_port, socket_info);
                }

                break;
#endif /* ( (MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) ) */

            /* if none of above protocol is matched then simply return false */
            default:

                status = NU_FALSE;
                break;
        }

    }
    else
    {
        status = NU_FALSE;
    }

    /* release the semaphores because list traversing is over */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }

    return (status);

} /* MIB2_Socket_List_Information */

#endif /* ( ((MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE)) || \
            ((MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE)) ) */

#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */


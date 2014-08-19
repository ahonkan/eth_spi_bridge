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
*       sck_cs.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Close_Socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Close_Socket
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

extern tx_ancillary_data    NET_Sticky_Options_Memory[];
extern UINT8                NET_Sticky_Options_Memory_Flags[];

#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Close_Socket
*
*   DESCRIPTION
*
*       This function is responsible for breaking the socket connection
*       in the SCK_Sockets at the index specified by socketd.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful close.
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been
*                               previously allocated via the NU_Socket
*                               call.
*       NU_NOT_CONNECTED        The socket is not connected
*
*************************************************************************/
STATUS NU_Close_Socket(INT socketd)
{
#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || \
     (INCLUDE_IP_RAW == NU_TRUE))
    struct      sock_struct      *sockptr;
#endif
#if (INCLUDE_SOCKETS == NU_TRUE)
    INT         port_num;                       /* local machine's port number */
#endif
    STATUS      return_status;
#if (INCLUDE_IP_RAW == NU_TRUE)
#if ( (INCLUDE_STATIC_BUILD == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
    INT         j;
#endif
    IPR_PORT    *ipraw_ptr;
#endif
#if (INCLUDE_TCP == NU_TRUE)
    STATUS      close_status;              /* status returned from netclose */
    UINT32      socket_id;
#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) || \
     (INCLUDE_IP_RAW == NU_TRUE))
    sockptr = SCK_Sockets[socketd];
#endif

#if (INCLUDE_TCP == NU_TRUE)

    /*  Do different processing based on the protocol type.  */
    if (sockptr->s_protocol == NU_PROTO_TCP)
    {
        port_num  = sockptr->s_port_index;

        /* If the port number is valid continue with the close process */
        if ( (port_num >= 0) && (port_num < TCP_MAX_PORTS) )
        {

            /* Linger option is off */
            if (sockptr->s_linger.linger_on == NU_FALSE)
            {
                /* Mark the socket state as disconnecting. */
                SCK_DISCONNECTING(socketd);

                /* Close this connection. */
                TCPSS_Net_Close(port_num, sockptr);
            }

            /* Linger option is on and a time was specified */
            else if ( (sockptr->s_linger.linger_on == NU_TRUE) &&
                      (sockptr->s_linger.linger_ticks > 0) )
            {
                /* Mark the socket state as disconnecting. */
                SCK_DISCONNECTING(socketd);

                /* Close this connection. */
                close_status = TCPSS_Net_Close(port_num, sockptr);

                /* If net close did not return a successful value, we
                   will wait until the connection is closed or the
                   linger time expires. */
                if (close_status < 0)
                {
                    /* Verify no other tasks are suspended on this
                       socket closing */
                    if (sockptr->s_CLSTask != NU_NULL)
                        return_status = NU_SUCCESS;

                    else
                    {
                        /* Set a timer and suspend */
                        if (TQ_Timerset(SELECT, (UNSIGNED)NU_Current_Task_Pointer(),
                                        sockptr->s_linger.linger_ticks, 0) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to set timer to wake up task",
                                           NERR_SEVERE, __FILE__, __LINE__);

                        /* Setup so the stack can also restart the task */
                        sockptr->s_CLSTask = NU_Current_Task_Pointer();

                        /* Get the socket ID to verify the socket after suspension */
                        socket_id = sockptr->s_struct_id;

                        SCK_Suspend_Task(NU_Current_Task_Pointer());

                        /* We either resumed because the close completed or
                        the timeout. Attempt to remove the timer. */
                        TQ_Timerunset(SELECT, TQ_CLEAR_EXACT, (UNSIGNED)NU_Current_Task_Pointer(), 0);

                        /* If this is a different socket, handle it appropriately */
                        if ( (!SCK_Sockets[socketd]) ||
                             (SCK_Sockets[socketd]->s_struct_id != socket_id) )
                        {
                            /* Release the semaphore */
                            SCK_Release_Socket();

                            return (NU_SOCKET_CLOSED);
                        }

                        /* Remove the task reference from the socket structure */
                        sockptr->s_CLSTask = NU_NULL;
                    }
                }
            }

            /* No wait time is specified, abort the connection */
            else if ( (sockptr->s_linger.linger_on == NU_TRUE) &&
                      (sockptr->s_linger.linger_ticks == 0) )
            {
                /* Release the semaphore */
                SCK_Release_Socket();

                return (NU_Abort(socketd));
            }

            else
            {
                /* Release the semaphore */
                SCK_Release_Socket();

                /* The linger option is invalid */
                return (NU_INVAL);
            }
        }
    }
#endif /* INCLUDE_TCP == NU_TRUE */

#if (INCLUDE_UDP == NU_TRUE)

#if (INCLUDE_TCP == NU_TRUE)

    else if (sockptr->s_protocol == NU_PROTO_UDP)

#else

    if (sockptr->s_protocol == NU_PROTO_UDP)

#endif /* INCLUDE_TCP == NU_TRUE */

    {
        port_num = UDP_Get_Pnum(sockptr);

        if ((port_num >= 0) && (port_num < UDP_MAX_PORTS))
        {
            /* Free any datagrams that were never received by the application. */
            MEM_Buffer_Cleanup(&sockptr->s_recvlist);

            UDP_Port_Cleanup((UINT16)port_num, sockptr);
        }
        else
            /*  The port structure has already been deallocated by the stack. */
            return_status = NU_SUCCESS;
    }
#endif /* INCLUDE_UDP == NU_TRUE */

#if (INCLUDE_IP_RAW == NU_TRUE)

#if (INCLUDE_UDP == NU_TRUE)

    else if ( (IS_RAW_PROTOCOL(sockptr->s_protocol)) ||
              (sockptr->s_protocol == 0) )

#else

    if ( (IS_RAW_PROTOCOL(sockptr->s_protocol)) ||
         (sockptr->s_protocol == 0) )

#endif /* INCLUDE_UDP == NU_TRUE */

    {

        port_num = IPRaw_Get_PCB(socketd, sockptr);

        if (port_num >= 0)
        {
            ipraw_ptr = IPR_Ports[port_num];

            /* Free the cached route */
            if (ipraw_ptr->ipraw_route.rt_route)
            {
#if (INCLUDE_IPV6 == NU_TRUE)

                if ( (sockptr->s_family == SK_FAM_IP6)
#if (INCLUDE_IPV4 == NU_TRUE)
                     &&
                     (!(IPV6_IS_ADDR_V4MAPPED(sockptr->s_foreign_addr.ip_num.is_ip_addrs)))
#endif
                     )
                    RTAB_Free((ROUTE_ENTRY*)ipraw_ptr->ipraw_routev6.rt_route, NU_FAMILY_IP6);

#if (INCLUDE_IPV4 == NU_TRUE)
                else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                    RTAB_Free((ROUTE_ENTRY*)ipraw_ptr->ipraw_route.rt_route,
                              NU_FAMILY_IP);
#endif
            }

#if (INCLUDE_IPV6 == NU_TRUE)
#if (INCLUDE_STATIC_BUILD == NU_TRUE)
            /* Traverse the flag array to find the used memory location*/
            for (j = 0; j != NSOCKETS; j++)
            {
                /* If this is the memory area being released */
                if (&NET_Sticky_Options_Memory[j] ==
                    IPR_Ports[port_num]->ip_sticky_options)
                {
                    /* Turn the memory flag off */
                    NET_Sticky_Options_Memory_Flags[j] = NU_FALSE;
                }
            }

#else
            /* If there is memory allocated for Sticky Options, deallocate it */
            if (IPR_Ports[port_num]->ip_sticky_options)
            {
                if (NU_Deallocate_Memory((VOID*)IPR_Ports[port_num]->
                                         ip_sticky_options) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory for sticky options",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }
#endif
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /*  Clear this port list entry.  */
            if (NU_Deallocate_Memory((VOID *)IPR_Ports[port_num]) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for port", NERR_SEVERE,
                               __FILE__, __LINE__);
#endif
            /*  Indicate that this port is no longer used. */
            IPR_Ports[port_num] = NU_NULL;
        }
        else
            /*  The port structure has already been deallocated by the stack. */
            return_status = NU_SUCCESS;
    }

#endif /* INCLUDE_IP_RAW == NU_TRUE */

    /* If the socket has not been deallocated, cleanup resources. */
    if (SCK_Sockets[socketd] != NU_NULL)
    {
        /* Resume tasks pending on RX */
        SCK_Resume_All(&SCK_Sockets[socketd]->s_RXTask_List, 0);

        /* Resume tasks pending on TX, remove from buffer suspension
           list */
        SCK_Resume_All(&SCK_Sockets[socketd]->s_TXTask_List, SCK_RES_BUFF);

        /* Resume tasks pending on an accept call. */
        if (SCK_Sockets[socketd]->s_accept_list != NU_NULL)
            SCK_Resume_All(&SCK_Sockets[socketd]->s_accept_list->ssp_task_list, 0);

        SCK_Cleanup_Socket(socketd);
    }

    /* Trace log */
    T_SOCK_STATUS(0, socketd, return_status);

    /* Release the semaphore */
    SCK_Release_Socket();

    /* return to caller */
    return (return_status);

} /* NU_Close_Socket */

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
*       tcp_ska.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Keep-Alive for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_SO_KEEPALIVE
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern TQ_EVENT TCP_Keepalive_Event;

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Setsockopt_SO_KEEPALIVE
*
*   DESCRIPTION
*
*       This function enables or disables TCP Keep-Alive for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables TCP Keep-Alive.
*                               A non-zero value enables TCP Keep-Alive.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                No valid port structure.
*       NU_UNAVAILABLE          TCP Keep-Alive has not been enabled for
*                               the system.
*
*************************************************************************/
STATUS TCP_Setsockopt_SO_KEEPALIVE(INT socketd, UINT8 opt_val)
{
    STATUS              status;

#if (INCLUDE_TCP_KEEPALIVE == NU_TRUE)

    struct sock_struct  *sck_ptr = SCK_Sockets[socketd];
    TCP_PORT            *tcp_prt;

    tcp_prt = TCP_Ports[sck_ptr->s_port_index];

    if (tcp_prt)
    {
        /* Set the TCP Keep-Alive flag */
        if (opt_val)
            tcp_prt->portFlags |= TCP_KEEPALIVE;

        /* Unset the Keep-Alive flag */
        else
        {
            tcp_prt->portFlags &= ~TCP_KEEPALIVE;

            /* If the TCP port is valid, stop the timer */
            TQ_Timerunset(TCP_Keepalive_Event, TQ_CLEAR_ALL_EXTRA,
                          (UNSIGNED)tcp_prt->pindex, 0);
        }

        status = NU_SUCCESS;
    }
    else
        status = NU_INVAL;
#else
    /* Remove compiler warnings */
    UNUSED_PARAMETER(socketd);
    UNUSED_PARAMETER(opt_val);

    status = NU_UNAVAILABLE;

#endif

    return (status);

} /* TCP_Setsockopt_SO_KEEPALIVE */

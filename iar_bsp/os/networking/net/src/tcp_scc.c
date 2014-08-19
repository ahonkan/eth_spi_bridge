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
*       tcp_scc.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Congestion Control for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_CONGESTION_CTRL
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Setsockopt_TCP_CONGESTION_CTRL
*
*   DESCRIPTION
*
*       This function enables or disables TCP Congestion Control for a
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables Congestion Control.
*                               A non-zero value enables Congestion Control.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       No valid port structure
*       NU_INVAL                A connection has already been established.
*       NU_UNAVAILABLE          Congestion Control has not been enabled for
*                               the system.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 opt_val)
{
    STATUS  status;

#if (INCLUDE_CONGESTION_CONTROL == NU_TRUE)

    INT     pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* If a connection has not been established on the socket. */
        if (TCP_Ports[pindex]->state == SREADY)
        {
            /* Disable Congestion Control */
            if (!opt_val)
                TCP_Ports[pindex]->portFlags |= TCP_DIS_CONGESTION;

            /* Enable Congestion Control */
            else
                TCP_Ports[pindex]->portFlags &= ~TCP_DIS_CONGESTION;

            status = NU_SUCCESS;
        }

        else
        {
            status = NU_INVAL;
        }
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

#else
    /* Remove compiler warnings */
    UNUSED_PARAMETER(socketd);
    UNUSED_PARAMETER(opt_val);

    status = NU_UNAVAILABLE;

#endif

    return (status);

} /* TCP_Setsockopt_TCP_CONGESTION_CTRL */

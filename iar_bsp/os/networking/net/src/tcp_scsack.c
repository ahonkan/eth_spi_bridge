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
*       tcp_scsack.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       SACK support for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_CFG_SACK
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
*       TCP_Setsockopt_TCP_CFG_SACK
*
*   DESCRIPTION
*
*       This function enables or disables TCP SACK support for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables SACK.
*                               A non-zero value enables SACK.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                A connection has already been established.
*       NU_INVALID_SOCKET       The socket is invalid.
*       NU_UNAVAILABLE          SACK support has not been enabled for
*                               the system.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_CFG_SACK(INT socketd, UINT8 opt_val)
{
    STATUS              status;

#if (NET_INCLUDE_SACK == NU_TRUE)

    TCP_PORT            *tcp_prt;
    INT                 pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        tcp_prt = TCP_Ports[pindex];

        /* The port must not be connected. */
        if (tcp_prt->state == SREADY)
        {
            /* Enable SACK */
            if (opt_val)
                tcp_prt->portFlags |= TCP_SACK;

            /* Disable SACK */
            else
                tcp_prt->portFlags &= ~TCP_SACK;

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

} /* TCP_Setsockopt_TCP_CFG_SACK */

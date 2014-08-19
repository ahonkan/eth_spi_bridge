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
*       tcp_sts.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Timestamp option support for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_TIMESTAMP
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
*       TCP_Setsockopt_TCP_TIMESTAMP
*
*   DESCRIPTION
*
*       This function enables or disables TCP Timestamp option support
*       for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables the Timestamp
*                               option for the socket.
*                               A non-zero value enables the Timestamp
*                               option for the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       No valid port structure.
*       NU_INVAL                A connection has already been established.
*       NU_UNAVAILABLE          Timestamp Option support has not been
*                               enabled for the system.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_TIMESTAMP(INT socketd, UINT8 opt_val)
{
    STATUS              status = NU_SUCCESS;

#if (NET_INCLUDE_TIMESTAMP == NU_TRUE)

    TCP_PORT            *tcp_prt;
    INT                 pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        tcp_prt = TCP_Ports[pindex];

        /* The socket must not be connected yet. */
        if (tcp_prt->state == SREADY)
        {
            /* Enable the Timestamp Option */
            if (opt_val != 0)
            {
                tcp_prt->portFlags |= TCP_REPORT_TIMESTAMP;
            }

            /* Disable the Timestamp Option */
            else
            {
                tcp_prt->portFlags &= ~TCP_REPORT_TIMESTAMP;
            }
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

} /* TCP_Setsockopt_TCP_TIMESTAMP */

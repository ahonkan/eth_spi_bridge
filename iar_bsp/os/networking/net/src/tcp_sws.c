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
*       tcp_sws.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Window Scale option support for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_WINDOWSCALE
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
*       TCP_Setsockopt_TCP_WINDOWSCALE
*
*   DESCRIPTION
*
*       This function enables or disables TCP Window Scale option support
*       for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of -1 disables the Window Scale
*                               option for the socket.
*                               A positive value enables the Window Scale
*                               option for the socket, and that value is
*                               used as the scale factor for the Window
*                               Scale Option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       No valid port structure.
*       NU_INVAL                A connection has already been established
*                               or the scale factor is not a valid value.
*       NU_UNAVAILABLE          Window Scale Option support has not been
*                               enabled for the system.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_WINDOWSCALE(INT socketd, INT opt_val)
{
    STATUS              status = NU_SUCCESS;

#if (NET_INCLUDE_WINDOWSCALE == NU_TRUE)

    TCP_PORT            *tcp_prt;
    INT                 pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        tcp_prt = TCP_Ports[pindex];

        /* The socket must be unconnected. */
        if (tcp_prt->state == SREADY)
        {
            /* Enable the Window Scale Option */
            if (opt_val >= 0)
            {
                /* Ensure the value is within the valid range and
                 * that shifting the window size right by shift
                 * count bytes will produce a 16-bit value.
                 */
                if ( (opt_val <= TCP_MAX_WINDOWSCALE_FACTOR) &&
                     ((tcp_prt->in.size >> opt_val) <= 65535) )
                {
                    tcp_prt->portFlags |= TCP_REPORT_WINDOWSCALE;

                    /* Set the scale factor value. */
                    tcp_prt->in.p_win_shift = (UINT8)opt_val;
                }

                else
                {
                    status = NU_INVAL;
                }
            }

            /* Disable the Window Scale Option */
            else
            {
                tcp_prt->portFlags &= ~TCP_REPORT_WINDOWSCALE;

                /* Set the scale factor to zero for shift operations on the
                 * send and receive window.
                 */
                tcp_prt->in.p_win_shift = 0;
                tcp_prt->out.p_win_shift = 0;

                /* Set the window size back to the default. */
                if (WINDOW_SIZE <= 65535)
                {
                    tcp_prt->in.size = tcp_prt->credit = WINDOW_SIZE;
                }

                else
                {
                    tcp_prt->in.size = tcp_prt->credit = 65535;
                }
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

} /* TCP_Setsockopt_TCP_WINDOWSCALE */

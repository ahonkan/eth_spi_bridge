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
*       tcp_srws.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the local window
*       size for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_RCV_WINDOWSIZE
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
*       TCP_Setsockopt_TCP_RCV_WINDOWSIZE
*
*   DESCRIPTION
*
*       This function configures the size of the local window size for a
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 The window size to use for the local side
*                               of the connection.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       No valid port structure.
*       NU_INVAL                A connection has already been established,
*                               or opt_val is zero or greater than
*                               TCP_MAX_WINDOW_SIZE, or opt_val is greater
*                               than 65535 and Window Scaling is disabled.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_RCV_WINDOWSIZE(INT socketd, UINT32 opt_val)
{
    STATUS      status = NU_SUCCESS;
    TCP_PORT    *tcp_prt;
    INT         pindex;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        tcp_prt = TCP_Ports[pindex];

        /* The socket must not be connected. */
        if (tcp_prt->state == SREADY)
        {
            /* Ensure the new window size is valid.  It must be greater than
             * zero and less than TCP_MAX_WINDOW_SIZE.  If Window Scaling is
             * disabled for the port, the value must fit into a 16-bit field.
             */
            if ( (opt_val > 0) && (opt_val <= TCP_MAX_WINDOW_SIZE) &&
                 ((tcp_prt->portFlags & TCP_REPORT_WINDOWSCALE) || (opt_val <= 65535)) )
            {
                /* Update the window size and the credit parameter. */
                tcp_prt->in.size = tcp_prt->credit = opt_val;

                /* Compute the shift count for this window size. */
                tcp_prt->in.p_win_shift =
                    TCP_Configure_Shift_Count(tcp_prt->in.size);
            }

            else
            {
                status = NU_INVAL;
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

    return (status);

} /* TCP_Setsockopt_TCP_RCV_WINDOWSIZE */

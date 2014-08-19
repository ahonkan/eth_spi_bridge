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
*       tcp_gka.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine whether TCP
*       Keep-Alive is set on a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_SO_KEEPALIVE
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
*       TCP_Getsockopt_SO_KEEPALIVE
*
*   DESCRIPTION
*
*       This function determines whether TCP Keep-Alive is set on
*       a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the TCP Keep-Alive.
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                Invalid port structure.
*
*************************************************************************/
STATUS TCP_Getsockopt_SO_KEEPALIVE(INT socketd, INT *optval, INT *optlen)
{
    struct sock_struct  *sck_ptr = SCK_Sockets[socketd];
    TCP_PORT            *tcp_prt;
    STATUS              status;

    tcp_prt = TCP_Ports[sck_ptr->s_port_index];

    if (tcp_prt)
    {
        /* Set the TCP Keep-Alive flag */
        if (tcp_prt->portFlags & TCP_KEEPALIVE)
            *optval = 1;

        /* Unset the Keep-Alive flag */
        else
            *optval = 0;

        *optlen = sizeof(UINT8);

        status = NU_SUCCESS;
    }
    else
        status = NU_INVAL;

    return (status);

} /* TCP_Getsockopt_SO_KEEPALIVE */

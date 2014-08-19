/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       tcp_skai.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the TCP Keep-Alive interval
*       (the time interval between Keep-Alive probes) for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_KEEPINTVL
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
*       TCP_Setsockopt_TCP_KEEPINTVL
*
*   DESCRIPTION
*
*       This function sets the time interval between Keep-Alive probes
*       for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       interval                Time interval between keep alive probes.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*       NU_INVAL                The value of the interval parameter is
*                               not a positive integer.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_KEEPINTVL(INT socketd, UINT32 interval)
{
    STATUS  status;
    INT     pindex;

    /* Validate the interval value. */
    if (interval != 0)
    {
        /* Retrieve the port index. */
        pindex = SCK_Sockets[socketd]->s_port_index;

        /* If the TCP port is valid, configure the option. */
        if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
        {
            /* Set the value for the connection. */
            TCP_Ports[pindex]->p_keepalive_intvl = interval;

            status = NU_SUCCESS;
        }

        else
        {
            status = NU_INVALID_SOCKET;
        }
    }

    /* The input is invalid. */
    else
    {
        status = NU_INVAL;
    }

    return (status);

} /* TCP_Setsockopt_TCP_KEEPINTVL */

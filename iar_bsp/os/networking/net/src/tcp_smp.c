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
*       tcp_smp.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the maximum number of
*       Zero Window probes to be transmitted before giving up on window
*       probing on a TCP socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_MAX_PROBES
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
*       TCP_Setsockopt_TCP_MAX_PROBES
*
*   DESCRIPTION
*
*       Configure the maximum number of Zero Window probes to be
*       transmitted before giving up on window probing on a TCP socket
*       at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       max_probes              Maximum number of Zero Window probes to
*                               transmit.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_MAX_PROBES(INT socketd, UINT8 max_probes)
{
    INT     pindex;
    STATUS  status;

    /* Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, configure the option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        /* Set the max probes value for the connection. */
        TCP_Ports[pindex]->p_max_probes = max_probes;

        status = NU_SUCCESS;
    }

    else
    {
        status = NU_INVALID_SOCKET;
    }

    return (status);

} /* TCP_Setsockopt_TCP_MAX_PROBES */

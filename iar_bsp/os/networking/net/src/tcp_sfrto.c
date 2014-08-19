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
*       tcp_sfrto.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the value of the first
*       retransmission timeout on a TCP socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_FIRST_RTO
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
*       TCP_Setsockopt_TCP_FIRST_RTO
*
*   DESCRIPTION
*
*       Configure the value of the first retransmission timeout on a TCP
*       socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       timeout                 Value of the first retransmission timeout.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket does not reference a valid
*                               TCP connection.
*       NU_INVAL                The value of the timeout parameter is not
*                               a positive integer.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_FIRST_RTO(INT socketd, INT32 timeout)
{
    INT     pindex;
    STATUS  status;

    /* Validate the timeout input parameter. */
    if (timeout > 0)
    {
        /* Retrieve the port index. */
        pindex = SCK_Sockets[socketd]->s_port_index;

        /* If the TCP port is valid, configure the option. */
        if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
        {
            /* Set the first probe timeout value for the connection.  The
             * initial RTO value is also set to the first RTO.
             */
            TCP_Ports[pindex]->p_first_rto = TCP_Ports[pindex]->p_rto = timeout;

            status = NU_SUCCESS;
        }

        else
        {
            status = NU_INVALID_SOCKET;
        }
    }

    /* The input parameter is not valid. */
    else
    {
        status = NU_INVAL;
    }

    return (status);

} /* TCP_Setsockopt_TCP_FIRST_RTO */

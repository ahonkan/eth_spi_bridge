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
*       sck_skaw.c
*
*   DESCRIPTION
*
*       This file contains the API routine to configure the amount of time
*       to remain idle on a TCP socket before invoking the TCP keep-alive
*       protocol at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_KEEPALIVE_WAIT
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
*       NU_Setsockopt_TCP_KEEPALIVE_WAIT
*
*   DESCRIPTION
*
*       API routine to configure the amount of time to remain idle on a
*       TCP socket before invoking the TCP keep-alive protocol at
*       run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       delay                   Value in hardware ticks to allow a
*                               connection to remain idle before invoking
*                               the keep-alive protocol.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*       NU_NOT_CONNECTED        The socket passed in has not been created
*                               via a call to NU_Socket.
*       NU_INVAL                The delay parameter is set to zero.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_KEEPALIVE_WAIT(INT socketd, UINT32 delay)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Set the value. */
        status = TCP_Setsockopt_TCP_KEEPALIVE_WAIT(socketd, delay);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_KEEPALIVE_WAIT */

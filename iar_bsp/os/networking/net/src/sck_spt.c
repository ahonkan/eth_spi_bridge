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
*       sck_spt.c
*
*   DESCRIPTION
*
*       This file contains the API routine to configure the maximum
*       timeout value for sending successive Zero Window probes on a TCP
*       socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_PROBE_TIMEOUT
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
*       NU_Setsockopt_TCP_PROBE_TIMEOUT
*
*   DESCRIPTION
*
*       API routine to configure the maximum timeout value for sending
*       successive Zero Window probes on a TCP socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       timeout                 Value in hardware ticks to delay between
*                               successive Zero Window probes on the
*                               specified socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*       NU_NOT_CONNECTED        The socket passed in has not been created
*                               via a call to NU_Socket.
*       NU_INVAL                The value of the timeout parameter is not
*                               a positive integer.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_PROBE_TIMEOUT(INT socketd, INT32 timeout)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Set the value. */
        status = TCP_Setsockopt_TCP_PROBE_TIMEOUT(socketd, timeout);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_PROBE_TIMEOUT */

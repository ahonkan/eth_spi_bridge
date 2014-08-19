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
*       sck_smp.c
*
*   DESCRIPTION
*
*       This file contains the API routine to configure maximum number of
*       Zero Window probes to be transmitted before giving up on window
*       probing on a TCP socket at run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_MAX_PROBES
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
*       NU_Setsockopt_TCP_MAX_PROBES
*
*   DESCRIPTION
*
*       API routine to configure maximum number of Zero Window probes to
*       be transmitted before giving up on window probing on a TCP socket
*       at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       max_probes              The maximum number of unanswered
*                               Zero Window probes to transmit over the
*                               specified socket before closing the
*                               connection.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*       NU_NOT_CONNECTED        The socket passed in has not been created
*                               via a call to NU_Socket.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_MAX_PROBES(INT socketd, UINT8 max_probes)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Set the value. */
        status = TCP_Setsockopt_TCP_MAX_PROBES(socketd, max_probes);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_MAX_PROBES */

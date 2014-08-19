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
*       sck_skar2.c
*
*   DESCRIPTION
*
*       This file contains the API routine to configure the maximum number
*       of unanswered keep-alive probes to transmit on a TCP socket at
*       run-time.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_KEEPALIVE_R2
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
*       NU_Setsockopt_TCP_KEEPALIVE_R2
*
*   DESCRIPTION
*
*       API routine to configure the maximum number of unanswered
*       keep-alive probes to transmit on a TCP socket at run-time.
*
*   INPUTS
*
*       socketd                 Socket for which to configure the value.
*       max_retrans             Maximum number of retransmissions of
*                               unanswered Keep-Alive packets for the
*                               specified socket before the connection is
*                               closed.
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
STATUS NU_Setsockopt_TCP_KEEPALIVE_R2(INT socketd, UINT8 max_retrans)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Set the value. */
        status = TCP_Setsockopt_TCP_KEEPALIVE_R2(socketd, max_retrans);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_KEEPALIVE_R2 */

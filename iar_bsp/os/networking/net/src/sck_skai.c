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
*       sck_skai.c
*
*   DESCRIPTION
*
*       This file contains the API routine to set the TCP Keep-Alive interval
*       (the time interval between Keep-Alive probes) for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_KEEPINTVL
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
*       NU_Setsockopt_TCP_KEEPINTVL
*
*   DESCRIPTION
*
*       API routine to set the TCP Keep-Alive interval (the time
*       interval between Keep-Alive probes) for a socket.
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
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*       NU_NOT_CONNECTED        The socket passed in has not been created
*                               via a call to NU_Socket.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_KEEPINTVL(INT socketd, UINT32 interval)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Set the value. */
        status = TCP_Setsockopt_TCP_KEEPINTVL(socketd, interval);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_KEEPINTVL */

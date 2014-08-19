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
*       sck_ska.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Keep-Alive for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_SO_KEEPALIVE
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
*       NU_Setsockopt_SO_KEEPALIVE
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Keep-Alive for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables TCP Keep-Alive.
*                               A non-zero value enables TCP Keep-Alive.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Invalid parameter.
*       NU_UNAVAILABLE          TCP Keep-Alive has not been enabled for
*                               the system.
*
*************************************************************************/
STATUS NU_Setsockopt_SO_KEEPALIVE(INT socketd, UINT8 opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = TCP_Setsockopt_SO_KEEPALIVE(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_SO_KEEPALIVE */

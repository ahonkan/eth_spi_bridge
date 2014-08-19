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
*       sck_ssb.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the broadcast status
*       of a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_SO_BROADCAST
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
*       NU_Setsockopt_SO_BROADCAST
*
*   DESCRIPTION
*
*       This function sets the broadcast status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  A value of zero disables broadcast for
*                               the socket.  A non-zero value enables
*                               broadcast for the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates the socket option was set
*                               successfully.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*
*************************************************************************/
STATUS NU_Setsockopt_SO_BROADCAST(INT socketd, INT16 optval)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        SOL_Setsockopt_SO_BROADCAST(socketd, optval);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_SO_BROADCAST */

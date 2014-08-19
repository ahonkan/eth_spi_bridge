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
*       sck_srws.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the local window
*       size for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_RCV_WINDOWSIZE
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
*       NU_Setsockopt_TCP_RCV_WINDOWSIZE
*
*   DESCRIPTION
*
*       This function configures the size of the local window size for a
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 The window size to use for the local side
*                               of the connection.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                A connection has already been established or
*                               the value is invalid.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_RCV_WINDOWSIZE(INT socketd, UINT32 opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = TCP_Setsockopt_TCP_RCV_WINDOWSIZE(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_RCV_WINDOWSIZE */

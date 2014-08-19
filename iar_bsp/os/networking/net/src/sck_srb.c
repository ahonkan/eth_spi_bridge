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
*       sck_srb.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the TCP window size on a
*       connection.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_SO_RCVBUF
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
*       NU_Setsockopt_SO_RCVBUF
*
*   DESCRIPTION
*
*       This function sets the TCP window size for the local side of
*       the connection on a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 The new TCP Window size.  This must be
*                               greater than zero.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVALID_PARM         The window size is invalid.
*
*************************************************************************/
STATUS NU_Setsockopt_SO_RCVBUF(INT socketd, INT32 opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = SOL_Setsockopt_SO_RCVBUF(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_SO_RCVBUF */

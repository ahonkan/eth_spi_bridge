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
*       sck_sra.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the reuse address status
*       of a socket.  This option allows multiple addresses to bind
*       to the same port number.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_SO_REUSEADDR
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
*       NU_Setsockopt_SO_REUSEADDR
*
*   DESCRIPTION
*
*       This function sets the reuse address status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  A value of zero disables reuse address
*                               for the socket.  A non-zero value enables
*                               reuse address for the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates the socket option was set
*                               successfully.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVALID_OPTION		This socket option has been disabled at
*       						compile-time.
*
*************************************************************************/
STATUS NU_Setsockopt_SO_REUSEADDR(INT socketd, INT16 optval)
{
    STATUS  status;

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        SOL_Setsockopt_SO_REUSEADDR(socketd, optval);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

#else
    status = NU_INVALID_OPTION;
#endif

    return (status);

} /* NU_Setsockopt_SO_REUSEADDR */

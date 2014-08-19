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
*       sck_sws.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Window Scale Option support for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_WINDOWSCALE
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
*       NU_Setsockopt_TCP_WINDOWSCALE
*
*   DESCRIPTION
*
*       This routine is used to enable or disable TCP Window Scale Option
*       support for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A negative value disables Window Scale
*                               option support on the socket.
*                               A positive value enables the Window Scale
*                               option for the socket, and that value is
*                               used as the scale factor for the Window
*                               Scale Option.  A value of zero indicates
*                               a sale factor of 1; ie, no scaling.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                A connection has already been established or
*                               the scale factor is invalid.
*       NU_UNAVAILABLE          Window Scale option support has not been
*                               enabled for the system.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_WINDOWSCALE(INT socketd, INT opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = TCP_Setsockopt_TCP_WINDOWSCALE(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_WINDOWSCALE */

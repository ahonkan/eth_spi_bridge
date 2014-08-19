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
*       sck_snd.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable the Nagle
*       Algorithm.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_NODELAY
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
*       NU_Setsockopt_TCP_NODELAY
*
*   DESCRIPTION
*
*       This function enables or disables the Nagle Algorithm.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A non-zero value disables the Nagle
*                               Algorithm.  A value of zero enables
*                               the Nagle Algorithm.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Invalid parameter.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_NODELAY(INT socketd, UINT8 opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = TCP_Setsockopt_TCP_NODELAY(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_NODELAY */

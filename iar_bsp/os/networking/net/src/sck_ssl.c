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
*       sck_ssl.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the linger socket
*       option.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_SO_LINGER
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
*       NU_Setsockopt_SO_LINGER
*
*   DESCRIPTION
*
*       This function configures socket linger option.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  Linger socket option value
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates the socket option was set
*                               successfully.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*
*
*************************************************************************/
STATUS NU_Setsockopt_SO_LINGER(INT socketd, struct sck_linger_struct optval)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        SOL_Setsockopt_SO_LINGER(socketd, optval);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_SO_LINGER */

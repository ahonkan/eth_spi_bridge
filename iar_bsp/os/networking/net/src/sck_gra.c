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
*       sck_gra.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the reuse address status
*       of a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_SO_REUSEADDR
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
*       NU_Getsockopt_SO_REUSEADDR
*
*   DESCRIPTION
*
*       This function gets the reuse address status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The reuse address status of the socket.
*       *optlen                 The size of the option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         Optval was not valid.
*       NU_INVALID_SOCKET       The specified socket descriptor is
*                               invalid.
*       NU_INVALID_OPTION		This socket option has been disabled at
*       						compile-time.
*
*************************************************************************/
STATUS NU_Getsockopt_SO_REUSEADDR(INT socketd, INT16 *optval, INT *optlen)
{
    STATUS  status;

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (optval == NU_NULL) || (optlen == NU_NULL) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        SOL_Getsockopt_SO_REUSEADDR(socketd, optval, optlen);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

#else

    status = NU_INVALID_OPTION;

#endif

    return (status);

} /* NU_Getsockopt_SO_REUSEADDR */

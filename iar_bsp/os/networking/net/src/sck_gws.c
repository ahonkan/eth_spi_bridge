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
*       sck_gws.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine whether TCP Window
*       Scale Option support is set for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_TCP_WINDOWSCALE
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
*       NU_Getsockopt_TCP_WINDOWSCALE
*
*   DESCRIPTION
*
*       This function determines whether the TCP Window Scale option support
*       is set for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor.
*       *optval                 The value of TCP Window Scale option support
*                               for the socket.  If disabled, this value
*                               will be negative.  If enabled, the value will
*                               contain the scale factor for the Window
*                               Scale option.  A value of zero indicates a
*                               scale factor of 1; ie, no scaling.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         Optval was not valid.
*       NU_INVALID_SOCKET       The specified socket descriptor is
*                               invalid.
*
*************************************************************************/
STATUS NU_Getsockopt_TCP_WINDOWSCALE(INT socketd, INT *optval)
{
    STATUS  status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (optval == NU_NULL)
    	return (NU_INVALID_PARM);

#endif

	/* Obtain the semaphore and validate the socket */
	status = SCK_Protect_Socket_Block(socketd);

	if (status == NU_SUCCESS)
	{
		status = TCP_Getsockopt_TCP_WINDOWSCALE(socketd, optval);

		/* Release the semaphore */
		SCK_Release_Socket();
	}

    return (status);

} /* NU_Getsockopt_TCP_WINDOWSCALE */

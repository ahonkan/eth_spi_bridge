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
*       sck_gcdsack.c
*
*   DESCRIPTION
*
*       This file contains the routine to determine whether TCP D-SACK
*       support is set for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_TCP_CFG_DSACK
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
*       NU_Getsockopt_TCP_CFG_DSACK
*
*   DESCRIPTION
*
*       This function determines whether the TCP D-SACK support is set for
*       a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor.
*       *optval                 The value of TCP D-SACK support for the
*                               socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         Optval was not valid.
*       NU_INVALID_SOCKET       The specified socket descriptor is
*                               invalid.
*
*************************************************************************/
STATUS NU_Getsockopt_TCP_CFG_DSACK(INT socketd, UINT8 *optval)
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
		status = TCP_Getsockopt_TCP_CFG_DSACK(socketd, optval);

		/* Release the semaphore */
		SCK_Release_Socket();
	}

    return (status);

} /* NU_Getsockopt_TCP_CFG_DSACK */

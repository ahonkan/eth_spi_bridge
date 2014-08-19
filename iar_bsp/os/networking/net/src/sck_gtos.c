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
*       sck_gtos.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the Type of Service
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IP_TOS
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
*       NU_Getsockopt_IP_TOS
*
*   DESCRIPTION
*
*       This function gets the default TOS.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the default TOS.
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_NOT_CONNECTED        The connection is broken for some reason.
*                               Stop using the socket; it is best to close
*                               it.
*       NU_UNAVAILABLE          The semaphore is unavailable.
*
*************************************************************************/
STATUS NU_Getsockopt_IP_TOS(INT socketd, UINT8 *optval, INT *optlen)
{
    STATUS  status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (optval == NU_NULL) || (optlen == NU_NULL) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        IP_Getsockopt_IP_TOS(socketd, optval, optlen);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (NU_SUCCESS);

} /* NU_Getsockopt_IP_TOS */

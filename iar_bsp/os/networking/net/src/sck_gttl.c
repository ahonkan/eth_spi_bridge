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
*       sck_gttl.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the default Time To Live
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IP_TTL
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
*       NU_Getsockopt_IP_TTL
*
*   DESCRIPTION
*
*       This function gets the default TTL.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the default TTL.
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         Optval was not valid.
*       NU_INVALID_SOCKET       The specified socket descriptor is
*                               invalid.
*
*************************************************************************/
STATUS NU_Getsockopt_IP_TTL(INT socketd, UINT16 *optval, INT *optlen)
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
        IP_Getsockopt_IP_TTL(socketd, optval, optlen);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Getsockopt_IP_TTL */

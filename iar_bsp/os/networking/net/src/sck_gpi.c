/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       sck_gpi.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Getsockopt_IP_PKTINFO.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IP_PKTINFO
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
*       NU_Getsockopt_IP_PKTINFO
*
*   DESCRIPTION
*
*       This function retrieves the value of the socket option to
*       return the destination address and interface index of the datagram
*       received via the NU_Recvmsg() call.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the option.
*       *optlen                 The length of the value of the option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVALID_PARM         Either optval or optlen are invalid.
*
*************************************************************************/
STATUS NU_Getsockopt_IP_PKTINFO(INT socketd, INT *optval, INT *optlen)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP_Getsockopt_IP_PKTINFO(socketd, optval, optlen);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Getsockopt_IP_PKTINFO */

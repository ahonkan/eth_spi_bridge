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
*       sck_snc.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable the UDP
*       NOCHECKSUM option
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_UDP_NOCHECKSUM
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
*       NU_Setsockopt_UDP_NOCHECKSUM
*
*   DESCRIPTION
*
*       This function enables or disables the UDP NOCHECKSUM value
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of 0 disables the UDP
*                               NOCHECKSUM option; the checksum will be
*                               computed as usual on the packet.
*                               A value of 1 enables the UDP NOCHECKSUM
*                               option; a zero will be placed in the
*                               checksum field of the UDP header, and
*                               no checksum will be computed on the
*                               packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Invalid parameter.
*
*************************************************************************/
STATUS NU_Setsockopt_UDP_NOCHECKSUM(INT socketd, UINT8 opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        UDP_Setsockopt_UDP_NOCHECKSUM(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_UDP_NOCHECKSUM */

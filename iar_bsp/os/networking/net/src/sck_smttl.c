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
*       sck_smttl.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the multicast Time To
*       Live.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IP_MULTICAST_TTL
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
*       NU_Setsockopt_IP_MULTICAST_TTL
*
*   DESCRIPTION
*
*       This function sets the multicast TTL.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       sck_ttl                 The new Time To Live.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*
*************************************************************************/
STATUS NU_Setsockopt_IP_MULTICAST_TTL(INT socketd, UINT8 sck_ttl)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP_Setsockopt_IP_MULTICAST_TTL(socketd, sck_ttl);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IP_MULTICAST_TTL */

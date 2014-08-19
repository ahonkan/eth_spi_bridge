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
*       sck_smi.c
*
*   DESCRIPTION
*
*       This file contains the routine to change the interface to use
*       for sending multicast packets.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IP_MULTICAST_IF
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
*       NU_Setsockopt_IP_MULTICAST_IF
*
*   DESCRIPTION
*
*       This function sets the interface to use to transmit multicast
*       packets when transmitting multicast packets on the specified
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *dev_address            A pointer to the IP address of the
*                               interface to set.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                One of the parameters is invalid.
*
*************************************************************************/
STATUS NU_Setsockopt_IP_MULTICAST_IF(INT socketd, UINT8 *dev_address)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP_Setsockopt_IP_MULTICAST_IF(socketd, dev_address);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IP_MULTICAST_IF */

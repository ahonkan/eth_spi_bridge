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
*       sck_sbi.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the interface to be used
*       to send broadcast packets.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IP_BROADCAST_IF
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
*       NU_Setsockopt_IP_BROADCAST_IF
*
*   DESCRIPTION
*
*       This function sets the broadcast interface of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *dev_address            A pointer to the IP address of the
*                               interface to set as the broadcast
*                               interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates the socket option was set
*                               successfully.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                The address passed in does not
*                               reference an interface on the node.
*
*************************************************************************/
STATUS NU_Setsockopt_IP_BROADCAST_IF(INT socketd, UINT8 *dev_address)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP_Setsockopt_IP_BROADCAST_IF(socketd, dev_address);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IP_BROADCAST_IF */

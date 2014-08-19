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
*       sck_stos.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the default Type of Service.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IP_TOS
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
*       NU_Setsockopt_IP_TOS
*
*   DESCRIPTION
*
*       This function sets the default TOS.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       sck_tos                 The new default Type of Service.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_UNAVAILABLE          The semaphore is unavailable.
*
*************************************************************************/
STATUS NU_Setsockopt_IP_TOS(INT socketd, UINT8 sck_tos)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP_Setsockopt_IP_TOS(socketd, sck_tos);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IP_TOS */

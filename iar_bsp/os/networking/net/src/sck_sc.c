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
* FILE NAME
*
*       sck_sc.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Socket_Connected.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Socket_Connected
*       SCK_Socket_Connected
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Socket_Connected
*
*   DESCRIPTION
*
*       This function returns NU_TRUE if the specified socket has a
*       foreign connection and NU_FALSE if it does not.
*
*   INPUTS
*
*       socketd                 Socket descriptor
*
*   OUTPUTS
*
*       > 0                     The socket is connected
*       NU_INVALID_SOCKET       The socket descriptor is invalid
*       NU_NOT_CONNECTED        The socket is not connected
*
*************************************************************************/
STATUS NU_Socket_Connected(INT socketd)
{
    STATUS  return_status;

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        return_status = SCK_Socket_Connected(socketd);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (return_status);

} /* NU_Socket_Connected */

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Socket_Connected
*
*   DESCRIPTION
*
*       This function returns NU_TRUE if the specified socket has a
*       foreign connection and NU_FALSE if it does not.
*
*   INPUTS
*
*       socketd                 Socket descriptor
*
*   OUTPUTS
*
*       > 0                     The socket is connected
*       NU_INVALID_SOCKET       The socket descriptor is invalid
*       NU_NOT_CONNECTED        The socket is not connected
*
*************************************************************************/
STATUS SCK_Socket_Connected(INT socketd)
{
    return (SCK_Sockets[socketd]->s_state & SS_ISCONNECTED);

} /* SCK_Socket_Connected */

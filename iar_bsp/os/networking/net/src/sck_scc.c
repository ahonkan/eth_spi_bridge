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
*       sck_scc.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Congestion Control for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_TCP_CONGESTION_CTRL
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
*       NU_Setsockopt_TCP_CONGESTION_CTRL
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable TCP
*       Congestion Control for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables Congestion Control.
*                               A non-zero value enables Congestion Control.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                A connection has already been established.
*       NU_UNAVAILABLE          Congestion Control has not been enabled for
*                               the system.
*
*************************************************************************/
STATUS NU_Setsockopt_TCP_CONGESTION_CTRL(INT socketd, UINT8 opt_val)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = TCP_Setsockopt_TCP_CONGESTION_CTRL(socketd, opt_val);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_TCP_CONGESTION_CTRL */

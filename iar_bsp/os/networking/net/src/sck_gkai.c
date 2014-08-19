/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       sck_gkai.c
*
*   DESCRIPTION
*
*       This file contains the API routine to get the TCP Keep-Alive interval
*       (the time interval between Keep-Alive probes) for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_TCP_KEEPINTVL
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
*       NU_Getsockopt_TCP_KEEPINTVL
*
*   DESCRIPTION
*
*       API routine to get the TCP Keep-Alive interval (the time interval
*       between Keep-Alive probes) for a socket.
*
*   INPUTS
*
*       socketd                 Socket for which to retrieve the value.
*       interval                The pointer to memory in which to store
*                               the time interval between keep alive
*                               probes.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_INVALID_SOCKET       The socket passed in does not reference a
*                               valid TCP socket descriptor.
*       NU_NOT_CONNECTED        The socket passed in has not been created
*                               via a call to NU_Socket.
*       NU_INVALID_PARM         The pointer in which to store the return
*                               value is NULL.
*
*************************************************************************/
STATUS NU_Getsockopt_TCP_KEEPINTVL(INT socketd, UINT32 *interval)
{
    STATUS  status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the interval input parameter. */
    if (interval == NU_NULL)
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Retrieve the value. */
        status = TCP_Getsockopt_TCP_KEEPINTVL(socketd, interval);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Getsockopt_TCP_KEEPINTVL */

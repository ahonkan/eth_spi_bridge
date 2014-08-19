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
*       sck_ps.c
*
*   DESCRIPTION
*
*       This file contains the routines to protect and validate a socket
*       structure.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SCK_Protect_Socket_Block
*       SCK_Release_Socket
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
*       SCK_Protect_Socket_Block
*
*   DESCRIPTION
*
*       This function is responsible for validating a socket and enabling
*       protection on the socket so the caller can manipulate the members
*       of the socket structure.  This routine will always suspend to
*       obtain the semaphore, regardless of whether the socket is a
*       blocking socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*
*   OUTPUTS
*
*       NU_SUCCESS              The socket is valid and in a protected
*                               mode for usage.
*       NU_INVALID_SOCKET       The socket parameter was not a valid socket
*                               value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_NOT_CONNECTED        The socket has been closed.
*       NU_UNAVAILABLE          The semaphore is unavailable.
*
*************************************************************************/
STATUS SCK_Protect_Socket_Block(INT socketd)
{
    STATUS      return_status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /*  Validate the socket number.  */
    if ( (socketd < 0) || (socketd >= NSOCKETS) )
        return (NU_INVALID_SOCKET);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    return_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (return_status == NU_SUCCESS)
    {
        /* If socketd does not reference a valid socket, return an error */
        if (SCK_Sockets[socketd] == NU_NULL)
        {
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            return_status = NU_NOT_CONNECTED;
        }
    }

    if (return_status != NU_SUCCESS)
        NU_USER_MODE();

    return (return_status);

} /* SCK_Protect_Socket_Block */

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Release_Socket
*
*   DESCRIPTION
*
*       This function is responsible for releasing protection on a socket.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SCK_Release_Socket(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

} /* SCK_Release_Socket */

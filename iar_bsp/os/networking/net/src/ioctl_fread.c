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
*       ioctl_fread.c
*
*   DESCRIPTION
*
*       This file contains the routine that returns the number of
*       bytes of data pending on the socket to be read
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       Ioctl_FIONREAD
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
*       Ioctl_FIONREAD
*
*   DESCRIPTION
*
*       This function returns the number of bytes of data pending
*       on the socket to be read
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*       *status                 Pointer to the status
*
*   OUTPUTS
*
*       NU_SUCCESS              The entry was successfully deleted.
*       NU_INVALID_SOCKET       The socket is invalid.
*
*************************************************************************/
STATUS Ioctl_FIONREAD(SCK_IOCTL_OPTION *option)
{
    STATUS  status;

       /* Validate the socket */
    if ( ((*(INT*)option->s_optval >= 0) &&
        (*(INT*)option->s_optval < NSOCKETS)) &&
        (SCK_Sockets[*(INT*)option->s_optval]) )
    {
        /* Get the number of bytes pending to be read by the
         * application layer.
         */
        option->s_ret.sck_bytes_pending =
            SCK_Sockets[*(INT*)option->s_optval]->s_recvbytes;

        status = NU_SUCCESS;
    }
    else
        status = NU_INVALID_SOCKET;

    return (status);

} /* Ioctl_FIONREAD */


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
*       sol_srb.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the TCP window size
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Setsockopt_SO_RCVBUF
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
*       SOL_Setsockopt_SO_RCVBUF
*
*   DESCRIPTION
*
*       This function sets the TCP window size on the socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  The new TCP window size.
*
*   OUTPUTS
*
*       NU_SUCCESS              The TCP window was updated.
*       NU_INVALID_SOCKET       The socket has already been connected
*                               or is in the listening state.
*       NU_INVALID_PARM         The window size is invalid.
*
*************************************************************************/
STATUS SOL_Setsockopt_SO_RCVBUF(INT socketd, INT32 optval)
{
    STATUS  status;

    /* Call the new routine that uses the proper data size and takes
     * care of updating all parameters.
     */
    status = TCP_Setsockopt_TCP_RCV_WINDOWSIZE(socketd, (UINT32)optval);

    return (status);

} /* SOL_Setsockopt_SO_RCVBUF */

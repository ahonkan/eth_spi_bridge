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
*       sol_grb.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the TCP window size
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Getsockopt_SO_RCVBUF
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
*       SOL_Getsockopt_SO_RCVBUF
*
*   DESCRIPTION
*
*       This function gets the TCP window size on the socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor.
*       *opt_val                The local Window Size set on the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              The TCP window was updated.
*       NU_INVALID_PARM         optval is NULL.
*
*************************************************************************/
STATUS SOL_Getsockopt_SO_RCVBUF(INT socketd, INT32 *optval)
{
    STATUS  status;

    /* Use the new routine that operates on the proper data size and
     * fills in the value accordingly.
     */
    status = TCP_Getsockopt_TCP_RCV_WINDOWSIZE(socketd, (UINT32*)optval);

    return (status);

} /* SOL_Getsockopt_SO_RCVBUF */

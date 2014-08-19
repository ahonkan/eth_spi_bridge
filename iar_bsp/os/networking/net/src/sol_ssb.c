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
*       sol_ssb.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the broadcast status
*       of a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Setsockopt_SO_BROADCAST
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
*       SOL_Setsockopt_SO_BROADCAST
*
*   DESCRIPTION
*
*       This function sets the broadcast status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  A value of zero disables broadcast for
*                               the socket.  A non-zero value enables
*                               broadcast for the socket.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SOL_Setsockopt_SO_BROADCAST(INT socketd, INT16 optval)
{
    /* The flag is set when a nonzero value is used and cleared when
     * a 0 is passed in.
     */
    if (optval)
        SCK_Sockets[socketd]->s_options |= SO_BROADCAST_OP;
    else
        SCK_Sockets[socketd]->s_options &= ~SO_BROADCAST_OP;

} /* SOL_Setsockopt_SO_BROADCAST */

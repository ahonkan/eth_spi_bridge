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
*       sol_gsb.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the broadcast status
*       of a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Getsockopt_SO_BROADCAST
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
*       SOL_Getsockopt_SO_BROADCAST
*
*   DESCRIPTION
*
*       This function gets the broadcast status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The broadcast status of the socket.
*       *optlen                 The size of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SOL_Getsockopt_SO_BROADCAST(INT socketd, INT16 *optval, INT *optlen)
{
    /* If the flag is already set then the & will result in a
     * non-zero value, else zero will result.
     */
    *optval =
        (INT16)((SCK_Sockets[socketd]->s_options & SO_BROADCAST_OP) ? (SO_BROADCAST) : 0);

    *optlen = sizeof(SCK_Sockets[socketd]->s_options);

} /* SOL_Getsockopt_SO_BROADCAST */

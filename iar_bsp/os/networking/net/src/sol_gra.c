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
*       sol_gra.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the reuse address status
*       of a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Getsockopt_SO_REUSEADDR
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
*       SOL_Getsockopt_SO_REUSEADDR
*
*   DESCRIPTION
*
*       This function gets the reuse address status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The reuse address status of the socket.
*       *optlen                 The size of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SOL_Getsockopt_SO_REUSEADDR(INT socketd, INT16 *optval, INT *optlen)
{
    /* If the flag is already set then the & will result in a
     * non-zero value, else zero will result.
     */
    *optval = (INT16)(SCK_Sockets[socketd]->s_options & SO_REUSEADDR_OP);
    *optlen = sizeof(SCK_Sockets[socketd]->s_options);

} /* SOL_Getsockopt_SO_REUSEADDR */

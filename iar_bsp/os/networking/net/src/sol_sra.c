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
*       sol_sra.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the reuse address status
*       of the socket.  This option allows multiple addresses to bind
*       to the same port number.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Setsockopt_SO_REUSEADDR
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

INT SCK_ReuseAddr_Set = 0;

/*************************************************************************
*
*   FUNCTION
*
*       SOL_Setsockopt_SO_REUSEADDR
*
*   DESCRIPTION
*
*       This function sets the reuse address status of a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  A value of zero disables reuse address
*                               for the socket.  A non-zero value enables
*                               reuse address for the socket.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SOL_Setsockopt_SO_REUSEADDR(INT socketd, INT16 optval)
{
    /* The flag is set when a nonzero value is used and cleared when
     * a 0 is passed in.
     */
    if (optval)
    {
        SCK_Sockets[socketd]->s_options |= SO_REUSEADDR_OP;
        SCK_ReuseAddr_Set = 1;
    }
    else
        SCK_Sockets[socketd]->s_options &= ~SO_REUSEADDR_OP;

} /* SOL_Setsockopt_SO_REUSEADDR */

#endif

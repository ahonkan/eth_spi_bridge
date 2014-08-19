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
*       sol_gsl.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the linger socket option
*       values for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Getsockopt_SO_LINGER
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
*       SOL_Getsockopt_SO_LINGER
*
*   DESCRIPTION
*
*       This function gets the socket linger option value.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 Where to store the linger option value.
*       *optlen                 The size of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SOL_Getsockopt_SO_LINGER(INT socketd, struct sck_linger_struct *optval,
                              INT *optlen)
{
    /* Retrieve the linger settings from the socket. */
    optval->linger_on    = SCK_Sockets[socketd]->s_linger.linger_on;
    optval->linger_ticks = SCK_Sockets[socketd]->s_linger.linger_ticks;

    /* Update the size of the returning structure. */
    *optlen = sizeof(SCK_Sockets[socketd]->s_linger);

} /* SOL_Getsockopt_SO_LINGER */

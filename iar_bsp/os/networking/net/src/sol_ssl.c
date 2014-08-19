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
*       sol_ssl.c
*
*   DESCRIPTION
*
*       This file contains the routine to configure the socket linger
*       option.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SOL_Setsockopt_SO_LINGER
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
*       SOL_Setsockopt_SO_LINGER
*
*   DESCRIPTION
*
*       This function configures the socket linger option.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       optval                  Pointer to the linger option
*                               settings to be applied to the socket.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SOL_Setsockopt_SO_LINGER(INT socketd, struct sck_linger_struct optval)
{

    /* Assign the linger option */
    SCK_Sockets[socketd]->s_linger.linger_on = optval.linger_on;

    /* Assign the requested linger time */
    SCK_Sockets[socketd]->s_linger.linger_ticks = optval.linger_ticks;

} /* SOL_Setsockopt_SO_LINGER */

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
*       ip_sri.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the receive interface
*       address option.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_RECVIFADDR
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
*       IP_Setsockopt_IP_RECVIFADDR
*
*   DESCRIPTION
*
*       This function sets the receive interface address option.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables.  A non-zero
*                               value enables.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Setsockopt_IP_RECVIFADDR(INT socketd, UINT16 opt_val)
{
    /* The flag is set when a nonzero value is used and cleared when
     * a 0 is passed in.
     */
    if (opt_val)
        SCK_Sockets[socketd]->s_options |= IP_RECVIFADDR_OP;
    else
        SCK_Sockets[socketd]->s_options &= ~IP_RECVIFADDR_OP;

} /* IP_Setsockopt_IP_RECVIFADDR */

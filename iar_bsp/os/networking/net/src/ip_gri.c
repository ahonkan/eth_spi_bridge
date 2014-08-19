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
*       ip_gri.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the receive interface
*       address option.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_RECVIFADDR
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
*       IP_Getsockopt_IP_RECVIFADDR
*
*   DESCRIPTION
*
*       This function gets the receive interface address option.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the value of the option.
*       *optlen                 A pointer to the length of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Getsockopt_IP_RECVIFADDR(INT socketd, INT16 *optval, INT *optlen)
{
    *optval = (INT16)(SCK_Sockets[socketd]->s_options & IP_RECVIFADDR_OP);
    *optlen = sizeof(SCK_Sockets[socketd]->s_options);

} /* IP_Getsockopt_IP_RECVIFADDR */

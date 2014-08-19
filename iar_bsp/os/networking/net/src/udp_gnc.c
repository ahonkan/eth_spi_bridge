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
*       udp_gnc.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the value of the UDP
*       NOCHECKSUM Option
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       UDP_Getsockopt_UDP_NOCHECKSUM
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
*       UDP_Getsockopt_UDP_NOCHECKSUM
*
*   DESCRIPTION
*
*       This function gets the value of the UDP NOCHECKSUM option
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the UDP NOCHECKSUM option
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID UDP_Getsockopt_UDP_NOCHECKSUM(INT socketd, INT16 *optval, INT *optlen)
{
    *optval = (INT16)(SCK_Sockets[socketd]->s_options & SO_UDP_NOCHECKSUM);
    *optlen = sizeof(SCK_Sockets[socketd]->s_options);

} /* UDP_Getsockopt_UDP_NOCHECKSUM */

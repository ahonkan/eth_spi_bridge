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
*       ip_gmttl.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the multicast Time To Live
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_MULTICAST_TTL
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       IP_Getsockopt_IP_MULTICAST_TTL
*
*   DESCRIPTION
*
*       This function gets the multicast TTL.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the multicast TTL.
*       *optlen                 A pointer to the length of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Getsockopt_IP_MULTICAST_TTL(INT socketd, UINT8 *optval, INT *optlen)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];

    *optlen = 1;

    if (sck_ptr->s_moptions_v4)
        *optval = sck_ptr->s_moptions_v4->multio_ttl;
    else
        *optval = IP_DEFAULT_MULTICAST_TTL;

} /* IP_Getsockopt_IP_MULTICAST_TTL */

#endif

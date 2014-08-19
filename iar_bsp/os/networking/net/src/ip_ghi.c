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
*       ip_ghi.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the flag for the IP layer
*       create headers for RAWIP IP packets.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_HDRINCL
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
*       IP_Getsockopt_IP_HDRINCL
*
*   DESCRIPTION
*
*       This function sets the flag for the IP layer to create headers
*       for IPRAW IP packets.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 A pointer to the option value.
*       *optlen                 A pointer to the length of the option.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Getsockopt_IP_HDRINCL(INT socketd, INT16 *optval, INT *optlen)
{
    /* If the flag is already set then the & will result in a
     * non-zero value, else zero will result.
     */
    if (SCK_Sockets[socketd]->s_options)
    {
        *optval = (INT16)(SCK_Sockets[socketd]->s_options & SO_IP_HDRINCL);
        *optlen = sizeof(SCK_Sockets[socketd]->s_options);
    }

} /* IP_Getsockopt_IP_HDRINCL */

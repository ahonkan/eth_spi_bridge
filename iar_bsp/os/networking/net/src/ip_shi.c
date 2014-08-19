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
*       ip_shi.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the flag for the IP layer
*       create headers for RAWIP IP packets.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_HDRINCL
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
*       IP_Setsockopt_IP_HDRINCL
*
*   DESCRIPTION
*
*       This function sets the flag for the IP layer to create headers
*       for IPRAW IP packets.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of non-zero indicates that the IP
*                               layer should not build the IP header.
*                               A zero value indicates that the IP
*                               layer should build the IP header.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Setsockopt_IP_HDRINCL(INT socketd, INT16 opt_val)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];

    /* The flag is set when a nonzero value is used and cleared when
     * a 0 is passed in.
     */
    if (opt_val)
    {
        sck_ptr->s_flags |= IP_RAWOUTPUT;
        sck_ptr->s_options |= SO_IP_HDRINCL;
    }
    else
    {
        sck_ptr->s_flags &= ~IP_RAWOUTPUT;
        sck_ptr->s_options &= ~SO_IP_HDRINCL;
    }

} /* IP_Setsockopt_IP_HDRINCL */

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
*       udp_snc.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable the UDP
*       NOCHECKSUM option.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       UDP_Setsockopt_UDP_NOCHECKSUM
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
*       UDP_Setsockopt_UDP_NOCHECKSUM
*
*   DESCRIPTION
*
*       This function enables or disables the UDP NOCHECKSUM option.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A value of zero disables the UDP
*                               NOCHECKSUM option. A value of one
*                               enables the NOCHECKSUM option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                Invalid socketd parameter.
*
*************************************************************************/
VOID UDP_Setsockopt_UDP_NOCHECKSUM(INT socketd, INT16 opt_val)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];

    if (opt_val)
    {
        sck_ptr->s_options |= SO_UDP_NOCHECKSUM;
    }
    else
    {
        sck_ptr->s_options &= ~SO_UDP_NOCHECKSUM;
    }

} /* UDP_Setsockopt_UDP_NOCHECKSUM */

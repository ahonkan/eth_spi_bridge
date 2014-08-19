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
*   FILENAME
*
*       udp_go.c
*
*   DESCRIPTION
*
*       UDP Get Opt - get socket options routine
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       UDP_Get_Opt
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       UDP_Get_Opt
*
*   DESCRIPTION
*
*       Get UDP options.
*
*   INPUTS
*
*       socketd                 The socket descriptor
*       optname                 The option name
*       *optval                 Pointer to the option value
*       *optlen                 Pointer to the option length
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATUS UDP_Get_Opt (INT socketd, INT optname, VOID *optval, INT *optlen)
{
    STATUS      status = NU_SUCCESS;

    switch (optname)
    {
    case UDP_NOCHECKSUM :

        UDP_Getsockopt_UDP_NOCHECKSUM(socketd, (INT16*)optval, optlen);
        break;

    default :

        status = NU_INVALID_OPTION;
    }

    return (status);

} /* UDP_Get_Opt */

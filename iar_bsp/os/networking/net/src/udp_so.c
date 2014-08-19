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
*       udp_so.c
*
*   DESCRIPTION
*
*       UDP Set Opt - socket options routine
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       UDP_Set_Opt
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
*       UDP_Set_Opt
*
*   DESCRIPTION
*
*       Set UDP options.
*
*   INPUTS
*
*       socketd                 The socket descriptor
*       optname                 The option name
*       *optval                 Pointer to the option value
*       optlen                  The option length
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATUS UDP_Set_Opt(INT socketd, INT optname, const VOID *optval, INT optlen)
{
    STATUS      status = NU_SUCCESS;

    /* No NULL pointers allowed. */
    if (!optval)
        return (NU_INVAL);

    switch (optname)
    {
    case UDP_NOCHECKSUM :

        /* Validate the parameters. */
        if (optlen < (INT)sizeof(INT16))
            status = NU_INVAL;
        else
            UDP_Setsockopt_UDP_NOCHECKSUM(socketd, *(INT16*)optval);

        break;

    default :

        status = NU_INVALID_OPTION;
    }

    return (status);

} /* UDP_Set_Opt */

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
*       tcp_snd.c
*
*   DESCRIPTION
*
*       This file contains the routine to enable or disable the Nagle
*       Algorithm.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Setsockopt_TCP_NODELAY
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
*       TCP_Setsockopt_TCP_NODELAY
*
*   DESCRIPTION
*
*       This function enables or disables the Nagle Algorithm.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       opt_val                 A non-zero value disables the Nagle
*                               Algorithm.  A value of zero enables
*                               the Nagle Algorithm.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                Invalid socketd parameter.
*
*************************************************************************/
STATUS TCP_Setsockopt_TCP_NODELAY(INT socketd, UINT8 opt_val)
{
    INT     pindex;
    STATUS  status;

    /*  Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, then set the NO Delay option. */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        TCP_Ports[pindex]->out.push = opt_val;

        status = NU_SUCCESS;
    }
    else
        status = NU_INVAL;

    return (status);

} /* TCP_Setsockopt_TCP_NODELAY */

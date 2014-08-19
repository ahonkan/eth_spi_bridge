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
*       tcp_gnd.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the value of the Nagle
*       Algorithm.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TCP_Getsockopt_TCP_NODELAY
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
*       TCP_Getsockopt_TCP_NODELAY
*
*   DESCRIPTION
*
*       This function gets the value of the Nagle Algorithm.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the Nagle Algorithm.
*       *optlen                 The length of the option.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                Invalid socketd parameter.
*
*************************************************************************/
STATUS TCP_Getsockopt_TCP_NODELAY(INT socketd, INT *optval, INT *optlen)
{
    INT     pindex;
    STATUS  status;

    /*  Retrieve the port index. */
    pindex = SCK_Sockets[socketd]->s_port_index;

    /* If the TCP port is valid, then return the value of the NO Delay
     * option.
     */
    if ( (pindex != NU_IGNORE_VALUE) && (TCP_Ports[pindex] != NU_NULL) )
    {
        *optval = TCP_Ports[pindex]->out.push;
        *optlen = sizeof(INT);
        status = NU_SUCCESS;
    }
    else
        status = NU_INVAL;

    return (status);

} /* TCP_Getsockopt_TCP_NODELAY */

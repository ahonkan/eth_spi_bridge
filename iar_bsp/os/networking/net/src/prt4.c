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
*       prt4.c
*
*   DESCRIPTION
*
*       Common IPv4 port handling routines.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PRT4_Find_Matching_Port
*
*   DEPENDENCIES
*
*       nu_net.h
*       tcp4.h
*       udp4.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#include "networking/tcp4.h"
#include "networking/udp4.h"

/*************************************************************************
*
*   FUNCTION
*
*       PRT4_Find_Matching_Port
*
*   DESCRIPTION
*
*       This function returns the port associated with the parameters
*       provided.
*
*   INPUTS
*
*       protocol                The protocol of the port to find.
*       source_ip               The Source Address of the target entry.
*       dest_ip                 The Destination Address of the target
*                               entry.
*       source_port             The source port of the target entry.
*       dest_port               The destination port of the target
*                               entry.
*
*   OUTPUTS
*
*       The index of the corresponding port.
*       -1 if no port found.
*
*************************************************************************/
INT32 PRT4_Find_Matching_Port(INT protocol, UINT32 source_ip, UINT32 dest_ip,
                              UINT16 source_port, UINT16 dest_port)
{
    INT32  status;

#if ((INCLUDE_UDP != NU_TRUE) && (INCLUDE_TCP != NU_TRUE))
    /* Remove compiler warnings */
    UNUSED_PARAMETER(source_ip);
    UNUSED_PARAMETER(dest_ip);
    UNUSED_PARAMETER(source_port);
    UNUSED_PARAMETER(dest_port);
#endif

    switch (protocol)
    {
#if (INCLUDE_UDP == NU_TRUE)
    case NU_PROTO_UDP:

        status = UDP4_Find_Matching_UDP_Port(source_ip, dest_ip, source_port,
                                             dest_port);
        break;
#endif

#if (INCLUDE_TCP == NU_TRUE)
    case NU_PROTO_TCP:

        status = TCP4_Find_Matching_TCP_Port(source_ip, dest_ip,
                                             source_port, dest_port);
        break;
#endif

    default:

        status = -1;
        break;
    }

    return (status);

} /* PRT4_Find_Matching_Port */

#endif

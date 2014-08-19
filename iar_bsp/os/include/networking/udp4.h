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

/***************************************************************************
*
*   FILE NAME
*
*       udp4.h
*
*   DESCRIPTION
*
*       This file contains the structure definitions required by the UDP
*       module of Nucleus NET for IPv4 packets.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       udp.h
*
***************************************************************************/

#ifndef UDP4_H
#define UDP4_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/udp.h"

INT16   UDP4_Input(IPLAYER *pkt, NET_BUFFER *, struct pseudotcp *);
STATUS  UDP4_Cache_Route (UDP_PORT *, UINT32);
INT32   UDP4_Find_Matching_UDP_Port(UINT32, UINT32, UINT16, UINT16);

#if (INCLUDE_IPV4 == NU_TRUE)
VOID    UDP4_Free_Cached_Route(RTAB4_ROUTE_ENTRY *);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

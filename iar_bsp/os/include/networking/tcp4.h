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
*       tcp4.h
*
*   DESCRIPTION
*
*       This file contains the structure definitions required by the TCP
*       module of Nucleus NET for IPv4 packets.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       tcp.h
*
***************************************************************************/

#ifndef TCP4_H
#define TCP4_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/tcp.h"

INT16   TCP4_Input(NET_BUFFER *, struct pseudotcp *);
INT32   TCP4_Find_Matching_TCP_Port(UINT32, UINT32, UINT16, UINT16);

#if (INCLUDE_IPV4 == NU_TRUE)
VOID    TCP4_Free_Cached_Route(RTAB4_ROUTE_ENTRY *);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

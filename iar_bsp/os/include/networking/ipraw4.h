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
*       ipraw4.h
*
*   DESCRIPTION
*
*       This file contains the structure definitions required by the
*       IPRAW module of Nucleus NET.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       ipraw.h
*
*************************************************************************/

#ifndef IPRAW4_H
#define IPRAW4_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/ipraw.h"

/* IP Raw prototypes. */
STATUS  IPRaw4_Interpret(IPLAYER *, NET_BUFFER *);
STATUS  IPRaw4_Cache_Route (struct iport *, UINT32);

#if (INCLUDE_IPV4 == NU_TRUE)
VOID    IPRaw4_Free_Cached_Route(RTAB4_ROUTE_ENTRY *);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* IPRAW4_H */

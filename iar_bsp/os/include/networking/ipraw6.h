/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       ipraw6.h                                     
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

/* Portions of this program were written by: */
/*************************************************************************
*                                                                         
* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 and 2001 WIDE Project.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this 
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS 
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*                                                                         
*************************************************************************/

#ifndef IPRAW6_H
#define IPRAW6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/ipraw.h"

/* IP Raw prototypes. */
STATUS  IPRaw6_Interpret(IP6LAYER *, NET_BUFFER *, struct pseudohdr *, UINT8);
STATUS  IPRaw6_Cache_Route (struct iport *, const UINT8 *);
VOID    IPRaw6_Free_Cached_Route(RTAB6_ROUTE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IPRAW6_H */

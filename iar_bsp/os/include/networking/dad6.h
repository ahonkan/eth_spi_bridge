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

/*************************************************************************
*
*   FILE NAME
*
*       dad6.h
*
*   DESCRIPTION
*
*       This file contains the data structures and defines necessary
*       to support DAD.
*
*   DATA STRUCTURES
*
*       DADQ
*       DAD_LIST
*
*   DEPENDENCIES
*
*       target.h
*
*************************************************************************/

#ifndef DAD6_H
#define DAD6_H

#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if (INCLUDE_DAD6 == NU_TRUE)

typedef struct dadq
{
    struct dadq             *dadq_next;
    struct dadq             *dadq_previous;
    UINT32                  dad_addr_id;
    int                     dad_count;      /* max NS to send */
    int                     dad_ns_tcount;  /* # of trials to send NS */
    int                     dad_ns_ocount;  /* NS sent so far */
    int                     dad_ns_icount;
    int                     dad_na_icount;
} DADQ;

typedef struct _DAD_LIST
{
    DADQ   *dad_head;
    DADQ   *dad_tail;
} DAD_LIST;

VOID nd6_dad_initialize(VOID);
void nd6_dad_na_input(DEV6_IF_ADDRESS *ifa);
void nd6_dad_ns_input(DEV6_IF_ADDRESS *ifa);
void nd6_dad_duplicated(DEV6_IF_ADDRESS *ifa);
void nd6_dad_start(DEV6_IF_ADDRESS *ifa, UINT8 retry, UINT32 *tick);
VOID nd6_dad_event(TQ_EVENT, UNSIGNED, UNSIGNED);
void nd6_dad_timer(DEV6_IF_ADDRESS *ifa, DADQ *dp);
void nd6_dad_stop(const DEV6_IF_ADDRESS *ifa);

#else

#define nd6_dad_ns_input(value)
#define nd6_dad_na_input(value)
#define nd6_dad_event(value)

#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

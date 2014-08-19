/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*       if.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contains definitions for sockets local interfaces.
*
* DATA STRUCTURES
*
*       None.
*
* DEPENDENCIES
*
*       socket.h
*
************************************************************************/
#ifndef _NET_IF_H
#define _NET_IF_H

/* The data structure is implemented by Nucleus NET  */
#include "services/socket.h"

/* Interface name length.
   The length of a buffer containing an interface name
   (including the terminating NULL character) */

#define IF_NAMESIZE     DEV_NAME_LENGTH

#ifdef __cplusplus
extern "C" {
#endif

unsigned                if_nametoindex(const char *);
char                    *if_indextoname(unsigned, char *);
struct if_nameindex     *if_nameindex(void);
void                    if_freenameindex(struct if_nameindex *);

#ifdef __cplusplus
}
#endif

#endif /* _NET_IF_H */


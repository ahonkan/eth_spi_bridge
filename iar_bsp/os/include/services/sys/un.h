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
*       un.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contains definition for UNIX domain sockets.
*
* DATA STRUCTURES
*
*       sockaddr_un
*
* DEPENDENCIES
*
*       socket.h
*
************************************************************************/
#ifndef _SYS_UN_H
#define _SYS_UN_H

#include "services/sys/socket.h"

/* Structure used for socket address and length (128bits = 16 bytes) */
struct sockaddr_un
{
    sa_family_t  sun_family;                /* Address family              */
    char         sun_path[14];              /* Socket pathname             */
};

#endif /* _SYS_UN_H */

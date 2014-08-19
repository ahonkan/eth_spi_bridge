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
*       inet.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contains definitions for Internet operations.
*
* DATA STRUCTURES
*
*       none
*
* DEPENDENCIES
*
*       in.h
*
************************************************************************/
#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include "services/inet/in.h"

/* X can be h/n or host/network byte order */
#define htons PNET_X_to_Xs
#define ntohs PNET_X_to_Xs
#define htonl PNET_X_to_Xl
#define ntohl PNET_X_to_Xl

#ifdef __cplusplus
extern "C" {
#endif

in_addr_t    inet_addr(const char *);
char        *inet_ntoa(struct in_addr);
const char  *inet_ntop(int, const void *, char *, socklen_t);
int          inet_pton(int, const char *, void *);
uint32_t     PNET_X_to_Xl(uint32_t number);
uint16_t     PNET_X_to_Xs(uint16_t number);

#ifdef __cplusplus
}
#endif

#endif /* _ARPA_INET_H */

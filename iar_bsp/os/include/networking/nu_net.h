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
*       nu_net.h
*
*   COMPONENT
*
*       Net - Application include file.
*
*   DESCRIPTION
*
*       This file includes all required header files to allow
*       access to the Nucleus NET API. This includes the sockets,
*       RIP2 API, and all related data structures.
*
*   DATA STRUCTURES
*
*       None
*
*   FILE DEPENDENCIES
*
*       target.h
*       ncl.h
*       externs.h
*       socketd.h
*       tcpdefs.h
*       rip2.h
*       dhcp.h
*       bootp.h
*       dns.h
*       ip.h
*       pcdisk.h
*       zc_defs.h
*       gre.h
*
*************************************************************************/

#ifndef NU_NET_H
#define NU_NET_H

#include "networking/target.h"
#include "networking/ncl.h"
#include "networking/externs.h"
#include "networking/socketd.h"
#include "networking/tcpdefs.h"
#include "networking/dns_sd.h"
#include "networking/rip2.h"
#include "networking/dhcp.h"
#include "networking/bootp.h"
#include "networking/dns.h"
#include "networking/ip.h"
#include "networking/zc_defs.h"
#include "networking/gre.h"
#include "networking/cmsg_defs.h"
#include "networking/net_dbg.h"
#include "networking/igmp.h"
#include "networking/test_extr.h"
#if (INCLUDE_MDNS == NU_TRUE)
#include "networking/mdns.h"
#endif

#endif /* NU_NET_H */

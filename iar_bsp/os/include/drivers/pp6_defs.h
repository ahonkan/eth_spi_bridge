/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       pp6_defs.h
*
*   COMPONENT
*
*       IPV6 - IPv6 support for PPP
*
*   DESCRIPTION
*
*       Includes header files from Nucleus Netv6 for IPv6
*       support in PPP.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nc6.h
*       defrtr6.h
*       socketd6.h
*       ip6.h
*       prefix6.h
*
*************************************************************************/
#ifndef PPP_INC_PP6_DEFS_H
#define PPP_INC_PP6_DEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nc6.h"
#include "networking/defrtr6.h"

#include "networking/socketd6.h"
#include "networking/ip6.h"
#include "networking/prefix6.h"
#endif

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PP6_DEFS_H */

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
*       pp6_extr.h
*
*   COMPONENT
*
*       IPV6 - IPv6 support for PPP
*
*   DESCRIPTION
*
*       This file contains function prototypes used by the IPv6
*       component of Nucleus PPP and which are also accessible to
*       other modules.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       pp6_defs.h
*
*************************************************************************/
#ifndef PPP_INC_PP6_EXTR_H
#define PPP_INC_PP6_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif                                      /* __cplusplus */

#include "drivers/pp6_defs.h"

#if (INCLUDE_IPV6 == NU_TRUE)
STATUS PPP6_Init_IPv6_Device(DV_DEVICE_ENTRY*);
STATUS PPP6_Attach_IP_Address(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP6_Detach_IP_Address(DV_DEVICE_ENTRY *dev_ptr);
STATUS PPP6_Generate_Addr(UINT8 type, IPV6CP_OPTIONS *ipv6cp);


IP6_NEIGHBOR_CACHE_ENTRY *NC6PPP_Add_NeighCache_Entry(DV_DEVICE_ENTRY*,
                          UINT8*, const UINT8*, UINT32, NET_BUFFER*,
                          UINT8);

IP6_NEIGHBOR_CACHE_ENTRY *NC6PPP_Find_NeighCache_Entry(
                          const DV_DEVICE_ENTRY*, const UINT8*);

IP6_NEIGHBOR_CACHE_ENTRY *NC6PPP_Retrieve_NeighCache_Entry(
                          const DV_DEVICE_ENTRY*, const UINT8*);

STATUS NC6PPP_Add_RemotePPP_Entry(DV_DEVICE_ENTRY*, UINT8*);
STATUS NC6PPP_Delete_NeighCache_Entry(DV_DEVICE_ENTRY*, UINT8*);
VOID   NC6PPP_CleanUp_Entry(IP6_NEIGHBOR_CACHE_ENTRY*);
#endif

#ifdef          __cplusplus
}
#endif                                      /* __cplusplus */

#endif                                      /* PPP_INC_PP6_EXTR_H */

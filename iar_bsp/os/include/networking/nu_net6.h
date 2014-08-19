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
*       nu_net6.h                                    
*                                                                     
*   COMPONENT                                                   
*                                                                       
*       Net - IPv6 Application include file.
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This file includes all required header files to allow 
*       access to the Nucleus NET API for IPv6.
*                                                                       
*   DATA STRUCTURES
*
*       None
*
*   FILE DEPENDENCIES
*
*       None
*                                                                       
*************************************************************************/

#ifndef NU_NET6_H
#define NU_NET6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/net6_cfg.h"
#include "networking/ip6.h"
#include "networking/externs6.h"
#include "networking/ripng.h"
#include "networking/cfg6.h"
#include "networking/6to4.h"
#include "networking/prefix6.h"
#include "networking/dhcp6.h"

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_NET6_H */

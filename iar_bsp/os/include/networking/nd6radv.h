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
*       nd6radv.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for processing and transmitting Router 
*       Advertisements.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       None   
*
*   DEPENDENCIES                                                             
*                                                                          
*       nd6opts.h
*                                                                          
*************************************************************************/

#ifndef ND6RADV_H
#define ND6RADV_H

#include "networking/nd6opts.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define ND6RADV_SOL_ADV     0    
#define ND6RADV_UNSOL_ADV   1

STATUS ND6RADV_Input(IP6LAYER *, DV_DEVICE_ENTRY *, const NET_BUFFER *);
STATUS ND6RADV_Process_Options(DV_DEVICE_ENTRY *, UINT8 *, union nd_opts *, 
                               IP6_DEFAULT_ROUTER_ENTRY *, const NET_BUFFER *);
STATUS ND6RADV_Process_Prefix_Option(DV_DEVICE_ENTRY *, const UINT8 *, UINT8, 
                                     UINT32, UINT32, UINT8, const NET_BUFFER *);
VOID   ND6RADV_Process_Prefixes(DV_DEVICE_ENTRY *, const union nd_opts *, 
                                IP6_DEFAULT_ROUTER_ENTRY *, const NET_BUFFER *);
STATUS ND6RADV_Process_MTU_Option(DV_DEVICE_ENTRY *, UINT32);
VOID   ND6RADV_Output(DV_DEVICE_ENTRY *);
VOID   ND6RADV_Handle_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID   ND6RADV_Router_Input(const DV_DEVICE_ENTRY *, const NET_BUFFER *, 
                            const union nd_opts *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

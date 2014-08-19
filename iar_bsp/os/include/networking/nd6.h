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
*       nd6.h                                        
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for Neighbor Discovery.
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

#ifndef ND6_H
#define ND6_H

#include "networking/nd6opts.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT             ND6_Validate_Message(UINT8, UINT8 *, const UINT8 *, 
                                     const UINT8 *, const IP6LAYER *, UINT32, 
                                     UINT32, union nd_opts *, const NET_BUFFER *);
VOID            ND6_Build_Link_Layer_Opt(UINT8, const UINT8 *, 
                                         const NET_BUFFER *, UINT8);
DEV6_IF_ADDRESS *ND6_Find_Target_Address(DV_DEVICE_ENTRY *, UINT8 *);
STATUS          ND6_Process_Link_Layer_Option(UINT8 *, DV_DEVICE_ENTRY *, 
                                              const UINT8 *, UINT32);
VOID            ND6_Transmit_Queued_Data(IP6_NEIGHBOR_CACHE_ENTRY *, 
                                         DV_DEVICE_ENTRY*);
UINT32          ND6_Compute_Random_Timeout(const UINT32, const UINT32);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

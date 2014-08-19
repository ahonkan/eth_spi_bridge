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
*       nud6.h                                       
*                                                                                  
*   DESCRIPTION                                                              
*           
*       This file contains macros, data structures and function 
*       declarations for the Neighbor Unreachability Detection Module.                                                               
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       None
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef NUD6_H
#define NUD6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

VOID    NUD6_Check_Neighbors(UINT32);
VOID    NUD6_Probe_Node(UINT32, UINT32);
VOID    NUD6_Confirm_Reachability(IP6_NEIGHBOR_CACHE_ENTRY *);
VOID    NUD6_Confirm_Reachability_By_IP_Addr(const UINT8 *);
VOID    NUD6_Stale_Neighbor(IP6_NEIGHBOR_CACHE_ENTRY *);
VOID    NUD6_Init(const DV_DEVICE_ENTRY *);
VOID    NUD6_Handle_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID    NUD6_Stop_Probing(IP6_NEIGHBOR_CACHE_ENTRY *, UINT32);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

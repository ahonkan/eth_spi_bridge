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
*       nc6_eth.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the ethernet IPv6 Neighbor Cache.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       _ip6_eth_neighbor_cache_entry   
*
*   DEPENDENCIES                                                             
*                                                                          
*       rtab6.h
*                                                                          
*************************************************************************/

#ifndef NC6_ETH_H
#define NC6_ETH_H

#include "networking/rtab6.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Implement the Neighbor Cache as an array on a per-interface basis */
struct _ip6_eth_neighbor_cache_entry
{
    UINT8   ip6_neigh_cache_hw_addr[DADDLEN];
    UINT8   padN[2];
};

IP6_NEIGHBOR_CACHE_ENTRY    *NC6ETH_Add_NeighCache_Entry(DV_DEVICE_ENTRY *, UINT8 *, 
                                                         const UINT8 *, UINT32, 
                                                         NET_BUFFER *, UINT8);
STATUS                      NC6ETH_Delete_NeighCache_Entry(DV_DEVICE_ENTRY *, UINT8 *);
IP6_NEIGHBOR_CACHE_ENTRY    *NC6ETH_Find_NeighCache_Entry(const DV_DEVICE_ENTRY *, 
                                                          const UINT8 *);
INT                         NC6ETH_Link_Addrs_Equal(const UINT8 *, 
                                                    const IP6_NEIGHBOR_CACHE_ENTRY *);
VOID                        NC6ETH_Update_NeighCache_Link_Addr(const IP6_NEIGHBOR_CACHE_ENTRY *,
                                                               const UINT8 *);
VOID                        NC6ETH_CleanUp_Entry(IP6_NEIGHBOR_CACHE_ENTRY *);
IP6_NEIGHBOR_CACHE_ENTRY    *NC6ETH_Retrieve_NeighCache_Entry(const DV_DEVICE_ENTRY *, 
                                                              const UINT8 *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

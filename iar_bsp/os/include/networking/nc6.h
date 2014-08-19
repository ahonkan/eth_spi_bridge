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
*       nc6.h                                        
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the generic IPv6 Neighbor Cache.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       _ip6_neighbor_cache_entry   
*       _ip6_next_hop_list_entry                                                      
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef NC6_H
#define NC6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Only one entry is needed in the Neighbor Cache for the Loopback Device */
#define IP6_LDC_NEIGHBOR_CACHE_ENTRIES  1

/* The length of the interface ID for a loopback device - this will always 
 * be 0, because the loopback has no interface ID.
 */
#define IP6_LDC_INTERFACE_ID_LENGTH     0

/* Neighbor Cache Entry States */
#define NC_NEIGH_INCOMPLETE     1
#define NC_NEIGH_REACHABLE      2
#define NC_NEIGH_STALE          3
#define NC_NEIGH_DELAY          4
#define NC_NEIGH_PROBE          5

/* Neighbor Cache Entry Flags */
#define NC_UP                   0x1    /* Is this entry valid. */
#define NC_PERMANENT            0x2    /* Is this entry permanent. */
#define NC_ISROUTER             0x4

/* Implement the Neighbor Cache as an array on a per-interface basis */
struct _ip6_neighbor_cache_entry
{
    UINT8                       ip6_neigh_cache_ip_addr[IP6_ADDR_LEN];
    UINT8                       ip6_neigh_cache_state;
    UINT8                       ip6_neigh_cache_unans_probes;
    UINT8                       ip6_neigh_cache_qpkts_count;
    UINT8                       ip6_neigh_cache_rsend_count;
    UINT32                      ip6_neigh_cache_nud_time;
    UINT32                      ip6_neigh_cache_timestamp;
    UINT32                      ip6_neigh_cache_resolve_id;
    UINT32                      ip6_neigh_cache_probe_index;
    UINT32                      ip6_neigh_cache_flags;
    NET_BUFFER_HEADER           ip6_neigh_cache_packet_list;
    DV_DEVICE_ENTRY             *ip6_neigh_cache_device;
    IP6_DEST_LIST               ip6_neigh_cache_dest_list;
    IP6_DEFAULT_ROUTER_ENTRY    *ip6_neigh_cache_def_rtr;
    VOID                        *ip6_neigh_cache_link_spec;
};

struct _ip6_next_hop_list_entry
{
    IP6_NEXT_HOP_LIST_ENTRY     *ip6_next_hop_list_entry_next;
    IP6_NEXT_HOP_LIST_ENTRY     *ip6_next_hop_list_entry_previous;
    IP6_NEIGHBOR_CACHE_ENTRY    *ip6_next_hop_list_entry;
    UINT32                      ip6_next_hop_list_entry_flags;
};

VOID                    NC6_Transition_To_Router(IP6_NEIGHBOR_CACHE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

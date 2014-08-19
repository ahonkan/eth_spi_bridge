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
*   FILENAME                                                                     
*                                                                                  
*       rtab6.h                                      
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the IPv6 Routing Table.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       rtab6_route_entry
*       _ip6_dest_list_entry  
*       _ip6_dest_list 
*       _rtab6_rt_ip_dest
*       _RTAB6_ROUTE
*
*   DEPENDENCIES                                                             
*                                                                          
*       socketd6.h
*                                                                          
*************************************************************************/

#ifndef RTAB6_H
#define RTAB6_H

#include "networking/socketd6.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

struct rtab6_route_entry
{
    struct rtab6_route_entry    *rt_entry_next;
    struct rtab6_route_entry    *rt_entry_prev;
    struct route_entry_parms    rt_entry_parms;
    struct route_node           *rt_route_node; /* ROUTE_NODE to which the entry
                                                 * belongs. */
    IP6_NEIGHBOR_CACHE_ENTRY    *rt_next_hop_entry;
    SCK6_SOCKADDR_IP            rt_next_hop;
};

typedef struct rtab6_route_entry    RTAB6_ROUTE_ENTRY;

struct _ip6_dest_list_entry
{
    IP6_DEST_LIST_ENTRY         *ip6_dest_list_entry_next;
    IP6_DEST_LIST_ENTRY         *ip6_dest_list_entry_previous;
    RTAB6_ROUTE_ENTRY           *ip6_dest_list_entry;
};

struct _ip6_dest_list
{
    IP6_DEST_LIST_ENTRY     *dv_head;
    IP6_DEST_LIST_ENTRY     *dv_tail;
};

typedef struct _rtab6_rt_ip_dest
{
    SCK6_SOCKADDR_IP        rtab6_rt_ip_dest;
    DV_DEVICE_ENTRY         *rtab6_rt_device;
} RTAB6_RT_IP_DEST;

struct _RTAB6_ROUTE
{
    RTAB6_ROUTE_ENTRY   *rt_route;
    RTAB6_RT_IP_DEST    rt_ip_dest;
};

VOID                    RTAB6_Init(VOID);
VOID                    RTAB6_Add_DestList_Entry(IP6_DEST_LIST *, 
                                                 RTAB6_ROUTE_ENTRY *);
VOID                    RTAB6_Delete_DestList_Entry(IP6_DEST_LIST *,
                                                    const RTAB6_ROUTE_ENTRY *);
IP6_DEST_LIST_ENTRY     *RTAB6_Find_DestList_Entry(const IP6_DEST_LIST *, 
                                                   const RTAB6_ROUTE_ENTRY *);
STATUS                  RTAB6_Set_Default_Route(DV_DEVICE_ENTRY *, UINT8 *, UINT32);
RTAB6_ROUTE_ENTRY       *RTAB6_Find_Next_Hop_Entry(const RTAB6_ROUTE_ENTRY *, 
                                                   const IP6_NEIGHBOR_CACHE_ENTRY *);
VOID                    RTAB6_Delete_Routes_For_Device(const DV_DEVICE_ENTRY *);
STATUS                  RTAB6_Add_Route(DV_DEVICE_ENTRY *, const UINT8 *, 
                                        const UINT8 *, UINT8, UINT32);
ROUTE_NODE              *RTAB6_Setup_New_Node(const ROUTE_NODE *);
INT                     RTAB6_Insert_Route_Entry(ROUTE_NODE *, const ROUTE_NODE *);
STATUS                  RTAB6_Delete_Route(const UINT8 *, const UINT8 *);
STATUS                  RTAB6_Delete_Node(ROUTE_NODE *);
VOID                    RTAB6_Unlink_Next_Hop(const RTAB6_ROUTE_ENTRY *);
RTAB6_ROUTE_ENTRY       *RTAB6_Find_Route(const UINT8 *, INT32);
ROUTE_ENTRY             *RTAB6_Find_Next_Route_Entry(ROUTE_ENTRY *current_route);
STATUS                  RTAB6_Delete_Route_From_Node(ROUTE_NODE *, const UINT8 *);
UINT8                   RTAB6_Determine_Matching_Prefix(const UINT8 *, const UINT8 *, 
                                                        const UINT8 *, UINT8);
ROUTE_ENTRY             *RTAB6_Find_Route_By_Gateway(const UINT8 *, const UINT8 *, INT32);
ROUTE_ENTRY             *RTAB6_Find_Route_By_Device(UINT8 *, DV_DEVICE_ENTRY *);
RTAB6_ROUTE_ENTRY       *RTAB6_Next_Hop_Determination(UINT8 *ip_addr);
RTAB6_ROUTE_ENTRY       *RTAB6_Find_Next_Route(const UINT8 *target_address);
ROUTE_NODE              *RTAB6_Get_Default_Route(VOID);
STATUS                  RTAB6_Update_Route(const UINT8 *, const UINT8 *, 
                                           const UPDATED_ROUTE_NODE *);
VOID                    RTAB6_Delete_Route_By_Gateway(UINT8 *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

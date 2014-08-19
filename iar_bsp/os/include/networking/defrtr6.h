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
*       defrtr6.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                       
*       This file contains the data structures and defines necessary
*       to support the Default Router component of IPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*           
*       _ip6_default_router_entry                                                               
*       _ip6_default_router_list
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef DEFRTR_H
#define DEFRTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

typedef struct _ip6_default_router_addr
{
    UINT8       ip6_def_rtr_ip_addr[16];
    UINT8       ip6_def_rtr_prfx_len;   
    UINT8       padN[3];
    UINT32      ip6_def_rtr_valid_lifetime;
    UINT32      ip6_def_rtr_pref_lifetime;
} IP6_DEFAULT_ROUTER_ADDR;

struct _ip6_default_router_entry
{
    struct _ip6_default_router_entry *ip6_def_rtr_next;
    struct _ip6_default_router_entry *ip6_def_rtr_previous;
    UINT8                            ip6_def_rtr_ip_addr[16];
    UINT8                            ip6_def_rtr_use_next;
    UINT8                            ip6_def_rtr_adv_int_exp;
    UINT8                            padN[2];
    UINT32                           ip6_def_rtr_inval_timer;
    UINT32                           ip6_def_rtr_index;
    IP6_NEIGHBOR_CACHE_ENTRY         *ip6_def_rtr_nc_entry;
    DV_DEVICE_ENTRY                  *ip6_def_rtr_device;
};

#define ip6_def_rtr_global_addr ip6_def_rtr_global.ip6_def_rtr_ip_addr
#define ip6_def_rtr_prefix_len  ip6_def_rtr_global.ip6_def_rtr_prfx_len
#define ip6_def_rtr_val_life    ip6_def_rtr_global.ip6_def_rtr_valid_lifetime
#define ip6_def_rtr_pref_life   ip6_def_rtr_global.ip6_def_rtr_pref_lifetime

typedef struct _ip6_default_router_list
{
    IP6_DEFAULT_ROUTER_ENTRY        *dv_head;
    IP6_DEFAULT_ROUTER_ENTRY        *dv_tail;
} IP6_DEFAULT_ROUTER_LIST;

IP6_DEFAULT_ROUTER_ENTRY    *DEFRTR6_Create_Default_Router_Entry(DV_DEVICE_ENTRY *, 
                                                                 const UINT8 *, UINT16);
VOID                        DEFRTR6_Delete_Default_Router(const UINT8 *);
VOID                        DEFRTR6_Delete_Entry(IP6_DEFAULT_ROUTER_ENTRY *);
IP6_DEFAULT_ROUTER_ENTRY    *DEFRTR6_Find_Default_Router_Entry(const UINT8 *);
VOID                        DEFRTR6_Update_Default_Router_List(IP6_DEFAULT_ROUTER_ENTRY *, UINT8);
IP6_DEFAULT_ROUTER_ENTRY    *DEFRTR6_Find_Default_Router(VOID);
VOID                        DEFRTR6_Expire_Default_Router_Entry(TQ_EVENT, UNSIGNED, UNSIGNED);
IP6_DEFAULT_ROUTER_ENTRY    *DEFRTR6_Get_Router_By_Index(const INT);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

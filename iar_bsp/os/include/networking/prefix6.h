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
*       prefix6.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the IPv6 Prefix List.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       _ip6_prefix_entry   
*       _ip6_prefix_list
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef PREFIX6_H
#define PREFIX6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define LINK_LOCAL_PREFIX_LENGTH    8

#define PRFX6_NO_ADV_ON_LINK        0x1    
#define PRFX6_NO_ADV_AUTO           0x2
#define PRFX6_DEC_VAL_LIFE          0x4
#define PRFX6_DEC_PREF_LIFE         0x8
#define PRFX6_HOME_NETWORK          0x10

struct _ip6_prefix_entry
{
    struct _ip6_prefix_entry *ip6_prefx_lst_next;
    struct _ip6_prefix_entry *ip6_prefx_lst_previous;
    UINT8                    ip6_prfx_lst_prefix[16];
    UINT8                    ip6_prfx_lst_prfx_length;
    UINT8                    padN[3];
    UINT32                   ip6_prfx_lst_flags;
    UINT32                   ip6_prfx_lst_valid_life_exp;
    UINT32                   ip6_prfx_lst_valid_life;
    UINT32					 ip6_prfx_lst_stored_life;
    UINT32                   ip6_prfx_lst_pref_life_exp;
    UINT32                   ip6_prfx_lst_pref_life;
    UINT32                   ip6_prfx_lst_index;
    DV_DEVICE_ENTRY          *ip6_prefx_lst_device;
};

struct _ip6_prefix_list
{
    IP6_PREFIX_ENTRY    *dv_head;
    IP6_PREFIX_ENTRY    *dv_tail;
};

IP6_PREFIX_ENTRY    *PREFIX6_Find_Prefix_List_Entry(const DV_DEVICE_ENTRY *, 
                                                    const UINT8 *);
STATUS              PREFIX6_New_Prefix_Entry(DV_DEVICE_ENTRY *, const UINT8 *, UINT8, 
                                             UINT32, UINT32, UINT32);
STATUS              PREFIX6_Configure_DEV_Prefix_Entry(DEV6_PRFX_ENTRY *,
                                                       DV_DEVICE_ENTRY *);
VOID                PREFIX6_Delete_Prefix(const DV_DEVICE_ENTRY *, const UINT8 *);
VOID                PREFIX6_Delete_Entry(IP6_PREFIX_ENTRY *);
IP6_PREFIX_ENTRY    *PREFIX6_Match_Longest_Prefix_By_Device(const IP6_PREFIX_LIST *, UINT8 *);
VOID                PREFIX6_Expire_Entry(TQ_EVENT, UNSIGNED, UNSIGNED);
IP6_PREFIX_ENTRY    *PREFIX6_Match_Longest_Prefix(UINT8 *);
IP6_PREFIX_ENTRY    *PREFIX6_Find_Prefix(const DV_DEVICE_ENTRY *, const UINT8 *);
STATUS              PREFIX6_Init_Prefix_List(IP6_PREFIX_LIST **);
IP6_PREFIX_ENTRY    *PREFIX6_Find_Home_Prefix(const DV_DEVICE_ENTRY *, const UINT8 *);
IP6_PREFIX_ENTRY    *PREFIX6_Find_On_Link_Prefix(const DV_DEVICE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

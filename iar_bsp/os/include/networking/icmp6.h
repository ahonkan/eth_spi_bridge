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

/* Portions of this program were written by: */
/*************************************************************************
*                                                                         
* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 and 2001 WIDE Project.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this 
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS 
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*                                                                         
*************************************************************************/

/*************************************************************************
*                                                                          
*   FILE NAME                                                                    
*                                                                                  
*       icmp6.h                                      
*                                                                                  
*   DESCRIPTION                                                              
*                       
*       This file contains the data structures and defines necessary
*       to support ICMPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*           
*       _icmp6_echo_list
*       ICMP6_LAYER
*       ND_ROUTER_SOLICIT
*       ND_ROUTER_ADVERT
*       ND_NEIGHBOR_SOLICIT
*       ND_NEIGHBOR_ADVERT
*       ND_REDIRECT
*       IP6_ROUTER_PREFIX
*       icmp6errstat
*       icmp6stat
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef ICMP6_H
#define ICMP6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */

#define ICMP6_COMPUTE_RTIME(x) \
        (((IP6_MIN_RANDOM_FACTOR * x) + (UTL_Rand() & \
         ((IP6_MAX_RANDOM_FACTOR - IP6_MIN_RANDOM_FACTOR) * x))) / (TICKS_PER_SECOND))

/* Base ICMP Header sizes */
#define IP6_RTR_SOL_HDR_SIZE           8
#define IP6_RTR_ADV_HDR_SIZE           16
#define IP6_NEIGH_SOL_HDR_SIZE         24
#define IP6_NEIGH_ADV_HDR_SIZE         24
#define IP6_LINK_LAYER_OPT_SIZE        2
#define IP6_LINK_LAYER_OPT_LENGTH      8
#define IP6_MTU_OPT_SIZE               8
#define IP6_PREFIX_OPT_SIZE            32
#define IP6_ECHO_REQUEST_HDR_SIZE      8
#define IP6_ECHO_REPLY_HDR_SIZE        8
#define IP6_REDIRECT_SIZE              40
#define IP6_DHAAD_REQ_HDR_SIZE         8
#define IP6_DHAAD_REPLY_HDR_SIZE       8
#define IP6_PRFXSOL_HDR_SIZE           8
#define IP6_PRFXADV_HDR_SIZE           8
#define IP6_RTR_ADV_INT_OPT_SIZE       8

/* ICMP Message Types */
#define ICMP6_RTR_SOL               133 /* Router Solicitation */
#define ICMP6_RTR_ADV               134 /* Router Advertisement */
#define ICMP6_NEIGH_SOL             135 /* Neighbor Solicitation */
#define ICMP6_NEIGH_ADV             136 /* Neighbor Advertisement */
#define ICMP6_REDIRECT              137 /* Redirect */
#define ICMP6_DST_UNREACH           1   /* Destination Unreachable */
#define ICMP6_PACKET_TOO_BIG        2   /* Packet Too Big */
#define ICMP6_TIME_EXCEEDED         3   /* Time Exceeded */
#define ICMP6_PARAM_PROB            4   /* Parameter Problem */
#define ICMP6_ECHO_REQUEST          128 /* Echo Request */
#define ICMP6_ECHO_REPLY            129 /* Echo Reply */
#define ICMP6_WRUREQUEST            139 /* Who Are You Request */
#define ICMP6_WRUREPLY              140 /* Who Are You Reply */
#define ICMP6_ROUTER_RENUMBERING    138 /* Router Renumbering */
#define ICMP6_HA_ADDR_DISC_REQ      144 /* Dynamic Home Agent Address Discovery Request */
#define ICMP6_HA_ADDR_DISC_REPLY    145 /* Dynamic Home Agent Address Discovery Reply */
#define ICMP6_MP_SOLICIT            146 /* Mobile Prefix Solicitation */
#define ICMP6_MP_ADV                147 /* Mobile Prefix Reply */


/* Destination Unreachable Codes */
#define ICMP6_DST_UNREACH_NOROUTE   0
#define ICMP6_DST_UNREACH_ADMIN     1
#define ICMP6_DST_UNREACH_ADDRESS   3
#define ICMP6_DST_UNREACH_PORT      4

/* Time Exceeded Codes */
#define ICMP6_TIME_EXCD_HPLMT       0   /* Hop Limit exceeded in transit */
#define ICMP6_TIME_EXCD_REASM       1   /* Fragment reassembly time exceeded */

/* Parameter Problem Codes */
#define ICMP6_PARM_PROB_HEADER      0
#define ICMP6_PARM_PROB_NEXT_HDR    1
#define ICMP6_PARM_PROB_OPTION      2

/* ICMP Option Types */
#define IP6_ICMP_OPTION_SRC_ADDR       1   /* Source Link-Layer Address */
#define IP6_ICMP_OPTION_TAR_ADDR       2   /* Target Link-Layer Address */
#define IP6_ICMP_OPTION_PREFIX         3   /* Prefix */
#define IP6_ICMP_OPTION_RED_HDR        4   /* Redirect Header */
#define IP6_ICMP_OPTION_MTU            5   /* Link MTU */
#define IP6_ICMP_OPTION_RTR_ADV_INT    7   /* Router Advertisement Interval */

/* ICMP Offsets */
#define IP6_ICMP_TYPE_OFFSET                   0
#define IP6_ICMP_CODE_OFFSET                   1
#define IP6_ICMP_CKSUM_OFFSET                  2
#define IP6_ICMP_RESERVED_OFFSET               4
#define IP6_ICMP_DATA_OFFSET                   8

/* ICMP Router Solicitation Offsets */
#define IP6_ICMP_RTR_SOL_RES_OFFSET            4
#define IP6_ICMP_RTR_SOL_OPTIONS_OFFSET        8

/* ICMP Router Advertisement Offsets */
#define IP6_ICMP_RTR_ADV_CURHOPLMT_OFFSET      4
#define IP6_ICMP_RTR_ADV_MGDANDCFG_BIT_OFFSET  5
#define IP6_ICMP_RTR_ADV_RTR_LIFETIME_OFFSET   6
#define IP6_ICMP_RTR_ADV_RCHBL_TIME_OFFSET     8
#define IP6_ICMP_RTR_ADV_RTRNS_TMR_OFFSET      12
#define IP6_ICMP_RTR_ADV_OPTIONS_OFFSET        16

/* ICMP Neighbor Solicitation Offsets */
#define IP6_ICMP_NEIGH_SOL_RES_OFFSET          4
#define IP6_ICMP_NEIGH_SOL_TRGT_ADDRS_OFFSET   8
#define IP6_ICMP_NEIGH_SOL_OPTIONS_OFFSET      24

/* ICMP Neighbor Advertisement Offsets */
#define IP6_ICMP_NEIGH_ADV_FLAG_OFFSET         4
#define IP6_ICMP_NEIGH_ADV_TRGT_ADDRS_OFFSET   8
#define IP6_ICMP_NEIGH_ADV_OPTIONS_OFFSET      24

/* ICMP Redirect Offsets */
#define IP6_ICMP_REDIRECT_TRGT_ADDRS_OFFSET    8
#define IP6_ICMP_REDIRECT_DEST_ADDRS_OFFSET    24
#define IP6_ICMP_REDIRECT_OPTIONS_OFFSET       40

/* ICMP Option Offsets */
#define IP6_ICMP_OPTION_TYPE_OFFSET            0
#define IP6_ICMP_OPTION_LENGTH_OFFSET          1

/* ICMP Link-Layer Address Option Offsets */
#define IP6_ICMP_LL_OPTION_ADDRESS_OFFSET      2

/* ICMP Prefix Option Offsets */
#define IP6_ICMP_PREFIX_PRE_LENGTH_OFFSET      2
#define IP6_ICMP_PREFIX_FLAG_OFFSET            3
#define IP6_ICMP_PREFIX_VALID_LIFETIME_OFFSET  4
#define IP6_ICMP_PREFIX_PREF_LIFETIME_OFFSET   8
#define IP6_ICMP_PREFIX_RES2_OFFSET            12
#define IP6_ICMP_PREFIX_PREFIX_OFFSET          16

/* ICMP Redirected Header Option Offsets */
#define IP6_ICMP_RED_OPTION_TYPE_OFFSET        0
#define IP6_ICMP_RED_OPTION_LEN_OFFSET         1
#define IP6_ICMP_RED_OPTION_RES1_OFFSET        2
#define IP6_ICMP_RED_OPTION_RES2_OFFSET        4
#define IP6_ICMP_RED_OPTION_DATA_OFFSET        8

/* ICMP MTU Option Offsets */
#define IP6_ICMP_MTU_RESERVED_OFFSET           2
#define IP6_ICMP_MTU_OFFSET                    4

/* ICMP Echo Request Offsets */
#define IP6_ICMP_ECHO_ID                       4
#define IP6_ICMP_ECHO_SEQ                      6
#define IP6_ICMP_ECHO_DATA                     8

/* ICMP Destination Unreachable Offsets */
#define IP6_DST_UNREACH_UNUSED                 4
#define IP6_DST_UNREACH_DATA                   8

/* ICMP Parameter Problem Offsets */
#define IP6_PARAM_PROB_PTR                     4
#define IP6_PARAM_PROT_DATA                    8

/* ICMP Time Exceeded Offsets */
#define IP6_TIME_EXCEEDED_DATA                 8

/* ICMP Packet Too Big Offsets */
#define IP6_PKT_TOO_BIG_MTU                    4
#define IP6_PKT_TOO_BIG_DATA                   8

/* Home Agent Address Discovery Request Header Offsets */
#define ICMP6_HA_ADDR_DISC_REQ_ID_OFFSET        4
#define ICMP6_HA_ADDR_DISC_REQ_RSVD_OFFSET      6

/* Home Agent Address Discovery Reply Header Offsets */
#define ICMP6_HA_ADDR_DISC_REPLY_ID_OFFSET      4
#define ICMP6_HA_ADDR_DISC_REPLY_RSVD_OFFSET    6
#define ICMP6_HA_ADDR_DISC_REPLY_HA_ADDR_OFFSET 8

/* Mobile Prefix Solicitation Header Offsets */
#define ICMP6_MP_SOLICIT_ID_OFFSET      4
#define ICMP6_MP_SOLICIT_RSVD_OFFSET    6

/* Mobile Prefix Advertisement Header Offsets */
#define ICMP6_MP_ADV_ID_OFFSET              4
#define ICMP6_MP_ADV_MGDANDCFG_BIT_OFFSET   6
#define ICMP6_MP_ADV_OPT_OFFSET             8

/* Advertisement Interval Option Header Offsets */
#define ICMP6_ADV_INT_TYPE_OFFSET       0
#define ICMP6_ADV_INT_LEN_OFFSET        1
#define ICMP6_ADV_INT_RSVD_OFFSET       2
#define ICMP6_ADV_INT_ADV_INT_OFFSET    4


#define ICMP6_HEADER_LEN            4

#define IP6_PREFIX_FLAG_ONLINK      0x80
#define IP6_PREFIX_FLAG_AUTO        0x40
#define IP6_PREFIX_FLAG_ROUTER      0x20

#define IP6_NA_FLAG_ROUTER      0x80
#define IP6_NA_FLAG_SOLICITED   0x40
#define IP6_NA_FLAG_OVERRIDE    0x20

/* Router Advertisement Flags */
#define IP6_RA_MANAGED_FLAG     0x80
#define IP6_RA_CONFIG_FLAG      0x40

/* Mobile Prefix Advertisement Flags */
#define IP6_PA_MANAGED_FLAG     0x80
#define IP6_PA_CONFIG_FLAG      0x40

/* Router Configuration Macros */
#define IP6_ROUTER_MESSAGES            NU_FALSE
#define IP6_MAX_RTR_ADV_INTERVAL       600 * TICKS_PER_SECOND
#define IP6_MIN_RTR_ADV_INTERVAL       IP6_MAX_RTR_ADV_INTERVAL / 3 * TICKS_PER_SECOND
#define IP6_ROUTER_MANAGED_FLAG        NU_FALSE
#define IP6_ROUTER_CONFIG_FLAG         NU_FALSE
#define IP6_ROUTER_LINK_MTU            0
#define IP6_ROUTER_REACHABLE_TIME      0
#define IP6_ROUTER_RETRANS_TIMER       0
#define IP6_ROUTER_CUR_HOP_LIMIT       0
#define IP6_ROUTER_LIFETIME            3 * IP6_MAX_RTR_ADV_INTERVAL * TICKS_PER_SECOND

/* Validation Values */
#define ICMP6_VALID_HOP_LIMIT           255 /* Valid Hop Limit */
#define ICMP6_VALID_CODE                0   /* Valid Code */
#define ICMP6_RTRSOL_MIN_LENGTH         8   /* Minimum valid length for Router 
                                               Solicitation */
#define ICMP6_RTRADV_MIN_LENGTH         16  /* Minimum valid length for Router 
                                               Advertisement */
#define ICMP6_NEIGHSOL_MIN_LENGTH       24  /* Minimum valid length for Neighbor
                                               Solicitation */
#define ICMP6_NEIGHADV_MIN_LENGTH       24  /* Minimum valid length for Neighbor
                                               Advertisement */
#define ICMP6_REDIRECT_MIN_LENGTH       40  /* Minimum valid length for Neighbor
                                               Advertisement */

#define ICMP6_ERROR_MSG(type)               \
        (type <= 127)                       \

#define ICMP6_INFO_MSG(type)                \
        ((type >= 128) && (type <= 255))    \

#define ICMP6_MAP_ERROR(type, code)                                         \
       ((type == ICMP6_DST_UNREACH)          ?                              \
        ((code == ICMP6_DST_UNREACH_NOROUTE) ? NU_NO_ROUTE_TO_HOST     :    \
        (code == ICMP6_DST_UNREACH_ADMIN)    ? NU_DEST_UNREACH_ADMIN   :    \
        (code == ICMP6_DST_UNREACH_ADDRESS)  ? NU_DEST_UNREACH_ADDRESS :    \
        (code == ICMP6_DST_UNREACH_PORT)     ? NU_DEST_UNREACH_PORT    : NU_INVAL) : \
        (type == ICMP6_TIME_EXCEEDED)        ?                                       \
        ((code == ICMP6_TIME_EXCD_HPLMT)     ? NU_TIME_EXCEED_HOPLIMIT :    \
        (code == ICMP6_TIME_EXCD_REASM)      ? NU_TIME_EXCEED_REASM    : NU_INVAL) : \
        (type == ICMP6_PARAM_PROB)           ?                                       \
        ((code == ICMP6_PARM_PROB_HEADER)    ? NU_PARM_PROB_HEADER     :    \
        (code == ICMP6_PARM_PROB_NEXT_HDR)   ? NU_PARM_PROB_NEXT_HDR   :    \
        (code == ICMP6_PARM_PROB_OPTION)     ? NU_PARM_PROB_OPTION     : NU_INVAL) : NU_INVAL)

/* This is the head of the linked list for echo requests. */
typedef struct _icmp6_echo_list
{
    ICMP_ECHO_LIST_ENTRY    *icmp6_head;
    ICMP_ECHO_LIST_ENTRY    *icmp6_tail;
}ICMP6_ECHO_LIST;

/* ICMP6 Header */
typedef struct icmp6_layer
{
    UINT8   icmp6_type; /* type field */
    UINT8   icmp6_code; /* code field */
    UINT16  icmp6_cksum;    /* checksum field */

    union 
    {
        UINT32  icmp6_un_data32[1]; /* type-specific field */
        UINT16  icmp6_un_data16[2]; /* type-specific field */
        UINT8   icmp6_un_data8[4];  /* type-specific field */
    } icmp6_dataun;
} ICMP6_LAYER;

#define icmp6_data32    icmp6_dataun.icmp6_un_data32
#define icmp6_data16    icmp6_dataun.icmp6_un_data16
#define icmp6_data8     icmp6_dataun.icmp6_un_data8
#define icmp6_pptr      icmp6_data32[0]     /* parameter prob */
#define icmp6_mtu       icmp6_data32[0]     /* packet too big */
#define icmp6_id        icmp6_data16[0]     /* echo request/reply */
#define icmp6_seq       icmp6_data16[1]     /* echo request/reply */
#define icmp6_maxdelay  icmp6_data16[0]     /* mcast group membership */

/*
 * Neighbor Discovery
 */

/* Router Solicitation */
typedef struct nd_router_solicit 
{
    ICMP6_LAYER         nd_rs_hdr;
    /* could be followed by options */
} ND_ROUTER_SOLICIT;

#define nd_rs_type      nd_rs_hdr.icmp6_type
#define nd_rs_code      nd_rs_hdr.icmp6_code
#define nd_rs_cksum     nd_rs_hdr.icmp6_cksum
#define nd_rs_reserved  nd_rs_hdr.icmp6_data32[0]

/* Router Advertisement */
struct nd_router_advert 
{
    ICMP6_LAYER  nd_ra_hdr;
    UINT32       nd_ra_reachable;   /* reachable time */
    UINT32       nd_ra_retransmit;  /* retransmit timer */
    /* could be followed by options */
};

#define nd_ra_type              nd_ra_hdr.icmp6_type
#define nd_ra_code              nd_ra_hdr.icmp6_code
#define nd_ra_cksum             nd_ra_hdr.icmp6_cksum
#define nd_ra_curhoplimit       nd_ra_hdr.icmp6_data8[0]
#define nd_ra_flags_reserved    nd_ra_hdr.icmp6_data8[1]
#define nd_ra_router_lifetime   nd_ra_hdr.icmp6_data16[1]

/* Neighbor Solicitation */
typedef struct nd_neighbor_solicit 
{
    ICMP6_LAYER         nd_ns_hdr;
    struct id_struct    nd_ns_target;   /*target address */
    /* could be followed by options */
} ND_NEIGHBOR_SOLICIT;

#define nd_ns_type      nd_ns_hdr.icmp6_type
#define nd_ns_code      nd_ns_hdr.icmp6_code
#define nd_ns_cksum     nd_ns_hdr.icmp6_cksum
#define nd_ns_reserved  nd_ns_hdr.icmp6_data32[0]

/* Neighbor Advertisement */
typedef struct nd_neighbor_advert 
{
    ICMP6_LAYER         nd_na_hdr;
    struct id_struct    nd_na_target;   /* target address */
    /* could be followed by options */
} ND_NEIGHBOR_ADVERT;

#define nd_na_type              nd_na_hdr.icmp6_type
#define nd_na_code              nd_na_hdr.icmp6_code
#define nd_na_cksum             nd_na_hdr.icmp6_cksum
#define nd_na_flags_reserved    nd_na_hdr.icmp6_data32[0]

/* Redirect */
typedef struct nd_redirect 
{
    ICMP6_LAYER         nd_rd_hdr;
    struct id_struct    nd_rd_target;   /* target address */
    struct id_struct    nd_rd_dst;  /* destination address */
    /* could be followed by options */
} ND_REDIRECT;

#define nd_rd_type          nd_rd_hdr.icmp6_type
#define nd_rd_code          nd_rd_hdr.icmp6_code
#define nd_rd_cksum         nd_rd_hdr.icmp6_cksum
#define nd_rd_reserved      nd_rd_hdr.icmp6_data32[0]

typedef struct _ip6_router_prefix
{
    UINT8   ip6_router_prefix[8];
    UINT32  ip6_router_prefix_valid_lifetime;
    UINT32  ip6_router_prefix_preferred_lifetime;
    UINT8   ip6_router_prefix_onlink_flag;
    UINT8   ip6_router_prefix_auto_flag;
    struct _ip6_router_prefix  *ip6_router_prefix_next;

} IP6_ROUTER_PREFIX;

VOID        ICMP6_Init(VOID);
VOID        ICMP6_RtrSolicitation_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
UINT32      ICMP6_Random_Delay(const UINT8 *, const UINT8 *, UINT32);
NET_BUFFER  *ICMP6_Header_Init(UINT8, UINT8, UINT32);
STATUS      ICMP6_Interpret(IP6LAYER *pkt, DV_DEVICE_ENTRY *, NET_BUFFER *, 
                            struct pseudohdr *);
STATUS      ICMP6_Echo_Reply (NET_BUFFER *, IP6LAYER *);
NET_BUFFER  *ICMP6_Echo_Request_Build(UINT16, UINT16);
VOID        ICMP6_Process_Packet_Too_Big(const NET_BUFFER *);
VOID        ICMP6_Process_Error(NET_BUFFER *);
VOID        ICMP6_Send_Error(IP6LAYER *, NET_BUFFER *, UINT8, UINT8, UINT32, 
                             DV_DEVICE_ENTRY *);
VOID        ICMP6_Process_IPv4_Error(NET_BUFFER *, const UINT8 *, UINT8, UINT8, UINT32);

#define IP6_Stateful_Config()

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

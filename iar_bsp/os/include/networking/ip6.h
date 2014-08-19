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
*       ip6.h                                        
*                                                                                  
*   DESCRIPTION                                                              
*                       
*       This file contains the data structures and defines necessary
*       to support the IP layer of IPv6.
*                                                                          
*   DATA STRUCTURES                                                          
*           
*       IP6LAYER
*       IP6_FRAG
*       _IP6_MULTI
*
*   DEPENDENCIES                                                             
*                                                                          
*       mem_defs.h
*       sockdefs.h
*       rtab6.h
*                                                                          
*************************************************************************/

#ifndef IPV6_H
#define IPV6_H

#include "networking/mem_defs.h"
#include "networking/sockdefs.h"
#include "networking/rtab6.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

extern struct id_struct     IP6_ADDR_ANY;
extern struct id_struct     IP6_LOOPBACK_ADDR;

#define IP_ICMPV6_PROT          58
#define IP6_MBLTY_PROT          135
#define IP6_HOP_BY_HOP          0
#define IP6_ROUTER_ALERT_OPT    5

#define IP6_MIN_LINK_MTU                1280

/* Defauly Policy Table actions. */
#define IP6_ADD_POLICY          0
#define IP6_DELETE_POLICY       1
#define IP6_UPDATE_POLICY       2

typedef struct _ip6_s_options
{
    UINT8       *tx_source_address;
    UINT8       *tx_dest_address;
    UINT8       *tx_route_hdr;  /* Type 0 Routing Header */
    UINT8       *tx_route_hdr2; /* Type 2 Routing Header */
    UINT8       *tx_hop_opt;
    UINT8       *tx_dest_opt;
    UINT8       tx_hop_limit;
    UINT8       tx_traffic_class;
    UINT8       *tx_rthrdest_opt;
    RTAB6_ROUTE tx_route;
#if (INCLUDE_IP_RAW == NU_TRUE)
    INT         tx_raw_chk_off;
#endif
} IP6_S_OPTIONS;

typedef struct ip6h
{
    UINT32  ip6_xxx;       /* Version, Traffic Class, and Flow Label */
    UINT16  ip6_paylen;    /* Payload Length */
    UINT8   ip6_next;      /* Next Header */
    UINT8   ip6_hop;       /* Hop Limit */
    UINT8   ip6_src[16];   /* Source IP Address */
    UINT8   ip6_dest[16];  /* Destination IP Address */
} IP6LAYER;

struct pseudohdr
{
    UINT8   source[16];     /* Source IP address. */
    UINT8   dest[16];       /* Destination IP address. */
    UINT32  length;         /* Upper-Layer packet length. */
    UINT8   zero;           /* Zero */
    UINT8   next;           /* Next Header */
    UINT8   padN[2];    
};

typedef struct IP6_FRAG_STRUCT 
{
    UINT32                  ipf6_xxx;        /* Version, Traffic Class, and Flow Label */
    UINT16                  ipf6_pll;        /* Payload Length */
    UINT8                   ipf6_nxt;        /* Next Header */
    UINT8                   ipf6_hop;        /* Hop Limit */
    UINT8                   ipf6_src[16];    /* Source Address */
    UINT8                   ipf6_dest[16];   /* Destination Address */
    UINT8                   ipf6_buf_offset; /* The protocol field is used to 
                                               store the offset of the IP layer 
                                               from the start of the memory 
                                               buffer.
                                            */
    struct IPV6_FRAG_STRUCT   *ipf6_next;
    struct IPV6_FRAG_STRUCT   *ipf6_prev;
} HUGE IP6_FRAG;

struct ip6_ext 
{
    UINT8   ip6e_nxt;
    UINT8   ip6e_len;
};

/* Hop-by-Hop options header */
struct ip6_hbh 
{
    UINT8   ip6h_nxt;   /* next header */
    UINT8   ip6h_len;   /* length in units of 8 octets */
};

/* Destination options header */
struct ip6_dest 
{
    UINT8   ip6d_nxt;   /* next header */
    UINT8   ip6d_len;   /* length in units of 8 octets */
};

/* Routing header */
struct ip6_rthdr 
{
    UINT8   ip6r_nxt;        /* next header */
    UINT8   ip6r_len;        /* length in units of 8 octets */
    UINT8   ip6r_type;       /* routing type */
    UINT8   ip6r_segleft;    /* segments left */
};

/* Type 0 Routing header */
struct ip6_rthdr0 
{
    UINT8   ip6r0_nxt;       /* next header */
    UINT8   ip6r0_len;       /* length in units of 8 octets */
    UINT8   ip6r0_type;      /* always zero */
    UINT8   ip6r0_segleft;   /* segments left */
    UINT32  ip6r0_reserved;  /* reserved field */
};

/* Fragment header */
struct ip6_frag 
{
    UINT8   ip6f_nxt;       /* next header */
    UINT8   ip6f_reserved;  /* reserved field */
    UINT16  ip6f_offlg;     /* offset, reserved, and flag */
    UINT32  ip6f_ident;     /* identification */
};

/* Reassembly Structure. This overlays the 16 byte source address
 * field of the IPv6 Header. Please note that the size of this
 * structure is also 16 bytes.
 */
typedef struct IP6_REASM_STRUCT 
{    
    UINT8                   ipf_mff;        /* More fragment Bit. */
    UINT8                   ipf_buf_offset; /* Buffer Offset */
    UINT16                  unfrag_len;     /* Unfragmented length */    
    UINT16                  ipf_plen;       /* Payload Length */    
    UINT16                  ipf_off;        /* Contains only the fragment offset */
    UINT32                  ip6f_hdr;       /* Contains the start of the IPV6 packet */
    struct IP6F_FRAG_STRUCT *ipf_next;  
    
} IP6_REASM;

/* Define the MAX size of data that can be contained in one IPv6 packet. */
#define IP6_MAX_DATA_SIZE   65476

#define IP6_VERSION         0x60000000UL

/* Length of IP address in bytes */
#define IP6_ADDR_LEN        16

/* Length of Interface ID's for different transport mediums */
#define IP6_ETHERNET_ID_LENGTH  8

/* Length of the IP header with no next header */
#define IP6_HEADER_LEN      40

/* The length of the Solicited Node Multicast Address Prefix */
#define IP6_SOL_NODE_MULTICAST_LENGTH   13

/* The length of a Fragment Header */
#define IP6_FRAGMENT_HDR_LENGTH         8

/* The Link-Local Prefix */
#define IP6_LINK_LOCAL_PREFIX   0xFE80

/* The Multicast Prefix */
#define IP6_MULTICAST_PREFIX    0xFF02

typedef struct _IP6_QUEUE_ELEMENT 
{
    struct _IP6_QUEUE_ELEMENT   *ipq_next;      
    struct _IP6_QUEUE_ELEMENT   *ipq_prev;      
    IP6_REASM    HUGE           *ipq_first_frag;    
    UINT8                       ipq_source[IP6_ADDR_LEN];
    UINT8                       ipq_dest[IP6_ADDR_LEN];
    UINT32                      ipq_id;
    
} IP6_QUEUE_ELEMENT;

typedef struct _IP6_QUEUE
{
     IP6_QUEUE_ELEMENT  *ipq_head;
     IP6_QUEUE_ELEMENT  *ipq_tail;
} IP6_QUEUE;

typedef struct ip6_policy_entry
{
    UINT8                   prefix[IP6_ADDR_LEN];
    UINT16                  prefix_len;
    UINT8                   precedence;
    UINT8                   label;
} IP6_POLICY_ENTRY;

typedef struct ip6_policy_table_entry
{
    struct ip6_policy_table_entry   *next;
    struct ip6_policy_table_entry   *prev;
    IP6_POLICY_ENTRY                policy;
} IP6_POLICY_TABLE_ENTRY;

typedef struct ip6_policy_table_struct
{
    IP6_POLICY_TABLE_ENTRY  *head;
    IP6_POLICY_TABLE_ENTRY  *tail;
} IP6_POLICY_TABLE_STRUCT;

/* The following macro returns a pointer to the IPv6 header within a buffer. */
#define IP6_BUFF_TO_IP(buff_ptr) \
        (IP6LAYER *)(buff_ptr->data_ptr - IP6_HEADER_LEN)

#define IP6_HEADER_COPY(dest, src)                                         \
        /* Copy the IPv6 header over. We will do this 32 bits at a time. */ \
                                                                            \
        /* Version, Traffic Class, and Flow Label */                        \
        PUT32((UINT8*)dest, IP6_XXX_OFFSET,                                \
              GET32((UINT8*)src, IP6_XXX_OFFSET));                         \
                                                                            \
        /* Payload Length, Next Header and Hop Limit */                     \
        PUT32((UINT8*)dest, IP6_PAYLEN_OFFSET,                             \
              GET32((UINT8*)src, IP6_PAYLEN_OFFSET));                      \
                                                                            \
        /* Source Address (first 4 bytes) */                                \
        PUT32((UINT8*)dest, IP6_SRCADDR_OFFSET,                            \
              GET32((UINT8*)src, IP6_SRCADDR_OFFSET));                       \
                                                                            \
        /* Source Address (second 4 bytes) */                               \
        PUT32((UINT8*)dest, IP6_SRCADDR_OFFSET + 4,                        \
              GET32((UINT8*)src, IP6_SRCADDR_OFFSET + 4));                   \
                                                                            \
        /* Source Address (third 4 bytes) */                                \
        PUT32((UINT8*)dest, IP6_SRCADDR_OFFSET + 8,                        \
              GET32((UINT8*)src, IP6_SRCADDR_OFFSET + 8));                   \
                                                                            \
        /* Source Address (fourth 4 bytes) */                               \
        PUT32((UINT8*)dest, IP6_SRCADDR_OFFSET + 12,                       \
              GET32((UINT8*)src, IP6_SRCADDR_OFFSET + 12));                  \
                                                                            \
        /* Destination Address (first 4 bytes) */                           \
        PUT32((UINT8*)dest, IP6_DESTADDR_OFFSET,                           \
              GET32((UINT8*)src, IP6_DESTADDR_OFFSET));                      \
                                                                            \
        /* Destination Address (second 4 bytes) */                          \
        PUT32((UINT8*)dest, IP6_DESTADDR_OFFSET + 4,                       \
              GET32((UINT8*)src, IP6_DESTADDR_OFFSET + 4));                  \
                                                                            \
        /* Destination Address (third 4 bytes) */                           \
        PUT32((UINT8*)dest, IP6_DESTADDR_OFFSET + 8,                       \
              GET32((UINT8*)src, IP6_DESTADDR_OFFSET + 8));                  \
                                                                            \
        /* Destination Address (fourth 4 bytes) */                          \
        PUT32((UINT8*)dest, IP6_DESTADDR_OFFSET + 12,                      \
              GET32((UINT8*)src, IP6_DESTADDR_OFFSET + 12));

#define IP6_ADDRS_EQUAL(source, dest)   \
    !((*((UINT32*)(&source[12]))) ^ (*((UINT32*)(&dest[12])))) &&   \
    !((*((UINT32*)(&source[8]))) ^ (*((UINT32*)(&dest[8]))))   &&   \
    !((*((UINT32*)(&source[4]))) ^ (*((UINT32*)(&dest[4]))))   &&   \
    !((*((UINT32*)(&source[0]))) ^ (*((UINT32*)(&dest[0]))))

/* The following macros determine whether an IPv6 address is of the
 * specified type.
 */

/* The Unspecified Address */
#define IPV6_IS_ADDR_UNSPECIFIED(addr)  \
    ((GET32(addr, 0)  == 0) && \
     (GET32(addr, 4)  == 0) && \
     (GET32(addr, 8)  == 0) && \
     (GET32(addr, 12) == 0))

/* The Loopback Address */
#define IPV6_IS_ADDR_LOOPBACK(addr)     \
    ((GET32(addr, 0)  == 0) && \
     (GET32(addr, 4)  == 0) && \
     (GET32(addr, 8)  == 0) && \
     (GET32(addr, 12) == 1))

/* An IPv4 compatible Address */
#define IPV6_IS_ADDR_V4COMPAT(addr)     \
    ((GET32(addr, 0)  == 0) && \
     (GET32(addr, 4)  == 0) && \
     (GET32(addr, 8)  == 0) && \
     (GET32(addr, 12) != 0) && \
     (GET32(addr, 12) != 1))

/* A Mapped Address */
#define IPV6_IS_ADDR_V4MAPPED(addr)           \
    ((GET32(addr, 0)  == 0) && \
     (GET32(addr, 4)  == 0) && \
     (GET32(addr, 8)  == 0x0000ffff))

/* The following macros determine the scope of the address. */

/* Link-Local */
#define IPV6_IS_ADDR_LINKLOCAL(addr)    \
    (((addr)[0] == 0xfe) && (((addr)[1] & 0xc0) == 0x80))

#define IN6_IS_ADDR_SITELOCAL(addr) \
    (((addr)[0] == 0xfe) && (((addr)[1] & 0xc0) == 0xc0))

/* Multicast */
#define IPV6_IS_ADDR_MULTICAST(addr)    ((addr)[0] == 0xff)

#define IPV6_ADDR_MC_SCOPE(addr)        ((addr)[1] & 0x0f)

#define IPV6_ADDR_SCOPE_NODELOCAL   0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL   0x02
#define IPV6_ADDR_SCOPE_SITELOCAL   0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL    0x08    /* just used in this file */
#define IPV6_ADDR_SCOPE_GLOBAL      0x0e

/* Multicast Node-Local */
#define IPV6_IS_ADDR_MC_NODELOCAL(a)    \
    (IPV6_IS_ADDR_MULTICAST(a) &&   \
     (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_NODELOCAL))

/* Multicast Link-Local */
#define IPV6_IS_ADDR_MC_LINKLOCAL(a)    \
    (IPV6_IS_ADDR_MULTICAST(a) &&   \
     (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_LINKLOCAL))

/* Multicast Site-Local */
#define IPV6_IS_ADDR_MC_SITELOCAL(a)    \
    (IPV6_IS_ADDR_MULTICAST(a) &&   \
     (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_SITELOCAL))

/* Multicast Organization-Local */
#define IPV6_IS_ADDR_MC_ORGLOCAL(a) \
    (IPV6_IS_ADDR_MULTICAST(a) &&   \
     (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_ORGLOCAL))

/* Multicast Global */
#define IPV6_IS_ADDR_MC_GLOBAL(a)   \
    (IPV6_IS_ADDR_MULTICAST(a) &&   \
     (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_GLOBAL))

/*
 * Definition of some useful macros to handle IP6 addresses
 */
#define IP6ADDR_ANY_INIT \
    {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IP6ADDR_LOOPBACK_INIT \
    {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IP6ADDR_NODELOCAL_ALLNODES_INIT \
    {{{ 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IP6ADDR_LINKLOCAL_ALLNODES_INIT \
    {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IP6ADDR_LINKLOCAL_ALLROUTERS_INIT \
    {{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 }}}

#define IP6_IS_NXTHDR_RECPROT(next)      \
        (next == IP_UDP_PROT)    || (next == IP_TCP_PROT)  || \
        (next == IP_ICMPV6_PROT) || (next == IP_RAW_PROT)  || \
        (next == IP_HELLO_PROT)  || (next == IP_OSPF_PROT) || \
        (next == IP6_MBLTY_PROT)

#define IP6_EXTRACT_IPV4_ADDR(ipv4_addr, ipv6_addr) \
        ipv4_addr = IP_ADDR(&ipv6_addr[12])


struct _IP6_MULTI
{
    IP6_MULTI               *ipm6_next;
    MULTI_DATA              ipm6_data;
    UINT16                  ipm6_num_query_src_list;
    INT16                   ipm6_msg_to_send;
    UINT8                   *ipm6_addr;
    UINT8  HUGE             *ipm6_query_src_list;
};


STATUS      IP6_Interpret(IP6LAYER *, DV_DEVICE_ENTRY *, NET_BUFFER *);
STATUS      IP6_Send(NET_BUFFER *, IP6_S_OPTIONS *, UINT8, RTAB6_ROUTE *,
                     INT32, const IP6_MULTI_OPTIONS *);
STATUS      IP6_Get_Opt(INT, INT, VOID *, INT *);
STATUS      IP6_Get_Socket_Unicast_Hops(INT, VOID *, INT *);
STATUS      IP6_Set_Opt(INT, INT, VOID *, INT);
IP6_MULTI   *IP6_Lookup_Multi(const UINT8 *, IP6_MULTI *);
IP6_MULTI   *IP6_Add_Multi(const UINT8 *, DV_DEVICE_ENTRY *, MULTI_SCK_STATE *);
IP6_MULTI   *IP6_Allocate_Multi(const UINT8 *, DV_DEVICE_ENTRY *, MULTI_SCK_STATE *);
STATUS      IP6_Delete_Multi(IP6_MULTI *);
VOID        IP6_Find_Route(RTAB6_ROUTE *);
DEV6_IF_ADDRESS *IP6_Find_Link_Local_Addr(const DV_DEVICE_ENTRY *);
DEV6_IF_ADDRESS *IP6_Find_Home_Addr(const DV_DEVICE_ENTRY *);
DEV6_IF_ADDRESS *IP6_Find_Care_Of_Addr(const DV_DEVICE_ENTRY *);
DEV6_IF_ADDRESS *IP6_Find_Global_Addr(const DV_DEVICE_ENTRY *);
STATUS      IP6_Set_Socket_Unicast_Hops(INT, INT16);
STATUS      IP6_Set_Multicast_Opt(INT, INT, VOID *, INT);
VOID        IP6_Initialize(VOID);
VOID        IP6_Join_Solicited_Node_Multi(DV_DEVICE_ENTRY *, const UINT8 *, UINT32);
VOID 		IP6_Handle_Solicited_Node_Multi_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID        IP6_Leave_Solicited_Node_Multi(DV_DEVICE_ENTRY *, const UINT8 *);
VOID        IP6_Create_IPv4_Mapped_Addr(UINT8 *, UINT32);
STATUS      IP6_Setup_Options(tx_ancillary_data *, tx_ancillary_data *, 
                              IP6_S_OPTIONS *, UINT8 *, RTAB6_ROUTE *, UINT8);
STATUS      IP6_Set_IPV6_RECVPKTINFO(INT, INT);
STATUS      IP6_Set_IPV6_RECVHOPLIMIT(INT, INT);
STATUS      IP6_Get_IPV6_RECVHOPLIMIT(INT, INT *, INT *);
STATUS      IP6_Get_IPV6_RECVPKTINFO(INT, INT *, INT *);
STATUS      IP6_Set_IPV6_PKTINFO(INT, in6_pktinfo *, INT);
STATUS      IP6_Get_IPV6_PKTINFO(INT, in6_pktinfo *);
STATUS      IP6_Set_IPV6_NEXTHOP(INT, const struct addr_struct *, INT);
STATUS      IP6_Get_IPV6_NEXTHOP(INT, struct addr_struct *, INT *);
STATUS      IP6_Get_IPV6_RECVTCLASS(INT, INT *, INT *);
STATUS      IP6_Set_IPV6_RECVTCLASS(INT, INT);
STATUS      IP6_Get_IPV6_TCLASS(INT, INT *, INT *);
STATUS      IP6_Set_IPV6_TCLASS(INT, INT);
STATUS      IP6_Get_IPV6_RECVRTHDR(INT, INT *, INT *);
STATUS      IP6_Set_IPV6_RECVRTHDR(INT, INT);
STATUS      IP6_Set_IPV6_RTHDR(INT, const struct ip6_rthdr *, INT);
STATUS      IP6_Set_IPV6_RECVHOPOPTS(INT, INT);
STATUS      IP6_Get_IPV6_RECVHOPOPTS(INT, INT *, INT *);
STATUS      IP6_Get_IPV6_RTHDR(INT, struct ip6_rthdr *, INT *);
STATUS      IP6_Set_IPV6_RECVDSTOPTS(INT, INT);
STATUS      IP6_Get_IPV6_RECVDSTOPTS(INT, INT *, INT *);
STATUS      IP6_Get_IPV6_HOPOPTS(INT, struct ip6_hbh *, INT *);
STATUS      IP6_Set_IPV6_HOPOPTS(INT, const struct ip6_hbh *, INT);
STATUS      IP6_Get_IPV6_DSTOPTS(INT, struct ip6_dest *, INT *);
STATUS      IP6_Set_IPV6_DSTOPTS(INT, const struct ip6_dest *, INT);
STATUS      IP6_Get_IPV6_RTHDRDSTOPTS(INT, struct ip6_dest *, INT *);
STATUS      IP6_Set_IPV6_RTHDRDSTOPTS(INT, const struct ip6_dest *, INT);
STATUS      IP6_Fragment(NET_BUFFER *, const IP6LAYER *, DV_DEVICE_ENTRY *, 
                         SCK6_SOCKADDR_IP *, RTAB6_ROUTE *);
STATUS      IP6_SFragment(NET_BUFFER *buf_ptr, const IP6LAYER *ip,
                    DV_DEVICE_ENTRY *int_face, RTAB6_ROUTE *ro,
                    NET_BUFFER  **work_buf_ret);
STATUS      IP6_Set_IPV6_MULTICAST_IF(INT, INT32);
STATUS      IP6_Get_IPV6_MULTICAST_IF(INT, INT32 *);
STATUS      IP6_Set_IPV6_MULTICAST_HOPS(INT, INT);
STATUS      IP6_Get_IPV6_MULTICAST_HOPS(INT, INT *);
STATUS      IP6_Set_IPV6_V6ONLY(INT, INT);
STATUS      IP6_Get_IPV6_V6ONLY(INT, INT *, INT *);
STATUS      IP6_Set_IPV6_CHECKSUM(INT, INT);
STATUS      IP6_Get_IPV6_CHECKSUM(INT, INT *, INT *);
STATUS      IP6_Set_IPV6_UNICAST_HOPS(INT, INT16);
STATUS      IP6_Get_IPV6_UNICAST_HOPS(INT, INT16 *);

STATUS      IP6_Forward(IP6LAYER *, NET_BUFFER *);

STATUS      IP6_Reassemble_Packet(IP6LAYER **, NET_BUFFER **, const UINT16*, UINT8 *);
NET_BUFFER* IP6_Reassembly(IP6_REASM HUGE *, IP6LAYER *, IP6_QUEUE_ELEMENT *, NET_BUFFER *, 
                           UINT16, UINT8 *, const UINT8 *);
VOID        IP6_Reassembly_Event(IP6_QUEUE_ELEMENT *);
VOID        IP6_Free_Queue_Element(IP6_QUEUE_ELEMENT *);
VOID        IP6_Remove_Frag(IP6_REASM HUGE *, IP6_REASM HUGE *, IP6_QUEUE_ELEMENT *);
VOID        IP6_Insert_Frag(IP6_REASM HUGE *, IP6_QUEUE_ELEMENT *);

IP6_POLICY_TABLE_ENTRY* IP6_Find_Policy_For_Address(UINT8 *);
STATUS                  IP6_Add_Policy(IP6_POLICY_ENTRY *);


/* IPv6 Packet Offsets */
#define IP6_XXX_OFFSET                 0
#define IP6_PAYLEN_OFFSET              4
#define IP6_NEXTHDR_OFFSET             6
#define IP6_HOPLMT_OFFSET              7
#define IP6_SRCADDR_OFFSET             8
#define IP6_DESTADDR_OFFSET            24

/* IPv6 Extension Header Offsets */
#define IP6_EXTHDR_NEXTHDR_OFFSET      0
#define IP6_EXTHDR_LENGTH_OFFSET       1

/* IPv6 Hop-by-Hop Option Header Offsets */
#define IP6_HOPBYHOP_OPTS_OFFSET       2

/* IPv6 Options Offset */
#define IP6_TYPE_OFFSET                0
#define IP6_LENGTH_OFFSET              1
#define IP6_DATA_OFFSET                2

/* IPv6 Routing Option Header Offsets */
#define IP6_ROUTING_TYPE_OFFSET        2
#define IP6_ROUTING_SEGLEFT_OFFSET     3
#define IP6_ROUTING_DATA_OFFSET        4

/* IPv6 Type 2 Routing Option Header Offsets */
#define IP6_ROUTING_TYPE2_RSVD_OFFSET        4
#define IP6_ROUTING_TYPE2_HOME_ADDR_OFFSET   8

/* IPv6 Type 2 Routing Header Constants */
#define IP6_ROUTING_TYPE2_LENGTH       2
#define IP6_ROUTING_TYPE2_SEG_LEFT     1
#define IP6_ROUTING_TYPE2_RSVD         0x00000000
#define IP6_ROUTING_TYPE2_HDR_LEN      24

/* IPv6 Fragment Option Header Offsets */
#define IP6_FRAGMENT_RES1_OFFSET       1
#define IP6_FRAGMENT_FRGOFFSET_OFFSET  2
#define IP6_FRAGMENT_ID_OFFSET         4

/* IPv6 Destination Option Header Offsets */
#define IP6_DEST_OPTS_OFFSET           2

/* HOP by HOP Options defines */
#define IP6_MLD_ROUTER_ALERT           0
#define IP6_PADN_OPTION                1

/* Routing Header defines */
#define IPV6_RTHDR_TYPE_0              0
#define IPV6_RTHDR_TYPE_2              2

/* IPV6 Reassembly defines */
#define IP6F_MFF_OFFSET                 0
#define IP6F_BUF_OFFSET                 1
#define IP6F_UNFRAGLEN_OFFSET           2
#define IP6F_PAYLOAD_OFFSET             4
#define IP6F_FRGOFFSET_OFFSET           6
#define IP6F_HDR_OFFSET                 8
#define IP6F_NEXT_OFFSET                12
#define IP6_REASM_MAX_SIZE              65535UL

/* Home Address Destination option offsets */
#define IP6_DESTOPT_HOME_ADDR_TYPE_OFFSET     0
#define IP6_DESTOPT_HOME_ADDR_LEN_OFFSET      1
#define IP6_DESTOPT_HOME_ADDR_OFFSET          2

/* Home Address Destination option length */
#define IP6_DEST_OPT_HOME_ADDR_LENGTH   20

/* Padding for Home Address Destination option */
#define IP6_DEST_OPT_HOME_ADDR_PAD_LENGTH   4
#define IP6_MBLIP_ALT_CAREOF_ADDR_PAD_LEN   2
#define IP6_MBLIP_BA_PAD_LEN                4
#define IP6_TYPE_2_RTHDR_LEN                22

/* Nonce Indices Option constants */
#define IP6_NONCE_LEN                   4

/* Home Test Init constants */
#define IP6_MBLIP_HOTI_COOKIE_LEN       8

/* Care-of Test Init constants */
#define IP6_MBLIP_COTI_COOKIE_LEN       8

/* Mobility Header offsets */
#define IP6_MBLIP_PAYLOADPROTO_OFFSET   0
#define IP6_MBLIP_HDRLEN_OFFSET         1
#define IP6_MBLIP_MHTYPE_OFFSET         2
#define IP6_MBLIP_RSVD_OFFSET           3
#define IP6_MBLIP_CHCKSM_OFFSET         4
#define IP6_MBLIP_MSGDATA_OFFSET        6

/* Mobility Header type constants */
#define IP6_MBLIP_BNDNG_RFRSH           0
#define IP6_MBLIP_HOME_TEST_INIT        1
#define IP6_MBLIP_CAREOF_TEST_INIT      2
#define IP6_MBLIP_HOME_TEST             3
#define IP6_MBLIP_CAREOF_TEST           4
#define IP6_MBLIP_BNDNG_UPDATE          5
#define IP6_MBLIP_BNDNG_ACK             6
#define IP6_MBLIP_BNDNG_ERROR           7

/* Mobility Header Lengths */
#define IP6_MBLIP_MBLTYHDR_LEN          6
#define IP6_MBLIP_BU_LEN                6
#define IP6_MBLIP_BA_LEN                6
#define IP6_MBLIP_CAREOF_ADDR_OPT_LEN   18
#define IP6_MBLIP_NONCE_INDICES_OPT_LEN 6
#define IP6_MBLIP_BNDNG_AUTH_OPT_LEN    14
#define IP6_MBLIP_HOTI_LEN              16
#define IP6_MBLIP_COTI_LEN              16
#define IP6_MBLIP_HOT_LEN               24
#define IP6_MBLIP_COT_LEN               24
#define IP6_MBLIP_BE_LEN                24
#define IP6_MBLIP_BRR_LEN               8

/* Mobility Option offsets */
#define IP6_MBLIP_OPT_TYPE_OFFSET       0
#define IP6_MBLIP_OPT_LEN_OFFSET        1
#define IP6_MBLIP_OPT_DATA_OFFSET       2
#define IP6_MBLIP_OPT_NONCE_HOME_OFFSET 2
#define IP6_MBLIP_OPT_NONCE_CARE_OFFSET 4

/* Mobility Option type constants */
#define IP6_MBLIP_OPT_PAD1              0
#define IP6_MBLIP_OPT_PADN              1
#define IP6_MBLIP_OPT_BNDNG_RFRSH       2
#define IP6_MBLIP_OPT_ALT_CAREOF_ADDR   3
#define IP6_MBLIP_OPT_NONCE_INDICES     4
#define IP6_MBLIP_OPT_BNDNG_AUTH_DATA   5

/* Binding Update offsets */
#define IP6_MBLIP_BU_SEQ_OFFSET         6
#define IP6_MBLIP_BU_FLAG_OFFSET        8
#define IP6_MBLIP_BU_LIFETIME_OFFSET    10
#define IP6_MBLIP_BU_MBLTY_OPT_OFFSET   12

/* Binding Acknowledgement offsets */
#define IP6_MBLIP_BA_STATUS_OFFSET      6
#define IP6_MBLIP_BA_FLAG_OFFSET        7
#define IP6_MBLIP_BA_SEQ_OFFSET         8
#define IP6_MBLIP_BA_LIFETIME_OFFSET    10
#define IP6_MBLIP_BA_MBLTY_OPT_OFFSET   12

/* Binding Error offsets */
#define IP6_MBLIP_BE_STATUS_OFFSET      6
#define IP6_MBLIP_BE_RSVD_OFFSET        7
#define IP6_MBLIP_BE_HOMEADDR_OFFSET    8

/* Home Test Init offsets */
#define IP6_MBLIP_HOTI_RSVD_OFFSET      6
#define IP6_MBLIP_HOTI_COOKIE_OFFSET    8

/* Care-of Test Init offsets */
#define IP6_MBLIP_COTI_RSVD_OFFSET      6
#define IP6_MBLIP_COTI_COOKIE_OFFSET    8

/* Home Test offsets */
#define IP6_MBLIP_HOT_NONCE_OFFSET      6
#define IP6_MBLIP_HOT_COOKIE_OFFSET     8
#define IP6_MBLIP_HOT_KEYGEN_OFFSET     16

/* Care-of Test offsets */
#define IP6_MBLIP_COT_NONCE_OFFSET      6
#define IP6_MBLIP_COT_COOKIE_OFFSET     8
#define IP6_MBLIP_COT_KEYGEN_OFFSET     16

/* Binding Refresh Request offsets. */
#define IP6_MBLIP_BRR_RSVD_OFFSET       6

/* This function computes the value that should be placed in the length
 * field of a padN option.  N is the number of bytes of padding that need
 * to be inserted into the header.
 */
#define IP6_PADN_OPTION_LENGTH(N)   \
    (N - 2)

/* Option types and related macros */
#define IP6OPT_PAD1           0x00  /* 00 0 00000 */
#define IP6OPT_PADN           0x01  /* 00 0 00001 */
#define IP6OPT_JUMBO          0xc2  /* 11 0 00010 */
#define IP6OPT_NSAP_ADDR      0xc3  /* 11 0 00011 */
#define IP6OPT_TUNNEL_LIMIT   0x04  /* 00 0 00100 */
#define IP6OPT_ROUTER_ALERT   0x05  /* 00 0 00101 */
#define IP6OPT_HOME_ADDR      0xc9  /* 11 0 01001 */

#define IP6OPT_RTALERT_LEN      4
#define IP6OPT_RTALERT_MLD      0   /* Datagram contains an MLD message */
#define IP6OPT_RTALERT_RSVP     1   /* Datagram contains an RSVP message */
#define IP6OPT_RTALERT_ACTNET   2   /* contains an Active Networks msg */
#define IP6OPT_MINLEN           2

#define IP6OPT_TYPE(o)        ((o) & 0xc0)
#define IP6OPT_TYPE_SKIP      0x00
#define IP6OPT_TYPE_DISCARD   0x40
#define IP6OPT_TYPE_FORCEICMP 0x80
#define IP6OPT_TYPE_ICMP      0xc0
#define IP6OPT_MUTABLE        0x20

#define IP6_MF                0x01

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IPV6_H */

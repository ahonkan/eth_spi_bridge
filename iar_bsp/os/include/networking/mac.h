/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       mac.h                                                    
*
*   DESCRIPTION
*
*       This file contains the macros, data structures and function
*       definitions used in the file MAC.C.
*
*   DATA STRUCTURES
*
*       mac_type_s
*       mac_prot_s
*       mac_coll_s
*       mac_hdr_s
*       mac_stat_s
*       mac_info_s
*       mac_perf_s
*
*   DEPENDENCIES
*
*       externs.h
*       rip.h
*       prot.h
*
************************************************************************/

#ifndef MAC_H
#define MAC_H

#include "networking/externs.h"
#include "networking/rip.h"
#include "networking/prot.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Macros for driver types */
#define MAC_TYPE_OTHER              MIB2_MAC_TYPE_OTHER
#define MAC_TYPE_REGULAR_1822       MIB2_MAC_TYPE_REGULAR_1822
#define MAC_TYPE_HDH_1822           MIB2_MAC_TYPE_HDH_1822
#define MAC_TYPE_DDN_X25            MIB2_MAC_TYPE_DDN_X25
#define MAC_TYPE_RFC877_X25         MIB2_MAC_TYPE_RFC877_X25
#define MAC_TYPE_ETHERNET_CSMACD    MIB2_MAC_TYPE_ETHERNET_CSMACD
#define MAC_TYPE_88023_CSMACD       MIB2_MAC_TYPE_88023_CSMACD
#define MAC_TYPE_88024_TOKENBUS     MIB2_MAC_TYPE_88024_TOKENBUS
#define MAC_TYPE_88025_TOKENRING    MIB2_MAC_TYPE_88025_TOKENRING
#define MAC_TYPE_88026_MAN          MIB2_MAC_TYPE_88026_MAN
#define MAC_TYPE_SOFT_LOOPBACK      MIB2_MAC_TYPE_SOFT_LOOPBACK

#define MAC_ETHER_ADDR_LEN          6

typedef struct mac_type_s           mac_type_t;
typedef struct mac_prot_s           mac_prot_t;
typedef struct mac_coll_s           mac_coll_t;
typedef struct mac_hdr_s            mac_hdr_t;
typedef struct mac_stat_s           mac_stat_t;
typedef struct mac_info_s           mac_info_t;
typedef struct mac_perf_s           mac_perf_t;

struct mac_type_s
{
    link_t              *(*Encode)(link_t *, mac_hdr_t *);
    link_t              *(*Decode)(link_t *, mac_hdr_t *);
    struct mac_type_s   *next;
    UINT16              type;

    UINT8           snmp_pad[2];
};


struct mac_prot_s
{
    struct mac_prot_s   *next;
    UINT16              type;

    UINT8               snmp_pad[2];
};


struct mac_coll_s
{
	BOOLEAN     (*Rcve)(struct mac_coll_s *, prot_pkt_t *);
    UINT32      ifindex;
    VOID        *specific;
    mac_coll_t  *next;
};

struct mac_hdr_s
{
    UINT8       *src;
    UINT8       *dst;
    BOOLEAN     broadcast;
    UINT16      type;

    UINT8       snmp_pad;
};

struct mac_stat_s
{
    UINT32         inPkts;
    UINT32         inOctets;
    UINT32         inUcastPkts;
    UINT32         inNUcastPkts;
    UINT32         inDiscards;
    UINT32         inErrors;
    UINT32         inUnknownProtos;
    UINT32         outPkts;
    UINT32         outOctets;
    UINT32         outUcastPkts;
    UINT32         outNUcastPkts;
    UINT32         outDiscards;
    UINT32         outErrors;
    UINT32         outQLen;
    UINT32         ifOverflows;
    UINT32         LostPkts;
    UINT32         Octets;
    UINT32         Pkts;
    UINT32         BroadcastPkts;
    UINT32         MulticastPkts;
    UINT32         CRCAlignErrors;
    UINT32         UndersizePkts;
    UINT32         OversizePkts;
    UINT32         Fragments;
    UINT32         Jabbers;
    UINT32         Collisions;
    UINT32         BufferedPkts;
    UINT32         DiscardedPkts;
    UINT32         StackedPkts;
    UINT32         TruncatedPkts;
};

struct mac_info_s
{
    rip_t           *rip;
    UINT32          time;
    BOOLEAN         promiscuous;
    UINT16          length;
    UINT16          copied;
    UINT16          status;

    UINT8           snmp_pad;
};

struct mac_perf_s
{
    UINT32          pkts;
    UINT32          octets;
    UINT32          timeTotal;
    UINT32          timeMin;
    UINT32          timeMax;
    BOOLEAN         on;

    UINT8           snmp_pad[3];
};

BOOLEAN            MacInit(VOID);
BOOLEAN            MacCollRegister(mac_coll_t *);
BOOLEAN            MacCollRemove(mac_coll_t *);
UINT16             MacIfaceCount(VOID);
BOOLEAN            MacIfaceCheck(VOID);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       rip2.h
*
*   COMPONENT
*
*       RIP2 - Routing Internet Protocol  V2.0
*
*   DESCRIPTION
*
*       Holds the defines for the RIP2 protocol.
*
*   DATA STRUCTURES
*
*       RIP2_PACKET
*       RIP2_HEADER
*       RIP2_ENTRY
*       RIP2_AUTH_ENTRY
*       RIP2_AUTHENTICATION
*       RIP2_STRUCT
*       RIP2_LIST_NODE
*       RIP2_LIST_STRUCT
*
*   DEPENDENCIES
*
*      socketd.h
*      target.h
*
*************************************************************************/

#ifndef _RIP2_H_
#define _RIP2_H_

#include "networking/socketd.h"
#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define RIP_PORT        520

#define RIP_PKT_SIZE    20
#define RT_INFINITY     16
#define LOOPBACK_ADDR       0x7f000000UL
#define CLASSA_BROADCAST    0x00ffffffUL
#define CLASSB_BROADCAST    0x0000ffffUL
#define CLASSC_BROADCAST    0x000000ffUL

/* Following defines have these meanings. */
/* SEND_NONE   - No RIP1 or 2 messages are ever broadcast or multicast */
/* SEND_RIP1   - Only RIP1 messages are ever broadcast, NO multicast */
/* SEND_RIP2   - Only RIP2 messages are ever broadcast, NO multicast */
/* SEND_BOTH   - Both RIP1 & RIP2 messages are broadcast, NO multicast */
/* SEND_MULTI  - Only RIP-2 messages are ever multicast, NO broadcast */
#define SEND_NONE       0x00
#define SEND_RIP1       0x01
#define SEND_RIP2       0x02

#define SEND_BOTH           (SEND_RIP1 | SEND_RIP2)
#define SEND_RIP1_COMPAT    SEND_BOTH
#define SEND_MULTI          SEND_RIP2

/* RECV_RIP1   - Only process RIP1 packets. */
/* RECV_RIP2   - Only process RIP2 packets. */
/* RECV_BOTH   - Process both RIP1 and RIP2 packets */
#define RECV_NONE           0x00
#define RECV_RIP1           0x01
#define RECV_RIP2           0x02

#define RECV_BOTH           (RECV_RIP1 | RECV_RIP2)
#define RECV_RIP1_COMPAT    RECV_BOTH

#define RIP2_REQUEST    1
#define RIP2_RESPONSE   2
#define RIP2_TRACEON    3   /* obsolete, to be ignored */
#define RIP2_TRACEOFF   4   /* obsolete, to be ignored */
#define RIP2_RESERVED   5

#define RIP1_VERSION    1
#define RIP2_VERSION    2

#define RIP2_FFFF       0xFFFF

struct rip2_packet {
    UINT8 command;
    UINT8 version;          /* as of RFC1722 this can be 1 or 2 */
    UINT16 unused;          /* not used in RIPv1 RFC1058 */
    UINT16 af_id;           /* Address Family Identifier */
    UINT16 routetag;        /* not used in RIPv1 RFC1058 */
    UINT8 ip_addr[4];       /* IP Address */
    UINT8 submask[4];       /* not used in RIPv1 RFC1058 */
    UINT8 nexthop[4];       /* not used in RIPv1 RFC1058 */
    UINT32 metric;          /* must be a value of 1 to 16 */
};
typedef struct rip2_packet RIP2_PACKET;

struct rip2_header {
    UINT8 command;
    UINT8 version;          /* as of RFC1722 this can be 1 or 2 */
    INT16 unused;
};

typedef struct rip2_header RIP2_HEADER;

struct rip2_entry {
    UINT16 af_id;           /* Address Family Identifier */
    UINT16 routetag;        /* not used in RIPv1 RFC1058 */
    UINT8 ip_addr[4];       /* IP address */
    UINT8 submask[4];       /* not used in RIPv1 RFC1058 */
    UINT8 nexthop[4];       /* not used in RIPv1 RFC1058 */
    UINT32 metric;          /* must be a value of 1 to 16 */
};
typedef struct rip2_entry RIP2_ENTRY;

struct rip2_auth_entry {
    UINT16 af_id;           /* will always be 0xFFFF for this type packet */
    UINT16 authtype;        /* not used in RIPv1 RFC1058 */
    UINT8 auth[16];         /* authentication entry */
};
typedef struct rip2_auth_entry RIP2_AUTH_ENTRY;

struct rip2_authentication {
    UINT16 authtype;        /* must be set to 2 */
    UINT8 auth[16];         /* authentication entry */
    UINT8   padding[2];
    /* the auth field must be left justified and zero padded to 16 octets */
};
typedef struct rip2_authentication RIP2_AUTH;

/* This is the RIP2 initialization structure. Applications will fill in one of
   these for each device that RIP should be used with. */
typedef struct _RIP2_STRUCT
{
    CHAR            *rip2_device_name;
    UINT32          rip2_metric;
    INT8            rip2_sendmode;
    INT8            rip2_recvmode;
    UINT8           pad1;
    UINT8           pad2;
} RIP2_STRUCT;

/* There will be a list of these that includes one for each device that RIP is
   used with. */
typedef struct _RIP2_DEVICE
{
    UINT8   rip2_dev_metric;
    UINT8   rip2_dev_padN[3];
    UINT32  rip2_dev_index;
    UINT32  rip2_dev_mtu;
} RIP2_DEVICE;

/* There will be a list of these that includes one for each device that RIPng
 * is used with.
 */
typedef struct _RIP2_LIST_NODE
{
    struct _RIP2_LIST_NODE      *r2_next;
    struct _RIP2_LIST_NODE      *r2_prev;
    RIP2_DEVICE                 r2_device;
    INT8                        r2_sendmode;
    INT8                        r2_recvmode;
    UINT8                       pad1;
    UINT8                       pad2;
} RIP2_LIST_NODE;

typedef struct _RIP2_LIST_STRUCT
{
    RIP2_LIST_NODE      *r2_head;
    RIP2_LIST_NODE      *r2_tail;
} RIP2_LIST_STRUCT;

/* The function prototypes known to the outside world. */
STATUS NU_Rip2_Initialize(const RIP2_STRUCT *rip2, INT num);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* RIP2_H */

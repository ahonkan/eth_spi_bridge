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
*       rtab.h
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       Holds the defines for routing.
*
*   DATA STRUCTURES
*
*       SUBNET_MASK_LIST
*       ROUTE_ENTRY_LIST
*       ROUTE_NODE
*       ROUTE_ENTRY_PARMS
*       ROUTE_ENTRY
*       RTAB4_ROUTE_ENTRY
*       DEST_ADDR
*       UPDATED_ROUTE_NODE
*       RTAB_ROUTE_PARMS
*       _RTAB_ROUTE
*
*   DEPENDENCIES
*
*       socketd.h
*       dev.h
*       rip2.h
*
*************************************************************************/

#ifndef _RTAB_H
#define _RTAB_H

#include "networking/sockdefs.h"
#include "networking/dev.h"
#include "networking/rip2.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define MORE        0       /* RIGHT */
#define LESS        1       /* LEFT */
#define MIDDLE      2
#define CMP_ERROR   3

#define NULL_ROUTE_NODE     (ROUTE_NODE *)0

struct subnet_mask_list
{
    struct route_node   *submask_head;
    struct route_node   *submask_tail;
};

struct route_entry_list
{
    VOID    *rt_entry_head;
    VOID    *rt_entry_tail;
};

struct route_node
{
    struct route_node       *rt_next;
    struct route_node       *rt_previous;
    struct route_node       *rt_submask_list_parent;
    struct route_node       *rt_parent;
    struct route_node       *rt_child[2];
    struct subnet_mask_list rt_submask_list;
    struct route_entry_list rt_route_entry_list;
    UINT8                   rt_ip_addr[MAX_ADDRESS_SIZE];
    UINT8                   rt_submask[4];
    UINT8                   rt_bit_index;
    UINT8                   rt_submask_length;
    UINT8                   rt_padding[2];
};

struct route_entry_parms
{
    INT16               rt_parm_refcnt;      /* # held references */
    UINT16              rt_parm_routetag;
    UINT32              rt_parm_flags;       /* Up/Down, Host/Net */
    UINT32              rt_parm_use;         /* # of packets sent over this route */
    UINT32              rt_parm_clock;       /* number of clock ticks of last update */
    UINT32              rt_parm_metric;      /* must be a value of 1 to 16 */
    DV_DEVICE_ENTRY     *rt_parm_device;     /* pointer to the interface structure */
    UINT32              rt_parm_path_mtu;
#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)
    UINT32              rt_parm_pmtu_timestamp;
#endif
};

struct route_entry
{
    VOID                        *rt_flink;
    VOID                        *rt_blink;
    struct route_entry_parms    rt_entry_parms;
    struct route_node           *rt_route_node;
};

typedef struct subnet_mask_list     SUBNET_MASK_LIST;
typedef struct route_node           ROUTE_NODE;
typedef struct route_entry          ROUTE_ENTRY;

#if (INCLUDE_IPV4 == NU_TRUE)
typedef struct rtab4_route_entry    RTAB4_ROUTE_ENTRY;
#endif

/* The previous members of the route data structure must be defined for
 * backward compatibility.
 */
#define rt_flags            rt_entry_parms.rt_parm_flags
#define rt_refcnt           rt_entry_parms.rt_parm_refcnt
#define rt_use              rt_entry_parms.rt_parm_use
#define rt_clock            rt_entry_parms.rt_parm_clock
#define rt_lastsent         rt_entry_parms.rt_parm_lastsent
#define rt_metric           rt_entry_parms.rt_parm_metric
#define rt_device           rt_entry_parms.rt_parm_device
#define rt_path_mtu         rt_entry_parms.rt_parm_path_mtu
#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)
#define rt_pmtu_timestamp   rt_entry_parms.rt_parm_pmtu_timestamp
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
#define rt_gateway          rt_gateway_v4
#endif
#define rt_list_head        rt_route_entry_list.rt_entry_head

struct destination_addr {
    UINT8 ip_addr[4];
    UINT8 ip_mask[4];
};

typedef struct destination_addr DEST_ADDR;

#if (INCLUDE_IPV4 == NU_TRUE)
struct _RTAB_Route
{
    RTAB4_ROUTE_ENTRY   *rt_route;
    SCK_SOCKADDR_IP     rt_ip_dest;
};
#endif

typedef struct updated_route_node UPDATED_ROUTE_NODE;

struct updated_route_node
{
    UINT8   *urt_dest;
    UINT8   *urt_gateway;
    INT     urt_prefix_length;

    union
    {
        CHAR    *urt_dev_name;
        INT32   urt_dev_index;
    }urt_dev;

    INT32   urt_flags;

    union
    {
        INT32   urt4_metric;
        INT16   urt6_metric;
    } urt_metric;

    INT32   urt_age;
    INT32   urt_routetag;
    INT32   urt_path_mtu;
};

struct rtab_route_parms
{
    INT16       rt_family;
    UINT8       rt_byte_ip_len;
    UINT8       rt_bit_ip_len;
    ROUTE_NODE  **rt_root_node;
    UINT8       (*rt_determine_matching_prefix)(const UINT8*, const UINT8*, const UINT8*, UINT8);
    INT         (*rt_insert_route_entry)(ROUTE_NODE*, const ROUTE_NODE*);
    ROUTE_NODE  *(*rt_setup_new_node)(const ROUTE_NODE*);
};

typedef struct rtab_route_parms RTAB_ROUTE_PARMS;

/* Route Flags */
#define RT_UP          0x1         /* route usable */
#define RT_GATEWAY     0x2         /* destination is a gateway */
#define RT_HOST        0x4         /* host entry (net otherwise) */
#define RT_REJECT      0x8         /* host or net unreachable */
#define RT_DYNAMIC     0x10        /* created dynamically (by redirect) */
#define RT_MODIFIED    0x20        /* modified dynamically (by redirect) */
#define RT_DONE        0x40        /* message confirmed */
#define RT_MASK        0x80        /* subnet mask present */
#define RT_CLONING     0x100       /* generate new routes on use */
#define RT_XRESOLVE    0x200       /* external daemon resolves name */
#define RT_LLINFO      0x400       /* generated by ARP or ESIS */
#define RT_STATIC      0x800       /* manually added */
#define RT_BLACKHOLE   0x1000      /* just discard pkts (during updates) */
#define RT_SILENT      0x2000      /* this route is kept silent from routing protocol info.
                                      such as the loopback device's route */
#define RT_USED        0x4000      /* This entry in the routing table is */
                                   /* being used. */
#define RT_PROTO1      0x8000      /* protocol specific routing flag */

#define RT_LOCAL       0x10000UL   /* The route was added manually */
#define RT_NETMGMT     0x20000UL   /* The route was added by a network mgmt protocol */
#define RT_ICMP        0x40000UL   /* The route was obtained via ICMP */
#define RT_RIP2        0x80000UL   /* The route was obtained via RIP2 */

#define RT_PREFERRED   0x100000UL  /* Indicates the preferred route from a group
                                      of routes to the same destination. */
#define RT_CHANGED     0x200000UL   /* RIP-2 flag to indicate a route has changed */
#define RT_RIPNG       0x400000UL   /* The route was obtained via RIPng */
#define RT_STOP_PMTU   0x800000UL   /* Do not perform PMTU on this route */
#define RT_NOT_GATEWAY 0x1000000UL  /* Not a gateway route - used by SNMP only */

/* Route Find Flags */
#define RT_OVERRIDE_METRIC      0x1     /* Return the route regardless of the metric */
#define RT_OVERRIDE_RT_STATE    0x2     /* Return the route regardless of the state of the route */
#define RT_OVERRIDE_DV_STATE    0x4     /* Return the route regardless of the state of the device */
#define RT_HOST_MATCH           0x8     /* Return only a host route match */
#define RT_BEST_METRIC          0x10    /* Return the route with the best metric */

/* The function prototypes known to the outside world. */
STATUS      RTAB_Delete_Node(ROUTE_NODE *, const RTAB_ROUTE_PARMS *);
ROUTE_ENTRY *RTAB_Find_Route_Entry(const UINT8 *, const RTAB_ROUTE_PARMS *, INT32);
VOID        RTAB_Free(ROUTE_ENTRY *, INT16);
STATUS      RTAB_Insert_Node(const ROUTE_NODE *, const RTAB_ROUTE_PARMS *);
STATUS      RTAB_Delete_Route_Entry(ROUTE_NODE *, ROUTE_ENTRY *, const RTAB_ROUTE_PARMS *);
ROUTE_NODE  *RTAB_Find_Closest_Node(const UINT8 *, UINT8 *, const RTAB_ROUTE_PARMS *);
UINT8       RTAB_Determine_Branch(UINT8, const UINT8 *, const RTAB_ROUTE_PARMS *);
VOID        RTAB_Establish_Subnet_Mask_Links(ROUTE_NODE *, const RTAB_ROUTE_PARMS *);
ROUTE_NODE  *RTAB_Find_Route_For_Device(const DV_DEVICE_ENTRY *, ROUTE_NODE *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* RTAB_H */



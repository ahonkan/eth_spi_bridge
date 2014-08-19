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
*       dev6.h
*
*   DESCRIPTION
*
*       This file contains the data structures and defines necessary
*       to support the multiple device driver interface for IPv6.
*
*   DATA STRUCTURES
*
*       _dev6_if_address
*       _DV6_REQ
*
*   DEPENDENCIES
*
*       netevent.h
*       mem_defs.h
*       net.h
*       rtab.h
*       net.h
*       ip6_mib.h
*
*************************************************************************/

#ifndef IPV6_DEV
#define IPV6_DEV

#include "networking/netevent.h"
#include "networking/mem_defs.h"
#include "networking/net.h"
#include "networking/rtab.h"
#include "networking/net.h"

#include "networking/net6_cfg.h"
#include "networking/ip6_mib.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#ifdef NET_5_4

/* Due to circular references, this structure is defined in dev.h for
 * NET 5.3 and lower, but added here for forward compatibility with
 * future versions of Nucleus NET.
 */
struct _DEV_MULTI_MLD_LIST
{
    MULTI_DEV_STATE *head;
    MULTI_DEV_STATE *tail;
};

#endif

/* Address states */
#define DV6_ANYCAST     0x1         /* anycast address */
#define DV6_TENTATIVE   0x2         /* tentative address */
#define DV6_DUPLICATED  0x4         /* DAD detected duplicate */
#define DV6_DETACHED    0x8         /* may be detached from the link */
#define DV6_DEPRECATED  0x10        /* deprecated address */

/* Address flags */
#define ADDR6_HOME_ADDR         0x1  /* Home Address */
#define ADDR6_NO_DAD            0x2  /* Do not perform DAD on this address */
#define ADDR6_CAREOF_ADDR       0x4  /* Primary Care-of Address */
#define ADDR6_DHCP_ADDR         0x8  /* Address obtained via DHCPv6 */
#define ADDR6_DHCP_DECLINE      0x10 /* Decline the address with DHCPv6 */
#define ADDR6_DHCP_RELEASE      0x20 /* Release the address with DHCPv6 */
#define ADDR6_STATELESS_AUTO    0x40 /* The address was configured via stateless address autoconfiguration. */
#define ADDR6_DELAY_MLD         0x80 /* Delay joining the solicited-node multicast group. */

/* Mobile IPv6 flags */
#define MBLIP6_HOME_LINK        0x1 /* The node is on the home link */
#define MBLIP6_MOVING_LINKS     0x2 /* The node is in the process of moving links. */

/* IPv6 router configuration flags */
#define DV6_REM_FLGS    0
#define DV6_ADD_FLGS    1
#define DV6_IGN_FLGS    2

/* IPv6-specific interface flags set in the IPv6 specific portion of the
 * interface structure.
 */
#define DV6_DISABLE_ADDR_CONFIG 0x1
#define DV6_NO_DHCP_RELEASE     0x2

/* Macros for configuring IPv6 interface features. */
#define IP6_AUTO_ADDR_CONFIG    0

struct _dev6_if_address
{
    DEV6_IF_ADDRESS     *dev6_next;
    DEV6_IF_ADDRESS     *dev6_previous;
    UINT8               dev6_ip_addr[16];           /* address of interface */
    UINT8               dev6_prefix_length;
    UINT8               padN[3];
    UINT32              dev6_preferred_lifetime;
    UINT32              dev6_valid_lifetime;
    UINT32              dev6_timestamp;
    UINT32              dev6_addr_state;
    UINT32              dev6_addr_flags;
    UINT32              dev6_id;
    DV_DEVICE_ENTRY     *dev6_device;

#if (INCLUDE_DHCP6 == NU_TRUE)
   /* These fields are used only if the IP address is obtained via DHCP. */
    UINT32              dev6_dhcp_t1;
    UINT32              dev6_dhcp_t2;
    UINT32              dev6_dhcp_rebind;            /* Rebinding time for the IP addr. */
    UINT32              dev6_dhcp_state;             /* The DHCP state. */
    UINT32              dev6_dhcp_server_addr;       /* The DHCP server's IP address. */
    INT32               dev6_dhcp_opts_length;       /* Length of the options */
    CHAR                dev6_dhcp_options;           /* DHCP Options */
#endif /* INCLUDE_DHCP */
};

typedef struct _dev6_if_address_list
{
    DEV6_IF_ADDRESS *dv_head;
    DEV6_IF_ADDRESS *dv_tail;

    UINT8           dev6_dst_ip_addr[16];       /* other end of p-to-p link */

} DEV6_IF_ADDRESS_LIST;

typedef struct _dev6_prfx_entry
{
    UINT8                   prfx_prefix[16];
    INT                     prfx_length;
    UINT32                  prfx_adv_valid_lifetime;
    UINT32                  prfx_adv_pref_lifetime;
    UINT32                  prfx_flags;
} DEV6_PRFX_ENTRY;

typedef struct _dev6_rtr_opts
{
    INT             rtr_MaxRtrAdvInterval;
    INT             rtr_MinRtrAdvInterval;
    INT             rtr_AdvCurHopLimit;
    INT             rtr_AdvDefaultLifetime;
    INT32           rtr_AdvLinkMTU;
    INT32           rtr_AdvReachableTime;
    INT32           rtr_AdvRetransTimer;
    DEV6_PRFX_ENTRY *rtr_AdvPrefixList;
} DEV6_RTR_OPTS;

typedef struct mbl_ip6_probe_router
{
    struct mbl_ip6_probe_router *mbl_ip6_rtr_next;
    UINT8                       mbl_ip6_rtr_addr[16];
    UINT8                       mbl_ip6_rtr_id;
    UINT8                       mbl_ip6_rtr_trans;
    UINT8                       padN[2];
} MBL_IP6_PROBE_ROUTER;

typedef struct mbl_ip6_probe_router_list
{
    MBL_IP6_PROBE_ROUTER    *probe_head;
    MBL_IP6_PROBE_ROUTER    *probe_tail;
} MBL_IP6_PROBE_ROUTER_LIST;

#ifdef NET_5_4

typedef struct _dev6_ip6_struct
{
#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)
    IP6_MIB_INTERFACE        *dev6_ip6_interface_mib;
#endif

#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)
    UINT32                   dev6_ip6_icmp_mib[IP6_ICMP_MIB_COUNTERS];
#endif

    UINT32                   dev6_ip6_flags;        /* Flags relative to IPv6
                                                     * functionality on the
                                                     * interface.
                                                     */
    UINT32                   dev6_ip6_link_mtu;     /* The LinkMTU as updated
                                                     * by Router Advertisements.
                                                     * This is the initial PMTU
                                                     * value of a path.
                                                     */
    UINT32                   dev6_ip6_default_mtu;  /* The default LinkMTU
                                                     * specified in the link
                                                     * type specific document.
                                                     */

    UINT32                   dev6_ip6_last_error_msg_timestamp;

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

    UINT32                   dev6_ip6_AdvLinkMTU;
    UINT32                   dev6_ip6_AdvReachableTime;
    UINT32                   dev6_ip6_AdvRetransTimer;
    UINT32                   dev6_ip6_next_radv_time;
    UINT32                   dev6_ip6_last_radv_time;

    UINT16                   dev6_ip6_MaxRtrAdvInterval;
    UINT16                   dev6_ip6_MinRtrAdvInterval;
    UINT16                   dev6_ip6_AdvDefaultLifetime;

    UINT8                    dev6_ip6_init_max_RtrAdv;
    UINT8                    dev6_ip6_AdvCurHopLimit;

#endif

    UINT32                   dev6_ip6_base_reachable_time;
    UINT32                   dev6_ip6_retrans_timer;
    UINT32                   dev6_ip6_reachable_time;

    UINT8                    dev6_ip6_interface_id[8];/* Interface ID */
    UINT8                    dev6_ip6_rtr_sols;
    UINT8                    dev6_ip6_cur_hop_limit;
    UINT8                    dev6_ip6_interface_id_length;
    UINT8                    dev6_ip6_dup_addr_detect_trans;

    INT16                    dev6_ip6_nc_entries;
    UINT8                    dev_pad_2[2];

    IP6_PREFIX_LIST*         dev6_ip6_prefix_list;
    DEV6_IF_ADDRESS_LIST     dev6_ip6_addr_list;      /* IPv6 Address info */

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    IP6_MULTI                *dev6_ip6_multiaddrs;    /* IPv6 Multicast Addr info */
    MULTI_DEV_MLD_LIST       dev6_ip6_mld_state_list;
    UNSIGNED                 dev6_ip6_mld_compat_mode; /* Indicates the MLD
                                                        * version of the device */
    UNSIGNED                 dev6_ip6_mld_compat_timers;
    UNSIGNED                 dev6_ip6_mld_last_query_int;
#endif

#if (INCLUDE_DHCP6 == NU_TRUE)

    UINT32                  dev6_iaid;

#endif

    IP6_NEIGHBOR_CACHE_ENTRY *dev6_ip6_neighbor_cache;

    INT     (*dev6_ip6_neighcache_entry_equal) (const UINT8 *, const IP6_NEIGHBOR_CACHE_ENTRY *);
    VOID    (*dev6_ip6_update_neighcache_link_addr) (const IP6_NEIGHBOR_CACHE_ENTRY *, const UINT8 *);

    /* procedure handles to manipulate the Neighbor Cache */
    IP6_NEIGHBOR_CACHE_ENTRY    *(*dev6_ip6_add_neighcache_entry) (DV_DEVICE_ENTRY*, UINT8*, const UINT8*, UINT32, NET_BUFFER*, UINT8);
    STATUS  (*dev6_ip6_del_neighcache_entry) (DV_DEVICE_ENTRY*, UINT8*);
    IP6_NEIGHBOR_CACHE_ENTRY    *(*dev6_ip6_fnd_neighcache_entry) (const DV_DEVICE_ENTRY*, const UINT8*);

} DEV6_IPV6_STRUCT;

#define ip6_interface_mib               dev6_ipv6_data->dev6_ip6_interface_mib

#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)
#define ip6_icmp_mib                    dev6_ipv6_data->dev6_ip6_icmp_mib
#endif

#define dev6_link_mtu                   dev6_ipv6_data->dev6_ip6_link_mtu
#define dev6_default_mtu                dev6_ipv6_data->dev6_ip6_default_mtu
#define dev6_last_error_msg_timestamp   dev6_ipv6_data->dev6_ip6_last_error_msg_timestamp

#if (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE)

#define dev6_AdvLinkMTU                 dev6_ipv6_data->dev6_ip6_AdvLinkMTU
#define dev6_AdvReachableTime           dev6_ipv6_data->dev6_ip6_AdvReachableTime
#define dev6_AdvRetransTimer            dev6_ipv6_data->dev6_ip6_AdvRetransTimer
#define dev6_next_radv_time             dev6_ipv6_data->dev6_ip6_next_radv_time
#define dev6_last_radv_time             dev6_ipv6_data->dev6_ip6_last_radv_time
#define dev6_MaxRtrAdvInterval          dev6_ipv6_data->dev6_ip6_MaxRtrAdvInterval
#define dev6_MinRtrAdvInterval          dev6_ipv6_data->dev6_ip6_MinRtrAdvInterval
#define dev6_AdvDefaultLifetime         dev6_ipv6_data->dev6_ip6_AdvDefaultLifetime
#define dev6_init_max_RtrAdv            dev6_ipv6_data->dev6_ip6_init_max_RtrAdv
#define dev6_AdvCurHopLimit             dev6_ipv6_data->dev6_ip6_AdvCurHopLimit

#endif

#define dev6_flags                      dev6_ipv6_data->dev6_ip6_flags
#define dev6_base_reachable_time        dev6_ipv6_data->dev6_ip6_base_reachable_time
#define dev6_retrans_timer              dev6_ipv6_data->dev6_ip6_retrans_timer
#define dev6_reachable_time             dev6_ipv6_data->dev6_ip6_reachable_time
#define dev6_rtr_sols                   dev6_ipv6_data->dev6_ip6_rtr_sols
#define dev6_interface_id               dev6_ipv6_data->dev6_ip6_interface_id
#define dev6_cur_hop_limit              dev6_ipv6_data->dev6_ip6_cur_hop_limit
#define dev6_interface_id_length        dev6_ipv6_data->dev6_ip6_interface_id_length
#define dev6_nc_entries                 dev6_ipv6_data->dev6_ip6_nc_entries
#define dev6_dup_addr_detect_trans      dev6_ipv6_data->dev6_ip6_dup_addr_detect_trans

#define dev6_prefix_list                dev6_ipv6_data->dev6_ip6_prefix_list
#define dev6_addr_list                  dev6_ipv6_data->dev6_ip6_addr_list

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

#define dev6_multiaddrs                 dev6_ipv6_data->dev6_ip6_multiaddrs
#define dev6_mld_state_list             dev6_ipv6_data->dev6_ip6_mld_state_list
#define dev6_mld_compat_mode            dev6_ipv6_data->dev6_ip6_mld_compat_mode
#define dev6_mld_compat_timers          dev6_ipv6_data->dev6_ip6_mld_compat_timers
#define dev6_mld_last_query_int         dev6_ipv6_data->dev6_ip6_mld_last_query_int

#endif

#define dev6_neighbor_cache             dev6_ipv6_data->dev6_ip6_neighbor_cache

#define dev6_neighcache_entry_equal         dev6_ipv6_data->dev6_ip6_neighcache_entry_equal
#define dev6_update_neighcache_link_addr    dev6_ipv6_data->dev6_ip6_update_neighcache_link_addr
#define dev6_add_neighcache_entry           dev6_ipv6_data->dev6_ip6_add_neighcache_entry
#define dev6_del_neighcache_entry           dev6_ipv6_data->dev6_ip6_del_neighcache_entry
#define dev6_fnd_neighcache_entry           dev6_ipv6_data->dev6_ip6_fnd_neighcache_entry

#define dev6_home_agent_bu                  dev6_ipv6_data->dev6_mobile_ip6.mbl_ip6_home_agent_bu
#define dev6_home_agent                     dev6_ipv6_data->dev6_mobile_ip6.mbl_ip6_home_agent_bu->bu_destination
#define dev6_mbl_ip_flags                   dev6_ipv6_data->dev6_mobile_ip6.mbl_ip6_flags

#endif

STATUS          DEV6_Init_Device(DV_DEVICE_ENTRY *, const DEV_DEVICE *);
INT             DEV6_AutoConfigure_Device(DV_DEVICE_ENTRY *);
STATUS          DEV6_Autoconfigure_Addrs(DV_DEVICE_ENTRY *);
INT             DEV6_Attach_IP_To_Device(const CHAR *, const UINT8 *, UINT32);
VOID            DEV6_Insert_Address(DEV6_IF_ADDRESS *);
DEV6_IF_ADDRESS *DEV6_Find_Target_Address(const DV_DEVICE_ENTRY *, const UINT8 *);
VOID            DEV6_Verify_Valid_Addr(TQ_EVENT, UNSIGNED, UNSIGNED);
DV_DEVICE_ENTRY *DEV6_Get_Primary_Interface(VOID);
INT             DEV6_Add_IP_To_Device(DV_DEVICE_ENTRY *, const UINT8 *, UINT8,
                                      UINT32, UINT32, UINT32);
VOID            DEV6_Expire_Address(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID            DEV6_Delete_IP_From_Device(DEV6_IF_ADDRESS *);
VOID            DEV6_Deprecate_Address(TQ_EVENT, UNSIGNED, UNSIGNED);
DV_DEVICE_ENTRY *DEV6_Get_Dev_By_Addr(const UINT8 *);
STATUS          DEV6_Detach_IP_From_Device(DV_DEVICE_ENTRY *);
VOID            DEV6_Detach_Addrs_From_Device(DV_DEVICE_ENTRY *);
DV_DEVICE_ENTRY *DEV6_Find_Configured_Device(VOID);
STATUS          DEV6_Create_Address_From_Prefix(DV_DEVICE_ENTRY *, const UINT8 *,
                                                UINT8, UINT32, UINT32, UINT32);
VOID            DEV6_Configure_Router(DV_DEVICE_ENTRY *, UINT32, INT, const DEV6_RTR_OPTS *);
STATUS          DEV6_Configure_Prefix_List(DV_DEVICE_ENTRY *, DEV6_PRFX_ENTRY *);
VOID            DEV6_Cleanup_Device(DV_DEVICE_ENTRY *dev, UINT32 flags);
VOID            DEV6_Update_Address_Entry(DV_DEVICE_ENTRY *, DEV6_IF_ADDRESS *,
                                          UINT32, UINT32);
VOID            DEV6_Initialize_Router(DV_DEVICE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

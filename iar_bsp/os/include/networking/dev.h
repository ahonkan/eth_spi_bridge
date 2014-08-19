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
*       dev.h
*
*   DESCRIPTION
*
*       Definitions for multiple device driver interface.
*
*   DATA STRUCTURES
*
*       DEV_IF_ADDRESS
*       _DV_DEVICE_ENTRY
*       DV_DEVICE_LIST
*       _DV_REQ
*       URT_DEV
*       ETHER_DEV
*       _DEV_DEVICE
*
*   DEPENDENCIES
*
*       mem_defs.h
*       net.h
*       mib2.h
*       net6_cfg.h
*       dev6.h
*       ip6_mib.h
        net_evt.h
*
*************************************************************************/

#ifndef DEV_H
#define DEV_H

#include "networking/mem_defs.h"
#include "networking/net.h"
#include "networking/mib2.h"
#include "kernel/dev_mgr.h"
#include "networking/net_evt.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/dev6.h"
#include "networking/ip6_mib.h"
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/sockdefs.h"
#include "networking/ips_cfg.h"
#include "networking/ips_enc.h"
#ifdef IPSEC_VERSION_COMP
#include "networking/ips_esn.h"
#endif
#include "networking/ips_ar.h"
#include "networking/ips_tim.h"
#include "networking/ips_sadb.h"
#include "networking/ips_spdb.h"
#include "networking/ips_grp.h"
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

struct _dev_if_addr_entry
{
    DEV_IF_ADDR_ENTRY   *dev_entry_next;
    DEV_IF_ADDR_ENTRY   *dev_entry_prev;
    UINT32  dev_entry_ip_addr;                /* address of interface */
    UINT32  dev_entry_netmask;                /* used to determine subnet */
    UINT32  dev_entry_net;                    /* Network number. */
    UINT32  dev_entry_net_brdcast;            /* Network broadcast. */
    INT8    dev_entry_dup_addr_detections;    /* dup addr detected */
    UINT8   padN[3];
};

typedef struct _dev_if_address_list
{
    DEV_IF_ADDR_ENTRY   *dv_head;
    DEV_IF_ADDR_ENTRY   *dv_tail;
} DEV_IF_ADDRESS_LIST;

typedef struct dev_if_address
{
    DEV_IF_ADDRESS_LIST dev_addr_list;          /* List of IPv4 addresses */
    UINT32              dev_dst_ip_addr;        /* other end of p-to-p link */

#if (INCLUDE_LL_CONFIG == NU_TRUE)
    UINT32              dev_link_local_addr;    /* Link-local address. */
#endif

    IP_MULTI            *dev_multiaddrs;        /* List of IPv4 multicast
                                                 * addresses */
#if (INCLUDE_DHCP == NU_TRUE)
    /* These fields are used only if the IP address is obtained via DHCP. */
    UINT32  dev_dhcp_addr;              /* The IP address obtained via DHCP */
    UINT32  dev_dhcp_lease;             /* Lease time for the IP addr. */
    UINT32  dev_dhcp_renew;             /* Renewal time for IP addr. */
    UINT32  dev_dhcp_rebind;            /* Rebinding time for the IP addr. */
    UINT32  dev_dhcp_state;             /* The DHCP state. */
    UINT32  dev_dhcp_server_addr;       /* The DHCP server's IP address. */
    INT32   dev_dhcp_opts_length;       /* Length of the options */
    CHAR   *dev_dhcp_options;           /* DHCP Options */
#endif /* INCLUDE_DHCP */

} DEV_IF_ADDRESS;


/* Double Link List for device's multicast state */
struct _DEV_MULTI_IGMP_LIST
{
    MULTI_DEV_STATE         *head;
    MULTI_DEV_STATE         *tail;
};

typedef struct _DV_PHY_DEVICE
{
    UINT32                  dev_phy_irq;
    UINT32                  dev_phy_sm_addr;       /* shared memory address */
    UINT32                  dev_phy_com_port;
    UINT32                  dev_phy_baud_rate;
    UINT32                  dev_phy_parity;
    UINT32                  dev_phy_stop_bits;
    UINT32                  dev_phy_data_bits;
    UINT32                  dev_phy_data_mode;
    UINT16                  dev_phy_vect;
    UINT8                   dev_phy_pad_1[2];

    STATUS                  (*dev_phy_open) (UINT8 *, DV_DEVICE_ENTRY *);
    STATUS                  (*dev_phy_receive) (DV_DEVICE_ENTRY *);
    STATUS                  (*dev_phy_event) (DV_DEVICE_ENTRY *, UNSIGNED);
    STATUS                  (*dev_phy_hw_query) (DV_DEVICE_ENTRY *);

    NET_MULTI               *dev_phy_ethermulti;    /* List of multicast ethernet   */

#if (INCLUDE_IPSEC == NU_TRUE)

    /* Pointer to the IPsec Group that this interface is associated with.
     * This will determine the security policies and the security
     * associations that are available for this interface.
     */
    IPSEC_POLICY_GROUP      *dev_phy_ips_group;

#endif

#if (INCLUDE_IKE == NU_TRUE)

     /* Pointer to the IKE Group that this interface is associated with.
      * This will determine the IKE security policies and the security
      * associations that are available for this interface. Data type
      * of this pointer is IKE_POLICY_GROUP* but has been made a
      * VOID* because of a circular dependency in the IKE header files.
      */
    VOID                    *dev_phy_ike_group;

#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT16                  dev_phy_vlan_mode;      /* Device's VLAN mode */
    UINT8                   dev_phy_pad_2[2];
#endif

#if (USE_SW_VLAN_METHOD == NU_TRUE)

    struct _DV_DEVICE_ENTRY *dev_phy_vlan_associated_device;   /* Real Ethernet device associated with the Virtual device */
    UINT16                  dev_phy_vlan_vid;
    UINT8                   dev_phy_vlan_prio;
    UINT8                   dev_phy_pad_3[1];

#endif

#if (HARDWARE_OFFLOAD == NU_TRUE)

    UINT32                  dev_phy_hw_options;          /* Set of HW offloadable options */
    UINT32                  dev_phy_hw_options_enabled;  /* Subset of selected HW offloadable options to be enabled */

#endif


} DV_PHY_DEVICE;

/* The device request structure is used when making requests to a driver via
   the driver's ioctl function. */
struct _DV_REQ
{
    INT     dvr_optname;
    UINT32  dvr_flags;
    UINT32  dvr_pcmd;       /* Driver primary command word */
    UINT32  dvr_scmd;       /* Driver secondary command word */
    UINT32  dvr_value_1;
    UINT32  dvr_value_2;
    VOID    *dvr_value_ptr_1;
    VOID    *dvr_value_ptr_2;

    union
    {
        UINT32      dvru_addr;
        UINT32      dvru_dstaddr;
        UINT32      dvru_broadaddr;
        int         dvru_metric;
        UINT8       *dvru_data;
        UINT8       dvru_addrv6[16];
    } dvr_dvru;
};

#include "networking/rtab.h"

struct  _DV_DEVICE_ENTRY {

    struct _DV_DEVICE_ENTRY *dev_next;
    struct _DV_DEVICE_ENTRY *dev_previous;

    union
    {
        DV_PHY_DEVICE       *dev_phy;      /* Physical device in non-virtual device case. */
        VOID                *dev_ext_1;    /* Pointer for device extension. */
    } dev_ext;

    CHAR                    dev_net_if_name[DEV_NAME_LENGTH];   /* Must be unique. */
    UINT32                  dev_io_addr;
    UINT32                  dev_flags;
    UINT32                  dev_flags2;
    UINT32                  dev_index;           /* Unique identifier. */
    UINT8                   dev_mac_addr[6];     /* Address of device. */
    UINT8                   dev_type;            /* ethernet, token ring, etc */
    UINT8                   dev_hdrlen;          /* media header length */
    UINT8                   dev_addrlen;         /* media address length */
    UINT8                   dev_max_error_msg;   /* ICMP error rate */
    UINT8                   dev_error_msg_count; /* Count of ICMP errors sent */

#if (INCLUDE_LL_CONFIG == NU_TRUE)
    UINT8                   dev_ll_conflict_count;
    UINT8                   dev_ll_probe_count;
    UINT8                   dev_ll_announce_count;
    UINT8                   dev_ll_state;
#endif

    UINT8                   dev_pad_1[1];

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )
    IP_MULTI                *dev_multi_addr;     /* IPv4 Multicast Addr info */
    UNSIGNED                dev_igmp_compat_mode;/* Indicates the IGMP
                                                  * version of the device
                                                  */
    UNSIGNED                dev_igmp_compat_timers;
    MULTI_DEV_IGMP_LIST     dev_igmp_state_list;
    UNSIGNED                dev_igmp_last_query_interval;
    UNSIGNED                dev_igmp_last_query_response_interval;
#endif


#if (INCLUDE_IPV6 == NU_TRUE)

    DEV6_IPV6_STRUCT            *dev6_ipv6_data;    /* IPv6-specific data. */

#endif

    /* procedure handles */
    STATUS  (*dev_start) (DV_DEVICE_ENTRY *, NET_BUFFER *);
    STATUS  (*dev_output) (NET_BUFFER *, DV_DEVICE_ENTRY *, VOID *, VOID *);
    STATUS  (*dev_input) (VOID);
    STATUS  (*dev_ioctl) (DV_DEVICE_ENTRY *, INT, DV_REQ *);

    /* transmit list pointer. */
    NET_BUFFER_HEADER       dev_transq;
    UINT32                  dev_transq_length;

#if (INCLUDE_IPV4 == NU_TRUE)
    DEV_IF_ADDRESS          dev_addr;           /* IPV4 Address information */

#if (INCLUDE_IP_FORWARDING == NU_TRUE)
    RTAB_ROUTE              dev_forward_rt;
#endif
#endif

    VOID                    *dev_link_layer;    /* Additional information regarding a
                                                 * specific link protocol, such as
                                                 * configuration, events, timers, etc.
                                                 */

    UINT32                  dev_mtu;            /* maximum transmission unit, excluding media
                                                 * header length, i.e., 1500 for ethernet */
    UINT32                  dev_metric;         /* routing metric (external only) */
    UINT32                  dev_driver_options;
    UINT32                  dev_reasm_max_size;

    /* MIB2 Interface */
#if (MIB2_IF_INCLUDE == NU_TRUE)
    MIB2_INTERFACE_STRUCT   dev_mibInterface;
#endif

#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )
    UINT32                  dev_dhcp_iaid;      /* The IAID for this interface. */
#endif

#if (INCLUDE_MDNS == NU_TRUE)
    CHAR                    *dev_host_name;
#endif

    UINT32                  dev_first_error_msg_timestamp;   /* Timestamp of last ICMP error */
    UINT32                  dev_error_msg_rate_limit;       /* ICMP error rate */

    UINT32                  user_defined_1;     /* Available for users for anything. */
    UINT32                  user_defined_2;     /* Available for users for anything. */
    UINT32                  system_use_1;       /* Reserved for System use. */
    UINT32                  system_use_2;       /* Reserved for System use. */
    UINT32                  system_use_3;       /* Reserved for System use. */

    DV_DEV_HANDLE           dev_handle;
};

/* These macros simplify access to the fields in struct DV_PHY_DEVICE. */
#define dev_physical                 dev_ext.dev_phy
#define dev_extension                dev_ext.dev_ext_1
#define dev_sm_addr                  dev_ext.dev_phy->dev_phy_sm_addr
#define dev_irq                      dev_ext.dev_phy->dev_phy_irq
#define dev_com_port                 dev_ext.dev_phy->dev_phy_com_port
#define dev_data_mode                dev_ext.dev_phy->dev_phy_data_mode
#define dev_parity                   dev_ext.dev_phy->dev_phy_parity
#define dev_stop_bits                dev_ext.dev_phy->dev_phy_stop_bits
#define dev_data_bits                dev_ext.dev_phy->dev_phy_data_bits
#define dev_open                     dev_ext.dev_phy->dev_phy_open
#define dev_receive                  dev_ext.dev_phy->dev_phy_receive
#define dev_event                    dev_ext.dev_phy->dev_phy_event
#define dev_hw_query                 dev_ext.dev_phy->dev_phy_hw_query
#define dev_baud_rate                dev_ext.dev_phy->dev_phy_baud_rate
#define dev_vect                     dev_ext.dev_phy->dev_phy_vect
#define dev_ethermulti               dev_ext.dev_phy->dev_phy_ethermulti

/* SW VLAN Interface */
#if (USE_SW_VLAN_METHOD == NU_TRUE)

#define dev_vlan_vid                 dev_ext.dev_phy->dev_phy_vlan_vid
#define dev_vlan_associated_device   dev_ext.dev_phy->dev_phy_vlan_associated_device
#define dev_vlan_vprio               dev_ext.dev_phy->dev_phy_vlan_prio

#endif

#if (HARDWARE_OFFLOAD == NU_TRUE)

#define dev_hw_options               dev_ext.dev_phy->dev_phy_hw_options
#define dev_hw_options_enabled       dev_ext.dev_phy->dev_phy_hw_options_enabled

#endif

#define dev_ip_addr             dev_addr_list.dv_head->dev_entry_ip_addr
#define dev_netmask             dev_addr_list.dv_head->dev_entry_netmask
#define dev_dest_ip_addr        dev_addr_list.dev_entry_dest_ip_addr
#define dev_net                 dev_addr_list.dv_head->dev_entry_net
#define dev_net_brdcast         dev_addr_list.dv_head->dev_entry_net_brdcast


typedef struct _DV_DEVICE_LIST
{
    struct _DV_DEVICE_ENTRY   *dv_head;
    struct _DV_DEVICE_ENTRY   *dv_tail;
} DV_DEVICE_LIST;

struct if_nameindex
{
    INT32       if_index;   /* 1, 2, ... */
    CHAR        *if_name;   /* null terminated name: "le0", ... */
};

/* These macros simplify access to the fields in struct _DV_REQ. */
#define dvr_addr        dvr_dvru.dvru_addr
#define dvr_dstaddr     dvr_dvru.dvru_dstaddr
#define dvr_broadaddr   dvr_dvru.dvru_broadaddr
#define dvr_metric      dvr_dvru.dvru_metric
#define dvr_data        dvr_dvru.dvru_data

/* Device ioctl options. */
#define DEV_ADDMULTI            1
#define DEV_DELMULTI            2
#define DEV_HW_OFFLOAD_CTRL     3
#define DEV_GET_HW_OFLD_CAP     4
#define DEV_REMDEV              5
#define DEV_SET_VLAN_TAG        6  /* Change VLAN TAG field */
#define DEV_SET_VLAN_RX_MODE    7  /* Change VLAN reception mode */
#define DEV_GET_VLAN_TAG        8  /* Retrieve VLAN TAG field */
#define DEV_SET_HW_ADDR         9  /* Set Physical/MAC Address of the Hardware */
#define DEV_GET_HW_ADDR         10 /* Get the Physical/MAC Address of the Hardware */

/* Device ioctl options specific to 802.11. */
#define SIOCIWFIRST             19
#define SIOCSIWCOMMIT           19 /* Commit pending changes to driver */
#define SIOCSIWNAME             20 /* Set name. Wireless Protocol */
#define SIOCGIWNAME             21 /* Get name. Wireless Protocol : IEEE 802.11a */
#define SIOCSIWNWID             22 /* Set network ID */
#define SIOCGIWNWID             23 /* Get network ID */
#define SIOCSIWFREQ             24 /* Set channel/frequency */
#define SIOCGIWFREQ             25 /* Get channel/frequency */
#define SIOCSIWMODE             26 /* Set operation mode. */
#define SIOCGIWMODE             27 /* Get operation mode : Infrastructure, ADHOC */
#define SIOCSIWSENS             28 /* Set sensitivity threshold */
#define SIOCGIWSENS             29 /* Get sensitivity threshold */
#define SIOCSIWRANGE            30 /* Set range of parameters */
#define SIOCGIWRANGE            31 /* Get range of parameters */
#define SIOCSIWPRIV             32 /* Set private IOCTLs for the interface */
#define SIOCGIWPRIV             33 /* Get private IOCTLs for the interface */
#define SIOCSIWSPY              34 /* Set spy addressess */
#define SIOCGIWSPY              35 /* Get spy addressess */
#define SIOCSIWAP               36 /* Set the BSSID (Access Point MAC Address) */
#define SIOCGIWAP               37 /* Get the BSSID (Access Point MAC Address) */
#define SIOCSIWESSID            38 /* Set ESSID */
#define SIOCGIWESSID            39 /* Get ESSID */
#define SIOCSIWNICKN            40 /* Set node name */
#define SIOCGIWNICKN            41 /* Get node name */
#define SIOCSIWRATE             42 /* Set default bit rate */
#define SIOCGIWRATE             43 /* Get default bit rate */
#define SIOCSIWRTS              44 /* Set RTS threshold */
#define SIOCGIWRTS              45 /* Get RTS threshold */
#define SIOCSIWFRAG             46 /* Set fragmentation threshold */
#define SIOCGIWFRAG             47 /* Get fragmentation threshold */
#define SIOCSIWTXPOW            48 /* Set TX power */
#define SIOCGIWTXPOW            49 /* Get TX power */
#define SIOCSIWRETRY            50 /* Set retry limits and lifetime */
#define SIOCGIWRETRY            51 /* Get retry limits and lifetime */
#define SIOCSIWENCODE           52 /* Set encryption key */
#define SIOCGIWENCODE           53 /* Get encryption key */
#define SIOCSIWPOWER            54 /* Set power management settings */
#define SIOCGIWPOWER            55 /* Get power management settings */
#define SIOCSIWSTATS            56 /* Set the wireless/net statistics */
#define SIOCGIWSTATS            57 /* Get the wireless/net statistics */
#define SIOCSIWSCAN             58 /* Trigger scanning */
#define SIOCGIWSCAN             59 /* Get scanning results */
#define SIOCSIWGENIE            60 /* Set generic IE */
#define SIOCGIWGENIE            61 /* Get generic IE */
#define SIOCSIWMLME             62 /* Request MLME operation; */
#define SIOCSIWAUTH             63 /* Set authentication mode params */
#define SIOCGIWAUTH             64 /* Get authentication mode params */
#define SIOCSIWENCODEEXT        65 /* Set encoding token & mode */
#define SIOCGIWENCODEEXT        66 /* Get encoding token & mode */
#define SIOCSIWPMKSA            67 /* PMKSA cache operation */
#define SIOCGIWAPLIST           68 /* Deprecated in favor of scanning */

/* Private IOCTLs constants.
*/
#ifndef SIOCS80211
#define SIOCS80211              70 /* Set 802.11 specific parameter */
#endif

#ifndef SIOCG80211
#define SIOCG80211              71 /* Get 802.11 specific parameter */
#endif

#define SIOCIWFIRSTPRIV         80
#define SIOCIWLASTPRIV          119
#define SIOCIWLAST              SIOCIWLASTPRIV

#define DEV_80211_INIT_CHANN           122 /* Initialize the channel list */
#define DEV_80211_INIT_ADHOC           123 /* Start a new Independent BSS */
#define DEV_80211_ROAM                 124 /* Initialize roaming. */
#define DEV_80211_JOIN                 125 /* Join with the specified BSS */
#define DEV_80211_SYNC                 126 /* Synchronize with the BSS */
#define DEV_80211_AUTH                 127 /* Authenticate with the AP */
#define DEV_80211_ASOC                 128 /* Associate/Re-associate with AP */
#define DEV_80211_DEAUTH               129 /* Deauthenticate peer station */
#define DEV_80211_DEASOC               130 /* Disassociate peer station */
#define DEV_80211_AUTH_MODE            131 /* Set/Get authentication mode */
#define DEV_80211_DEFAULT_KEY          132 /* Set/Get default WEP key */
#define DEV_80211_PRIVACY              133 /* Set privacy state machine */
#define DEV_80211_GROUP_ADDRESS_TABLE  134 /* Set an entry in group address table */

#define DEV_80211_SET_PHY_ANTENNA      135 /* Set PHY antenna for Tx/Rx */
#define DEV_80211_SET_PHY_OPERATION    136 /* Set PHY operation mode */
#define DEV_80211_SET_PHYOFDM_ENTRY    137 /* Set PHY OFDM entry */
#define DEV_80211_SET_PHYDSSS_ENTRY    138 /* Set PHY DSSS entry */
#define DEV_80211_SET_PHYHRDSSS_ENTRY  139 /* Set PHY HR DSSS entry */
#define DEV_80211_SET_PHY_ERP_ENTRY    140 /* Set PHY ERP entry */
#define DEV_80211_SET_PHYFHSS_ENTRY    141 /* Set PHY FHSS entry */
#define DEV_80211_SET_PHYIR_ENTRY      142 /* Set PHY IR */

#define DEV_80211_GET_PHY_ANTENNA      143 /* Get PHY antenna for Tx/Rx */
#define DEV_80211_GET_PHY_OPERATION    144 /* Get PHY operation mode */
#define DEV_80211_GET_PHYOFDM_ENTRY    145 /* Get PHY OFDM entry */
#define DEV_80211_GET_PHYDSSS_ENTRY    146 /* Get PHY DSSS entry */
#define DEV_80211_GET_PHYHRDSSS_ENTRY  147 /* Get PHY HR DSSS entry */
#define DEV_80211_GET_PHY_ERP_ENTRY    148 /* Get PHY ERP entry */
#define DEV_80211_GET_PHYFHSS_ENTRY    149 /* Get PHY FHSS entry */
#define DEV_80211_GET_PHYIR_ENTRY      150 /* Get PHY IR */

/* These flags are used in device->dev_hw_options and device->dev_hw_options_enabled    */
/* for TRANSMISSIONS.  These flags are used in buf_ptr->mem_hw_options for RECEIVES.    */

#define HW_TX_IP4_CHKSUM    0x01   /* Enable TX IP4 H/W checksum offloading */
#define HW_RX_IP4_CHKSUM    0x02   /* Enable RX IP4 H/W checksum offloading */

#define HW_TX_TCP_CHKSUM    0x04    /* Enable TX TCP H/W checksum offloading for IPv4 */
#define HW_RX_TCP_CHKSUM    0x08    /* Enable RX TCP H/W checksum offloading for IPv4 */

#define HW_TX_TCP6_CHKSUM   0x10    /* Enable TX TCP H/W checksum offloading for IPv6 */
#define HW_RX_TCP6_CHKSUM   0x20    /* Enable RX TCP H/W checksum offloading for IPv6 */

#define HW_TX_UDP_CHKSUM    0x40    /* Enable TX UDP H/W checksum offloading for IPv4 */
#define HW_RX_UDP_CHKSUM    0x80    /* Enable RX UDP H/W checksum offloading for IPv4 */

#define HW_TX_UDP6_CHKSUM   0x100   /* Enable TX UDP H/W checksum offloading for IPv6 */
#define HW_RX_UDP6_CHKSUM   0x200   /* Enable RX UDP H/W checksum offloading for IPv6 */

#define HW_TX_VLAN          0x400   /* Enable TX VLAN H/W offloading         */
#define HW_RX_VLAN          0x800   /* Enable RX VLAN H/W offloading         */

#define HW_TX_TCP4_SEG      0x1000  /* Enable TX TCP4 Segmentation H/W offloading */
#define HW_TX_TCP6_SEG      0x2000  /* Enable TX TCP6 Segmentation H/W offloading */

#define HW_RX_WAKE_UP       0x4000  /* Enable RX WAKE UP feature             */
#define HW_TX_FLOW_CTRL     0x8000  /* Enable TX H/W XON-XOFF Flow Control   */

#define HW_TX_ICMP6_CHKSUM  0x10000 /* Enable TX ICMPv6 checksum offloading */
#define HW_RX_ICMP6_CHKSUM  0x20000 /* Enable RX ICMPv6 checksum offloading */

#define HW_RX_PROM_ENABLE   0x40000 /* RX promiscuous mode */

#define HW_SET_MAC_ADDRESS  0x80000 /* Override the default MAC Address */

/* These definitions are used in NU_Ioctl (sck_io.c) */

#define DEV_DISABLE_HW_OPTIONS  0   /* Disable HW Offloading Option */

#define DEV_ENABLE_HW_OPTIONS   1   /* Enable HW Offloading Option  */


#define DEV_MULTIV6         0x1

/*  Defines for the DEV_NET_IF.dev_flags field.  */

#define DV_UP                   0x1     /* interface is up                  */
#define DV_BROADCAST            0x2     /* broadcast address valid          */
#define DV_CFG_IPV4_LL_ADDR     0x4     /* turn on debugging                */
#define DV_LOOPBACK             0x8     /* is a loopback net                */
#define DV_POINTTOPOINT         0x10    /* interface is point-to-point link */
#define DV_NOTRAILERS           0x20    /* avoid use of trailers            */
#define DV_RUNNING              0x40    /* resources allocated              */
#define DV_NOARP                0x80    /* no address resolution protocol   */
#define DV_PROMISC              0x100   /* receive all packets              */
#define DV_ALLMULTI             0x200   /* receive all multicast packets    */
#define DV_OACTIVE              0x400   /* transmission in progress         */
#define DV_SIMPLEX              0x800   /* can't hear own transmissions     */
#define DV_LINK0                0x1000  /* per link layer defined bit       */
#define DV_LINK1                0x2000  /* per link layer defined bit       */
#define DV_PROXYARP             0x4000  /* proxy address resolution protocol*/
#define DV_MULTICAST            0x8000  /* supports multicast               */
#define DV6_IPV6                0x10000UL /* supports IPv6 */
#define DV6_NOPFX               0x20000UL   /* skip kernel prefix management. */
#define DV6_ISROUTER            0x40000UL  /* The interface is router enabled. */
#define DV6_CFG_FLAG            0x80000UL
#define DV6_MGD_FLAG            0x100000UL
#define DV6_PRIMARY_INT         0x200000UL
#define DV6_VIRTUAL_DEV         0x400000UL
#define DV6_6TO4_DEV            0x800000UL
#define DV6_CFG6_DEV            0x1000000UL
#define DV6_NODAD               0x2000000UL /* don't perform DAD on this interface
                                             * (used only at first SIOC* call)
                                             */
#define DV_ADDR_CFG             0x4000000UL /* Dynamic configuration is in progress
                                             * for this interface; ie, DHCP, BOOTP, RARP */
#define DV6_ADV_MGD             0x8000000UL /* The value to be placed in the Managed
                                             * Address Configuration flag field in the
                                             * Router Advertisement. */
#define DV6_ADV_OTH_CFG         0x10000000UL /* The value to be placed in the Other
                                              * Stateful Configuration flag field in the
                                              * Router Advertisement. */
#define DV_INT_EXT              0x20000000UL /* Interface extension is implemented. */
#define DV_INT_HC               0x40000000UL /* 64 bit counters are implemented. */

#define DV_VIRTUAL_DEV          0x80000000UL /* Flag to represent the virtual device. */

/*  Defines for the DEV_NET_IF.dev_flags2 field.  */
#define DV_IPSEC_ENABLE         0x1 /* Enable IPsec on the interface. */
#define DV6_UP                  0x2 /* Interface is able to receive IPv6 packets. */

/* Device types. */
#define DVT_OTHER       1
#define DVT_ETHER       2       /* Ethernet */
#define DVT_LOOP        3       /* Loop back interface. */
#define DVT_SLIP        4       /* Serial Line IP */
#define DVT_PPP         5       /* Point to Point Protocol */
#define DVT_PPPOE       6       /* Point to Point Protocol */
#define DVT_6TO4        7       /* IPv6 6to4 tunnel virtual device */
#define DVT_CFG6        8       /* IPv6 configured tunnel virtual device */
#define DVT_VLAN        9       /* Virtual Local Area Network device */
#define DVT_L2TP_TUNNEL 10      /* L2TP tunnel interface. */
#define DVT_LAC_PPP     11      /* LAC PPP interface (L2TP). */
#define DVT_L2TP_SSN    12      /* L2TP Virtual session. */
#define DVT_PPTP_TUNNEL 13      /* PPTP tunnel interface. */
#define DVT_PAC_PPP     14      /* PAC PPP interface (PPTP). */
#define DVT_PPTP_SSN    15      /* PPTP Virtual session. */

/* Device VLAN mode types */
#define DEV_NO_VLAN  0    /* Device does not use VLAN */
#define DEV_SW_VLAN  1    /* Device uses software VLAN operations */
#define DEV_HW_VLAN  2    /* Device uses hardware VLAN operations */

/* NU_Remove_Device flags */
#define RMDEV_CLEAN_MCAST   0x1

/* Link Local States */
#define LL_STATE_PROBE_WAIT    1
#define LL_STATE_ANNOUNCE_WAIT 2
#define LL_STATE_ARP_ANNOUNCE  3
#define LL_STATE_CLAIMED       4
#define LL_STATE_CONFLICT      5

typedef struct _URT_INIT_STRUCT
{
    UINT32       com_port;
    UINT32       baud_rate;
    UINT32       data_mode;
    UINT32       parity;
    UINT32       stop_bits;
    UINT32       data_bits;
} URT_DEV;

typedef struct _ETHER_INIT_STRUCT
{
    UINT32      dv_irq;
    UINT32      dv_io_addr;
    UINT32      dv_shared_addr;
    UINT8       dv_mac_addr[6];
    UINT8       padN[2];
} ETHER_DEV;

struct _DEV_DEVICE
{
    CHAR        *dv_name;
    UINT32      dv_flags;
    UINT32      dv_driver_options;
    STATUS      (*dv_init) (DV_DEVICE_ENTRY *);

#if (INCLUDE_IPV4 == NU_TRUE)

    UINT8       dv_ip_addr[4];
    UINT8       dv_subnet_mask[4];
    UINT8       dv_gw[4];

#endif

    /* HDLC Driver interface needs a pointer container to allow PPP to */
    /* use the same interface as is supported by direct API usage.     */

    VOID        *dv_hdlc_ptr;  /* void pointer is still pointer size bytes */

#if (INCLUDE_IPV6 == NU_TRUE)

    UINT32          dv6_flags;
    UINT8           dv6_ip_addr[16];
    DEV6_RTR_OPTS   dv6_rtr_opts;

#endif

#if ( (INCLUDE_VLAN == NU_TRUE) && (USE_SW_VLAN_METHOD) )

    UINT32      dv_type;

#endif

    /* This union defines the hardware specific portion of the device
       initialization structure. */
    union _dv_hw
    {
        URT_DEV     uart;
        ETHER_DEV   ether;
    } dv_hw;

    DV_DEV_HANDLE   dev_handle;

};

extern DV_DEVICE_LIST    DEV_Table;

/*  DEV.C Function Prototypes.  */
DV_DEVICE_ENTRY *DEV_Get_Dev_For_Vector(INT vector);
DV_DEVICE_ENTRY *DEV_Get_Dev_By_Index(UINT32 device_index);
STATUS  DEV_Device_Up(const CHAR *if_name);
VOID    DEV_Recover_TX_Buffers (DV_DEVICE_ENTRY *);
STATUS  DEV_Set_Phys_Address(DV_DEVICE_ENTRY *dev_ptr, UINT8 *mac_addr);
STATUS  DEV_Set_Net_Address(const UINT8 *ip_addr, const UINT8 *new_ip_addr);
INT     DEV_Get_Ether_Address(const CHAR *name, UINT8 *ether_addr);
INT     DEV_Attach_IP_To_Device(const CHAR *name, const UINT8 *ip_addr,
                                const UINT8 *subnet);
STATUS  DEV_Initialize_IP(DV_DEVICE_ENTRY *device, const UINT8 *ip_addr,
                          const UINT8 *subnet, DEV_IF_ADDR_ENTRY *new_entry);
VOID    DEV_Detach_Addrs_From_Device(DV_DEVICE_ENTRY *dev);
DV_DEVICE_ENTRY *DEV_Get_Dev_By_Name(const CHAR *name);
DV_DEVICE_ENTRY *DEV_Get_Dev_By_Addr(const UINT8 *addr);
DEV_IF_ADDR_ENTRY *DEV_Get_Next_By_Addr(UINT8 *addr);
DEV_IF_ADDR_ENTRY *DEV_Find_Target_Address(const DV_DEVICE_ENTRY *device,
                                           const UINT32 target_addr);
STATUS  DEV4_Delete_IP_From_Device(DV_DEVICE_ENTRY *, DEV_IF_ADDR_ENTRY *);
VOID    DEV_Remove_Device(DV_DEVICE_ENTRY *dev, UINT32 flags);
STATUS  DEV_Init_Devices(const DEV_DEVICE *devices, INT dev_count);
VOID    DEV_Resume_All_Open_Sockets(VOID);
VOID    DEV_Configure_Link_Local_Addr(DV_DEVICE_ENTRY *, UINT8 *);
UINT32  DEV_Generate_LL_Addr(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* DEV_H */

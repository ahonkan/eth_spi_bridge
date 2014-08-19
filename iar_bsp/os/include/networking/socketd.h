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
*       socketd.h
*
*   COMPONENT
*
*       Sockets
*
*   DESCRIPTION
*
*       Holds the defines for data structures related to sockets.
*
*   DATA STRUCTURES
*
*       id_struct
*       addr_struct
*       sockaddr_struct
*       sck_linger_struct
*       SCK_SOCKADDR
*       SCK_IP_ADDR_STRUCT
*       SCK_SOCKADDR_IP_STRUCT
*       SCK_TASK_ENT
*       ARP_REQUEST
*       sock_struct
*       TASK_TABLE_STRUCT
*       NU_Host_Ent
*       host
*       nu_fd_set
*       IP_MREQ
*       SCK_IOCTL_OPTION
*
*   DEPENDENCIES
*
*       os.h
*       sockdefs.h
*       target.h
*       dev.h
*
*************************************************************************/

#ifndef SOCKETD_H
#define SOCKETD_H

#include "networking/target.h"
#include "networking/dev.h"
#include "networking/sockdefs.h"   /* socket definitions */

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* 32-bit structure containing 4-digit ip number */
struct id_struct
{
    UINT8 is_ip_addrs[MAX_ADDRESS_SIZE];        /* IP address number */
};

struct addr_struct
{
    INT16    family;             /* family = INTERNET */
    UINT16   port;               /* machine's port number */
    struct   id_struct id;       /* contains the 4-digit ip number for the host machine */
    char     *name;              /* points to machine's name */
};

struct sockaddr_struct
{
    struct  id_struct ip_num;     /* the address = the ip num */
    UINT16  port_num;             /* the process = the port num */
    INT16   family;
};

/* Structure used for the linger socket option */
struct sck_linger_struct
{
    INT         linger_on;      /* Wait = NU_TRUE, don't wait = NU_FALSE */
    UNSIGNED    linger_ticks;   /* How many system ticks to wait. */
};

typedef struct SCK_SOCKADDR_STRUCT
{
    UINT8       sck_len;
    UINT8       sck_family;
    UINT8       sck_data[14];
}SCK_SOCKADDR;

struct SCK_IP_ADDR_STRUCT
{
    UINT32      sck_ip_addr;
};

struct SCK_TASK_ENT
{
    struct SCK_TASK_ENT             *flink;
    struct SCK_TASK_ENT             *blink;
    NU_TASK                         *task;
    NET_BUFFER_SUSPENSION_ELEMENT   *buff_elmt;
};

struct _tx_ancillary_data
{
    UINT16              tx_buff_length;
    UINT16              tx_flags;
    UINT8               *tx_source_address;         /* Pointer to the source address */
    UINT8               *tx_hop_limit;              /* Pointer to the hop limit */
    struct addr_struct  *tx_next_hop;               /* Pointer to the next-hop */
    UINT8               *tx_interface_index;        /* Pointer to the interface index */
    UINT8               *tx_traffic_class;
    UINT8               *tx_hop_opt;                /* Pointer to the extension headers */
    UINT8               *tx_dest_opt;               /* Pointer to the extension headers */
    UINT8               *tx_rthrdest_opt;           /* Destination Options preceding Routing Header */
    UINT8               *tx_route_header;           /* Pointer to the extension headers */
    UINT8               *tx_buff;                   /* Pointer to the data */
};

/* this is the socket 5-tuple */
struct sock_struct
{
  struct sockaddr_struct    s_local_addr;
  struct sockaddr_struct    s_foreign_addr;
  NET_BUFFER_HEADER         s_recvlist;     /* List of received packets. */
  NU_TASK                   *s_CLSTask;     /* Task pending on a transmit. */
  struct SCK_TASK_ENT       s_RXTask_List;  /* List of tasks pending on receive */
  struct SCK_TASK_ENT       s_TXTask_List;  /* List of tasks pending on transmit */

#if (INCLUDE_IPV4 == NU_TRUE)
  MULTI_SCK_OPTIONS         *s_moptions_v4; /* IP multicast options. */
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
  MULTI_SCK_OPTIONS         *s_moptions_v6; /* IP6 multicast options */
#endif

  struct sck_linger_struct  s_linger;       /* Socket linger settings */
  DV_DEVICE_ENTRY           *s_bcast_if;    /* default bcast IF */
  DV_DEVICE_ENTRY           *s_recv_if;     /* IF of last RX datagram */
  UINT32                    s_recvbytes;    /* Total bytes in s_recvlist. */
  UINT32                    s_recvpackets;  /* Total packets in s_recvlist. */
  struct TASK_TABLE_STRUCT  *s_accept_list; /* Established connections that have
                                               yet to be accepted. */
  INT                       s_accept_index;
  INT                       s_port_index;   /* Port number. */
  UINT16                    s_state;        /* Internal state flags. */
  UINT16                    s_options;      /* Socket options as defined by BSD.  Currently */
                                            /* the only implemented option is SO_BROADCAST. */
  UINT16                    s_flags;
  UINT16                    s_protocol;

  INT16                     s_family;
  UINT8                     padN[2];

  INT32                     s_error;
  UINT32                    s_struct_id;
  NET_BUFFER                *s_rx_ancillary_data;   /* Incoming ancillary data */
};

struct _msghdr
{
    struct addr_struct  *msg_name;          /* Pointer to the address structure */
    UINT16              msg_namelen;        /* Length of the address structure */
    CHAR                *msg_iov;           /* Pointer to buffer of data */
    UINT16              msg_iovlen;         /* Length of buffer of data */
    VOID                *msg_control;       /* Pointer to buffer of ancillary data */
    UINT16              msg_controllen;     /* Length of buffer of ancillary data */
    UINT16              flags;              /* Flags on received message */
};

struct _cmsghdr
{
    UINT16  cmsg_len;           /* Number of bytes, including this header */
    INT     cmsg_level;         /* Originating protocol */
    INT     cmsg_type;          /* Protocol-specific type */
    /* followed by CHAR cmsg_data[] */
};

typedef struct _in_pktinfo
{
    UINT32  ipi_ifindex;        /* Interface index */
    UINT8   ipi_spec_dst[4];    /* Local address */
    UINT8   ipi_addr[4];        /* Header Destination address */
} in_pktinfo;

#if (INCLUDE_IPV6 == NU_TRUE)

struct _in6_pktinfo
{
    UINT8   ipi6_addr[MAX_ADDRESS_SIZE];
    UINT32  ipi6_ifindex;
};

#endif

/* task table structure - created during an NU_Listen call to
   store status on x number of connections for a single port number
   from a single task id */

struct TASK_TABLE_STRUCT
{
  struct TASK_TABLE_STRUCT *next;  /* pointer to the next task structure */
  struct SCK_TASK_ENT       ssp_task_list; /* List of tasks suspended on
                                              a listening socket */
  INT     *stat_entry;      /* status of each connection */
  INT     *socket_index;    /* portlist entry number of each connection */
  INT     socketd;
  UINT16  local_port_num;   /* port number of server */
  UINT16  current_idx;      /* points to oldest entry in the table; a task
                              should service this connection before the others */
  UINT16  total_entries;    /* number of backlog queues possible */
  INT8    acceptFlag;       /* Used to indicate that the task is suspended in the
                              NU_Accept service. */
  INT8    pad[1];
};

/* host structure */
typedef struct NU_Host_Ent
{
  CHAR   *h_name;
  CHAR   **h_alias;        /* unused */
  INT16  h_addrtype;
  INT16  h_length;
  CHAR   **h_addr_list;

#define   h_addr  h_addr_list[0]   /* address, for backward compatibility */
} NU_HOSTENT;

/* Host information.  Used to match a host name with an address.  Used in
   hosts.c to setup information on foreign hosts. */
struct host
{
    CHAR    name[MAX_HOST_NAME_LENGTH];
    UINT8   address[MAX_ADDRESS_SIZE];
    INT16   h_family;
    UINT8   padN[2];
};

typedef struct nu_fd_set
{
    UINT32 words[FD_ELEMENTS];
} FD_SET;


/* Clear the connecting flag and set the connected flag. */
#define     SCK_CONNECTED(desc) \
        if (SCK_Sockets[desc])                              \
        {                                                   \
          SCK_Sockets[desc]->s_state &= (~SS_ISCONNECTING); \
          SCK_Sockets[desc]->s_state |= SS_ISCONNECTED;     \
        }

/* Change the the socket state to disconnecting. */
#define     SCK_DISCONNECTING(desc) \
            if ( (desc >= 0) && (SCK_Sockets[desc]) )               \
            {                                                       \
                SCK_Sockets[desc]->s_state &= (~SS_ISCONNECTED);    \
                SCK_Sockets[desc]->s_state |= SS_ISDISCONNECTING;   \
            }

/* IP Multicast Request structure. This structure is used when using
   NU_Setsockopt or NU_Getsockopt to set or get IP multicasting options. */
typedef struct _ip_mreq {
    UINT32      sck_multiaddr;      /* IP multicast address. */
    UINT32      sck_inaddr;         /* IP address of the interface. */
} IP_MREQ;


/*  Miscellaneous Defines for application layer interface to DEV, DHCP and BOOTP structures. */
#define  NU_DEVICE              DEV_DEVICE
#define  NU_BOOTP_STRUCT        BOOTP_STRUCT
#define  NU_DHCP_STRUCT         DHCP_STRUCT
#define  NU_Get_Host_Name       SCK_Get_Host_Name
#define  NU_Get_Domain_Name     SCK_Get_Domain_Name
#define  NU_Set_Host_Name       SCK_Set_Host_Name
#define  NU_Set_Domain_Name     SCK_Set_Domain_Name
#define  NU_Recv_IF_Addr        SCK_Recv_IF_Addr

/* This structure is used by the application layer in making an ioctl call to
   modify an existing ARP cache entry, create a new ARP cache entry, get an
   ARP cache entry or delete an ARP cache entry */
typedef struct arp_request ARP_REQUEST;

struct arp_request
{
    SCK_SOCKADDR    arp_pa;     /* Protocol Address */
    SCK_SOCKADDR    arp_ha;     /* Hardware Address */
    INT16           arp_flags;  /* Flags */

    UINT8           arp_pad[2];
};

/* Option commands for the NU_Ioctl service call. */
#define SIOCGIFADDR         1       /* Get the IP address associated with an
                                       interface. */
#define SIOCGIFDSTADDR      2       /* Get the IP address on the foreign side
                                       of a PPP link. Only valid for PPP links. */
#define SIOCSPHYSADDR       3       /* Set the MAC address associated with an
                                       interface */
#define SIOCSIFADDR         4       /* Set the IP address associated with an
                                       interface */
#define SIOCSARP            5       /* Add or modify an entry in the ARP cache */
#define SIOCDARP            6       /* Delete an entry from the ARP cache */
#define SIOCGARP            7       /* Get an entry from the ARP cache */
#define SIOCIFREQ           8       /* Interface specific IOCTL command */

#define SIOCGIFADDR_IN6     9       /* Get the IPv6 address associated with an
                                       interface. */
#define SIOCGIFDSTADDR_IN6  10      /* Get the IPv6 address on the foreign side
                                       of a PPP link. Only valid for PPP links. */
#define SIOCSETVLAN         11     /* Change the VLAN ID association for device */
#define SIOCGETVLAN         12     /* Get the VLAN ID association for device */

/****** Ethernet Controller Hardware Offloading Configuration Options ******/

/* These NU_IOCTL options are not related to SOCKET options, but since */
/* all the other NU_IOCTL options were defined here it made sense to   */
/* add the HW OFFLOAD options here as well.                            */

#define SIOCGHWCAP          13     /* Request hw offloading features */
#define SIOCSHWOPTS         14     /* Set HW Offloading Mode */

#define FIONREAD            15

#define SIOCGETVLANPRIO     16     /* Change the VLAN priority association for device */
#define SIOCSETVLANPRIO     17     /* Get the VLAN priority association for device */

#define SIOCGIFNETMASK      18      /* Get the subnet mask associated with an IP address */
#define SIOCICMPLIMIT       19      /* Set the ICMP error rate-limiting interval */

/* Extended option commands for the NU_Ioctl service call. */
#define SIOCLIFGETND        20      /* Get the MAC address associated with an IPv6 address. */

#define SIOCSIFHWADDR       21      /* Set the MAC Address of the Interface by using device Name*/
#define SIOCGIFHWADDR       22      /* Get the MAC Address of the Interface by using device Name*/

/* VLAN frame reception supports these sub-modes */

#define VLAN_RX_NORMAL       0x00    /* Allow normal (MAC Address) packet filtering */

#define VLAN_RX_NO_STRIP     0x01    /* Filter by VLAN group ID and leave VLAN tag field in place */

#define VLAN_RX_NORMAL_STRIP 0x02    /* Normal packet filtering and strip off VLAN tag field */

#define VLAN_RX_STRIP        0x03    /* Filter by VLAN group ID and strip off VLAN tag field */

typedef struct _SCK_IOCTL_OPTION
{
    UINT8       *s_optval;
    /* XXX - Temp patch to workaround bug dts0100655508 in NET.
     * XXX - Have converted union to a struct. */
    struct
    {
        ARP_REQUEST         arp_request;
        UINT8               s_ipaddr[MAX_ADDRESS_SIZE];
        UINT8               mac_address[DADDLEN];
        DV_REQ              s_dvreq;
        UINT16              vlan_id;
        UINT8               vlan_prio;
        UINT8               padN[3];
        UINT32              sck_bytes_pending;
    } s_ret;

    UINT8               s_optval_octet;
    UINT8               sck_max_msgs;
    UINT8               sck_padN[2];
    UINT32              sck_interval;

} SCK_IOCTL_OPTION;

#define NU_IOCTL_OPTION     SCK_IOCTL_OPTION

#define NET_ALIGNBYTES          7
#define NET_ALIGN(p)            (((UINT32)(p) + NET_ALIGNBYTES) &~ NET_ALIGNBYTES)

/* Function Prototypes */
INT  SCK_Check_Listeners(UINT16 port_num, INT16 family, const VOID *tcp_chk);
VOID SCK_Clear_Accept_Entry(const struct TASK_TABLE_STRUCT *task_entry, INT index);

/* External References */
extern struct sock_struct *SCK_Sockets[NSOCKETS];
extern        UINT32       SCK_Ticks_Per_Second;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* SOCKETD_H */

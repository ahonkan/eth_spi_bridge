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
* FILE NAME
*
*       dhcp.h
*
* DESCRIPTION
*
*       Definitions for creating DHCP resources.
*
* DATA STRUCTURES
*
*       DHCPLAYER
*       DHCP_STRUCT
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef _DHCP_H_
#define _DHCP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* delay in seconds to start with */
#define DHCP_DELAY_MASK                     3

#define DHCP_Q_MESSAGE_SIZE                 2           /* Size of messages exchanged
                                                           on the DHCP queue. */
#define ACCEPT_BIT                      0x01

/* DHCP Flags */
#define DHCP_BROADCAST_FLAG             0x8000

/* RFC 2132 Vendor Extensions. */
#define DHCP_PAD                        0
#define DHCP_NETMASK                    1
#define DHCP_TIME_OFFSET                2
#define DHCP_ROUTE                      3
#define DHCP_TIME                       4
#define DHCP_NAME_SERVER                5
#define DHCP_DNS                        6
#define DHCP_LOG_SERVER                 7
#define DHCP_COOKIE_SERVER              8
#define DHCP_LPR_SERVER                 9
#define DHCP_IMPRESS_SERVER             10
#define DHCP_RESOURCE_SERVER            11
#define DHCP_HOSTNAME                   12
#define DHCP_BOOT_FILE_SIZE             13
#define DHCP_MERIT_DUMP_FILE            14
#define DHCP_DOMAIN_NAME                15
#define DHCP_SWAP_SERVER                16
#define DHCP_ROOT_PATH                  17
#define DHCP_EXTENSIONS_PATH            18

/* IP Layer Parameters per Host. */
#define DHCP_IP_FORWARDING              19
#define DHCP_NL_SOURCE_ROUTING          20
#define DHCP_POLICY_FILTER              21
#define DHCP_MAX_DATAGRAM_SIZE          22
#define DHCP_IP_TIME_TO_LIVE            23
#define DHCP_MTU_AGING_TIMEOUT          24
#define DHCP_MTU_PLATEAU_TABLE          25

/* IP Layer Parameters per Interface. */
#define DHCP_INTERFACE_MTU              26
#define DHCP_ALL_SUBNETS                27
#define DHCP_BROADCAST_ADDR             28
#define DHCP_MASK_DISCOVERY             29
#define DHCP_MASK_SUPPLIER              30
#define DHCP_ROUTER_DISCOVERY           31
#define DHCP_ROUTER_SOLICI_ADDR         32
#define DHCP_STATIC_ROUTE               33

/* Link Layer Parameters per Interface. */
#define DHCP_TRAILER_ENCAP              34
#define DHCP_ARP_CACHE_TIMEOUT          35
#define DHCP_ETHERNET_ENCAP             36

/* TCP Parameters. */
#define DHCP_TCP_DEFAULT_TTL            37
#define DHCP_TCP_KEEPALIVE_TIME         38
#define DHCP_TCP_KEEPALIVE_GARB         39

/* Application and Service Parameters. */
#define DHCP_NIS_DOMAIN                 40
#define DHCP_NIS                        41
#define DHCP_NTP_SERVERS                42
#define DHCP_VENDOR_SPECIFIC            43
#define DHCP_NetBIOS_NAME_SER           44
#define DHCP_NetBIOS_DATA_SER           45
#define DHCP_NetBIOS_NODE_TYPE          46
#define DHCP_NetBIOS_SCOPE              47
#define DHCP_X11_FONT_SERVER            48
#define DHCP_X11_DISPLAY_MGR            49

#define DHCP_NIS_PLUS_DOMAIN            64
#define DHCP_NIS_PLUS_SERVERS           65
#define DHCP_MOBILE_IP_HOME             68
#define DHCP_SMTP_SERVER                69
#define DHCP_POP3_SERVER                70
#define DHCP_NNTP_SERVER                71
#define DHCP_WWW_SERVER                 72
#define DHCP_FINGER_SERVER              73
#define DHCP_IRC_SERVER                 74
#define DHCP_STREETTALK_SERVER          75
#define DHCP_STDA_SERVER                76

/* DHCP Extensions */
#define DHCP_REQUEST_IP                 50
#define DHCP_IP_LEASE_TIME              51
#define DHCP_OVERLOAD                   52
#define DHCP_MSG_TYPE                   53
#define DHCP_SERVER_ID                  54
#define DHCP_REQUEST_LIST               55
#define DHCP_MESSAGE                    56
#define DHCP_MAX_MSG_SIZE               57
#define DHCP_RENEWAL_T1                 58
#define DHCP_REBINDING_T2               59
#define DHCP_VENDOR_CLASS_ID            60
#define DHCP_CLIENT_CLASS_ID            61

#define DHCP_RFC4361_CLIENT_ID          255

#define DHCP_TFTP_SERVER_NAME           66
#define DHCP_BOOT_FILE_NAME             67

#define DHCP_END                        255

#define DHCPDISCOVER                    1
#define DHCPOFFER                       2
#define DHCPREQUEST                     3
#define DHCPDECLINE                     4
#define DHCPACK                         5
#define DHCPNAK                         6
#define DHCPRELEASE                     7
#define DHCPINFORM                      8

#define RETRIES_COUNT                   5

/* DHCP States */
#define DHCP_INIT_STATE                 1
#define DHCP_SELECTING_STATE            2
#define DHCP_BOUND_STATE                3
#define DHCP_REQUESTING_STATE           4
#define DHCP_RENEW_STATE                5
#define DHCP_REBIND_STATE               6

/* DHCP Options Offset */
#define DHCP_OPTION_CODE                0
#define DHCP_OPTION_LENGTH              1
#define DHCP_OPTION_DATA                2

#define DHCP_OPTION_HDR_LENGTH          2

/*
 * UDP port numbers, server and client.
 */
#define IPPORT_DHCPS                    67
#define IPPORT_DHCPC                    68
#define DHCP_LARGEST_OPT_SIZE           255
#define MAX_DHCP_TIMEOUT                63

/* magic cookie value that mark the beginning of vendor options part */
#define DHCP_COOKIE                     0x63825363UL

/* Size of fixed part of DHCP message header */
#define DHCP_PKT_FIXED_PART_LEN         236

/* Default maximum length of options field. Extra options would be stored in
 * 'sname' and 'file' fields via Options-Override option. If even more space
 * is required, then the client has to inform the server about the new maximum
 * message size via 'Maximum DHCP Message Size' option in the request message.
 * For that purpose, NU_Dhcp() parameters would have to be updated. So change
 * this value accordingly.  
 */
#define DHCP_PKT_OPTS_PART_LEN          312

struct _DHCPLAYER
{
    UINT8  dp_op;                           /* packet opcode type */
    UINT8  dp_htype;                        /* hardware addr type */
    UINT8  dp_hlen;                         /* hardware addr length */
    UINT8  dp_hops;                         /* gateway hops */
    UINT32 dp_xid;                          /* transaction ID */
    UINT16 dp_secs;                         /* seconds since boot began */
    UINT16 dp_flags;
    UINT8  dp_ciaddr[4];                    /* client IP address */
    UINT8  dp_yiaddr[4];                    /* 'your' IP address */
    UINT8  dp_siaddr[4];                    /* server IP address */
    UINT8  dp_giaddr[4];                    /* gateway IP address */
    UINT8  dp_chaddr[16];                   /* client hardware address */
    UINT8  dp_sname[64];                    /* server host name */
    UINT8  dp_file[128];                    /* boot file name */
    UINT8  dp_vend[DHCP_PKT_OPTS_PART_LEN]; /* vendor-specific area,
                                             * as of RFC2131 it is variable
                                             * length (default max length 312 octets).
                                             */
};
typedef struct _DHCPLAYER DHCPLAYER;

#define DHCP_PACKET_LEN     (sizeof(DHCPLAYER))


/* The following struct is used to pass in information to the NU_Dhcp    */
/* function call and also used to send information back to caller from   */
/* the DHCP server.                                                      */

struct dhcp_struct {
        UINT8  dhcp_siaddr[4];      /* DHCP server IP address */
        UINT8  dhcp_giaddr[4];      /* Gateway IP address */
        UINT8  dhcp_yiaddr[4];      /* Your IP address */
        UINT8  dhcp_net_mask[4];    /* Net mask for new IP address */
        UINT32 dhcp_xid;            /* Transaction ID. */
        UINT8  dhcp_mac_addr[6];    /* MAC address of client. */
        UINT8  padding1[2];
        UINT8  dhcp_sname[64];      /* optional server host name field. */
        UINT8  dhcp_file[128];      /* fully pathed filename */
        UINT8 *dhcp_opts;           /* options to be added to discover packet */
                /*   See RFC 2132 for valid options. */
        UINT8 dhcp_opts_len;        /* length in octets od opts field */
        UINT16 dhcp_secs;             /* seconds since boot began */
        UINT8  padding2[1];
};

typedef struct dhcp_struct DHCP_STRUCT;

/* The function prototypes known to the outside world. */
STATUS NU_Dhcp(DHCP_STRUCT *, const CHAR *);
STATUS NU_Dhcp_Release(const DHCP_STRUCT *, const CHAR *);
STATUS DHCP_Release_Address(const DHCP_STRUCT *, const CHAR *, UINT8);
VOID   DHCP_Queue_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
STATUS DHCP_Initialize(VOID);
INT    DHCP_Validate(const DHCP_STRUCT*, const CHAR*, const UINT8*);


#define DHCP_MAX_HEADER_SIZE \
   (IP_HEADER_LEN + UDP_HEADER_LEN + DHCP_PACKET_LEN)

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _DHCP_H_ */

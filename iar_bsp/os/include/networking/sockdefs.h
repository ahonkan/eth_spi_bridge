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

/***************************************************************************
*
*   FILENAME
*
*       sockdefs.h
*
*   COMPONENTS
*
*       Sockets
*
*   DESCRIPTION
*
*       This include file will define socket type error return codes, socket
*       options, and socket protocol types.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
***************************************************************************/

#ifndef SOCKDEFS_H
#define SOCKDEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus.h"

/* A generic catch-all for unused parameters. */
#define NU_NONE         0

/* Address family equates */
#define SK_FAM_UNSPEC   0               /* unspecified */
#define SK_FAM_LOCAL    1
#define SK_FAM_UNIX     SK_FAM_LOCAL
#define SK_FAM_IP       2               /* Internet:  UDP, TCP, etc. */
#define SK_FAM_ROUTE    17              /* Internal routing protocol */
#define SK_FAM_LINK     18              /* Link layer interface.     */
#define SK_FAM_IP6      28              /* IPv6 */

/* These equates are for backwards compatibility */
#define NU_FAMILY_UNIX      SK_FAM_UNIX        /* Unix */
#define NU_FAMILY_IP        SK_FAM_IP          /* Internet       - valid entry */
#define NU_FAMILY_IP6       SK_FAM_IP6         /* IPv6 */
#define NU_FAMILY_UNSPEC    SK_FAM_UNSPEC

/* TYPE equates */
#define NU_TYPE_STREAM    0     /* stream Socket             - valid entry */
#define NU_TYPE_DGRAM     1     /* datagram Socket           - valid entry */
#define NU_TYPE_RAW       2     /* raw Socket                - valid entry */
#define NU_TYPE_SEQPACKET 3     /* sequenced packet Socket */
#define NU_TYPE_RDM       4     /* reliably delivered msg Socket */

/* PROTOCOL equates */
#define NU_PROTO_INVALID  0
#define NU_PROTO_TCP      1
#define NU_PROTO_UDP      2
#define NU_PROTO_ICMP     3

/* Shutdown options - bit settings */
#define SHUT_RD           0x00000001    /* Close read-half */
#define SHUT_WR           0x00000002    /* Close write-half */
#define SHUT_RDWR         (SHUT_RD|SHUT_WR)

/* Bitmap for Nucleus Net initialized modules */
#define DHCP_CLIENT_MODULE  0x00000001

/* Select index defines */
#define SEL_READABLE_IDX    0
#define SEL_WRITABLE_IDX    1
#define SEL_MAX_FDSET       2

/* Socket multitask flags */
#define SCK_RES_BUFF        2

#define     NU_IGNORE_VALUE  -1 /* Null parameter value     */
#define     NULL_IP        0    /* Used to initialize ip addresses to NULL */

#if (INCLUDE_IPV6 == NU_TRUE)
#define MAX_ADDRESS_SIZE    16
#else
#define MAX_ADDRESS_SIZE    4
#endif

#if (INCLUDE_UDP == NU_FALSE)
#define UDP_MAX_PORTS       1
#endif

#if (INCLUDE_TCP == NU_FALSE)
#define TCP_MAX_PORTS       1
#endif

#if (INCLUDE_IP_RAW == NU_FALSE)
#define IPR_MAX_PORTS       1
#endif

/* Total number of socket descriptors. This should be
   TCP_MAX_PORTS + UDP_MAX_PORTS + IPR_MAX_PORTS */
#define NSOCKETS        (TCP_MAX_PORTS + UDP_MAX_PORTS + IPR_MAX_PORTS)

/*
 * Socket state bits.
 */
#define SS_NOFDREF              0x0001   /* no file table ref any more */
#define SS_ISCONNECTED          0x0002   /* socket connected to a peer */
#define SS_ISCONNECTING         0x0004   /* in process of connecting to peer */
#define SS_ISDISCONNECTING      0x0008   /* in process of disconnecting */
#define SS_DEVICEDOWN           0x0010   /* Used only by UDP sockets. Indicates
                                            that the device that was being used by
                                            a UDP socket/port has gone down. */
#define SS_TIMEDOUT             0x0020   /* The connection timed out */
#define SS_WAITWINDOW           0x0040   /* Used for the waiting window */
#define SS_CANTRCVMORE          0x0080   /* Indicate that a socket's read-half is
                                            closed. */
#define SS_CANTWRTMORE          0x0100   /* The socket's write-half is closed. */
/*
 *  Socket Flag bits.
 */
#define SF_BLOCK                0x0001  /* Indicates blocking or non-blocking */
#define SF_LISTENER             0x0002  /* Is a TCP server listening */
#define SF_ZC_MODE              0x0004  /* Set zerocopy mode for this socket */
#define SF_V4_MAPPED            0x0008
#define SF_BIND                 0x0010

/* Defines added for the NU_Select service call. */
#define FD_BITS                 32
#define FD_ELEMENTS     ((NSOCKETS/FD_BITS)+1)

#define SCK_EVENT_Q_ELEMENT_SIZE    3                       /* event queue element size
                                                               do not change the size */

/***************************  SOCKET OPTIONS  *****************************/
/* SOCKET OPTION control flags */
#define NU_SETFLAG        1
#define NU_BLOCK          1
#define NU_NO_BLOCK       0

/* {DCP 1-30-02} Added ZeroCopy mode flags */

#define NU_SET_ZC_MODE    2
#define NU_ZC_ENABLE      1
#define NU_ZC_DISABLE     0

#define DSCP_LOW_MIN     0
#define DSCP_LOW_MAX     7

#define DSCP_LOWHI_MIN   8
#define DSCP_LOWHI_MAX   15

#define DSCP_NORMAL_MIN  16
#define DSCP_NORMAL_MAX  23

#define DSCP_NORMALHI_MIN 24
#define DSCP_NORMALHI_MAX 31

#define DSCP_MEDIUM_MIN   32
#define DSCP_MEDIUM_MAX   39

#define DSCP_MEDIUMHI_MIN 40
#define DSCP_MEDIUMHI_MAX 47

#define DSCP_HIGH_MIN     48
#define DSCP_HIGH_MAX     55

#define DSCP_HIGHHI_MIN   56
#define DSCP_HIGHHI_MAX   63

/* Note when VLAN is enabled the three bit priority field is extracted from  */
/* the IP_TOS field set for the socket by use of the NU_Setsockopt function. */
/* The name IP_TOS should not cause confusion when using DiffServ.  The DSCP */
/* will be inserted into the IP_TOS field.  The VLAN priority is computed by */
/* shifting the DSCP value 3 positions to the right (dividing by 8).  This   */
/* correctly maps the 64 DSCP values to the 8 available VLAN priorities.     */

#define MAP_DSCP_VLAN_PRIORITY(x) (UINT8)((x) >> 3)



/* IP TOS FIELD Bit Layout | 7 | 6 5 4 3 | 2 1 0 | */
/*                           0 |  T O S  | V L P | */

/* TOS PRECEDENCE HAS BEEN REPLACED BY DIFFSERV VALUES PER RFC2474 */

#define IP_TOS_NORMAL_SERVICE 0x0
#define IP_TOS_MIN_DELAY      0x8   /* MIN-DELAY       service = 1 << 3 */
#define IP_TOS_MAX_THRUPUT    0x10  /* MAX-THRUPUT     service = 2 << 3 */
#define IP_TOS_MAX_REL        0x20  /* MAX-RELIABILITY service = 4 << 3 */
#define IP_TOS_MIN_DOLLARS    0x40  /* MIN-DOLLAR COST service = 8 << 3 */



/* PROTOCOL LEVELS */
#define NU_SOCKET_LEVEL   0


/* Levels used in the call to NU_Setsockopt */
#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_IGMP    2
#define IPPROTO_GGP     3
#define IPPROTO_TCP     6
#define IPPROTO_EGP     8
#define IPPROTO_PUP     12
#define IPPROTO_UDP     17
#define IPPROTO_IPV6    41
#define SOL_SOCKET      100

/* Protocol used int call to NU_Socket with a Raw IP socket */
#define IPPROTO_HELLO   63
#define IPPROTO_RAW     255
#define IPPROTO_OSPF    89

#define IS_RAW_PROTOCOL(next)      \
        (next == IPPROTO_RAW) || \
        (next == IPPROTO_HELLO)  || (next == IPPROTO_OSPF)

/* IPv6 Defines */
#define     IPPROTO_HOPBYHOP    0       /* Hop-by-Hop Options */
#define     IPPROTO_ROUTING     43      /* Routing Header */
#define     IPPROTO_FRAGMENT    44      /* Fragment Header */
#define     IPPROTO_ESP         50
#define     IPPROTO_AUTH        51
#define     IPPROTO_ICMPV6      58
#define     IPPROTO_NONEXTHDR   59
#define     IPPROTO_DEST        60      /* Destination Options Header */

/*
 * Options for use with [gs]etsockopt at the socket level.
 * First word of comment is data type; bool is stored in int.
 */
#define SO_BROADCAST        1  /* permission to transmit broadcast messages? */
#define SO_LINGER           2  /* linger on socket close */
#define SO_REUSEADDR        3  /* socket option to bind multiple addresses to
                                  the same port number. */
#define SO_RCVBUF           4  /* socket option to set the local TCP Window Size
                                  for a socket. */

/*
 * Options for use with [gs]etsockopt at the IP level.
 * First word of comment is data type; bool is stored in int.
 */
#define IP_OPTIONS          1    /* buf/ip_opts; set/get IP options */
#define IP_HDRINCL          2    /* int; header is included with data */
#define IP_TOS              3    /* int; IP type of service and precedence. */
#define IP_TTL              4    /* int; IP time to live */
#define IP_RECVOPTS         5    /* bool; receive all IP opts w/dgram */
#define IP_RECVRETOPTS      6    /* bool; receive IP opts for response */
#define IP_RECVDSTADDR      7    /* bool; receive IP dst addr w/dgram */
#define IP_RETOPTS          8    /* ip_opts; set/get IP options */
#define IP_MULTICAST_IF     9    /* u_char; set/get IP multicast i/f  */
#define IP_MULTICAST_TTL    10   /* u_char; set/get IP multicast ttl */
#define IP_MULTICAST_LOOP   11   /* u_char; set/get IP multicast loopback */
#define IP_ADD_MEMBERSHIP   12   /* ip_mreq; add an IP group membership */
#define IP_DROP_MEMBERSHIP  13   /* ip_mreq; drop an IP group membership */
#define IP_BROADCAST_IF     14   /* u_char; set/get IP broadcast IF */
#define IP_RECVIFADDR       15   /* bool; recv IP of IF of last RX datagram */

/* IPv6 socket options */
#define IPV6_UNICAST_HOPS   16
#define IPV6_MULTICAST_IF   17
#define IPV6_MULTICAST_HOPS 18
#define IPV6_MULTICAST_LOOP 19
#define IPV6_JOIN_GROUP     20
#define IPV6_LEAVE_GROUP    21
#define IPV6_RECVHOPLIMIT   22
#define IPV6_RECVRTHDR      23
#define IPV6_RECVHOPOPTS    24
#define IPV6_RECVDSTOPTS    25
#define IPV6_RECVTCLASS     26
#define IPV6_RECVPKTINFO    27
#define IPV6_PKTINFO        28
#define IPV6_HOPLIMIT       29
#define IPV6_NEXTHOP        30
#define IPV6_RTHDR          31
#define IPV6_HOPOPTS        32
#define IPV6_DSTOPTS        33
#define IPV6_RTHDRDSTOPTS   34
#define IPV6_TCLASS         35
#define IPV6_V6ONLY         36
#define IPV6_CHECKSUM       37

/* NOTE ...Please note that Options (optname)for use with [gs]etsockopt
 * at the UDP level use 38- 40 and are defined in udp.h.
 */

/* Additional socket options. */
#define IP_PKTINFO          41


#define IP_RECVIFADDR_OP    0x001
#define SO_REUSEADDR_OP     0x002  /* SO_REUSEADDR has been set on the socket */
#define SO_IPV6_PKTINFO_OP  0x004
#define SO_IPV6_HOPLIMIT_OP 0x008
#define SO_IPV6_TCLASS_OP   0x010
#define SO_IPV6_RTHDR_OP    0x020
#define SO_IPV6_HOPOPTS     0x040
#define SO_IPV6_DESTOPTS    0x080
#define SO_IPV6_V6ONLY      0x100  /* NU_FAMILY_IP6 socket for IPv6 comm. only */
#define SO_IPV6_CHECKSUM    0x200

#define SO_UDP_NOCHECKSUM   0x400
#define SO_IP_HDRINCL       0x800
#define SO_BROADCAST_OP     0x1000
#define SO_IP_PKTINFO_OP    0x2000

/*******************  SOCKET ERROR CODES  ****************************

 The range for Nucleus NET error codes is -251 to -500.

*/

#define NU_INVALID_PROTOCOL     -251    /*  Invalid network protocol */
#define NU_NO_DATA_TRANSFER     -252    /*  Data was not written/read
                                            during send/receive function */
#define NU_NO_PORT_NUMBER       -253    /*  No local port number was stored
                                            in the socket descriptor */
#define NU_NO_TASK_MATCH        -254    /*  No task/port number combination
                                            existed in the task table */
#define NU_NO_SOCKET_SPACE      -255    /*  The socket structure list was full
                                            when a new socket descriptor was
                                            requested */
#define NU_NO_ACTION            -256    /*  No action was processed by
                                            the function */
#define NU_NOT_CONNECTED        -257    /*  A connection has been closed
                                            by the network.  */
#define NU_INVALID_SOCKET       -258    /*  The socket ID passed in was
                                            not in a valid range.  */
#define NU_NO_SOCK_MEMORY       -259    /*  Memory allocation failed for
                                            internal sockets structure.  */
#define NU_INVALID_ADDRESS      -260    /*  An incomplete address was sent */
#define NU_NO_HOST_NAME         -261    /*  No host name specified in a  */
#define NU_RARP_INIT_FAILED     -262    /*  During initialization RARP failed. */
#define NU_BOOTP_INIT_FAILED    -263    /*  During initialization BOOTP failed. */
#define NU_INVALID_PORT         -264    /*  The port number passed in was
                                            not in a valid range. */
#define NU_NO_BUFFERS           -265    /*  There were no buffers to place */
                                        /*  the outgoing packet in. */
#define NU_NOT_ESTAB            -266    /*  A connection is open but not in
                                            an established state. */
#define NU_WINDOW_FULL          -267    /*  The foreign host's in window is
                                            full. */
#define NU_NO_SOCKETS           -268    /*  No sockets were specified. */
#define NU_NO_DATA              -269    /*  None of the specified sockets were
                                            data ready.  NU_Select. */



/* The following errors are reported by the NU_Setsockopt and NU_Getsockopt
   service calls. */
#define NU_INVALID_LEVEL        -270    /*  The specified level is invalid. */
#define NU_INVALID_OPTION       -271    /*  The specified option is invalid. */
#define NU_INVAL                -272    /*  General purpose error condition. */
#define NU_ACCESS               -273    /*  The attempted operation is not   */
                                        /*  allowed on the  socket           */
#define NU_ADDRINUSE            -274

#define NU_HOST_UNREACHABLE     -275    /*  Host unreachable */
#define NU_MSGSIZE              -276    /*  Packet is to large for interface. */
#define NU_NOBUFS               -277    /*  Could not allocate a memory buffer. */
#define NU_UNRESOLVED_ADDR      -278    /*  The MAC address was not resolved.*/
#define NU_CLOSING              -279    /*  The other side in a TCP connection*/
                                        /*  has sent a FIN */
#define NU_MEM_ALLOC            -280    /* Failed to allocate memory. */
#define NU_RESET                -281
#define NU_DEVICE_DOWN          -282    /* A device being used by the socket has
                                           gone down. Most likely because a PPP
                                           link has been disconnected or a DHCP
                                           IP address lease has expired. */
/* These error codes are returned by DNS. */
#define NU_INVALID_LABEL        -283    /* Indicates a domain name with an
                                           invalid label.                   */
#define NU_FAILED_QUERY         -284    /* No response received for a DNS
                                           Query. */
#define NU_DNS_ERROR            -285    /* A general DNS error status. */
#define NU_NOT_A_HOST           -286    /* The host name was not found. */
#define NU_INVALID_PARM         -287    /*  A parameter has an invalid value. */

#define NU_NO_DNS_SERVER        -288    /* No DNS server has been registered with
                                            the stack. */

/* Error codes for DHCP */
#define NU_DHCP_INIT_FAILED     -289    /*  During initialization DHCP failed. */
#define NU_DHCP_REQUEST_FAILED  -290

/*  Error codes for BOOTP */
#define NU_BOOTP_SEND_FAILED          -291
#define NU_BOOTP_RECV_FAILED          -292
#define NU_BOOTP_ATTACH_IP_FAILED     -293
#define NU_BOOTP_SELECT_FAILED        -294
#define NU_BOOTP_FAILED               -295

#define NU_NO_ROUTE_TO_HOST     -296    /* ICMP Destination Unreachable specific error */
#define NU_CONNECTION_REFUSED   -297    /* ICMP Destination Unreachable specific error */
#define NU_MSG_TOO_LONG         -298    /* ICMP Destination Unreachable specific error */

#define NU_BAD_SOCKETD          -299
#define NU_BAD_LEVEL            -300
#define NU_BAD_OPTION           -301

/* IPv6 Errors */
#define NU_DUP_ADDR_FAILED      -302
#define NU_DISCARD_PACKET       -303

/* ICMP Error Codes */
#define NU_DEST_UNREACH_ADMIN   -304
#define NU_DEST_UNREACH_ADDRESS -305
#define NU_DEST_UNREACH_PORT    -306
#define NU_TIME_EXCEED_HOPLIMIT -307
#define NU_TIME_EXCEED_REASM    -308
#define NU_PARM_PROB_HEADER     -309
#define NU_PARM_PROB_NEXT_HDR   -310
#define NU_PARM_PROB_OPTION     -311
#define NU_DEST_UNREACH_NET     -312
#define NU_DEST_UNREACH_HOST    -313
#define NU_DEST_UNREACH_PROT    -314
#define NU_DEST_UNREACH_FRAG    -315
#define NU_DEST_UNREACH_SRCFAIL -316
#define NU_PARM_PROB            -317
#define NU_SOURCE_QUENCH        -318

/* This macro will determine whether an error code is an ICMP error message -
 * if any ICMP error messages are added above, be sure to change this macro
 * to reflect the additions/changes.
 */
#define ICMP_ERROR_CODE(a)  ( (a <= NU_DEST_UNREACH_ADMIN) && (a >= NU_SOURCE_QUENCH) ) ? NU_TRUE : NU_FALSE

/* Error Code for NON Blocking  */
#define NU_WOULD_BLOCK          -319

/* Error Code for TCP Keep-Alive */
#define NU_CONNECTION_TIMED_OUT -320

/* Return status for NON Blocking connect */
#define NU_IS_CONNECTING        -321

#define NU_SOCKET_CLOSED        -322    /* The specified socket has been closed */

/* Error codes for event registration functions. */
#define NU_TABLE_FULL           -323
#define NU_NOT_FOUND            -324

/* IPv6 error codes for processing incoming extension headers */
#define NU_INVAL_NEXT_HEADER    -325
#define NU_SEND_ICMP_ERROR      -326

/* Error Codes for multicasting */
#define NU_MULTI_TOO_MANY_SRC_ADDRS -327
#define NU_NOT_A_GROUP_MEMBER       -328
#define NU_TOO_MANY_GROUP_MEMBERS   -329

#define NU_ETH_CABLE_UNPLUGGED      -330
#define NU_ETH_CABLE_PLUGGED_IN     -331
#define NU_DEVICE_NOT_DOWN          -332

struct SCK_SOCKADDR_IP_STRUCT
{
    UINT8           sck_len;
    UINT8           sck_family;
    UINT16          sck_port;
    UINT32          sck_addr;
    INT8            sck_unused[8];
};

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* SOCKDEFS_H */

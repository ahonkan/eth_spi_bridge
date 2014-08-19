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

/**************************************************************************
*
* FILENAME
*
*       bootp.h
*
* DESCRIPTION
*
*       This include file will handle bootstrap protocol defines -- RFC 951.
*
* DATA STRUCTURES
*
*       BOOTPLAYER
*       BOOTP_STRUCT
*       vend
*
* DEPENDENCIES
*
*       socketd.h
*
****************************************************************************/

#ifndef BOOTP_H
#define BOOTP_H

#include "networking/socketd.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define BOOTREQUEST     1
#define BOOTREPLY       2
/*
 * UDP port numbers, server and client.
 */
#define IPPORT_BOOTPS       67
#define IPPORT_BOOTPC       68

#define VM_STANFORD     "STAN"  /* v_magic for Stanford */

#define VM_RFC1048      "\143\202\123\143"


/*  BOOTP Vendor Extensions  */

#define BOOTP_PAD           0
#define BOOTP_SUBNET        1
#define BOOTP_TIMEOFF       2
#define BOOTP_GATEWAY       3
#define BOOTP_TIMESERV      4
#define BOOTP_NAMESERV      5
#define BOOTP_DNS_SERVER    6
#define BOOTP_LOGSERV       7
#define BOOTP_COOKSRV       8
#define BOOTP_LPRSRV        9
#define BOOTP_IMPRSRV       10
#define BOOTP_RLPSRV        11
#define BOOTP_HOSTNAME      12
#define BOOTP_BFILSZ        13
#define BOOTP_VEND_SPEC     43
#define BOOTP_END           255
#define BOOTP_COOKIE    { 0x63,0x82, 0x53, 0x63 }
#define BOOTP_LARGEST_OPT_SIZE       255

/* v_flags values */
#define VF_PCBOOT           1    /* an IBMPC or Mac wants environment info */
#define VF_HELP             2    /* help me, I'm not registered */
#define TAG_BOOTFILE_SIZE   13   /* tag used by vend fields rfc 1048 */

#define BOOTP_RETRIES       6    /* The maximum number of times bootp will send
                                  * a request before giving up.     */
#define MAX_BOOTP_TIMEOUT   63   /* The maximum time bootp will wait for a
                                  * response before retransmitting a request. */

#define BOOTP_HEADER_LEN    300

typedef struct bootp HUGE BOOTPLAYER;
struct bootp
{
    UINT8  bp_op;      /* packet opcode type */
    UINT8  bp_htype;   /* hardware addr type */
    UINT8  bp_hlen;    /* hardware addr length */
    UINT8  bp_hops;    /* gateway hops */
    UINT32  bp_xid;     /* transaction ID */
    UINT16 bp_secs;    /* seconds since boot began */
    UINT16 bp_unused;
    UINT8  bp_ciaddr[4];  /* client IP address */
    UINT8  bp_yiaddr[4];  /* 'your' IP address */
    UINT8  bp_siaddr[4];  /* server IP address */
    UINT8  bp_giaddr[4];  /* gateway IP address */
    UINT8  bp_chaddr[16];  /* client hardware address */
    UINT8  bp_sname[64];   /* server host name */
    UINT8  bp_file[128];   /* boot file name */
    UINT8  bp_vend[64];    /* vendor-specific area */
};


struct bootp_struct {
        UINT8 bp_ip_addr[4];                       /* r/s new IP address of client. */
        UINT8 bp_mac_addr[6];                      /* r MAC address of client. */
        UINT8 bp_sname[64];                        /* r optional server host name field. */
        UINT8 bp_file[128];                        /* r fully pathed filename */
        UINT8 bp_siaddr[4];                        /* r DHCP server IP address */
        UINT8 bp_giaddr[4];                        /* r Gateway IP address */
        UINT8 bp_yiaddr[4];                        /* r Your IP address */
        UINT8 bp_net_mask[4];                      /* r Net mask for new IP address */
        UINT8 bp_vend_opt[64];                     /* r Vendor-specific options */
        UINT8 padding[2];
};

typedef struct bootp_struct BOOTP_STRUCT;

/*
 * "vendor" data permitted for Stanford boot clients.
 */
struct vend
{
    UINT8  v_magic[4];      /* magic number */
    UINT32  v_flags;         /* flags/opcodes, etc. */
    UINT8  v_unused[56];    /* currently unused */
};

#define BOOTP_MAX_HEADER_SIZE \
  (IP_HEADER_LEN + UDP_HEADER_LEN + BOOTP_HEADER_LEN)

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif  /* BOOTP_H */

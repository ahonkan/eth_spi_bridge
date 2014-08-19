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
*   FILE NAME
*
*       udp.h
*
*   DESCRIPTION
*
*       This file contains the structure definitions required by the UDP
*       module of Nucleus NET.
*
*   DATA STRUCTURES
*
*       UDPLAYER
*       UDP_PORT
*       up_addr_struct
*
*   DEPENDENCIES
*
*       ip.h
*
***************************************************************************/

#ifndef UDP_H
#define UDP_H

#include "networking/ip.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/ip6.h"
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_api.h"
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/*************************************************************************/
/*  UDP
*   User Datagram Protocol
*   Each packet is an independent datagram, no sequencing information
*
*   UDP uses the identical checksum to TCP
*/

/* UDP Header Length */
#define UDP_HEADER_LEN    8

typedef struct udph
{
    UINT16 udp_src;         /* UDP source port number. */
    UINT16 udp_dest;        /* UDP destination port number. */
    UINT16 udp_length;      /* Length of packet, including hdr */
    UINT16 udp_check;       /* UDP checksum - optional */
} UDPLAYER;


#define UDP_SRC_OFFSET              0
#define UDP_DEST_OFFSET             2
#define UDP_LENGTH_OFFSET           4
#define UDP_CHECK_OFFSET            6

typedef struct _up_addr_struct
{
    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        UINT32      up_addrv4;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        UINT8       up_addrv6[IP6_ADDR_LEN];
#endif
    } up_addr;

    UINT16      up_family;
    UINT8       padN[2];
} up_addr_struct;

struct uport
{
#if (INCLUDE_IPSEC == NU_TRUE)

    /* The following variable specifies the packet selector for the last
     * received packet on this UDP port.
     */
    IPSEC_SELECTOR          up_ips_in_select;

    /* The following variable specifies the policy group which is
     * associated with the last interface which received a UDP packet
     * for this port.
     */
    IPSEC_POLICY_GROUP      *up_ips_in_group;

    /* The following pointer specifies the policy that matches the last
     * received packet on this port.
     */
    IPSEC_POLICY            *up_ips_in_policy;

    /* The following variable specifies the packet selector for the last
     * sent packet on this port.
     */
    IPSEC_SELECTOR          up_ips_out_select;

    /* The following variable specifies the policy group which is
     * associated with the last interface which sent a UDP packet
     * for this port.
     */
    IPSEC_POLICY_GROUP      *up_ips_out_group;

    /* The following pointer specifies the policy which has been used
     * while sending the last packet through this port.
     */
    IPSEC_POLICY            *up_ips_out_policy;

    /* The following pointer specifies the Outbound SA Bundle that was
     * used to apply security to the last sent packet.
     */
    IPSEC_OUTBOUND_BUNDLE   *up_ips_out_bundle;

#endif

    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        RTAB_ROUTE          up_route_v4;    /* A cached route. */
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        RTAB6_ROUTE         up_route_v6;
#endif
    } up_cache_route;

    up_addr_struct          up_laddr_struct;
    up_addr_struct          up_faddr_struct;

    UINT16                  up_lport;       /* Local port number */
    UINT16                  up_fport;       /* Foreign port number */
    INT                     up_socketd;     /* the socket associated with
                                               this port. */
#if (INCLUDE_IPV6 == NU_TRUE)
    tx_ancillary_data       *up_ancillary_data;
    tx_ancillary_data       *up_sticky_options;
#endif

    UINT16                  up_ttl;
    UINT8                   up_tos;
    UINT8                   padN[1];
};
typedef struct uport UDP_PORT;

#if (INCLUDE_IPV4 == NU_TRUE)
#define up_route    up_cache_route.up_route_v4
#define up_laddr    up_laddr_struct.up_addr.up_addrv4
#define up_faddr    up_faddr_struct.up_addr.up_addrv4
#else
#define up_route    up_cache_route.up_route_v6
#define up_laddr    up_laddr_struct.up_addr.up_addrv6
#define up_faddr    up_faddr_struct.up_addr.up_addrv6
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#define up_routev6  up_cache_route.up_route_v6
#define up_laddrv6  up_laddr_struct.up_addr.up_addrv6
#define up_faddrv6  up_faddr_struct.up_addr.up_addrv6
#endif

extern struct uport *UDP_Ports[UDP_MAX_PORTS];  /* allocate like iobuffers in UNIX */

/*
 * Options for use with [gs]etsockopt at the UDP level.
 */
#define UDP_NOCHECKSUM                 38   /* uint8; Checksum not included/checked */


/* Prototypes */
VOID   UDP_Init(VOID);
STATUS UDP_Port_Cleanup(UINT16 uport_index, struct sock_struct *);
INT16  UDP_Interpret(NET_BUFFER *, UDP_PORT *, INT, UINT8);
INT32  UDP_Recv_Data(INT, CHAR *, UINT16, struct addr_struct *);
INT32  UDP_Send_Data(INT socketd, CHAR *buff, UINT16 nbytes,
                     const struct addr_struct *to);
INT32  UDP_Make_Port(UINT16, INT);
STATUS UDP_Set_Opt(INT, INT, const VOID *, INT);
STATUS UDP_Get_Opt(INT, INT, VOID *, INT *);
STATUS UDP_Handle_Datagram_Error(INT16, const NET_BUFFER *, const UINT8 *,
                                 const UINT8 *, INT32);
INT32  UDP_Send_Datagram(INT, CHAR *, INT32);

/***** UDP_SNC.C *****/
VOID   UDP_Setsockopt_UDP_NOCHECKSUM(INT, INT16);

/***** UDP_GNC.C *****/
VOID   UDP_Getsockopt_UDP_NOCHECKSUM(INT, INT16 *, INT *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* INCLUDE_UDP */

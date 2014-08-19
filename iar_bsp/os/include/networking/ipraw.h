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
*       ipraw.h
*
*   DESCRIPTION
*
*       This file contains the structure definitions required by the
*       IPRAW module of Nucleus NET.
*
*   DATA STRUCTURES
*
*       IPR_PORT
*       IPRAW_ADDR_STRUCT
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef IPRAW_H
#define IPRAW_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/rtab6.h"
#endif

/* Maximum IP raw message size */
#define IMAXLEN (MTU  - IP_HEADER_LEN)

/* Maximum IPv6 raw message size */
/* Leave IP_HEADER_LEN in until the new buffer scheme is implemented */
#define IMAXLEN6 (MTU  - (IP6_HEADER_LEN + IP_HEADER_LEN))

typedef struct _ipraw_addr_struct
{
    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        UINT32      ipraw_addrv4;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        UINT8       ipraw_addrv6[IP6_ADDR_LEN];
#endif
    } ipraw_addr;

    UINT16      ipraw_family;
    UINT8       padN[2];
} ipraw_addr_struct;

/* PCB for IP Raw Datagrams */
struct iport
{
    union
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        RTAB_ROUTE          ip_route_v4;       /* A cached route. */
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        RTAB6_ROUTE         ip_route_v6;
#endif
    } iport_route;

    UINT32                  iportFlags;     /* What type of events are
                                               currently pending. */
    ipraw_addr_struct       ipraw_laddr_struct;
    ipraw_addr_struct       ipraw_faddr_struct;

    UINT16                  ip_lport;       /* Local port number */
    UINT16                  listen;         /* what port should this one
                                               listen to? */
    UINT16                  length;         /* how much data arrived in
                                               last packet? */
    UINT16                  ip_ttl;         /* The ttl associated with the
                                               socket associated with the port */
    UINT16                  ip_protocol;    /* Protocol number */
    UINT8                   ip_tos;
    UINT8                   padN[1];

    INT                     ip_socketd;     /* the socket associated with
                                               this port. */
#if (INCLUDE_IPV6 == NU_TRUE)
    INT                     ip_chk_off;
    tx_ancillary_data       *ip_sticky_options;
    tx_ancillary_data       *ip_ancillary_data;
#endif
};

typedef struct iport IPR_PORT;

#if (INCLUDE_IPV4 == NU_TRUE)
#define ip_laddr        ipraw_laddr_struct.ipraw_addr.ipraw_addrv4
#define ip_faddr        ipraw_faddr_struct.ipraw_addr.ipraw_addrv4
#define ipraw_route     iport_route.ip_route_v4
#else
#define ipraw_route     iport_route.ip_route_v6
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#define ip_laddrv6      ipraw_laddr_struct.ipraw_addr.ipraw_addrv6
#define ip_faddrv6      ipraw_faddr_struct.ipraw_addr.ipraw_addrv6
#define ipraw_routev6   iport_route.ip_route_v6
#endif

extern struct iport *IPR_Ports[IPR_MAX_PORTS];

/* IP Raw prototypes. */
VOID    IPRaw_Init(VOID);
STATUS  IPRaw_Interpret (NET_BUFFER *buf_ptr, INT index);
INT32   IPRAW_Recv_Data(INT, CHAR *, UINT16, struct addr_struct *);
INT32   IPRaw_Read (CHAR *, struct addr_struct *, struct sock_struct *, UINT16);
INT32   IPRAW_Send_Data(INT, CHAR *, UINT16, const struct addr_struct *);
INT32   IPRaw_Send (struct iport *, UINT8 *, UINT16, UINT16);
INT32   IPRaw_Make_Port(INT);
INT16   IPRaw_Get_PCB(INT socketd, const struct sock_struct *);


#ifdef          __cplusplus
}
#endif /* _cplusplus */


#endif /* IPRAW_H */

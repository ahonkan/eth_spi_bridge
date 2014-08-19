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
*       net.h
*
*   COMPONENT
*
*       MAC Layer
*
*   DESCRIPTION
*
*       This include file will handle defines relating to mac layer.
*
*   DATA STRUCTURES
*
*       DLAYER
*       NET_MULTI
*
*   DEPENDENCIES
*
*       None
*
***************************************************************************/

#ifndef NET_H
#define NET_H

#include "networking/netevent.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Standard GUID's for NET */
#define NETWORKING_LABEL   {0x42,0xb5,0xd3,0x8f,0x95,0x5e,0x4b,0xca,0x9a,0x9f,0xa0,0xb1,0x7a,0x1c,0x4b,0x34}

/* This structure is used in NET initialization */
typedef struct _NET_INIT
{
    VOID    *net_buffered_mem;  /* This memory pool will be used for
                                           NET buffers. On most platforms it
                                           is required that this memory pool
                                           be located in non-cached memory
                                           to avoid caching problems with
                                           NICs that used DMA. */
    VOID    *net_internal_mem;  /* This is where Net will draw all
                                           non-buffer memory */
}NU_NET_INIT_DATA;

/*
 * The MTU value depends on the hardware.  Ethernet has a maximum MTU of
 * 1500 bytes.
 */
#define ETHERNET_MTU    1500
#define MTU             ETHERNET_MTU

/*
*   Defines which have to do with Ethernet addressing.
*   Ethernet has 6 bytes of hardware address.
*/
#define DADDLEN         6               /* Length of Ethernet header in bytes */


#define  EIP     0x0800
#define  EARP    0x0806
#define  ERARP   0x8035         /* RARP */
#define  EVLAN   0x8100
#define  EIP6    0x86DD
#define  PPPOED  0x8863
#define  PPPOES  0x8864
#define  DOT1X   0x888E

/********************* Hardware Type Defines ***************************/
#define ETHER           0x0001   /*  Ethernet hardware type, needs swapping */
#define HARDWARE_TYPE   ETHER


/************************************************************************/
/*  Ethernet frames
*      All Ethernet transmissions should use this Ethernet header which
*   denotes what type of packet is being sent.
*
*   The header is 14 bytes.  The first 6 bytes are the target's hardware
*   Ethernet address, the second 6 are the sender's hardware address and
*   the last two bytes are the packet type.  Some packet type definitions
*   are included here.
*
*   the two-byte packet type is byte-swapped, PC is lo-hi, Ether is hi-lo
*/
typedef struct ether
{
    UINT8   dest[DADDLEN];         /* where the packet is going */
    UINT8   me[DADDLEN];           /* who am i to send this packet */
    UINT16  type;                  /* Ethernet packet type  */
} DLAYER;

#define ETHER_DEST_OFFSET 0
#define ETHER_ME_OFFSET (ETHER_DEST_OFFSET + DADDLEN)
#define ETHER_TYPE_OFFSET (ETHER_ME_OFFSET + DADDLEN)

#if (INCLUDE_IPV4 == NU_TRUE)

/* This macro determines if an ethernet address is a multicast ethernet address.
   It works on the most significant four bytes of the ethernet address.  */
#define NET_MULTICAST_ADDR(i)        (((long)(i) & 0xffffff00) == 0x01005e00)

/* This macro maps a multicast IP address to a multicast ethernet address. */
/* UINT8 *ip_addr
   UINT8 ether_addr[6]
*/
#define NET_MAP_IP_TO_ETHER_MULTI(ip_addr, ether_addr) \
{    \
    (ether_addr)[0] = 0x01; \
    (ether_addr)[1] = 0x00; \
    (ether_addr)[2] = 0x5e; \
    (ether_addr)[3] = (UINT8)(((UINT8 *)ip_addr)[1] & 0x7f); \
    (ether_addr)[4] = ((UINT8 *)ip_addr)[2]; \
    (ether_addr)[5] = ((UINT8 *)ip_addr)[3]; \
}

#endif

typedef struct _NET_MULTI NET_MULTI;
struct _NET_MULTI
{
    NET_MULTI       *nm_next;
    DV_DEVICE_ENTRY *nm_device;
    UINT32          nm_refcount;
    UINT8           nm_addr[6];
    UINT8           nm_pad[2];   /* Add padding to make life easier on platforms
                                   that require word alignment. */
};


/*  Global data structures declared in NET.C. */
extern const UINT8  NET_Ether_Broadaddr[DADDLEN];

STATUS ETH_Ether_Send(NET_BUFFER *, DV_DEVICE_ENTRY *, VOID *, VOID *);

STATUS ETH_Ether_Input (VOID);

VOID   NET_Resume (TQ_EVENT, UNSIGNED, UNSIGNED);
VOID   NET_Sleep(UINT32);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NET_H */

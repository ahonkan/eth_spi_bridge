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

/****************************************************************************
*
* FILENAME
*
*       target.h
*
* DESCRIPTION
*
*       This file will hold all of those defines and setups used by the
*       TCP/IP code which are processor/configuration dependent.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*
****************************************************************************/
#ifndef TARGET_H
#define TARGET_H

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"

#if ( (defined(INCLUDE_NU_POSIX)) && (INCLUDE_NU_POSIX) )

#include "posix/inc/posix.h"

#ifdef POSIX_1_17
#include "posix_rtl/inc/stdio.h"
#include "posix_rtl/inc/string.h"
#include "posix_rtl/inc/stdlib.h"

#else
#include "posix/rtl/inc/stdio.h"
#include "posix/rtl/inc/string.h"
#include "posix/rtl/inc/stdlib.h"
#endif

#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#include "networking/net_cfg.h"
#include "networking/isnmp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/********************* Nucleus NET Porting Section **************************/

/* This is the number of Nucleus PLUS timer ticks that occur per second. */
#ifndef NU_TICKS_PER_SECOND
#define NU_TICKS_PER_SECOND     ((UINT32)100)
#endif

/* Keep TICKS_PER_SECOND for backwards compatibility */
#define TICKS_PER_SECOND        NU_TICKS_PER_SECOND

/*
*  Setup the current limits and size type defs for the defined processor.
*  This will define constants for the sizes of integral types, with
*  minimum and maximum values allowed for the define.
*  Note: mins and maxs will be different for different processors.
*                               Size (Bytes)    Alignment (Bytes)
*/
/* Defining the constant so that the following data types should
   not be used, instead underlying OS data types should be used.
   This should be defined when underlying OS is one
   other than PLUS (for example, OSEK) */
/* #define PLUS_VERSION_COMP */ /* should be uncommented for OSEK */

#ifndef PLUS_VERSION_COMP

typedef char           INT8;    /*  1                   1   */
typedef unsigned char  UINT8;   /*  1                   1   */
typedef signed short   INT16;   /*  2                   2   */
typedef unsigned short UINT16;  /*  2                   2   */
typedef signed long    INT32;   /*  4                   4   */
typedef unsigned long  UINT32;  /*  4                   4   */

#endif /* PLUS_VERSION_COMP */

/* The PACKET definition controls how packets are sent.  If PACKET is defined
   then each packet is transmitted as soon as it is ready, waiting for
   the previous packet to complete transmission.  If PACKET is
   undefined the packets are placed into a transmit queue when they are
   ready and are then sent by transmit complete interrupts. PACKET undefined
   is the default and preferred mode of operation. Note that not all AT drivers
   support PACKET mode.

 */
#undef  PACKET

/* These macros are used for the versions of Nucleus NET that run in real-mode
   on the PC.  For all architectures other than rel-mode PC these should be
   defined to be nothing.
*/
#undef   FAR
#undef   HUGE
#define  FAR
#define  HUGE

/* Define this if using x86 real mode. */
#undef  NORM_PTR

/* The following definitions specify which routines have been implemented
 * in assembly language. A C implementation of each can be found in the
 * file TLS.C. An assembly implementation of tcpcheck may especially
 * increase performance. A NU_TRUE indicates an assembly implementation exists.
 */
#define IPCHECK_ASM     NU_FALSE
#define TCPCHECK_ASM    NU_FALSE
#define LONGSWAP_ASM    NU_FALSE
#define INTSWAP_ASM     NU_FALSE
#define COMPAREN_ASM    NU_FALSE

/* Map the 'C' library macros used by Nucleus NET and other networking
   protocol products to the actual functions supplied by Nucleus NET.
   If needed, these mappings can be changed to use a different set
   of functions, possibly from a customer/tool supplied 'C' library.
*/
#define NU_STRICMP      NCL_Stricmp
#define NU_ITOA         NCL_Itoa
#define NU_ULTOA        NCL_Ultoa
#define NU_ATOI         NCL_Atoi
#define NU_ATOL         NCL_Atol
#define NU_AHTOI        NCL_Ahtoi
#define NU_TOUPPER      NCL_To_Upper
#define NU_BLOCK_COPY   memcpy

/* Define the two macros that point to the NET swapping routines. These
   routines are endian proof. Meaning that no matter what the endianness
   of the target hardware is they will return data in the correct
   byte order. Since no swapping is required on big endian platforms
   these macros could be defined as nothing for a performance improvement.

   ex: #define LONGSWAP

   This will replace the LONGSWAPs in the code with nothing, thus using
   the value of the variable.
*/

#define LONGSWAP    TLS_Longswap

#define INTSWAP     TLS_Intswap


#ifndef CFG_NU_OS_NET_STACK_CFG_H
/* This macro specifies the required byte alignment for Nucleus NET packet
   buffers. Generally this value is dictated by the DMA requirements. For
   Example the DMA may only work to/from addresses on 8 byte boundaries. A
   value less than 4 should not be specified. */
#define REQ_ALIGNMENT           16
#else
#define REQ_ALIGNMENT           CFG_NU_OS_NET_STACK_REQ_ALIGNMENT
#endif

/********************* End Nucleus NET Porting Section **********************/


/* These typedef's are not target specific, but it is useful to have them here. */
typedef struct _RTAB_Route          RTAB_ROUTE;
typedef struct SCK_IP_ADDR_STRUCT   SCK_IP_ADDR;
typedef struct _DV_DEVICE_ENTRY     DV_DEVICE_ENTRY;
typedef struct _DEV_DEVICE          DEV_DEVICE;
typedef struct _DEV_MULTI_IGMP_LIST MULTI_DEV_IGMP_LIST;
typedef struct _DV_REQ              DV_REQ;
typedef struct _IP_MULTI            IP_MULTI;
typedef struct _MULTI_SCK_OPTIONS       MULTI_SCK_OPTIONS;
typedef struct _MULTI_DEV_STATE         MULTI_DEV_STATE;
typedef struct _MULTI_IP                MULTI_IP;
typedef struct _MULTI_SCK_STATE_LIST    MULTI_SCK_STATE_LIST;
typedef struct _MULTI_SCK_STATE         MULTI_SCK_STATE;
typedef struct sock_struct          SOCKET_STRUCT;
typedef struct SCK_SOCKADDR_IP_STRUCT SCK_SOCKADDR_IP;
typedef struct _tx_ancillary_data   tx_ancillary_data;
typedef struct _rx_ancillary_data   rx_ancillary_data;
typedef struct _msghdr              msghdr;
typedef struct _cmsghdr             cmsghdr;
typedef struct _in6_pktinfo         in6_pktinfo;
typedef struct _dev_if_addr_entry   DEV_IF_ADDR_ENTRY;

#if (INCLUDE_IPV6 == NU_TRUE)
typedef struct _IP6_MULTI                       IP6_MULTI;
typedef struct _DEV_MULTI_MLD_LIST              MULTI_DEV_MLD_LIST;
typedef struct _DV6_REQ                         DV6_REQ;
typedef struct nd_router_advert                 ND_ROUTER_ADVERT;
typedef struct SCK6_SOCKADDR_IP_STRUCT          SCK6_SOCKADDR_IP;
typedef struct _RTAB6_ROUTE                     RTAB6_ROUTE;
typedef struct _ip6_next_hop_list               IP6_NEXT_HOP_LIST;
typedef struct _ip6_dest_list                   IP6_DEST_LIST;
typedef struct _ip6_prefix_list                 IP6_PREFIX_LIST;
typedef struct _ip6_prefix_entry                IP6_PREFIX_ENTRY;
typedef struct _ip6_destination_cache_entry     IP6_DESTINATION_CACHE_ENTRY;
typedef struct _ip6_default_router_entry        IP6_DEFAULT_ROUTER_ENTRY;
typedef struct _ip6_eth_neighbor_cache_entry    IP6_ETH_NEIGHBOR_CACHE_ENTRY;
typedef struct _ip6_neighbor_cache_entry        IP6_NEIGHBOR_CACHE_ENTRY;
typedef struct _dest_cache_node                 DEST_CACHE_NODE;
typedef struct _ip6_dest_list_entry             IP6_DEST_LIST_ENTRY;
typedef struct _ip6_next_hop_list_entry         IP6_NEXT_HOP_LIST_ENTRY;
typedef struct _dev6_if_address                 DEV6_IF_ADDRESS;
typedef struct _dest_entry_list                 DEST_ENTRY_LIST;
#endif

#define GET64(bufferP, offset) \
  TLS_Get64((unsigned char *)bufferP, offset)

#define PUT64(bufferP, offset, value) \
  TLS_Put64((unsigned char *)bufferP, offset, (value))

#define GET32(bufferP, offset) \
  TLS_Get32((unsigned char *)bufferP, offset)

#define PUT32(bufferP, offset, value) \
  TLS_Put32((unsigned char *)bufferP, offset, (value))

#define GET16(bufferP, offset) \
  TLS_Get16((unsigned char *)bufferP, offset)

#define PUT16(bufferP, offset, value) \
  TLS_Put16((unsigned char *)bufferP, offset, (value))

#define GET8(bufferP, offset) \
  (((unsigned char *)(bufferP))[offset])

#define PUT8(bufferP, offset, value) \
  (((unsigned char *)(bufferP))[offset]) = (value)

#define PUT_STRING(dest, offset, src, size) \
   TLS_Put_String((unsigned char *)(dest), (offset), \
                    (unsigned char *)(src), (size))

#define GET_STRING(src, offset, dest, size) \
   TLS_Get_String((unsigned char *)(src), (offset), \
                    (unsigned char *)(dest), (size))

#define EQ_STRING(packet, offset, local, size) \
   TLS_Eq_String((unsigned char *)(packet), (offset), \
                   (unsigned char *)(local), (size))

/* This macro is used to remove warnings. */
#define UNUSED_PARAMETER(x)     NET_Unused_Parameter = ((UNSIGNED)(x))

/* Define Supervisor and User mode functions */
#if (!defined(NU_SUPERV_USER_MODE)) || (NU_SUPERV_USER_MODE < 1)
#if (defined(NU_IS_SUPERVISOR_MODE))
#undef NU_IS_SUPERVISOR_MODE
#endif
#define NU_IS_SUPERVISOR_MODE() (NU_TRUE)
#if (!(defined(NU_SUPERVISOR_MODE)))
#define NU_SUPERVISOR_MODE() ((void) 0)
#endif
#if (!(defined(NU_USER_MODE)))
#define NU_USER_MODE() ((void) 0)
#endif
#if (!(defined(NU_SUPERV_USER_VARIABLES)))
#define NU_SUPERV_USER_VARIABLES    /* Not a Supervisor/User kernel */
#endif
#endif /* NU_SUPERV_USER_MODE */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* TARGET_H */

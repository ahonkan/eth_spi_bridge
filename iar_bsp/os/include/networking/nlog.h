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
*       nlog.h
*
*   COMPONENT
*
*       Prototyping
*
*   DESCRIPTION
*
*       This file will hold the configuration parameters for logging network
*           parameters to the specific protocol arrays.
*
*   DATA STRUCTURES
*
*       NLOG_ENTRY
*
*   DEPENDENCIES
*
*       tcp.h
*       udp.h
*       icmp.h
*
*************************************************************************/

#ifndef NLOG_H
#define NLOG_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "networking/tcp.h"
#include "networking/udp.h"
#include "networking/icmp.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/icmp6.h"
#endif

/*
 * This area will be used for the prototyping of all the TCP/IP error
 * routines and functions.
 */
VOID NERRS_Clear_All_Errors (VOID);

#if ((INCLUDE_NET_ERROR_LOGGING == NU_TRUE) || (PRINT_NET_ERROR_MSG == NU_TRUE))
VOID NLOG_Error_Log (CHAR *, STATUS, const CHAR *, INT);
VOID NLOG_Error_Log_IS(CHAR *, STATUS, const CHAR *, INT);
#else
#define NLOG_Error_Log(a,b,c,d)
#define NLOG_Error_Log_IS(a,b,c,d)
#endif

/* Define the size of the elements in the nlog Q. Do not change! */
#define NLOG_QUEUE_ELEMENT_SIZE     1


/* These defines are used by the error stuff for severity */
#define NERR_INFORMATIONAL 0      /* error may cause some trivial problems */
#define NERR_RECOVERABLE   1      /* error is recoverable */
#define NERR_SEVERE        2      /* error will cause severe problems */
#define NERR_FATAL         3      /* error is fatal and not recoverable */

/* This macros will be used to signify whether the packet info is from a received
    packet or a sent packet */
#define NLOG_RX_PACK        0x01
#define NLOG_TX_PACK        0x02
#define NLOG_RETX_PACK      0x04


/* This structure is used to hold the IP, TCP, and/or UDP header information that
    has been received and transmitted.  The info will be stored as a character string,
    just as it would appear if it were printed. */
typedef struct NLOG_STRUCT
{
    CHAR        log_msg [NLOG_MAX_BUFFER_SIZE];
} NLOG_ENTRY;



VOID NLOG_Init(VOID);

#if ( ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) || (PRINT_IP_MSG == NU_TRUE)) \
      && (INCLUDE_IPV4 == NU_TRUE) )
VOID NLOG_IP_Info(IPLAYER *, UINT8);
#else
#define NLOG_IP_Info(a,b)
#endif

#if ((INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || (PRINT_TCP_MSG == NU_TRUE))
VOID NLOG_TCP_Info(TCPLAYER *, INT16, UINT8);
#else
#define NLOG_TCP_Info(a,b,c)
#endif

#if ((INCLUDE_UDP_INFO_LOGGING == NU_TRUE) || (PRINT_UDP_MSG == NU_TRUE))
VOID NLOG_UDP_Info(UDPLAYER *, UINT8);
#else
#define NLOG_UDP_Info(a,b)
#endif

#if ( ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE)) \
      && (INCLUDE_IPV4 == NU_TRUE) )
VOID NLOG_ARP_Info(ARP_LAYER *, UINT8);
#else
#define NLOG_ARP_Info(a,b)
#endif

#if ( ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE)) \
      && (INCLUDE_IPV4 == NU_TRUE) )
VOID NLOG_ICMP_Info(ICMP_LAYER *, UINT8);
#else
#define NLOG_ICMP_Info(a,b)
#endif

VOID NLOG_Time_Puts(CHAR *);
STATUS NLOG_Clear_All_Errors (VOID);

#if (INCLUDE_IPV6 == NU_TRUE)
#if ((INCLUDE_IP_INFO_LOGGING == NU_TRUE) || (PRINT_IP_MSG == NU_TRUE))
VOID NLOG_IP6_Info(IP6LAYER *, UINT8);
#else
#define NLOG_IP6_Info(a,b)
#endif

#if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE))
VOID NLOG_ICMP6_Info(ICMP6_LAYER *, UINT8);
#else
#define NLOG_ICMP6_Info(a,b)
#endif
#endif


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif  /* NLOG_H */



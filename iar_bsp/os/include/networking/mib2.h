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
* FILE NAME
*
*        mib2.h
*
* COMPONENT
*
*       MIB2 Statistics
*
* DESCRIPTION
*
*       Statistics for ICMP, Interfaces, IP, TCP and UDP.
*
* DATA STRUCTURES
*
*       MIB2_ICMP_STRUCT
*       MIB2_ETHERSTATS_STRUCT
*       MIB2_INTERFACE_HC_STRUCT
*       MIB2_INTERFACE_EXT_STRUCT
*       MIB2_RCV_ADDR_STRUCT
*       MIB2_RCV_ADDR_ROOT
*       MIB2_INTERFACE_STRUCT
*       MIB2_IF_STACK_STRUCT
*       MIB2_IF_STACK_ROOT
*       MIB2_LAYER_STRUCT
*       MIB2_IP_STRUCT
*       MIB2_TCP_STRUCT
*       MIB2_UDP_STRUCT
*
* DEPENDENCIES
*
*       snmp_prt.h
*
*************************************************************************/

#ifndef MIB2_H
#define MIB2_H

#if (INCLUDE_SR_SNMP == NU_FALSE)

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_prt.h"
#endif

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/*-----------------------------------------------------------------------
 * Constant Definitions
 *-----------------------------------------------------------------------*/

#define MIB2_LEAST_BCAST_BIT           1            /* LSB for Broadcast */
#define MIB2_REASM_MAX_SIZE            65535
#define MIB2_MAX_STRSIZE               256
#define MIB2_MAX_BUFINT                128
#define MIB2_MAX_OIDSIZE               128
#define MIB2_INT_DESC_LENGTH           48
#define MIB2_MAX_IF                    MAX_PORTS
#define MIB2_MAX_PADDRSIZE             DADDLEN
#define MIB2_MAX_NETADDRSIZE           IP_ADDR_LEN
#define MIB2_MAX_UDPLISTEN             2
#define MIB2_METRIC_NOT_USED           (UNSIGNED)-1
#define MIB2_UNSUCCESSFUL              -1
#define MIB2_VAN_JACOBSON              4
#define MIB2_NULL_OID                  {0,0}

/* The following macros define constants for Interface Types. */
#define MIB2_MAC_TYPE_OTHER            1
#define MIB2_TYPE_REGULAR_1822         2
#define MIB2_MAC_TYPE_HDH_1822         3
#define MIB2_MAC_TYPE_DDN_X25          4
#define MIB2_MAC_TYPE_RFC877_X25       5
#define MIB2_MAC_TYPE_ETHERNET_CSMACD  6
#define MIB2_MAC_TYPE_88023_CSMACD     7
#define MIB2_MAC_TYPE_88024_TOKENBUS   8
#define MIB2_MAC_TYPE_88025_TOKENRING  9
#define MIB2_MAC_TYPE_88026_MAN        10
#define MIB2_MAC_TYPE_SOFT_LOOPBACK    24
#define MIB2_MAC_TYPE_80211_WLAN       71
#define MIB2_TYPE_MP                   121
#define MIB2_TYPE_TUNNEL               131

/* The following macros define constants for Receive Address (storage) Type
   object in the Receive Address Table. */
#define MIB2_STORAGE_OTHER             1
#define MIB2_STORAGE_VOLATILE          2
#define MIB2_STORAGE_NONVOLATILE       3

/* The following macro is used to define 64 bit counter. */
#define MIB2_COUNTER_64                 1

#if( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) )
/*-----------------------------------------------------------------------
 * ICMP Group - Data Structures
 *-----------------------------------------------------------------------*/

typedef struct _MIB2_ICMP_STRUCT
{
    UINT32                  icmpInMsgs;
    UINT32                  icmpInErrors;
    UINT32                  icmpInDestUnreachs;
    UINT32                  icmpInTimeExcds;
    UINT32                  icmpInParmProbs;
    UINT32                  icmpInSrcQuenchs;
    UINT32                  icmpInRedirects;
    UINT32                  icmpInEchos;
    UINT32                  icmpInEchoReps;
    UINT32                  icmpInTimestamps;
    UINT32                  icmpInTimestampReps;
    UINT32                  icmpInAddrMasks;
    UINT32                  icmpInAddrMaskReps;
    UINT32                  icmpOutMsgs;
    UINT32                  icmpOutErrors;
    UINT32                  icmpOutDestUnreachs;
    UINT32                  icmpOutTimeExcds;
    UINT32                  icmpOutParmProbs;
    UINT32                  icmpOutSrcQuenchs;
    UINT32                  icmpOutRedirects;
    UINT32                  icmpOutEchos;
    UINT32                  icmpOutEchoReps;
    UINT32                  icmpOutTimestamps;
    UINT32                  icmpOutTimestampReps;
    UINT32                  icmpOutAddrMasks;
    UINT32                  icmpOutAddrMaskReps;
}MIB2_ICMP_STRUCT;

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) ) */

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )

/*-----------------------------------------------------------------------
 * Interfaces Group - Data Structures
 *-----------------------------------------------------------------------*/

typedef struct _MIB2_ETHERSTATS_STRUCT
{
#if (INCLUDE_SNMP == NU_TRUE)
#if (INCLUDE_MIB_RMON1 == NU_TRUE)
/*
 * Support for RFC1757, EtherStats Group
 */
    UINT32   Octets;
    UINT32   Pkts;
    UINT32   BroadcastPkts;
    UINT32   MulticastPkts;
    UINT32   CRCAlignErrors;
    UINT32   UndersizePkts;
    UINT32   OversizePkts;
    UINT32   Fragments;
    UINT32   Jabbers;
    UINT32   Collisions;
    UINT32   Pkts64Octets;
    UINT32   Pkts65to127Octets;
    UINT32   Pkts128to255Octets;
    UINT32   Pkts256to511Octets;
    UINT32   Pkts512to1023Octets;
    UINT32   Pkts1024to1518Octets;
/*
 * Support for RFC1757, Host, Stats, Matrix groups
 */
    UINT32  LostPkts;
    UINT32  DiscardedPkts;
    UINT32  OutPkts;
    UINT32  OutOctets;
    UINT32  OutMulticastPkts;
    UINT32  OutBroadcastPkts;
    UINT32  OutErrors;
#endif /* (INCLUDE_MIB_RMON1 == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */

/*
 * Support for RFC1213, Interfaces Group
 */
    UINT32  outErrors;
    UINT32  outDiscards;
    UINT32  outNUcastPkts;
    UINT32  outUcastPkts;
    UINT32  outOctets;
    UINT32  inUcastPkts;
    UINT32  inNUcastPkts;
    UINT32  inUnknownProtos;
    UINT32  inErrors;
    UINT32  inDiscards;
    UINT32  inOctets;
}MIB2_ETHERSTATS_STRUCT;

#if (INCLUDE_IF_EXT == NU_TRUE)

/* This structure is used when High-Capacity Counters are required. The
   lower 32 bits of data for these counters is placed in the corresponding
   Counter32 Variables. */
typedef struct _MIB2_INTERFACE_HC_STRUCT
{
    UINT32                  mib2_hc_in_octets;
    UINT32                  mib2_hc_in_ucast_pkts;
    UINT32                  mib2_hc_in_multicast_pkts;
    UINT32                  mib2_hc_in_broadcast_pkts;
    UINT32                  mib2_hc_out_octets;
    UINT32                  mib2_hc_out_ucast_pkts;
    UINT32                  mib2_hc_out_multicast_pkts;
    UINT32                  mib2_hc_out_broadcast_pkts;

}MIB2_INTERFACE_HC_STRUCT;

/* This structure extends the Interfaces Table. */
typedef struct _MIB2_INTERFACE_EXT_STRUCT
{
    /* High-Capacity counters. */
    MIB2_INTERFACE_HC_STRUCT *mib2_hc;

    UINT32                   mib2_in_multicast_pkts;
    UINT32                   mib2_in_broadcast_pkts;
    UINT32                   mib2_out_multicast_pkts;
    UINT32                   mib2_out_broadcast_pkts;
    UINT32                   mib2_high_speed;
    UINT32                   mib2_counter_discontinuity;
    UINT8                    mib2_alias[64];
    UINT8                    mib2_linkupdown_trap_enable;
    UINT8                    mib2_connector_present;

    /* Making the structure word-aligned. */
    UINT8                    mib2_pad[2];

}MIB2_INTERFACE_EXT_STRUCT;

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

/* The interfaces group. */
typedef struct _MIB2_INTERFACE_STRUCT
{
    VOID                        *hostcontrol;
    VOID                        *matrixcontrol;
    VOID                        *channel;
    MIB2_ETHERSTATS_STRUCT      *eth;

#if (INCLUDE_IF_EXT == NU_TRUE)
    MIB2_INTERFACE_EXT_STRUCT   *mib2_ext_tbl;
#endif /* (INCLUDE_IF_EXT == NU_TRUE) */
    UINT32                      speed;
    UINT32                      lastChange;
    UINT32                      frameId;
    UINT16                      type;
    INT16                       statusAdmin;
    INT16                       statusOper;
    UINT16                      addrLength;
    UINT16                      addrHostLen;

    /* Making the structure word-aligned. */
    UINT8                       pad[2];

    UINT8                       descr[MIB2_INT_DESC_LENGTH];
    UINT8                       addrBroadcast[16];
    UINT8                       addrHost[8];

} MIB2_INTERFACE_STRUCT;

#define MIB2_IF_OPER_STAT_UP            1
#define MIB2_IF_OPER_STAT_DOWN          2
#define MIB2_IF_OPER_STAT_TEST          3
#define MIB2_IF_OPER_STAT_UNKNOWN       4
#define MIB2_IF_OPER_STAT_DORMANT       5
#define MIB2_IF_OPER_STAT_NOT_PRES      6
#define MIB2_IF_OPER_STAT_LOWER_DOWN    7

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) ) */


/*-----------------------------------------------------------------------
 * Interfaces Stack Group - Data Structures
 *-----------------------------------------------------------------------*/
#if (INCLUDE_IF_STACK == NU_TRUE)

/* The following two structures define the Stack Table. This
   is used to define the Layering for the interfaces. */
typedef struct _MIB2_IF_STACK_STRUCT
{
    struct _MIB2_IF_STACK_STRUCT *mib2_flink;
    struct _MIB2_IF_STACK_STRUCT *mib2_blink;

    DV_DEVICE_ENTRY         *mib2_higher_layer;
    DV_DEVICE_ENTRY         *mib2_lower_layer;
#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )
    UINT8                   mib2_row_status;

    /* Making the structure word-aligned. */
    UINT8                   mib2_pad[3];
#endif

}MIB2_IF_STACK_STRUCT;

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )

#define MIB2_IF_STACK_ACTIVATE(mib2_if_stack_ptr) \
    (mib2_if_stack_ptr)->mib2_row_status = SNMP_ROW_ACTIVE

#define IF_MIB2_IF_STACK_IS_ACTIVE(mib2_if_stack_ptr) \
    if((mib2_if_stack_ptr)->mib2_row_status == SNMP_ROW_ACTIVE)

#define MIB2_IF_STACK_NOT_ACTIVE(if_stack_ptr) \
    if((mib2_if_stack_ptr)->mib2_row_status != SNMP_ROW_ACTIVE)

#else /* ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) ) */

#define MIB2_IF_STACK_ACTIVATE(mib_if_stack_ptr)
#define IF_MIB2_IF_STACK_IS_ACTIVE(mib2_if_stack_ptr)
#define MIB2_IF_STACK_NOT_ACTIVE(if_stack_ptr) \
    if (if_stack_ptr == NU_NULL)

#endif /* ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) ) */

typedef struct _MIB2_IF_STACK_ROOT
{
    struct _MIB2_IF_STACK_STRUCT *mib2_flink;
    struct _MIB2_IF_STACK_STRUCT *mib2_blink;

    UINT32                  mib2_stack_last_change;

}MIB2_IF_STACK_ROOT;

#endif /* (INCLUDE_IF_STACK == NU_TRUE) */


#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE))
/*-----------------------------------------------------------------------
 * IP Group - Data Structures
 *-----------------------------------------------------------------------*/

/*
 * IP Statistics Table
 */

typedef struct _MIB2_IP_STRUCT
{
    UINT32                  ipInReceives;
    UINT32                  ipInHdrErrors;
    UINT32                  ipInAddrErrors;
    UINT32                  ipForwDatagrams;
    UINT32                  ipInUnknownProtos;
    UINT32                  ipInDiscards;
    UINT32                  ipInDelivers;
    UINT32                  ipOutRequests;
    UINT32                  ipOutDiscards;
    UINT32                  ipOutNoRoutes;
    UINT32                  ipReasmTimeout;
    UINT32                  ipReasmReqds;
    UINT32                  ipReasmOKs;
    UINT32                  ipReasmFails;
    UINT32                  ipFragOKs;
    UINT32                  ipFragFails;
    UINT32                  ipFragCreates;
    UINT32                  ipRoutingDiscards;
} MIB2_IP_STRUCT;

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE)) */

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_TCP == NU_TRUE))

/*-----------------------------------------------------------------------
 * TCP group  - Data Structures
 *-----------------------------------------------------------------------*/

typedef struct _MIB2_TCP_STRUCT
{
    UINT32                  tcpRtoAlgorithm;
    UINT32                  tcpActiveOpens;
    UINT32                  tcpPassiveOpens;
    UINT32                  tcpAttemptFails;
    UINT32                  tcpEstabResets;
    UINT32                  tcpCurrEstab;
    UINT32                  tcpInSegs;
    UINT32                  tcpOutSegs;
    UINT32                  tcpRetransSegs;
    UINT32                  tcpInErrs;
    UINT32                  tcpOutRsts;
} MIB2_TCP_STRUCT;

#endif  /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_TCP == NU_TRUE)) */


#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_UDP == NU_TRUE))
/*-----------------------------------------------------------------------
 * The UDP group - Data Structures
 *-----------------------------------------------------------------------*/

typedef struct _MIB2_UDP_STRUCT
{
    UINT32                  udpInDatagrams;
    UINT32                  udpNoPorts;
    UINT32                  udpInErrors;
    UINT32                  udpOutDatagrams;
} MIB2_UDP_STRUCT;

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_UDP == NU_TRUE)) */

/*-----------------------------------------------------------------------
 * External Variables
 *-----------------------------------------------------------------------*/

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) )
extern MIB2_ICMP_STRUCT             Mib2_Icmp_Data;
#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) ) */

#if (INCLUDE_IF_STACK == NU_TRUE)
extern MIB2_IF_STACK_ROOT           MIB2_Interface_Stack_Root;
#endif /* (INCLUDE_IF_STACK == NU_TRUE) */

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE))
extern MIB2_IP_STRUCT               Mib2_Ip_Data;
#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE)) */

#if ( (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) )
extern MIB2_TCP_STRUCT              Mib2_Tcp_Data;
#endif /* ( (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) ) */

#if ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) )
extern MIB2_UDP_STRUCT              Mib2_Udp_Data;
#endif /* ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) ) */

/*-----------------------------------------------------------------------
 * MIB-II Functional Declarations
 *-----------------------------------------------------------------------*/

STATUS MIB2_Init                        (VOID);

/*-----------------------------------------------------------------------
 * Interfaces Groups - Function Declarations
 *-----------------------------------------------------------------------*/

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )

STATUS  MIB2_Interface_Init             (DV_DEVICE_ENTRY *dev_ptr);
UINT16  MIB2_Interface_Count            (VOID);
MIB2_INTERFACE_STRUCT *MIB2_Interface_Get_Next(UINT32 index);
INT16 MIB2_Get_Next_Device_By_Index(UINT32 *index);
INT16 MIB2_Get_Next_Phy_Device(UINT32 *if_index);
INT16 MIB2_Get_ifSpecific(VOID);

#if (INCLUDE_IF_EXT == NU_TRUE)

/* Interface Extension Table. */
STATUS  MIB2_Init_Interface_Ext         (DV_DEVICE_ENTRY *dev_ptr);

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

INT16 MIB2_Get_Next_If_Address(UINT32 *if_index, UINT8 *recv_addr);
INT16 MIB2_Get_If_Address(UINT32 if_index, const UINT8 *recv_addr);

INT16  MIB2_Get_IfOutQLen                (UINT32 index, UINT32 *length);
INT16  MIB2_Add_IfOutQLen                (UINT32 index, INT32 length, INT8 reset);
INT16  MIB2_Get_IfOutErrors              (UINT32 index, UINT32 *errors);
INT16  MIB2_Add_IfOutErrors              (UINT32 index, UINT32 errors, INT8 reset);
INT16  MIB2_Get_IfOutDiscards            (UINT32 index, UINT32 *discards);
INT16  MIB2_Add_IfOutDiscards            (UINT32 index, UINT32 discards, INT8 reset);
INT16  MIB2_Get_IfOutNUcastPkts          (UINT32 index, UINT32 *packets);
INT16  MIB2_Add_IfOutNUcastPkts          (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfOutUcastPkts           (UINT32 index, UINT32 *packets, UINT8 counter_type);
INT16  MIB2_Add_IfOutUcastPkts           (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfOutOctets              (UINT32 index, UINT32 *octets, UINT8 counter_type);
INT16  MIB2_Add_IfOutOctets              (UINT32 index, UINT32 octets, INT8 reset);
INT16  MIB2_Get_IfInUcastPkts            (UINT32 index, UINT32 *packets, UINT8 counter_type);
INT16  MIB2_Add_IfInUcastPkts            (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfInNUcastPkts           (UINT32 index, UINT32 *packets);
INT16  MIB2_Add_IfInNUcastPkts           (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfInUnknownProtos        (UINT32 index, UINT32 *packets);
INT16  MIB2_Add_IfInUnknownProtos        (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfInErrors               (UINT32 index, UINT32 *errors);
INT16  MIB2_Add_IfInErrors               (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfInDiscards             (UINT32 index, UINT32 *discards);
INT16  MIB2_Add_IfInDiscards             (UINT32 index, UINT32 packets, INT8 reset);
INT16  MIB2_Get_IfInOctets               (UINT32 index, UINT32 *octets, UINT8 counter_type);
INT16  MIB2_Add_IfInOctets               (UINT32 index, UINT32 octets, INT8 reset);
INT16  MIB2_Get_IfDescr                  (UINT32 index, UINT8 *string, UINT32 *length);
INT16  MIB2_Set_IfDescr                  (UINT32 index, UINT8 *string);
INT16  MIB2_Get_IfType                   (UINT32 index, INT32 *type);
INT16  MIB2_Set_IfType                   (UINT32 index, INT32 type);
INT16  MIB2_Get_IfaceIndex               (const CHAR *name);
INT16  MIB2_Get_IfIndex                  (UINT32 index);
INT16  MIB2_Get_IfMtu                    (UINT32 index, UINT32 *octets);
INT16  MIB2_Set_IfMtu                    (UINT32 index, UINT32 octets);
INT16  MIB2_Get_IfSpeed                  (UINT32 index, UINT32 *speed);
INT16  MIB2_Set_IfSpeed                  (UINT32 index, UINT32 speed);
INT16  MIB2_Get_IfLastChange             (UINT32 index, UINT32 *time);
INT16  MIB2_Set_IfLastChange             (UINT32 index, UINT32 time);
INT16  MIB2_Get_IfStatusAdmin            (UINT32 index, INT32 *state);
INT16  MIB2_Set_IfStatusAdmin            (UINT32 index, INT32 value);
INT16  MIB2_Get_IfStatusOper             (UINT32 index, INT32 *state);
INT16  MIB2_Set_IfStatusOper             (DV_DEVICE_ENTRY *dev_ptr, INT32 value);
INT16  MIB2_Get_IfAddr                   (UINT32 index, UINT8 *address, UINT32 *length);
INT16  MIB2_Set_IfAddr                   (UINT32 index, UINT8 *address);
INT16  MIB2_Get_IfChannel                (UINT32 index, VOID **channel);
INT16  MIB2_Set_IfChannel                (UINT32 index, VOID *channel);
INT16  MIB2_Get_IfHostControl            (UINT32 index, VOID **hostcontrol);
INT16  MIB2_Set_IfHostControl            (UINT32 index, VOID *hostcontrol);
INT16  MIB2_Get_IfMatrixControl          (UINT32 index, VOID **matrixcontrol);
INT16  MIB2_Set_IfMatrixControl          (UINT32 index, VOID *matrixcontrol);
INT16  MIB2_Get_IfFrameId                (UINT32 index, UINT32 *frameid);
INT16  MIB2_Inc_IfFrameId                (UINT32 index);

#if (INCLUDE_IF_EXT == NU_TRUE)

INT16  MIB2_Get_Next_IfIndex_Ext         (UINT32 *index);
INT16  MIB2_Get_IfName                   (UINT32 index, CHAR *if_name);
INT16  MIB2_Get_IfLinkTrapEnable         (UINT32 index, UINT32 *if_link_trap_enable);
INT16  MIB2_Set_IfLinkTrapEnable         (UINT32 index, UINT32 if_link_trap_enable);
INT16  MIB2_Get_IfHighSpeed              (UINT32 index, UINT32 *if_high_speed);
INT16  MIB2_Get_IfConnectorPresent       (UINT32 index, UINT32 *if_connector_present);
INT16  MIB2_Get_IfAlias                  (UINT32 index, CHAR *if_alias);
INT16  MIB2_Set_IfAlias                  (UINT32 index, const CHAR *if_alias);
INT16  MIB2_Get_IfCounterDiscontinTime   (UINT32 index, UINT32 *if_discontinuity_time);
/* Higher Counter interface. */
INT16 MIB2_Get_IfInBrdcastPkts(UINT32 index, UINT32 *packets, UINT8 counter_type);
INT16 MIB2_Add_IfInBrdcastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets, INT8 reset);
INT16 MIB2_Get_IfOutBrdcastPkts(UINT32 index, UINT32 *packets, UINT8 counter_type);
INT16 MIB2_Add_IfOutBrdcastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets, INT8 reset);
INT16 MIB2_Get_IfInMulticastPkts(UINT32 index, UINT32 *packets, UINT8 counter_type);
INT16 MIB2_Add_IfInMulticastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets, INT8 reset);
INT16 MIB2_Get_IfOutMulticastPkts(UINT32 index, UINT32 *packets, UINT8 counter_type);
INT16 MIB2_Add_IfOutMulticastPkts(const DV_DEVICE_ENTRY *dev, UINT32 packets, INT8 reset);
INT16 MIB2_Get_IfPromiscMode(UINT32 index, UINT32 *mode);

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) ) */

/*-----------------------------------------------------------------------
 * Interfaces Stack Groups - Function Declarations
 *-----------------------------------------------------------------------*/
#if (INCLUDE_IF_STACK == NU_TRUE)

/* Stack Table. */
STATUS MIB2_If_Stack_Add_Entry            (const MIB2_IF_STACK_STRUCT *node);
STATUS MIB2_If_Stack_Remove_Entry         (MIB2_IF_STACK_STRUCT *node);
VOID MIB2_If_Stack_Remove_Dev             (const DV_DEVICE_ENTRY * dev);
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Entry(UINT32 higher_interface,
                                                  UINT32 lower_interface);
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_HI_Entry(UINT32 higher_interface, UINT8 active);
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_LI_Entry(UINT32 lower_interface, UINT8 active);
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Next_Entry(UINT32 higher_interface,
                                                   UINT32 lower_interface);
MIB2_IF_STACK_STRUCT *MIB2_If_Stack_Get_Next_Entry2(const MIB2_IF_STACK_STRUCT *stack_entry,
                                                    UINT8 active);

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE))

INT16  MIB2_Get_IfStack(UINT32 higher_if_index, UINT32 lower_if_index);
INT16  MIB2_Get_Next_IfStack(UINT32 *higher_if_index, UINT32 *lower_if_index);
INT16  MIB2_Get_IfStack_Row_Status(UINT32 higher_if_index, UINT32 lower_if_index, UINT8 *row_status);

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE)) */

#endif /* (INCLUDE_IF_STACK == NU_TRUE) */

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE))

INT16 MIB2_Get_IpAdEntAddr(UINT8 *addr, UINT8 *dev_addr, UINT8 getflag);
INT16 MIB2_Get_IpAdEntIfIndex(UINT8 *addr, UINT32 *dev_index, UINT8 getflag);
INT16 MIB2_Get_IpAdEntNetMask(UINT8 *addr, UINT8 *net_mask, UINT8 getflag);
INT16 MIB2_Get_IpRouteDest(UINT8 *addr, UINT8 *dest, UINT8 getflag);
INT16 MIB2_Get_IpRouteIfIndex(UINT8 *addr, UINT32 *dev_index, UINT8 getflag);
INT16 MIB2_Get_IpRouteMetric1(UINT8 *addr, UINT32 *metric1, UINT8 getflag);
INT16 MIB2_Get_IpRouteNextHop(UINT8 *addr, UINT8 *nexthop, UINT8 getflag);
INT16 MIB2_Get_IpRouteType(UINT8 *addr, UINT32 *type, UINT8 getflag);
INT16 MIB2_Get_IpRouteProto(UINT8 *addr, UINT32 *proto, UINT8 getflag);
INT16 MIB2_Get_IpRouteAge(UINT8 *addr, UINT32 *age, UINT8 getflag);
INT16 MIB2_Get_IpRouteMask(UINT8 *addr, UINT8 *mask, UINT8 getflag);

INT16 MIB2_Set_IpRouteDest(const UINT8 *addr, UINT8 *addr_new);
INT16 MIB2_Set_IpRouteIfIndex(const UINT8 *addr, UINT32 dev_index);
INT16 MIB2_Set_IpRouteMetric1(const UINT8 *addr, UINT32 metric1);
INT16 MIB2_Set_IpRouteNextHop(const UINT8 *addr, UINT8 *nexthop);
INT16 MIB2_Set_IpRouteType(const UINT8 *addr, UINT32 type);
INT16 MIB2_Set_IpRouteAge(const UINT8 *addr, UINT32 age);
INT16 MIB2_Set_IpRouteMask(const UINT8 *addr, const UINT8 *mask);

INT16 MIB2_Get_IpNetToMediaIfIndex(UINT8 *addr, INT32 *index, UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaIfIndex_If(UINT32 *index, UINT8 *addr,
                                      UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaPhysAddress(UINT8 *addr, UINT8 *paddr, UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaPhysAddress_If(UINT32 *index, UINT8 *addr,
                                          UINT8 *paddr, UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaNetAddress(UINT8 *addr, UINT8 *ip_addr, UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaNetAddress_If(UINT32 *index, UINT8 *addr,
                                         UINT8 *ip_addr, UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaType(UINT8 *addr, UINT32 *type, UINT8 getflag);
INT16 MIB2_Get_IpNetToMediaType_If(UINT32 *index, UINT8 *addr,
                                   UINT32 *type, UINT8 getflag);

INT16 MIB2_Set_IpNetToMediaIfIndex(const UINT8 *addr, UINT32 index);
INT16 MIB2_Set_IpNetToMediaNetAddress(const UINT8 *addr, const UINT8 *ip_addr);
INT16 MIB2_Set_IpNetToMediaPhysAddress(const UINT8 *addr, const UINT8 *paddr);
INT16 MIB2_Set_IpNetToMediaType(const UINT8 *addr, UINT32 type);

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE)) */

#if ( ((MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE)) || \
      ((MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE)) )

STATUS MIB2_Socket_List_Information(INT protocol, UINT8 getflag,
                                    UINT8 *local_addr, UINT32 *local_port,
                                    UINT8 *remote_addr, UINT32 *remote_port,
                                    SOCKET_STRUCT *socket_info);

#endif /* ( ((MIB2_UDP_INCLUDE == NU_TRUE) && (INCLUDE_UDP == NU_TRUE)) || \
            ((MIB2_TCP_INCLUDE == NU_TRUE) && (INCLUDE_TCP == NU_TRUE)) ) */

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE))

/*-----------------------------------------------------------------------
 * TCP Group - Function Declarations
 *-----------------------------------------------------------------------*/

INT16 MIB2_Get_TcpConnState             (UINT8 *local_addr, UINT32 *local_port,
                                         UINT8 *remote_addr, UINT32 *remote_port,
                                         UINT32 *state, UINT8 getflag);

INT16 MIB2_Set_TcpConnState             (UINT8 *local_addr, UINT32 local_port,
                                         UINT8 *remote_addr, UINT32 remote_port,
                                         UINT32 state);

INT16 MIB2_Get_TcpConnLocalAddress      (UINT8 *local_addr, UINT32 *local_port,
                                         UINT8 *remote_addr, UINT32 *remote_port,
                                         UINT8 *addr, UINT8 getflag);

INT16 MIB2_Get_TcpConnLocalPort         (UINT8 *local_addr, UINT32 *local_port,
                                         UINT8 *remote_addr, UINT32 *remote_port,
                                         UINT32 *port, UINT8 getflag);

INT16 MIB2_Get_TcpConnRemAddress        (UINT8 *local_addr, UINT32 *local_port,
                                         UINT8 *remote_addr, UINT32 *remote_port,
                                         UINT8 *addr, UINT8 getflag);

INT16 MIB2_Get_TcpConnRemPort           (UINT8 *local_addr, UINT32 *local_port,
                                         UINT8 *remote_addr, UINT32 *remote_port,
                                         UINT32 *port, UINT8 getflag);

UINT32 MIB2_SizeTcpTab                  (VOID);

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE)) */

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE))
/*-----------------------------------------------------------------------
 * UDP Group - Function Declarations
 *-----------------------------------------------------------------------*/

INT16 MIB2_Get_UdpConnLocalAddress      (UINT8 *local_addr, UINT32 *local_port,
                                         UINT8 *addr, UINT8 getflag);

INT16 MIB2_Get_UdpConnLocalPort         (UINT8 *local_addr, UINT32 *local_port,
                                         UINT32 *port, UINT8 getflag);

UINT32 MIB2_SizeUdpTab                  (VOID);

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE)) */

/*-----------------------------------------------------------------------
 * RMON Group - Function Declarations
 *-----------------------------------------------------------------------*/
#if(INCLUDE_SNMP == NU_TRUE)
#if(INCLUDE_MIB_RMON1 == NU_TRUE)

UINT32 MIB2_Get_1757Octets              (UINT32 index);
UINT32 MIB2_Get_1757Pkts                (UINT32 index);
UINT32 MIB2_Get_1757BroadcastPkts       (UINT32 index);
UINT32 MIB2_Get_1757MulticastPkts       (UINT32 index);
UINT32 MIB2_Get_1757CRCAlignErrors      (UINT32 index);
UINT32 MIB2_Get_1757UndersizePkts       (UINT32 index);
UINT32 MIB2_Get_1757OversizePkts        (UINT32 index);
UINT32 MIB2_Get_1757Fragments           (UINT32 index);
UINT32 MIB2_Get_1757Jabbers             (UINT32 index);
UINT32 MIB2_Get_1757Collisions          (UINT32 index);
UINT32 MIB2_Get_1757Pkts64Octets        (UINT32 index);
UINT32 MIB2_Get_1757Pkts65to127Octets   (UINT32 index);
UINT32 MIB2_Get_1757Pkts128to255Octets  (UINT32 index);
UINT32 MIB2_Get_1757Pkts256to511Octets  (UINT32 index);
UINT32 MIB2_Get_1757Pkts512to1023Octets (UINT32 index);
UINT32 MIB2_Get_1757Pkts1024to1518Octets(UINT32 index);
UINT32 MIB2_Get_1757LostPkts            (UINT32 index);
UINT32 MIB2_Get_1757DiscardedPkts       (UINT32 index);
UINT32 MIB2_Get_1757OutPkts             (UINT32 index);
UINT32 MIB2_Get_1757OutOctets           (UINT32 index);
UINT32 MIB2_Get_1757OutMulticastPkts    (UINT32 index);
UINT32 MIB2_Get_1757OutBroadcastPkts    (UINT32 index);
UINT32 MIB2_Get_1757OutErrors           (UINT32 index);

INT16  MIB2_Add_1757Octets              (UINT32 index, UINT32 octets,    INT16 reset);
INT16  MIB2_Add_1757Pkts                (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757BroadcastPkts       (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757MulticastPkts       (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757CRCAlignErrors      (UINT32 index, UINT32 errors,    INT16 reset);
INT16  MIB2_Add_1757UndersizePkts       (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757OversizePkts        (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757Fragments           (UINT32 index, UINT32 fragments, INT16 reset);
INT16  MIB2_Add_1757Jabbers             (UINT32 index, UINT32 jabbers,   INT16 reset);
INT16  MIB2_Add_1757Collisions          (UINT32 index, UINT32 collisions,INT16 reset);
INT16  MIB2_Add_1757Pkts64Octets        (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757Pkts65to127Octets   (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757Pkts128to255Octets  (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757Pkts256to511Octets  (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757Pkts512to1023Octets (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757Pkts1024to1518Octets(UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757LostPkts            (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757DiscardedPkts       (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757OutPkts             (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757OutOctets           (UINT32 index, UINT32 octets,    INT16 reset);
INT16  MIB2_Add_1757OutMulticastPkts    (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757OutBroadcastPkts    (UINT32 index, UINT32 packets,   INT16 reset);
INT16  MIB2_Add_1757OutErrors           (UINT32 index, UINT32 errors,    INT16 reset);

#endif /* (INCLUDE_MIB_RMON1 == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */


/*--------------------------------------------------------------------------
 * MIB-II Macros
 *--------------------------------------------------------------------------*/

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) )

/*-----------------------------------------------------------------------
 * ICMP Group - Macros
 *-----------------------------------------------------------------------*/

#define MIB2_icmpInMsgs_Get \
    Mib2_Icmp_Data.icmpInMsgs

#define MIB2_icmpInMsgs_Inc \
    Mib2_Icmp_Data.icmpInMsgs++


#define MIB2_icmpInErrors_Get \
    Mib2_Icmp_Data.icmpInErrors

#define MIB2_icmpInErrors_Inc \
    Mib2_Icmp_Data.icmpInErrors++

#define MIB2_icmpInDestUnreachs_Get \
        Mib2_Icmp_Data.icmpInDestUnreachs

#define MIB2_icmpInDestUnreachs_Inc \
        Mib2_Icmp_Data.icmpInDestUnreachs++

#define MIB2_icmpInTimeExcds_Get \
        Mib2_Icmp_Data.icmpInTimeExcds

#define MIB2_icmpInTimeExcds_Inc \
        Mib2_Icmp_Data.icmpInTimeExcds++

#define MIB2_icmpInParmProbs_Get \
        Mib2_Icmp_Data.icmpInParmProbs

#define MIB2_icmpInParmProbs_Inc \
        Mib2_Icmp_Data.icmpInParmProbs++

#define MIB2_icmpInSrcQuenchs_Get \
        Mib2_Icmp_Data.icmpInSrcQuenchs

#define MIB2_icmpInSrcQuenchs_Inc \
        Mib2_Icmp_Data.icmpInSrcQuenchs++

#define MIB2_icmpInRedirects_Get \
        Mib2_Icmp_Data.icmpInRedirects

#define MIB2_icmpInRedirects_Inc \
        Mib2_Icmp_Data.icmpInRedirects++

#define MIB2_icmpInEchos_Get \
        Mib2_Icmp_Data.icmpInEchos

#define MIB2_icmpInEchos_Inc \
        Mib2_Icmp_Data.icmpInEchos++

#define MIB2_icmpInEchoReps_Get \
        Mib2_Icmp_Data.icmpInEchoReps

#define MIB2_icmpInEchoReps_Inc \
        Mib2_Icmp_Data.icmpInEchoReps++

#define MIB2_icmpInTimeStamps_Get \
        Mib2_Icmp_Data.icmpInTimestamps

#define MIB2_icmpInTimeStamps_Inc \
        Mib2_Icmp_Data.icmpInTimestamps++

#define MIB2_icmpInTimeStampReps_Get \
        Mib2_Icmp_Data.icmpInTimestampReps

#define MIB2_icmpInTimeStampReps_Inc \
        Mib2_Icmp_Data.icmpInTimestampReps++

#define MIB2_icmpInAddrMasks_Get \
        Mib2_Icmp_Data.icmpInAddrMasks

#define MIB2_icmpInAddrMasks_Inc \
        Mib2_Icmp_Data.icmpInAddrMasks++

#define MIB2_icmpInAddrMaskReps_Get \
        Mib2_Icmp_Data.icmpInAddrMaskReps

#define MIB2_icmpInAddrMaskReps_Inc \
        Mib2_Icmp_Data.icmpInAddrMaskReps++

#define MIB2_icmpOutMsgs_Get \
        Mib2_Icmp_Data.icmpOutMsgs

#define MIB2_icmpOutMsgs_Inc \
        Mib2_Icmp_Data.icmpOutMsgs++

#define MIB2_icmpOutErrors_Get \
        Mib2_Icmp_Data.icmpOutErrors

#define MIB2_icmpOutErrors_Inc \
        Mib2_Icmp_Data.icmpOutErrors++

#define MIB2_icmpOutDestUnreachs_Get \
        Mib2_Icmp_Data.icmpOutDestUnreachs

#define MIB2_icmpOutDestUnreachs_Inc \
        Mib2_Icmp_Data.icmpOutDestUnreachs++

#define MIB2_icmpOutTimeExcds_Get \
        Mib2_Icmp_Data.icmpOutTimeExcds

#define MIB2_icmpOutTimeExcds_Inc \
        Mib2_Icmp_Data.icmpOutTimeExcds++

#define MIB2_icmpOutParmProbs_Get \
        Mib2_Icmp_Data.icmpOutParmProbs

#define MIB2_icmpOutParmProbs_Inc \
        Mib2_Icmp_Data.icmpOutParmProbs++

#define MIB2_icmpOutSrcQuenchs_Get \
        Mib2_Icmp_Data.icmpOutSrcQuenchs

#define MIB2_icmpOutSrcQuenchs_Inc \
        Mib2_Icmp_Data.icmpOutSrcQuenchs++

#define MIB2_icmpOutRedirects_Get \
        Mib2_Icmp_Data.icmpOutRedirects

#define MIB2_icmpOutRedirects_Inc \
        Mib2_Icmp_Data.icmpOutRedirects++

#define MIB2_icmpOutEchos_Get \
        Mib2_Icmp_Data.icmpOutEchos

#define MIB2_icmpOutEchos_Inc \
        Mib2_Icmp_Data.icmpOutEchos++

#define MIB2_icmpOutEchoReps_Get \
        Mib2_Icmp_Data.icmpOutEchoReps

#define MIB2_icmpOutEchoReps_Inc \
        Mib2_Icmp_Data.icmpOutEchoReps++

#define MIB2_icmpOutTimestamps_Get \
        Mib2_Icmp_Data.icmpOutTimestamps

#define MIB2_icmpOutTimestamps_Inc \
        Mib2_Icmp_Data.icmpOutTimestamps++

#define MIB2_icmpOutTimestampReps_Get \
        Mib2_Icmp_Data.icmpOutTimestampReps

#define MIB2_icmpOutTimestampReps_Inc \
        Mib2_Icmp_Data.icmpOutTimestampReps++

#define MIB2_icmpOutAddrMasks_Get \
        Mib2_Icmp_Data.icmpOutAddrMasks

#define MIB2_icmpOutAddrMasks_Inc \
        Mib2_Icmp_Data.icmpOutAddrMasks++

#define MIB2_icmpOutAddrMaskReps_Get \
        Mib2_Icmp_Data.icmpOutAddrMaskReps

#define MIB2_icmpOutAddrMaskReps_Inc \
        Mib2_Icmp_Data.icmpOutAddrMaskReps++

#else /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) ) */

#define MIB2_icmpInMsgs_Get
#define MIB2_icmpInMsgs_Inc
#define MIB2_icmpInErrors_Get
#define MIB2_icmpInErrors_Inc
#define MIB2_icmpInDestUnreachs_Get
#define MIB2_icmpInDestUnreachs_Inc
#define MIB2_icmpInTimeExcds_Get
#define MIB2_icmpInTimeExcds_Inc
#define MIB2_icmpInParmProbs_Get
#define MIB2_icmpInParmProbs_Inc
#define MIB2_icmpInSrcQuenchs_Get
#define MIB2_icmpInSrcQuenchs_Inc
#define MIB2_icmpInRedirects_Get
#define MIB2_icmpInRedirects_Inc
#define MIB2_icmpInEchos_Get
#define MIB2_icmpInEchos_Inc
#define MIB2_icmpInEchoReps_Get
#define MIB2_icmpInEchoReps_Inc
#define MIB2_icmpInTimeStamps_Get
#define MIB2_icmpInTimeStamps_Inc
#define MIB2_icmpInTimeStampReps_Get
#define MIB2_icmpInTimeStampReps_Inc
#define MIB2_icmpInAddrMasks_Get
#define MIB2_icmpInAddrMasks_Inc
#define MIB2_icmpInAddrMaskReps_Get
#define MIB2_icmpInAddrMaskReps_Inc
#define MIB2_icmpOutMsgs_Get
#define MIB2_icmpOutMsgs_Inc
#define MIB2_icmpOutErrors_Get
#define MIB2_icmpOutErrors_Inc
#define MIB2_icmpOutDestUnreachs_Get
#define MIB2_icmpOutDestUnreachs_Inc
#define MIB2_icmpOutTimeExcds_Get
#define MIB2_icmpOutTimeExcds_Inc
#define MIB2_icmpOutParmProbs_Get
#define MIB2_icmpOutParmProbs_Inc
#define MIB2_icmpOutSrcQuenchs_Get
#define MIB2_icmpOutSrcQuenchs_Inc
#define MIB2_icmpOutRedirects_Get
#define MIB2_icmpOutRedirects_Inc
#define MIB2_icmpOutEchos_Get
#define MIB2_icmpOutEchos_Inc
#define MIB2_icmpOutEchoReps_Get
#define MIB2_icmpOutEchoReps_Inc
#define MIB2_icmpOutTimestamps_Get
#define MIB2_icmpOutTimestamps_Inc
#define MIB2_icmpOutTimestampReps_Get
#define MIB2_icmpOutTimestampReps_Inc
#define MIB2_icmpOutAddrMasks_Get
#define MIB2_icmpOutAddrMasks_Inc
#define MIB2_icmpOutAddrMaskReps_Get
#define MIB2_icmpOutAddrMaskReps_Inc

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE) ) */


/*-----------------------------------------------------------------------
 * Interfaces Group - Macros
 *-----------------------------------------------------------------------*/

/*
 * Support for RFC1213, Interfaces Group
 */

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )

#define MIB2_totalInterface_Get \
    MIB2_Interface_Count()

#define MIB2_ifOutQLen_Get(index, length) \
    MIB2_Get_IfOutQLen (index, (UINT32 *)&length)

#define MIB2_ifOutQLen_Inc(index) \
    MIB2_Add_IfOutQLen (index, 1, NU_FALSE)

#define MIB2_ifOutQLen_Dec(index) \
    MIB2_Add_IfOutQLen (index, -1, NU_FALSE)

#define MIB2_ifOutErrors_Get(index, errors) \
    MIB2_Get_IfOutErrors (index, (UINT32 *)&errors)

#define MIB2_ifOutErrors_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->outErrors++

#define MIB2_ifOutErrors_Inci(index) \
    MIB2_Add_IfOutErrors (index, 1, NU_FALSE)

#define MIB2_ifOutDiscards_Get(index, discards) \
    MIB2_Get_IfOutDiscards (index, (UINT32 *)&discards)

#define MIB2_ifOutDiscards_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->outDiscards++

#define MIB2_ifOutDiscards_Inci(index) \
    MIB2_Add_IfOutDiscards(index, 1, NU_FALSE)

#define MIB2_ifOutNUcastPkts_Get(index, packets) \
    MIB2_Get_IfOutNUcastPkts(index, (UINT32 *)&packets)

#define MIB2_ifOutNUcastPkts_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->outNUcastPkts++

#define MIB2_ifOutNUcastPkts_Inci(index) \
    MIB2_Add_IfOutNUcastPkts(index, 1, NU_FALSE)

#define MIB2_ifOutUcastPkts_Get(index, packets) \
    MIB2_Get_IfOutUcastPkts (index, (UINT32 *)&packets, 0)

#define MIB2_ifOutUcastPkts_Get64(index, packets) \
    MIB2_Get_IfOutUcastPkts (index, (UINT32 *)&packets, MIB2_COUNTER_64)

#define MIB2_ifOutUcastPkts_Inc(dev_ptr) \
    MIB2_Add_IfOutUcastPkts(dev_ptr->dev_index, 1, NU_FALSE)

#define MIB2_ifOutUcastPkts_Inci(index) \
    MIB2_Add_IfOutUcastPkts(index, 1, NU_FALSE)

#define MIB2_ifOutOctets_Get(index, octets) \
    MIB2_Get_IfOutOctets (index, (UINT32 *)&octets, 0)

#define MIB2_ifOutOctets_Get64(index, octets) \
    MIB2_Get_IfOutOctets (index, (UINT32 *)&octets, MIB2_COUNTER_64)

#define MIB2_ifOutOctets_Add(dev_ptr, value) \
    MIB2_Add_IfOutOctets (dev_ptr->dev_index, value, NU_FALSE)

#define MIB2_ifOutOctets_Addi(index, value) \
    MIB2_Add_IfOutOctets (index, value, NU_FALSE)

#define MIB2_ifInUcastPkts_Get(index, packets) \
    MIB2_Get_IfInUcastPkts (index, (UINT32 *)&packets, 0)

#define MIB2_ifInUcastPkts_Get64(index, packets) \
    MIB2_Get_IfInUcastPkts (index, (UINT32 *)&packets, MIB2_COUNTER_64)

#define MIB2_ifInUcastPkts_Inc(dev_ptr) \
    MIB2_Add_IfInUcastPkts (dev_ptr->dev_index, 1, NU_FALSE)

#define MIB2_ifInUcastPkts_Inci(index) \
    MIB2_Add_IfInUcastPkts (index, 1, NU_FALSE)

#define MIB2_ifInNUcastPkts_Get(index, packets) \
    MIB2_Get_IfInNUcastPkts(index, (UINT32 *)&packets)

#define MIB2_ifInNUcastPkts_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->inNUcastPkts++;

#define MIB2_ifInNUcastPkts_Inci(index) \
    MIB2_Add_IfInNUcastPkts(index, 1, NU_FALSE)

#define MIB2_ifInUnknownProtos_Get(index, packets) \
    MIB2_Get_IfInUnknownProtos (index, (UINT32 *)&packets)

#define MIB2_ifInUnknownProtos_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->inUnknownProtos++

#define MIB2_ifInUnknownProtos_Inci(index) \
    MIB2_Add_IfInUnknownProtos (index, 1, NU_FALSE)

#define MIB2_ifInErrors_Get(index, errors) \
    MIB2_Get_IfInErrors (index, (UINT32 *)&errors)

#define MIB2_ifInErrors_Inci(index) \
    MIB2_Add_IfInErrors (index, 1, NU_FALSE)

#define MIB2_ifInErrors_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->inErrors++

#define MIB2_ifInDiscards_Get(index, discards) \
    MIB2_Get_IfInDiscards (index, (UINT32 *)&discards)

#define MIB2_ifInDiscards_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->inDiscards++

#define MIB2_ifInDiscards_Inci(index) \
    MIB2_Add_IfInDiscards (index, 1, NU_FALSE)

#define MIB2_ifInOctets_Get(index, octets) \
    MIB2_Get_IfInOctets (index, (UINT32 *)&octets, 0)

#define MIB2_ifInOctets_Get64(index, octets) \
    MIB2_Get_IfInOctets (index, (UINT32 *)&octets, MIB2_COUNTER_64)

#define MIB2_ifInOctets_Add(dev_ptr, octets) \
    MIB2_Add_IfInOctets (dev_ptr->dev_index, octets, NU_FALSE)

#define MIB2_ifInOctets_Addi(index, octets) \
    MIB2_Add_IfInOctets (index, octets, NU_FALSE)

#define MIB2_ifDescr_Get(index, string, length) \
    MIB2_Get_IfDescr (index, string, (UINT32 *)&length)

#define MIB2_ifDescr_Seti(index, string) \
    MIB2_Set_IfDescr (index, (UINT8 *)string)

#define MIB2_ifDescr_Set(dev_ptr, if_decr)                                                  \
    if ( (strlen(if_decr)) > (MIB2_INT_DESC_LENGTH - 1) )                                   \
    {                                                                                       \
        NU_BLOCK_COPY(dev_ptr->dev_mibInterface.descr, if_decr, (MIB2_INT_DESC_LENGTH - 1));\
        dev_ptr->dev_mibInterface.descr[MIB2_INT_DESC_LENGTH - 1] = NU_NULL;                \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        strcpy((CHAR *)dev_ptr->dev_mibInterface.descr, if_decr);                           \
    }

#define MIB2_ifSpecific_Get(index, string, length) \
    MIB2_Get_ifSpecific()

#define MIB2_ifSpecific_Set(index, string)              /* Depreciated */

#define MIB2_ifType_Get(index, type) \
    MIB2_Get_IfType (index, (INT32 *)&type)

#define MIB2_ifType_Set(dev_ptr, value) \
    dev_ptr->dev_mibInterface.type = value

#define MIB2_ifType_Seti(index, value) \
    MIB2_Set_IfType (index, value)

#define MIB2_ifIndex_Get(index) \
    MIB2_Get_IfIndex (index)

#define MIB2_ifMtu_Get(index, octets) \
    MIB2_Get_IfMtu (index, (UINT32 *)(&octets))

#define MIB2_ifMtu_Set(index, octets) \
    MIB2_Set_IfMtu (index, octets)

#define MIB2_ifAddr_Get(index, address, length) \
    MIB2_Get_IfAddr (index, address, (UINT32 *)&length)

#define MIB2_ifAddr_Set(index, address) \
    MIB2_Set_IfAddr (index, address)

#define MIB2_ifSpeed_Get(index, speed) \
    MIB2_Get_IfSpeed (index, (UINT32 *)&speed)

#define MIB2_ifSpeed_Set(dev_ptr, value) \
    dev_ptr->dev_mibInterface.speed = value

#define MIB2_ifSpeed_Seti(index, value) \
    MIB2_Set_IfSpeed (index, value)

#define MIB2_ifLastChange_Get(index, time) \
    MIB2_Get_IfLastChange (index, (UINT32 *)&time)

#define MIB2_IfLastChange_Set(index, value) \
    MIB2_Set_IfLastChange (index, value)

#define MIB2_ifStatusAdmin_Get(index, state) \
    MIB2_Get_IfStatusAdmin (index, (INT32 *)&state)

#define MIB2_ifStatusAdmin_Set(index, value) \
    MIB2_Set_IfStatusAdmin (index, value)

#define MIB2_ifStatusOper_Get(index, state) \
    MIB2_Get_IfStatusOper (index, (INT32 *)&state)

#define MIB2_ifStatusOper_Set(dev_ptr, value) \
    MIB2_Set_IfStatusOper(dev_ptr, value)


#if (INCLUDE_IF_EXT == NU_TRUE)
/*
 * MIB-II Interface extension.
 */


#define MIB2_IfName_Get(index, if_name) \
    MIB2_Get_IfName(index, (CHAR *)if_name)

#define MIB2_IfLinkTrapEnable_Get(index, if_link_trap_enable) \
    MIB2_Get_IfLinkTrapEnable(index, ((UINT32 *)(&if_link_trap_enable)))

#define MIB2_IfLinkTrapEnable_Set(index, if_link_trap_enable) \
    MIB2_Set_IfLinkTrapEnable(index, if_link_trap_enable)

#define MIB2_IfHighSpeed_Get(index, if_high_speed) \
    MIB2_Get_IfHighSpeed(index, ((UINT32 *)(&if_high_speed)))

#define MIB2_IfHighSpeed_Set(dev_ptr, if_high_speed) \
    if (dev_ptr->dev_mibInterface.mib2_ext_tbl) \
        dev_ptr->dev_mibInterface.mib2_ext_tbl->mib2_high_speed = if_high_speed

#define MIB2_IfConnectorPresent_Get(index, if_connector_present) \
    MIB2_Get_IfConnectorPresent(index, ((UINT32 *)(&if_connector_present)))

#define MIB2_IfConnectorPresent_Set(dev_ptr, if_connector_present) \
    if (dev_ptr->dev_mibInterface.mib2_ext_tbl) \
        dev_ptr->dev_mibInterface.mib2_ext_tbl->mib2_connector_present = if_connector_present

#define MIB2_IfAlias_Get(index, if_alias) \
    MIB2_Get_IfAlias(index, (CHAR *)if_alias)

#define MIB2_IfAlias_Set(index, if_alias) \
    MIB2_Set_IfAlias(index, (CHAR *)if_alias)

#define MIB2_IfCounterDiscontinTime_Get(index, if_discontinuity_time) \
    MIB2_Get_IfCounterDiscontinTime(index, ((UINT32 *)(&if_discontinuity_time)))

#define MIB2_InBroadcast_Pkts_Get(index, packets) \
    MIB2_Get_IfInBrdcastPkts(index, (UINT32*)&packets, 0)

#define MIB2_InBroadcast_Pkts_Get64(index, packets) \
    MIB2_Get_IfInBrdcastPkts(index, (UINT32*)&packets, MIB2_COUNTER_64)

#define MIB2_InBroadcast_Pkts_Inc(device) \
    MIB2_Add_IfInBrdcastPkts(device, 1, NU_FALSE)

#define MIB2_OutBroadcast_Pkts_Get(index,packets) \
    MIB2_Get_IfOutBrdcastPkts(index, (UINT32*)&packets, 0)

#define MIB2_OutBroadcast_Pkts_Get64(index,packets) \
    MIB2_Get_IfOutBrdcastPkts(index, (UINT32*)&packets, MIB2_COUNTER_64)

#define MIB2_OutBroadcast_Pkts_Inc(device) \
    MIB2_Add_IfOutBrdcastPkts(device, 1, NU_FALSE)

#define MIB2_InMulticast_Pkts_Get(index, packets) \
    MIB2_Get_IfInMulticastPkts(index, (UINT32*)&packets, 0)

#define MIB2_InMulticast_Pkts_Get64(index, packets) \
    MIB2_Get_IfInMulticastPkts(index, (UINT32*)&packets, MIB2_COUNTER_64)

#define MIB2_InMulticast_Pkts_Inc(device) \
    MIB2_Add_IfInMulticastPkts(device, 1, NU_FALSE)

#define MIB2_OutMulticast_Pkts_Get(index,packets) \
    MIB2_Get_IfOutMulticastPkts(index, (UINT32*)&packets, 0)

#define MIB2_OutMulticast_Pkts_Get64(index,packets) \
    MIB2_Get_IfOutMulticastPkts(index, (UINT32*)&packets, MIB2_COUNTER_64)

#define MIB2_OutMulticast_Pkts_Inc(device) \
    MIB2_Add_IfOutMulticastPkts(device, 1, NU_FALSE)

#define MIB2_Promiscuous_Mode_Get(index, mode) \
    MIB2_Get_IfPromiscMode(index, (UINT32 *)&mode)

#else /* (INCLUDE_IF_EXT == NU_TRUE) */

#define MIB2_IfName_Get(index, if_name)
#define MIB2_IfLinkTrapEnable_Get(index, if_link_trap_enable)
#define MIB2_IfLinkTrapEnable_Set(index, if_link_trap_enable)
#define MIB2_ifHighSpeed_Get(index, if_high_speed)
#define MIB2_IfHighSpeed_Set(dev_ptr, if_high_speed)
#define MIB2_IfConnectorPresent_Get(index, if_connector_present)
#define MIB2_IfConnectorPresent_Set(dev_ptr, if_connector_present)
#define MIB2_IfAlias_Get(index, if_alias)
#define MIB2_IfAlias_Set(index, if_alias)
#define MIB2_IfCounterDiscontinTime_Get(index, if_discontinuity_time)
#define MIB2_InBroadcast_Pkts_Get(index, packets)
#define MIB2_InBroadcast_Pkts_Get64(index, packets)

#define MIB2_InBroadcast_Pkts_Inc(device)   \
    device->dev_mibInterface.eth->inNUcastPkts++

#define MIB2_OutBroadcast_Pkts_Get(index,packets)
#define MIB2_OutBroadcast_Pkts_Get64(index,packets)

#define MIB2_OutBroadcast_Pkts_Inc(device)  \
    device->dev_mibInterface.eth->outNUcastPkts++

#define MIB2_InMulticast_Pkts_Get(index, packets)
#define MIB2_InMulticast_Pkts_Get64(index, packets)

#define MIB2_InMulticast_Pkts_Inc(device)   \
    device->dev_mibInterface.eth->inNUcastPkts++

#define MIB2_OutMulticast_Pkts_Get(index,packets)
#define MIB2_OutMulticast_Pkts_Get64(index,packets)

#define MIB2_OutMulticast_Pkts_Inc(device)  \
    device->dev_mibInterface.eth->outNUcastPkts++

#define MIB2_Promiscuous_Mode_Get(index, mode)

#endif /* (INCLUDE_IF_EXT == NU_TRUE) */

#define MIB2_RCV_ADDR_GET_NEXT(if_index, phy_addr) \
    MIB2_Get_Next_If_Address((&if_index), phy_addr)

#define MIB2_RCV_ADDR_GET(if_index, phy_addr)  \
    MIB2_Get_If_Address(if_index, phy_addr)

#else /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) ) */

#define MIB2_Interface_Init(dev_ptr)    (NU_SUCCESS)

#define MIB2_Set_IfDescr(if_index, desc)
#define MIB2_Set_IfType(if_index, type)
#define MIB2_Set_IfMtu(if_index, mtu_value)
#define MIB2_Set_IfSpeed(if_index, speed)
#define MIB2_Set_IfAddr(if_index, addr)
#define MIB2_Set_IfStatusAdmin(if_index, value)
#define MIB2_Set_IfLastChange(if_index, value)
#define MIB2_Add_IfOutErrors(if_index, value, reset)
#define MIB2_Add_IfOutDiscards(if_index, value, reset)
#define MIB2_Add_IfOutUcastPkts(if_index, value, reset)
#define MIB2_Add_IfOutOctets(if_index, value, reset)
#define MIB2_Add_IfInUcastPkts(if_index, value, reset)
#define MIB2_Add_IfInNUcastPkts(if_index, value, reset)
#define MIB2_Add_IfOutNUcastPkts(if_index, value, reset)
#define MIB2_Add_IfInUnknownProtos(if_index, value, reset)
#define MIB2_Add_IfInErrors(if_index, value, reset)
#define MIB2_Add_IfInDiscards(if_index, value, reset)
#define MIB2_Add_IfInOctets(if_index, value, reset)
#define MIB2_totalInterface_Get
#define MIB2_ifOutQLen_Get(index, length)
#define MIB2_ifOutQLen_Inc(index)
#define MIB2_ifOutQLen_Dec(index)
#define MIB2_ifOutErrors_Get(index, errors)
#define MIB2_ifOutErrors_Inc(dev_ptr)
#define MIB2_ifOutErrors_Inci(index)
#define MIB2_ifOutDiscards_Get(index, discards)
#define MIB2_ifOutDiscards_Inc(dev_ptr)
#define MIB2_ifOutDiscards_Inci(index)
#define MIB2_ifOutNUcastPkts_Get(index, packets)
#define MIB2_ifOutNUcastPkts_Inc(dev_ptr)
#define MIB2_ifOutNUcastPkts_Inci(index)
#define MIB2_ifOutUcastPkts_Get(index, packets)
#define MIB2_ifOutUcastPkts_Get64(index, packets)
#define MIB2_ifOutUcastPkts_Inc(dev_ptr)
#define MIB2_ifOutUcastPkts_Inci(index)
#define MIB2_ifOutOctets_Get(index, octets)
#define MIB2_ifOutOctets_Get64(index, octets)
#define MIB2_ifOutOctets_Add(dev_ptr, value)
#define MIB2_ifOutOctets_Addi(index, value)
#define MIB2_ifInUcastPkts_Get(index, packets)
#define MIB2_ifInUcastPkts_Get64(index, packets)
#define MIB2_ifInUcastPkts_Inc(dev_ptr)
#define MIB2_ifInUcastPkts_Inci(index)
#define MIB2_ifInNUcastPkts_Get(index, packets)
#define MIB2_ifInNUcastPkts_Inc(dev_ptr)
#define MIB2_ifInNUcastPkts_Inci(index)
#define MIB2_ifInUnknownProtos_Get(index, packets)
#define MIB2_ifInUnknownProtos_Inc(dev_ptr)
#define MIB2_ifInUnknownProtos_Inci(index)
#define MIB2_ifInErrors_Get(index, errors)
#define MIB2_ifInErrors_Inci(index)
#define MIB2_ifInErrors_Inc(dev_ptr)
#define MIB2_ifInDiscards_Get(index, discards)
#define MIB2_ifInDiscards_Inc(dev_ptr)
#define MIB2_ifInDiscards_Inci(index)
#define MIB2_ifInOctets_Get(index, octets)
#define MIB2_ifInOctets_Get64(index, octets)
#define MIB2_ifInOctets_Add(dev_ptr, octets)
#define MIB2_ifInOctets_Addi(index, octets)
#define MIB2_ifDescr_Get(index, string, length)
#define MIB2_ifDescr_Seti(index, string)
#define MIB2_ifDescr_Set(dev_ptr, if_decr)
#define MIB2_ifSpecific_Get(index, string, length)
#define MIB2_ifSpecific_Set(index, string)
#define MIB2_ifType_Get(index, type)
#define MIB2_ifType_Set(dev_ptr, value)
#define MIB2_ifType_Seti(index, value)
#define MIB2_ifIndex_Get(index)
#define MIB2_ifMtu_Get(index, octets)
#define MIB2_ifMtu_Set(index, octets)
#define MIB2_ifAddr_Get(index, address, length)
#define MIB2_ifAddr_Set(index, address)
#define MIB2_ifSpeed_Get(index, speed)
#define MIB2_ifSpeed_Set(dev_ptr, value)
#define MIB2_ifSpeed_Seti(index, value)
#define MIB2_ifLastChange_Get(index, time)
#define MIB2_IfLastChange_Set(index, value)
#define MIB2_ifStatusAdmin_Get(index, state)
#define MIB2_ifStatusAdmin_Set(index, value)
#define MIB2_ifStatusOper_Get(index, state)
#define MIB2_ifStatusOper_Set(dev_ptr, value)

#define MIB2_IfName_Get(index, if_name)
#define MIB2_IfLinkTrapEnable_Get(index, if_link_trap_enable)
#define MIB2_IfLinkTrapEnable_Set(index, if_link_trap_enable)
#define MIB2_ifHighSpeed_Get(index, if_high_speed)
#define MIB2_IfHighSpeed_Set(dev_ptr, if_high_speed)
#define MIB2_IfConnectorPresent_Get(index, if_connector_present)
#define MIB2_IfConnectorPresent_Set(dev_ptr, if_connector_present)
#define MIB2_IfAlias_Get(index, if_alias)
#define MIB2_IfAlias_Set(index, if_alias)
#define MIB2_IfCounterDiscontinTime_Get(index, if_discontinuity_time)
#define MIB2_InBroadcast_Pkts_Get(index, packets)
#define MIB2_InBroadcast_Pkts_Get64(index, packets)
#define MIB2_InBroadcast_Pkts_Inc(index)
#define MIB2_OutBroadcast_Pkts_Get(index,packets)
#define MIB2_OutBroadcast_Pkts_Get64(index,packets)
#define MIB2_OutBroadcast_Pkts_Inc(device)
#define MIB2_InMulticast_Pkts_Get(index, packets)
#define MIB2_InMulticast_Pkts_Get64(index, packets)
#define MIB2_InMulticast_Pkts_Inc(device)
#define MIB2_OutMulticast_Pkts_Get(index,packets)
#define MIB2_OutMulticast_Pkts_Get64(index,packets)
#define MIB2_OutMulticast_Pkts_Inc(device)
#define MIB2_Promiscuous_Mode_Get(index, mode)
#define MIB2_Promiscuous_Mode_Set(index, mode)

#define MIB2_RCV_ADDR_GET_NEXT(if_index, phy_addr)
#define MIB2_RCV_ADDR_GET(if_index, phy_addr)

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) ) */


/*-----------------------------------------------------------------------
 * Interfaces Stack Group - Macros
 *-----------------------------------------------------------------------*/
#if( (INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) && \
     (MIB2_IF_INCLUDE == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

#define MIB2_IF_STACK_GET(higher_if_index, lower_if_index) \
    MIB2_Get_IfStack(higher_if_index, lower_if_index)

#define MIB2_IF_STACK_GET_NEXT(higher_if_index, lower_if_index) \
    MIB2_Get_Next_IfStack(&(higher_if_index), &(lower_if_index))

#define MIB2_IF_STACK_ROW_STATUS_GET(higher_if_index, lower_if_index, row_status) \
    MIB2_Get_IfStack_Row_Status(higher_if_index, lower_if_index, &(row_status))

#define MIB2_IF_STACK_LAST_CHANGE_GET \
        MIB2_Interface_Stack_Root.mib2_stack_last_change;

#else /* ( (INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) && \
           (MIB2_IF_INCLUDE == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) ) */

#define MIB2_IF_STACK_GET(higher_if_index, lower_if_index)
#define MIB2_IF_STACK_GET_NEXT(higher_if_index, lower_if_index)
#define MIB2_IF_STACK_ROW_STATUS_GET(higher_if_index, lower_if_index, row_status)
#define MIB2_IF_STACK_LAST_CHANGE_GET

#endif /* ( (INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_MIB2_RFC1213 == NU_TRUE) &&   \
            (MIB2_IF_INCLUDE == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) ) */




/*-----------------------------------------------------------------------
 * The IP Group - Macros
 *-----------------------------------------------------------------------*/

#if ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE))
/*
 * IP Address Table
 */

#define MIB2_ipAdEntAddr_Get(addr, dev_addr, getflag) \
    MIB2_Get_IpAdEntAddr (addr, dev_addr, getflag)

#define MIB2_ipAdEntNetMask_Get(addr, net_mask, getflag) \
    MIB2_Get_IpAdEntNetMask (addr, net_mask, getflag)

#define MIB2_ipAdEntIfIndex_Get(addr, dev_index, getflag) \
    MIB2_Get_IpAdEntIfIndex (addr, (UINT32 *)(&dev_index), getflag)

#define MIB2_ipAdEntBcastAddr_Get \
    MIB2_LEAST_BCAST_BIT

#define MIB2_ipAdEntReasmMaxSize_Get \
    MIB2_REASM_MAX_SIZE

/*
 * IP Statistics Table
 */

#if INCLUDE_IP_FORWARDING

#define MIB2_ipForwarding_Get \
    ((IP_Forwarding - 1) * -1) + 1          /* IP_Forwarding = 0 implies ipForwarding = 2
                                               IP_Forwarding = 1 implies ipForwarding = 1 */
#define MIB2_ipForwarding_Set(value) \
    NU_Set_IP_Forwarding(((value - 1) * -1) + 1)

#else

#define MIB2_ipForwarding_Get
#define MIB2_ipForwarding_Set(value)

#endif

#define MIB2_ipDefaultTTL_Get \
    IP_Time_To_Live

#define MIB2_ipDefaultTTL_Set(value) \
    NU_Set_Default_TTL(value)

#define MIB2_ipInHdrErrors_Get \
    Mib2_Ip_Data.ipInHdrErrors

#define MIB2_ipInHdrErrors_Inc \
    Mib2_Ip_Data.ipInHdrErrors++

#define MIB2_ipInReceives_Get \
    Mib2_Ip_Data.ipInReceives

#define MIB2_ipInReceives_Inc \
    Mib2_Ip_Data.ipInReceives++

#define MIB2_ipInAddrErrors_Get \
    Mib2_Ip_Data.ipInAddrErrors

#define MIB2_ipInAddrErrors_Inc \
    Mib2_Ip_Data.ipInAddrErrors++

#define MIB2_ipForwDatagrams_Get \
    Mib2_Ip_Data.ipForwDatagrams

#define MIB2_ipForwDatagrams_Inc \
    Mib2_Ip_Data.ipForwDatagrams++

#define MIB2_ipInUnknownProtos_Get \
    Mib2_Ip_Data.ipInUnknownProtos

#define MIB2_ipInUnknownProtos_Inc \
    Mib2_Ip_Data.ipInUnknownProtos++

#define MIB2_ipInDiscards_Get \
    Mib2_Ip_Data.ipInDiscards

#define MIB2_ipInDiscards_Inc \
    Mib2_Ip_Data.ipInDiscards++

#define MIB2_ipInDelivers_Get \
    Mib2_Ip_Data.ipInDelivers

#define MIB2_ipInDelivers_Inc \
    Mib2_Ip_Data.ipInDelivers++

#define MIB2_ipOutRequests_Get \
    Mib2_Ip_Data.ipOutRequests

#define MIB2_ipOutRequests_Inc \
    Mib2_Ip_Data.ipOutRequests++

#define MIB2_ipOutDiscards_Get \
    Mib2_Ip_Data.ipOutDiscards

#define MIB2_ipOutDiscards_Inc \
    Mib2_Ip_Data.ipOutDiscards++

#define MIB2_ipOutNoRoutes_Get \
    Mib2_Ip_Data.ipOutNoRoutes

#define MIB2_ipOutNoRoutes_Inc \
    Mib2_Ip_Data.ipOutNoRoutes++

#define MIB2_ipReasmTimeout_Get \
    Mib2_Ip_Data.ipReasmTimeout

#define MIB2_ipReasmTimeout_Set(value) \
    Mib2_Ip_Data.ipReasmTimeout = value

#define MIB2_ipReasmReqds_Get \
    Mib2_Ip_Data.ipReasmReqds

#define MIB2_ipReasmReqds_Inc \
    Mib2_Ip_Data.ipReasmReqds++

#define MIB2_ipReasmOKs_Get \
    Mib2_Ip_Data.ipReasmOKs

#define MIB2_ipReasmOKs_Inc \
    Mib2_Ip_Data.ipReasmOKs++

#define MIB2_ipReasmFails_Get \
    Mib2_Ip_Data.ipReasmFails

#define MIB2_ipReasmFails_Inc \
    Mib2_Ip_Data.ipReasmFails++

#define MIB2_ipFragOKs_Get \
    Mib2_Ip_Data.ipFragOKs

#define MIB2_ipFragOKs_Inc \
    Mib2_Ip_Data.ipFragOKs++

#define MIB2_ipFragFails_Get \
    Mib2_Ip_Data.ipFragFails

#define MIB2_ipFragFails_Inc \
    Mib2_Ip_Data.ipFragFails++

#define MIB2_ipFragCreates_Get \
    Mib2_Ip_Data.ipFragCreates

#define MIB2_ipFragCreates_Inc \
    Mib2_Ip_Data.ipFragCreates++

/*
 * IP Route Table
 */

#define MIB2_DEFAULT_ROUTE_EXIST \
    RTAB4_Get_Default_Route()

#define MIB2_ipRoutingDiscards_Get \
    Mib2_Ip_Data.ipRoutingDiscards

#define MIB2_ipRoutingDiscards_Inc \
    Mib2_Ip_Data.ipRoutingDiscards++

#define MIB2_ipRouteAge_Get(addr, age, getflag) \
    MIB2_Get_IpRouteAge (addr, (UINT32 *)(&age), getflag)

#define MIB2_ipRouteAge_Set(addr, age) \
    MIB2_Set_IpRouteAge (addr, age)

#define MIB2_ipRouteDest_Get(addr, dest, getflag) \
    MIB2_Get_IpRouteDest (addr, dest, getflag)

#define MIB2_ipRouteDest_Set(addr, addr_new) \
    MIB2_Set_IpRouteDest (addr, addr_new)

#define MIB2_ipRouteIfIndex_Get(addr, dev_index, getflag) \
    MIB2_Get_IpRouteIfIndex (addr, (UINT32 *)(&dev_index), getflag)

#define MIB2_ipRouteIfIndex_Set(addr, index) \
    MIB2_Set_IpRouteIfIndex (addr, index)

#define MIB2_ipRouteMetric1_Get(addr, metric1, getflag) \
    MIB2_Get_IpRouteMetric1 (addr, (UINT32 *)(&metric1), getflag)

#define MIB2_ipRouteMetric1_Set(addr, metric1) \
    MIB2_Set_IpRouteMetric1 (addr, metric1)

#define MIB2_ipRouteMetric2_Get \
    MIB2_METRIC_NOT_USED

#define MIB2_ipRouteMetric3_Get \
    MIB2_METRIC_NOT_USED

#define MIB2_ipRouteMetric4_Get \
    MIB2_METRIC_NOT_USED

#define MIB2_ipRouteMetric5_Get \
    MIB2_METRIC_NOT_USED

#define MIB2_ipRouteNextHop_Get(addr, nexthop, getflag) \
    MIB2_Get_IpRouteNextHop (addr, nexthop, getflag)

#define MIB2_ipRouteNextHop_Set(addr, nexthop) \
    MIB2_Set_IpRouteNextHop (addr, nexthop)

#define MIB2_ipRouteType_Get(addr, type, getflag) \
    MIB2_Get_IpRouteType (addr, (UINT32 *)(&type), getflag)

#define MIB2_ipRouteType_Set(addr, type) \
    MIB2_Set_IpRouteType (addr, type)

#define MIB2_ipRouteProto_Get(addr, proto, getflag) \
    MIB2_Get_IpRouteProto (addr, (UINT32 *)(&proto), getflag)

#define MIB2_ipRouteMask_Get(addr, mask, getflag) \
    MIB2_Get_IpRouteMask (addr, (UINT8 *)(mask), getflag)

#define MIB2_ipRouteMask_Set(addr, mask) \
    MIB2_Set_IpRouteMask (addr, mask)

#define MIB2_ipRouteInfo_Get \
    MIB2_NULL_OID

/*
 * IP Net to Media
 */

#define MIB2_IpNetToMediaIfIndex_Get(addr, index, getflag) \
    MIB2_Get_IpNetToMediaIfIndex (addr, (INT32 *)(&index), getflag)

#define MIB2_IpNetToMediaIfIndex_Get_If(index, addr, getflag) \
    MIB2_Get_IpNetToMediaIfIndex_If ((&index), addr, getflag)

#define MIB2_IpNetToMediaIfIndex_Set(addr, index) \
    MIB2_Set_IpNetToMediaIfIndex (addr, index)

#define MIB2_ipNetToMediaPhysAddress_Get(addr, paddr, getflag) \
    MIB2_Get_IpNetToMediaPhysAddress (addr, paddr, getflag)

#define MIB2_ipNetToMediaPhysAddress_Get_If(index, addr, paddr, getflag) \
    MIB2_Get_IpNetToMediaPhysAddress_If ((&index), addr, paddr, getflag)

#define MIB2_ipNetToMediaPhysAddress_Set(addr, paddr) \
    MIB2_Set_IpNetToMediaPhysAddress (addr, paddr)

#define MIB2_ipNetToMediaNetAddress_Get(addr, ip_addr, getflag) \
    MIB2_Get_IpNetToMediaNetAddress (addr, ip_addr, getflag)

#define MIB2_ipNetToMediaNetAddress_Get_If(index, addr, ip_addr, getflag) \
    MIB2_Get_IpNetToMediaNetAddress_If ((&index), addr, ip_addr, getflag)

#define MIB2_ipNetToMediaNetAddress_Set(addr, ip_addr) \
    MIB2_Set_IpNetToMediaNetAddress (addr, ip_addr)

#define MIB2_ipNetToMediaType_Get(addr, type, getflag) \
    MIB2_Get_IpNetToMediaType (addr, (UINT32 *)(&type), getflag)

#define MIB2_ipNetToMediaType_Get_If(index, addr, type, getflag) \
    MIB2_Get_IpNetToMediaType_If ((&index),addr, (UINT32 *)(&type), getflag)

#define MIB2_ipNetToMediaType_Set(addr, type) \
    MIB2_Set_IpNetToMediaType (addr, type)

#else /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE)) */

#define MIB2_ipAdEntAddr_Get(addr, dev_addr, getflag)
#define MIB2_ipAdEntNetMask_Get(addr, net_mask)
#define MIB2_ipAdEntIfIndex_Get(addr, dev_index)
#define MIB2_ipAdEntBcastAddr_Get
#define MIB2_ipAdEntReasmMaxSize_Get
#define MIB2_ipForwarding_Get
#define MIB2_ipForwarding_Set(value)
#define MIB2_ipDefaultTTL_Get
#define MIB2_ipDefaultTTL_Set(value)
#define MIB2_ipInReceives_Get
#define MIB2_ipInReceives_Inc
#define MIB2_ipInHdrErrors_Get
#define MIB2_ipInHdrErrors_Inc
#define MIB2_ipInAddrErrors_Get
#define MIB2_ipInAddrErrors_Inc
#define MIB2_ipForwDatagrams_Get
#define MIB2_ipForwDatagrams_Inc
#define MIB2_ipInUnknownProtos_Get
#define MIB2_ipInUnknownProtos_Inc
#define MIB2_ipInDiscards_Get
#define MIB2_ipInDiscards_Inc
#define MIB2_ipInDelivers_Get
#define MIB2_ipInDelivers_Inc
#define MIB2_ipOutRequests_Get
#define MIB2_ipOutRequests_Inc
#define MIB2_ipOutDiscards_Get
#define MIB2_ipOutDiscards_Inc
#define MIB2_ipOutNoRoutes_Get
#define MIB2_ipOutNoRoutes_Inc
#define MIB2_ipReasmTimeout_Get
#define MIB2_ipReasmTimeout_Set(value)
#define MIB2_ipReasmReqds_Get
#define MIB2_ipReasmReqds_Inc
#define MIB2_ipReasmOKs_Get
#define MIB2_ipReasmOKs_Inc
#define MIB2_ipReasmFails_Get
#define MIB2_ipReasmFails_Inc
#define MIB2_ipFragOKs_Get
#define MIB2_ipFragOKs_Inc
#define MIB2_ipFragFails_Get
#define MIB2_ipFragFails_Inc
#define MIB2_ipFragCreates_Get
#define MIB2_ipFragCreates_Inc
#define MIB2_ipRoutingDiscards_Get
#define MIB2_ipRoutingDiscards_Inc
#define MIB2_ipRouteAge_Get(addr, age)
#define MIB2_ipRouteAge_Set(addr, age)
#define MIB2_ipRouteDest_Get(addr)
#define MIB2_ipRouteDest_Set(addr, addr_new)
#define MIB2_ipRouteIfIndex_Get(addr, dev_index)
#define MIB2_ipRouteIfIndex_Set(addr, index)
#define MIB2_ipRouteMetric1_Get(addr, metric1)
#define MIB2_ipRouteMetric1_Set(addr, metric1)
#define MIB2_ipRouteMetric2_Get
#define MIB2_ipRouteMetric3_Get
#define MIB2_ipRouteMetric4_Get
#define MIB2_ipRouteMetric5_Get
#define MIB2_ipRouteNextHop_Get(addr, nexthop)
#define MIB2_ipRouteNextHop_Set(addr, nexthop)
#define MIB2_ipRouteType_Get(addr, type)
#define MIB2_ipRouteType_Set(addr, type)
#define MIB2_ipRouteProto_Get(addr, proto)
#define MIB2_ipRouteMask_Get(addr, mask)
#define MIB2_ipRouteMask_Set(addr, mask)
#define MIB2_ipRouteInfo_Get
#define MIB2_IpNetToMediaIfIndex_Get(addr, index, getflag)
#define MIB2_IpNetToMediaIfIndex_Set(addr, index)
#define MIB2_ipNetToMediaPhysAddress_Get(addr, paddr, getflag)
#define MIB2_ipNetToMediaPhysAddress_Set(addr, paddr)
#define MIB2_ipNetToMediaNetAddress_Get(addr, ip_addr, getflag)
#define MIB2_ipNetToMediaNetAddress_Set(addr, ip_addr)
#define MIB2_ipNetToMediaType_Get(addr, type, getflag)
#define MIB2_ipNetToMediaType_Set(addr, type)

#endif /* ((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE)) */

/*-----------------------------------------------------------------------
 * TCP group  - Macros
 *-----------------------------------------------------------------------*/

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) )

#define MIB2_tcpConnState_Get(local_addr, local_port, remote_addr, remote_port, state, getflag) \
    MIB2_Get_TcpConnState (local_addr, (UINT32 *)&local_port, remote_addr, (UINT32 *)&remote_port, (UINT32 *)(&state), getflag)

#define MIB2_tcpConnState_Set(local_addr, local_port, remote_addr, remote_port, state) \
    MIB2_Set_TcpConnState (local_addr, local_port, remote_addr, remote_port, state)

#define MIB2_tcpConnLocalAddress_Get(local_addr, local_port, remote_addr, remote_port, addr, getflag) \
    MIB2_Get_TcpConnLocalAddress (local_addr, (UINT32 *)&local_port, remote_addr, (UINT32 *)&remote_port, (UINT8 *)(addr), getflag)

#define MIB2_tcpConnLocalPort_Get(local_addr, local_port, remote_addr, remote_port, port, getflag) \
    MIB2_Get_TcpConnLocalPort (local_addr, (UINT32 *)&local_port, remote_addr, (UINT32 *)&remote_port, (UINT32 *)(&port), getflag)

#define MIB2_tcpConnRemAddress_Get(local_addr, local_port, remote_addr, remote_port, addr, getflag) \
    MIB2_Get_TcpConnRemAddress (local_addr, (UINT32 *)&local_port, remote_addr, (UINT32 *)&remote_port, (UINT8 *)(addr), getflag)

#define MIB2_tcpConnRemPort_Get(local_addr, local_port, remote_addr, remote_port, port, getflag) \
    MIB2_Get_TcpConnRemPort (local_addr, (UINT32 *)&local_port, remote_addr, (UINT32 *)&remote_port, (UINT32 *)(&port), getflag)

#define MIB2_tcpTabSize \
    MIB2_SizeTcpTab ()

#define MIB2_tcpRtoAlgorithm_Get \
    Mib2_Tcp_Data.tcpRtoAlgorithm

#define MIB2_tcpRtoAlgorithm_Set(value) \
    Mib2_Tcp_Data.tcpRtoAlgorithm = value

#define MIB2_tcpRtoMin_Get \
    MINRTO

#define MIB2_tcpRtoMax_Get \
    MAXRTO

#define MIB2_tcpMaxConn_Get \
    TCP_MAX_PORTS

#define MIB2_tcpActiveOpens_Get \
    Mib2_Tcp_Data.tcpActiveOpens

#define MIB2_tcpActiveOpens_Inc \
    Mib2_Tcp_Data.tcpActiveOpens++

#define MIB2_tcpPassiveOpens_Get \
    Mib2_Tcp_Data.tcpPassiveOpens

#define MIB2_tcpPassiveOpens_Inc \
    Mib2_Tcp_Data.tcpPassiveOpens++

#define MIB2_tcpAttemptFails_Get \
    Mib2_Tcp_Data.tcpAttemptFails

#define MIB2_tcpAttemptFails_Inc \
    Mib2_Tcp_Data.tcpAttemptFails++

#define MIB2_tcpEstabResets_Get \
    Mib2_Tcp_Data.tcpEstabResets

#define MIB2_tcpEstabResets_Inc \
    Mib2_Tcp_Data.tcpEstabResets++

#define MIB2_tcpCurrEstab_Get \
    Mib2_Tcp_Data.tcpCurrEstab

#define MIB2_tcpCurrEstab_Inc \
    Mib2_Tcp_Data.tcpCurrEstab++

#define MIB2_tcpCurrEstab_Dec \
    Mib2_Tcp_Data.tcpCurrEstab--

#define MIB2_tcpInSegs_Get \
    Mib2_Tcp_Data.tcpInSegs

#define MIB2_tcpInSegs_Inc \
    Mib2_Tcp_Data.tcpInSegs++

#define MIB2_tcpOutSegs_Get \
    Mib2_Tcp_Data.tcpOutSegs

#define MIB2_tcpOutSegs_Inc \
    Mib2_Tcp_Data.tcpOutSegs++

#define MIB2_tcpRetransSegs_Get \
    Mib2_Tcp_Data.tcpRetransSegs

#define MIB2_tcpRetransSegs_Inc \
    Mib2_Tcp_Data.tcpRetransSegs++

#define MIB2_tcpInErrs_Get \
    Mib2_Tcp_Data.tcpInErrs

#define MIB2_tcpInErrs_Inc \
    Mib2_Tcp_Data.tcpInErrs++

#define MIB2_tcpOutRsts_Get \
    Mib2_Tcp_Data.tcpOutRsts

#define MIB2_tcpOutRsts_Inc \
    Mib2_Tcp_Data.tcpOutRsts++

#else /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) ) */

#define MIB2_tcpConnState_Get(local_addr, local_port, remote_addr, remote_port, state, getflag)
#define MIB2_tcpConnState_Set(local_addr, local_port, remote_addr, remote_port, state)
#define MIB2_tcpConnLocalAddress_Get(local_addr, local_port, remote_addr, remote_port, addr, getflag)
#define MIB2_tcpConnLocalPort_Get(local_addr, local_port, remote_addr, remote_port, port, getflag)
#define MIB2_tcpConnRemAddress_Get(local_addr, local_port, remote_addr, remote_port, addr, getflag)
#define MIB2_tcpConnRemPort_Get(local_addr, local_port, remote_addr, remote_port, port, getflag)
#define MIB2_tcpTabSize
#define MIB2_tcpRtoAlgorithm_Get
#define MIB2_tcpRtoMin_Get
#define MIB2_tcpRtoMax_Get
#define MIB2_tcpMaxConn_Get
#define MIB2_tcpActiveOpens_Get
#define MIB2_tcpActiveOpens_Inc
#define MIB2_tcpPassiveOpens_Get
#define MIB2_tcpPassiveOpens_Inc
#define MIB2_tcpAttemptFails_Get
#define MIB2_tcpAttemptFails_Inc
#define MIB2_tcpEstabResets_Get
#define MIB2_tcpEstabResets_Inc
#define MIB2_tcpCurrEstab_Get
#define MIB2_tcpCurrEstab_Inc
#define MIB2_tcpCurrEstab_Dec
#define MIB2_tcpInSegs_Get
#define MIB2_tcpInSegs_Inc
#define MIB2_tcpOutSegs_Get
#define MIB2_tcpOutSegs_Inc
#define MIB2_tcpRetransSegs_Get
#define MIB2_tcpRetransSegs_Inc
#define MIB2_tcpInErrs_Get
#define MIB2_tcpInErrs_Inc
#define MIB2_tcpOutRsts_Get
#define MIB2_tcpOutRsts_Inc

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) ) */

/*-----------------------------------------------------------------------
 * UDP Group - Macros
 *-----------------------------------------------------------------------*/

#if ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) )

#define MIB2_udpConnLocalAddress_Get(local_addr, local_port, addr, getflag) \
        MIB2_Get_UdpConnLocalAddress(local_addr, (UINT32 *)&local_port, addr, getflag)

#define MIB2_udpConnLocalPort_Get(local_addr, local_port, port, getflag) \
        MIB2_Get_UdpConnLocalPort(local_addr, (UINT32 *)(&local_port), (UINT32 *)(&port), getflag)

#define MIB2_udpTabSize \
        MIB2_SizeUdpTab ()

#define MIB2_udpInDatagrams_Get \
    Mib2_Udp_Data.udpInDatagrams

#define MIB2_udpInDatagrams_Inc \
    Mib2_Udp_Data.udpInDatagrams++

#define MIB2_udpNoPorts_Get \
    Mib2_Udp_Data.udpNoPorts

#define MIB2_udpNoPorts_Inc \
    Mib2_Udp_Data.udpNoPorts++

#define MIB2_udpInErrors_Get \
    Mib2_Udp_Data.udpInErrors

#define MIB2_udpInErrors_Inc \
    Mib2_Udp_Data.udpInErrors++

#define MIB2_udpOutDatagrams_Get \
    Mib2_Udp_Data.udpOutDatagrams

#define MIB2_udpOutDatagrams_Inc \
    Mib2_Udp_Data.udpOutDatagrams++

#else   /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) ) */

#define MIB2_udpConnLocalAddress_Get(local_addr, local_port, addr, getflag)
#define MIB2_udpConnLocalPort_Get(local_addr, local_port, port, getflag)
#define MIB2_udpTabSize
#define MIB2_udpInDatagrams_Get
#define MIB2_udpInDatagrams_Inc
#define MIB2_udpNoPorts_Get
#define MIB2_udpNoPorts_Inc
#define MIB2_udpInErrors_Get
#define MIB2_udpInErrors_Inc
#define MIB2_udpOutDatagrams_Get
#define MIB2_udpOutDatagrams_Inc

#endif /* ( (INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) ) */

/*-----------------------------------------------------------------------
 * RMON Group - Macros
 *-----------------------------------------------------------------------*/
/* Include MIB RMON1? */
#if (INCLUDE_SNMP == NU_TRUE)
#if (INCLUDE_MIB_RMON1 == NU_TRUE)

#define MIB2_BroadcastPkts_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->BroadcastPkts++

#define MIB2_MulticastPkts_Inc(dev_ptr) \
    dev_ptr->dev_mibInterface.eth->MulticastPkts++

#else

#define MIB2_BroadcastPkts_Inc(dev_ptr)
#define MIB2_MulticastPkts_Inc(dev_ptr)

#endif /* #if (INCLUDE_MIB_RMON1 == NU_TRUE) */

#else

#define MIB2_BroadcastPkts_Inc(dev_ptr)
#define MIB2_MulticastPkts_Inc(dev_ptr)

#endif /* #if (INCLUDE_SNMP == NU_TRUE) */

#if((INCLUDE_MIB2_RFC1213 == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE))
#define MIB2_UPDATE_IF_TAB_LAST_CHNG MIB2_If_Tbl_Last_Change = SysTime()
#else
#define MIB2_UPDATE_IF_TAB_LAST_CHNG
#endif

#if(INCLUDE_MIB2_RFC1213 == NU_TRUE)
#define MIB2_IfTableLastChange_Get MIB2_If_Tbl_Last_Change
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* (INCLUDE_SR_SNMP == NU_FALSE) */

#endif /* #ifndef MIB2_H */

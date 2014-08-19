/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       snmp_g.h                                                 
*
*   DESCRIPTION
*
*       This file contains function macro definitions for MIB services
*       used by Nucleus NET.
*
*   DATA STRUCTURES
*
*       None.
*
*   DEPENDENCIES
*
*       externs.h
*       target.h
*       snmp_prt.h
*       xtypes.h
*       1213xxxx.h
*       agent.h
*       link.h
*       mac.h
*       snmp_cfg.h
*       snmp.h
*       snmp_mp.h
*       snmp_ss.h
*       vacm.h
*       snmp_no.h
*
************************************************************************/

#ifndef SNMP_G_H
#define SNMP_G_H

#include "networking/externs.h"
#include "networking/target.h"

#include "networking/snmp_prt.h"
#include "networking/xtypes.h"
#include "networking/1213xxxx.h"
#include "networking/agent.h"
#include "networking/link.h"
#include "networking/mac.h"
#include "networking/snmp_cfg.h"
#include "networking/snmp.h"
#include "networking/snmp_mp.h"
#include "networking/snmp_ss.h"
#include "networking/vacm.h"
#include "networking/snmp_no.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */


/* If RMON is included, the task size must be larger */
#if ( (INCLUDE_MIB_RMON1 == NU_FALSE) || (RFC1757_RMON_LITE == NU_TRUE) )
#define     SNMP_TASK_SIZE  9000
#else
#define     SNMP_TASK_SIZE  11000
#endif




/* This macro defines whether the current version of NET is 4.4b or
 * greater
 */
#define     SNMP_NET_4_4_B  NU_TRUE

/* This macro defines the SNMP version. */
#define     SNMP_VER        2

#if (RFC1213_SYS_INCLUDE == NU_TRUE)
STATUS   NU_SNMP_System_Group_Initialize(const rfc1213_sys_t *sys_group);
#else
#define  NU_SNMP_System_Group_Initialize(sys_group)
#endif

VOID     NU_SNMP_Init_Task(UNSIGNED argc, VOID *argv);
VOID     SNMP_Wait_For_Init (VOID);
STATUS   NU_SNMP_Init(VOID);

#define NU_SNMP_Initialize()    NU_SNMP_Init()

STATUS      NU_SNMP_Set_Host_Id(const UINT8*);
#if (INCLUDE_IPV6 == NU_TRUE)
STATUS      NU_SNMP_Set_Host_Id6(UINT8 *addr);
#endif
VOID        SNMP_Task_Entry(UNSIGNED argc, VOID *argv);


/* If the application is a pre-SNMP 1.4 application, #define out
 * xsnmp_task, and SNMP_System_Group_Initialize, and define
 * SNMP_Initialize to be NU_SNMP_Initialize, because NU_SNMP_Initialize
 * will call NU_SNMP_System_Group_Initialize and create the SNMP Task.
 */
#define     xsnmp_task()
#define     SNMP_System_Group_Initialize
#define     SNMP_Initialize     NU_SNMP_Initialize

#define     SR_SNMP_Initialize()
#define     NU_Add_Community    MibAddComm
#define     NU_Add_Host         MibAddHost
#define     NU_Clear_Community  MibClearComm
#define     NU_Clear_Host       AgentClearHost

#define     VAN_JACOBSON        MIB2_VAN_JACOBSON

/* Define these macros to nothing since Nucleus SNMP has been changed
 * to use the NET tables.
 */
#define     SNMP_ipNetToMediaTableUpdate(action, index, mac_address, \
                                         dev_address, parm)
#define     SNMP_atTableUpdate(action, index, mac_address, dev_address)
#define     SNMP_ipAdEntUpdate(action, index, dev_address, dev_mask, \
                               parm1, parm2)
#define     SNMP_ifCreate(index)
#define     SNMP_ifOutQLen_Dec(index)

#if (INCLUDE_MIB_RMON1==NU_TRUE)
#include "rmon/inc/1757xxxx.h"
#endif

/* INCLUDE_PPP_MIB is defined in snmp_prt.h */
#if (INCLUDE_PPP_MIB == NU_TRUE)
#include "drivers/nu_ppp.h"
#include "networking/ppp147x.h"
#else
#define INCLUDE_LCP_MIB         NU_FALSE
#define INCLUDE_SEC_MIB         NU_FALSE
#define INCLUDE_NCP_MIB         NU_FALSE
#endif

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)
#if (MAX_PORTS < 2)
#error MAX_PORTS should be at least 2. There will be two interfaces
#endif
#endif

extern rfc1213_vars_t   rfc1213_vars;
extern snmp_cfig_t      snmp_cfg;

VOID set_1213outQLen                (UINT32 unit, UINT32 value);
VOID set_1213outErrors              (UINT32 unit, UINT32 value);
VOID set_1213outDiscards            (UINT32 unit, UINT32 value);
VOID set_1213outNUcastPkts          (UINT32 unit, UINT32 value);
VOID set_1213outUcastPkts           (UINT32 unit, UINT32 value);
VOID set_1213outOctets              (UINT32 unit, UINT32 value);
VOID set_1213inUcastPkts            (UINT32 unit, UINT32 value);
VOID set_1213inNUcastPkts           (UINT32 unit, UINT32 value);
VOID set_1213inUnknownProtos        (UINT32 unit, UINT32 value);
VOID set_1213inErrors               (UINT32 unit, UINT32 value);
VOID set_1213inDiscards             (UINT32 unit, UINT32 value);
VOID set_1213inOctets               (UINT32 unit, UINT32 value);
VOID set_1213ifType                 (UINT32 unit, UINT32 value);
VOID set_1213ifMtu                  (UINT32 unit, UINT32 value);
VOID set_1213ifSpeed                (UINT32 unit, UINT32 value);
VOID set_1213ifPhysAddress          (UINT32 unit, UINT8 *value);
VOID set_1213ifAdminStatus          (UINT32 unit, UINT32 value);
VOID set_1213ifOperStatus           (UINT32 unit, UINT32 value);
VOID set_1213ifLastChange           (UINT32 unit, UINT32 value);
VOID set_1213ifIndex                (UINT32 unit);
VOID set_1213ifDescr                (UINT32 unit, CHAR *descr);

VOID xrmon1_pktcontent              (UINT32 unit, UINT32 size,
                                     UINT8 *pkt);
UINT32 xrmon1_pktsize               (UINT32 unit, UINT32 size);
VOID xset_1757Octets                (UINT32 unit, UINT32 value);
VOID xset_1757Pkts                  (UINT32 unit, UINT32 value);
VOID xset_1757BroadcastPkts         (UINT32 unit, UINT32 value);
VOID xset_1757MulticastPkts         (UINT32 unit, UINT32 value);
VOID xset_1757CRCAlignErrors        (UINT32 unit, UINT32 value);
VOID xset_1757UndersizePkts         (UINT32 unit, UINT32 value);
VOID xset_1757OversizePkts          (UINT32 unit, UINT32 value);
VOID xset_1757Fragments             (UINT32 unit, UINT32 value);
VOID xset_1757Jabbers               (UINT32 unit, UINT32 value);
VOID xset_1757Collisions            (UINT32 unit, UINT32 value);
VOID xset_1757Pkts64Octets          (UINT32 unit, UINT32 value);
VOID xset_1757Pkts65to127Octets     (UINT32 unit, UINT32 value);
VOID xset_1757Pkts128to255Octets    (UINT32 unit, UINT32 value);
VOID xset_1757Pkts256to511Octets    (UINT32 unit, UINT32 value);
VOID xset_1757Pkts512to1023Octets   (UINT32 unit, UINT32 value);
VOID xset_1757Pkts1024to1518Octets  (UINT32 unit, UINT32 value);

#define SNMP_System_Group   rfc1213_sys_t

/* Define service completion status constants.  */
#define SNMP_INVALID_POINTER        NU_INVALID_POINTER
#define SNMP_INVALID_PARAMETER      -100

/* Command definitions. */
#define     SNMP_ADD            1
#define     SNMP_DELETE         2

/* Definitions related to the IP group of MIB2. */
#define RFC1213_IP_FORWARD          1
#define RFC1213_IP_NO_FORWARD       2

/* Definitions for the interface group of MIB2. */
#define RFC1213_IF_UP               1
#define RFC1213_IF_DOWN             2
#define RFC1213_IF_TESTING          3

#define SNMP_sysUpTime(value)       \
    rfc1213_vars.rfc1213_sys.sysUpTime = value

#define SNMP_sysUpTime_Inc    \
    rfc1213_vars.rfc1213_sys.sysUpTime++

#if (RFC1213_SYS_INCLUDE == NU_TRUE)
/* These macros are for updating the System Group variables. */
#define SNMP_sysDescr(string)       \
    strcpy((char *)rfc1213_vars.rfc1213_sys.sysDescr, (char *)string)

#define SNMP_sysObjectID(string)    \
    strcpy((char *)rfc1213_vars.rfc1213_sys.sysObjectID, (char *)string)

#define SNMP_sysContact(string)     \
    strcpy((char *)rfc1213_vars.rfc1213_sys.sysContact, (char *)string)

#define SNMP_sysName(string)        \
    strcpy((char *)rfc1213_vars.rfc1213_sys.sysName, (char *)string)

#define SNMP_sysLocation(string)    \
    strcpy((char *)rfc1213_vars.rfc1213_sys.sysLocation, (char *)string)

#define SNMP_sysServices(value)     \
    rfc1213_vars.rfc1213_sys.sysServices = value

#else
#define SNMP_sysDescr(string)
#define SNMP_sysObjectID(string)
#define SNMP_sysContact(string)
#define SNMP_sysName(string)
#define SNMP_sysLocation(string)
#define SNMP_sysServices(value)
#endif

#if (RFC1213_IP_INCLUDE == NU_TRUE)
/* These macros are for updating the IP Group. */
#define SNMP_ipForwarding(value)     \
    MIB2_ipForwarding_Set(value)

#define SNMP_ipDefaultTTL(value)     \
    MIB2_ipDefaultTTL_Set(value)

#define SNMP_ipInReceives_Inc     \
    MIB2_ipInReceives_Inc

#define SNMP_ipInHdrErrors_Inc    \
    MIB2_ipInHdrErrors_Inc

#define SNMP_ipInAddrErrors_Inc   \
    MIB2_ipInAddrErrors_Inc

#define SNMP_ipForwDatagrams_Inc  \
    MIB2_ipForwDatagrams_Inc

#define SNMP_ipInUnknownProtos_Inc  \
    MIB2_ipInUnknownProtos_Inc

#define SNMP_ipInDiscards_Inc  \
    MIB2_ipInDiscards_Inc

#define SNMP_ipInDelivers_Inc  \
    MIB2_ipInDelivers_Inc

#define SNMP_ipOutRequests_Inc  \
    MIB2_ipOutRequests_Inc

#define SNMP_ipOutDiscards_Inc  \
    MIB2_ipOutDiscards_Inc

#define SNMP_ipOutNoRoutes_Inc  \
    MIB2_ipOutNoRoutes_Inc

#define SNMP_ipReasmTimeout(value)  \
    MIB2_ipReasmTimeout_Set(value)

#define SNMP_ipReasmReqds_Inc  \
    MIB2_ipReasmReqds_Inc

#define SNMP_ipReasmOKs_Inc  \
    MIB2_ipReasmOKs_Inc

#define SNMP_ipReasmFails_Inc  \
    MIB2_ipReasmFails_Inc

#define SNMP_ipFragOKs_Inc  \
    MIB2_ipFragOKs_Inc

#define SNMP_ipFragFails_Inc  \
    MIB2_ipFragFails_Inc

#define SNMP_ipFragCreates_Inc  \
    MIB2_ipFragCreates_Inc

#define SNMP_ipRoutingDiscards_Inc \
    MIB2_ipRoutingDiscards_Inc

#else
#define SNMP_ipForwarding(value)
#define SNMP_ipDefaultTTL(value)
#define SNMP_ipInReceives_Inc
#define SNMP_ipInHdrErrors_Inc
#define SNMP_ipInAddrErrors_Inc
#define SNMP_ipForwDatagrams_Inc
#define SNMP_ipInUnknownProtos_Inc
#define SNMP_ipInDiscards_Inc
#define SNMP_ipInDelivers_Inc
#define SNMP_ipOutRequests_Inc
#define SNMP_ipOutDiscards_Inc
#define SNMP_ipOutNoRoutes_Inc
#define SNMP_ipReasmTimeout(value)
#define SNMP_ipReasmReqds_Inc
#define SNMP_ipReasmOKs_Inc
#define SNMP_ipReasmFails_Inc
#define SNMP_ipFragOKs_Inc
#define SNMP_ipFragFails_Inc
#define SNMP_ipFragCreates_Inc
#define SNMP_ipRoutingDiscards_Inc

#endif

#if (RFC1213_ICMP_INCLUDE == NU_TRUE)
/* These macros are for updating the ICMP Group. */
#define SNMP_icmpInMsgs_Inc   \
    MIB2_icmpInMsgs_Inc

#define SNMP_icmpInErrors_Inc   \
    MIB2_icmpInErrors_Inc

#define SNMP_icmpInDestUnreachs_Inc   \
    MIB2_icmpInDestUnreachs_Inc

#define SNMP_icmpInTimeExcds_Inc   \
    MIB2_icmpInTimeExcds_Inc

#define SNMP_icmpInParmProbs_Inc   \
    MIB2_icmpInParmProbs_Inc

#define SNMP_icmpInSrcQuenchs_Inc   \
    MIB2_icmpInSrcQuenchs_Inc

#define SNMP_icmpInRedirects_Inc   \
    MIB2_icmpInRedirects_Inc

#define SNMP_icmpInEchos_Inc   \
    MIB2_icmpInEchos_Inc

#define SNMP_icmpInEchoReps_Inc   \
    MIB2_icmpInEchoReps_Inc

#define SNMP_icmpInTimeStamps_Inc   \
    MIB2_icmpInTimeStamps_Inc

#define SNMP_icmpInTimeStampReps_Inc   \
    MIB2_icmpInTimeStampReps_Inc

#define SNMP_icmpInAddrMasks_Inc   \
    MIB2_icmpInAddrMasks_Inc

#define SNMP_icmpInAddrMaskReps_Inc   \
    MIB2_icmpInAddrMaskReps_Inc

#define SNMP_icmpOutMsgs_Inc   \
    MIB2_icmpOutMsgs_Inc

#define SNMP_icmpOutErrors_Inc   \
    MIB2_icmpOutErrors_Inc

#define SNMP_icmpOutDestUnreachs_Inc   \
    MIB2_icmpOutDestUnreachs_Inc

#define SNMP_icmpOutTimeExcds_Inc   \
    MIB2_icmpOutTimeExcds_Inc

#define SNMP_icmpOutParmProbs_Inc   \
    MIB2_icmpOutParmProbs_Inc

#define SNMP_icmpOutSrcQuenchs_Inc   \
    MIB2_icmpOutSrcQuenchs_Inc

#define SNMP_icmpOutRedirects_Inc   \
    MIB2_icmpOutRedirects_Inc

#define SNMP_icmpOutEchos_Inc   \
    MIB2_icmpOutEchos_Inc

#define SNMP_icmpOutEchoReps_Inc   \
    MIB2_icmpOutEchoReps_Inc

#define SNMP_icmpOutTimestamps_Inc   \
    MIB2_icmpOutTimestamps_Inc

#define SNMP_icmpOutTimestampReps_Inc   \
    MIB2_icmpOutTimestampReps_Inc

#define SNMP_icmpOutAddrMasks_Inc   \
    MIB2_icmpOutAddrMasks_Inc

#define SNMP_icmpOutAddrMaskReps_Inc   \
    MIB2_icmpOutAddrMaskReps_Inc

#else
#define SNMP_icmpInMsgs_Inc
#define SNMP_icmpInErrors_Inc
#define SNMP_icmpInDestUnreachs_Inc
#define SNMP_icmpInTimeExcds_Inc
#define SNMP_icmpInParmProbs_Inc
#define SNMP_icmpInSrcQuenchs_Inc
#define SNMP_icmpInRedirects_Inc
#define SNMP_icmpInEchos_Inc
#define SNMP_icmpInEchoReps_Inc
#define SNMP_icmpInTimeStamps_Inc
#define SNMP_icmpInTimeStampReps_Inc
#define SNMP_icmpInAddrMasks_Inc
#define SNMP_icmpInAddrMaskReps_Inc
#define SNMP_icmpOutMsgs_Inc
#define SNMP_icmpOutErrors_Inc
#define SNMP_icmpOutDestUnreachs_Inc
#define SNMP_icmpOutTimeExcds_Inc
#define SNMP_icmpOutParmProbs_Inc
#define SNMP_icmpOutSrcQuenchs_Inc
#define SNMP_icmpOutRedirects_Inc
#define SNMP_icmpOutEchos_Inc
#define SNMP_icmpOutEchoReps_Inc
#define SNMP_icmpOutTimestamps_Inc
#define SNMP_icmpOutTimestampReps_Inc
#define SNMP_icmpOutAddrMasks_Inc
#define SNMP_icmpOutAddrMaskReps_Inc
#endif

#if (RFC1213_TCP_INCLUDE == NU_TRUE)
/* These macros are for updating the TCP Group. */
#define SNMP_tcpRtoAlgorithm(value) \
    MIB2_tcpRtoAlgorithm_Set(value)

/* value is the minimum RTO in ticks. */
#define SNMP_tcpRtoMin(value)
#define SNMP_tcpRtoMax(value)
#define SNMP_tcpMaxCon(value)

#define SNMP_tcpActiveOpens_Inc   \
    MIB2_tcpActiveOpens_Inc

#define SNMP_tcpPassiveOpens_Inc   \
    MIB2_tcpPassiveOpens_Inc

#define SNMP_tcpAttemptFails_Inc   \
    MIB2_tcpAttemptFails_Inc

#define SNMP_tcpEstabResets_Inc   \
    MIB2_tcpEstabResets_Inc

#define SNMP_tcpInSegs_Inc   \
    MIB2_tcpInSegs_Inc

#define SNMP_tcpOutSegs_Inc   \
    MIB2_tcpOutSegs_Inc

#define SNMP_tcpRetransSegs_Inc   \
    MIB2_tcpRetransSegs_Inc

#define SNMP_tcpInErrs_Inc   \
    MIB2_tcpInErrs_Inc

#define SNMP_tcpOutRsts_Inc   \
    MIB2_tcpOutRsts_Inc

#else
#define SNMP_tcpRtoAlgorithm(value)
#define SNMP_tcpRtoMin(value)
#define SNMP_tcpRtoMax(value)
#define SNMP_tcpMaxCon(value)
#define SNMP_tcpActiveOpens_Inc
#define SNMP_tcpPassiveOpens_Inc
#define SNMP_tcpAttemptFails_Inc
#define SNMP_tcpEstabResets_Inc
#define SNMP_tcpInSegs_Inc
#define SNMP_tcpOutSegs_Inc
#define SNMP_tcpRetransSegs_Inc
#define SNMP_tcpInErrs_Inc
#define SNMP_tcpOutRsts_Inc
#endif

#if (RFC1213_UDP_INCLUDE == NU_TRUE)
/* These macros are for updating the UDP Group. */

#define SNMP_udpInDatagrams_Inc   \
    MIB2_udpInDatagrams_Inc

#define SNMP_udpNoPorts_Inc   \
    MIB2_udpNoPorts_Inc

#define SNMP_udpInErrors_Inc   \
    MIB2_udpInErrors_Inc

#define SNMP_udpoutDatagrams_Inc   \
    MIB2_udpOutDatagrams_Inc

#else
#define SNMP_udpInDatagrams_Inc
#define SNMP_udpNoPorts_Inc
#define SNMP_udpInErrors_Inc
#define SNMP_udpoutDatagrams_Inc
#endif

#if (RFC1213_IF_INCLUDE == NU_TRUE)

#define SNMP_ifTotalInterfaces(value)
#define SNMP_ifNumber()
#define SNMP_ifIndex(index)

#define SNMP_ifDescr(index, string)    \
    MIB2_ifDescr_Seti(index, string)

#define SNMP_ifType(index, value)  \
    MIB2_ifType_Seti(index, value)

#define SNMP_ifMtu(index, value)  \
    MIB2_ifMtu_Set(index, value)

#define SNMP_ifSpeed(index, value)  \
    MIB2_ifSpeed_Seti(index, value)

#define SNMP_ifPhysAddress(index, addr)

#define SNMP_ifAdminStatus(index, status)

#define SNMP_ifOperStatus(index, status)

#define SNMP_ifLastChange(index, time) \
    MIB2_ifLastChange_Set(index, time)

#define SNMP_ifInOctets(index, value) \
    MIB2_ifInOctets_Addi(index, value)

#define SNMP_ifInUcastPkts_Inc(index) \
    MIB2_ifInUcastPkts_Inci(index)

#define SNMP_ifInNUcastPkts_Inc(index) \
    MIB2_ifInNUcastPkts_Inci(index)

#define SNMP_ifInDiscards_Inc(index) \
    MIB2_ifInDiscards_Inci(index)

#define SNMP_ifInErrors_Inc(index) \
    MIB2_ifInErrors_Inci(index)

#define SNMP_ifInUnknownProtos_Inc(index) \
    MIB2_ifInUnknownProtos_Inci(index)

#define SNMP_ifOutOctets(index, value) \
    MIB2_ifOutOctets_Addi(index, value)

#define SNMP_ifOutUcastPkts_Inc(index) \
    MIB2_ifOutUcastPkts_Inci(index)

#define SNMP_ifOutNUcastPkts_Inc(index) \
    MIB2_ifOutNUcastPkts_Inci(index)

#define SNMP_ifOutDiscards_Inc(index) \
    MIB2_ifOutDiscards_Inci(index)

#define SNMP_ifOutErrors_Inc(index) \
    MIB2_ifOutErrors_Inci(index)


#define SNMP_ifOutQLen_Inc(index) \
    MIB2_ifOutQLen_Inc(index)

#define SNMP_ifSpecific(index, string) \
    MIB2_ifSpecific_Set(index, (UINT8 *)string)

#else
#define SNMP_ifTotalInterfaces(value)
#define SNMP_ifNumber()
#define SNMP_ifIndex(index)
#define SNMP_ifDescr(index, string)
#define SNMP_ifType(index, value)
#define SNMP_ifMtu(index, value)
#define SNMP_ifSpeed(index, value)
#define SNMP_ifPhysAddress(index, addr)
#define SNMP_ifAdminStatus(index, status)
#define SNMP_ifOperStatus(index, status)
#define SNMP_ifLastChange(index, time)
#define SNMP_ifInOctets(index, value)
#define SNMP_ifInUcastPkts_Inc(index)
#define SNMP_ifInNUcastPkts_Inc(index)
#define SNMP_ifInDiscards_Inc(index)
#define SNMP_ifInErrors_Inc(index)
#define SNMP_ifInUnknownProtos_Inc(index)
#define SNMP_ifOutOctets(index, value)
#define SNMP_ifOutUcastPkts_Inc(index)
#define SNMP_ifOutNUcastPkts_Inc(index)
#define SNMP_ifOutDiscards_Inc(index)
#define SNMP_ifOutErrors_Inc(index)
#define SNMP_ifOutQLen_Inc(index)
#define SNMP_ifSpecific(index, string)
#endif

#if (INCLUDE_MIB_RMON1==NU_TRUE)
#define RMON_Octets(index, value) \
    MIB2_Add_1757Octets(index - 1, value, NU_FALSE)

#define RMON_Pkts_Inc(index) \
    MIB2_Add_1757Pkts(index - 1, 1, NU_FALSE)

#define RMON_BroadcastPkts_Inc(index) \
    MIB2_Add_1757BroadcastPkts(index - 1, 1, NU_FALSE)

#define RMON_MulticastPkts_Inc(index) \
    MIB2_Add_1757MulticastPkts(index - 1, 1, NU_FALSE)

#define RMON_CRCAlignErrors_Inc(index) \
    MIB2_Add_1757CRCAlignErrors(index - 1, 1, NU_FALSE)

#define RMON_UndersizePkts_Inc(index) \
    MIB2_Add_1757UndersizePkts(index - 1, 1, NU_FALSE)

#define RMON_OversizePkts_Inc(index) \
    MIB2_Add_1757OversizePkts(index - 1, 1, NU_FALSE)

#define RMON_Fragments_Inc(index) \
    MIB2_Add_1757Fragments(index - 1, 1, NU_FALSE)

#define RMON_Jabbers_Inc(index) \
    MIB2_Add_1757Jabbers(index - 1, 1, NU_FALSE)

#define RMON_Collisions_Inc(index) \
    MIB2_Add_1757Collisions(index - 1, 1, NU_FALSE)

#define RMON_Pkts64Octets_Inc(index) \
    MIB2_Add_1757Pkts64Octets(index - 1, 1, NU_FALSE)

#define RMON_Pkts65to127Octets_Inc(index) \
    MIB2_Add_1757Pkts65to127Octets(index - 1, 1, NU_FALSE)

#define RMON_Pkts128to255Octets_Inc(index) \
    MIB2_Add_1757Pkts128to255Octets(index - 1, 1, NU_FALSE)

#define RMON_Pkts256to511Octets_Inc(index) \
    MIB2_Add_1757Pkts256to511Octets(index - 1, 1, NU_FALSE)

#define RMON_Pkts512to1023Octets_Inc(index) \
    MIB2_Add_1757Pkts512to1023Octets(index - 1, 1, NU_FALSE)

#define RMON_Pkts1024to1518Octets_Inc(index) \
    MIB2_Add_1757Pkts1024to1518Octets(index - 1, 1, NU_FALSE)
#endif

#define xsnmp_pkttype               snmp_pkttype
#define xset_1213outQLen            set_1213outQLen
#define xset_1213outErrors          set_1213outErrors
#define xset_1213outDiscards        set_1213outDiscards
#define xset_1213outNUcastPkts      set_1213outNUcastPkts
#define xset_1213outUcastPkts       set_1213outUcastPkts
#define xset_1213outOctets          set_1213outOctets
#define xset_1213inUcastPkts        set_1213inUcastPkts
#define xset_1213inNUcastPkts       set_1213inNUcastPkts
#define xset_1213inUnknownProtos    set_1213inUnknownProtos
#define xset_1213inErrors           set_1213inErrors
#define xset_1213inDiscards         set_1213inDiscards
#define xset_1213inOctets           set_1213inOctets
#define xset_1213ifType             set_1213ifType
#define xset_1213ifMtu              set_1213ifMtu
#define xset_1213ifSpeed            set_1213ifSpeed
#define xset_1213ifPhysAddress      set_1213ifPhysAddress
#define xset_1213ifAdminStatus      set_1213ifAdminStatus
#define xset_1213ifOperStatus       set_1213ifOperStatus
#define xset_1213ifLastChange       set_1213ifLastChange
#define xset_1213ifIndex            set_1213ifIndex
#define xset_1213ifDescr            set_1213ifDescr

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_H */

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

/***********************************************************************
*
*   FILE NAME
*
*       isnmp.h
*
*   COMPONENT
*
*       Configuration
*
*   DESCRIPTION
*
*       The only purpose of this file is to control the inclusion of
*       Nucleus SNMP code into the Nucleus NET library.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*      None
*
***********************************************************************/

#ifndef ISNMP_H
#define ISNMP_H

#include "networking/net_cfg.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++  */
#endif /* _cplusplus */

#if (INCLUDE_SNMP == NU_TRUE)


/* This macro specifies the SNMP glue header file to include. The first
   (snmp_g.h) is for Nucleus SNMP. The second (sr_snmp_g.h) is for SNMP
   Research SNMP*/
#define SNMP_GLUE  "networking/snmp_g.h"
/* #define SNMP_GLUE  "networking/sr_snmpg.h" */

/* If SNMP is included, but MIB-II is not included, SNMP Research SNMP is
 * being used.  Define the MIB-II macros to the SNMP macros expected by
 * SNMP Research.
 */
#if (INCLUDE_SR_SNMP == NU_TRUE)

#define MIB2_ipForwarding_Set(value)                SNMP_ipForwarding(value)
#define MIB2_ipDefaultTTL_Set(value)                SNMP_ipDefaultTTL(value)
#define MIB2_ipInReceives_Inc                       SNMP_ipInReceives_Inc
#define MIB2_ipInHdrErrors_Inc                      SNMP_ipInHdrErrors_Inc
#define MIB2_ipInAddrErrors_Inc                     SNMP_ipInAddrErrors_Inc
#define MIB2_ipForwDatagrams_Inc                    SNMP_ipForwDatagrams_Inc
#define MIB2_ipInUnknownProtos_Inc                  SNMP_ipInUnknownProtos_Inc
#define MIB2_ipInDiscards_Inc                       SNMP_ipInDiscards_Inc
#define MIB2_ipInDelivers_Inc                       SNMP_ipInDelivers_Inc
#define MIB2_ipOutRequests_Inc                      SNMP_ipOutRequests_Inc
#define MIB2_ipOutDiscards_Inc                      SNMP_ipOutDiscards_Inc
#define MIB2_ipOutNoRoutes_Inc                      SNMP_ipOutNoRoutes_Inc
#define MIB2_ipReasmTimeout_Set(value)              SNMP_ipReasmTimeout((INT32)value)
#define MIB2_ipReasmReqds_Inc                       SNMP_ipReasmReqds_Inc
#define MIB2_ipReasmOKs_Inc                         SNMP_ipReasmOKs_Inc
#define MIB2_ipReasmFails_Inc                       SNMP_ipReasmFails_Inc
#define MIB2_ipFragOKs_Inc                          SNMP_ipFragOKs_Inc
#define MIB2_ipFragFails_Inc                        SNMP_ipFragFails_Inc
#define MIB2_ipFragCreates_Inc                      SNMP_ipFragCreates_Inc
#define MIB2_ipRoutingDiscards_Inc                  SNMP_ipRoutingDiscards_Inc
#define MIB2_icmpInMsgs_Inc                         SNMP_icmpInMsgs_Inc
#define MIB2_icmpInErrors_Inc                       SNMP_icmpInErrors_Inc
#define MIB2_icmpInDestUnreachs_Inc                 SNMP_icmpInDestUnreachs_Inc
#define MIB2_icmpInTimeExcds_Inc                    SNMP_icmpInTimeExcds_Inc
#define MIB2_icmpInParmProbs_Inc                    SNMP_icmpInParmProbs_Inc
#define MIB2_icmpInSrcQuenchs_Inc                   SNMP_icmpInSrcQuenchs_Inc
#define MIB2_icmpInRedirects_Inc                    SNMP_icmpInRedirects_Inc
#define MIB2_icmpInEchos_Inc                        SNMP_icmpInEchos_Inc
#define MIB2_icmpInEchoReps_Inc                     SNMP_icmpInEchoReps_Inc
#define MIB2_icmpInTimeStamps_Inc                   SNMP_icmpInTimeStamps_Inc
#define MIB2_icmpInTimeStampReps_Inc                SNMP_icmpInTimeStampReps_Inc
#define MIB2_icmpInAddrMasks_Inc                    SNMP_icmpInAddrMasks_Inc
#define MIB2_icmpInAddrMaskReps_Inc                 SNMP_icmpInAddrMaskReps_Inc
#define MIB2_icmpOutMsgs_Inc                        SNMP_icmpOutMsgs_Inc
#define MIB2_icmpOutErrors_Inc                      SNMP_icmpOutErrors_Inc
#define MIB2_icmpOutDestUnreachs_Inc                SNMP_icmpOutDestUnreachs_Inc
#define MIB2_icmpOutTimeExcds_Inc                   SNMP_icmpOutTimeExcds_Inc
#define MIB2_icmpOutParmProbs_Inc                   SNMP_icmpOutParmProbs_Inc
#define MIB2_icmpOutSrcQuenchs_Inc                  SNMP_icmpOutSrcQuenchs_Inc
#define MIB2_icmpOutRedirects_Inc                   SNMP_icmpOutRedirects_Inc
#define MIB2_icmpOutEchos_Inc                       SNMP_icmpOutEchos_Inc
#define MIB2_icmpOutEchoReps_Inc                    SNMP_icmpOutEchoReps_Inc
#define MIB2_icmpOutTimestamps_Inc                  SNMP_icmpOutTimestamps_Inc
#define MIB2_icmpOutTimestampReps_Inc               SNMP_icmpOutTimestampReps_Inc
#define MIB2_icmpOutAddrMasks_Inc                   SNMP_icmpOutAddrMasks_Inc
#define MIB2_icmpOutAddrMaskReps_Inc                SNMP_icmpOutAddrMaskReps_Inc
#define MIB2_tcpRtoAlgorithm_Set(value)             SNMP_tcpRtoAlgorithm(value)
#define MIB2_tcpActiveOpens_Inc                     SNMP_tcpActiveOpens_Inc
#define MIB2_tcpPassiveOpens_Inc                    SNMP_tcpPassiveOpens_Inc
#define MIB2_tcpAttemptFails_Inc                    SNMP_tcpAttemptFails_Inc
#define MIB2_tcpEstabResets_Inc                     SNMP_tcpEstabResets_Inc
#define MIB2_tcpInSegs_Inc                          SNMP_tcpInSegs_Inc
#define MIB2_tcpOutSegs_Inc                         SNMP_tcpOutSegs_Inc
#define MIB2_tcpRetransSegs_Inc                     SNMP_tcpRetransSegs_Inc
#define MIB2_tcpInErrs_Inc                          SNMP_tcpInErrs_Inc
#define MIB2_tcpOutRsts_Inc                         SNMP_tcpOutRsts_Inc
#define MIB2_tcpCurrEstab_Inc
#define MIB2_tcpCurrEstab_Dec
#define MIB2_udpInDatagrams_Inc                     SNMP_udpInDatagrams_Inc
#define MIB2_udpNoPorts_Inc                         SNMP_udpNoPorts_Inc
#define MIB2_udpInErrors_Inc                        SNMP_udpInErrors_Inc
#define MIB2_udpOutDatagrams_Inc                    SNMP_udpoutDatagrams_Inc
#define MIB2_ifMtu_Set(index, value)                SNMP_ifMtu(index, value)
#define MIB2_ifAddr_Set(index, addr)                SNMP_ifPhysAddress(index, addr)
#define MIB2_Set_IfStatusAdmin (index, status)      SNMP_ifAdminStatus(index, status)
#define MIB2_ifLastChange_Set(index, time)          SNMP_ifLastChange(index, time)
#define MIB2_ifOutQLen_Inc(index)                   SNMP_ifOutQLen_Inc((INT32)index)
#define MIB2_ifOutQLen_Dec(index)                   SNMP_ifOutQLen_Dec(index)
#define MIB2_ifSpecific_Set(index, string)          SNMP_ifSpecific(index, string)
#define MIB2_ifDescr_Set(device, string)            SNMP_ifDescr((INT32)device->dev_index, string)
#define MIB2_ifType_Set(device, value)              SNMP_ifType((INT32)device->dev_index, value)
#define MIB2_ifSpeed_Set(device, value)             SNMP_ifSpeed((INT32)device->dev_index, value)
#define MIB2_ifInOctets_Add(device, value)          SNMP_ifInOctets((INT32)device->dev_index, (INT32)value)
#define MIB2_ifOutOctets_Add(device, value)         SNMP_ifOutOctets((INT32)device->dev_index, (INT32)value)
#define MIB2_ifOutDiscards_Inc(device)              SNMP_ifOutDiscards_Inc((INT32)device->dev_index)
#define MIB2_ifInNUcastPkts_Inc(device)             SNMP_ifInNUcastPkts_Inc((INT32)device->dev_index)
#define MIB2_ifInUcastPkts_Inc(device)              SNMP_ifInUcastPkts_Inc((INT32)device->dev_index)
#define MIB2_ifInUnknownProtos_Inc(device)          SNMP_ifInUnknownProtos_Inc((INT32)device->dev_index)
#define MIB2_ifOutNUcastPkts_Inc(device)            SNMP_ifOutNUcastPkts_Inc((INT32)device->dev_index)
#define MIB2_ifOutUcastPkts_Inc(device)             SNMP_ifOutUcastPkts_Inc((INT32)device->dev_index)
#define MIB2_ifType_Seti(index, value)              SNMP_ifType((INT32)index, value)
#define MIB2_ifDescr_Seti(index, string)            SNMP_ifDescr((INT32)index, string)
#define MIB2_ifSpeed_Seti(index, value)             SNMP_ifSpeed((INT32)index, value)
#define MIB2_ifOutNUcastPkts_Inci(index)            SNMP_ifOutNUcastPkts_Inc((INT32)index)
#define MIB2_InMulticast_Pkts_Inc(device)
#define MIB2_OutMulticast_Pkts_Inc(device)
#define MIB2_InBroadcast_Pkts_Inc(device)
#define MIB2_OutBroadcast_Pkts_Inc(device)


#endif

#else

/* If no SNMP support is desired then define the SNMP macros to be null strings. */
#define NU_SNMP_Initialize()
#define SR_SNMP_Initialize()
#define SNMP_sysDescr(string)
#define SNMP_sysObjectID(string)
#define SNMP_sysUpTime(value)
#define SNMP_sysUpTime_Inc
#define SNMP_sysContact(string)
#define SNMP_sysName(string)
#define SNMP_sysLocation(string)
#define SNMP_sysServices(value)
#define SNMP_ipNetToMediaTableUpdate
#define SNMP_ipAdEntUpdate
#define SNMP_atTableUpdate
#define SNMP_ipInReceives_Inc
#define SNMP_ipInHdrErrors_Inc
#define SNMP_ipInAddrErrors_Inc
#define SNMP_ipForwarding
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
#define SNMP_udpInDatagrams_Inc
#define SNMP_udpNoPorts_Inc
#define SNMP_udpInErrors_Inc
#define SNMP_udpoutDatagrams_Inc
#define SNMP_ifTotalInterfaces(value)
#define SNMP_ifCreate(index)
#define SNMP_ifNumber(value)
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
#define SNMP_ifOutQLen_Dec(index)
#define SNMP_ifSpecific(index, string)

#endif /* INCLUDE_SNMP */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* ifndef ISNMP_H */

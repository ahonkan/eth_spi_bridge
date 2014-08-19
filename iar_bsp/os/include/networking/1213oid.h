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
*       1213oid.h                                                
*
*   COMPONENT
*
*       MIB-II
*
* DESCRIPTION
*
*       This file contains declarations for each of the MIB-II objects.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef _1213_OID_H_
#define _1213_OID_H_

    { {1,3,6,1,2,1,1,3}, 8,  sysUpTime, SNMP_TIMETICKS, MIB_READ},

#if (RFC1213_SYS_INCLUDE == NU_TRUE)
/*------------------ System Group --------------------------------------*/
    { {1,3,6,1,2,1,1,1},       8,  sysDescr,    SNMP_DISPLAYSTR, MIB_READ},
    { {1,3,6,1,2,1,1,2},       8,  sysObjectID, SNMP_OBJECTID,   MIB_READ},
    { {1,3,6,1,2,1,1,4},       8,  sysContact,  SNMP_DISPLAYSTR, MIB_READ | MIB_WRITE},
    { {1,3,6,1,2,1,1,5},       8,  sysName,     SNMP_DISPLAYSTR, MIB_READ | MIB_WRITE},
    { {1,3,6,1,2,1,1,6},       8,  sysLocation, SNMP_DISPLAYSTR, MIB_READ | MIB_WRITE},
    { {1,3,6,1,2,1,1,7},       8,  sysServices, SNMP_INTEGER,    MIB_READ},
#endif

#if ( (RFC1213_IF_INCLUDE == NU_TRUE) && (MIB2_IF_INCLUDE == NU_TRUE) )
/*------------------ Interfaces Group ----------------------------------*/
    { {1,3,6,1,2,1,2,1},       8, IfNumber,    SNMP_INTEGER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,1},  10, If1213Entry, SNMP_INTEGER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,2},  10, If1213Entry, SNMP_DISPLAYSTR, MIB_READ},
    { {1,3,6,1,2,1,2,2,1,3},  10, If1213Entry, SNMP_INTEGER ,   MIB_READ},
    { {1,3,6,1,2,1,2,2,1,4},  10, If1213Entry, SNMP_INTEGER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,5},  10, If1213Entry, SNMP_GAUGE,      MIB_READ},
    { {1,3,6,1,2,1,2,2,1,6},  10, If1213Entry, SNMP_OCTETSTR,   MIB_READ},
    { {1,3,6,1,2,1,2,2,1,7},  10, If1213Entry, SNMP_INTEGER,    MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,2,2,1,8},  10, If1213Entry, SNMP_INTEGER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,9},  10, If1213Entry, SNMP_TIMETICKS,  MIB_READ},
    { {1,3,6,1,2,1,2,2,1,10}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,11}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,12}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,13}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,14}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,15}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,16}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,17}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,18}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,19}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,20}, 10, If1213Entry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,2,2,1,21}, 10, If1213Entry, SNMP_GAUGE,      MIB_READ},
    { {1,3,6,1,2,1,2,2,1,22}, 10, If1213Entry, SNMP_OBJECTID,   MIB_READ},

#if ((INCLUDE_IF_EXT == NU_TRUE) && (INCLUDE_IF_EXT_MIB == NU_TRUE))
    { {1,3,6,1,2,1,31,1,1,1,1} ,  11, IfXEntry, SNMP_DISPLAYSTR, MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,2} ,  11, IfXEntry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,3} ,  11, IfXEntry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,4} ,  11, IfXEntry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,5} ,  11, IfXEntry, SNMP_COUNTER,    MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,6} ,  11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,7} ,  11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,8} ,  11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,9} ,  11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,10} , 11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,11} , 11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,12} , 11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,13} , 11, IfXEntry, SNMP_COUNTER64,  MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,14} , 11, IfXEntry, SNMP_INTEGER,    MIB_READ | MIB_WRITE},
    { {1,3,6,1,2,1,31,1,1,1,15} , 11, IfXEntry, SNMP_GAUGE,      MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,16} , 11, IfXEntry, SNMP_INTEGER,    MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,17} , 11, IfXEntry, SNMP_INTEGER,    MIB_READ},
    { {1,3,6,1,2,1,31,1,1,1,18} , 11, IfXEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_WRITE},
    { {1,3,6,1,2,1,31,1,1,1,19} , 11, IfXEntry, SNMP_TIMETICKS,  MIB_READ},
#endif /* ((INCLUDE_IF_EXT == NU_TRUE) && \
           (INCLUDE_IF_EXT_MIB == NU_TRUE)) */

#if ((INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_IF_STACK_MIB == NU_TRUE))
    { {1,3,6,1,2,1,31,1,2,1,3}  , 11, IfStackEntry, SNMP_INTEGER, MIB_READ | MIB_CREATE},
#endif /* ((INCLUDE_IF_STACK == NU_TRUE) && \
           (INCLUDE_IF_STACK_MIB == NU_TRUE)) */

#if (INCLUDE_RCV_ADDR_MIB == NU_TRUE)
    { {1,3,6,1,2,1,31,1,4,1,2}  , 11, IfRcvAddressEntry, SNMP_INTEGER, MIB_READ | MIB_CREATE},
    { {1,3,6,1,2,1,31,1,4,1,3}  , 11, IfRcvAddressEntry, SNMP_INTEGER, MIB_READ | MIB_CREATE},
#endif /* (INCLUDE_RCV_ADDR_MIB == NU_TRUE) */

#if ((INCLUDE_IF_EXT == NU_TRUE) && (INCLUDE_IF_EXT_MIB == NU_TRUE))
    { {1,3,6,1,2,1,31,1,5}  , 9, IfTableLastChange, SNMP_TIMETICKS, MIB_READ},
#endif /* ((INCLUDE_IF_EXT == NU_TRUE) && \
           (INCLUDE_IF_EXT_MIB == NU_TRUE)) */

#if ((INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_IF_STACK_MIB == NU_TRUE))
    { {1,3,6,1,2,1,31,1,6}  , 9, IfStackLastChange, SNMP_TIMETICKS, MIB_READ},
#endif /* ((INCLUDE_IF_STACK == NU_TRUE) && \
           (INCLUDE_IF_STACK_MIB == NU_TRUE)) */

#endif

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)
#if ((RFC1213_IP_INCLUDE == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE))
#if (RFC1213_IP_INCLUDE == NU_TRUE)
/*------------------ IP Group ------------------------------------------*/
    { {1,3,6,1,2,1,4,1},        8,  ipForwarding,       SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,2},        8,  ipDefaultTTL,       SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,3},        8,  ipInReceives,       SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,4},        8,  ipInHdrErrors,      SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,5},        8,  ipInAddrErrors,     SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,6},        8,  ipForwDatagrams,    SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,7},        8,  ipInUnknownProtos,  SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,8},        8,  ipInDiscards,       SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,9},        8,  ipInDelivers,       SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,10},       8,  ipOutRequests,      SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,11},       8,  ipOutDiscards,      SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,12},       8,  ipOutNoRoutes,      SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,13},       8,  ipReasmTimeout,     SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,14},       8,  ipReasmReqds,       SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,15},       8,  ipReasmOKs,         SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,16},       8,  ipReasmFails,       SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,17},       8,  ipFragOKs,          SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,18},       8,  ipFragFails,        SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,19},       8,  ipFragCreates,      SNMP_COUNTER,  MIB_READ},
    { {1,3,6,1,2,1,4,20,1,1},  10,  ipAddrEntry,        SNMP_IPADDR,   MIB_READ},
    { {1,3,6,1,2,1,4,20,1,2},  10,  ipAddrEntry,        SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,20,1,3},  10,  ipAddrEntry,        SNMP_IPADDR,   MIB_READ},
    { {1,3,6,1,2,1,4,20,1,4},  10,  ipAddrEntry,        SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,20,1,5},  10,  ipAddrEntry,        SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,21,1,1},  10,  ipRouteEntry,       SNMP_IPADDR,   MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,2},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,3},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,4},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,21,1,5},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,21,1,6},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,21,1,7},  10,  ipRouteEntry,       SNMP_IPADDR,   MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,8},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,9},  10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,21,1,10}, 10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,11}, 10,  ipRouteEntry,       SNMP_IPADDR,   MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,21,1,12}, 10,  ipRouteEntry,       SNMP_INTEGER,  MIB_READ},
    { {1,3,6,1,2,1,4,21,1,13}, 10,  ipRouteEntry,       SNMP_OBJECTID, MIB_READ},
#endif
    { {1,3,6,1,2,1,4,22,1,1},  10, ipNetToMediaEntry,   SNMP_INTEGER,  MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,22,1,2},  10, ipNetToMediaEntry,   SNMP_OCTETSTR, MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,22,1,3},  10, ipNetToMediaEntry,   SNMP_IPADDR,   MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,4,22,1,4},  10, ipNetToMediaEntry,   SNMP_INTEGER,  MIB_READ|MIB_WRITE},

#if (RFC1213_IP_INCLUDE == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE)
    { {1,3,6,1,2,1,4,23},       8, ipRoutingDiscards,   SNMP_COUNTER,  MIB_READ},
#endif
#endif

#if ((RFC1213_ICMP_INCLUDE == NU_TRUE) && (MIB2_ICMP_INCLUDE == NU_TRUE))
/*------------------ ICMP Group ----------------------------------------*/
    { {1,3,6,1,2,1,5,1},         8, icmpInMsgs,           SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,2},         8, icmpInErrors,         SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,3},         8, icmpInDestUnreachs,   SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,4},         8, icmpInTimeExcds,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,5},         8, icmpInParmProbs,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,6},         8, icmpInSrcQuenchs,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,7},         8, icmpInRedirects,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,8},         8, icmpInEchos,          SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,9},         8, icmpInEchoReps,       SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,10},        8, icmpInTimestamps,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,11},        8, icmpInTimestampReps,  SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,12},        8, icmpInAddrMasks,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,13},        8, icmpInAddrMaskReps,   SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,14},        8, icmpOutMsgs,          SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,15},        8, icmpOutErrors,        SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,16},        8, icmpOutDestUnreachs,  SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,17},        8, icmpOutTimeExcds,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,18},        8, icmpOutParmProbs,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,19},        8, icmpOutSrcQuenchs,    SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,20},        8, icmpOutRedirects,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,21},        8, icmpOutEchos,         SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,22},        8, icmpOutEchoReps,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,23},        8, icmpOutTimestamps,    SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,24},        8, icmpOutTimestampReps, SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,25},        8, icmpOutAddrMasks,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,5,26},        8, icmpOutAddrMaskReps,  SNMP_COUNTER, MIB_READ},
#endif

#if ((RFC1213_TCP_INCLUDE == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE))
/*------------------ TCP Group -----------------------------------------*/
    { {1,3,6,1,2,1,6,1},         8, tcpRtoAlgorithm,     SNMP_INTEGER, MIB_READ},
    { {1,3,6,1,2,1,6,2},         8, tcpRtoMin,           SNMP_INTEGER, MIB_READ},
    { {1,3,6,1,2,1,6,3},         8, tcpRtoMax,           SNMP_INTEGER, MIB_READ},
    { {1,3,6,1,2,1,6,4},         8, tcpMaxConn,          SNMP_INTEGER, MIB_READ},
    { {1,3,6,1,2,1,6,5},         8, tcpActiveOpens,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,6},         8, tcpPassiveOpens,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,7},         8, tcpAttemptFails,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,8},         8, tcpEstabResets,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,9},         8, tcpCurrEstab,        SNMP_GAUGE,   MIB_READ},
    { {1,3,6,1,2,1,6,10},        8, tcpInSegs,           SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,11},        8, tcpOutSegs,          SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,12},        8, tcpRetransSegs,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,13,1,1},   10, tcpConnEntry,        SNMP_INTEGER, MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,6,13,1,2},   10, tcpConnEntry,        SNMP_IPADDR,  MIB_READ},
    { {1,3,6,1,2,1,6,13,1,3},   10, tcpConnEntry,        SNMP_INTEGER, MIB_READ},
    { {1,3,6,1,2,1,6,13,1,4},   10, tcpConnEntry,        SNMP_IPADDR,  MIB_READ},
    { {1,3,6,1,2,1,6,13,1,5},   10, tcpConnEntry,        SNMP_INTEGER, MIB_READ},
    { {1,3,6,1,2,1,6,14},        8, tcpInErrs,           SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,6,15},        8, tcpOutRsts,          SNMP_COUNTER, MIB_READ},
#endif

#if ((RFC1213_UDP_INCLUDE == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE))
/*------------------ UDP Group -----------------------------------------*/
    { {1,3,6,1,2,1,7,1},         8, udpInDatagrams,      SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,7,2},         8, udpNoPorts,          SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,7,3},         8, udpInErrors,         SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,7,4},         8, udpOutDatagrams,     SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,7,5,1,1},    10, udpEntry,            SNMP_IPADDR,    MIB_READ},
    { {1,3,6,1,2,1,7,5,1,2},    10, udpEntry,            SNMP_INTEGER,   MIB_READ},
#endif

#if RFC1213_EGP_INCLUDE == NU_TRUE
/*------------------ EGP Group -----------------------------------------*/
    { {1,3,6,1,2,1,8,1},         8, egpInMsgs,      SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,2},         8, egpInErrors,    SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,3},         8, egpOutMsgs,     SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,4},         8, egpOutErrors,   SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,1},    10, egpNeighEntry,  SNMP_INTEGER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,2},    10, egpNeighEntry,  SNMP_IPADDR,    MIB_READ},
    { {1,3,6,1,2,1,8,5,1,3},    10, egpNeighEntry,  SNMP_INTEGER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,4},    10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,5},    10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,6},    10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,7},    10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,8},    10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,9},    10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,10},   10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,11},   10, egpNeighEntry,  SNMP_COUNTER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,12},   10, egpNeighEntry,  SNMP_INTEGER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,13},   10, egpNeighEntry,  SNMP_INTEGER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,14},   10, egpNeighEntry,  SNMP_INTEGER,   MIB_READ},
    { {1,3,6,1,2,1,8,5,1,15},   10, egpNeighEntry,  SNMP_INTEGER,   MIB_READ|MIB_WRITE},
    { {1,3,6,1,2,1,8,6},         8, egpAs,          SNMP_INTEGER,   MIB_READ},
#endif

#if RFC1213_SNMP_INCLUDE == NU_TRUE
/*------------------ SNMP Group ----------------------------------------*/
    { {1,3,6,1,2,1,11,1},       8, snmpInPkts,              SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,2},       8, snmpOutPkts,             SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,3},       8, snmpInBadVersions,       SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,4},       8, snmpInBadCommunityNames, SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,5},       8, snmpInBadCommunityUses,  SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,6},       8, snmpInASNParseErrs,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,8},       8, snmpInTooBigs,           SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,9},       8, snmpInNoSuchNames,       SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,10},      8, snmpInBadValues,         SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,11},      8, snmpInReadOnlys,         SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,12},      8, snmpInGenErrs,           SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,13},      8, snmpInTotalReqVars,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,14},      8, snmpInTotalSetVars,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,15},      8, snmpInGetRequests,       SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,16},      8, snmpInGetNexts,          SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,17},      8, snmpInSetRequests,       SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,18},      8, snmpInGetResponses,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,19},      8, snmpInTraps,             SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,20},      8, snmpOutTooBigs,          SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,21},      8, snmpOutNoSuchNames,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,22},      8, snmpOutBadValues,        SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,24},      8, snmpOutGenErrs,          SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,25},      8, snmpOutGetRequests,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,26},      8, snmpOutGetNexts,         SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,27},      8, snmpOutSetRequests,      SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,28},      8, snmpOutGetResponses,     SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,29},      8, snmpOutTraps,            SNMP_COUNTER, MIB_READ},
    { {1,3,6,1,2,1,11,30},      8, snmpEnableAuthenTraps,   SNMP_INTEGER, MIB_READ|MIB_WRITE},
#endif
#endif
#endif

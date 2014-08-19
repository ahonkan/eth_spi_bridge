/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*        ip6_mib_oid_s.h                       	     
*
*   DESCRIPTION
*
*        This file provides the OID definitions for IPv6 MIBs.
*
*   DATA STRUCTURES
*
*        [global component data structures defined in this file]       
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_OID_S_H
#define IP6_MIB_OID_S_H

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE) )

/************************************************************************/
/*                          ipv6MIBObjects                              */
/************************************************************************/
{ {1,3,6,1,2,1,55,1,1}, 9, ipv6Forwarding,        SNMP_INTEGER,   MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,55,1,2}, 9, ipv6DefaultHopLimit,   SNMP_INTEGER,   MIB_READ | MIB_WRITE},

/*----------------------------------------------------------------------
 * ipv6IfTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)
{ {1,3,6,1,2,1,55,1,3}, 9, ipv6Interfaces,        SNMP_GAUGE,     MIB_READ},
{ {1,3,6,1,2,1,55,1,4}, 9, ipv6IfTableLastChange, SNMP_TIMETICKS, MIB_READ},

{ {1,3,6,1,2,1,55,1,5,1,2},  11, ipv6IfEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,55,1,5,1,3},  11, ipv6IfEntry, SNMP_OBJECTID,   MIB_READ},
{ {1,3,6,1,2,1,55,1,5,1,4},  11, ipv6IfEntry, SNMP_GAUGE,      MIB_READ},
{ {1,3,6,1,2,1,55,1,5,1,5},  11, ipv6IfEntry, SNMP_GAUGE,      MIB_READ},
{ {1,3,6,1,2,1,55,1,5,1,6},  11, ipv6IfEntry, SNMP_OCTETSTR,   MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,55,1,5,1,7},  11, ipv6IfEntry, SNMP_INTEGER,    MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,55,1,5,1,8},  11, ipv6IfEntry, SNMP_OCTETSTR,   MIB_READ},
{ {1,3,6,1,2,1,55,1,5,1,9},  11, ipv6IfEntry, SNMP_INTEGER,    MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,55,1,5,1,10}, 11, ipv6IfEntry, SNMP_INTEGER,    MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,55,1,5,1,11}, 11, ipv6IfEntry, SNMP_TIMETICKS,  MIB_READ | MIB_WRITE},

/*----------------------------------------------------------------------
 * ipv6IfStatsTable
 *---------------------------------------------------------------------*/
{ {1,3,6,1,2,1,55,1,6,1,1},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,2},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,3},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,4},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,5},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,6},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,7},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,8},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,9},  11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,10}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,11}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,12}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,13}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,14}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,15}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,16}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,17}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,18}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,19}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,6,1,20}, 11, ipv6IfStatsEntry, SNMP_COUNTER,  MIB_READ},

#endif /* (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) */
/*----------------------------------------------------------------------
 * ipv6AddrPrefixTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE)
{ {1,3,6,1,2,1,55,1,7,1,3}, 11, ipv6AddrPrefixEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,55,1,7,1,4}, 11, ipv6AddrPrefixEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,55,1,7,1,5}, 11, ipv6AddrPrefixEntry, SNMP_GAUGE,   MIB_READ},
{ {1,3,6,1,2,1,55,1,7,1,6}, 11, ipv6AddrPrefixEntry, SNMP_GAUGE,   MIB_READ},
#endif /* (INCLUDE_IP6_MIB_ADDR_PREFIX == NU_TRUE) */

/*----------------------------------------------------------------------
 * ipv6AddrTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE)
{ {1,3,6,1,2,1,55,1,8,1,2}, 11, ipv6AddrEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,55,1,8,1,3}, 11, ipv6AddrEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,55,1,8,1,4}, 11, ipv6AddrEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,55,1,8,1,5}, 11, ipv6AddrEntry, SNMP_INTEGER, MIB_READ},
#endif /* (INCLUDE_IPV6_IF_ADDR_MIB == NU_TRUE) */

/*----------------------------------------------------------------------
 * ipv6RouteTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IPV6_RT == NU_TRUE)
{ {1,3,6,1,2,1,55,1,9},       9,  ipv6RouteNumber,     SNMP_GAUGE,    MIB_READ},
{ {1,3,6,1,2,1,55,1,10},      9,  ipv6DiscardedRoutes, SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,4},  11, ipv6RouteEntry,      SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,5},  11, ipv6RouteEntry,      SNMP_OCTETSTR, MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,6},  11, ipv6RouteEntry,      SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,7},  11, ipv6RouteEntry,      SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,8},  11, ipv6RouteEntry,      SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,9},  11, ipv6RouteEntry,      SNMP_GAUGE,    MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,10}, 11, ipv6RouteEntry,      SNMP_GAUGE,    MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,11}, 11, ipv6RouteEntry,      SNMP_GAUGE,    MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,12}, 11, ipv6RouteEntry,      SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,13}, 11, ipv6RouteEntry,      SNMP_OBJECTID, MIB_READ},
{ {1,3,6,1,2,1,55,1,11,1,14}, 11, ipv6RouteEntry,      SNMP_INTEGER,  MIB_READ | MIB_WRITE},
#endif /*(INCLUDE_IPV6_RT == NU_TRUE) */
/*----------------------------------------------------------------------
 * ipv6NetToMediaTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IPV6_MIB_NTM == NU_TRUE)
{ {1,3,6,1,2,1,55,1,12,1,2},  11, ipv6NetToMediaEntry,      SNMP_OCTETSTR,  MIB_READ},
{ {1,3,6,1,2,1,55,1,12,1,3},  11, ipv6NetToMediaEntry,      SNMP_INTEGER,   MIB_READ},
{ {1,3,6,1,2,1,55,1,12,1,4},  11, ipv6NetToMediaEntry,      SNMP_INTEGER,   MIB_READ},
{ {1,3,6,1,2,1,55,1,12,1,5},  11, ipv6NetToMediaEntry,      SNMP_TIMETICKS, MIB_READ},
{ {1,3,6,1,2,1,55,1,12,1,6},  11, ipv6NetToMediaEntry,      SNMP_INTEGER,   MIB_READ},
#endif /* (INCLUDE_IPV6_MIB_NTM == NU_TRUE) */

/*----------------------------------------------------------------------
 * ipv6IfIcmpTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)
{ {1,3,6,1,2,1,56,1,1,1,1},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,2},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,3},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,4},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,5},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,6},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,7},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,8},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,9},   11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,10},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,11},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,12},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,13},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,14},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,15},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,16},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,17},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,18},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,19},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,20},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,21},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,22},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,23},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,24},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,25},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,26},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,27},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,28},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,29},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,30},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,31},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,32},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,33},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},
{ {1,3,6,1,2,1,56,1,1,1,34},  11, ipv6IfIcmpEntry,     SNMP_COUNTER,  MIB_READ},

#endif /* (INCLUDE_IPV6_ICMP_MIB == NU_TRUE) */

/*----------------------------------------------------------------------
 * ipv6TcpConnTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IPV6_TCP_MIB == NU_TRUE)
{ {1,3,6,1,2,1,6,16,1,6},  10, ipv6TcpConnEntry,       SNMP_INTEGER,  MIB_READ | MIB_WRITE},
#endif

/*----------------------------------------------------------------------
 * ipv6UdpTable
 *---------------------------------------------------------------------*/
#if (INCLUDE_IPV6_UDP_MIB == NU_TRUE)
{ {1,3,6,1,2,1,7,6,1,3},   10, ipv6UdpEntry,            SNMP_INTEGER,  MIB_READ},
#endif

/*----------------------------------------------------------------------
 * mldMIBObjects
 *---------------------------------------------------------------------*/
#if ((INCLUDE_IPV6_MLD_MIB == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE))
{ {1,3,6,1,2,1,91,1,1,1,3}, 11, mldInterfaceEntry,     SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,91,1,1,1,5}, 11, mldInterfaceEntry,     SNMP_OCTETSTR, MIB_READ},
{ {1,3,6,1,2,1,91,1,2,1,3}, 11, mldCacheEntry,         SNMP_INTEGER,  MIB_READ},
{ {1,3,6,1,2,1,91,1,2,1,7}, 11, mldCacheEntry,         SNMP_INTEGER,  MIB_READ},
#endif /* ((INCLUDE_IPV6_MLD_MIB == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE)) */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE)) */

#endif /* IP6_MIB_OID_S_H */

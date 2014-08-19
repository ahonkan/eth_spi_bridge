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
*       1213xxxx.h                                               
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the data structure declarations for each of
*       the MIB-II objects and stubbed out functions for those MIB-II
*       objects not being included in the build.
*
*   DATA STRUCTURES
*
*       rfc1213_sys_s
*       egpneightab_s
*       rfc1213_egp_s
*       rfc1213_trans_s
*       rfc1213_vars_s
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       snmp_prt.h
*       snmp.h
*
************************************************************************/

#ifndef _1213XXXX_H_
#define _1213XXXX_H_

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/snmp_prt.h"
#include "networking/snmp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define RFC1213_IP_INCLUDE          NU_TRUE
#define RFC1213_IF_INCLUDE          NU_TRUE
#define RFC1213_TCP_INCLUDE         NU_TRUE
#define RFC1213_UDP_INCLUDE         NU_TRUE
#define RFC1213_SYS_INCLUDE         NU_TRUE
#define RFC1213_EGP_INCLUDE         NU_TRUE
#define RFC1213_ICMP_INCLUDE        NU_TRUE
#define RFC1213_SNMP_INCLUDE        NU_TRUE

#define INCLUDE_IF_STACK_MIB        NU_TRUE
#define INCLUDE_IF_EXT_MIB          NU_TRUE
#define INCLUDE_RCV_ADDR_MIB        NU_TRUE

#if (RFC1213_IP_INCLUDE   == NU_FALSE  && \
     RFC1213_TCP_INCLUDE  == NU_FALSE  && \
     RFC1213_UDP_INCLUDE  == NU_FALSE  && \
     RFC1213_EGP_INCLUDE  == NU_FALSE  && \
     RFC1213_ICMP_INCLUDE == NU_FALSE  && \
     RFC1213_SNMP_INCLUDE == NU_FALSE)

#define INCLUDE_SNMPv1_FULL_MIBII NU_FALSE
#else
#define INCLUDE_SNMPv1_FULL_MIBII NU_TRUE
#endif

/*
 * Miscellaneous defines.  These defines should be migrated to
 * a "config structure" so that a particular environment can
 * be set for the stack/MIB/SNMP
 * For now, these defines are sufficient.
 */

/* The definitions have been moved to mib2.h */
#define MAX_1213_STRSIZE        MIB2_MAX_STRSIZE
#define MAX_1213_BUFINT         MIB2_MAX_BUFINT
#define MAX_1213_OIDSIZE        MIB2_MAX_OIDSIZE
#define MAX_1213_IF             MIB2_MAX_IF
#define MAX_1213_PADDRSIZE      MIB2_MAX_PADDRSIZE
#define MAX_1213_NETADDRSIZE    MIB2_MAX_NETADDRSIZE
#define MAX_1213_UDPLISTEN      MIB2_MAX_UDPLISTEN

/*
 * The system group
 */
typedef struct rfc1213_sys_s
{
    INT8                    sysDescr[MAX_1213_STRSIZE];
    INT32                   sysObjectID[MAX_1213_BUFINT];
    UINT16                  sysObjectIDLen;
    UINT16                  pad;
    INT8                    sysContact[MAX_1213_STRSIZE];
    INT8                    sysLocation[MAX_1213_STRSIZE];
    INT32                   sysServices;
    INT32                   sysUpTime;
} rfc1213_sys_t;


#if (RFC1213_EGP_INCLUDE == NU_TRUE)
typedef struct egpneightab_s
{
    struct egpneightab_s    *next;
    struct egpneightab_s    *last;
    UINT32                  egpNeighState;
    UINT8                   egpNeighAddr[MAX_1213_NETADDRSIZE];
    UINT32                  egpNeighAs;
    UINT32                  egpNeighInMsgs;
    UINT32                  egpNeighInErrs;
    UINT32                  egpNeighOutMsgs;
    UINT32                  egpNeighOutErrs;
    UINT32                  egpNeighInErrMsgs;
    UINT32                  egpNeighOutErrMsgs;
    UINT32                  egpNeighStateUps;
    UINT32                  egpNeighStateDowns;
    UINT32                  egpNeighIntervalHello;
    UINT32                  egpNeighIntervalPoll;
    UINT32                  egpNeighMode;
    UINT32                  egpNeighEventTrigger;
} egpneightab_t;

/*
 * The EGP Group
 * Only if you have Exterior Gateway Protocol
 */
typedef struct rfc1213_egp_s
{
    UINT32                  egpInMsgs;
    UINT32                  egpInErrors;
    UINT32                  egpOutMsgs;
    UINT32                  egpOutErrors;
    egpneightab_t           *egpNeighTab;
    UINT32                  egpAs;
} rfc1213_egp_t;
#endif

/*
 * The Transmission Group
 * This info is media specific
 */
typedef struct rfc1213_trans_s
{
    INT32                   transNumber[MAX_1213_OIDSIZE];
    UINT16                  transLen;

#if (PAD_2)
    UINT8                   snmp_pad[PAD_2];
#endif
} rfc1213_trans_t;

/*
 * MIBII Group/Object Container
 */
typedef struct rfc1213_vars_s
{
    rfc1213_sys_t           rfc1213_sys;

#if ( (defined(INCLUDE_SNMPv1_FULL_MIBII)==NU_FALSE) || \
      (INCLUDE_SNMPv1_FULL_MIBII==NU_TRUE) )

#if RFC1213_EGP_INCLUDE == NU_TRUE
    rfc1213_egp_t           rfc1213_egp;
#endif
#endif

    rfc1213_trans_t         rfc1213_trans;
} rfc1213_vars_t;

#if RFC1213_EGP_INCLUDE == NU_FALSE
#define egpInMsgs
#define egpInErrors
#define egpOutMsgs
#define egpOutErrors
#define egpNeighState
#define egpNeighAddr
#define egpNeighAs
#define egpNeighInMsgs
#define egpNeighInErrs
#define egpNeighOutMsgs
#define egpNeighOutErrs
#define egpNeighInErrMsgs
#define egpNeighOutErrMsgs
#define egpNeighStateUps
#define egpNeighStateDowns
#define egpNeighIntervalHello
#define egpNeighIntervalPoll
#define egpNeighMode
#define egpNeighEventTrigger
#define egpAs
#endif

#if RFC1213_SNMP_INCLUDE == NU_FALSE
#define snmpInPkts
#define snmpOutPkts
#define snmpInBadVersions
#define snmpInBadCommunityNames
#define snmpInBadCommunityUses
#define snmpInASNParseErrs
#define snmpInTooBigs
#define snmpInNoSuchNames
#define snmpInBadValues
#define snmpInReadOnlys
#define snmpInGenErrs
#define snmpInTotalReqVars
#define snmpInTotalSetVars
#define snmpInGetRequests
#define snmpInGetNexts
#define snmpInSetRequests
#define snmpInGetResponses
#define snmpInTraps
#define snmpOutTooBigs
#define snmpOutNoSuchNames
#define snmpOutBadValues
#define snmpOutGenErrs
#define snmpOutGetRequests
#define snmpOutGetNexts
#define snmpOutSetRequests
#define snmpOutGetResponses
#define snmpOutTraps
#define snmpEnableAuthenTraps
#endif

#if (INCLUDE_MIB_RMON1 == NU_FALSE)
#define sysTime
#endif

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

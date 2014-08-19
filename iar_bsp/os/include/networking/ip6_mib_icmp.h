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
*        ip6_mib_icmp.h                              
*
*   DESCRIPTION
*
*        This file contains the functions declarations and macro
*        definitions to provide an interface to SNMP MIBs.
*
*   DATA STRUCTURES
*
*        IF_ICMP_STAT
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_ICMP_H
#define IP6_MIB_ICMP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

#if (INCLUDE_IPV6_ICMP_MIB == NU_TRUE)

/* Total number of ICMPv6 statistics. */
#define IP6_ICMP_MIB_COUNTERS           34

/* The total number of ICMP messages received by the interface which
 * includes all those counted by ipv6IfIcmpInErrors. Note that this
 * interface is the interface to which the ICMP messages were
 * addressed which may not be necessarily the input interface for the
 * messages.
 */
#define IPV6_IF_ICMP_IN_MSGS            0

/* The number of ICMP messages which the interface received but
 * determined as having ICMP-specific errors (bad ICMP checksums, bad
 * length, etc.).
 */
#define IPV6_IF_ICMP_IN_ERRORS          1

/* The number of ICMP Destination Unreachable messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_DEST_UNREACHS   2

/* The number of ICMP destination unreachable/communication
 * administratively prohibited messages received by the interface.
 */
#define IPV6_IF_ICMP_IN_ADMIN_PROHIBS   3

/* The number of ICMP Time Exceeded messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_TIME_EXCDS      4

/* The number of ICMP Parameter Problem messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_PARM_PROBLEMS   5

/* The number of ICMP Packet Too Big messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_PKT_TOO_BIGS    6

/* The number of ICMP Echo (request) messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_ECHOS           7

/* The number of ICMP Echo Reply messages received by the interface.
 */
#define IPV6_IF_ICMP_IN_ECHO_REPLIES    8

/* The number of ICMP Router Solicit messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_ROUTER_SOL      9

/* The number of ICMP Router Advertisement messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_ROUTER_ADV      10

/* The number of ICMP Neighbor Solicit messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_NEIGHBOR_SOL    11

/* The number of ICMP Neighbor Advertisement messages received by the
 * interface.
 */
#define IPV6_IF_ICMP_IN_NEIGHBOR_ADV    12

/* The number of Redirect messages received by the interface. */
#define IPV6_IF_ICMP_IN_REDIRECTS       13

/* The number of ICMPv6 Group Membership Query messages received by
 * the interface.
 */
#define IPV6_IF_ICMP_IN_GRP_MEMB_QUER   14

/* The number of ICMPv6 Group Membership Response messages received by
 * the interface.
 */
#define IPV6_IF_ICMP_IN_GRP_MEMB_RESP   15

/* The number of ICMPv6 Group Membership Reduction messages received
 * by the interface.
 */
#define IPV6_IF_ICMP_IN_GRP_MEMB_RED    16

/* The total number of ICMP messages which this interface attempted to
 * send.  Note that this counter includes all those counted by
 * icmpOutErrors.
 */
#define IPV6_IF_ICMP_OUT_MSGS           17

/* The number of ICMP messages which this interface did not send due
 * to problems discovered within ICMP such as a lack of buffers. This
 * value should not include errors discovered outside the ICMP layer
 * such as the inability of IPv6 to route the resultant datagram.In
 * some implementations there may be no types of error which
 * contribute to this counter's value.
 */
#define IPV6_IF_ICMP_OUT_ERRORS         18

/* The number of ICMP Destination Unreachable messages sent by the
 * interface.
 */
#define IPV6_IF_ICMP_OUT_DEST_UNREACHS  19

/* Number of ICMP dest unreachable/communication administratively
 * prohibited messages sent.
 */
#define IPV6_IF_ICMP_OUT_ADMIN_PROHIBS  20

/* The number of ICMP Time Exceeded messages sent by the interface. */
#define IPV6_IF_ICMP_OUT_TIME_EXCDS     21

/* The number of ICMP Parameter Problem messages sent by the
 * interface.
 */
#define IPV6_IF_ICMP_OUT_PARM_PROB      22

/* The number of ICMP Packet Too Big messages sent by the interface. */
#define IPV6_IF_ICMP_OUT_PKT_TOO_BIGS   23

/* The number of ICMP Echo (request) messages sent by the interface. */
#define IPV6_IF_ICMP_OUT_ECHOS          24

/* The number of ICMP Echo Reply messages sent by the interface. */
#define IPV6_IF_ICMP_OUT_ECHO_REPLIES   25

/* The number of ICMP Router Solicitation messages sent by the interface.
 */
#define IPV6_IF_ICMP_OUT_ROUTER_SOL     26

/* The number of ICMP Router Advertisement messages sent by the interface.
 */
#define IPV6_IF_ICMP_OUT_ROUTER_ADV     27

/* The number of ICMP Neighbor Solicitation messages sent by the 
 * interface.
 */
#define IPV6_IF_ICMP_OUT_NEIGHBOR_SOL   28

/* The number of ICMP Neighbor Advertisement messages sent by the
 * interface.
 */
#define IPV6_IF_ICMP_OUT_NEIGHBOR_ADV   29

/* The number of Redirect messages sent. For a host, this object will
 * always be zero, since hosts do not send redirects.
 */
#define IPV6_IF_ICMP_OUT_REDIRECTS      30

/* The number of ICMPv6 Group Membership Query messages sent. */
#define IPV6_IF_ICMP_OUT_GRP_MEMB_QUER  31

/* The number of ICMPv6 Group Membership Response messages sent. */
#define IPV6_IF_ICMP_OUT_GRP_MEMB_RES   32

/* The number of ICMPv6 Group Membership Reduction messages sent. */
#define IPV6_IF_ICMP_OUT_GRP_MEMB_RED   33

#define MIB_ipv6IfIcmpInMsgs_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_MSGS])++

#define MIB_ipv6IfIcmpInErrors_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_ERRORS])++

#define MIB_ipv6IfIcmpInDestUreach_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_DEST_UNREACHS])++

#define MIB_ipv6IfIcmpInAdminProh_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_ADMIN_PROHIBS])++

#define MIB_ipv6IfIcmpInTimeExcds_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_TIME_EXCDS])++

#define MIB_ipv6IfIcmpInParmProb_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_PARM_PROBLEMS])++

#define MIB_ipv6IfIcmpInPktTooBig_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_PKT_TOO_BIGS])++

#define MIB_ipv6IfIcmpInEchos_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_ECHOS])++

#define MIB_ipv6IfIcmpInEchoReply_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_ECHO_REPLIES])++

#define MIB_ipv6IfIcmpInRtSolic_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_ROUTER_SOL])++

#define MIB_ipv6IfIcmpInRtAdv_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_ROUTER_ADV])++

#define MIB_ipv6IfIcmpInNeigSolic_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_NEIGHBOR_SOL])++

#define MIB_ipv6IfIcmpInNeigAdv_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_NEIGHBOR_ADV])++

#define MIB_ipv6IfIcmpInRedirects_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_REDIRECTS])++

#define MIB_ipv6IfIcmpInGrpMemQur_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_GRP_MEMB_QUER])++

#define MIB_ipv6IfIcmpInGrpMemRes_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_GRP_MEMB_RESP])++

#define MIB_ipv6IfIcmpInGrpMemRed_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_IN_GRP_MEMB_RED])++

#define MIB_ipv6IfIcmpOutMsgs_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_MSGS])++

#define MIB_ipv6IfIcmpOutErrors_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_ERRORS])++

#define MIB_ipv6IfIcmpOutDstUreach_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_DEST_UNREACHS])++

#define MIB_ipv6IfIcmpOutAdminProh_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_ADMIN_PROHIBS])++

#define MIB_ipv6IfIcmpOutTimeExcds_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_TIME_EXCDS])++

#define MIB_ipv6IfIcmpOutParmProb_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_PARM_PROB])++

#define MIB_ipv6IfIcmpOutPktTooBig_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_PKT_TOO_BIGS])++

#define MIB_ipv6IfIcmpOutEchos_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_ECHOS])++

#define MIB_ipv6IfIcmpOutEchoRep_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_ECHO_REPLIES])++

#define MIB_ipv6IfIcmpOutRtSolic_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_ROUTER_SOL])++

#define MIB_ipv6IfIcmpOutRtAdv_Inc(device)  \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_ROUTER_ADV])++

#define MIB_ipv6IfIcmpOutNeigSolic_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_NEIGHBOR_SOL])++

#define MIB_ipv6IfIcmpOutNeigAdv_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_NEIGHBOR_ADV])++

#define MIB_ipv6IfIcmpOutRedirects_Inc(device)   \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_REDIRECTS])++

#define MIB_ipv6IfIcmpOutGrpMemQur_Inc(device)    \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_GRP_MEMB_QUER])++

#define MIB_ipv6IfIcmpOutGrpMemRes_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_GRP_MEMB_RES])++

#define MIB_ipv6IfIcmpOutGrpMemRed_Inc(device) \
    ((device)->ip6_icmp_mib[IPV6_IF_ICMP_OUT_GRP_MEMB_RED])++

UINT16 IP6_MIB_ICMP_Get_Stat(UINT32 if_index, UINT32 option, UINT32 *value);

#define IP6_MIB_ICMP_STAT_GET(if_index, option, value, status)  \
    (status) = IP6_MIB_ICMP_Get_Stat((if_index), (option), &(value))

#define IP6_ICMP_MIB_GET_NEXT(if_index, status) \
    (status) = IP6_IF_MIB_Get_Next_Index(&(if_index))

#else /* (INCLUDE_IPV6_ICMP_MIB == NU_TRUE) */

#define MIB_ipv6IfIcmpInMsgs_Inc(device)
#define MIB_ipv6IfIcmpInErrors_Inc(device)
#define MIB_ipv6IfIcmpInDestUreach_Inc(device)
#define MIB_ipv6IfIcmpInAdminProh_Inc(device)
#define MIB_ipv6IfIcmpInTimeExcds_Inc(device)
#define MIB_ipv6IfIcmpInParmProb_Inc(device)
#define MIB_ipv6IfIcmpInPktTooBig_Inc(device)
#define MIB_ipv6IfIcmpInEchos_Inc(device)
#define MIB_ipv6IfIcmpInEchoReply_Inc(device)
#define MIB_ipv6IfIcmpInRtSolic_Inc(device)
#define MIB_ipv6IfIcmpInRtAdv_Inc(device)
#define MIB_ipv6IfIcmpInNeigSolic_Inc(device)
#define MIB_ipv6IfIcmpInNeigAdv_Inc(device)
#define MIB_ipv6IfIcmpInRedirects_Inc(device)
#define MIB_ipv6IfIcmpInGrpMemQur_Inc(device)
#define MIB_ipv6IfIcmpInGrpMemRes_Inc(device)
#define MIB_ipv6IfIcmpInGrpMemRed_Inc(device)
#define MIB_ipv6IfIcmpOutMsgs_Inc(device)
#define MIB_ipv6IfIcmpOutErrors_Inc(device)
#define MIB_ipv6IfIcmpOutDstUreach_Inc(device)
#define MIB_ipv6IfIcmpOutAdminProh_Inc(device)
#define MIB_ipv6IfIcmpOutTimeExcds_Inc(device)
#define MIB_ipv6IfIcmpOutParmProb_Inc(device)
#define MIB_ipv6IfIcmpOutPktTooBig_Inc(device)
#define MIB_ipv6IfIcmpOutEchos_Inc(device)
#define MIB_ipv6IfIcmpOutEchoRep_Inc(device)
#define MIB_ipv6IfIcmpOutRtSolic_Inc(device)
#define MIB_ipv6IfIcmpOutRtAdv_Inc(device)
#define MIB_ipv6IfIcmpOutNeigSolic_Inc(device)
#define MIB_ipv6IfIcmpOutNeigAdv_Inc(device)
#define MIB_ipv6IfIcmpOutRedirects_Inc(device)
#define MIB_ipv6IfIcmpOutGrpMemQur_Inc(device)
#define MIB_ipv6IfIcmpOutGrpMemRes_Inc(device)
#define MIB_ipv6IfIcmpOutGrpMemRed_Inc(device)

#endif /* INCLUDE_IPV6_ICMP_MIB */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_ICMP_H */

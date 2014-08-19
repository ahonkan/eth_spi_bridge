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
*       net_bkcp.h
*
*   DESCRIPTION
*
*       This file holds all backward compatibility macros for NET.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       rtab4.h
*       rtab.h
*       dns.h
*       nlog.h
*
*************************************************************************/

#ifndef NET_BKCP_H
#define NET_BKCP_H

#include "networking/net_cfg.h"
#include "networking/rtab4.h"
#include "networking/rtab.h"
#include "networking/dns.h"
#include "networking/nlog.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/************************** DNS *****************************************
 *
 */

#define NU_Add_DNS_Server(new_dns_server, where)   NU_Add_DNS_Server2(new_dns_server, where, NU_FAMILY_IP)
#define NU_Delete_DNS_Server(dns_ip)               NU_Delete_DNS_Server2(dns_ip, NU_FAMILY_IP)
#define NU_Get_DNS_Servers(dest, size)             NU_Get_DNS_Servers2(dest, size, NU_FAMILY_IP)

/************************** LOGGING *************************************
 *
 */

/* This macro is included for backwards compatibility */
#define NERRS_Log               NERRS_Log_Error
#define NERRS_Log_Error(a,b,c)  NLOG_Error_Log_IS("No error msg",a,b,c)
#define PRINT_ERROR_MSG         INCLUDE_NET_ERROR_LOGGING

/************************** ROUTING *************************************
 *
 */

/* Maintain backward compatibility with pre-NET 5.1 releases */
#define NU_Delete_Route(ip_dest)  NU_Delete_Route2(ip_dest, NU_NULL, NU_FAMILY_IP)

#define     RTAB_Add_Route          RTAB4_Add_Route
#define     RTAB_Delete_Route(rt)   RTAB4_Delete_Route(rt, NU_NULL)
#define     RTAB_Find_Route(de)     RTAB4_Find_Route(de, 0)

/************************** SOCKETS *************************************
 *
 */

/* This definition was added to maintain compatibility with older versions
 * of Nucleus NET.
 */
#define NU_fcntl(p1, p2, p3, p4)      NU_Fcntl(p1, p2, p3)

#define NU_Get_Host_by_NAME   NU_Get_Host_By_Name

/* These two commands were replaced with SIOCGIFADDR and SIOCGIFDSTADDR but
 * were left in the code for backwards compatibility
 */
#define IOCTL_GETIFADDR     SIOCGIFADDR
#define IOCTL_GETIFDSTADDR  SIOCGIFDSTADDR

/************************** TFTP ****************************************
 *
 */

#define NU_TFTPC_Get(remote_ip, rpath, lpath, ops) \
    TFTPC_Get2(remote_ip, rpath, lpath, ops, NU_FAMILY_IP)

#define NU_TFTPC_Put(remote_ip, rpath, lpath, ops) \
    TFTPC_Put2(remote_ip, rpath, lpath, ops, NU_FAMILY_IP)

/************************** UTILITIES ***********************************
 *
 */

#define UTL_Timerset(event, data, howlong, extra)   \
    TQ_Timerset(event, data, howlong, (UNSIGNED)extra)

#define UTL_Timerunset(event, dat, extra)             \
    TQ_Timerunset(event, TQ_CLEAR_ALL, dat, (UNSIGNED)extra)

#define UTL_Check_Duetime   TQ_Check_Duetime

#define TLS_Put_Event(event,dat)    EQ_Put_Event((TQ_EVENT)event, dat, 0)

/* The function UTL_Get_Unique_Port_Number was renamed to PRT_Get_Unique_Port_Number
 * and modified to take a protocol and family type.  FTP has been the only
 * other NET product found to be using UTL_Get_Unique_Port_Number so it has
 * been defined to pass in a protocol type of TCP and family of IPv4.
 */
#define UTL_Get_Unique_Port_Number()  PRT_Get_Unique_Port_Number(NU_PROTO_TCP, NU_FAMILY_IP)

/************************** ETHERNET *************************************
 *
 */

#define NET_Ether_Input     ETH_Ether_Input
#define NET_Ether_Send      ETH_Ether_Send
#define NET_Add_Multi       ETH_Add_Multi
#define NET_Del_Multi       ETH_Del_Multi

/************************** PPP ******************************************
 *
 */

/* Backward compatibility constant, in case an old version of
 * PPP is used with this version of Net.
 */
#define dev_ppp_layer   dev_link_layer

/************************* Interfaces ************************************
 *
 */

#if (INCLUDE_IPV6 == NU_TRUE)

#define PREFIX6_Add_Prefix_Entry(device, prefix, prefix_length, valid_lifetime) \
        PREFIX6_New_Prefix_Entry(device, prefix, prefix_length, valid_lifetime, valid_lifetime, 0)

#endif

/* These macros weren't used as of NET 5.2 and PPP 3.2, but they are left
 * for backward compatibility.
 */
#define NET_PPP_HEADER_OFFSET_SIZE  NET_MAX_MAC_HEADER_SIZE
#define NET_SLIP_HEADER_OFFSET_SIZE NET_MAX_MAC_HEADER_SIZE

/*********************** User Management *********************************
 *
 */

#define UM_OVERWRITE    UM_UPDATE_MODE

/*********************** System Memory ***********************************
 *
 */
extern NU_MEMORY_POOL  System_Memory;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NET_BKCP_H */

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
*       externs6.h                                   
*                                                                                  
*   DESCRIPTION                                                              
*                       
*       This file contains the data structures and defines necessary
*       to support external functions for IPv6 in nucleus NET.
*                                                                          
*   DATA STRUCTURES                                                          
*           
*       None
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef EXTERNS6_H
#define EXTERNS6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define IP6_UNSPECIFIED     0xffffffffUL

#define NU_Is_IPv4_Mapped_Addr      IPV6_IS_ADDR_V4MAPPED
#define NU_Create_IPv4_Mapped_Addr  IP6_Create_IPv4_Mapped_Addr

typedef struct mbl_ip6_confg_struct
{
    UINT8   mbl_ip6;
    UINT8   mbl_ip6_rt_opt;
} MBL_IP6_CONFG_STRUCT;

/***** CFG6.C *****/

#define NU_Setup_Configured_Tunnel  CFG6_Configure_Tunnel

/***/

/***** SCK6_AR.C *****/

STATUS NU_Add_Route6(UINT8 *, UINT8 *, UINT8);

/***/

/***** SCK6_AIP.C *****/

STATUS NU_Add_IP_To_Device(const CHAR *, const UINT8 *, UINT8, UINT32, UINT32);

/***/

/***** ICMP6_ER.C *****/

STATUS ICMP6_Send_Echo_Request(UINT8 *, UINT32);

/***/

/***** TLS6.C *****/

UINT16 TLS6_Prot_Check(UINT16*, NET_BUFFER*);

/***/

/***** UTL6.C *****/

UINT16  UTL6_Checksum(NET_BUFFER*, const UINT8*, const UINT8*, UINT32, 
                      UINT8, UINT8);
UINT8   UTL6_Strip_Extension_Headers(NET_BUFFER *, UINT16 *);

/***/

/***** PRT6.C *****/

INT32 PRT6_Find_Matching_Port(INT, const UINT8 *, const UINT8 *, UINT16, 
                              UINT16);
/***/

/******* SCK6_GIA.C *****/

STATUS NU_Ioctl_SIOCGIFADDR_IN6(SCK_IOCTL_OPTION *, INT);

/***/

/***** SCK6_GDA.C ****/

STATUS NU_Ioctl_SIOCGIFDSTADDR_IN6(SCK_IOCTL_OPTION *, INT);

/****/

/******* SCK6_GND.C *****/

STATUS NU_Ioctl_SIOCLIFGETND(SCK_IOCTL_OPTION *);

/***/

/***** IP6_GIA.C *****/

STATUS IP6_Ioctl_SIOCGIFADDR_IN6(const CHAR *, UINT8 *, UINT8 *);

/***/

/***** IP6_GDA.C *****/

STATUS IP6_Ioctl_SIOCGIFDSTADDR_IN6(const CHAR *, UINT8 *);

/***/

/***** IP6_GND.C *****/

STATUS IP6_Ioctl_SIOCLIFGETND(const CHAR *, UINT8 *, UINT8 *);

/***/

/***** SCK6_SRHL.C *****/

STATUS NU_Setsockopt_IPV6_RECVHOPLIMIT(INT, INT);

/***/

/***** SCK6_SRPI.C *****/

STATUS NU_Setsockopt_IPV6_RECVPKTINFO(INT, INT);

/***/

/***** SCK6_GRHL.C *****/

STATUS NU_Getsockopt_IPV6_RECVHOPLIMIT(INT, INT *, INT *);

/***/


/***** SCK6_GRPI.C *****/

STATUS NU_Getsockopt_IPV6_RECVPKTINFO(INT, INT *, INT *);

/***/


/***** SCK6_SPI.C *****/

STATUS NU_Setsockopt_IPV6_PKTINFO(INT, in6_pktinfo *);

/***/

/***** SCK6_GPI.C *****/

STATUS NU_Getsockopt_IPV6_PKTINFO(INT, in6_pktinfo *);

/***/

/***** SCK6_GNH.C *****/

STATUS NU_Getsockopt_IPV6_NEXTHOP(INT, struct addr_struct *, INT *);

/***/

/***** SCK6_SNH.C *****/

STATUS NU_Setsockopt_IPV6_NEXTHOP(INT, const struct addr_struct *, INT);

/***/

/***** SCK6_GRTC.C *****/

STATUS NU_Getsockopt_IPV6_RECVTCLASS(INT, INT *, INT *);

/***/

/***** SCK6_SRTC.C *****/

STATUS NU_Setsockopt_IPV6_RECVTCLASS(INT, INT);

/***/

/***** SCK6_STC.C *****/

STATUS NU_Setsockopt_IPV6_TCLASS(INT, INT);

/***/

/***** SCK6_GTC.C *****/

STATUS NU_Getsockopt_IPV6_TCLASS(INT, INT *, INT *);

/***/

/***** SCK6_SRH.C *****/

STATUS NU_Setsockopt_IPV6_RTHDR(INT, const struct ip6_rthdr *, INT);

/***/

/***** SCK6_SRRH.C *****/

STATUS NU_Setsockopt_IPV6_RECVRTHDR(INT, INT);

/***/

/***** SCK6_GRRH.C *****/

STATUS NU_Getsockopt_IPV6_RECVRTHDR(INT, INT *, INT *);

/***/

/***** RTHDR6.C *****/

UINT32  inet6_rth_space(INT, INT);
VOID    *inet6_rth_init(VOID *, UINT32, INT, INT);
INT     inet6_rth_add(VOID *, const UINT8 *);
INT     inet6_rth_reverse(const VOID *, VOID *);
INT     inet6_rth_segments(const VOID *);
UINT8   *inet6_rth_getaddr(const VOID *, INT);

#define NU_Inet6_Rth_Space      inet6_rth_space
#define NU_Inet6_Rth_Init       inet6_rth_init
#define NU_Inet6_Rth_Add        inet6_rth_add
#define NU_Inet6_Rth_Reverse    inet6_rth_reverse
#define NU_Inet6_Rth_Segments   inet6_rth_segments
#define NU_Inet6_Rth_GetAddr    inet6_rth_getaddr

/***/

/***** SCK6_SRHO.C *****/

STATUS NU_Setsockopt_IPV6_RECVHOPOPTS(INT, INT);

/***/

/***** SCK6_GRHO.C *****/

STATUS NU_Getsockopt_IPV6_RECVHOPOPTS(INT, INT *, INT *);

/***/

/***** SCK6_SRDO.C *****/

STATUS NU_Setsockopt_IPV6_RECVDSTOPTS(INT, INT);

/***/

/***** SCK6_GRDO.C *****/

STATUS NU_Getsockopt_IPV6_RECVDSTOPTS(INT, INT *, INT *);

/***/

/***** SCK6_GHO.C *****/

STATUS NU_Getsockopt_IPV6_HOPOPTS(INT, struct ip6_hbh *, INT *);

/***/

/***** SCK6_SHO.C *****/

STATUS NU_Setsockopt_IPV6_HOPOPTS(INT, const struct ip6_hbh *, INT);

/***/

/***** SCK6_GDO.C *****/

STATUS NU_Getsockopt_IPV6_DSTOPTS(INT, struct ip6_dest *, INT *);

/***/

/***** SCK6_SDO.C *****/

STATUS NU_Setsockopt_IPV6_DSTOPTS(INT, const struct ip6_dest *, INT);

/***/

/***** IP6OPT.C *****/

INT inet6_opt_init(VOID *, UINT32 );
INT inet6_opt_append(VOID *, UINT32, INT, UINT8, UINT32, UINT8, VOID **);
INT inet6_opt_finish(VOID *, UINT32, INT);
INT inet6_opt_set_val(VOID *, UINT32, const VOID *, INT);
INT inet6_opt_next(VOID *, UINT32, INT, UINT8 *, UINT32 *, VOID **);
INT inet6_opt_find(VOID *, UINT32, INT, UINT8, UINT32 *, VOID **);
INT inet6_opt_get_val(const VOID *, UINT32, VOID *, INT);

#define NU_Inet6_Opt_Init      inet6_opt_init
#define NU_Inet6_Opt_Append    inet6_opt_append
#define NU_Inet6_Opt_Finish    inet6_opt_finish
#define NU_Inet6_Opt_Set_Val   inet6_opt_set_val
#define NU_Inet6_Opt_Next      inet6_opt_next
#define NU_Inet6_Opt_Find      inet6_opt_find
#define NU_Inet6_Opt_Get_Val   inet6_opt_get_val

/***/

/***** SCK6_GRH.C *****/

STATUS NU_Getsockopt_IPV6_RTHDR(INT, struct ip6_rthdr *, INT *);

/***/

/***** SCK6_SRHDO.C *****/

STATUS NU_Setsockopt_IPV6_RTHDRDSTOPTS(INT, const struct ip6_dest *, INT);

/***/

/***** SCK6_GRHDO.C *****/

STATUS NU_Getsockopt_IPV6_RTHDRDSTOPTS(INT, struct ip6_dest *, INT *);

/***/

/***** SCK6_GMH.C *****/

STATUS NU_Getsockopt_IPV6_MULTICAST_HOPS(INT socketd, INT *hop_limit);

/***/

/***** SCK6_GMI.C *****/

STATUS NU_Getsockopt_IPV6_MULTICAST_IF(INT socketd, INT32 *if_index);

/***/

/***** SCK6_JG.C *****/

STATUS NU_Setsockopt_IPV6_JOIN_GROUP(INT socketd, const IP6_MREQ *mreq);

/***/

/***** SCK6_LG.C *****/

STATUS NU_Setsockopt_IPV6_LEAVE_GROUP(INT socketd, const IP6_MREQ *mreq);

/***/

/***** SCK6_SMH.C *****/

STATUS NU_Setsockopt_IPV6_MULTICAST_HOPS(INT socketd, INT hop_limit);

/***/

/***** SCK6_SMI.C *****/

STATUS NU_Setsockopt_IPV6_MULTICAST_IF(INT socketd, INT32 if_index);

/***/

/***** SCK6_SV6O.C *****/

STATUS NU_Setsockopt_IPV6_V6ONLY(INT socketd, INT optval);

/***/

/***** SCK6_GV6O.C *****/

STATUS NU_Getsockopt_IPV6_V6ONLY(INT socketd, INT *optval, INT *optlen);

/***/

/***** SCK6_SC.C *****/

STATUS NU_Setsockopt_IPV6_CHECKSUM(INT socketd, INT optval);

/***/

/***** SCK6_GC.C *****/

STATUS NU_Getsockopt_IPV6_CHECKSUM(INT socketd, INT *optval, INT *optlen);

/***/

/***** SCK6_CR.C *****/

STATUS NU_Configure_Router(UINT32, UINT32, INT, const DEV6_RTR_OPTS *);

/***/

/***** SCK6_DPE.C *****/

STATUS NU_Delete_Prefix_Entry(UINT32, const UINT8 *);

/***/

/***** SCK6_APE.C *****/

STATUS NU_Add_Prefix_Entry(UINT32, DEV6_PRFX_ENTRY *);

/***/

/***** SCK6_SUH.C *****/

STATUS NU_Setsockopt_IPV6_UNICAST_HOPS(INT, INT16);

/***/

/***** SCK6_GUH.C *****/

STATUS NU_Getsockopt_IPV6_UNICAST_HOPS(INT, INT16 *);

/***/

/***** SCK6_IPML.C *****/

STATUS NU_IP6_Multicast_Listen(INT, UINT32, const UINT8 *, UINT16, 
                               const UINT8 *, UINT16);

/***/

/***** IP6_PML.C *****/

STATUS IP6_Process_Multicast_Listen(INT, UINT32, const UINT8 *, UINT16, 
                                    const UINT8 *, UINT16);

/***/

/***** SCK6_SDHL.C *****/

STATUS NU_Set_Default_Hop_Limit(UINT8);

/***/

/***** IP6_BUILD_TYPE2_RH.C *****/

VOID IP6_Build_Type_2_RtHdr(UINT8 *, UINT8, const UINT8 *);

/***/

/***** DHCP6_SIAID.C *****/

STATUS NU_Set_DHCP6_IAID(const UINT32, UINT32);

/***/

/***** DHCP6_GIAID.C *****/

STATUS NU_Get_DHCP6_IAID(const UINT32, UINT32 *);

/***/

/***** DHCP6.C *****/

STATUS DHCP6_Init(VOID);

/***/

/***** SCK6_GLLADDR.C *****/

STATUS NU_Get_Link_Local_Addr(const UINT32, UINT8 *);

/***/

/***** SCK6_CII.C *****/

STATUS NU_Configure_IPv6_Interface(UINT32, INT, VOID *);

/***/

/***** SCK6_CPT.C *****/

STATUS NU_Configure_Policy_Table(IP6_POLICY_ENTRY *, UINT32);

/***/

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

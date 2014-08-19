/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
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
*       netv6_exp.c
*
*   COMPONENT
*
*       IPv6 Export Symbols
*
*   DESCRIPTION
*
*       Export symbols for the IPv6 component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       None
*
*************************************************************************/

#include "networking/nu_networking.h"

#if (CFG_NU_OS_NET_IPV6_EXPORT_SYMBOLS == NU_TRUE)

#include "kernel/proc_extern.h"

/* Define component name for these symbols */
NU_SYMBOL_COMPONENT(NU_OS_NET_IPV6);

#if (INCLUDE_IPV6 == NU_TRUE)
/* Nucleus IPv6 symbols */
NU_EXPORT_SYMBOL(NU_Add_IP_To_Device);
NU_EXPORT_SYMBOL(NU_Add_Prefix_Entry);
NU_EXPORT_SYMBOL(NU_Add_Route6);
NU_EXPORT_SYMBOL(NU_Configure_IPv6_Interface);
NU_EXPORT_SYMBOL(NU_Configure_Policy_Table);
NU_EXPORT_SYMBOL(IP6_Create_IPv4_Mapped_Addr);      /* NU_Create_IPv4_Mapped_Addr maps to this. */
NU_EXPORT_SYMBOL(NU_Delete_Prefix_Entry);
NU_EXPORT_SYMBOL(NU_Get_Link_Local_Addr);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_CHECKSUM);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_DSTOPTS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_HOPOPTS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_MULTICAST_HOPS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_MULTICAST_IF);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_NEXTHOP);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_PKTINFO);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RECVDSTOPTS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RECVHOPLIMIT);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RECVHOPOPTS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RECVPKTINFO);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RECVRTHDR);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RECVTCLASS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RTHDR);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_RTHDRDSTOPTS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_TCLASS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_UNICAST_HOPS);
NU_EXPORT_SYMBOL(NU_Getsockopt_IPV6_V6ONLY);
NU_EXPORT_SYMBOL(inet6_opt_append);                 /* NU_Inet6_Opt_Append maps to this. */
NU_EXPORT_SYMBOL(inet6_opt_find);                   /* NU_Inet6_Opt_Find maps to this. */
NU_EXPORT_SYMBOL(inet6_opt_finish);                 /* NU_Inet6_Opt_Finish maps to this. */
NU_EXPORT_SYMBOL(inet6_opt_get_val);                /* NU_Inet6_Opt_Get_Val maps to this. */
NU_EXPORT_SYMBOL(inet6_opt_init);                   /* NU_Inet6_Opt_Init maps to this. */
NU_EXPORT_SYMBOL(inet6_opt_next);                   /* NU_Inet6_Opt_Next maps to this. */
NU_EXPORT_SYMBOL(inet6_opt_set_val);                /* NU_Inet6_Opt_Set_Val maps to this. */
NU_EXPORT_SYMBOL(inet6_rth_add);                    /* NU_Inet6_Rth_Add maps to this. */
NU_EXPORT_SYMBOL(inet6_rth_getaddr);                /* NU_Inet6_Rth_GetAddr maps to this. */
NU_EXPORT_SYMBOL(inet6_rth_init);                   /* NU_Inet6_Rth_Init maps to this. */
NU_EXPORT_SYMBOL(inet6_rth_reverse);                /* NU_Inet6_Rth_Reverse maps to this. */
NU_EXPORT_SYMBOL(inet6_rth_segments);               /* NU_Inet6_Rth_Segments maps to this. */
NU_EXPORT_SYMBOL(inet6_rth_space);                  /* NU_Inet6_Rth_Space maps to this. */
NU_EXPORT_SYMBOL(NU_Ioctl_SIOCGIFADDR_IN6);
NU_EXPORT_SYMBOL(NU_Ioctl_SIOCGIFDSTADDR_IN6);
NU_EXPORT_SYMBOL(NU_IP6_Multicast_Listen);
NU_EXPORT_SYMBOL(NU_Set_Default_Hop_Limit);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_CHECKSUM);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_DSTOPTS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_HOPOPTS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_JOIN_GROUP);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_LEAVE_GROUP);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_MULTICAST_HOPS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_MULTICAST_IF);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_NEXTHOP);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_PKTINFO);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RECVDSTOPTS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RECVHOPLIMIT);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RECVHOPOPTS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RECVPKTINFO);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RECVRTHDR);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RECVTCLASS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RTHDR);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_RTHDRDSTOPTS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_TCLASS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_UNICAST_HOPS);
NU_EXPORT_SYMBOL(NU_Setsockopt_IPV6_V6ONLY);
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
NU_EXPORT_SYMBOL(CFG6_Configure_Tunnel);            /* NU_Setup_Configured_Tunnel maps to this. */
#endif

#if (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_UDP == NU_TRUE)
NU_EXPORT_SYMBOL(NU_Ripng_Initialize);
#endif

#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_DHCP6 == NU_TRUE))
NU_EXPORT_SYMBOL(NU_Dhcp6);
NU_EXPORT_SYMBOL(NU_Dhcp6_Shutdown);
#endif

#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE))
NU_EXPORT_SYMBOL(NU_Configure_Router);
#endif

#endif /* CFG_NU_OS_NET_STACK_EXPORT_SYMBOLS == NU_TRUE */

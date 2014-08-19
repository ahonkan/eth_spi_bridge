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
*        ip6_mib_udp.h                               
*
*   DESCRIPTION
*
*        This file contains the declaration of the functions that provides
*        interface to the SNMP.
*
*   DATA STRUCTURES
*
*        None.
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_UDP_H
#define IP6_MIB_UDP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#if (INCLUDE_IPV6_UDP_MIB == NU_TRUE)

UINT16 IP6_MIB_UDP_Get_Socket_Info(UINT8 getflag, UINT8 *local_addr,
                              UINT16 *local_port, UINT32 *local_if_index);


#define IP6_MIB_UDP_SOCK_INFO_GET(local_addr, local_port, local_if_index,\
                                  status)                                \
    (status) = IP6_MIB_UDP_Get_Socket_Info(((UINT8)NU_TRUE),            \
                      (local_addr), (&(local_port)), (&(local_if_index)))

#define IP6_MIB_UDP_SOCK_INFO_GET_NEXT(local_addr, local_port,      \
                                       local_if_index, status)      \
    (status) = IP6_MIB_UDP_Get_Socket_Info(((UINT8)NU_FALSE),       \
                      (local_addr), (&(local_port)), (&(local_if_index)))

#endif /* (INCLUDE_IPV6_UDP_MIB == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_UDP_H */

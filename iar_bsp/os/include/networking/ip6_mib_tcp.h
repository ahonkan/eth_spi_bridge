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
*        ip6_mib_tcp.h                               
*
*   DESCRIPTION
*
*        This file hold the declaration of functions and macros that will
*        be used to maintain 'ipv6TcpConnTable'.
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

#ifndef IP6_MIB_TCP_H
#define IP6_MIB_TCP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

#if (INCLUDE_IPV6_TCP_MIB == NU_TRUE)

/* Macro for the delete TCP socket state. */
#define IP6_MIB_TCB_DELETE      12

UINT16 IP6_MIB_TCP_Get_State(UINT8 getflag, UINT8 *local_addr,
                             UINT16 *local_port, UINT8 *remote_addr,
                             UINT16 *remote_port, UINT32 *if_index,
                             UINT32 *sock_state);

UINT16 IP6_MIB_TCP_Set_State(UINT8 *local_addr, UINT16 local_port,
                             UINT8 *remote_addr, UINT16 remote_port,
                             UINT32 if_index, UINT32 sock_state);

#define IP6_MIB_TCP_GET_NEXT_STATE(local_addr, local_port, remote_addr,  \
                                   remote_port, if_index, state, status) \
    (status) = IP6_MIB_TCP_Get_State((UINT8)NU_FALSE, (local_addr),      \
                (&(local_port)), (remote_addr), (&(remote_port)),        \
                (&(if_index)), (&(state)))

#define IP6_MIB_TCP_GET_STATE(local_addr, local_port, remote_addr,      \
                              remote_port, if_index, state, status)     \
    (status) = IP6_MIB_TCP_Get_State((UINT8)NU_TRUE, (local_addr),      \
                (&(local_port)), (remote_addr), (&(remote_port)),       \
                (&(if_index)), (&(state)))

#define IP6_MIB_TCP_SET_STATE(local_addr, local_port, remote_addr,      \
                              remote_port, if_index, state, status)     \
    (status) = IP6_MIB_TCP_Set_State((local_addr), (local_port),        \
                       (remote_addr), (remote_port), (if_index), (state))

#endif /* (INCLUDE_IPV6_TCP_MIB == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_TCP_H */

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
*   FILENAME                                                                     
*                                                                                  
*       socketd6.h                                   
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the IPv6 Sockets layer.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       SCK6_SOCKADDR_IP_STRUCT
*       IP6_MREQ
*
*   DEPENDENCIES                                                             
*                                                                          
*       None.
*                                                                          
*************************************************************************/

#ifndef SOCKETD6_H
#define SOCKETD6_H

/*
 * IPv6 address
 */

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

struct SCK6_SOCKADDR_IP_STRUCT
{
    UINT8           sck_len;
    UINT8           sck_family;
    UINT16          sck_port;
    UINT8           sck_addr[16];
    INT8            sck_unused[8];
};

/* IP Multicast Request structure. This structure is used when using 
   NU_Setsockopt or NU_Getsockopt to set or get IPv6 multicasting options. */
typedef struct _ip6_mreq 
{
    UINT8   ip6_mreq_multiaddr[16];      /* IPv6 multicast address. */
    UINT32  ip6_mreq_dev_index;
} IP6_MREQ;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

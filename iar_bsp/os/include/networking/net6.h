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
*       net6.h                                       
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the ethernet layer specific to IPv6.
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

#ifndef NET6_H
#define NET6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* This macro maps an IPv6 multicast IP address to a multicast ethernet address. */
#define NET6_MAP_IP_TO_ETHER_MULTI(ip_addr, ether_addr) \
{    \
    (ether_addr)[0] = 0x33; \
    (ether_addr)[1] = 0x33; \
    (ether_addr)[2] = ip_addr[12]; \
    (ether_addr)[3] = ip_addr[13]; \
    (ether_addr)[4] = ip_addr[14]; \
    (ether_addr)[5] = ip_addr[15]; \
}

STATUS ETH6_Map_Multi(const DV_REQ *, UINT8 *);
STATUS ETH6_Init_IPv6_Device(DV_DEVICE_ENTRY *, INT16, UINT8, UINT8);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NET6_H */

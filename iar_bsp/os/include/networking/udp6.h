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
*       udp6.h                                       
*                                                                           
*   DESCRIPTION                                                             
*                                                                           
*       This file contains the structure definitions required by the UDP 
*       module of Nucleus NET for IPv6 packets.
*                                                                           
*   DATA STRUCTURES                                                         
*                                                                           
*       None
*                                                                           
*   DEPENDENCIES                                                            
*                                                                           
*       udp.h                                                                
*                                                                           
*************************************************************************/

#ifndef UDP6_H
#define UDP6_H

#include "networking/udp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS  UDP6_Cache_Route (UDP_PORT *, UINT8 *);
STATUS  UDP6_Input(IP6LAYER *, NET_BUFFER *, struct pseudohdr *);
INT32   UDP6_Find_Matching_UDP_Port(const UINT8 *, const UINT8 *, UINT16, 
                                    UINT16);
VOID    UDP6_Free_Cached_Route(RTAB6_ROUTE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

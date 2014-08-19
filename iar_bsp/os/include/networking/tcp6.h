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
*       tcp6.h                                       
*                                                                           
*   DESCRIPTION                                                             
*                                                                           
*       This file contains the structure definitions required by the TCP 
*       module of Nucleus NET for IPv6 packets.
*                                                                           
*   DATA STRUCTURES                                                         
*                                                                           
*       None
*                                                                           
*   DEPENDENCIES                                                            
*                                                                           
*       tcp.h
*                                                                           
*************************************************************************/

#ifndef TCP6_H
#define TCP6_H

#include "networking/tcp.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT16   TCP6_Input(NET_BUFFER *, struct pseudohdr *);
INT32   TCP6_Find_Matching_TCP_Port(const UINT8 *, const UINT8 *, UINT16, 
                                    UINT16);
VOID    TCP6_Free_Cached_Route(RTAB6_ROUTE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

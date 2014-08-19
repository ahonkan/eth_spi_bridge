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
*       resolve6.h                                   
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the Address Resolution.
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

#ifndef RESOLVE6_H
#define RESOLVE6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS Resolve6_Get_Link_Addr(DV_DEVICE_ENTRY *, SCK6_SOCKADDR_IP *, UINT8 *,
                              NET_BUFFER*);
VOID Resolve6_Event(TQ_EVENT, UNSIGNED, UNSIGNED);
VOID Resolve6_Init(VOID);
VOID Resolve6_Tx_Queued_Data(IP6_NEIGHBOR_CACHE_ENTRY *, DV_DEVICE_ENTRY *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

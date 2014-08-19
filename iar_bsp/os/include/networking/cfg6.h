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
*       cfg6.h                                       
*                                                                           
*   DESCRIPTION                                                             
*                                                                           
*       This file contains definitions for macros, data structures and
*       functions for the IPv6 Configured Tunnel module.
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

#ifndef CFG6_H
#define CFG6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Only 1 Neighbor Cache entry is needed for a configured tunnel,
 * since there is only one possible next-hop - the tunnel endpoint.
 */
#define IP6_CFG6_NEIGHCACHE_ENTRIES     1

STATUS  CFG6_Configure_Tunnel(const UINT8 *, const UINT8 *, const UINT8 *);
STATUS  CFG6_Initialize_Tunnel(DV_DEVICE_ENTRY *);
STATUS  CFG6_Output(NET_BUFFER *, DV_DEVICE_ENTRY *, VOID *, VOID *);    

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

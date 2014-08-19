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
*       6to4.h                                       
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for the 6to4 tunnel module.
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

#ifndef SIXTOFOUR
#define SIXTOFOUR

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define IP6_6TO4_NEIGHCACHE_ENTRIES     5

STATUS  SIXTOFOUR_Initialize(DV_DEVICE_ENTRY *);
STATUS  SIXTOFOUR_Output(NET_BUFFER *, DV_DEVICE_ENTRY *, VOID *, VOID *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

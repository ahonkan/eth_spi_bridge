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
*       nd6rdrct.h                                   
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for processing and transmitting Redirects.
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

#ifndef ND6RDRCT
#define ND6RDRCT

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS ND6RDRCT_Input(IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                      const NET_BUFFER *buf_ptr);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

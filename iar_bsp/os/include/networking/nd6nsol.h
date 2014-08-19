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
*       nd6nsol.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for processing and transmitting Neighbor 
*       Solicitations.
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

#ifndef ND6NSOL_H
#define ND6NSOL_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS ND6NSOL_Input(IP6LAYER *, DV_DEVICE_ENTRY *, const NET_BUFFER *);
NET_BUFFER *ND6NSOL_Build(const DV_DEVICE_ENTRY *, const UINT8 *, 
                          const UINT8 *);
STATUS ND6NSOL_Output(DV_DEVICE_ENTRY *, UINT8 *, const UINT8 *, const UINT8 *, 
                      UINT8);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

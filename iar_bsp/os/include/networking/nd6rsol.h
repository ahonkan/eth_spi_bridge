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
*       nd6rsol.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for processing and transmitting Router Solicitations.
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

#ifndef ND6RSOL_H
#define ND6RSOL_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

STATUS      ND6RSOL_Input(IP6LAYER *, DV_DEVICE_ENTRY *, const NET_BUFFER *);
NET_BUFFER  *ND6RSOL_Build(const DV_DEVICE_ENTRY *, const UINT8 *, UINT8 *);
STATUS      ND6RSOL_Output(DV_DEVICE_ENTRY *, UINT8 *, UINT8 *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif

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
*       tftpc6.h                                     
*                                                                       
*   DESCRIPTION                                                          
*                                                                       
*       This file contains function prototypes of all TFTP functions for
*       operation over IPv6.    
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

#ifndef NU_TFTPC6_H
#define NU_TFTPC6_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

INT32  TFTPC6_Get(UINT8 *, CHAR *, CHAR *, TFTP_OPTIONS *);
INT32  TFTPC6_Put(UINT8 *, CHAR *, CHAR *, TFTP_OPTIONS *);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTPC6_H */

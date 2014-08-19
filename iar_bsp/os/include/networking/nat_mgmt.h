/****************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2006 
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       nat_mgmt.h                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains those function definitions necessary to retrieve 
*       statistics about the Nucleus NAT environment.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.
*                                               
*   FUNCTIONS                                                                  
*              
*       None.
*                                             
*   DEPENDENCIES                                                               
*
*       None
*                                                                
******************************************************************************/

#ifndef _NAT_MGMT_
#define _NAT_MGMT_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

UINT16 NATMGMT_Calculate_Used_TCP_Ports(VOID);
UINT16 NATMGMT_Calculate_Used_UDP_Ports(VOID);
UINT16 NATMGMT_Calculate_Used_ICMP_Ports(VOID);
UINT16 NATMGMT_Calculate_Total_Portmap_Entries(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _NAT_MGMT_ */

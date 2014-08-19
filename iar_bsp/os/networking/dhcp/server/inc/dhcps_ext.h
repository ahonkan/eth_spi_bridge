/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*                                                                            
* FILE NAME                                                                         
*                                                                                    
*   dhcps_ext.h
*
* COMPONENT
*
*   Nucleus DHCP Server
*                                                                                    
* DESCRIPTION                                                                
*                                                                            
*   External variables used across multiple files
*
* DATA STRUCTURES
*
*   None
*
* DEPENDENCIES
*
*   None
*                                                                            
**************************************************************************/

#ifndef _DHCPS_EXT_H_
#define _DHCPS_EXT_H_

/* Global Variables */
extern NU_MEMORY_POOL               *DHCPServer_Memory;
extern DHCPS_CONFIGURATION_LIST     DHCPS_Config_List;
extern NU_SEMAPHORE                 DHCPS_Semaphore;
extern BOOLEAN                      DHCPS_Initialized;

#endif /* _DHCPS_EXT_H_ */

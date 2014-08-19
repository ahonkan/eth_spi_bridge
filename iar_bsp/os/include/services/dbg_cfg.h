/*************************************************************************
*
*               Copyright 2008 Mentor Graphics Corporation
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
*       dbg_cfg.h                                           
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Configuration
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C external interface for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       None       
*            
*   FUNCTIONS
*
*       None
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_CFG_H
#define DBG_CFG_H

/* (Internal) Configuration defines */

/* String Max - Maximum number of characters in a general string used by 
   the debug system.  Includes NULL-terminator, if used.  Default value is
   16. */

#define DBG_CFG_STR_MAX                                         16

#endif /* DBG_CFG_H */


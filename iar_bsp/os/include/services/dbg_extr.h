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
*       dbg_extr.h                        
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - External Interface
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C definitions for the component.                            
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
*       dbg_sts.h                                         
*       dbg_os.h                                   
*                                                                      
*************************************************************************/

#ifndef DBG_EXTR_H
#define DBG_EXTR_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Status component. */

#include "services/dbg_sts.h"

/* OS Abstraction components. */

#include "services/dbg_os.h"

#ifdef __cplusplus
}
#endif

#endif /* DBG_EXTR_H */

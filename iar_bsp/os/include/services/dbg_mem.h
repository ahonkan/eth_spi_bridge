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
*       dbg_mem.h                                           
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Memory
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
*       DBG_System_Memory_Allocate
*       DBG_System_Memory_Deallocate
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_MEM_H
#define DBG_MEM_H

#ifdef __cplusplus
extern "C"
{
#endif

/* System Memory Allocate parameter values */

#define DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN                1
#define DBG_SYSTEM_MEMORY_ALLOC_CACHED                  NU_TRUE
#define DBG_SYSTEM_MEMORY_ALLOC_UNCACHED                NU_FALSE 
#define DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT               NU_TRUE
#define DBG_SYSTEM_MEMORY_ALLOC_NO_ZERO_INIT            NU_FALSE

/***** Global functions */

VOID *  DBG_System_Memory_Allocate(UNSIGNED             size,
                                   UNSIGNED             alignment,
                                   BOOLEAN              cached,
                                   BOOLEAN              zeroed,
                                   UNSIGNED             suspend);

DBG_STATUS  DBG_System_Memory_Deallocate(VOID *   p_memory);

#ifdef __cplusplus
}
#endif

#endif /* DBG_MEM_H */

/*************************************************************************
*                                                                      
*               Copyright Mentor Graphics Corporation 2010              
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
*       dbg_mem.c
*             
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Memory
*                                                                
*   DESCRIPTION                                                          
*                                                                      
*       This file contains Debug Agent memory utility routines.               
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
*       dbg.h                                                                         
*                                                                      
*************************************************************************/

#include "services/dbg.h"

/* Nucleus system memory resources */

extern  NU_MEMORY_POOL           System_Memory;

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_System_Memory_Allocate                 
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       Utility function to allocate memory from the system memory.
*                                                                                                                                            
*   INPUTS                                                               
*
*       size - The size (in bytes) of the allocation.
*           
*       alignment - Indicates the required alignment of the memory to be
*                    provided.
*           
*       cached - Indicates if the memory should be cached or not.  A value
*                of NU_TRUE indicates cached memory will be provided and a
*                value of NU_FALSE indicates that un-cached memory should
*                be provided.
*
*       zeroed - Indicates if the memory should be zeroed out or not.  A
*                value of NU_TRUE indicates memory should be 
*                zero-initialized and a value of NU_FALSE indicates it
*                will be provided as returned from the allocation routine.
*
*       suspend - Indicates if the operation should suspend or not waiting
*                 for the memory to be available.  Valid values match
*                 those of the Nucleus PLUS memory allocation routines.
*
*   OUTPUTS                                                              
*                                                                      
*       <non-NULL> - Indicates successful operation.
*
*       NULL - Indicates operation failed.
*                                                                      
*************************************************************************/
VOID *  DBG_System_Memory_Allocate(UNSIGNED             size,
                                   UNSIGNED             alignment,
                                   BOOLEAN              cached,
                                   BOOLEAN              zeroed,
                                   UNSIGNED             suspend)
{
    STATUS              nu_status;
    VOID *              p_memory;
    NU_MEMORY_POOL      *p_memory_pool;

    /* Determine if memory should be allocated from the cached or 
       uncached system memory. */

    if (cached == NU_TRUE)
    {
        p_memory_pool = &System_Memory;
        
    }
    else
    {
        nu_status = NU_System_Memory_Get(NU_NULL, &p_memory_pool);
    }
        
    /* Determine if aligned memory is requested and provide memory
       appropriately. */
    
    if (alignment > 1)
    {
        nu_status = NU_Allocate_Aligned_Memory(p_memory_pool,
                                               &p_memory,
                                               size,
                                               alignment,
                                               suspend);
     
    }
    else
    {
        nu_status = NU_Allocate_Memory(p_memory_pool,
                                       &p_memory,
                                       size,
                                       suspend);
                                   
    }
    
    /* Determine how to proceed based on the success of the allocation. */

    if (nu_status == NU_SUCCESS)
    {
        if (zeroed == NU_TRUE)
        {
            /* Zero initialize the memory. */
            
           memset(p_memory,
                  0x00,
                  size);
       
        }
       
    }
    else
    {
        /* Update return value to indicate error. */

        p_memory = NU_NULL;
    
    }
    
    return (p_memory);
}

/*************************************************************************
*   
*   FUNCTION                                                             
*                                                                      
*       DBG_System_Memory_Deallocate                 
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       Deallocate memory from the system memory.
*                                                                                                                                            
*   INPUTS                                                               
*      
*       p_memory - Pointer to the memory to be deallocated.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates that operation failed.
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_System_Memory_Deallocate(VOID *   p_memory)
{
    DBG_STATUS  dbg_status;
    STATUS      nu_status;

    /* Attempt to deallocate memory to the system memory. */

    nu_status = NU_Deallocate_Memory(p_memory);
    
    if (nu_status == NU_SUCCESS)
    {
        dbg_status = DBG_STATUS_OK;
    }
    else
    {
        /* ERROR: Unable to de-allocate memory */
        
        dbg_status = DBG_STATUS_FAILED;
        
    }
    
    return (dbg_status);
}

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
*       dbg_eng_mem.c                                      
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Memory
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C main functions source code for the 
*       component.  This includes all global and local functions and 
*       variables.
*
*       This component is responsible for providing memory manipulation
*       services.
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       None       
*                                                                      
*   FUNCTIONS                                                            
*                 
*       dbg_eng_mem_memory_copy_8_bit
*       dbg_eng_mem_memory_copy_16_bit
*       dbg_eng_mem_memory_copy_32_bit
*        
*       DBG_ENG_MEM_Initialize
*       DBG_ENG_MEM_Terminate
*       DBG_ENG_MEM_Memory_Copy
*       DBG_ENG_MEM_Operation_Cancel
*                                        
*   DEPENDENCIES
*
*       dbg.h
*                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"

/***** Local functions */

/* Local function prototypes */

static DBG_STATUS dbg_eng_mem_memory_copy_8_bit(DBG_ENG_MEM_CB *       p_dbg_eng_mem,
                                                UINT                   copy_size,
                                                VOID *                 p_src,
                                                VOID *                 p_dst,
                                                UINT *                 p_actual_copy_size);

static DBG_STATUS dbg_eng_mem_memory_copy_16_bit(DBG_ENG_MEM_CB *      p_dbg_eng_mem,
                                                 UINT                  copy_size,
                                                 VOID *                p_src,
                                                 VOID *                p_dst,
                                                 UINT *                p_actual_copy_size);

static DBG_STATUS dbg_eng_mem_memory_copy_32_bit(DBG_ENG_MEM_CB *      p_dbg_eng_mem,
                                                 UINT                  copy_size,
                                                 VOID *                p_src,
                                                 VOID *                p_dst,
                                                 UINT *                p_actual_copy_size);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_mem_memory_copy_8_bit
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function copies memory using 1-byte accesses.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_mem - Pointer to control block.
*
*       copy_size - Size (in bytes) of the copy operation.
*
*       p_src - Pointer to the copy source.
*
*       p_dst - Pointer to the copy destination.
*
*       p_actual_copy_size - Return parameter that will be updated to
*                         indicate the actual size (in bytes) copied. Note
*                         that this value is updated regardless of whether
*                         the operation is successful or not (useful for
*                         partial copies...)
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the operation caused an
*                                      exception.
*
*       DBG_STATUS_VERIFY_FAIL - Indicates that the verification of the
*                                operation failed (after copy, values at
*                                source and destination did not match).
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_mem_memory_copy_8_bit(DBG_ENG_MEM_CB *       p_dbg_eng_mem,
                                                UINT                   copy_size,
                                                VOID *                 p_src,
                                                VOID *                 p_dst,
                                                UINT *                 p_actual_copy_size)
{
    DBG_STATUS      dbg_status;
    UINT            copy_remaining;
    UINT8 *         p_copy;
    UINT8 *         p_buffer;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Setup for copy operation. */
    
    copy_remaining = copy_size;
    p_copy = (UINT8 *)p_src;
    p_buffer = (UINT8 *)p_dst;
    
    /* Reset the operation status to allow operation (in case it was 
       cancelled on a previous operation). */

    p_dbg_eng_mem -> op_ok = NU_TRUE;    
    
    /* Perform copy operation. */
    
    while ((dbg_status == DBG_STATUS_OK) &&
           (copy_remaining > 0))
    {
        /* Read using appropriate memory access. */
        
        /* Perform memory access. */
        
        *p_buffer = *p_copy;
        
        /* The previous operation could cause an exception so
           check the status of the operation to determine if the
           operation should continue. */
           
        if (p_dbg_eng_mem -> op_ok == NU_FALSE)
        {
            /* The operation has been cancelled. */
            
            dbg_status = DBG_STATUS_INVALID_OPERATION;
        
        }
        else
        {
            /* Validate the copy. */
            
            if (*p_buffer != *p_copy)
            {
                /* ERROR: Validation of copy failed. */
                
                dbg_status = DBG_STATUS_VERIFY_FAIL;
                
            } 
          
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update the buffer address. */
                
                p_buffer++;
    
                /* Update copy address. */
                
                p_copy++;                
                
                /* Update copy remaining bytes. */
                
                copy_remaining -= sizeof(UINT8);
    
            }           
          
        } 

    } 
    
    /* Calculate the actual copy size.  Normally this will be the
       copy size parameter passed in, but it is possible that not
       all memory was copied in cases where the amount of data to
       copy was not evenly divisible by the access size. */
    
    *p_actual_copy_size = (copy_size - copy_remaining);    

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_mem_memory_copy_16_bit
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function copies memory using 16-bit accesses.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_mem - Pointer to control block.
*
*       copy_size - Size (in bytes) of the copy operation.
*
*       p_src - Pointer to the copy source.
*
*       p_dst - Pointer to the copy destination.
*
*       p_actual_copy_size - Return parameter that will be updated to
*                         indicate the actual size (in bytes) copied. Note
*                         that this value is updated regardless of whether
*                         the operation is successful or not (useful for
*                         partial copies...)
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the operation caused an
*                                      exception.
*
*       DBG_STATUS_VERIFY_FAIL - Indicates that the verification of the
*                                operation failed (after copy, values at
*                                source and destination did not match).
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_mem_memory_copy_16_bit(DBG_ENG_MEM_CB *      p_dbg_eng_mem,
                                                 UINT                  copy_size,
                                                 VOID *                p_src,
                                                 VOID *                p_dst,
                                                 UINT *                p_actual_copy_size)
{
    DBG_STATUS      dbg_status;
    UINT            copy_remaining;
    UINT16 *        p_copy;
    UINT16 *        p_buffer;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Setup for copy operation. */
    
    copy_remaining = copy_size;
    p_copy = (UINT16 *)p_src;
    p_buffer = (UINT16 *)p_dst;
    
    /* Reset the operation status to allow operation (in case it was 
       cancelled on a previous operatoin). */

    p_dbg_eng_mem -> op_ok = NU_TRUE;    
    
    /* Perform copy operation. */
    
    while ((dbg_status == DBG_STATUS_OK) &&
           (copy_remaining > 0))
    {
        /* Read using appropriate memory access. */
        
        /* Perform memory access. */
        
        *p_buffer = *p_copy;
        
        /* The previous operation could cause an exception so
           check the status of the operation to determine if the
           operation should continue. */
           
        if (p_dbg_eng_mem -> op_ok == NU_FALSE)
        {
            /* The operation has been cancelled. */
            
            dbg_status = DBG_STATUS_INVALID_OPERATION;
        
        }
        else
        {
            /* Validate the copy. */
            
            if (*p_buffer != *p_copy)
            {
                /* ERROR: Validation of copy failed. */
                
                dbg_status = DBG_STATUS_VERIFY_FAIL;
                
            } 
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update the buffer address. */
                
                p_buffer++;
    
                /* Update copy address. */
                
                p_copy++;                
                
                /* Update copy remaining bytes. */
                
                copy_remaining -= sizeof(UINT16);
        
            }             
            
        } 
        
    } 
    
    /* Calculate the actual copy size.  Normally this will be the
       copy size parameter passed in, but it is possible that not
       all memory was copied in cases where the amount of data to
       copy was not evenly divisible by the access size. */
    
    *p_actual_copy_size = (copy_size - copy_remaining);    

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_mem_memory_copy_32_bit
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function copies memory using 32-bit accesses.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_mem - Pointer to control block.
*
*       copy_size - Size (in bytes) of the copy operation.
*
*       p_src - Pointer to the copy source.
*
*       p_dst - Pointer to the copy destination.
*
*       p_actual_copy_size - Return parameter that will be updated to
*                         indicate the actual size (in bytes) copied. Note
*                         that this value is updated regardless of whether
*                         the operation is successful or not (useful for
*                         partial copies...)
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the operation caused an
*                                      exception.
*
*       DBG_STATUS_VERIFY_FAIL - Indicates that the verification of the
*                                operation failed (after copy, values at
*                                source and destination did not match).
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_mem_memory_copy_32_bit(DBG_ENG_MEM_CB *      p_dbg_eng_mem,
                                                 UINT                  copy_size,
                                                 VOID *                p_src,
                                                 VOID *                p_dst,
                                                 UINT *                p_actual_copy_size)
{
    DBG_STATUS      dbg_status;
    UINT            copy_remaining;
    UINT32 *        p_copy;
    UINT32 *        p_buffer;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Setup for copy operation. */
    
    copy_remaining = copy_size;
    p_copy = (UINT32 *)p_src;
    p_buffer = (UINT32 *)p_dst;
    
    /* Reset the operation status to allow operation (in case it was 
       cancelled on a previous operatoin). */

    p_dbg_eng_mem -> op_ok = NU_TRUE;    
    
    /* Perform copy operation. */
    
    while ((dbg_status == DBG_STATUS_OK) &&
           (copy_remaining > 0))
    {
        /* Read using appropriate memory access. */
        
        /* Perform memory access. */
        
        *p_buffer = *p_copy;
        
        /* The previous operation could cause an exception so
           check the status of the operation to determine if the
           operation should continue. */
           
        if (p_dbg_eng_mem -> op_ok == NU_FALSE)
        {
            /* The operation has been cancelled. */
            
            dbg_status = DBG_STATUS_INVALID_OPERATION;
        
        }
        else
        {
            /* Validate the copy. */
            
            if (*p_buffer != *p_copy)
            {
                /* ERROR: Validation of copy failed. */
                
                dbg_status = DBG_STATUS_VERIFY_FAIL;
                
            } 
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update the buffer address. */
                
                p_buffer++;
    
                /* Update copy address. */
                
                p_copy++;                
                
                /* Update copy remaining bytes. */
                
                copy_remaining -= sizeof(UINT32);
        
            }             
            
        } 
        
    } 
    
    /* Calculate the actual copy size.  Normally this will be the
       copy size parameter passed in, but it is possible that not
       all memory was copied in cases where the amount of data to
       copy was not evenly divisible by the access size. */
    
    *p_actual_copy_size = (copy_size - copy_remaining);    

    return (dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_MEM_Initialize
*
*   DESCRIPTION
*
*       Initialize debug memory component.
*
*   INPUTS
*
*       p_dbg_eng_mem - Pointer to the control block.
*
*       pDbgEng - Pointer to the debug control block.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Other (internal) error.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_MEM_Initialize(DBG_ENG_MEM_CB *              p_dbg_eng_mem,
                                   DBG_ENG_MEM_INIT_PARAM *      p_param)
{
    DBG_STATUS      dbg_status;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Set pointer to debug engine. */

    p_dbg_eng_mem -> p_dbg_eng = p_param -> p_dbg_eng;

    /* Set initial operation proceed value. */
    
    p_dbg_eng_mem -> op_ok = NU_TRUE;

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_MEM_Terminate
*
*   DESCRIPTION
*
*       Terminate debug memory component.
*
*   INPUTS
*
*       p_dbg_eng_mem - Pointer to the control block.
*
*       p_param - Pointer to the parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_MEM_Terminate(DBG_ENG_MEM_CB *              p_dbg_eng_mem,
                                  DBG_ENG_MEM_TERM_PARAM *      p_param)
{
    /* Nothing to do here! */
    
    return (DBG_STATUS_OK);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_MEM_Memory_Copy
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function copies memory.  Note that since this memory copy
*       operation is in support of debugging it is critical that the
*       memory copied must be "consistent".  That is to say, the memory
*       copied must be a complete snapshot and cannot tolerate some of the
*       memory being changed during the operation.  To enforce this
*       protection is employed.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_mem - Pointer to control block.
*
*       p_param - Pointer to the parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates the operation could not
*                                      be completed.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS DBG_ENG_MEM_Memory_Copy(DBG_ENG_MEM_CB *           p_dbg_eng_mem,
                                   DBG_ENG_MEM_COPY_PARAM *   p_param)
{
    DBG_STATUS      dbg_status;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Determine how to proceed based on the memory access mode. */
        
    switch (p_param -> access_mode)
    {
        case DBG_MEM_ACCESS_MODE_ANY :
        {
            /* Use optimal copy. */
            
            dbg_status = dbg_eng_mem_memory_copy_8_bit(p_dbg_eng_mem,
                                                       p_param -> copy_size,
                                                       p_param -> p_src,
                                                       p_param -> p_dst,
                                                       p_param -> p_actual_copy_size);
            
            break;
            
        } /* case */
    
        case DBG_MEM_ACCESS_MODE_8_BIT :
        {
            /* Use 1-byte copy. */
            
            dbg_status = dbg_eng_mem_memory_copy_8_bit(p_dbg_eng_mem,
                                                       p_param -> copy_size,
                                                       p_param -> p_src,
                                                       p_param -> p_dst,
                                                       p_param -> p_actual_copy_size);
            
            break;            
        
        } /* case */
        
        case DBG_MEM_ACCESS_MODE_16_BIT :
        {
            dbg_status = dbg_eng_mem_memory_copy_16_bit(p_dbg_eng_mem,
                                                        p_param -> copy_size,
                                                        p_param -> p_src,
                                                        p_param -> p_dst,
                                                        p_param -> p_actual_copy_size);          
            
            break;            
        
        } /* case */
        
        case DBG_MEM_ACCESS_MODE_32_BIT :
        {
            dbg_status = dbg_eng_mem_memory_copy_32_bit(p_dbg_eng_mem,
                                                        p_param -> copy_size,
                                                        p_param -> p_src,
                                                        p_param -> p_dst,
                                                        p_param -> p_actual_copy_size);         
            
            break;            
        
        } /* case */

        case DBG_MEM_ACCESS_MODE_NONE :
        case DBG_MEM_ACCESS_MODE_64_BIT :
        default :
        {
            /* ERROR: Unsupported access mode. */
            
            dbg_status = DBG_STATUS_NOT_SUPPORTED;
        
            break;
            
        } /* default */
            
    } /* switch */
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_MEM_Operation_Cancel
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function cancels any currently active memory operation.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_mem - Pointer to control block.
*
*       p_param - Pointer to the parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS DBG_ENG_MEM_Operation_Cancel(DBG_ENG_MEM_CB *                p_dbg_eng_mem,
                                        DBG_ENG_MEM_OP_CNCL_PARAM *     p_param)
{
    DBG_STATUS      dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Cancel any current memory operations by setting the control block
       flag to indicate that any operation status is not OK. */
       
    p_dbg_eng_mem -> op_ok = NU_FALSE;
    
    return (dbg_status);
}

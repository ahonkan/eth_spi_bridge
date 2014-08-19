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
*       dbg_eng_reg.c                                      
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Registers
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C main functions source code for the 
*       component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       None       
*                                                                      
*   FUNCTIONS                                                            
*                                                                      
*       DBG_ENG_REG_Initialize
*       DBG_ENG_REG_Terminate
*       DBG_ENG_REG_Register_Read
*       DBG_ENG_REG_Register_Write
*                                        
*   DEPENDENCIES
*
*       dbg.h
*       thread_control.h
*                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"
#include "os/kernel/plus/core/inc/thread_control.h"

/***** Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_REG_Initialize
*
*   DESCRIPTION
*
*       Initialize debug register component.
*
*   INPUTS
*
*       p_dbg_eng_reg - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_PARAMETERS - Indicates parameters are invalid. 
*
*************************************************************************/
DBG_STATUS  DBG_ENG_REG_Initialize(DBG_ENG_REG_CB *             p_dbg_eng_reg,
                                   DBG_ENG_REG_INIT_PARAM *     p_param)
{
    DBG_STATUS      dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if (p_param -> p_dbg_eng == NU_NULL)
    {
        /* ERROR: Invalid parameters. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set pointer to debug engine. */
    
        p_dbg_eng_reg -> p_dbg_eng = p_param -> p_dbg_eng;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_REG_Terminate
*
*   DESCRIPTION
*
*       Terminate debug register component.
*
*   INPUTS
*
*       p_dbg_eng_reg - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_REG_Terminate(DBG_ENG_REG_CB *              p_dbg_eng_reg,
                                  DBG_ENG_REG_TERM_PARAM *      p_param)
{
    /* Nothing to do! */

    return (DBG_STATUS_OK);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_REG_Register_Read
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function reads a register.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_reg - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates the target thread is 
*                                   invalid.
*
*       DBG_STATUS_INVALID_ID - Indicates an invalid register ID value.
*
*       DBG_STATUS_FAILED - Indicates operation failed.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_ENG_REG_Register_Read(DBG_ENG_REG_CB *          p_dbg_eng_reg,
                                      DBG_ENG_REG_READ_PARAM *  p_param)
{
    DBG_STATUS                  dbg_status;
    DBG_OS_REG_ID               os_reg_id;
    DBG_OS_REG_CMD_PARAM        os_reg_cmd_param;   
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    if ((p_param -> p_os_thread == NU_NULL) ||
        (p_param -> p_os_thread -> tc_id != TC_TASK_ID))
    {
        /* ERROR: Target thread is invalid */
        
        dbg_status = DBG_STATUS_INVALID_THREAD;
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Translate the debug service API register ID to an OS register
           ID value. */

        switch (p_param -> reg_id)
        {
            case DBG_REGISTER_ID_ALL :
            {
                os_reg_id = DBG_OS_REG_ID_ALL; 
                
                break;
                
            }    
            
            case DBG_REGISTER_ID_EXPEDITE :
            {
                os_reg_id = DBG_OS_REG_ID_EXPEDITE; 
                
                break;
                
            }
               
            case DBG_REGISTER_ID_NONE :
            {
                /* ERROR: Invalid register ID value. */
                
                dbg_status = DBG_STATUS_INVALID_ID;
            
                break;
                
            }
            
            default :
            {
                /* The value is a specific register. */
                
                os_reg_id = p_param -> reg_id;
                
                break;
                
            }
            
        }
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    { 
        /* Attempt to read the register. */
        
        os_reg_cmd_param.op = DBG_OS_REG_OP_READ;
        os_reg_cmd_param.op_param.read.p_stack_context = (VOID *)p_param -> p_os_thread -> tc_stack_pointer;
        os_reg_cmd_param.op_param.read.stack_context_type = DBG_OS_STACK_FRAME_TYPE_THREAD;
        os_reg_cmd_param.op_param.read.reg_id = os_reg_id;
        os_reg_cmd_param.op_param.read.p_reg_data = p_param -> p_reg_data;
        os_reg_cmd_param.op_param.read.p_actual_reg_data_size = p_param -> p_actual_reg_data_size;
        
        dbg_status = DBG_OS_Reg_Command(&os_reg_cmd_param); 
    
    }
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_REG_Register_Write
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This function writes a register.
*
*   INPUTS                                                               
*                                                              
*       p_dbg_eng_reg - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates the target thread is 
*                                   invalid.
*
*       DBG_STATUS_INVALID_ID - Indicates an invalid register ID value.
*
*       DBG_STATUS_FAILED - Indicates operation failed.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_ENG_REG_Register_Write(DBG_ENG_REG_CB *             p_dbg_eng_reg,
                                       DBG_ENG_REG_WRITE_PARAM *    p_param)
{
    DBG_STATUS                  dbg_status = DBG_STATUS_OK;
    DBG_OS_REG_ID               os_reg_id;
    DBG_OS_REG_CMD_PARAM        os_reg_cmd_param;      

    if ((p_param -> p_os_thread == NU_NULL) ||
        (p_param -> p_os_thread -> tc_id != TC_TASK_ID))
    {
        /* ERROR: Target thread is invalid */
        
        dbg_status = DBG_STATUS_INVALID_THREAD;
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Translate the debug service API register ID to an OS register
           ID value. */

        switch (p_param -> reg_id)
        {
            case DBG_REGISTER_ID_ALL :
            {
                os_reg_id = DBG_OS_REG_ID_ALL; 
                
                break;
                
            }
            
            case DBG_REGISTER_ID_NONE :
            {
                /* ERROR: Invalid register ID value. */
                
                dbg_status = DBG_STATUS_INVALID_ID;
            
                break;
                
            }
            
            default :
            {
                /* The value is a specific register. */
                
                os_reg_id = p_param -> reg_id;
                
                break;
                
            }
            
        }
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Attempt to write the register. */
        
        os_reg_cmd_param.op = DBG_OS_REG_OP_WRITE;
        os_reg_cmd_param.op_param.write.p_stack_context = (VOID *)p_param -> p_os_thread -> tc_stack_pointer;
        os_reg_cmd_param.op_param.write.stack_context_type = DBG_OS_STACK_FRAME_TYPE_THREAD;
        os_reg_cmd_param.op_param.write.reg_id = os_reg_id;
        os_reg_cmd_param.op_param.write.p_reg_data = p_param -> p_reg_data;
        
        dbg_status = DBG_OS_Reg_Command(&os_reg_cmd_param); 
    
    }
    
    return(dbg_status);
}

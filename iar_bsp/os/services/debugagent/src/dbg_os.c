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
*       dbg_os.c                                         
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - OS
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C source code for the component.                           
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       DBG_OS_cb       
*                                                                      
*   FUNCTIONS                                                            
*           
*       dbg_os_reg_mode_set
*
*       DBG_OS_Initialize
*       DBG_OS_Opc_Command
*       DBG_OS_Bkpt_Command
*       DBG_OS_Reg_Command
*       DBG_OS_Exec_Command
*       DBG_OS_Debug_Agent_Run_To
*       DBG_OS_Startup_Breakpoint_Address                      
*                                               
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       esal.h
*       nu_services.h
*       thread_control.h
*                                                      
*************************************************************************/

/***** Include files */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/core/inc/esal.h"
#include "services/nu_services.h"
#include "os/kernel/plus/core/inc/thread_control.h"

/***** External variables */

extern DBG_CB                   DBG_cb;

/* ESAL Debug Operation flag. */

extern INT                      ESAL_GE_DBG_Debug_Operation;

/***** Function prototypes */

static DBG_STATUS  dbg_os_reg_mode_set(DBG_OS_CB *             pCB,
                                       UINT8                   mode);

/***** Global variables */

/* Component control block */

DBG_OS_CB      DBG_OS_cb = {0,
                            0};

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_os_reg_mode_set          
*                                                                      
*   DESCRIPTION   
*                                                       
*       Sets a new register mode and updates the component control block
*       to use the new mode/register model.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_os - Pointer to the component control block.
*
*       mode - The new register system mode to be set.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS  dbg_os_reg_mode_set(DBG_OS_CB *             p_dbg_os,
                                       UINT8                   mode)
{
    DBG_STATUS      dbg_status;
    INT             esal_status;

    /* Set the new register mode and retrieve the register ID values for
       the PC and SP registers. */ 

    esal_status = ESAL_AR_DBG_Set_Protocol(mode,
                                          &p_dbg_os -> sp_reg_id,
                                          &p_dbg_os -> pc_reg_id);
    
    dbg_status = DBG_STATUS_FROM_ESAL_STATUS(esal_status);
    
    return (dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Initialize          
*                                                                      
*   DESCRIPTION   
*                                                       
*       Performs initialization of the PLUS OS support.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       pParam - Pointer to parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Initialize(DBG_OS_INIT_PARAM *       p_param)
{
    DBG_STATUS              dbg_status;
    DBG_OS_CB *             p_dbg_os;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;    
    
    /* Get pointer to control block. */
    
    p_dbg_os = &DBG_OS_cb;
    
    /* Set the default register mode. */ 
    
    dbg_status = dbg_os_reg_mode_set(p_dbg_os,
                                     DBG_OS_REG_MODE_RSP);
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Debug_Begin          
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function indicates the beginning of debugging support.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_param - Pointer to parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Debug_Begin(DBG_OS_DEBUG_BEGIN_PARAM *   p_param)
{
    DBG_STATUS              dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;    
    
    /* Initialize ESAL DBG component and register the software 
       breakpoint and hardware single-step LISR functions. */

    ESAL_GE_DBG_Initialize(p_param -> soft_bkpt_hdlr,
                           p_param -> hw_step_hdlr,
                           p_param -> data_abrt_hdlr);
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Debug_End          
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function indicates the end of debugging support.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_param - Pointer to parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Debug_End(DBG_OS_DEBUG_END_PARAM *   p_param)
{
    DBG_STATUS              dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Terminate the ESAL DBG component. */

    ESAL_GE_DBG_Terminate();
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Debug_Operation_Begin          
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function indicates the beginning of a debug operation.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       None
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Debug_Operation_Begin(void)
{
    DBG_STATUS              dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;    
    
    /* Update ESAL debug operation flag to indicate a debug operation is
       occurring. */
    
    ESAL_GE_DBG_Debug_Operation = 1;
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Debug_Operation_End          
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function indicates the end of a debug operation.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       None
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Debug_Operation_End(void)
{
    DBG_STATUS              dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;    
    
    /* Update ESAL debug operation flag to indicate a debug operation is
       occurring. */
    
    ESAL_GE_DBG_Debug_Operation = 0;
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Opc_Command          
*                                                                      
*   DESCRIPTION   
*                                                       
*       Performs an Op Code command.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_param - Pointer to the parameters.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_ADDRESS - Indicates an invalid address was
*                                    returned from a get breakpoint
*                                    address operation or the parameter
*                                    structure pointer was invalid.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates an invalid operation was
*                                      attempted.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Opc_Command(DBG_OS_OPC_CMD_PARAM *  p_param)
{
    DBG_STATUS              dbg_status;
    INT                     esal_status;
    DBG_OS_CB *             p_dbg_os;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Get pointer to control block. */
    
    p_dbg_os = &DBG_OS_cb;    
    
    /* Ensure that the parameter is valid. */
    
    dbg_status = DBG_STATUS_FROM_NULL_PTR_CHECK(p_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine how to proceed based on the operation. */
        
        switch (p_param -> op)
        {
            case DBG_OS_OPC_OP_GET_NEXT_INST_PTR :
            {
                ESAL_GE_DBG_REG     pc;
                
                /* Retrieve the value of the PC register value from the
                   specified stack frame. */                
                
                esal_status = ESAL_AR_DBG_Reg_Read(p_param -> op_param.get_next_inst_ptr.p_stack_context,
                                                   p_param -> op_param.get_next_inst_ptr.stack_context_type,
                                                   p_dbg_os -> pc_reg_id,
                                                   &pc);
                
                if (esal_status == NU_TRUE)
                {
                    /* Convert the PC value to a pointer for return. */
                    
                    p_param -> op_param.get_next_inst_ptr.p_next_inst = (VOID *)pc; 
                    
                }
                else
                {
                    /* ERROR: Register not available. */
                    
                    dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                    
                }                        
                
                break;
                
            }         
            
            case DBG_OS_OPC_OP_READ :
            {
                p_param -> op_param.read.op_code = (UINT)ESAL_GE_DBG_Opcode_Read((VOID *)p_param -> op_param.read.p_address);             
                
                break;
                
            }          
            
            case DBG_OS_OPC_OP_WRITE :            
            {
                ESAL_GE_DBG_Opcode_Write((VOID *)p_param -> op_param.write.p_address, 
                                         (ESAL_GE_DBG_OPCODE)p_param -> op_param.write.op_code);            
                
                break;
                
            }          

            case DBG_OS_OPC_OP_GET_BKPT_VALUE :
            {
                p_param -> op_param.get_bkpt_value.bkpt_value = (UINT)ESAL_GE_DBG_Opcode_Brk_Get((VOID *)p_param -> op_param.get_bkpt_value.p_address);
                
                break;
                
            }           
            
            case DBG_OS_OPC_OP_GET_NOP_VALUE :
            {
                p_param -> op_param.get_nop_value.nop_value = (UINT)ESAL_GE_DBG_Opcode_Nop_Get((VOID *)p_param -> op_param.get_nop_value.p_address);
                
                break;
                
            }             
            
            case DBG_OS_OPC_OP_GET_BKPT_ADDRESS :             
            {
                VOID *              p_bkpt_address;
                
                p_bkpt_address = (VOID *)ESAL_GE_DBG_Step_Addr_Get((VOID *)p_param -> op_param.get_bkpt_address.p_next_op_code,
                                                                   (VOID *)p_param -> op_param.get_bkpt_address.p_stack_context,
                                                                   p_param -> op_param.get_bkpt_address.stack_context_type);

                if (p_bkpt_address != NU_NULL)
                {
                    p_param -> op_param.get_bkpt_address.p_bkpt_address = p_bkpt_address;
             
                }
                else
                {
                    /* ERROR: Unable to determine breakpoint
                       address. */
                       
                    dbg_status = DBG_STATUS_FAILED;
                    
                }
                     
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Ensure that the breakpoint address is valid. */
                    
                    if (ESAL_GE_MEM_ALIGNED_CHECK(p_param -> op_param.get_bkpt_address.p_bkpt_address, 2) == NU_FALSE)
                    {
                        dbg_status = DBG_STATUS_INVALID_ADDRESS;
                    
                    } 
                
                }
                
                break;
                
            }           
            
            case DBG_OS_OPC_OP_NONE :            
            default :
            {
                /* ERROR: Invalid operation. */
                
                dbg_status = DBG_STATUS_INVALID_OPERATION;
            
                break;
                
            }         
            
        } 
    
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Reg_Command          
*                                                                      
*   DESCRIPTION   
*                                                       
*       Performs a register command.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_param - Pointer to the parameters.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates operation failed.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Reg_Command(DBG_OS_REG_CMD_PARAM *  p_param)
{
    DBG_STATUS              dbg_status;
    INT                     esal_status;
    DBG_OS_CB *             p_dbg_os;
    
    /* Get pointer to control block. */
    
    p_dbg_os = &DBG_OS_cb;    
    
    /* Ensure that the parameter is valid. */
    
    dbg_status = DBG_STATUS_FROM_NULL_PTR_CHECK(p_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine how to proceed based on the operation. */
        
        switch (p_param -> op)
        {
            case DBG_OS_REG_OP_READ :
            {
                /* Read register(s) */
                
                /* Determine which registers are to be read. */
                
                switch (p_param -> op_param.read.reg_id)
                {
                    case DBG_OS_REG_ID_NONE:
                    {
                        /* ERROR: Invalid register ID value. */
                        
                        /* ERROR RECOVERY: None. */
                        
                        dbg_status = DBG_STATUS_INVALID_ID;
                        
                        break;
                        
                    } 
                                        
                    case DBG_OS_REG_ID_ALL:
                    {
                        UINT32      actualRegDataSize;
                        
                        esal_status = ESAL_AR_DBG_Reg_Block_Read(p_param -> op_param.read.p_stack_context,
                                                                 p_param -> op_param.read.stack_context_type,
                                                                 p_param -> op_param.read.p_reg_data,
                                                                 &actualRegDataSize);
                        
                        /* Check for status */
                        
                        dbg_status = DBG_STATUS_FROM_ESAL_STATUS(esal_status);
                        
                        if (dbg_status == DBG_STATUS_OK)
                        {
                            *p_param -> op_param.read.p_actual_reg_data_size = (UINT)actualRegDataSize;
                            
                        } 
                        
                        break;
                        
                    } 

                    case DBG_OS_REG_ID_EXPEDITE:
                    {
                        UINT32      actualRegDataSize;
                        
                        esal_status = ESAL_AR_DBG_Reg_Expedite_Read(p_param -> op_param.read.p_stack_context,
                                                                    p_param -> op_param.read.stack_context_type,                        
                                                                    p_param -> op_param.read.p_reg_data,
                                                                    &actualRegDataSize);
                                                                            
                        /* Check for status */
                        
                        dbg_status = DBG_STATUS_FROM_ESAL_STATUS(esal_status);
                        
                        if (dbg_status == DBG_STATUS_OK)
                        {
                            *p_param -> op_param.read.p_actual_reg_data_size = (UINT)actualRegDataSize;
                            
                        } 
                        
                        break;
                        
                    } 
                    
                    case DBG_OS_REG_ID_PC:
                    {
                        /* Retrieve pc register value. */
                        
                        esal_status = ESAL_AR_DBG_Reg_Read(p_param -> op_param.read.p_stack_context,
                                                           p_param -> op_param.read.stack_context_type,
                                                           p_dbg_os -> pc_reg_id,
                                                           p_param -> op_param.read.p_reg_data);
                        
                        if (esal_status != NU_TRUE)
                        {
                            dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                            
                        } 
                        
                        if (dbg_status == DBG_STATUS_OK)
                        {
                            *p_param -> op_param.read.p_actual_reg_data_size = sizeof(ESAL_GE_DBG_REG);
                            
                        } 
                            
                        break;
                        
                    }                 
                    
                    default:
                    {
                        /* Retrieve specified register ID value. */
                        
                        esal_status = ESAL_AR_DBG_Reg_Read(p_param -> op_param.read.p_stack_context,
                                                           p_param -> op_param.read.stack_context_type,
                                                           p_param -> op_param.read.reg_id,
                                                           p_param -> op_param.read.p_reg_data);
                                                
                        /* Check for status */
                        
                        if (esal_status != NU_TRUE)
                        {
                            dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                            
                        } 
                        
                        if (dbg_status == DBG_STATUS_OK)
                        {
                            *p_param -> op_param.read.p_actual_reg_data_size = sizeof(ESAL_GE_DBG_REG);
                            
                        }                           
                            
                        break;
                        
                    } 
                    
                } 
                
                break;
                
            }          
            
            case DBG_OS_REG_OP_WRITE :            
            {
                /* Write regsiter(s). */
                
                /* Determine which registers are to be written. */
                
                switch (p_param -> op_param.write.reg_id)
                {
                    case DBG_OS_REG_ID_NONE:
                    {
                        /* ERROR: Invalid register ID value. */
                        
                        /* ERROR RECOVERY: None. */
                        
                        dbg_status = DBG_STATUS_INVALID_ID;
                        
                        break;
                        
                    } 
                                        
                    case DBG_OS_REG_ID_ALL:
                    {
                        /* Retrieve all register values. */
                        
                        esal_status = ESAL_AR_DBG_Reg_Block_Write(p_param -> op_param.write.p_stack_context,
                                                                  p_param -> op_param.write.stack_context_type,
                                                                  p_param -> op_param.write.p_reg_data);
                        
                        dbg_status = DBG_STATUS_FROM_ESAL_STATUS(esal_status);
                        
                        break;
                        
                    } 
                    
                    case DBG_OS_REG_ID_PC:
                    {
                        /* write pc register value. */
                        
                        esal_status = ESAL_AR_DBG_Reg_Write(p_param -> op_param.write.p_stack_context,
                                                            p_param -> op_param.write.stack_context_type,
                                                            p_dbg_os -> pc_reg_id,
                                                            p_param -> op_param.write.p_reg_data);
                        
                        if (esal_status != NU_TRUE)
                        {
                            dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                            
                        } 
                                               
                        break;
                        
                    }                 
                    
                    default:
                    {
                        /* Write the specified register ID value. */
                        
                        esal_status = ESAL_AR_DBG_Reg_Write(p_param -> op_param.write.p_stack_context,
                                                            p_param -> op_param.write.stack_context_type,
                                                            p_param -> op_param.write.reg_id,
                                                            p_param -> op_param.write.p_reg_data);
                        
                        if (esal_status != NU_TRUE)
                        {
                            dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                            
                        } 
                        
                        break;                      
                        
                    } 
                    
                } 
                
                break;
                
            }          
            
            case DBG_OS_REG_OP_SET_MODE :
            {
                UINT8           mode;
                
                /* Set the register format (comm) mode based on the 
                   OS abstraction mode id value. */
                   
                switch (p_param->op_param.set_mode.mode)
                {
                    case DBG_OS_REG_MODE_RSP:
                    {
                        mode = NU_REG_RSP_MODE;
                        
                        break;
                        
                    } 
                    
                    case DBG_OS_REG_MODE_NONE:
                    default:
                    {
                        /* ERROR: Unknown register mode. */
                        
                        /* ERROR RECOVERY: None */
                        
                        dbg_status = DBG_STATUS_INVALID_ID;
                        
                        break;
                        
                    } 
                    
                } 
                
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Update the OS register mode. */
                    
                    dbg_status = dbg_os_reg_mode_set(p_dbg_os,
                                                     mode);                
                
                } 
                
                break;
                
            }          
            
            case DBG_OS_REG_OP_INT_SAVE :
            {
                DBG_OS_REG_INT_STATE    int_state;                
                
                /* Save the interrupt state in stack frame. */

                esal_status = ESAL_GE_DBG_Int_Read(p_param -> op_param.int_sav.p_stack_context,
                                                   p_param -> op_param.int_sav.stack_context_type,
                                                   &int_state);
                if (esal_status != NU_TRUE)
                {
                    /* ERROR: Unable to save interrupt state. */

                    dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                }
                else
                {
                    /* Update return parameters. */

                    *(p_param -> op_param.int_sav.p_int_state) = int_state;
                    
                }
                
                break;
                
            } 
                        
            case DBG_OS_REG_OP_INT_RESTORE :
            {
                /* Restore interrupts in stack frame. */

                esal_status = ESAL_GE_DBG_Int_Write(p_param -> op_param.int_res.p_stack_context,
                                                    p_param -> op_param.int_res.stack_context_type,
                                                    p_param -> op_param.int_res.int_state);
                if (esal_status != NU_TRUE)
                {
                    /* ERROR: Unable to restore interrupt state. */

                    dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                    
                }
                
                break;
                
            } 
                        
            case DBG_OS_REG_OP_INT_ENABLE :
            {
                /* Enable interrupts in stack frame. */

                esal_status = ESAL_GE_DBG_Int_Enable(p_param -> op_param.int_enbl.p_stack_context,
                                                     p_param -> op_param.int_enbl.stack_context_type);
                if (esal_status != NU_TRUE)
                {
                    /* ERROR: Unable to enable interrupts. */

                    dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                    
                }
                
                break;
                
            } 
                        
            case DBG_OS_REG_OP_INT_DISABLE :
            {
                /* Disable interrupts in stack frame. */

                esal_status = ESAL_GE_DBG_Int_Disable(p_param -> op_param.int_dsbl.p_stack_context,
                                                      p_param -> op_param.int_dsbl.stack_context_type);
                if (esal_status != NU_TRUE)
                {
                    /* ERROR: Unable to disable interrupts. */

                    dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                    
                }
                
                break;
                
            } 
            
            case DBG_OS_REG_OP_NONE :            
            default :
            {
                /* ERROR: Invalid (unknown) operation. */
                
                dbg_status = DBG_STATUS_INVALID_OPERATION;
            
                break;
                
            }         
            
        } 
    
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Exec_Command          
*                                                                      
*   DESCRIPTION   
*                                                       
*       Performs an execution command.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_param - Pointer to the parameters.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error. 
*                                                                      
*************************************************************************/
DBG_STATUS  DBG_OS_Exec_Command(DBG_OS_EXEC_CMD_PARAM *  p_param)
{
    DBG_STATUS              dbg_status;
    INT                     esal_status;
    STATUS                  nu_status;
    UINT32                  esal_dbg_support_flags;
    
    /* Ensure that the parameter is valid. */
    
    dbg_status = DBG_STATUS_FROM_NULL_PTR_CHECK(p_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine how to proceed based on the operation. */
        
        switch (p_param -> op)
        {
            case DBG_OS_EXEC_OP_STATUS :
            {
                /* Retrieve the status of the OS debug system. */
                
                esal_status = ESAL_GE_DBG_Get_Support_Flags(&esal_dbg_support_flags);
                
                dbg_status = DBG_STATUS_FROM_ESAL_STATUS(esal_status);
                
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Update returned status for hardware single-step
                       support. */
                       
                    p_param->op_param.status.hw_step_supported = (esal_dbg_support_flags & ESAL_GE_DBG_SUPPORT_HW_STEP) ? NU_TRUE : NU_FALSE;
                    
                } 
                
                break;
                
            }          
            
            case DBG_OS_EXEC_OP_HARDWARE_SINGLESTEP :            
            {
                /* Perform a hardware single-step. */
                
                esal_status = ESAL_GE_DBG_Hardware_Step(p_param -> op_param.hw_step.p_stack_context,
                                                        p_param -> op_param.hw_step.stack_context_type);
                
                dbg_status = DBG_STATUS_FROM_ESAL_STATUS(esal_status);
                
                break;
                
            }          
            
            case DBG_OS_EXEC_OP_DEBUG_SUSPEND:
            {
                /* Perform a debug suspend on the specified task. */
                
                nu_status = TCC_Debug_Suspend_Service((NU_TASK *)p_param -> op_param.dbg_susp.p_task);
                
                dbg_status = DBG_STATUS_FROM_NU_STATUS(nu_status);
                
                break;
                
            } 
            
            case DBG_OS_EXEC_OP_DEBUG_RESUME:
            {
                /* Perform a debug resume of the specified task. */
                
                nu_status = TCC_Debug_Resume_Service((NU_TASK *)p_param -> op_param.dbg_res.p_task);
                
                dbg_status = DBG_STATUS_FROM_NU_STATUS(nu_status);                
                
                break;
                
            }         
            
            case DBG_OS_EXEC_OP_NONE :            
            default :
            {
                /* ERROR: Invalid (unknown) operation. */
                
                dbg_status = DBG_STATUS_INVALID_OPERATION;
            
                break;
                
            }         
            
        } 
    
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Debug_Agent_Run_To         
*                                                                      
*   DESCRIPTION   
*                                                       
*       This is a weakly defined function which returns the default 
*       address for a breakpoint set during startup of the debug agent. An
*       implementation provided by the application will override this
*       function definition. 
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       None
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       Default address for debug agent startup breakpoint. 
*                                                                      
*************************************************************************/
ESAL_TS_WEAK_DEF(VOID * DBG_OS_Debug_Agent_Run_To(VOID))
{
    /* Default to start of Application_Initialize() */
    
    return(Application_Initialize);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_OS_Startup_Breakpoint_Address         
*                                                                      
*   DESCRIPTION   
*                                                       
*       Returns the address of the startup breakpoint. 
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       None
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       Default address for debug agent startup breakpoint. 
*                                                                      
*************************************************************************/
VOID * DBG_OS_Startup_Breakpoint_Address(VOID)
{
    return(DBG_OS_Debug_Agent_Run_To());
}

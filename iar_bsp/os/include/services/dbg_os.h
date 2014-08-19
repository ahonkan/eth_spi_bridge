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
*       dbg_os.h                              
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - OS
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C definitions for the component.
*
*       NOTE: Only basic Nucleus data types (nucleus.h) may be used in
*             this file as it is itself included from (the bottom of)
*             nucleus.h.
*                                                                      
*   DATA STRUCTURES                                                      
*                       
*       DBG_OS_PROT_THD_INIT_PARAM
*       DBG_OS_PROT_THD_SET_PARAM
*       DBG_OS_PROT_THD_CHK_PARAM
*       DBG_OS_PROT_HSR_SET_PARAM
*       DBG_OS_PROT_HSR_CHK_PARAM
*       DBG_OS_INIT_PARAM 
*       DBG_OS_DEBUG_BEGIN_PARAM
*       DBG_OS_DEBUG_END_PARAM                                              
*       DBG_OS_CB     
*            
*   FUNCTIONS
*
*       DBG_OS_Initialize
*       DBG_OS_Opc_Command
*       DBG_OS_Bkpt_Command
*       DBG_OS_Reg_Command
*       DBG_OS_Exec_Command
*       DBG_OS_Prot_Thread_Set
*       DBG_OS_Prot_Thread_Check
*       DBG_OS_Prot_System_Set
*       DBG_OS_Prot_System_Get
*       DBG_OS_Hook_Register
*       DBG_OS_Hook_Unregister
*       DBG_OS_Debug_Agent_Run_To
*       DBG_OS_Startup_Breakpoint_Address
*
*   DEPENDENCIES
*                                                         
*       nucleus.h
*                                                                      
*************************************************************************/

#ifndef DBG_OS_H
#define DBG_OS_H

#include "nucleus.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/***** Global defines */

/* Debug Agent Run To - Macro to be placed in an application to indicate
   where a breakpoint should be set when the debug agent starts up.  A
   value of NU_NULL indicates that no breakpoint should be set. */

#ifdef CFG_NU_OS_SVCS_DBG_ENABLE
#define DEBUG_AGENT_RUN_TO(address)     VOID * DBG_OS_Debug_Agent_Run_To(VOID){return(address);}           
#else
#define DEBUG_AGENT_RUN_TO(address)
#endif /* CFG_NU_OS_SVCS_DBG_ENABLE */

/* Debug Agent Assert - Macro used to conditionally stop application
   execution. */

#ifdef CFG_NU_OS_SVCS_DBG_ENABLE
#define DEBUG_AGENT_ASSERT(condition)   if ((condition) == 0) ESAL_GE_DBG_BREAK_EXECUTE();
#else
#define DEBUG_AGENT_ASSERT(condition)
#endif /* CFG_NU_OS_SVCS_DBG_ENABLE */

/* Stack Frame type */
   
typedef enum _dbg_os_stack_frame_type_enum
{
    DBG_OS_STACK_FRAME_TYPE_THREAD      = ESAL_GE_DBG_STACK_FRAME_TYPE_THREAD,
    DBG_OS_STACK_FRAME_TYPE_EXCEPTION   = ESAL_GE_DBG_STACK_FRAME_TYPE_EXCEPTION
    
} DBG_OS_STACK_FRAME_TYPE;

/* Op-Code type */

typedef unsigned long   DBG_OS_OPCODE;

/* Op Code Operation */

typedef enum _dbg_os_opc_op_enum
{
    DBG_OS_OPC_OP_NONE,
    DBG_OS_OPC_OP_GET_NEXT_INST_PTR,
    DBG_OS_OPC_OP_READ,
    DBG_OS_OPC_OP_WRITE,
    DBG_OS_OPC_OP_GET_BKPT_VALUE,
    DBG_OS_OPC_OP_GET_NOP_VALUE,
    DBG_OS_OPC_OP_GET_BKPT_ADDRESS
    
} DBG_OS_OPC_OP;

/* Op Code Get Next Instruction Pointer parameters */

typedef struct _dbg_os_opc_op_get_next_inst_ptr_param_struct
{
    VOID *                      p_stack_context;    /* Stack Context (frame) */
    DBG_OS_STACK_FRAME_TYPE     stack_context_type; /* Stack Context type */
    VOID *                      p_next_inst;        /* Next Instruction pointer */

} DBG_OS_OPC_OP_GET_NEXT_INST_PTR_PARAM;

/* Op Code Read parameters */

typedef struct _dbg_os_opc_op_read_param_struct
{
    VOID *                      p_address;          /* Address */
    UINT                        op_code;            /* Op Code */

} DBG_OS_OPC_OP_READ_PARAM;

/* Op Code Write parameters */

typedef struct _dbg_os_opc_op_write_param_struct
{
    VOID *                      p_address;          /* Address */
    UINT                        op_code;            /* Op Code */

} DBG_OS_OPC_OP_WRITE_PARAM;

/* Op Code Get Breakpoint Value parameters */

typedef struct _dbg_os_opc_op_get_bkpt_value_param_struct
{
    VOID *                      p_address;          /* Address */
    UINT                        bkpt_value;         /* (breakpoint value) Op Code */

} DBG_OS_OPC_OP_GET_BKPT_VALUE_PARAM;

/* Op Code Get NOP Value parameters */

typedef struct _dbg_os_opc_op_get_nop_value_param_struct
{
    VOID *                      p_address;          /* Address where NOP will be */
    UINT                        nop_value;          /* NOP Op Code value for address */

} DBG_OS_OPC_OP_GET_NOP_VALUE_PARAM;

/* Op Code Get Brekapoint Address parameters */

typedef struct _dbg_os_opc_op_get_bkpt_address_param_struct
{
    VOID *                      p_next_op_code;     /* Next Op Code pointer */
    VOID *                      p_stack_context;    /* Stack Context (frame) */ 
    DBG_OS_STACK_FRAME_TYPE     stack_context_type; /* Stack Context type */       
    VOID *                      p_bkpt_address;     /* Breakpoint Address */    

} DBG_OS_OPC_OP_GET_BKPT_ADDRESS_PARAM;

/* Op Code Command parameters */

typedef struct _dbg_os_opc_cmd_param_struct
{
    DBG_OS_OPC_OP                                   op;     /* Operation */
    
    union _dbg_os_opc_cmd_param_op_union                    /* Operation parameters */
    {
        DBG_OS_OPC_OP_GET_NEXT_INST_PTR_PARAM       get_next_inst_ptr;
        DBG_OS_OPC_OP_READ_PARAM                    read;
        DBG_OS_OPC_OP_WRITE_PARAM                   write;
        DBG_OS_OPC_OP_GET_BKPT_VALUE_PARAM          get_bkpt_value;
        DBG_OS_OPC_OP_GET_NOP_VALUE_PARAM           get_nop_value;
        DBG_OS_OPC_OP_GET_BKPT_ADDRESS_PARAM        get_bkpt_address;
    
    } op_param;

} DBG_OS_OPC_CMD_PARAM;

/* Exception */

/* OS Exception Handler function. */

typedef VOID ** (*DBG_OS_EXCP_HDLR_FUNC)(VOID * stack_ptr);

/* ESAL Exception ISR function. */

typedef VOID (*DBG_OS_ESAL_EXCP_HDLR_FUNC)(INT     except_vector_id,
                                           VOID *  stack_ptr);

/* Registers */

/* Register ID - A register identifying value. */

typedef UINT        DBG_OS_REG_ID;

/* Register ID values */

/* NOTE: The register ID parameter for ESAL register manipulation 
   functions is a UINT8 value. */

#define DBG_OS_REG_ID_MAX                      0xFF
#define DBG_OS_REG_ID_NONE                     (DBG_OS_REG_ID_MAX + 1)
#define DBG_OS_REG_ID_ALL                      (DBG_OS_REG_ID_MAX + 2)
#define DBG_OS_REG_ID_PC                       (DBG_OS_REG_ID_MAX + 3)    
#define DBG_OS_REG_ID_SP                       (DBG_OS_REG_ID_MAX + 4)
#define DBG_OS_REG_ID_EXPEDITE                 (DBG_OS_REG_ID_MAX + 5)

/* Register Mode - The mode of the register system.  This determines the
   format of data read and written by the register system.  Typcially the
   modes follow a debugging protocol-specific format. */
   
typedef enum _dbg_os_reg_mod_enum
{
    DBG_OS_REG_MODE_NONE,
    DBG_OS_REG_MODE_RSP 
    
} DBG_OS_REG_MODE;

/* Register Interrupt State - Interrupt state data used with the interrupt
   state save / restore operations. */
   
typedef UINT32      DBG_OS_REG_INT_STATE;

/* Register Operation */

typedef enum _dbg_os_reg_op_enum
{
    DBG_OS_REG_OP_NONE,
    DBG_OS_REG_OP_READ,
    DBG_OS_REG_OP_WRITE,
    DBG_OS_REG_OP_SET_MODE,
    DBG_OS_REG_OP_INT_SAVE,
    DBG_OS_REG_OP_INT_RESTORE,
    DBG_OS_REG_OP_INT_ENABLE,
    DBG_OS_REG_OP_INT_DISABLE
        
} DBG_OS_REG_OP;

/* Register Mode Set parameters */

typedef struct _dbg_os_reg_op_set_mode_param_struct
{
    DBG_OS_REG_MODE         mode;           /* Indicates the new mode to be set */
    
} DBG_OS_REG_OP_SET_MODE_PARAM;

/* Register Model Get parameters */

typedef struct _dbg_os_reg_op_get_model_param_struct
{
    VOID *                  p_model;        /* Pointer to the register model used by the OS */
    
} DBG_OS_REG_OP_GET_MODEL_PARAM;

/* Register Read parameters */

typedef struct _dbg_os_reg_op_read_param_struct
{
    VOID *                  p_stack_context;        /* Pointer to thread stack frame (context) */
    DBG_OS_STACK_FRAME_TYPE stack_context_type;     /* Stack Context type */
    DBG_OS_REG_ID           reg_id;                 /* Specifies the target register(s) */
    VOID *                  p_reg_data;             /* Returned register data */
    UINT *                  p_actual_reg_data_size; /* Returned actual register data size */

} DBG_OS_REG_OP_READ_PARAM;

/* Register Write parameters */

typedef struct _dbg_os_reg_op_write_param_struct
{
    VOID *                  p_stack_context;    /* Pointer to thread stack frame (context) */
    DBG_OS_STACK_FRAME_TYPE stack_context_type; /* Stack Context type */
    DBG_OS_REG_ID           reg_id;             /* Specifies the target register(s) */
    VOID *                  p_reg_data;         /* Register data to be written */

} DBG_OS_REG_OP_WRITE_PARAM;

/* Register Interrupt Save parameters */

typedef struct _dbg_os_reg_op_int_save_param_struct
{
    VOID *                  p_stack_context;    /* Pointer to thread stack frame (context) */
    DBG_OS_STACK_FRAME_TYPE stack_context_type; /* Stack Context type */ 
    DBG_OS_REG_INT_STATE *  p_int_state;        /* Returned saved interrupt state value */

} DBG_OS_REG_OP_INT_SAVE_PARAM;

/* Register Interrupt Restore parameters */

typedef struct _dbg_os_reg_op_int_restore_param_struct
{
    VOID *                  p_stack_context;    /* Pointer to thread stack frame (context) */
    DBG_OS_STACK_FRAME_TYPE stack_context_type; /* Stack Context type */ 
    DBG_OS_REG_INT_STATE    int_state;          /* Interrupt state data to be restored */

} DBG_OS_REG_OP_INT_RESTORE_PARAM;

/* Register Interrupt Enable parameters */

typedef struct _dbg_os_reg_op_int_enable_param_struct
{
    VOID *                  p_stack_context;    /* Pointer to thread stack frame (context) */
    DBG_OS_STACK_FRAME_TYPE stack_context_type; /* Stack Context type */ 
    
} DBG_OS_REG_OP_INT_ENABLE_PARAM;

/* Register Interrupt Disable parameters */

typedef struct _dbg_os_reg_op_int_disable_param_struct
{
    VOID *                  p_stack_context;    /* Pointer to thread stack frame (context) */
    DBG_OS_STACK_FRAME_TYPE stack_context_type; /* Stack Context type */ 
    
} DBG_OS_REG_OP_INT_DISABLE_PARAM;

/* Register Command parameters */

typedef struct _dbg_os_reg_cmd_param_struct
{
    DBG_OS_REG_OP                           op;     /* Operation */
    
    union _dbg_os_reg_cmd_op_param_union            /* Operation parameters */
    {
        DBG_OS_REG_OP_READ_PARAM                    read;
        DBG_OS_REG_OP_WRITE_PARAM                   write;      
        DBG_OS_REG_OP_SET_MODE_PARAM                set_mode;
        DBG_OS_REG_OP_GET_MODEL_PARAM               get_model;
        DBG_OS_REG_OP_INT_SAVE_PARAM                int_sav;
        DBG_OS_REG_OP_INT_RESTORE_PARAM             int_res; 
        DBG_OS_REG_OP_INT_ENABLE_PARAM              int_enbl;
        DBG_OS_REG_OP_INT_DISABLE_PARAM             int_dsbl;
    
    } op_param;

} DBG_OS_REG_CMD_PARAM;

/* Execution */

/* Execution Operation */

typedef enum _dbg_os_exec_op_enum
{
    DBG_OS_EXEC_OP_NONE,
    DBG_OS_EXEC_OP_STATUS,
    DBG_OS_EXEC_OP_HARDWARE_SINGLESTEP,
    DBG_OS_EXEC_OP_DEBUG_SUSPEND,
    DBG_OS_EXEC_OP_DEBUG_RESUME  
    
} DBG_OS_EXEC_OP;

/* Execution Status parameters */

typedef struct _dbg_os_exec_op_status_param_struct
{
    BOOLEAN                 hw_step_supported;  /* Hardware Single-Step Supported */
    
} DBG_OS_EXEC_OP_STATUS_PARAM;

/* Execution Hardware Single-Step parameters */

typedef struct _dbg_os_exec_op_hardware_singlestep_param_struct
{
    VOID *                  p_stack_context;    /* Pointer to thread stack frame (context) */ 
    DBG_OS_STACK_FRAME_TYPE stack_context_type; /* Stack Context type */    
    
} DBG_OS_EXEC_OP_HARDWARE_SINGLESTEP_PARAM;

/* Execution Debug Suspend parameters */

typedef struct _dbg_os_exec_op_debug_suspend_param_struct
{
    VOID *                  p_task;             /* Task to be placed in debug suspended */
    
} DBG_OS_EXEC_OP_DEBUG_SUSPEND_PARAM;

/* Execution Debug Resume parameters */

typedef struct _dbg_os_exec_op_debug_resume_param_struct
{
    VOID *                  p_task;             /* Task to be resumed from debug suspend */
    
} DBG_OS_EXEC_OP_DEBUG_RESUME_PARAM;

/* Execution Command parameters */

typedef struct _dbg_os_exec_cmd_param_struct
{
    DBG_OS_EXEC_OP                                  op;     /* Operation */
    
    union DBG_OS_EXEC_OP_PARAM_UNION                        /* Operation parameters */
    {
        DBG_OS_EXEC_OP_STATUS_PARAM                 status;
        DBG_OS_EXEC_OP_HARDWARE_SINGLESTEP_PARAM    hw_step;
        DBG_OS_EXEC_OP_DEBUG_SUSPEND_PARAM          dbg_susp;
        DBG_OS_EXEC_OP_DEBUG_RESUME_PARAM           dbg_res;
    
    } op_param;

} DBG_OS_EXEC_CMD_PARAM;

/* Initialize parameters */

typedef struct _dbg_os_init_param_struct
{
    VOID *                      reserved;       /* Reserved */
    
} DBG_OS_INIT_PARAM;

/* Debug Begin parameters */

typedef struct _dbg_os_debug_begin_param_struct
{
    DBG_OS_EXCP_HDLR_FUNC       soft_bkpt_hdlr; /* Software Breakpoint Handler */
    DBG_OS_EXCP_HDLR_FUNC       hw_step_hdlr;   /* Hardware Single-Step Handler */    
    DBG_OS_EXCP_HDLR_FUNC       data_abrt_hdlr; /* Data Abort Handler */
    
} DBG_OS_DEBUG_BEGIN_PARAM;

/* Debug End parameters */

typedef struct _dbg_os_debug_end_param_struct
{
    VOID *                      reserved;       /* Reserved */
    
} DBG_OS_DEBUG_END_PARAM;

/* PLUS Status */

#define DBG_STATUS_FROM_NU_STATUS(nu_status_code)     (((nu_status_code) == NU_SUCCESS) ? DBG_STATUS_OK : DBG_STATUS_FAILED)
#define DBG_STATUS_TO_NU_STATUS(dbg_status_code)      (((dbg_status_code) == DBG_STATUS_OK) ? NU_SUCCESS : NU_UNAVAILABLE)

/* ESAL Status (boolean value) */

#define DBG_STATUS_FROM_ESAL_STATUS(esal_status_code) (((esal_status_code) == NU_TRUE) ? DBG_STATUS_OK : DBG_STATUS_FAILED)
#define DBG_STATUS_TO_ESAL_STATUS(dbg_status_code)    (((dbg_status_code) == DBG_STATUS_OK) ? NU_TRUE : NU_FALSE)

/* Component control block. */

typedef struct _dbg_os_cb_struct
{
    UINT8                           pc_reg_id;                  /* ID value of the PC register. */
    UINT8                           sp_reg_id;                  /* ID value of the SP register. */
    
} DBG_OS_CB;    

/***** Global functions */

/* Initialization */

DBG_STATUS  DBG_OS_Initialize(DBG_OS_INIT_PARAM *       p_param);

DBG_STATUS  DBG_OS_Debug_Begin(DBG_OS_DEBUG_BEGIN_PARAM *   p_param);

DBG_STATUS  DBG_OS_Debug_End(DBG_OS_DEBUG_END_PARAM *   p_param);

DBG_STATUS  DBG_OS_Debug_Operation_Begin(void);

DBG_STATUS  DBG_OS_Debug_Operation_End(void);

/* Op Codes */

DBG_STATUS  DBG_OS_Opc_Command(DBG_OS_OPC_CMD_PARAM *  p_param);

/* Register */

DBG_STATUS  DBG_OS_Reg_Command(DBG_OS_REG_CMD_PARAM *  p_param);

/* Execution */

DBG_STATUS  DBG_OS_Exec_Command(DBG_OS_EXEC_CMD_PARAM *  p_param);    

/* Startup Breakpoint */

NU_WEAK_REF(VOID * DBG_OS_Debug_Agent_Run_To(VOID));

VOID * DBG_OS_Startup_Breakpoint_Address(VOID);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* DBG_OS_H */

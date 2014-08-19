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
*       dbg_eng_bkpt.h                                    
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Breakpoint
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the declarations for the component.                        
*                                                                      
*   DATA STRUCTURES                                                      
*                 
*       DBG_ENG_ADDRESS_BREAKPOINT_STATE
*       DBG_ENG_ADDRESS_BREAKPOINT_TYPE
*       DBG_ENG_ADDRESS_BREAKPOINT
*       DBG_ENG_CONTEXT_BREAKPOINT
*       DBG_ENG_BKPT_CONTROL_ID
*       DBG_ENG_BKPT_INIT_PARAM
*       DBG_ENG_BKPT_TERM_PARAM
*       DBG_ENG_BKPT_CONTROL_ID_ILLEGAL_INST
*       DBG_ENG_BKPT_CONTROL_PARAM
*       DBG_ENG_BKPT_INTERRUPT_TYPE
*       DBG_ENG_BKPT_CB
*       DBG_ENG_BKPT_SET_PARAM
*       DBG_ENG_BKPT_SET_SKIP_STEP_PARAM
*       DBG_ENG_BKPT_CLEAR_PARAM  
*       DBG_ENG_BKPT_RMV_TEMP_PARAM 
*       DBG_ENG_BKPT_RMV_ALL_PARAM
*            
*   FUNCTIONS
*
*       DBG_ENG_BKPT_Initialize
*       DBG_ENG_BKPT_Terminate
*       DBG_ENG_BKPT_Control
*       DBG_ENG_BKPT_Breakpoint_Set_Skip_Step
*       DBG_ENG_BKPT_Breakpoint_Set
*       DBG_ENG_BKPT_Breakpoint_Clear
*       DBG_ENG_BKPT_Breakpoints_Remove_Temporary
*       DBG_ENG_BKPT_Breakpoints_Remove_All
*       DBG_ENG_BKPT_Breakpoint_List_Get_First
*       DBG_ENG_BKPT_Breakpoint_List_Get_Next
*       DBG_ENG_BKPT_Breakpoint_List_Get_Information
*       DBG_ENG_BKPT_Software_Breakpoint_Handler
*       DBG_ENG_BKPT_Hardware_Singlestep_Handler
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_ENG_BKPT_H
#define DBG_ENG_BKPT_H

/***** Global defines */

/* Brekapoint Evaluation Function Parameter - Breakpoint function 
   parameter type. */

typedef VOID *  DBG_ENG_BKPT_EVAL_FUNC_PARAM;

/* Breakpoint Evalutation Function - Breakpoint function type. */

typedef INT (*DBG_ENG_BKPT_EVAL_FUNC)(DBG_ENG_BKPT_EVAL_FUNC_PARAM  param);

/* Breakpoint Evalutation Function values */

#define DBG_ENG_BKPT_EVAL_FUNC_NONE                         NU_NULL
#define DBG_ENG_BKPT_EVAL_FUNC_PARAM_NONE                   NU_NULL

/* Brekapoint Hit Function Parameter - Breakpoint function parameter type. */

typedef VOID *  DBG_ENG_BKPT_HIT_FUNC_PARAM;

/* Breakpoint Hit Function - Breakpoint function type. */

typedef INT (*DBG_ENG_BKPT_HIT_FUNC)(DBG_THREAD_ID                  execCtxtID,
                                     DBG_BREAKPOINT_ID              bkptID,
                                     DBG_ENG_BKPT_HIT_FUNC_PARAM    param);

/* Breakpoint Hit Function values */

#define DBG_ENG_BKPT_HIT_FUNC_NONE                          NU_NULL
#define DBG_ENG_BKPT_HIT_FUNC_PARAM_NONE                    NU_NULL

/* Breakpoint Type - These are the various types of breakpoints 
   used in the system.  Each type as a 'normal' and 'inactive' value. */

typedef enum _dbg_eng_breakpoint_type_enum
{
    DBG_ENG_BREAKPOINT_TYPE_NONE,
    DBG_ENG_BREAKPOINT_TYPE_ACTIVE,
    DBG_ENG_BREAKPOINT_TYPE_SKIPOVER,   
    DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP,    
    DBG_ENG_BREAKPOINT_TYPE_HIT,   
    DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP
    
} DBG_ENG_BREAKPOINT_TYPE;

/* Address Breakpoint State */

typedef enum _dbg_eng_address_breakpoint_state_enum
{
    DBG_ENG_ADDRESS_BREAKPOINT_STATE_NONE,
    DBG_ENG_ADDRESS_BREAKPOINT_STATE_INACTIVE,
    DBG_ENG_ADDRESS_BREAKPOINT_STATE_ACTIVE

} DBG_ENG_ADDRESS_BREAKPOINT_STATE;

/* Address Breakpoint Type */

typedef enum _dbg_eng_address_breakpoint_type_enum
{
    DBG_ENG_ADDRESS_BREAKPOINT_TYPE_NONE,
    DBG_ENG_ADDRESS_BREAKPOINT_TYPE_ISR

} DBG_ENG_ADDRESS_BREAKPOINT_TYPE;

/* Address Breakpoint */

typedef struct _dbg_eng_address_breakpoint_struct
{
    struct _dbg_eng_address_breakpoint_struct *     prev;           /* Previous breakpoint in a list */  
    struct _dbg_eng_address_breakpoint_struct *     next;           /* Next brekapoint in a list */  
    DBG_OS_OPCODE *                                 address;        /* Address of breakpoint */     
    DBG_OS_OPCODE                                   store;          /* Stored original op-code */    
    DBG_SET                                         ctxt_bkpt_set;  /* Set of context breakpoints */
    DBG_ENG_ADDRESS_BREAKPOINT_STATE                state;          /* State of address breakpoint */       
    DBG_ENG_ADDRESS_BREAKPOINT_TYPE                 type;           /* Type of address breakpoint */
    
} DBG_ENG_ADDRESS_BREAKPOINT;

/* Context Breakpoint - This structure contains information for a context
   type breakpoint.  Multiple context breakpoints may be linked to a
   single address breakpoint. */

typedef struct _dbg_eng_context_breakpoint_struct
{
    DBG_SET_NODE                                    set_node;           /* Set node */
    DBG_THREAD_ID                                   exec_ctxt_id;       /* Execution context ID */
    VOID *                                          exec_ctxt_os_data;  /* Execution context OS data */    
    DBG_THREAD_ID                                   susp_exec_ctxt_id;  /* Suspend execution context ID */
    DBG_THREAD_ID                                   aux_exec_ctxt_id;   /* Auxillary execution context ID */    
    DBG_ENG_BKPT_EVAL_FUNC                          eval_func;          /* Evalutation function */   
    DBG_ENG_BKPT_EVAL_FUNC_PARAM                    eval_func_param;    /* Evalutation function parameter */
    DBG_ENG_BKPT_HIT_FUNC                           hit_func;           /* Hit function */   
    DBG_ENG_BKPT_HIT_FUNC_PARAM                     hit_func_param;     /* Hit function parameter */     
    UINT                                            pass_count;         /* Pass count */
    UINT                                            pass_cycle;         /* Pass cycle */    
    DBG_ENG_BREAKPOINT_TYPE                         type;               /* Type */    
    struct _dbg_eng_context_breakpoint_struct *     p_aux_ctxt_bkpt;    /* Auxillary context breakpoint */   
    struct _dbg_eng_context_breakpoint_struct *     p_twin_ctxt_bkpt;   /* Twin context breakpoint */     
    DBG_ENG_ADDRESS_BREAKPOINT *                    p_addr_bkpt;        /* Address breakpoint */  
    BOOLEAN                                         int_state_restore;  /* Interrupt state restore */
    DBG_OS_REG_INT_STATE                            int_state;          /* Interrupt state */
    UINT                                            hit_count;          /* Hit count */    
    
} DBG_ENG_CONTEXT_BREAKPOINT;

/* Breakpoint System Control ID - ID value of a control item. */

typedef enum _dbg_eng_bkpt_control_id_enum
{
    DBG_ENG_BKPT_CONTROL_ID_NONE,
    DBG_ENG_BKPT_CONTROL_ID_ILLEGAL_INST
    
} DBG_ENG_BKPT_CONTROL_ID;

/* Breakpoint - Initialize */

typedef struct _dbg_eng_bkpt_init_param_struct
{
    VOID *                          p_dbg_eng;              /* Pointer to debug engine */
    
} DBG_ENG_BKPT_INIT_PARAM;

/* Breakpoint - Terminate */

typedef struct _dbg_eng_bkpt_term_param_struct
{
    VOID *                          reserved;               /* Reserved for future development */
    
} DBG_ENG_BKPT_TERM_PARAM;

/* Breakpoint System Control - Illegal Instruction parameters */

typedef struct _dbg_eng_bkpt_control_id_ill_inst_param_struct
{
    BOOLEAN                         stop_thread;            /* Stop only hit thread or application */
    BOOLEAN                         replaced_with_bkpt;     /* Replace illegal instruction with breakpoint or not. */    
    DBG_ENG_BKPT_HIT_FUNC           hit_func;               /* Hit Function */   
    DBG_ENG_BKPT_HIT_FUNC_PARAM     hit_func_param;         /* Hit Function Parameter */    
    
} DBG_ENG_BKPT_CONTROL_ID_ILL_INST_PARAM;

/* Breakpoint System Control - Breakpoint system control parameters */

typedef struct _dbg_eng_bkpt_control_param_struct
{
    DBG_ENG_BKPT_CONTROL_ID                         id;      /* ID of control item to modify */
    
    union _dbg_eng_bkpt_control_param_union
    {
        DBG_ENG_BKPT_CONTROL_ID_ILL_INST_PARAM      ill_inst;
    
    } id_param;                                             /* Parameters */
    
} DBG_ENG_BKPT_CONTROL_PARAM;

/* Breakpoint System Control Block. */

typedef struct _dbg_eng_bkpt_cb_struct
{
    VOID *                          p_dbg_eng;                  /* Pointer to the debug engine control block. */
    DBG_ENG_ADDRESS_BREAKPOINT      addr_bkpts[CFG_NU_OS_SVCS_DBG_BREAKPOINT_MAX]; /* Array of address bkpts */
    DBG_ENG_ADDRESS_BREAKPOINT *    p_all_addr_bkpts;           /* List of all (active) address bkpts */
    DBG_ENG_ADDRESS_BREAKPOINT *    p_free_addr_bkpts;          /* List of free address bkpts */ 
    DBG_ENG_CONTEXT_BREAKPOINT      ctxt_bkpts[CFG_NU_OS_SVCS_DBG_BREAKPOINT_MAX]; /* Array of context bkpts */
    DBG_SET                         free_ctxt_bkpts_set;        /* Set of free context bkpts */     
    BOOLEAN                         ill_inst_stops_thd;         /* Illegal Instruction Stops Thread? */
    BOOLEAN                         ill_inst_replace;           /* Illegal Instruction Replace? */
    DBG_ENG_BKPT_HIT_FUNC           ill_inst_hit_func;          /* Illegal Instruction Hit Function */   
    DBG_ENG_BKPT_HIT_FUNC_PARAM     ill_inst_hit_func_param;    /* Illegal Instruction Hit Function Parameter */    
    BOOLEAN                         hw_step_supported;          /* Hardware Single-Step Supported */
    DBG_SET                         hw_step_ctxt_bkpt_set;      /* Hardware Single-step Context bkpts set */    

} DBG_ENG_BKPT_CB;

/* Breakpoint Set parameters */

typedef struct _dbg_eng_bkpt_set_param_struct
{
    DBG_THREAD_ID                   hit_exec_ctxt_id;           /* Hit Execution Context ID */
    DBG_THREAD_ID                   stop_exec_ctxt_id;          /* Stop Execution Context ID */     
    DBG_OS_OPCODE *                 p_addr;                     /* Address of the breakpoint */
    DBG_ENG_BKPT_EVAL_FUNC          eval_func;                  /* Evalucation Function */
    DBG_ENG_BKPT_EVAL_FUNC_PARAM    eval_func_param;            /* Evalucation Function Parameter */
    DBG_ENG_BKPT_HIT_FUNC           hit_func;                   /* Hit Function */
    DBG_ENG_BKPT_HIT_FUNC_PARAM     hit_func_param;             /* Hit Function Parameter */
    UINT                            pass_count;                 /* Pass Count */
    UINT                            pass_cycle;                 /* Pass Cycle */
    UINT                            hit_count;                  /* Hit Count */

} DBG_ENG_BKPT_SET_PARAM;

/* Breakpoint Set Skip Step parameters */

typedef struct _dbg_eng_bkpt_set_skip_step_param_struct
{
    NU_TASK *                       p_os_thread;                /* Target OS thread */ 
    VOID *                          p_stack_frame;              /* Stack frame */
    DBG_OS_STACK_FRAME_TYPE         stack_frame_type;           /* Stack frame type */    
    DBG_THREAD_ID                   step_exec_ctxt_id;          /* Step execution context ID */
    DBG_THREAD_ID                   stop_exec_ctxt_id;          /* Stop execution context ID */
    DBG_ENG_BKPT_HIT_FUNC           hit_func;                   /* Hit function */
    DBG_ENG_BKPT_HIT_FUNC_PARAM     hit_func_param;             /* Hit function parameters */

} DBG_ENG_BKPT_SET_SKIP_STEP_PARAM;

/* Breakpoint Clear parameters */

typedef struct _dbg_eng_bkpt_clear_param_struct
{
    DBG_THREAD_ID                   hit_exec_ctxt_id;           /* Hit Execution Context ID */
    DBG_OS_OPCODE *                 p_addr;                     /* Address of the breakpoint */

} DBG_ENG_BKPT_CLEAR_PARAM;

/* Breakpoints Remove Temporary parameters */

typedef struct _dbg_eng_bkpt_rmv_temp_param_struct
{
    NU_TASK *                       p_os_thread;                /* Target OS thread */

} DBG_ENG_BKPT_RMV_TEMP_PARAM;

/* Breakpoints Remove All parameters */

typedef struct _dbg_eng_bkpt_rmv_all_param_struct
{
    VOID *                          reserved;                   /* Reserved for future development */
    
} DBG_ENG_BKPT_RMV_ALL_PARAM;

/***** Global functions */

DBG_STATUS  DBG_ENG_BKPT_Initialize(DBG_ENG_BKPT_CB *           p_dbg_eng_bkpt,
                                    DBG_ENG_BKPT_INIT_PARAM *   p_param);

DBG_STATUS  DBG_ENG_BKPT_Terminate(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                   DBG_ENG_BKPT_TERM_PARAM *    p_param);
                                   
DBG_STATUS DBG_ENG_BKPT_Control(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                DBG_ENG_BKPT_CONTROL_PARAM *  p_param);

DBG_STATUS DBG_ENG_BKPT_Breakpoint_Set_Skip_Step(DBG_ENG_BKPT_CB *                      p_dbg_eng_bkpt,
                                                 DBG_ENG_BKPT_SET_SKIP_STEP_PARAM *     p_param);

DBG_STATUS DBG_ENG_BKPT_Breakpoint_Set(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                       DBG_ENG_BKPT_SET_PARAM *       p_param);

DBG_STATUS DBG_ENG_BKPT_Breakpoint_Clear(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                         DBG_ENG_BKPT_CLEAR_PARAM *   p_param);

DBG_STATUS DBG_ENG_BKPT_Breakpoints_Remove_Temporary(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                     DBG_ENG_BKPT_RMV_TEMP_PARAM *  p_param);

DBG_STATUS DBG_ENG_BKPT_Breakpoints_Remove_All(DBG_ENG_BKPT_CB *                p_dbg_eng_bkpt,
                                               DBG_ENG_BKPT_RMV_ALL_PARAM *     p_param);

VOID ** DBG_ENG_BKPT_Software_Breakpoint_Handler(VOID *   p_excp_stack_frame);

VOID ** DBG_ENG_BKPT_Hardware_Singlestep_Handler(VOID *   p_excp_stack_frame);

#endif /* DBG_ENG_BKPT_H */

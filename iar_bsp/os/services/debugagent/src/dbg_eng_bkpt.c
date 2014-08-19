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
*       dbg_eng_bkpt.c                                     
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Breakpoint
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C main functions source code for the 
*       component.
*
*       All hope abandon ye who enter here...                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       None       
*                                                                      
*   FUNCTIONS                                                            
*           
*       dbg_eng_bkpt_breakpoint_log_begin
*       dbg_eng_bkpt_breakpoint_log_end
*       dbg_eng_bkpt_context_breakpoint_create
*       dbg_eng_bkpt_context_breakpoint_delete
*       dbg_eng_bkpt_context_breakpoint_set_find        
*       dbg_eng_bkpt_address_breakpoint_list_count
*       dbg_eng_bkpt_address_breakpoint_list_find_address
*       dbg_eng_bkpt_address_breakpoint_list_find_breakpoint
*       dbg_eng_bkpt_address_breakpoint_list_add
*       dbg_eng_bkpt_address_breakpoint_list_remove
*       dbg_eng_bkpt_address_breakpoint_activate
*       dbg_eng_bkpt_address_breakpoint_deactivate
*       dbg_eng_bkpt_address_breakpoint_initialize
*       dbg_eng_bkpt_address_breakpoint_create
*       dbg_eng_bkpt_address_breakpoint_remove
*       dbg_eng_bkpt_breakpoints_erase_references
*       dbg_eng_bkpt_breakpoint_create
*       dbg_eng_bkpt_breakpoint_remove
*       dbg_eng_bkpt_breakpoint_handler_update_related
*       dbg_eng_bkpt_breakpoint_handler_suspend_execution
*       dbg_eng_bkpt_breakpoint_handler_handle_normal
*       dbg_eng_bkpt_breakpoint_handler_software_thread
*       dbg_eng_bkpt_breakpoint_handler_software_other
*       dbg_eng_bkpt_breakpoint_handler_software
*       dbg_eng_bkpt_breakpoint_handler_hardware_singlestep
*       dbg_eng_bkpt_breakpoint_handler_hardware
*       dbg_eng_bkpt_breakpoint_setup_skipover
*       dbg_eng_bkpt_breakpoint_setup_skipover_crosslink
*       dbg_eng_bkpt_breakpoint_find
*
*       DBG_ENG_BKPT_Initialize
*       DBG_ENG_BKPT_Terminate
*       DBG_ENG_BKPT_Control
*       DBG_ENG_BKPT_Breakpoint_Set_Skipover
*       DBG_ENG_BKPT_Breakpoint_Set_Singlestep
*       DBG_ENG_BKPT_Breakpoint_Set
*       DBG_ENG_BKPT_Breakpoint_Clear
*       DBG_ENG_BKPT_Breakpoints_Remove_Temporary
*       DBG_ENG_BKPT_Breakpoints_Remove_All
*       DBG_ENG_BKPT_Breakpoint_List_Get_First
*       DBG_ENG_BKPT_Breakpoint_List_Get_Next
*       DBG_ENG_BKPT_Breakpoint_Get_Information
*       DBG_ENG_BKPT_Software_Breakpoint_Handler
*       DBG_ENG_BKPT_Hardware_Singlestep_Handler
*                                        
*   DEPENDENCIES
*
*       dbg.h
*       error_management.h
*       thread_control.h
*                                                      
*************************************************************************/

/***** Include files */

#include "services/dbg.h"
#include "os/kernel/plus/supplement/inc/error_management.h"
#include "os/kernel/plus/core/inc/thread_control.h"

/***** External variables */

extern DBG_CB *                 DBG_p_cb;
extern DBG_ENG_CB *             DBG_ENG_p_cb;
extern void * volatile          TCD_Current_Thread;

#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE
extern UINT32 DBG_Trace_Mask;
#endif /* CFG_NU_OS_SVCS_TRACE_ENABLE */

/***** Local functions */

/* Local function declarations */

static DBG_STATUS dbg_eng_bkpt_context_breakpoint_create(DBG_ENG_BKPT_CB *                 p_dbg_eng_bkpt,
                                                         DBG_THREAD_ID                     exec_ctxt_id,
                                                         VOID *                            exec_ctxt_os_data,
                                                         DBG_ENG_BREAKPOINT_TYPE           type,
                                                         DBG_THREAD_ID                     susp_exec_ctxt_id,
                                                         DBG_ENG_BKPT_EVAL_FUNC            eval_func,
                                                         DBG_ENG_BKPT_EVAL_FUNC_PARAM      eval_func_param,
                                                         DBG_ENG_BKPT_HIT_FUNC             hit_func,
                                                         DBG_ENG_BKPT_HIT_FUNC_PARAM       hit_funcParam,                                                   
                                                         UINT                              pass_count,
                                                         UINT                              pass_cycle,
                                                         UINT                              hit_count,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *      p_aux_ctxt_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *      p_twin_ctxt_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT *      p_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT **     p_p_ctxt_bkpt);

static DBG_STATUS dbg_eng_bkpt_context_breakpoint_delete(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt);

static DBG_ENG_CONTEXT_BREAKPOINT * dbg_eng_bkpt_context_breakpoint_set_find(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                                             DBG_SET *                      p_ctxt_bkptSet,
                                                                             NU_TASK *                      p_os_thread,
                                                                             DBG_ENG_BREAKPOINT_TYPE        bkpt_type,
                                                                             DBG_ENG_CONTEXT_BREAKPOINT *   bkpt_pointer,
                                                                             UINT *                         p_ctxt_bkpt_index);

static UINT dbg_eng_bkpt_address_breakpoint_list_count(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                       DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt_list,
                                                       UINT                          at_least);

static DBG_ENG_ADDRESS_BREAKPOINT * dbg_eng_bkpt_address_breakpoint_list_find_address(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                                                      DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt_list,
                                                                                      DBG_OS_OPCODE *          addr);
                                                                                      
static DBG_ENG_ADDRESS_BREAKPOINT * dbg_eng_bkpt_address_breakpoint_list_find_breakpoint(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                                                         DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt_list,
                                                                                         DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt);

static VOID dbg_eng_bkpt_address_breakpoint_list_add(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                     DBG_ENG_ADDRESS_BREAKPOINT **  p_p_addr_bkpt_list,
                                                     DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt);

static DBG_ENG_ADDRESS_BREAKPOINT * dbg_eng_bkpt_address_breakpoint_list_remove(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                                                DBG_ENG_ADDRESS_BREAKPOINT ** p_p_addr_bkpt_list,
                                                                                DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt);

static DBG_STATUS dbg_eng_bkpt_address_breakpoint_activate(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                           DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt);

static DBG_STATUS dbg_eng_bkpt_address_breakpoint_deactivate(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                             DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt);

static DBG_STATUS dbg_eng_bkpt_address_breakpoint_initialize(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                             DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt);

static DBG_STATUS dbg_eng_bkpt_address_breakpoint_create(DBG_ENG_BKPT_CB *                  p_dbg_eng_bkpt,
                                                         DBG_OS_OPCODE *                    address,
                                                         DBG_ENG_ADDRESS_BREAKPOINT **      p_p_addr_bkpt);

static DBG_STATUS dbg_eng_bkpt_address_breakpoint_remove(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt);

static DBG_STATUS dbg_eng_bkpt_breakpoints_erase_references(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                            DBG_ENG_CONTEXT_BREAKPOINT *   p_erase_ctxt_bkpt);

static DBG_STATUS dbg_eng_bkpt_breakpoint_create(DBG_ENG_BKPT_CB *                  p_dbg_eng_bkpt,
                                                 DBG_THREAD_ID                      exec_ctxt_id,
                                                 VOID *                             exec_ctxt_os_data,
                                                 DBG_OS_OPCODE *                    address,
                                                 DBG_ENG_BREAKPOINT_TYPE            type,                                                   
                                                 DBG_ENG_BKPT_EVAL_FUNC             eval_func,
                                                 DBG_ENG_BKPT_EVAL_FUNC_PARAM       eval_func_param,
                                                 DBG_ENG_BKPT_HIT_FUNC              hit_func,
                                                 DBG_ENG_BKPT_HIT_FUNC_PARAM        hit_funcParam,
                                                 DBG_THREAD_ID                      susp_exec_ctxt_id,
                                                 DBG_ENG_CONTEXT_BREAKPOINT *       p_aux_ctxt_bkpt,
                                                 DBG_ENG_CONTEXT_BREAKPOINT *       p_twin_ctxt_bkpt,
                                                 UINT                               pass_count,
                                                 UINT                               pass_cycle,
                                                 UINT                               hit_count,
                                                 DBG_ENG_ADDRESS_BREAKPOINT **      p_p_addr_bkpt,
                                                 DBG_ENG_CONTEXT_BREAKPOINT **      p_p_ctxt_bkpt);

static DBG_STATUS dbg_eng_bkpt_breakpoint_remove(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                 DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt,
                                                 DBG_ENG_CONTEXT_BREAKPOINT *   p_ctxt_bkpt,
                                                 UINT                           ctxt_bkpt_index);
                                                 
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_update_related(DBG_ENG_BKPT_CB *                p_dbg_eng_bkpt,
                                                                 DBG_ENG_CONTEXT_BREAKPOINT *     p_ctxt_bkpt,
                                                                 VOID *                           p_stack_frame,
                                                                 DBG_OS_STACK_FRAME_TYPE          stack_context_type);
                                                                 
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_suspend_execution(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                                    DBG_ENG_CONTEXT_BREAKPOINT *   p_ctxt_bkpt,
                                                                    NU_TASK *                      p_os_thread);

static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_handle_normal(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                                DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt,
                                                                DBG_ENG_CONTEXT_BREAKPOINT *   p_ctxt_bkpt,
                                                                BOOLEAN *                      p_suspend_exec_ctxt,
                                                                BOOLEAN *                      p_skip_over_bkpt,
                                                                BOOLEAN *                      p_remove_bkpt);

static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_software_thread(DBG_ENG_BKPT_CB *        p_dbg_eng_bkpt,
                                                                  NU_TASK *                p_os_thread,
                                                                  VOID *                   p_excp_stack_frame);

static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_software_other(DBG_ENG_BKPT_CB *        p_dbg_eng_bkpt,
                                                                 VOID *                   p_os_exec_ctxt,
                                                                 VOID *                   p_excp_stack_frame);

static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_software(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                                           VOID *                       p_os_exec_ctxt,
                                                           VOID *                       p_excp_stack_frame);
                                                                                                                      
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_hardware_singlestep(DBG_ENG_BKPT_CB *        p_dbg_eng_bkpt,
                                                                      NU_TASK *                p_os_thread,
                                                                      VOID *                   p_excp_stack_frame);

static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_hardware(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                                           VOID *                       p_os_exec_ctxt,
                                                           VOID *                       p_excp_stack_frame);
                                                           
static DBG_STATUS dbg_eng_bkpt_breakpoint_setup_skipover(DBG_ENG_BKPT_CB *                 p_dbg_eng_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT *      p_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *      p_ctxt_bkpt,
                                                         NU_TASK *                         p_os_thread,
                                                         VOID *                            p_stack_frame,
                                                         DBG_OS_STACK_FRAME_TYPE           stack_frame_type,
                                                         DBG_THREAD_ID                     exec_ctxt_id,
                                                         DBG_THREAD_ID                     susp_exec_ctxt_id,
                                                         DBG_ENG_BKPT_HIT_FUNC             hit_func,
                                                         DBG_ENG_BKPT_HIT_FUNC_PARAM       hit_func_param,
                                                         DBG_OS_OPCODE                     next_inst, 
                                                         DBG_OS_OPCODE *                   p_next_inst,
                                                         DBG_ENG_BREAKPOINT_TYPE           create_bkpt_type,
                                                         DBG_ENG_ADDRESS_BREAKPOINT **     p_p_twin_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT **     p_p_twin_ctxt_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT **     p_p_created_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT **     p_p_created_ctxt_bkpt);
                                                         
static DBG_STATUS dbg_eng_bkpt_breakpoint_setup_skipover_crosslink(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                                   DBG_ENG_CONTEXT_BREAKPOINT **   p_twin_ctxt_bkpt_array,
                                                                   DBG_ENG_CONTEXT_BREAKPOINT **   p_created_ctxt_bkpt_array,
                                                                   UINT                            bkpt_array_size);

static DBG_STATUS dbg_eng_bkpt_breakpoint_find(DBG_ENG_BKPT_CB *                p_dbg_eng_bkpt,
                                               DBG_OS_OPCODE *                  p_addr,
                                               DBG_THREAD_ID                    exec_ctxt_id,
                                               DBG_ENG_ADDRESS_BREAKPOINT **    p_p_addr_bkpt,
                                               DBG_ENG_CONTEXT_BREAKPOINT **    p_p_ctxt_bkpt,
                                               UINT *                           p_ctxt_bkpt_index);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_context_breakpoint_create
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function creates a context breakpoint.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       exec_ctxt_id - The execution context ID associated with the 
*                      breakpoint. 
*
*       exec_ctxt_os_data - Any OS data associated with the execution
*                           context.
*
*       type - The type of breakpoint to create.
*
*       susp_exec_ctxt_id - The execution context ID to be suspended when
*                           the breakpoint is hit.
*
*       eval_func - Function to be associated with the breakpoint that is 
*                   called to evaluate the situation when a breakpoint is 
*                   hit to determine if the breakpoint causes a stop of 
*                   execution.
*
*       eval_func_param - Parameters to be passed to the eval breakpoint 
*                         function.
*
*       hit_func - Function to be associated with the breakpoint that is 
*                  called when the breakpoint hits (stops).
*
*       hit_func_param- Parameters to be passed to the hit breakpoint 
*                       function.
*
*       pass_count - Value indicating the number of times a breakpoint
*                    will be passed before it hits.
*
*       pass_cycle - Reset value for pass count (once breakpoint hits).
*
*       hit_count - Value indicating the number of times a breakpoint
*                   will be hit before being removed.
*
*       p_aux_ctxt_bkpt - Pointer to an alternate breakpoint.
*
*       p_twin_ctxt_bkpt - Pointer to an alternate breakpoint in
*                          situations where there is a branch in 
*                          execution.
*
*       p_addr_bkpt - Pointer to the address breakpoint associated with
*                     the context breakpoint. 
*
*       p_p_ctxt_bkpt - Return parameter that will be updated with a
*                       pointer to the newly created context breakpoint if
*                       the operation succeeds.  If the operation fails 
*                       the value is undefined.
*                                                       
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_OUT_OF_MEMORY - Indicates there is not enough system
*                                  memory available to create the
*                                  breakpoint.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_context_breakpoint_create(DBG_ENG_BKPT_CB *                 p_dbg_eng_bkpt,
                                                         DBG_THREAD_ID                     exec_ctxt_id,
                                                         VOID *                            exec_ctxt_os_data,
                                                         DBG_ENG_BREAKPOINT_TYPE           type,
                                                         DBG_THREAD_ID                     susp_exec_ctxt_id,
                                                         DBG_ENG_BKPT_EVAL_FUNC            eval_func,
                                                         DBG_ENG_BKPT_EVAL_FUNC_PARAM      eval_func_param,
                                                         DBG_ENG_BKPT_HIT_FUNC             hit_func,
                                                         DBG_ENG_BKPT_HIT_FUNC_PARAM       hit_funcParam,                                                   
                                                         UINT                              pass_count,
                                                         UINT                              pass_cycle,
                                                         UINT                              hit_count,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *      p_aux_ctxt_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *      p_twin_ctxt_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT *      p_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT **     p_p_ctxt_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;
    
    /* Attempt to get a context breakpoint. */
    
    dbg_status = DBG_SET_Node_Remove(&p_dbg_eng_bkpt -> free_ctxt_bkpts_set,
                                     DBG_SET_LOCATION_DEFAULT,
                                     DBG_SET_NODE_ID_ANY,
                                     (DBG_SET_NODE **)&p_ctxt_bkpt);
    
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Initialize the context breakpoint. */
    
        p_ctxt_bkpt -> exec_ctxt_id = exec_ctxt_id;
        p_ctxt_bkpt -> exec_ctxt_os_data = exec_ctxt_os_data;
        p_ctxt_bkpt -> susp_exec_ctxt_id = susp_exec_ctxt_id;
        p_ctxt_bkpt -> aux_exec_ctxt_id = DBG_THREAD_ID_NONE;
        p_ctxt_bkpt -> eval_func = eval_func;
        p_ctxt_bkpt -> eval_func_param = eval_func_param;
        p_ctxt_bkpt -> hit_func = hit_func;
        p_ctxt_bkpt -> hit_func_param= hit_funcParam;
        p_ctxt_bkpt -> pass_count = pass_count;
        p_ctxt_bkpt -> pass_cycle = pass_cycle;
        p_ctxt_bkpt -> type = type;
        p_ctxt_bkpt -> p_aux_ctxt_bkpt = p_aux_ctxt_bkpt;
        p_ctxt_bkpt -> p_twin_ctxt_bkpt = p_twin_ctxt_bkpt;
        p_ctxt_bkpt -> p_addr_bkpt = p_addr_bkpt;
        p_ctxt_bkpt -> int_state_restore = NU_FALSE;
        p_ctxt_bkpt -> int_state = NU_NULL;
        p_ctxt_bkpt -> hit_count = hit_count;
        
    } 
        
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update return parameter. */
        
        *p_p_ctxt_bkpt = p_ctxt_bkpt;

    }     
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_context_breakpoint_delete
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function deletes a context breakpoint.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_ctxt_bkpt - Pointer to the context breakpoint to be deleted.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       DBG_STATUS_FAILED - Indicates unable to deallocate breakpoint.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_context_breakpoint_delete(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt)
{
    DBG_STATUS      dbg_status;

    /* Return the context breakpoint to the free context breakpoint 
       set. */
    
    dbg_status = DBG_SET_Node_Add(&p_dbg_eng_bkpt -> free_ctxt_bkpts_set,
                                  DBG_SET_LOCATION_DEFAULT,
                                  NU_NULL,
                                  (DBG_SET_NODE_ID)0,
                                  (DBG_SET_NODE *)p_ctxt_bkpt);    
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_context_breakpoint_set_find
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to find a context breakpoint in a context
*       breakpoint set based on a specified set of criteria.  Note that
*       the first matching context breakpoint in the set is returned.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_ctxt_bkptSet - Pointer to the context breakpoint set to be 
*                      searched.
*
*       p_os_thread - OS thread that will be used in the search if the
*                     value is non-NULL.
*
*       bkpt_type - Breakpoint type that will be used in the search if the
*                   value is not NONE.
*
*       bkpt_pointer - Context Breakpoint pointer that will be used
*                      to search if the value is non-NULL.  Note 
*                      that this type of search is used primarily to
*                      get the index of a context breakpoint in the
*                      set since its pointer is already known.
*
*       p_ctxt_bkpt_index - Return parameter that will contain the index
*                           of the context breakpoint in the set if a 
*                           matching context breakpoint is found.  If one
*                           is not found then the value is undefined.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       <non-NULL> - Pointer to an context breakpoint if the operation is 
*                    successful.
*
*       <NULL> - Indicates the operation failed.
*                                                                      
*************************************************************************/
static DBG_ENG_CONTEXT_BREAKPOINT * dbg_eng_bkpt_context_breakpoint_set_find(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                                             DBG_SET *                      p_ctxt_bkptSet,
                                                                             NU_TASK *                      p_os_thread,
                                                                             DBG_ENG_BREAKPOINT_TYPE        bkpt_type,
                                                                             DBG_ENG_CONTEXT_BREAKPOINT *   bkpt_pointer,
                                                                             UINT *                         p_ctxt_bkpt_index)
{
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;
    DBG_STATUS                      dbg_status;
    BOOLEAN                         os_thread_is_match;
    BOOLEAN                         bkpt_type_is_match;
    BOOLEAN                         bkpt_pointer_is_match;    
    BOOLEAN                         is_match;
    UINT                            ctxt_bkpt_index;
    
    /* Set initial match state. */
    
    is_match = NU_FALSE;
    
    /* Set initial index value. */
    
    ctxt_bkpt_index = 0;
    
    /* Attempt to get the first context breakpoint from the set. */
    
    dbg_status = DBG_SET_Node_Find(p_ctxt_bkptSet,
                                   DBG_SET_LOCATION_HEAD,
                                   DBG_SET_NODE_ID_ANY,
                                   (DBG_SET_NODE **)&p_ctxt_bkpt);
    
    while ((dbg_status == DBG_STATUS_OK) &&
           (is_match == NU_FALSE))
    {
        /* Determine if the OS thread match is required and met. */
           
        if (p_os_thread != NU_NULL)
        {
            /* Search is to include OS thread. */
        
            if ((p_ctxt_bkpt -> exec_ctxt_id == DBG_THREAD_ID_ALL) ||
                (p_ctxt_bkpt -> exec_ctxt_id == DBG_THREAD_ID_ANY))
            {
                /* Breakpoint execution context is the system.  The
                   system execution context encompasses everything and
                   so a any breakpoint should match. */
                
                os_thread_is_match = NU_TRUE;
            }
            else
            {                    
                /* Breakpoint execution context is a thread.  Use the
                   execution context OS data contained in the 
                   breakpoint to determine if the thread that the 
                   breakpoint exception occurred on is the target of 
                   the breakpoint. */
                
                if ((NU_TASK *)p_ctxt_bkpt -> exec_ctxt_os_data == p_os_thread)
                {
                    os_thread_is_match = NU_TRUE;
                    
                }
                else
                {
                    os_thread_is_match = NU_FALSE;
                    
                } 
                
            } 
    
        }
        else
        {
            /* Search does not include OS thread, so any context
               breakpoint is a match... */
            
            os_thread_is_match = NU_TRUE;
        
        } 
            
        /* Determine if the breakpoint type is required and met. */
        
        if (bkpt_type != DBG_ENG_BREAKPOINT_TYPE_NONE)
        {
            if (p_ctxt_bkpt->type == bkpt_type)
            {
                bkpt_type_is_match = NU_TRUE;
                
            }
            else
            {
                bkpt_type_is_match = NU_FALSE;
                
            } 
        
        }
        else
        {
            /* Search does not include the breakpoint type, so any context
               breakpoint is a match... */
               
            bkpt_type_is_match = NU_TRUE;
        
        } 
        
        /* Determine if the context breakpoint pointer match is required 
           and met. */
           
        if (bkpt_pointer != NU_NULL)
        { 
            if (p_ctxt_bkpt == bkpt_pointer)
            {
                bkpt_pointer_is_match = NU_TRUE;
                
            }
            else
            {
                bkpt_pointer_is_match = NU_FALSE;
                
            } 
        
        }
        else
        {
            /* Search does not include the breakpoint pointer, so any 
               context breakpoint is a match... */
               
            bkpt_pointer_is_match = NU_TRUE;
        
        } 
        
        /* Determine if a matching context breakpoint is found based on
           the search requirements. */
           
        if ((os_thread_is_match == NU_TRUE) && 
            (bkpt_type_is_match == NU_TRUE) && 
            (bkpt_pointer_is_match == NU_TRUE))
        {
            is_match = NU_TRUE;
            
        }
        else
        {
            is_match = NU_FALSE;
            
        } 
        
        if (is_match == NU_FALSE)
        {
            /* Attempt to get another context breakpoint from the set. */
            
            dbg_status = DBG_SET_Node_Find(p_ctxt_bkptSet,
                                           DBG_SET_LOCATION_NEXT_LINEAR,
                                           (DBG_SET_LOCATION_VALUE)p_ctxt_bkpt,
                                           (DBG_SET_NODE **)&p_ctxt_bkpt);       

            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update the context breakpoint index value. */
                
                ctxt_bkpt_index++;
                
            } 
            
        } 
            
    } 
            
    if (is_match == NU_TRUE)
    {
        /* A matching context breakpoint was found. */    
    
        /* Update the index return parameter. */
        
        *p_ctxt_bkpt_index = ctxt_bkpt_index;
        
    }
    else
    {
        /* No matching context breakpoint was found. */
        
        /* Update the return value to indicate no match. */
           
        p_ctxt_bkpt = NU_NULL;
        
    } 
    
    return (p_ctxt_bkpt);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_list_count
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function returns a value indicating the size of a breakpoint 
*       list or the minimum value if one is specified.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt_list - Pointer to the start of the breakpoint list to 
*                          be searched.
*
*       at_least - This parameter determines if the list contains "at 
*                 least" the number of items specified.  If this parameter
*                 is non-zero then the counting process will be exited 
*                 when the value is reached (if the list contains that 
*                 number or more).  In this case the number of items 
*                 returned will be the "at least" number.  If this 
*                 parameter is zero then all items in the list will be 
*                 counted and the number returned.  Essentially, this is a
*                 means for "early exit" from the counting process.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       Returns the number of items in the list if the "at least"
*       parameter is zero or the "at least" value if that many or more are
*       present.  In the case that there are not enough items to reach the
*       "at least" parameter, the actual count is returned.
*                                                                      
*************************************************************************/
static UINT dbg_eng_bkpt_address_breakpoint_list_count(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                       DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt_list,
                                                       UINT                          at_least)
{
    UINT                            count;
    UINT                            limit;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    
    /* Set count limit based on the "at least" parameter value. */
    
    if (at_least != 0)
    {
        limit = at_least;
    
    } 
    else
    {
        limit = ((UINT)-1);
    
    } 
    
    /* Set local breakpoint to start of the list. */

    p_addr_bkpt = p_addr_bkpt_list;
    
    /* Count items in the list holding to the limit value. */

    count = 0;
    while ((p_addr_bkpt != NU_NULL) &&
           (count < limit))
    {
        /* Move to the next breakpoint in the list. */
    
        p_addr_bkpt = p_addr_bkpt -> next;

        /* Increment the count. */
        
        count++;
    
    } 
    
    return (count);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_list_find_address
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function finds a breakpoint in a breakpoint list based on a 
*       target task and breakpoint address.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt_list - Pointer to the breakpoint list.
*
*       addr - Target breakpoint address.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       <non-NULL> - Pointer to a breakpoint if the operation is 
*                    successful.
*
*       <NULL> - Indicates the operation failed.
*                                                                      
*************************************************************************/
static DBG_ENG_ADDRESS_BREAKPOINT * dbg_eng_bkpt_address_breakpoint_list_find_address(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                                                      DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt_list,
                                                                                      DBG_OS_OPCODE *          addr)
{
    DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt;
    DBG_OS_OPCODE *           addr_or_1;
    BOOLEAN                        found;
    
    /* Calculate the target address OR'd with the value. */

    addr_or_1 = (DBG_OS_OPCODE *)((UINT)addr|1);

    /* Initialize the search result flag to FALSE. */
    
    found = NU_FALSE;

    /* Initialize breakpoint pointer to the start of the breakpoint
       list.  If the list is empty then it will be set to NULL by this
       assignment. */
 
    p_addr_bkpt = p_addr_bkpt_list; 
    
    /* Search breakpoint list starting at the root.  The end of this list
       is marked by a NULL. */

    while ((p_addr_bkpt != NU_NULL) &&
           (found == NU_FALSE))
    {
        /* Determine if the current breakpoint's address matches the
           target address exactly or matches the address OR'd with the
           value 1. */
    
        if ((addr == p_addr_bkpt -> address) || 
            (addr_or_1 == p_addr_bkpt -> address)) 
        {
            /* Found a breakpoint with the target address. */

            found = NU_TRUE;

        }
        else
        {
            /* Current breakpoint address does not match the target
               address. */
        
            /* Move to the next breakpoint. */

            p_addr_bkpt = p_addr_bkpt -> next;
            
        } 

    } 
    
    return (p_addr_bkpt);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_list_find_breakpoint
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function finds a breakpoint in a breakpoint list based on the 
*       breakpoint control block address.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt_list - Pointer to the breakpoint list.
*
*       p_addr_bkpt - Pointer to the target breakpoint control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       <non-NULL> - Pointer to a breakpoint if the operation is 
*                    successful.
*
*       <NULL> - Indicates the operation failed.
*                                                                      
*************************************************************************/
static DBG_ENG_ADDRESS_BREAKPOINT * dbg_eng_bkpt_address_breakpoint_list_find_breakpoint(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                                                         DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt_list,
                                                                                         DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt)
{
    DBG_ENG_ADDRESS_BREAKPOINT *    p_current_addr_bkpt;

    /* Initialize breakpoint pointer to the start of the breakpoint
       list.  If the list is empty then it will be set to NULL by this
       assignment. */
 
    p_current_addr_bkpt = p_addr_bkpt_list; 
    
    /* Search breakpoint list starting at the root.  The end of this list
       is marked by a NULL. */

    while ((p_current_addr_bkpt != p_addr_bkpt) &&
           (p_current_addr_bkpt != NU_NULL))
    {
        /* Current breakpoint address does not match the target
           address. */
    
        /* Move to the next breakpoint. */

        p_current_addr_bkpt = p_current_addr_bkpt -> next;
            
    } 
    
    return (p_current_addr_bkpt);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_list_add
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function adds a breakpoint to the breakpoint list.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_p_addr_bkpt_list - Pointer to Pointer to the start of the 
*                            breakpoint list to which the breakpoint is to 
*                            be added.  This will be updated.
*
*       p_addr_bkpt - Pointer to the breakpoint to be added to the list.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       None
*                                                                      
*************************************************************************/
static VOID dbg_eng_bkpt_address_breakpoint_list_add(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                     DBG_ENG_ADDRESS_BREAKPOINT **  p_p_addr_bkpt_list,
                                                     DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt)
{
    /* Add new breakpoint to the front (head) of the breakpoint list. */
    
    /* New breakpoint's next is the old root of the breakpoint list. */
    
    p_addr_bkpt->next = *p_p_addr_bkpt_list;
    
    /* The root of the breakpoint list is updated to point to new 
       breakpoint. */
    
    *p_p_addr_bkpt_list = p_addr_bkpt;
    
    /* New breakpoint's previous is NULL since it is now the front of the 
       breakpoint list. */
    
    p_addr_bkpt->prev = NU_NULL;
    
    /* Determine if the new breakpoint's next is NULL.  This would be the
       case if the breakpoints list was empty before the addition of the
       new breakpoint. */
    
    if (p_addr_bkpt->next != NU_NULL)
    {
        /* The breakpoint list contained elements before the addition of 
           the new breakpoint. */
    
        /* Update the breakpoint at the old root location to have the new
           breakpoint as it's previous. */
    
        p_addr_bkpt->next->prev = p_addr_bkpt;
        
    } 
    
    return;
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_list_remove
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to remove a breakpoint from the breakpoint 
*       list.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt_list - Pointer to pointer to the start of the 
*                         breakpoint list that the target breakpoint will 
*                         be removed from.
*
*       p_add_bkpt - Pointer to a specific breakpoint that is to be 
*                    removed from the breakpoint list.  If this value is
*                    NULL then the current head of the breakpoint list 
*                    is removed (if there is at least one breakpoint in 
*                    the list).
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       <non-NULL> - Pointer to a breakpoint if the operation is 
*                    successful.
*
*       <NULL> - Indicates the operation failed.
*                                                                      
*************************************************************************/
static DBG_ENG_ADDRESS_BREAKPOINT * dbg_eng_bkpt_address_breakpoint_list_remove(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                                                DBG_ENG_ADDRESS_BREAKPOINT ** p_p_addr_bkpt_list,
                                                                                DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt)
{
    DBG_ENG_ADDRESS_BREAKPOINT *    p_target_addr_bkpt;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt_list;
    
    /* Set local variable to breakpoint list. */

    p_addr_bkpt_list = *p_p_addr_bkpt_list;
    
    /* Determine if there is a specific target breakpoint to be removed. */

    if (p_addr_bkpt != NU_NULL)
    {
        /* A specific breakpoint has been specified for removal. */

        /* Set local variable to specified breakpoint. */

        p_target_addr_bkpt = p_addr_bkpt;

        /* Determine if the target breakpoint is the start of the
           breakpoint list. */
        
        if(p_addr_bkpt_list == p_target_addr_bkpt)  
        {
            /* Target breakpoint is the start of the breakpoint list. */
    
            /* Move breakpoint list start pointer to the next 
               breakpoint in the list. */
               
            p_addr_bkpt_list = p_target_addr_bkpt -> next;
    
            /* Determine if there are any breakpoints in the list at all
               now. */
            
            if (p_addr_bkpt_list != NU_NULL)
            {
                /* There is at least one breakpoint in the list. */
    
                /* Update the breakpoint at the start of the list to 
                   indicate it is the start by removing its previous 
                   pointer to the target breakpoint. */
            
                p_addr_bkpt_list -> prev = NU_NULL;
    
            } 
            
        }
        else
        {
            /* Target breakpoint is not the start of the breakpoint list.
               It is located somewhere else in the list. */
    
            /* Fix the previous item before the target breakpoint's next 
               pointer. */
               
            p_target_addr_bkpt -> prev -> next = p_target_addr_bkpt -> next;
    
            /* Determine if the target breakpoint was the last item in 
               the list. */
            
            if (p_target_addr_bkpt -> next != NU_NULL)
            {
                /* The target breakpoint was not the last item in the 
                   list, but was in the middle somewhere. */
    
                /* Fix the next item after the target breakpoint's
                   previous pointer. */
                   
                p_target_addr_bkpt -> next -> prev = p_target_addr_bkpt -> prev;
    
            } 
            
        } 

    }
    else
    {
        /* No specific breakpoint is specified for removal. */

        /* Set local variable to start of the breakpoint list. */

        p_target_addr_bkpt = p_addr_bkpt_list;

        /* Ensure that there is a breakpoint before continuing. */
        
        if (p_target_addr_bkpt != NU_NULL) 
        {
            /* There is a free breakpoint so update the breakpoint list 
               to point to the next breakpoint in the breakpoint list past
               the one just found. */
               
            p_addr_bkpt_list = p_target_addr_bkpt -> next;

            /* Determine if there is a breakpoint in the list now. */

            if (p_addr_bkpt_list != NU_NULL)
            {
                /* This is at least one more breakpoint in the list so
                   update it's previous value. */

                p_addr_bkpt_list -> prev = NU_NULL;
            
            } 
               
        } 

    } 

    /* Update breakpoint list with local variable value. */

    *p_p_addr_bkpt_list = p_addr_bkpt_list;
    
    return (p_target_addr_bkpt);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_activate
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function activates an address breakpoint.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt - Pointer to the address breakpoint to activate.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_address_breakpoint_activate(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                           DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_OS_OPCODE                   bkpt_op_code;
    DBG_OS_OPC_CMD_PARAM            os_opc_cmd_param;
    
    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine how to proceed based on the current activation state of
       the address breakpoint. */
       
    switch (p_addr_bkpt -> state)
    {
        case DBG_ENG_ADDRESS_BREAKPOINT_STATE_INACTIVE :
        {
            /* The address is currently inactive. */        
        
            /* Determine how to proceed based on the address breakpoint
               type. */
        
            switch (p_addr_bkpt -> type)
            {
                case DBG_ENG_ADDRESS_BREAKPOINT_TYPE_ISR :
                {
                    /* Update the address breakpoints activation state. */
                    
                    p_addr_bkpt -> state = DBG_ENG_ADDRESS_BREAKPOINT_STATE_ACTIVE;
                    
                    /* Store existing opcode at the address. */
                    
                    os_opc_cmd_param.op = DBG_OS_OPC_OP_READ;
                    os_opc_cmd_param.op_param.read.p_address = (VOID *)p_addr_bkpt -> address;
                    
                    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);                    
                    
                    if (dbg_status == DBG_STATUS_OK)
                    {
                        p_addr_bkpt -> store = (DBG_OS_OPCODE)os_opc_cmd_param.op_param.read.op_code;
                        
                        /* Determine the appropriate breakpoint value for
                           the address. */
                                                   
                        os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_BKPT_VALUE;
                        os_opc_cmd_param.op_param.get_bkpt_value.p_address = (VOID *)p_addr_bkpt -> address;
                        
                        dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);               
                
                    } 
                    
                    if (dbg_status == DBG_STATUS_OK)
                    {
                        bkpt_op_code = (DBG_OS_OPCODE)os_opc_cmd_param.op_param.get_bkpt_value.bkpt_value;
                        
                        /* Write the breakpoint opcode value appropriate
                           for the address. */
                    
                        os_opc_cmd_param.op = DBG_OS_OPC_OP_WRITE;
                        os_opc_cmd_param.op_param.write.p_address = (VOID *)p_addr_bkpt -> address;
                        os_opc_cmd_param.op_param.write.op_code = (UINT)bkpt_op_code;    
                        
                        dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);                    
                    
                    } 
                
                    break;
                    
                } 
                
                case DBG_ENG_ADDRESS_BREAKPOINT_TYPE_NONE :                
                default :
                {
                    /* ERROR: Invalid or unknown address breakpoint 
                       type. */
                
                    break;
                    
                } 
               
            } 
        
            break;
            
        } 
        
        case DBG_ENG_ADDRESS_BREAKPOINT_STATE_ACTIVE :
        case DBG_ENG_ADDRESS_BREAKPOINT_STATE_NONE :        
        default :
        {
            /* ERROR: Invalid or unknown activation state. */
        
            break;
            
        } 
    
    } 
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_deactivate
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function deactivates an address breakpoint.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt - Pointer to the address breakpoint to activate.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_address_breakpoint_deactivate(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                                             DBG_ENG_ADDRESS_BREAKPOINT *  p_addr_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_OS_OPC_CMD_PARAM            os_opc_cmd_param;   
    
    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine how to proceed based on the current activation state of
       the address breakpoint. */
       
    switch (p_addr_bkpt -> state)
    {
        case DBG_ENG_ADDRESS_BREAKPOINT_STATE_ACTIVE :
        {
            /* Determine how to proceed based on the address breakpoint
               type. */
        
            switch (p_addr_bkpt -> type)
            {
                case DBG_ENG_ADDRESS_BREAKPOINT_TYPE_ISR :
                {        
                    /* Update the address breakpoints activation state. */
                    
                    p_addr_bkpt->state = DBG_ENG_ADDRESS_BREAKPOINT_STATE_INACTIVE;
                
                    /* Write the stored opcode value. */
                    
                    os_opc_cmd_param.op = DBG_OS_OPC_OP_WRITE;
                    os_opc_cmd_param.op_param.write.p_address = (VOID *)p_addr_bkpt -> address;
                    os_opc_cmd_param.op_param.write.op_code = (UINT)p_addr_bkpt->store;    
                    
                    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);                    
                
                    break;
                    
                } 
            
                case DBG_ENG_ADDRESS_BREAKPOINT_TYPE_NONE :                
                default :
                {
                    /* ERROR: Invalid or unknown address breakpoint 
                       type. */

                    break;
                    
                } 
               
            }                 
                
        }         
        
        case DBG_ENG_ADDRESS_BREAKPOINT_STATE_INACTIVE :
        case DBG_ENG_ADDRESS_BREAKPOINT_STATE_NONE :        
        default :
        {
            /* ERROR: Invalid or unknown activation state. */
        
            break;
            
        } 
    
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_initialize
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function initializes a breakpoint.
*                                                                                                 
*   INPUTS                                                               
*                  
*       p_dbg_eng_bkpt - Pointer to breakpoint component control block.
*
*       p_addr_bkpt - Pointer to the address breakpoint to initialize.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       DBG_STATUS_FAILED - Indicates operation failed.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_address_breakpoint_initialize(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                             DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt)
{
    DBG_STATUS      dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;    
    
    /* Reset the breakpoints activation state. */
    
    p_addr_bkpt -> state = DBG_ENG_ADDRESS_BREAKPOINT_STATE_INACTIVE;
    
    /* Initialize the breakpoint's context breakpoint set. */
    
    dbg_status = DBG_SET_Initialize(&p_addr_bkpt -> ctxt_bkpt_set,
                                    DBG_SET_BEHAVIOR_FIFO);
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_create
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function creates an address breakpoint.  Note that the
*       newly created breakpoint is created in an inactive state.
*                                                                                                 
*   INPUTS                                                               
*        
*       p_dbg_eng_bkpt - Pointer to the control block.
*
*       address - Address where breakpoint is to be created.
*
*       p_p_addr_bkpt - Pointer to the created address breakpoint if the 
*                    operation is successful.  Value is undefined if 
*                    operation fails.
*                                                        
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       DBG_STATUS_ALREADY_EXISTS - A breakpoint with matching 
*                                   characteristics already exists.
*
*       DBG_STATUS_OUT_OF_MEMORY - Not enough system memory available to
*                                  create new breakpoint.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_address_breakpoint_create(DBG_ENG_BKPT_CB *                  p_dbg_eng_bkpt,
                                                         DBG_OS_OPCODE *                    address,
                                                         DBG_ENG_ADDRESS_BREAKPOINT **      p_p_addr_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_OS_OPC_CMD_PARAM            os_opc_cmd_param;

    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine if there is already an address breakpoint at the target 
       location. */
    
    p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_address(p_dbg_eng_bkpt,
                                                                    p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                    address);
    
    if (p_addr_bkpt != NU_NULL)
    {
        /* An address breakpoint already exists and so another cannot
           be created. */
    
        dbg_status = DBG_STATUS_ALREADY_EXISTS;
            
    }
    else
    {
        /* There is no address breakpoint at the target location so one
           will need to be created. */

        /* Attempt to get a free address breakpoint. */
        
        p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_remove(p_dbg_eng_bkpt,
                                                                  &p_dbg_eng_bkpt -> p_free_addr_bkpts,
                                                                  NU_NULL);

        /* Ensure a free address breakpoint was found before continuing. */
        
        if (p_addr_bkpt == NU_NULL) 
        {
            /* There are no free address breakpoints. */
        
            /* Indicate failure. */
        
            dbg_status = DBG_STATUS_OUT_OF_MEMORY;
    
        }
        else
        {
            /* A free address breakpoint was found. */
        
            /* Set up new address breakpoint structure. */
         
            p_addr_bkpt -> address = address; 
         
            /* Store op-code at address. */
         
            os_opc_cmd_param.op = DBG_OS_OPC_OP_READ;
            os_opc_cmd_param.op_param.read.p_address = (VOID *)address;
            
            dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);         
         
            if (dbg_status == DBG_STATUS_OK)
            {         
                p_addr_bkpt -> store = (DBG_OS_OPCODE)os_opc_cmd_param.op_param.read.op_code;
                
                /* Note: currently only ISR type breakpoints are 
                   supported. */
                
                p_addr_bkpt->type = DBG_ENG_ADDRESS_BREAKPOINT_TYPE_ISR;            
            
                /* Add the new breakpoint to the breakpoint list. */
    
                dbg_eng_bkpt_address_breakpoint_list_add(p_dbg_eng_bkpt,
                                                         &p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                         p_addr_bkpt);
            
            } 
            
        } 
        
    } 
        
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Update return parameters. */
        
        *p_p_addr_bkpt = p_addr_bkpt;
        
    } 
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_address_breakpoint_remove
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function removes an address breakpoint.
*                                                                                                 
*   INPUTS                                                               
*          
*       p_dbg_eng_bkpt - Pointer to the control block. 
*
*       p_addr_bkpt - Pointer to the target address breakpoint to be 
*                     removed.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       DBG_STATUS_INVALID_BREAKPOINT - Breakpoint was not found in the
*                                       breakpoint list.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_address_breakpoint_remove(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_temp_addr_bkpt;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Target breakpoint is a valid address breakpoint structure. */

    /* Attempt to find the target breakpoint in the address breakpoint 
       list. */
       
    p_temp_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_breakpoint(p_dbg_eng_bkpt,
                                                                            p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                            p_addr_bkpt);
    
    /* Determine if the target address breakpoint was found. */

    if (p_temp_addr_bkpt == NU_NULL)
    {
        /* The target breakpoint was NOT found in the address
           breakpoint list. */

        dbg_status = DBG_STATUS_INVALID_BREAKPOINT;
    
    } 
        
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* The target address breakpoint was found. */
        
        /* Remove the target breakpoint from the address breakpoint 
           list. */

        p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_remove(p_dbg_eng_bkpt,
                                                                  &p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                  p_addr_bkpt);

        /* Deactivate the address breakpoint. */
    
        dbg_status = dbg_eng_bkpt_address_breakpoint_deactivate(p_dbg_eng_bkpt,
                                                                p_addr_bkpt);
               
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Add the target breakpoint to the free breakpoint list. */
        
        dbg_eng_bkpt_address_breakpoint_list_add(p_dbg_eng_bkpt,
                                                 &p_dbg_eng_bkpt -> p_free_addr_bkpts,
                                                 p_temp_addr_bkpt);

    } 
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_breakpoints_erase_references
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function erases references to the specified breakpoint from
*       all other breakpoints.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_eng_bkpt - Pointer to the control block.
*
*       p_erase_ctxt_bkpt - Context breakpoint to be erased.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoints_erase_references(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                            DBG_ENG_CONTEXT_BREAKPOINT *   p_erase_ctxt_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Set the current breakpoint to the start of the all address 
       breakpoints list. */
    
    p_addr_bkpt = p_dbg_eng_bkpt -> p_all_addr_bkpts;
    
    /* Iterate through all address breakpoints. */

    while (p_addr_bkpt != NU_NULL)
    {
        /* Attempt to get the first context breakpoint from the set. */
        
        dbg_status = DBG_SET_Node_Find(&p_addr_bkpt -> ctxt_bkpt_set,
                                       DBG_SET_LOCATION_HEAD,
                                       DBG_SET_NODE_ID_ANY,
                                       (DBG_SET_NODE **)&p_ctxt_bkpt);
        
        while (dbg_status == DBG_STATUS_OK)
        {
            /* Determine if the current temporary breakpoint's
               auxiliary breakpoint is the target breakpoint and
               if so then set it to NULL. */
        
            if (p_ctxt_bkpt -> p_aux_ctxt_bkpt == p_erase_ctxt_bkpt)
            {
                p_ctxt_bkpt -> p_aux_ctxt_bkpt = NU_NULL;

            } 

            /* Determine if the current temporary breakpoint's
               twin breakpoint is the target breakpoint and if so
               then set it to NULL. */
            
            if (p_ctxt_bkpt -> p_twin_ctxt_bkpt == p_erase_ctxt_bkpt) 
            {
                p_ctxt_bkpt -> p_twin_ctxt_bkpt = NU_NULL;

            } 
        
            /* Attempt to get another context breakpoint from the set. */
            
            dbg_status = DBG_SET_Node_Find(&p_addr_bkpt -> ctxt_bkpt_set,
                                           DBG_SET_LOCATION_NEXT_LINEAR,
                                           (DBG_SET_LOCATION_VALUE)p_ctxt_bkpt,
                                           (DBG_SET_NODE **)&p_ctxt_bkpt);       
                
        }            
           
        /* Move to the next address breakpoint. */
        
        p_addr_bkpt = p_addr_bkpt -> next;
            
    } 

    /* Update return status.  Note that the function is successful whether
       the target was found or not since the operation (erase) was
       performed without errors. */
    
    dbg_status = DBG_STATUS_OK;    
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_breakpoint_create
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function creates a breakpoint.
*                                                                                                 
*   INPUTS                                                               
*        
*       p_dbg_eng_bkpt - Pointer to the control block.
*
*       exec_ctxt_id - The execution context ID associated with the 
*                      breakpoint. 
*
*       exec_ctxt_os_data - Any OS data associated with the execution
*                           context.
*
*       address - Address where breakpoint is to be created.
*
*       type - The type of breakpoint to create.
*
*       eval_func - Function to be associated with the breakpoint that is 
*                   called to evaluate the situation when a breakpoint is 
*                   hit to determine if the breakpoint causes a stop of 
*                   execution.
*
*       eval_func_param - Parameters to be passed to the eval breakpoint 
*                         function.
*
*       hit_func - Function to be associated with the breakpoint that is 
*                  called when the breakpoint hits (stops).
*
*       hit_func_param- Parameters to be passed to the hit breakpoint 
*                       function.
*
*       susp_exec_ctxt_id - The execution context ID to be suspended when
8                           the breakpoint is hit.
*
*       p_aux_ctxt_bkpt - Pointer to an alternate breakpoint.
*
*       p_twin_ctxt_bkpt - Pointer to an alternate breakpoint in situations 
*                          where there is a branch in execution.
*
*       pass_count - Value indicating the number of times a breakpoint will
*                    be passed before it hits.
*
*       pass_cycle - Reset value for pass count (once breakpoint hits).
*
*       hit_count - Value indicating the number of times a breakpoint will
*                   be hit before being removed.
*
*       next1 - Next i instruction.
*
*       next2 - Next i + 1 instruction.
*
*       i0type - Type flags for the breakpoint.
*
*       p_p_addr_bkpt - Pointer to the created address breakpoint if the 
*                       operation is successful.  Value is undefined if 
*                       operation fails.
*
*       p_p_ctxt_bkpt - Pointer to the created context breakpoint if the 
*                       operation is successful.  Value is undefined if 
*                       operation fails.
*                                                        
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       DBG_STATUS_ALREADY_EXISTS - A matching breakpoint already exists.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_create(DBG_ENG_BKPT_CB *                  p_dbg_eng_bkpt,
                                                 DBG_THREAD_ID                      exec_ctxt_id,
                                                 VOID *                             exec_ctxt_os_data,
                                                 DBG_OS_OPCODE *                    address,
                                                 DBG_ENG_BREAKPOINT_TYPE            type,                                                   
                                                 DBG_ENG_BKPT_EVAL_FUNC             eval_func,
                                                 DBG_ENG_BKPT_EVAL_FUNC_PARAM       eval_func_param,
                                                 DBG_ENG_BKPT_HIT_FUNC              hit_func,
                                                 DBG_ENG_BKPT_HIT_FUNC_PARAM        hit_funcParam,
                                                 DBG_THREAD_ID                      susp_exec_ctxt_id,
                                                 DBG_ENG_CONTEXT_BREAKPOINT *       p_aux_ctxt_bkpt,
                                                 DBG_ENG_CONTEXT_BREAKPOINT *       p_twin_ctxt_bkpt,
                                                 UINT                               pass_count,
                                                 UINT                               pass_cycle,
                                                 UINT                               hit_count,
                                                 DBG_ENG_ADDRESS_BREAKPOINT **      p_p_addr_bkpt,
                                                 DBG_ENG_CONTEXT_BREAKPOINT **      p_p_ctxt_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;    
    BOOLEAN                         create_ctxt_bkpt;
    BOOLEAN                         create_addr_bkpt;

    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine if there is already an address breakpoint at the target 
       location.  This will also determine which resources will need to be
       created. */
    
    p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_address(p_dbg_eng_bkpt,
                                                                    p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                    address);
    
    if (p_addr_bkpt == NU_NULL)
    {
        /* There is no address breakpoint currently so one will need to be
           created as well as a context breakpoint. */
           
        create_addr_bkpt = NU_TRUE;
        create_ctxt_bkpt = NU_TRUE;
    
    }
    else
    {
        /* There is an address breakpoint at the target address, so one
           does not need to be created. */
    
        create_addr_bkpt = NU_FALSE;
           
        /* Determine if the address breakpoint already has a context 
           breakpoint for the target context. */
           
        dbg_status = DBG_SET_Node_Find(&p_addr_bkpt -> ctxt_bkpt_set,
                                       DBG_SET_LOCATION_HEAD,
                                       (DBG_SET_LOCATION_VALUE)exec_ctxt_id,
                                       (DBG_SET_NODE **)&p_ctxt_bkpt);
           
        if (dbg_status == DBG_STATUS_OK)
        {
            /* ERROR: A context breakpoint already exists and so another 
               cannot be created. */
        
            dbg_status = DBG_STATUS_ALREADY_EXISTS;
            
        }
        else
        {
            /* There is no existing context breakpoint.  One will need to
               be created. */
               
            create_ctxt_bkpt = NU_TRUE;

        } 
        
    } 
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (create_addr_bkpt == NU_TRUE))
    {
        /* There is no address breakpoint at the target location so one
           will need to be created. */

        /* Create a new address breakpoint. */
        
        dbg_status = dbg_eng_bkpt_address_breakpoint_create(p_dbg_eng_bkpt,
                                                            address,
                                                            &p_addr_bkpt);
        
    } 
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (create_ctxt_bkpt == NU_TRUE))
    {
        /* Create a new context breakpoint.  Note that the hit context
           and the suspend context are set to the same (execution) context
           ID value. */
        
        dbg_status = dbg_eng_bkpt_context_breakpoint_create(p_dbg_eng_bkpt,
                                                            exec_ctxt_id,
                                                            exec_ctxt_os_data,
                                                            type,
                                                            susp_exec_ctxt_id,
                                                            eval_func,
                                                            eval_func_param,
                                                            hit_func,
                                                            hit_funcParam,
                                                            pass_count,
                                                            pass_cycle,
                                                            hit_count,
                                                            p_aux_ctxt_bkpt,
                                                            p_twin_ctxt_bkpt,
                                                            p_addr_bkpt,
                                                            &p_ctxt_bkpt);
        
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Add the new context breakpoint to the address breakpoint 
               with the execution context ID as the node ID. */
           
            dbg_status = DBG_SET_Node_Add(&p_addr_bkpt -> ctxt_bkpt_set,
                                          DBG_SET_LOCATION_DEFAULT,
                                          NU_NULL,
                                          (DBG_SET_NODE_ID)p_ctxt_bkpt -> exec_ctxt_id,
                                          (DBG_SET_NODE *)p_ctxt_bkpt);
         
        } 
        
    }    
        
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Update return parameters. */
        
        *p_p_addr_bkpt = p_addr_bkpt;
        *p_p_ctxt_bkpt = p_ctxt_bkpt;

    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_breakpoint_remove
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function removes a breakpoint.  The context breakpoint will
*       be removed first and then the address breakpoint ONLY if there are
*       no other context breakpoints associated with it.
*                                                                                                 
*   INPUTS                                                               
*          
*       p_dbg_eng_bkpt - Pointer to the control block. 
*
*       p_addr_bkpt - Pointer to the target address breakpoint.
*
*       p_ctxt_bkpt - Pointer to the target context breakpoint.  If this
*                     value is NULL then no attempt will be made to remove
*                     a context breakpoint.
*
*       ctxt_bkpt_index - Index of the context breakpoint into the context
*                         breakpoint set of the address breakpoint.  This
*                         is only used if context breakpoint is specified.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       DBG_STATUS_FAILED - Indicates operation failed.
*
*       DBG_STATUS_INVALID_PARAMETERS - Indicates invalid address
*                                       breakpoint specified.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_remove(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                 DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt,
                                                 DBG_ENG_CONTEXT_BREAKPOINT *   p_ctxt_bkpt,
                                                 UINT                           ctxt_bkpt_index)
{
    DBG_STATUS              dbg_status;
    UINT                    ctxt_bkpt_count;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Ensure there is an address breakpoint. */
    
    if (p_addr_bkpt == NU_NULL)
    {
        /* ERROR: Address breakpoint must be specified for operation. */
        
        dbg_status = DBG_STATUS_INVALID_PARAMETERS;
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {   
        /* Determine if a context breakpoint should be removed based on
           parameters passed in. */
    
        if (p_ctxt_bkpt != NU_NULL)
        {
            /* Remove the context breakpoint from the address 
               breakpoint. */
            
            dbg_status = DBG_SET_Node_Remove(&p_addr_bkpt -> ctxt_bkpt_set,
                                             DBG_SET_LOCATION_INDEX,
                                             (DBG_SET_LOCATION_VALUE)ctxt_bkpt_index,
                                             (DBG_SET_NODE **)&p_ctxt_bkpt);
        
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Remove any references to the target breakpoint from any
                   other breakpoint. */
        
                dbg_status = dbg_eng_bkpt_breakpoints_erase_references(p_dbg_eng_bkpt,
                                                                       p_ctxt_bkpt);
            
            }             
            
            if (dbg_status == DBG_STATUS_OK)
            {    
                /* Delete the context breakpoint. */
                
                dbg_eng_bkpt_context_breakpoint_delete(p_dbg_eng_bkpt,
                                                       p_ctxt_bkpt);
        
            }  
            
        } 
                
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Determine if there are any more context breakpoints 
               associated with the address breakpoint. */
               
            ctxt_bkpt_count = DBG_SET_Node_Count(&p_addr_bkpt -> ctxt_bkpt_set);
            
            if (ctxt_bkpt_count == 0)
            {
                /* There are no more context breakpoints for the address
                   breakpoint, so it may be removed. */
                   
                dbg_status = dbg_eng_bkpt_address_breakpoint_remove(p_dbg_eng_bkpt,
                                                                    p_addr_bkpt);            
            
            } 
    
        } 
           
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_update_related
*
*   DESCRIPTION
*
*       This function updates related breakpoints during breakpoint HISR 
*       handling activities.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control 
*                        block.
*
*       p_ctxt_bkpt - Pointer to the context breakpoint.
*
*       p_stack_frame - Pointer to stack frame for OS execution context.
*
*       stack_frame_type - Indicates the type of stack frame.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_update_related(DBG_ENG_BKPT_CB *                p_dbg_eng_bkpt,
                                                                 DBG_ENG_CONTEXT_BREAKPOINT *     p_ctxt_bkpt,
                                                                 VOID *                           p_stack_frame,
                                                                 DBG_OS_STACK_FRAME_TYPE          stack_context_type)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_twin_ctxt_bkpt;    
    DBG_ENG_ADDRESS_BREAKPOINT *    p_twin_addr_bkpt;
    UINT                            twin_ctxt_bkpt_index;
    DBG_OS_REG_CMD_PARAM            os_reg_cmd_param;

    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Check for an auxiliary context breakpoint associated with 
       the current context breakpoint. */
   
    if (p_ctxt_bkpt -> p_aux_ctxt_bkpt != NU_NULL)
    {
        /* An auxiliary context breakpoint exists. */

        /* Activate the auxiliary context breakpoint by activating its
           address breakpoint. */
     
        dbg_status = dbg_eng_bkpt_address_breakpoint_activate(p_dbg_eng_bkpt,
                                                              p_ctxt_bkpt -> p_aux_ctxt_bkpt -> p_addr_bkpt);
     
    } 

    if ((dbg_status == DBG_STATUS_OK) &&
        (p_ctxt_bkpt -> int_state_restore == NU_TRUE))
    {
        /* Restore the interrupt state of the thread. */
        
        os_reg_cmd_param.op = DBG_OS_REG_OP_INT_RESTORE;
        os_reg_cmd_param.op_param.int_res.p_stack_context = p_stack_frame;
        os_reg_cmd_param.op_param.int_res.stack_context_type = stack_context_type;
        os_reg_cmd_param.op_param.int_res.int_state = p_ctxt_bkpt -> int_state;
    
        dbg_status = DBG_OS_Reg_Command(&os_reg_cmd_param);        
      
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Check if a twin context breakpoint exists for the current 
           context breakpoint. */
       
        if (p_ctxt_bkpt -> p_twin_ctxt_bkpt != NU_NULL) 
        {
            /* A twin context breakpoint exists. */
    
            /* Get pointer to the twin context breakpoint. */
            
            p_twin_ctxt_bkpt = p_ctxt_bkpt -> p_twin_ctxt_bkpt;
            
            /* Get pointer to the address breakpoint associated with the
               context breakpoint. */
            
            p_twin_addr_bkpt = p_twin_ctxt_bkpt -> p_addr_bkpt;
               
            /* Get the twin context breakpoints index into the context 
               breakpoint set of its associated address breakpoint. */
               
            p_twin_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                                        &p_twin_addr_bkpt -> ctxt_bkpt_set,
                                                                        NU_NULL,
                                                                        DBG_ENG_BREAKPOINT_TYPE_NONE,
                                                                        p_twin_ctxt_bkpt,
                                                                        &twin_ctxt_bkpt_index);
            
            if (p_twin_ctxt_bkpt != NU_NULL)
            {
                /* Remove the twin context breakpoint and possibly the address
                   breakpoint associated with it. */
             
                dbg_status = dbg_eng_bkpt_breakpoint_remove(p_dbg_eng_bkpt,
                                                            p_twin_addr_bkpt,
                                                            p_twin_ctxt_bkpt,
                                                            twin_ctxt_bkpt_index);        
            
            } 
                
        }  
    
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_suspend_execution
*
*   DESCRIPTION
*
*       This function suspends execution during breakpoint HISR handling 
*       activities.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control block.
*
*       p_ctxt_bkpt - Pointer to the context breakpoint.
*
*       p_os_thread - OS Thread that the breakpoint occurred on.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_suspend_execution(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                                    DBG_ENG_CONTEXT_BREAKPOINT *   p_ctxt_bkpt,
                                                                    NU_TASK *                      p_os_thread)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_BREAKPOINT_ID               hit_func_bkpt_id;
    DBG_THREAD_ID                   susp_exec_ctxt_id;
    DBG_ENG_EXEC_STOP_PARAM         exec_stop_param;   
    DBG_THREAD_ID                   hit_thread_id;

    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Get pointer to the service control block. */
    
    p_dbg_eng = (DBG_ENG_CB *)p_dbg_eng_bkpt -> p_dbg_eng;
    
    /* Matching breakpoints were found.  Use the information from 
       the breakpoints to suspend the execution context. */

    /* Get thread ID for OS thread that the breakpoint hit occurred 
       on. */

    hit_thread_id = (DBG_THREAD_ID)p_os_thread;

    /* Determine which execution context ID to use for suspension
       based on the type of the breakpoint. */
       
    if (p_ctxt_bkpt -> type == DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP)
    {
        susp_exec_ctxt_id = p_ctxt_bkpt -> aux_exec_ctxt_id;
        
    }
    else
    {
        susp_exec_ctxt_id = p_ctxt_bkpt -> susp_exec_ctxt_id;
        
    } 
    
    /* Conditionally translate an suspension execution context ID
       of ANY to be the execution context that the breakpoint hit
       actually occurred on. */
    
    if (susp_exec_ctxt_id == DBG_THREAD_ID_ANY)
    {
        susp_exec_ctxt_id = hit_thread_id;
           
    } 
    
    /* Suspend the execution context. */
    
    exec_stop_param.thread_id = susp_exec_ctxt_id;
    exec_stop_param.use_direct_events = NU_FALSE;
    
    dbg_status = DBG_ENG_EXEC_Stop(&p_dbg_eng -> exec,
                                   &exec_stop_param);

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine if there is a breakpoint hit function associated
           with the context breakpoint. */
           
        if (p_ctxt_bkpt -> hit_func != NU_NULL)
        {
            /* Determine which breakpoint ID value to pass to the hit
               function based on the type of the (context) breakpoint. */
             
            switch (p_ctxt_bkpt -> type)
            {
                case DBG_ENG_BREAKPOINT_TYPE_HIT :
                {
                    /* Normal breakpoint.  Return the 'breakpoint hit' ID
                       value. */
                       
                    hit_func_bkpt_id = DBG_BREAKPOINT_ID_HIT;
                    
                    break;
                    
                } 
                                
                case DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP :
                case DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP :                
                {
                    /* Step-type breakpoint.  Return the special "step"
                       breakpoint ID value. */
                       
                    hit_func_bkpt_id = DBG_BREAKPOINT_ID_STEP;
                    
                    break;
                    
                } 
                
                case DBG_ENG_BREAKPOINT_TYPE_ACTIVE :                               
                case DBG_ENG_BREAKPOINT_TYPE_SKIPOVER :                
                case DBG_ENG_BREAKPOINT_TYPE_NONE :                
                default :
                {
                    /* ERROR: Invalid context breakpoint type. */
                    
                    /* ERROR RECOVERY: Assume it is not a step-type 
                       breakpoint and return the 'breakpoint hit' 
                       value. */
                       
                    hit_func_bkpt_id = DBG_BREAKPOINT_ID_HIT;
                    
                    break;
                    
                } 
            
            } 
            
            /* Call the context breakpoint hit function. */
            
            p_ctxt_bkpt -> hit_func(hit_thread_id,
                                    hit_func_bkpt_id,
                                    p_ctxt_bkpt -> hit_func_param);

        }

    }
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_handle_normal
*
*   DESCRIPTION
*
*       This function handles a normal type breakpoint exception at the 
*       HISR level.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control block.
*
*       p_addr_bkpt - Pointer to the address breakpoint corresponding to
*                     the exception.
*
*       p_ctxt_bkpt - Pointer to the context breakpoint corresponding to
*                     the exception.
*
*       p_suspend_exec_ctxt - Return parameter that will be updated to
*                             indicate whether the execution context that
*                             the breakpoint occurred on should be 
*                             suspended if the operation is successful. If
*                             the operation fails the value is undefined.
*
*       p_skip_over_bkpt - Return parameter that will be updated to 
*                          indicate if the current breakpoint should be 
*                          skipped over if the operation is successful.  
*                          If the operation fails the value is undefined.
*
*       p_remove_bkpt - Return parameter that will be updated to indicate
*                       if the current breakpoint should be removed or
*                       not if the operation is successful.  If the 
*                       operation fails the value is undefined.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation of the function.
*
*       <other> - Indicates other internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_handle_normal(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                                DBG_ENG_ADDRESS_BREAKPOINT *   p_addr_bkpt,
                                                                DBG_ENG_CONTEXT_BREAKPOINT *   p_ctxt_bkpt,
                                                                BOOLEAN *                      p_suspend_exec_ctxt,
                                                                BOOLEAN *                      p_skip_over_bkpt,
                                                                BOOLEAN *                      p_remove_bkpt)
{
    DBG_STATUS                      dbg_status;
    BOOLEAN                         func_hit;
    BOOLEAN                         pass_count_hit;
    BOOLEAN                         suspend_exec_ctxt;    
    BOOLEAN                         skip_over_bkpt;
    BOOLEAN                         remove_bkpt;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Set initial breakpoint handling behavior values. */
    
    skip_over_bkpt = NU_FALSE;
    suspend_exec_ctxt = NU_FALSE;        
    remove_bkpt = NU_FALSE;        
        
    /* Determine if an evaluation callback function is associated with 
       the context breakpoint. */
       
    if (p_ctxt_bkpt -> eval_func != NU_NULL)
    {
        INT     bkpt_callback_rtrn;
    
        /* A breakpoint evaluation function is defined so call it. */
        
        bkpt_callback_rtrn = p_ctxt_bkpt -> eval_func(p_ctxt_bkpt -> eval_func_param);
        
        /* Interpret the result of the callback. */
        
        if (bkpt_callback_rtrn == NU_NULL)
        {
            func_hit = NU_FALSE;
            
        }
        else
        {
            func_hit = NU_TRUE;
            
        } 
    
    }
    else
    {
        /* No evaluation callback function is associated so no 
           suspension. */
        
        func_hit = NU_FALSE;
        
    } 

    /* Decrement the pass counter while it is positive. */
 
    if (p_ctxt_bkpt->pass_count > 0)
    {
        (p_ctxt_bkpt->pass_count)--;
    
    } 

    /* Determine if the pass counter is still positive. */
 
    if (p_ctxt_bkpt -> pass_count > 0) 
    {
        /* Pass count is positive so continue... */
 
        /* Determine how to proceed based on the type of the context 
           breakpoint. */
        
        if (p_ctxt_bkpt -> type == DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP) 
        {
            /* Treat it as a Single-Step because the breakpoint was 
               not hit. */
           
            /* Indicate the task that the exception occurred on should 
               be suspended. */
     
            pass_count_hit = NU_TRUE;
           
        }
        else
        {
            /* Breakpoint is not an active-step type. */
            
            /* Indicate that the address breakpoint should be skipped 
               over. */
               
            skip_over_bkpt = NU_TRUE;
            
            /* Indicate the task that the exception occurred on should 
               not be suspended. */
   
            pass_count_hit = NU_FALSE;

        } 
     
    }
    else    
    {
        /* Reset the pass counter using the pass cycle value. */

        p_ctxt_bkpt -> pass_count = p_ctxt_bkpt -> pass_cycle;                        

        /* Indicate the task that the exception occurred on should be
           suspended. */

        pass_count_hit = NU_TRUE;
    
    }         
    
    /* Determine if the thread should be suspended. */
    
    if ((func_hit == NU_TRUE) ||
        (pass_count_hit == NU_TRUE))
    {
        /* Indicate that the thread should be suspended. */
        
        suspend_exec_ctxt = NU_TRUE;
    
        /* Update breakpoint hit count and remove flag. */
    
        if (p_ctxt_bkpt -> hit_count == 1)
        {
            remove_bkpt = NU_TRUE;
            
        }    
        else 
        {
            if (p_ctxt_bkpt -> hit_count > 0)
            {
                p_ctxt_bkpt -> hit_count--;
            
            }
            
        }
    
    }         
    
    /* Update the current context breakpoint's type. */
 
    p_ctxt_bkpt -> type = DBG_ENG_BREAKPOINT_TYPE_HIT;

    /* Update return parameter values. */
    
    *p_suspend_exec_ctxt = suspend_exec_ctxt;
    *p_skip_over_bkpt = skip_over_bkpt;
    *p_remove_bkpt = remove_bkpt;
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_software_thread
*
*   DESCRIPTION
*
*       This function handles the situation where a software breakpoint 
*       has been encountered on a thread.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control 
*                        block.
*
*       p_os_thread - Pointer to the thread control block that the
*                     exception occurred on.
*
*       p_excp_stack_frame - Pointer to exception stack frame for OS
*                            execution context.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_software_thread(DBG_ENG_BKPT_CB *        p_dbg_eng_bkpt,
                                                                  NU_TASK *                p_os_thread,
                                                                  VOID *                   p_excp_stack_frame)
{
    DBG_STATUS                          dbg_status;
    DBG_OS_OPCODE *                     pc;
    DBG_OS_OPCODE                       nop_op_code;    
    DBG_OS_OPC_CMD_PARAM                os_opc_cmd_param;   
    DBG_ENG_ADDRESS_BREAKPOINT *        p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *        p_ctxt_bkpt; 
    UNSIGNED                            thread_group_id;
    BOOLEAN                             is_protected;
    UINT                                ctxt_bkpt_index;
    BOOLEAN                             suspend_exec_ctxt;
    BOOLEAN                             remove_bkpt;
    BOOLEAN                             update_related_bkpts;  
    BOOLEAN                             skip_over_bkpt;
    DBG_ENG_BKPT_SET_SKIP_STEP_PARAM    bkpt_set_skip_step_param;
    DBG_ENG_BKPT_SET_PARAM              bkpt_set_param;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Get the Program Counter (PC) for the specified context. */
    
    os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_NEXT_INST_PTR;
    os_opc_cmd_param.op_param.get_next_inst_ptr.p_stack_context = p_excp_stack_frame;
    os_opc_cmd_param.op_param.get_next_inst_ptr.stack_context_type = DBG_OS_STACK_FRAME_TYPE_EXCEPTION;
    
    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {
        pc = (DBG_OS_OPCODE *)os_opc_cmd_param.op_param.get_next_inst_ptr.p_next_inst;
        
        /* Set initial state of context breakpoint pointer. */
        
        p_ctxt_bkpt = NU_NULL;
    
        /* Attempt to find a breakpoint matching the address where the 
           exception occurred. */
           
        p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_address(p_dbg_eng_bkpt,
                                                                        p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                        pc);
        
        if (p_addr_bkpt != NU_NULL) 
        {
            /* Found an address breakpoint matching the address of the
               exception that occurred.  The exception is from an opcode
               placed by the breakpoint system. */
    
            /* Attempt to find a matching context breakpoint associated with 
               the address breakpoint and the OS thread. */
               
            p_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                                   &p_addr_bkpt -> ctxt_bkpt_set,
                                                                   p_os_thread,
                                                                   DBG_ENG_BREAKPOINT_TYPE_NONE,
                                                                   NU_NULL,
                                                                   &ctxt_bkpt_index);
            
            /* Determine if a matching context breakpoint was found. */
               
            if (p_ctxt_bkpt != NU_NULL)
            {    
                /* Determine if the OS thread context that the 
                   breakpoint occurred on is protected. */
                
                thread_group_id = TCS_Task_Group_ID(p_os_thread);
                
                if (thread_group_id == TC_GRP_ID_SYS)
                {
                    is_protected = NU_TRUE;
                }
                else
                {
                    is_protected = NU_FALSE;
                }
                
                /* Proceed based on the protected state of the breakpoint
                   thread. */
                
                if (is_protected == NU_TRUE)
                {
                    /* Determine how to proceed based on the type of 
                       context breakpoint. */
                       
                    switch (p_ctxt_bkpt -> type) 
                    {
                       case DBG_ENG_BREAKPOINT_TYPE_SKIPOVER :
                       {
                            /* This is a skip-over for a breakpoint
                               previously encountered on a protected 
                               thread.  Handle it like a normal skip-over
                               so that the original breakpoint will be 
                               restored. */               
                  
                            remove_bkpt = NU_TRUE;
                            suspend_exec_ctxt = NU_FALSE;
                            update_related_bkpts = NU_TRUE;
                            skip_over_bkpt = NU_FALSE;                
                    
                            break;
                            
                        } 
                        
                        case DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP :
                        case DBG_ENG_BREAKPOINT_TYPE_ACTIVE :
                        case DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP :
                        case DBG_ENG_BREAKPOINT_TYPE_HIT :
                        { 
                            /* Breakpoints hit on a protected thread 
                               should be skipped over (and restored). */
                            
                            remove_bkpt = NU_FALSE;                           
                            suspend_exec_ctxt = NU_FALSE;
                            update_related_bkpts = NU_TRUE;
                            skip_over_bkpt = NU_TRUE;                         
                            
                            break;
                           
                        } 
                         
                        default :
                        {             
                            /* ERROR: Invalid context breakpoint type. */
        
                            /* ERROR RECOVERY: Halt the thread but don't 
                               change any other aspects of the breakpoint 
                               system since this is an unknown 
                               situation. */
        
                            suspend_exec_ctxt = NU_TRUE;
                            remove_bkpt = NU_FALSE;                    
                            update_related_bkpts = NU_FALSE;
                            skip_over_bkpt = NU_FALSE;                        
                    
                            break;
            
                        } 
                         
                    }                 
                    
                }
                else
                {                    
                    /* Determine the type of context breakpoint. */
                    
                    switch (p_ctxt_bkpt -> type) 
                    {
                       case DBG_ENG_BREAKPOINT_TYPE_SKIPOVER :
                       {
                            /* This type of breakpoint is put in the code 
                               so a thread can skip over the existing 
                               breakpoint, restore it (ie. change its 
                               flavor from inactive to active) and proceed
                               with execution without any intervention. */               
                  
                            remove_bkpt = NU_TRUE;
                            suspend_exec_ctxt = NU_FALSE;
                            update_related_bkpts = NU_TRUE;
                            skip_over_bkpt = NU_FALSE;
                              
                            break;
                            
                        } 
                        
                        case DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP :
                        {
                            /* These types of breakpoint are used to track
                               an assembly-level step operation. */                
    
                            remove_bkpt = NU_TRUE;                           
                            suspend_exec_ctxt = NU_TRUE;             
                            update_related_bkpts = NU_TRUE;
                            skip_over_bkpt = NU_FALSE;
                                
                            break;
            
                        } 
                         
                        case DBG_ENG_BREAKPOINT_TYPE_ACTIVE :
                        case DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP :
                        case DBG_ENG_BREAKPOINT_TYPE_HIT :
                        {
                            /* This is a 'normal' breakpoint type.  The
                               determination of suspension and skip-over
                               activities is made by a routine call.  The
                               breakpoint should persist until cleared. */
                            
                            dbg_status = dbg_eng_bkpt_breakpoint_handler_handle_normal(p_dbg_eng_bkpt,
                                                                                       p_addr_bkpt,
                                                                                       p_ctxt_bkpt,
                                                                                       &suspend_exec_ctxt,
                                                                                       &skip_over_bkpt,
                                                                                       &remove_bkpt);
        
                            update_related_bkpts = NU_TRUE;                    
                            
                            break;
                           
                        } 
                        
                        default :
                        {
                            /* ERROR: Invalid context breakpoint type. */
        
                            /* ERROR RECOVERY: Halt the thread but don't 
                               change any other aspects of the breakpoint 
                               system since this is an unknown situation. */
        
                            suspend_exec_ctxt = NU_TRUE;
                            remove_bkpt = NU_FALSE;                    
                            update_related_bkpts = NU_FALSE;
                            skip_over_bkpt = NU_FALSE;
                            
                            break;
            
                        } 
                         
                    }             
                    
                }                 
                
            }
            else
            {
                /* No matching context breakpoint was found.  This 
                   indicates that the breakpoint was for another 
                   context. */
                   
                /* The breakpoint should not be removed and execution 
                   should not be suspended.  Any related breakpoints 
                   should be updated at this time. */
                   
                remove_bkpt = NU_FALSE;                           
                suspend_exec_ctxt = NU_FALSE;
                update_related_bkpts = NU_TRUE;
                
                /* Skip over the breakpoint. */
                
                skip_over_bkpt = NU_TRUE;
    
            } 
                        
        }
        else
        {        
            /* ERROR: There is no address breakpoint matching the 
               exception address.  This is an illegal instruction... */
    
            /* ERROR RECOVERY: Do not perform any normal breakpoint 
               handling and conditionally perform illegal instruction
               handling. */
            
            suspend_exec_ctxt = NU_FALSE;            
            remove_bkpt = NU_FALSE;
            update_related_bkpts = NU_FALSE;
            skip_over_bkpt = NU_FALSE;          
            
            if (p_dbg_eng_bkpt -> ill_inst_replace == NU_TRUE)
            {
                /* Get the appropriate NOP value for the address. */
                
                os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_NOP_VALUE;
                os_opc_cmd_param.op_param.get_nop_value.p_address = (VOID *)pc;    
                
                dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);         
            
                if (dbg_status == DBG_STATUS_OK)
                {
                    nop_op_code = os_opc_cmd_param.op_param.get_nop_value.nop_value;
                    
                    /* Write a NOP at the breakpoint address. */
                    
                    os_opc_cmd_param.op = DBG_OS_OPC_OP_WRITE;
                    os_opc_cmd_param.op_param.write.p_address = (VOID *)pc;
                    os_opc_cmd_param.op_param.write.op_code = (UINT)nop_op_code;    
                    
                    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);
               
                }
                
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Create a new breakpoint for the address. */
                    
                    bkpt_set_param.hit_exec_ctxt_id = DBG_THREAD_ID_ALL;
                    
                    if (p_dbg_eng_bkpt -> ill_inst_stops_thd == NU_TRUE)
                    {
                        bkpt_set_param.stop_exec_ctxt_id = (DBG_THREAD_ID)p_os_thread;
                    }
                    else
                    {
                        bkpt_set_param.stop_exec_ctxt_id = DBG_THREAD_ID_ALL;   
                    }
                    
                    bkpt_set_param.p_addr = pc;
                    bkpt_set_param.eval_func = DBG_ENG_BKPT_EVAL_FUNC_NONE;
                    bkpt_set_param.eval_func_param = DBG_ENG_BKPT_EVAL_FUNC_PARAM_NONE;
                    bkpt_set_param.hit_func = p_dbg_eng_bkpt -> ill_inst_hit_func;
                    bkpt_set_param.hit_func_param = p_dbg_eng_bkpt -> ill_inst_hit_func_param;
                    bkpt_set_param.pass_count = 0;
                    bkpt_set_param.pass_cycle = 0;
                    bkpt_set_param.hit_count = 0;
                    
                    dbg_status = DBG_ENG_BKPT_Breakpoint_Set(p_dbg_eng_bkpt,
                                                             &bkpt_set_param);
                 
                }
                
            }             
            
        } 

    }

    if ((dbg_status == DBG_STATUS_OK) &&
        (skip_over_bkpt == NU_TRUE))
    {
        /* A skip-over breakpoint must be inserted to re-
           activate current breakpoint once execution has moved past the 
           current address.  The skip-over breakpoint will hit
           on all contexts. */ 
       
        bkpt_set_skip_step_param.p_os_thread = p_os_thread;
        bkpt_set_skip_step_param.p_stack_frame = p_excp_stack_frame;
        bkpt_set_skip_step_param.stack_frame_type = DBG_OS_STACK_FRAME_TYPE_EXCEPTION;
        bkpt_set_skip_step_param.step_exec_ctxt_id = DBG_THREAD_ID_ALL;
        bkpt_set_skip_step_param.stop_exec_ctxt_id = DBG_THREAD_ID_NONE;
        bkpt_set_skip_step_param.hit_func = DBG_ENG_BKPT_HIT_FUNC_NONE;
        bkpt_set_skip_step_param.hit_func_param = DBG_ENG_BKPT_HIT_FUNC_PARAM_NONE;
       
        dbg_status = DBG_ENG_BKPT_Breakpoint_Set_Skip_Step(p_dbg_eng_bkpt,
                                                           &bkpt_set_skip_step_param);        
        
    } 

    if ((dbg_status == DBG_STATUS_OK) &&
        (update_related_bkpts == NU_TRUE) &&
        (p_ctxt_bkpt != NU_NULL))
    {
        /* Update related breakpoints. */    
    
        dbg_status = dbg_eng_bkpt_breakpoint_handler_update_related(p_dbg_eng_bkpt,
                                                                    p_ctxt_bkpt,
                                                                    p_excp_stack_frame,
                                                                    DBG_OS_STACK_FRAME_TYPE_EXCEPTION);
    
    } 
            
    if ((dbg_status == DBG_STATUS_OK) &&
        (suspend_exec_ctxt == NU_TRUE))
    {
        /* Suspend execution context. */
            
        dbg_status = dbg_eng_bkpt_breakpoint_handler_suspend_execution(p_dbg_eng_bkpt,
                                                                       p_ctxt_bkpt,
                                                                       p_os_thread);
            
    }     
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (remove_bkpt == NU_TRUE))
    {
        /* Remove the breakpoint. */
        
        dbg_status = dbg_eng_bkpt_breakpoint_remove(p_dbg_eng_bkpt,
                                                    p_addr_bkpt,
                                                    p_ctxt_bkpt,
                                                    ctxt_bkpt_index);

    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_software_other
*
*   DESCRIPTION
*
*       This function handles the situation where a software breakpoint 
*       has been encountered on a context other than a thread (may be a
*       HISR or LISR context).
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control 
*                        block.
*
*       p_os_exec_ctxt - Context that the breakpoint occurred on.
*
*       p_excp_stack_frame - Pointer to exception stack frame for OS
*                            execution context.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_software_other(DBG_ENG_BKPT_CB *        p_dbg_eng_bkpt,
                                                                 VOID *                   p_os_exec_ctxt,
                                                                 VOID *                   p_excp_stack_frame)
{
    DBG_STATUS                          dbg_status;
    DBG_OS_OPCODE *                     pc;    
    DBG_OS_OPC_CMD_PARAM                os_opc_cmd_param;        
    DBG_ENG_ADDRESS_BREAKPOINT *        p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *        p_ctxt_bkpt; 
    UINT                                ctxt_bkpt_index;
    BOOLEAN                             remove_bkpt;
    BOOLEAN                             update_related_bkpts;  
    BOOLEAN                             skip_over_bkpt;
    DBG_ENG_BKPT_SET_SKIP_STEP_PARAM    bkpt_set_skip_step_param;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Get the Program Counter (PC) for the specified context. */
    
    os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_NEXT_INST_PTR;
    os_opc_cmd_param.op_param.get_next_inst_ptr.p_stack_context = p_excp_stack_frame;
    os_opc_cmd_param.op_param.get_next_inst_ptr.stack_context_type = DBG_OS_STACK_FRAME_TYPE_EXCEPTION;    
    
    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {
        pc = (DBG_OS_OPCODE *)os_opc_cmd_param.op_param.get_next_inst_ptr.p_next_inst;

        /* Set initial state of context breakpoint pointer. */
        
        p_ctxt_bkpt = NU_NULL;
    
        /* Attempt to find a breakpoint matching the address where the 
           exception occurred. */
           
        p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_address(p_dbg_eng_bkpt,
                                                                        p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                        pc);
        
        if (p_addr_bkpt != NU_NULL) 
        {
            /* Found an address breakpoint matching the address of the
               exception that occurred.  The exception is from an opcode
               placed by the breakpoint system. */
    
            /* Attempt to find a matching context breakpoint associated with 
               the address breakpoint. */
               
            p_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                                   &p_addr_bkpt -> ctxt_bkpt_set,
                                                                   NU_NULL,
                                                                   DBG_ENG_BREAKPOINT_TYPE_NONE,
                                                                   NU_NULL,
                                                                   &ctxt_bkpt_index);
            
            /* Determine if a matching context breakpoint was found. */
               
            if (p_ctxt_bkpt != NU_NULL)
            {    
                /* Determine how to proceed based on the type of 
                   context breakpoint. */
                   
                switch (p_ctxt_bkpt -> type) 
                {
                   case DBG_ENG_BREAKPOINT_TYPE_SKIPOVER :
                   {
                        /* This is a skip-over breakpoint.  Remove it and
                           update the related breakpoints (retores the
                           original). */               
              
                        remove_bkpt = NU_TRUE;
                        update_related_bkpts = NU_TRUE;
                        skip_over_bkpt = NU_FALSE;                
                
                        break;
                        
                    } 
                    
                    case DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP :
                    case DBG_ENG_BREAKPOINT_TYPE_ACTIVE :
                    case DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP :
                    case DBG_ENG_BREAKPOINT_TYPE_HIT :
                    { 
                        /* Breakpoints hit on a protected execution context 
                           should be skipped over (and restored). */
                        
                        remove_bkpt = NU_FALSE;                           
                        update_related_bkpts = NU_TRUE;
                        skip_over_bkpt = NU_TRUE;                         
                        
                        break;
                       
                    } 
                     
                    default :
                    {             
                        /* ERROR: Invalid context breakpoint type. */
    
                        /* ERROR RECOVERY: Remove breakpoint and try to move
                           past it (continue execution). */
    
                        remove_bkpt = NU_FALSE;                    
                        update_related_bkpts = NU_FALSE;
                        skip_over_bkpt = NU_TRUE;                        
                
                        break;
        
                    } 
                     
                }                                  
                    
            }
            else
            {
                /* No matching context breakpoint was found.  This 
                   indicates that the breakpoint was for another 
                   context. */
                   
                /* The breakpoint should not be removed.  Any related 
                   breakpoints should be updated at this time. */
                   
                remove_bkpt = NU_FALSE;                           
                update_related_bkpts = NU_TRUE;
                
                /* Skip over the breakpoint. */
                
                skip_over_bkpt = NU_TRUE;
    
            } 
                        
        }
        else
        {        
            /* ERROR: There is no breakpoint matching the exception.  The 
               illegal instruction that caused the interrupt is not our 
               breakpoint opcode! */
    
            /* ERROR RECOVERY: None. */
            
            /* Do not remove or change anything in the breakpoint 
               system. */
            
            remove_bkpt = NU_FALSE;
            update_related_bkpts = NU_FALSE;
            skip_over_bkpt = NU_FALSE;          
            
        } 

    }

    if ((dbg_status == DBG_STATUS_OK) &&
        (skip_over_bkpt == NU_TRUE))
    {
        /* A skip-over breakpoint must be inserted to re-
           activate current breakpoint once execution has moved past the 
           current address.  The skip-over breakpoint will hit
           on all contexts. */ 
       
        bkpt_set_skip_step_param.p_os_thread = NU_NULL;
        bkpt_set_skip_step_param.p_stack_frame = p_excp_stack_frame;
        bkpt_set_skip_step_param.stack_frame_type = DBG_OS_STACK_FRAME_TYPE_EXCEPTION;
        bkpt_set_skip_step_param.step_exec_ctxt_id = DBG_THREAD_ID_ALL;
        bkpt_set_skip_step_param.stop_exec_ctxt_id = DBG_THREAD_ID_NONE;
        bkpt_set_skip_step_param.hit_func = DBG_ENG_BKPT_HIT_FUNC_NONE;
        bkpt_set_skip_step_param.hit_func_param = DBG_ENG_BKPT_HIT_FUNC_PARAM_NONE;
       
        dbg_status = DBG_ENG_BKPT_Breakpoint_Set_Skip_Step(p_dbg_eng_bkpt,
                                                           &bkpt_set_skip_step_param);        
        
    } 

    if ((dbg_status == DBG_STATUS_OK) &&
        (update_related_bkpts == NU_TRUE) &&
        (p_ctxt_bkpt != NU_NULL))
    {
        /* Update related breakpoints. */    
    
        dbg_status = dbg_eng_bkpt_breakpoint_handler_update_related(p_dbg_eng_bkpt,
                                                                    p_ctxt_bkpt,
                                                                    p_excp_stack_frame,
                                                                    DBG_OS_STACK_FRAME_TYPE_EXCEPTION);
    
    }     
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (remove_bkpt == NU_TRUE))
    {
        /* Remove the breakpoint. */
        
        dbg_status = dbg_eng_bkpt_breakpoint_remove(p_dbg_eng_bkpt,
                                                    p_addr_bkpt,
                                                    p_ctxt_bkpt,
                                                    ctxt_bkpt_index);

    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_software
*
*   DESCRIPTION
*
*       This function handles a software breakpoint exception.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to component control block.
*
*       p_os_exec_ctxt - Pointer to the OS execution context that 
*                        breakpoint occurred on (may be a thread or HISR).
*
*       p_excp_stack_frame - Pointer to exception stack frame for OS
*                            execution context.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_software(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                                           VOID *                       p_os_exec_ctxt,
                                                           VOID *                       p_excp_stack_frame)
{
    DBG_STATUS                      dbg_status;    
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Ensure that the OS execution context is valid. */

    if (p_os_exec_ctxt == NU_NULL)
    {
        /* ERROR: Invalid OS execution context. */
        
        dbg_status = DBG_STATUS_INVALID_CONTEXT;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine how to proceed based on the execution context type 
           (thread or HISR). */               
        
        if (((NU_TASK *)p_os_exec_ctxt) -> tc_id == TC_TASK_ID)
        {
            dbg_status = dbg_eng_bkpt_breakpoint_handler_software_thread(p_dbg_eng_bkpt,
                                                                         (NU_TASK *)p_os_exec_ctxt,
                                                                         p_excp_stack_frame);

        }
        else
        {
            dbg_status = dbg_eng_bkpt_breakpoint_handler_software_other(p_dbg_eng_bkpt,
                                                                        p_os_exec_ctxt,
                                                                        p_excp_stack_frame);   
            
        }
        
    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_hardware_singlestep
*
*   DESCRIPTION
*
*       This function handles the situation where a hardware single-step 
*       has been encountered.  Note that the hardware single-step
*       encountered may be in support of either a single-step operation or
*       a skip-over operation (i.e. its not only used for stepping...).
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control block.
*
*       p_os_thread - Pointer to the thread control block that the
*                     exception occurred on.
*
*       p_excp_stack_frame - Pointer to exception stack frame for OS
*                            execution context.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*       
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_hardware_singlestep(DBG_ENG_BKPT_CB *        p_dbg_eng_bkpt,
                                                                      NU_TASK *                p_os_thread,
                                                                      VOID *                   p_excp_stack_frame)
{
    DBG_STATUS                      dbg_status;
    DBG_OS_OPC_CMD_PARAM            os_opc_cmd_param;     
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt; 
    UINT                            ctxt_bkpt_index;
    BOOLEAN                         suspend_exec_ctxt;
    BOOLEAN                         remove_bkpt;
    BOOLEAN                         update_related_bkpts;    

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Get the Program Counter (PC) for the specified context. */
    
    os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_NEXT_INST_PTR;
    os_opc_cmd_param.op_param.get_next_inst_ptr.p_stack_context = p_excp_stack_frame;
    os_opc_cmd_param.op_param.get_next_inst_ptr.stack_context_type = DBG_OS_STACK_FRAME_TYPE_EXCEPTION;    
    
    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);
    
    if (dbg_status == DBG_STATUS_OK)
    {

        /* Set initial state of context breakpoint pointer. */
        
        p_ctxt_bkpt = NU_NULL;
    
        /* Attempt to find a matching context breakpoint associated with the
           the OS thread. */
           
        p_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                               &p_dbg_eng_bkpt -> hw_step_ctxt_bkpt_set,
                                                               p_os_thread,
                                                               DBG_ENG_BREAKPOINT_TYPE_NONE,
                                                               NU_NULL,
                                                               &ctxt_bkpt_index);
        
        /* Determine if a matching context breakpoint was found. */
           
        if (p_ctxt_bkpt != NU_NULL)
        {              
            /* Found matching context breakpoint.  The OS thread that hit the 
               breakpoint is the one the step operation was for. */        
            
            /* Determine the type of context breakpoint. */
            
            switch (p_ctxt_bkpt -> type) 
            {
                case DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP:
                {
                    /* These types of breakpoint are used to track an assembly
                       -level step operations performed using hardware-based 
                       stepping. */                
                
                    /* Remove the context breakpoint after suspending the 
                       context and update any associated context 
                       breakpoints. */
                       
                    suspend_exec_ctxt = NU_TRUE;                   
                    remove_bkpt = NU_TRUE;                                        
                    update_related_bkpts = NU_TRUE;
                        
                    break;
    
                } 
                 
                case DBG_ENG_BREAKPOINT_TYPE_SKIPOVER:
                {
                    /* These types of breakpoint are used to effect a skip
                       -over operation performed using hardware-based 
                       stepping. */                
                
                    /* Remove the context breakpoint and update any associated
                       context breakpoints but do not suspend execution. */
                       
                    suspend_exec_ctxt = NU_FALSE;                   
                    remove_bkpt = NU_TRUE;                                        
                    update_related_bkpts = NU_TRUE;
                        
                    break;
    
                }              
                 
                case DBG_ENG_BREAKPOINT_TYPE_ACTIVE:
                case DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP:
                case DBG_ENG_BREAKPOINT_TYPE_HIT:
                default:
                {
                    /* ERROR: Invalid context breakpoint type. */
    
                    /* ERROR RECOVERY: Indicate the thread that the exception 
                       occurred on should be suspended.  Do not change the 
                       state of anything in the breakpoint system since this 
                       is an unknown situation. */
     
                    suspend_exec_ctxt = NU_TRUE;
                    remove_bkpt = NU_FALSE;                    
                    update_related_bkpts = NU_FALSE;
                    
                    break;
    
                } 
                 
            }             
        
        }
        else
        {
            /* ERROR: No matching context breakpoint. */                        
        
            /* ERROR RECOVERY: Remove the breakpoint but do not update any 
               related breakpoints.  Execution should be allowed to 
               continue. */
               
            remove_bkpt = NU_TRUE;                           
            suspend_exec_ctxt = NU_FALSE;
            update_related_bkpts = NU_FALSE;                      
               
        }             

    }

    if ((dbg_status == DBG_STATUS_OK) &&
        (update_related_bkpts == NU_TRUE))
    {
        /* Update related breakpoints. */    
    
        dbg_status = dbg_eng_bkpt_breakpoint_handler_update_related(p_dbg_eng_bkpt,
                                                                    p_ctxt_bkpt,
                                                                    p_excp_stack_frame,
                                                                    DBG_OS_STACK_FRAME_TYPE_EXCEPTION);
      
    } 
            
    if ((dbg_status == DBG_STATUS_OK) &&
        (suspend_exec_ctxt == NU_TRUE))
    {
        /* Suspend execution context. */
            
        dbg_status = dbg_eng_bkpt_breakpoint_handler_suspend_execution(p_dbg_eng_bkpt,
                                                                       p_ctxt_bkpt,
                                                                       p_os_thread);
            
    }     
    
    if ((dbg_status == DBG_STATUS_OK) &&
        (remove_bkpt == NU_TRUE))
    {
        /* Remove any references to the context breakpoint from any other 
           breakpoint in the system. */

        dbg_status = dbg_eng_bkpt_breakpoints_erase_references(p_dbg_eng_bkpt,
                                                                p_ctxt_bkpt);
        
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Remove the breakpoint from the hardware single-step
               (context) breakpoint list. */
               
            dbg_status = DBG_SET_Node_Remove(&p_dbg_eng_bkpt -> hw_step_ctxt_bkpt_set,
                                             DBG_SET_LOCATION_INDEX,
                                             (DBG_SET_LOCATION_VALUE)ctxt_bkpt_index,
                                             (DBG_SET_NODE **)&p_ctxt_bkpt);
            
        } 
        
        if (dbg_status == DBG_STATUS_OK)
        {    
            /* Delete the context breakpoint. */
            
            dbg_eng_bkpt_context_breakpoint_delete(p_dbg_eng_bkpt,
                                                   p_ctxt_bkpt);
    
        }  

    } 
            
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_handler_hardware
*
*   DESCRIPTION
*
*       This function handles a breakpoint exception.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to component control block.
*
*       p_os_exec_ctxt - Pointer to the OS execution context that 
*                        breakpoint occurred on (may be a thread or HISR).
*
*       p_excp_stack_frame - Pointer to exception stack frame for OS
*                            execution context.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_handler_hardware(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                                           VOID *                       p_os_exec_ctxt,
                                                           VOID *                       p_excp_stack_frame)
{
    DBG_STATUS                      dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Ensure that the OS execution context is valid. */

    if (p_os_exec_ctxt == NU_NULL)
    {
        /* ERROR: Invalid OS execution context. */
        
        dbg_status = DBG_STATUS_INVALID_CONTEXT;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Call handler for a thread. */
        
        dbg_status = dbg_eng_bkpt_breakpoint_handler_hardware_singlestep(p_dbg_eng_bkpt,
                                                                         (NU_TASK *)p_os_exec_ctxt,
                                                                         p_excp_stack_frame);                
        
    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_setup_skipover
*
*   DESCRIPTION
*
*       This function sets up a skip-over breakpoint for a given 
*       breakpoint.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control
*                        block.
*
*       p_addr_bkpt - The primary address breakpoint.  May be NULL for a
*                     single step operation.
*
*       p_ctxt_bkpt - The primary context breakpoint.  May be NULL for a 
*                     single step operation.
*
*       p_os_thread - Pointer to the target thread.
*
*       p_stack_frame - Pointer to the stack frame.
*
*       stack_frame_type - The type of stack frame.
*
*       exec_ctxt_id - ID of the execution context that the breakpoint is
*                      on (scope of the breakpoint).
*
*       susp_exec_ctxt_id - ID of the execution context that will be 
*                           suspended when the breakpoint hits.
*
*       hit_func - Callback for breakpoint hit.
*
*       hit_func_param- Parameter value to pass to hit callback.
*
*       next_inst - Pointer to the next 1 instruction.
*       
*       p_next_inst - Pointer to the next 2 instruction.
*
*       create_bkpt_type - Indicates if created breakpoint is a skip-over
*                          or single step.
*
*       p_p_twin_addr_bkpt - Return parameter that will contain a pointer
*                            to a twin address breakpoint if one is found
*                            or NULL otherwise.
*
*       p_p_twin_ctxt_bkpt - Return parameter that will contain a pointer
*                            to a twin context breakpoint if one is found
*                            or NULL otherwise.
*
*       p_p_created_addr_bkpt - Return parameter that will contain a
*                               pointer to a created address breakpoint if
*                               one was needed or NULL otherwise.
*
*       p_p_created_ctxt_bkpt - Return parameter that will contain a
*                               pointer to a created context breakpoint if
*                               one was needed or NULL otherwise.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_setup_skipover(DBG_ENG_BKPT_CB *                 p_dbg_eng_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT *      p_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT *      p_ctxt_bkpt,
                                                         NU_TASK *                         p_os_thread,
                                                         VOID *                            p_stack_frame,
                                                         DBG_OS_STACK_FRAME_TYPE           stack_frame_type,
                                                         DBG_THREAD_ID                     exec_ctxt_id,
                                                         DBG_THREAD_ID                     susp_exec_ctxt_id,
                                                         DBG_ENG_BKPT_HIT_FUNC             hit_func,
                                                         DBG_ENG_BKPT_HIT_FUNC_PARAM       hit_func_param,
                                                         DBG_OS_OPCODE                     next_inst, 
                                                         DBG_OS_OPCODE *                   p_next_inst,
                                                         DBG_ENG_BREAKPOINT_TYPE           create_bkpt_type,
                                                         DBG_ENG_ADDRESS_BREAKPOINT **     p_p_twin_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT **     p_p_twin_ctxt_bkpt,
                                                         DBG_ENG_ADDRESS_BREAKPOINT **     p_p_created_addr_bkpt,
                                                         DBG_ENG_CONTEXT_BREAKPOINT **     p_p_created_ctxt_bkpt)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_twin_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_twin_ctxt_bkpt;  
    DBG_ENG_ADDRESS_BREAKPOINT *    p_created_addr_bkpt;    
    DBG_ENG_CONTEXT_BREAKPOINT *    p_created_ctxt_bkpt;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_link_addr_bkpt;    
    UINT                            ctxt_bkpt_index;
    BOOLEAN                         create_addr_bkpt;
    BOOLEAN                         create_ctxt_bkpt;
    DBG_OS_REG_CMD_PARAM            os_reg_cmd_param; 
    DBG_OS_REG_INT_STATE            int_state;            
    
    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;        

    /* Initialize local variables. */
    
    p_twin_ctxt_bkpt = NU_NULL;
    p_link_addr_bkpt = NU_NULL;

    /* Determine if the next instruction is executable (not NULL). */
    
    if (next_inst != NU_NULL) 
    {
        /* The next instruction is executable. */        
        
        /* Save the interrupt state from stack frame. */
    
        os_reg_cmd_param.op = DBG_OS_REG_OP_INT_SAVE;
        os_reg_cmd_param.op_param.int_sav.p_stack_context = p_stack_frame;
        os_reg_cmd_param.op_param.int_sav.stack_context_type = stack_frame_type;
        os_reg_cmd_param.op_param.int_sav.p_int_state = &int_state;
    
        dbg_status = DBG_OS_Reg_Command(&os_reg_cmd_param);

        if (dbg_status == DBG_STATUS_OK)
        {
            /* Disable interrupts in stack frame. */
            
            os_reg_cmd_param.op = DBG_OS_REG_OP_INT_DISABLE;
            os_reg_cmd_param.op_param.int_dsbl.p_stack_context = p_stack_frame;
            os_reg_cmd_param.op_param.int_dsbl.stack_context_type = stack_frame_type;
    
            dbg_status = DBG_OS_Reg_Command(&os_reg_cmd_param);                
            
        }         
        
        if (dbg_status == DBG_STATUS_OK)
        {        
            /* Attempt to find a twin address breakpoint at the next
               instruction address. */
    
            p_twin_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_address(p_dbg_eng_bkpt,
                                                                                 p_dbg_eng_bkpt -> p_all_addr_bkpts,            
                                                                                 p_next_inst);
               
            if (p_twin_addr_bkpt == NU_NULL) 
            {
                /* No twin address breakpoint exists.  Both a twin address
                   breakpoint and a twin context breakpoint will need to be
                   created. */
                
                create_addr_bkpt = NU_TRUE;
                create_ctxt_bkpt = NU_TRUE;
                
            }
            else
            {
                /* A twin address breakpoint exists.  No address breakpoint
                   needs to be created. */
    
                create_addr_bkpt = NU_FALSE;
                
                /* Attempt to find a twin context breakpoint associated with
                   the twin address breakpoint. */
                
                p_twin_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                                            &p_twin_addr_bkpt -> ctxt_bkpt_set,
                                                                            p_os_thread,
                                                                            DBG_ENG_BREAKPOINT_TYPE_NONE,
                                                                            NU_NULL,
                                                                            &ctxt_bkpt_index);
                
                if (p_twin_ctxt_bkpt == NU_NULL)
                {
                    /* No twin context breakpoint exists.  One will need to
                       be created. */
                    
                    create_ctxt_bkpt = NU_TRUE;
                    
                    /* Indicate that the created context breakpoint will be
                       linked to the twin address breakpoint found. */
                       
                    p_link_addr_bkpt = p_twin_addr_bkpt;
                    
                }
                else
                {
                    /* A twin context breakpoint exists. */
    
                    create_ctxt_bkpt = NU_FALSE;
                
                    /* Setup the twin breakpoint to restore the interrupt
                       state when it is hit. */
                       
                    p_twin_ctxt_bkpt -> int_state_restore = NU_TRUE;
                    p_twin_ctxt_bkpt -> int_state = int_state;
                
                    /* Set the twin breakpoint's auxiliary to be the primary
                       context breakpoint. */
                       
                    p_twin_ctxt_bkpt -> p_aux_ctxt_bkpt = p_ctxt_bkpt;
                    
                    /* Determine how to proceed based on whether this is a
                       single-step skip-over setup and the type of the twin
                       context breakpoint. */
                    
                    if ((create_bkpt_type == DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP) &&
                        (p_twin_ctxt_bkpt -> type == DBG_ENG_BREAKPOINT_TYPE_ACTIVE))
                    {
                        /* Twin context breakpoint is "active". */
        
                        /* Change breakpoint's type to "active-step".  This 
                           will ensure that execution will stop even if the 
                           breakpoint has a pass-count value or a conditional 
                           function. */
                        
                        p_twin_ctxt_bkpt -> type = DBG_ENG_BREAKPOINT_TYPE_ACTIVESTEP;
                                         
                        /* Set the breakpoint's auxiliary execution context ID
                           to be the specified suspension execution context ID
                           value. */
                                       
                         p_twin_ctxt_bkpt -> aux_exec_ctxt_id = susp_exec_ctxt_id;              
                                       
                    }                 
                    
                } 
                
            } 
         
        } 
         
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Determine if an address breakpoint is to be created. */
        
            if (create_addr_bkpt == NU_TRUE)
            {
                /* Create an address breakpoint at the next instruction
                   address. */
            
                dbg_status = dbg_eng_bkpt_address_breakpoint_create(p_dbg_eng_bkpt,
                                                                    p_next_inst,
                                                                    &p_created_addr_bkpt);               
                   
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Set the link address breakpoint to be the newly 
                       created breakpoint. */
                       
                    p_link_addr_bkpt = p_created_addr_bkpt;            
                
                }             
        
            } 
            else
            {
                /* Indicate no address breakpoint was created. */
                
                p_created_addr_bkpt = NU_NULL;
            
            } 
            
        } 
        
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Determine if a context breakpoint is to be created. */
            
            if (create_ctxt_bkpt == NU_TRUE)
            {
                /* Create a new context breakpoint with the primary
                   context breakpoint as its auxiliary.  It is associated
                   with the link address breakpoint. */
                   
                dbg_status = dbg_eng_bkpt_context_breakpoint_create(p_dbg_eng_bkpt,
                                                                    exec_ctxt_id,
                                                                    p_os_thread,
                                                                    create_bkpt_type,                                                                     
                                                                    susp_exec_ctxt_id,
                                                                    DBG_ENG_BKPT_EVAL_FUNC_NONE,
                                                                    DBG_ENG_BKPT_EVAL_FUNC_PARAM_NONE,
                                                                    hit_func,
                                                                    hit_func_param,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    p_ctxt_bkpt,
                                                                    NU_NULL,
                                                                    p_link_addr_bkpt,
                                                                    &p_created_ctxt_bkpt);
    
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Setup the created breakpoint to restore the
                       interrupt state when it is hit. */
                       
                    p_created_ctxt_bkpt -> int_state_restore = NU_TRUE;
                    p_created_ctxt_bkpt -> int_state = int_state;                    
                    
                    /* Add the new context breakpoint to the address 
                       breakpoint indicated by the link pointer. */
                   
                    dbg_status = DBG_SET_Node_Add(&p_link_addr_bkpt -> ctxt_bkpt_set,
                                                  DBG_SET_LOCATION_DEFAULT,
                                                  NU_NULL,
                                                  (DBG_SET_NODE_ID)exec_ctxt_id,
                                                  (DBG_SET_NODE *)p_created_ctxt_bkpt);
                 
                }             

            }
            else
            {
                /* Indicate no context breakpoint was created. */
                
                p_created_ctxt_bkpt = NU_NULL;            
            
            } 
                
        }         
        
        if (dbg_status == DBG_STATUS_OK)
        {
            /* Update return parameter. */
            
            *p_p_twin_addr_bkpt = p_twin_addr_bkpt;
            *p_p_twin_ctxt_bkpt = p_twin_ctxt_bkpt;
            *p_p_created_addr_bkpt = p_created_addr_bkpt;
            *p_p_created_ctxt_bkpt = p_created_ctxt_bkpt;            

        } 
        
    } 
    else
    {
        /* The next instruction is not executable. */

        /* In this case there is no need for either the twin breakpoints 
           or the created breakpoints. */
    
            *p_p_twin_addr_bkpt = NU_NULL;
            *p_p_twin_ctxt_bkpt = NU_NULL;
            *p_p_created_addr_bkpt = NU_NULL;
            *p_p_created_ctxt_bkpt = NU_NULL; 

        /* Indicates successful operation. */
        
        dbg_status = DBG_STATUS_OK;
        
    }             

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_bkpt_breakpoint_setup_skipover_crosslink
*
*   DESCRIPTION
*
*       This function sets up the cross-linking that occurs for a skip-over
*       breakpoint.
*
*   INPUTS
*
*       p_dbg_eng_bkpt - Pointer to the breakpoint component control
*                        block.
*
*       p_twin_ctxt_bkpt_array - Array of possible twin breakpoint
*                                pointers.
*
*       p_created_ctxt_bkpt_array - Array of possible created breakpoint 
*                               pointers.
*
*       bkpt_array_size - The size of the breakpoint arrays (same size for
                          both).
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_setup_skipover_crosslink(DBG_ENG_BKPT_CB *               p_dbg_eng_bkpt,
                                                                   DBG_ENG_CONTEXT_BREAKPOINT **   p_twin_ctxt_bkpt_array,
                                                                   DBG_ENG_CONTEXT_BREAKPOINT **   p_created_ctxt_bkpt_array,
                                                                   UINT                            bkpt_array_size)
{
    DBG_STATUS          dbg_status;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine if the Next 0 instruction breakpoint 
       was created. */
    
    if (p_created_ctxt_bkpt_array[0] != NU_NULL)
    {
        /* Next 0 instruction breakpoint was created. */
        
        /* Determine if there is an existing breakpoint for 
           the Next 1 instruction. */
           
        if (p_twin_ctxt_bkpt_array[1] != NU_NULL)
        {
            /* Next 1 instruction breakpoint exists. */

            /* Set existing Next 1 instruction breakpoint's twin to 
               be the created Next 0 instruction breakpoint. */
            
            p_twin_ctxt_bkpt_array[1] -> p_twin_ctxt_bkpt = p_created_ctxt_bkpt_array[0];

        } 

        /* Determine if there was a created breakpoint for
           the next 1 instruction. */
        
        if (p_created_ctxt_bkpt_array[1] != NU_NULL)
        {
            /* Next 1 instruction breakpoint created. */

            /* Set the created Next 1 instruction breakpoint's twin to
               be the created Next 0 instruction breakpoint. */
            
            p_created_ctxt_bkpt_array[1] -> p_twin_ctxt_bkpt = p_created_ctxt_bkpt_array[0];

        } 

    } 

    /* Determine if a Next 1 instruction breakpoint was created. */
    
    if (p_created_ctxt_bkpt_array[1] != NU_NULL)
    {
        /* Next 1 instruction breakpoint created. */

        /* Determine if there is an existing Next 0 breakpoint. */
        
        if (p_twin_ctxt_bkpt_array[0] != NU_NULL)
        {
            /* Next 0 breakpoint exists. */

            /* Set the existing Next 0 breakpoint's twin to be the
               created Next 1 instruction breakpoint. */
            
            p_twin_ctxt_bkpt_array[0] -> p_twin_ctxt_bkpt = p_created_ctxt_bkpt_array[1];

        } 

        /* Determine if a Next 0 instruction breakpoint was 
           created. */
        
        if (p_created_ctxt_bkpt_array[0] != NU_NULL)
        {
            /* Next 0 instruction breakpoint created. */

            /* Set the created Next 0 instruction breakpoint's twin to
               be the created Next 1 instruction breakpoint. */
            
            p_created_ctxt_bkpt_array[0] -> p_twin_ctxt_bkpt = p_created_ctxt_bkpt_array[1];

        } 
            
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       dbg_eng_bkpt_breakpoint_find
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function attempts to find a breakpoint in the breakpoint
*       system based on its characteristics.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_eng_bkpt - Pointer to the control block.
*
*       p_addr - Pointer to the address of the breakpoint.
*
*       exec_ctxt_id - Execution context ID of the breakpoint.  A value of
*                      DBG_THREAD_ID_NONE will return the next context
*                      breakpoint associated with the address breakpoint. 
*
*       p_p_addr_bkpt - Return parameter that will be updated point to the
*                      address breakpoint that associated with the ID value
*                      if the operation is successful.  If the operation 
*                      fails the value is undefined.
*
*       p_p_ctxt_bkpt - Return parameter that will be updated point to the
*                       context breakpoint that associated with the ID
*                       value if the operation is successful.  If the 
*                       operation fails the value is undefined.
*
*       p_ctxt_bkpt_index - Return parameter that will be updated to
*                           contain the index of the context breakpoint in
*                           the set if the operation is successful.  If 
*                           the operation fails the value is undefined.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - The specified breakpoint was
*                                         not found.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
static DBG_STATUS dbg_eng_bkpt_breakpoint_find(DBG_ENG_BKPT_CB *                p_dbg_eng_bkpt,
                                               DBG_OS_OPCODE *             p_addr,
                                               DBG_THREAD_ID                    exec_ctxt_id,
                                               DBG_ENG_ADDRESS_BREAKPOINT **    p_p_addr_bkpt,
                                               DBG_ENG_CONTEXT_BREAKPOINT **    p_p_ctxt_bkpt,
                                               UINT *                           p_ctxt_bkpt_index)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;
    BOOLEAN                         target_id_found;
    UINT                            ctxt_bkpt_index;
    
    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;    
    
    /* Set initial found state. */
    
    target_id_found = NU_FALSE;
    
    /* Set the current breakpoint to the start of the all address 
       breakpoints list. */
    
    p_addr_bkpt = p_dbg_eng_bkpt -> p_all_addr_bkpts;
    
    /* Iterate through all address breakpoints looking for the specified
       target address. */

    while ((p_addr_bkpt != NU_NULL) &&
           (target_id_found == NU_FALSE))
    {
        if (p_addr_bkpt -> address == p_addr)
        {
            target_id_found = NU_TRUE;
            
        }
        else
        {
            /* Move to the next address breakpoint. */
            
            p_addr_bkpt = p_addr_bkpt -> next;

        }         
        
    } 

    if (target_id_found == NU_FALSE)
    {
        /* ERROR: No matching address breakpoint found. */
        
        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
        
    }     
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Found a matching address breakpoint.  Now search through the
           context breakpoints associated with the address breakpoint
           for the target execution context ID value. */
        
        /* Reset the context breakpoint index and target ID found. */
        
        ctxt_bkpt_index = 0;
        target_id_found = NU_FALSE;
    
        /* Attempt to get the first context breakpoint from the set. */
        
        dbg_status = DBG_SET_Node_Find(&p_addr_bkpt -> ctxt_bkpt_set,
                                       DBG_SET_LOCATION_HEAD,
                                       DBG_SET_NODE_ID_ANY,
                                       (DBG_SET_NODE **)&p_ctxt_bkpt);
        
        while ((dbg_status == DBG_STATUS_OK) &&
               (target_id_found == NU_FALSE))
        {
            /* Determine if the context breakpoint is a match for the 
               target execution context ID. */
               
            if ((p_ctxt_bkpt -> exec_ctxt_id == exec_ctxt_id) ||
                (exec_ctxt_id == DBG_THREAD_ID_NONE))
            {
                /* Indicate the context breakpoint was found. */
                
                target_id_found = NU_TRUE;
            
            }
            else
            {
                /* Attempt to get another context breakpoint from the set. */
                
                dbg_status = DBG_SET_Node_Find(&p_addr_bkpt -> ctxt_bkpt_set,
                                               DBG_SET_LOCATION_NEXT_LINEAR,
                                               (DBG_SET_LOCATION_VALUE)p_ctxt_bkpt,
                                               (DBG_SET_NODE **)&p_ctxt_bkpt);       
                
                if (dbg_status == DBG_STATUS_OK)
                {
                    /* Update the context breakpoint index. */
                    
                    ctxt_bkpt_index++;

                } 
                    
            }             
            
        } 
        
        if (target_id_found == NU_TRUE)
        {
            /* Indicate target found (and reset status after search). */
            
            dbg_status = DBG_STATUS_OK;
            
        }
        else
        {
            /* ERROR: No matching address breakpoint found. */
            
            dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
            
        }          
        
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update the return parameter values. */    
    
        *p_p_addr_bkpt = p_addr_bkpt;
        *p_p_ctxt_bkpt = p_ctxt_bkpt;
        *p_ctxt_bkpt_index = ctxt_bkpt_index;
        
    } 
    
    return (dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Initialize
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function initializes the component.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_eng_bkpt - Pointer to the control block.
*
*       p_param - Pointer to the parameter structure.
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
DBG_STATUS  DBG_ENG_BKPT_Initialize(DBG_ENG_BKPT_CB *           p_dbg_eng_bkpt,
                                    DBG_ENG_BKPT_INIT_PARAM *   p_param)
{
    DBG_STATUS                      dbg_status;    
    UINT                            i;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_OS_EXEC_CMD_PARAM           os_exec_cmd_param;    

    /* Set initial function status. */ 
    
    dbg_status = DBG_STATUS_OK;

    /* Set pointer to debug service. */

    p_dbg_eng_bkpt -> p_dbg_eng = p_param -> p_dbg_eng; 
    
    /* Setup illegal instruction handling behavior (initial state). */
    
    p_dbg_eng_bkpt -> ill_inst_stops_thd = NU_FALSE;
    p_dbg_eng_bkpt -> ill_inst_replace = NU_TRUE;
    p_dbg_eng_bkpt -> ill_inst_hit_func = NU_NULL;
    p_dbg_eng_bkpt -> ill_inst_hit_func_param = NU_NULL;
    
    /* Initialize the all breakpoints list. */

    p_dbg_eng_bkpt -> p_all_addr_bkpts = NU_NULL;
    
    /* Initialize the free breakpoints list. */

    p_dbg_eng_bkpt -> p_free_addr_bkpts = NU_NULL;
    
    /* Loop through all breakpoint structures in the breakpoint 
       array. */

    for (i = 0; i < CFG_NU_OS_SVCS_DBG_BREAKPOINT_MAX; i++) 
    {
        /* Get the current breakpoint. */

        p_addr_bkpt = &p_dbg_eng_bkpt -> addr_bkpts[i];
    
        /* Initialize the current breakpoint. */

        dbg_eng_bkpt_address_breakpoint_initialize(p_dbg_eng_bkpt,
                                                   p_addr_bkpt);
        
        /* Add the current breakpoint to the free breakpoints list. */
        
        dbg_eng_bkpt_address_breakpoint_list_add(p_dbg_eng_bkpt,
                                                 &p_dbg_eng_bkpt -> p_free_addr_bkpts,
                                                 p_addr_bkpt);

    }

    /* Initialize the hardware single-step context breakpoint set. */
    
    dbg_status = DBG_SET_Initialize(&p_dbg_eng_bkpt -> hw_step_ctxt_bkpt_set,
                                    DBG_SET_BEHAVIOR_FIFO); 
       
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Retrieve the status of the OS execution system. */
        
        os_exec_cmd_param.op = DBG_OS_EXEC_OP_STATUS;
        dbg_status = DBG_OS_Exec_Command(&os_exec_cmd_param);
    
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup hardware stepping enabled setting based on the state of
           the OS execution system. */

        p_dbg_eng_bkpt -> hw_step_supported = os_exec_cmd_param.op_param.status.hw_step_supported;

    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Initialize the free context breakpoint set. */
        
        dbg_status = DBG_SET_Initialize(&p_dbg_eng_bkpt -> free_ctxt_bkpts_set,
                                        DBG_SET_BEHAVIOR_FIFO); 
        
    }
    
    /* Add all context breakpoints to the free context breakpoints
       set. */
    
    i = 0;
    while ((dbg_status == DBG_STATUS_OK) &&
           (i < CFG_NU_OS_SVCS_DBG_BREAKPOINT_MAX))
    {
        dbg_status = DBG_SET_Node_Add(&p_dbg_eng_bkpt -> free_ctxt_bkpts_set,
                                      DBG_SET_LOCATION_DEFAULT,
                                      NU_NULL,
                                      (DBG_SET_NODE_ID)i,
                                      (DBG_SET_NODE *)(&p_dbg_eng_bkpt -> ctxt_bkpts[i]));

        i++;

    }
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Terminate
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function terminates the component.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_eng_bkpt - Pointer to the control block.
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
DBG_STATUS  DBG_ENG_BKPT_Terminate(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                   DBG_ENG_BKPT_TERM_PARAM *    p_param)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_BKPT_RMV_ALL_PARAM      bkpt_rmv_all_param;

    /* Set initial function status. */ 
    
    dbg_status = DBG_STATUS_OK;

    /* Remove all existing breakpoints. */
    
    dbg_status = DBG_ENG_BKPT_Breakpoints_Remove_All(p_dbg_eng_bkpt,
                                                     &bkpt_rmv_all_param);

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Control
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function controls the breakpoint system.
*                                                                                                 
*   INPUTS   
*
*       p_dbg_eng_bkpt - Pointer to the control block.
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
DBG_STATUS DBG_ENG_BKPT_Control(DBG_ENG_BKPT_CB *             p_dbg_eng_bkpt,
                                DBG_ENG_BKPT_CONTROL_PARAM *  p_param)
{
    DBG_STATUS                      dbg_status;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Determine how to proceed based on the control ID value. */
    
    switch (p_param -> id)
    {
        case DBG_ENG_BKPT_CONTROL_ID_ILLEGAL_INST :
        {
            /* Update the breakpoint system illegal instruction 
               behavior. */
            
            p_dbg_eng_bkpt -> ill_inst_stops_thd = p_param -> id_param.ill_inst.stop_thread;
            p_dbg_eng_bkpt -> ill_inst_replace = p_param -> id_param.ill_inst.replaced_with_bkpt;
            p_dbg_eng_bkpt -> ill_inst_hit_func = p_param -> id_param.ill_inst.hit_func;
            p_dbg_eng_bkpt -> ill_inst_hit_func_param = p_param -> id_param.ill_inst.hit_func_param;
            
            break;
            
        }         
        
        case DBG_ENG_BKPT_CONTROL_ID_NONE :
        default :
        {
            /* ERROR: Invalid breakpoint system control ID. */
            
            dbg_status = DBG_STATUS_INVALID_ID;
            
            break;
        
        } 
        
    } 
    
    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Breakpoint_Set_Skip_Step
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function sets up a skip-over or single step breakpoint.  The
*       only functional difference is that a skip-over does not stop 
*       execution while a single step does.  The values of the parameters
*       determine the behavior of this function (skip-over or single-
*       step creation).
*
*       NOTE: Skip-over breakpoints are only created if there is a primary
*       breakpoint!  
*                                                                                                 
*   INPUTS                                                               
*                            
*       p_dbg_eng_bkpt - Pointer to the control block.
*
*       p_param - Pointer to the parameter structure.
*
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates that a skip-over
*                                      breakpoint creation was requested
*                                      but there was no primary
*                                      breakpoint.
*
*       DBG_STATUS_INVALID_BREAKPOINT - Indicates an invalid primary
*                                       breakpoint was discovered at the
*                                       execution resume address of the OS
*                                       thread.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_ENG_BKPT_Breakpoint_Set_Skip_Step(DBG_ENG_BKPT_CB *                      p_dbg_eng_bkpt,
                                                 DBG_ENG_BKPT_SET_SKIP_STEP_PARAM *     p_param)
{
    DBG_STATUS                          dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *        p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *        p_ctxt_bkpt;
    UINT                                ctxt_bkpt_index;
    DBG_OS_OPCODE *                     pc;
    DBG_OS_OPC_CMD_PARAM                os_opc_cmd_param;   
    DBG_ENG_BREAKPOINT_TYPE             ctxt_bkpt_type;

    /* Set initial status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Get the Program Counter (PC) for the specified context. */
   
    os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_NEXT_INST_PTR;
    os_opc_cmd_param.op_param.get_next_inst_ptr.p_stack_context = p_param -> p_stack_frame;
    os_opc_cmd_param.op_param.get_next_inst_ptr.stack_context_type = p_param -> stack_frame_type;    
    
    dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);   
    
    if (dbg_status == DBG_STATUS_OK)
    {
        pc = (DBG_OS_OPCODE *)os_opc_cmd_param.op_param.get_next_inst_ptr.p_next_inst;        
        
        /* Attempt to find an existing address breakpoint at the PC for 
           the specified context.  If one exists it will be considered the
           "primary" breakpoint for which the skip-over breakpoints are
           being created.  Note that there may NOT be a primary breakpoint. */
        
        p_addr_bkpt = dbg_eng_bkpt_address_breakpoint_list_find_address(p_dbg_eng_bkpt,
                                                                        p_dbg_eng_bkpt -> p_all_addr_bkpts,
                                                                        pc);  
           
        if (p_addr_bkpt == NU_NULL)
        {
            /* No address breakpoint means no context breakpoints. */
            
            p_ctxt_bkpt = NU_NULL;
            
            /* Determine if this is an error situation based on whether a
               skip-over breakpoint is to be created. */
               
            if (p_param -> stop_exec_ctxt_id == DBG_THREAD_ID_NONE)
            {
                /* ERROR: Invalid creation of skip-over breakpoint with no
                   primary breakpoint. */
                   
                dbg_status = DBG_STATUS_INVALID_OPERATION;
                
            } 
            
        }
        else
        {
            /* Attempt to find any context breakpoint associated with the
               primary address breakpoint (any one will do and there 
               should be at least one!) */
               
            p_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                                   &p_addr_bkpt -> ctxt_bkpt_set,
                                                                   NU_NULL,
                                                                   DBG_ENG_BREAKPOINT_TYPE_NONE,
                                                                   NU_NULL,
                                                                   &ctxt_bkpt_index);           

            if (p_ctxt_bkpt == NU_NULL)
            {
                /* ERROR: Address breakpoint with no matching context
                   breakpoint. */
                
                dbg_status = DBG_STATUS_INVALID_BREAKPOINT;
            
            }   
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Deactivate the address breakpoint associated with the
                   primary (context) breakpoint. */ 
                
                dbg_status = dbg_eng_bkpt_address_breakpoint_deactivate(p_dbg_eng_bkpt,
                                                                        p_ctxt_bkpt -> p_addr_bkpt);
                   
            } 
    
        } 

    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {            
        /* Setup the context breakpoint type based on the execution 
           context to be stopped. */
           
        if (p_param -> stop_exec_ctxt_id == DBG_THREAD_ID_NONE)
        {
            ctxt_bkpt_type = DBG_ENG_BREAKPOINT_TYPE_SKIPOVER;            
        }
        else
        {
            ctxt_bkpt_type = DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP;
            
        } 
        
        /* Determine how to proceed based on whether hardware 
           single-stepping is supported or not. */
           
        if (p_dbg_eng_bkpt -> hw_step_supported == NU_TRUE)
        {
            DBG_ENG_CONTEXT_BREAKPOINT *    p_created_ctxt_bkpt;
            DBG_OS_EXEC_CMD_PARAM           os_exec_cmd_param;
                    
            /* Use hardware single-step for operation. */
    
            /* Create a new hardware single-step (context) 
               breakpoint.  Note  that this type of context 
               breakpoint has no associated address breakpoint.  
               The breakpoint type is skip-over. */
            
            dbg_status = dbg_eng_bkpt_context_breakpoint_create(p_dbg_eng_bkpt,
                                                                p_param -> step_exec_ctxt_id,
                                                                p_param -> p_os_thread,
                                                                ctxt_bkpt_type,                                                                     
                                                                p_param -> stop_exec_ctxt_id,
                                                                DBG_ENG_BKPT_EVAL_FUNC_NONE,
                                                                DBG_ENG_BKPT_EVAL_FUNC_PARAM_NONE,
                                                                p_param -> hit_func,
                                                                p_param -> hit_func_param,
                                                                0,
                                                                0,
                                                                0,
                                                                p_ctxt_bkpt,
                                                                NU_NULL,
                                                                NU_NULL,
                                                                &p_created_ctxt_bkpt);    
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Add the new context breakpoint to the hardware 
                   single-step breakpoint list. */
               
                dbg_status = DBG_SET_Node_Add(&p_dbg_eng_bkpt -> hw_step_ctxt_bkpt_set,
                                              DBG_SET_LOCATION_DEFAULT,
                                              NU_NULL,
                                              (DBG_SET_NODE_ID)p_ctxt_bkpt -> exec_ctxt_id,
                                              (DBG_SET_NODE *)p_created_ctxt_bkpt);
            
            }            
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Setup target thread for hardware-based 
                   stepping. */
                
                os_exec_cmd_param.op = DBG_OS_EXEC_OP_HARDWARE_SINGLESTEP;
                os_exec_cmd_param.op_param.hw_step.p_stack_context = p_param -> p_stack_frame;
                os_exec_cmd_param.op_param.hw_step.stack_context_type = p_param -> stack_frame_type;
                dbg_status = DBG_OS_Exec_Command(&os_exec_cmd_param);
            
            }  
            
        }
        else
        {
            DBG_ENG_ADDRESS_BREAKPOINT *    twin_addr_bkpt_array[2];
            DBG_ENG_CONTEXT_BREAKPOINT *    twin_ctxt_bkpt_array[2];
            DBG_ENG_ADDRESS_BREAKPOINT *    created_addr_bkpt_array[2];    
            DBG_ENG_CONTEXT_BREAKPOINT *    created_ctxt_bkpt_array[2];        
            DBG_OS_OPCODE                   next_inst_0;    
            DBG_OS_OPCODE *                 p_next_inst_0;
            DBG_OS_OPCODE                   next_inst_1;
            DBG_OS_OPCODE *                 p_next_inst_1;
            UINT                            i;

            /* Use software breakpoints for single-step operation. */

            /* Get the next instruction 0 pointer using the current PC. */
        
            os_opc_cmd_param.op = DBG_OS_OPC_OP_GET_BKPT_ADDRESS;
            os_opc_cmd_param.op_param.get_bkpt_address.p_next_op_code = (VOID *)pc;
            os_opc_cmd_param.op_param.get_bkpt_address.p_stack_context = p_param -> p_stack_frame;
            os_opc_cmd_param.op_param.get_bkpt_address.stack_context_type = p_param -> stack_frame_type;    
            
            dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);        
        
            if (dbg_status == DBG_STATUS_OK)
            {               
                p_next_inst_0 = (DBG_OS_OPCODE *)os_opc_cmd_param.op_param.get_bkpt_address.p_bkpt_address;                
                
                /* Set next instruction 0 value using the next
                   instruction 0 pointer. */      
                             
                os_opc_cmd_param.op = DBG_OS_OPC_OP_READ;
                os_opc_cmd_param.op_param.read.p_address = (VOID *)p_next_inst_0;
                
                dbg_status = DBG_OS_Opc_Command(&os_opc_cmd_param);                             
            
            } 
            
            if (dbg_status == DBG_STATUS_OK)
            {            
                next_inst_0 = (DBG_OS_OPCODE)os_opc_cmd_param.op_param.read.op_code;
                
                /* Set next instruction 1 value. */
                
                next_inst_1 = NU_NULL;
                
                /* Set next instruction 1 address. */
                
                p_next_inst_1 = (DBG_OS_OPCODE *)NU_NULL;        

            } 
        
            if (dbg_status == DBG_STATUS_OK)
            {                            
                /* Setup a skip-over breakpoint for the next 0 
                   instruction. */
                
                dbg_status = dbg_eng_bkpt_breakpoint_setup_skipover(p_dbg_eng_bkpt,
                                                                    p_addr_bkpt,
                                                                    p_ctxt_bkpt,
                                                                    p_param -> p_os_thread,
                                                                    p_param -> p_stack_frame,
                                                                    p_param -> stack_frame_type,
                                                                    p_param -> step_exec_ctxt_id,
                                                                    p_param -> stop_exec_ctxt_id,
                                                                    p_param -> hit_func,
                                                                    p_param -> hit_func_param,
                                                                    next_inst_0,
                                                                    p_next_inst_0,
                                                                    ctxt_bkpt_type,
                                                                    &twin_addr_bkpt_array[0],
                                                                    &twin_ctxt_bkpt_array[0],                                                                     
                                                                    &created_addr_bkpt_array[0],
                                                                    &created_ctxt_bkpt_array[0]);
            
            } 
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Setup a skip-over breakpoint for the next 1 
                   instruction. */
                
                dbg_status = dbg_eng_bkpt_breakpoint_setup_skipover(p_dbg_eng_bkpt,
                                                                    p_addr_bkpt,
                                                                    p_ctxt_bkpt,
                                                                    p_param -> p_os_thread,
                                                                    p_param -> p_stack_frame,
                                                                    p_param -> stack_frame_type,
                                                                    p_param -> step_exec_ctxt_id,
                                                                    p_param -> stop_exec_ctxt_id,
                                                                    p_param -> hit_func,
                                                                    p_param -> hit_func_param,
                                                                    next_inst_1,
                                                                    p_next_inst_1,
                                                                    ctxt_bkpt_type,
                                                                    &twin_addr_bkpt_array[1],
                                                                    &twin_ctxt_bkpt_array[1],
                                                                    &created_addr_bkpt_array[1],
                                                                    &created_ctxt_bkpt_array[1]);            
            
            } 
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Setup cross-referencing for the current 
                   breakpoint setup for next two instructions 
                   (called Next 0 and Next 1). */
                
                dbg_status = dbg_eng_bkpt_breakpoint_setup_skipover_crosslink(p_dbg_eng_bkpt,
                                                                              &twin_ctxt_bkpt_array[0],
                                                                              &created_ctxt_bkpt_array[0],
                                                                              2);
            
            } 

            if (dbg_status == DBG_STATUS_OK)
            {
                /* Activate any created address breakpoints. */
                
                i = 0;
                while ((dbg_status == DBG_STATUS_OK) &&
                       (i < 2))
                {
                    if (created_addr_bkpt_array[i] != NU_NULL)
                    {
                        /* Activate the address breakpoint. */
                        
                        dbg_status = dbg_eng_bkpt_address_breakpoint_activate(p_dbg_eng_bkpt,
                                                                              created_addr_bkpt_array[i]);       
        
                    }
                    
                    i++;
                
                }
                
            }

        } 

    } 
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Breakpoint_Set
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function sets a breakpoint.
*                                                                                                 
*   INPUTS   
*
*       p_dbg_eng_bkpt - Pointer to the control block.
*                 
*       p_param - Pointer to the parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_OUT_OF_MEMORY - Insufficient memory resources to set
*                                  the breakpoint.
*
*       DBG_STATUS_INVALID_THREAD - Invalid breakpoint hit thread 
*                                   specified.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_ENG_BKPT_Breakpoint_Set(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                       DBG_ENG_BKPT_SET_PARAM *       p_param)
{
    DBG_STATUS                      dbg_status;
    UINT                            count;
    VOID *                          exec_ctxt_os_data;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;    

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Determine if there are at least 2 free breakpoints. */
    
    count = dbg_eng_bkpt_address_breakpoint_list_count(p_dbg_eng_bkpt,
                                                       p_dbg_eng_bkpt -> p_free_addr_bkpts,
                                                       2);
    
    /* Ensure that there are enough available breakpoint structures to 
       fulfil the operation. */
    
    if (count < 2)
    {
        dbg_status = DBG_STATUS_OUT_OF_MEMORY;
        
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set the execution context OS data associated with the
           breakpoint taking into account the hit execution context ID
           value. */
        
        switch(p_param -> hit_exec_ctxt_id) 
        {
            case DBG_THREAD_ID_NONE : 
            {
                /* ERROR: Invalid thread ID. */
                
                dbg_status = DBG_STATUS_INVALID_THREAD;
                
                break;
                
            }
            
            case DBG_THREAD_ID_ALL :
            case DBG_THREAD_ID_ANY :
            {
                /* No OS data */
                
                exec_ctxt_os_data = NU_NULL;            

                break;
                
            }
            
            default :
            {
                /* OS data is the thread's OS control block. */
                            
                exec_ctxt_os_data = (VOID *)p_param -> hit_exec_ctxt_id;            
    
                break;
                
            }
            
        } 
        
    } 
        
    if (dbg_status == DBG_STATUS_OK)
    {    
        /* Create a new breakpoint using the specified parameters. */
        
        dbg_status = dbg_eng_bkpt_breakpoint_create(p_dbg_eng_bkpt,
                                                    p_param -> hit_exec_ctxt_id,
                                                    exec_ctxt_os_data,
                                                    p_param -> p_addr,
                                                    DBG_ENG_BREAKPOINT_TYPE_ACTIVE,                                                     
                                                    p_param -> eval_func, 
                                                    p_param -> eval_func_param,
                                                    p_param -> hit_func,
                                                    p_param -> hit_func_param,
                                                    p_param -> stop_exec_ctxt_id, 
                                                    NU_NULL,
                                                    NU_NULL,                             
                                                    p_param -> pass_count, 
                                                    p_param -> pass_cycle,
                                                    p_param -> hit_count, 
                                                    &p_addr_bkpt,
                                                    &p_ctxt_bkpt);
    } 
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Activate the address breakpoint. */
        
        dbg_status = dbg_eng_bkpt_address_breakpoint_activate(p_dbg_eng_bkpt,
                                                              p_addr_bkpt);       
        
    }     
    
    return(dbg_status);

}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Breakpoint_Clear
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function clears a breakpoint.
*                                                                                                 
*   INPUTS   
*
*       p_dbg_eng_bkpt - Pointer to the control block.
*                                                                      
*       p_param - Pointer to the parameter structure.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - The specified breakpoint was
*                                         not found.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_ENG_BKPT_Breakpoint_Clear(DBG_ENG_BKPT_CB *            p_dbg_eng_bkpt,
                                         DBG_ENG_BKPT_CLEAR_PARAM *   p_param)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt; 
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt; 
    UINT                            ctxt_bkpt_index;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Attempt to find the address and context breakpoints associated with
       the target breakpoint address. */

    dbg_status = dbg_eng_bkpt_breakpoint_find(p_dbg_eng_bkpt,
                                              p_param -> p_addr,
                                              p_param -> hit_exec_ctxt_id,
                                              &p_addr_bkpt,
                                              &p_ctxt_bkpt,
                                              &ctxt_bkpt_index);    
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* At least one matching breakpoint was found. */

        while (dbg_status == DBG_STATUS_OK)
        {
            /* Remove the breakpoint. */
            
            dbg_status = dbg_eng_bkpt_breakpoint_remove(p_dbg_eng_bkpt,
                                                        p_addr_bkpt,
                                                        p_ctxt_bkpt,
                                                        ctxt_bkpt_index);
        
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Attempt to find another matching breakpoint. */
                
                dbg_status = dbg_eng_bkpt_breakpoint_find(p_dbg_eng_bkpt,
                                                          p_param -> p_addr,
                                                          p_param -> hit_exec_ctxt_id,
                                                          &p_addr_bkpt,
                                                          &p_ctxt_bkpt,
                                                          &ctxt_bkpt_index);
                                                                       
            } 
        
        } 
        
        /* Intercept 'resource unavailable' status as it is expected when
           no more matching breakpoints found.  Any other status is an
           error. */
           
        if (dbg_status == DBG_STATUS_RESOURCE_UNAVAILABLE)
        {
            dbg_status = DBG_STATUS_OK;
            
        } 
        
    } 
        
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Breakpoints_Remove_Temporary
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function removes all temporary breakpoints for the thread.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_eng_bkpt - Pointer to control block.
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
DBG_STATUS DBG_ENG_BKPT_Breakpoints_Remove_Temporary(DBG_ENG_BKPT_CB *              p_dbg_eng_bkpt,
                                                     DBG_ENG_BKPT_RMV_TEMP_PARAM *  p_param)
{
    DBG_STATUS                          dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *        p_addr_bkpt;
    DBG_ENG_ADDRESS_BREAKPOINT *        p_next_addr_bkpt;    
    DBG_ENG_CONTEXT_BREAKPOINT *        p_ctxt_bkpt;
    UINT                                ctxt_bkpt_index;
    UINT                                ctxt_bkpt_count;    
    
    /* Set initial status value. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Set the current address breakpoint to the start of the all address 
       breakpoints list. */
    
    p_addr_bkpt = p_dbg_eng_bkpt -> p_all_addr_bkpts;
    
    /* Iterate through all address breakpoints removing temporary ones. */

    while ((dbg_status == DBG_STATUS_OK) &&
           (p_addr_bkpt != NU_NULL)) 
    {
        /* Attempt to find a matching context breakpoint. */
        
        p_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                               &p_addr_bkpt->ctxt_bkpt_set,
                                                               p_param -> p_os_thread,
                                                               DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP,
                                                               NU_NULL,
                                                               &ctxt_bkpt_index);
        
        while ((dbg_status == DBG_STATUS_OK) &&
               (p_ctxt_bkpt != NU_NULL))
        {    
            /* Remove the breakpoint from the set. */
            
            dbg_status = DBG_SET_Node_Remove(&p_addr_bkpt -> ctxt_bkpt_set,
                                             DBG_SET_LOCATION_INDEX,
                                             (DBG_SET_LOCATION_VALUE)ctxt_bkpt_index,
                                             (DBG_SET_NODE **)&p_ctxt_bkpt);
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Delete the context breakpoint. */
                
                dbg_status = dbg_eng_bkpt_context_breakpoint_delete(p_dbg_eng_bkpt,
                                                                    p_ctxt_bkpt);            

            } 
                    
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Attempt to find another matching context breakpoint. */
                
                p_ctxt_bkpt = dbg_eng_bkpt_context_breakpoint_set_find(p_dbg_eng_bkpt,
                                                                       &p_addr_bkpt->ctxt_bkpt_set,
                                                                       p_param -> p_os_thread,
                                                                       DBG_ENG_BREAKPOINT_TYPE_SINGLESTEP,
                                                                       NU_NULL,
                                                                       &ctxt_bkpt_index);       
            
            }             
            
        } 
    
        if (dbg_status == DBG_STATUS_OK)
        {     
            /* Determine if the address breakpoint is now empty (has no 
               associated context breakpoints). */        
               
            ctxt_bkpt_count = DBG_SET_Node_Count(&p_addr_bkpt -> ctxt_bkpt_set);
            
            if (ctxt_bkpt_count == 0)
            {
                /* There are no more context breakpoints for the address
                   breakpoint. */

                /* Get a pointer to the next address breakpoint. */
            
                p_next_addr_bkpt = p_addr_bkpt -> next;                   
                   
                /* Remove the current address breakpoint. */
                
                dbg_status = dbg_eng_bkpt_address_breakpoint_remove(p_dbg_eng_bkpt,
                                                                    p_addr_bkpt);
                
                /* Update the current to be the next. */
                
                p_addr_bkpt = p_next_addr_bkpt;
            
            }
            else
            {
                /* There are still context breakpoints. */
            
                /* Move to the next address breakpoint. */
            
                p_addr_bkpt = p_addr_bkpt -> next;
            
            }             
            
        }         
            
    } 

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION                                                             
*                                                                      
*       DBG_ENG_BKPT_Breakpoints_Remove_All
*                                                                      
*   DESCRIPTION   
*                                                       
*       This function removes all breakpoints.
*                                                                                                 
*   INPUTS                                                               
*                                                                      
*       p_dbg_eng_bkpt - Pointer to control block.
*                                                                      
*   OUTPUTS                                                              
*                                                                      
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*                                                                      
*************************************************************************/
DBG_STATUS DBG_ENG_BKPT_Breakpoints_Remove_All(DBG_ENG_BKPT_CB *                p_dbg_eng_bkpt,
                                               DBG_ENG_BKPT_RMV_ALL_PARAM *     p_param)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_ADDRESS_BREAKPOINT *    p_addr_bkpt;
    DBG_ENG_CONTEXT_BREAKPOINT *    p_ctxt_bkpt;

    /* Set initial status value. */
    
    dbg_status = DBG_STATUS_OK;
    
    /* Set the current address breakpoint to the start of the all address 
       breakpoints list. */
    
    p_addr_bkpt = p_dbg_eng_bkpt -> p_all_addr_bkpts;
    
    /* Iterate through all address breakpoints removing them 
       unconditionally. */

    while ((dbg_status == DBG_STATUS_OK) &&
           (p_addr_bkpt != NU_NULL)) 
    {
        /* Remove all context breakpoints associated with the address
           breakpoint. */
    
        /* Attempt to remove the first context breakpoint from the set. */
        
        dbg_status = DBG_SET_Node_Remove(&p_addr_bkpt -> ctxt_bkpt_set,
                                         DBG_SET_LOCATION_HEAD,
                                         DBG_SET_NODE_ID_ANY,
                                         (DBG_SET_NODE **)&p_ctxt_bkpt);
        
        while (dbg_status == DBG_STATUS_OK)
        {
            /* Delete the context breakpoint. */
            
            dbg_status = dbg_eng_bkpt_context_breakpoint_delete(p_dbg_eng_bkpt,
                                                                p_ctxt_bkpt);        

            if (dbg_status == DBG_STATUS_OK)
            {
                /* Attempt to remove another context breakpoint (any
                   remaining breakpoint) from the set. */
                
                dbg_status = DBG_SET_Node_Remove(&p_addr_bkpt -> ctxt_bkpt_set,
                                                 DBG_SET_LOCATION_HEAD,
                                                 DBG_SET_NODE_ID_ANY,
                                                 (DBG_SET_NODE **)&p_ctxt_bkpt);       
            
            } 
            
        }            
           
        /* Look for 'end of set' status value.  All others are errors. */
           
        if (dbg_status == DBG_STATUS_RESOURCE_UNAVAILABLE)
        {
            /* Remove the address breakpoint. */
            
            dbg_status = dbg_eng_bkpt_address_breakpoint_remove(p_dbg_eng_bkpt,
                                                                p_addr_bkpt);
              
        } 
        
        if (dbg_status == DBG_STATUS_OK)
        {        
            /* Move to any address breakpoint still in the all address 
               breakpoint list. */
            
            p_addr_bkpt = p_dbg_eng_bkpt -> p_all_addr_bkpts;
        
        } 
            
    } 
    
    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_BKPT_Software_Breakpoint_Handler
*
*   DESCRIPTION
*
*       This function is the entry routine for an ISR which handles 
*       software breakpoint interrupts.  Note that it is NOT in the 
*       standard Nucleus PLUS LISR function format.  This function is 
*       called as part of a running thread.
*
*   INPUTS
*
*       p_excp_stack_frame - Pointer to the stack frame.  Note: this is
*                            an exception stack frame (not solicited or
*                            unsolicited).
*
*   OUTPUTS
*
*       Pointer to pointer to the current task's stack.
*
*************************************************************************/
VOID ** DBG_ENG_BKPT_Software_Breakpoint_Handler(VOID *   p_excp_stack_frame)
{
    NU_TASK *                   p_os_exec_ctxt;
    VOID **                     p_p_os_exec_ctxt_stack;
    
#ifdef CFG_NU_OS_SVCS_TRACE_ENABLE

    DBG_Trace_Mask = NU_Trace_Get_Mask();
    NU_Trace_Disarm(DBG_Trace_Mask);

#endif /* CFG_NU_OS_SVCS_TRACE_ENABLE */

    /* Initialize local variables. */
    
    p_os_exec_ctxt = (NU_TASK *)TCD_Current_Thread;
    p_p_os_exec_ctxt_stack = NU_NULL;
    
    /* Ensure debug engine is active (indicated by global control block
       value). */ 

    if (DBG_ENG_p_cb != NU_NULL)
    {
        /* Call breakpoint handler. */
    
        (VOID)dbg_eng_bkpt_breakpoint_handler_software(&DBG_ENG_p_cb -> bkpt,
                                                       (VOID *)p_os_exec_ctxt,
                                                       p_excp_stack_frame);          
        
    }             
    
    /* Determine if a thread (or HISR) context is present. */
    
    if ((((NU_TASK *)p_os_exec_ctxt) -> tc_id == TC_TASK_ID) ||
        (((NU_HISR *)p_os_exec_ctxt) -> tc_id == TC_HISR_ID))
    {
        /* Save exception stack pointer in execution contexts control 
           block. */
        
        p_os_exec_ctxt -> tc_stack_pointer = p_excp_stack_frame;   
        
        /* Get pointer to the execution contexts stack pointer member of the
           control block.  Note that this is a pointer to the control block 
           structure, not the stack itself. */    
    
        p_p_os_exec_ctxt_stack = &p_os_exec_ctxt -> tc_stack_pointer;
        
    }
    else
    {
        /* Get pointer to the execution context of the exception. */
        
        ERC_System_Error(NU_MEMORY_CORRUPT);
        
    }
        
    /* Return a pointer to the stack pointer of the execution context. 
       This value is passed to the Unsolicited Switch function which will 
       return execution to the OS. */
    
    return (p_p_os_exec_ctxt_stack);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_BKPT_Hardware_Singlestep_Handler
*
*   DESCRIPTION
*
*       This function is the entry routine for an ISR which handles 
*       hardware stepping interrupts.  Note that it is NOT in the 
*       standard Nucleus PLUS LISR function format.  This function is 
*       called as part of a running thread.
*
*   INPUTS
*
*       p_excp_stack_frame - Pointer to the stack frame.  Note: this is
*                            an exception stack frame (not solicited or
*                            unsolicited).
*
*   OUTPUTS
*
*       Pointer to pointer to the current task's stack.
*
*************************************************************************/
VOID ** DBG_ENG_BKPT_Hardware_Singlestep_Handler(VOID *   p_excp_stack_frame)
{
    NU_TASK *                   p_os_exec_ctxt;
    VOID **                     p_p_os_exec_ctxt_stack;
    
    /* Initialize local variables. */
    
    p_os_exec_ctxt = (NU_TASK *)TCD_Current_Thread;
    p_p_os_exec_ctxt_stack = NU_NULL;

    /* Ensure debug engine is active (indicated by global control block
       value). */ 

    if (DBG_ENG_p_cb != NU_NULL)
    {
        /* Call breakpoint handler. */
    
        (VOID)dbg_eng_bkpt_breakpoint_handler_hardware(&DBG_ENG_p_cb -> bkpt,
                                                       (VOID *)p_os_exec_ctxt,
                                                       p_excp_stack_frame);          
        
    }

    /* Determine if a thread (or HISR) context is present. */
    
    if ((((NU_TASK *)p_os_exec_ctxt) -> tc_id == TC_TASK_ID) ||
        (((NU_HISR *)p_os_exec_ctxt) -> tc_id == TC_HISR_ID))
    {
        /* Save exception stack pointer in execution contexts control
           block. */

        p_os_exec_ctxt -> tc_stack_pointer = p_excp_stack_frame;

        /* Get pointer to the execution contexts stack pointer member of the
           control block.  Note that this is a pointer to the control block
           structure, not the stack itself. */
    
        p_p_os_exec_ctxt_stack = &p_os_exec_ctxt -> tc_stack_pointer;

    }
    else
    {
        /* Get pointer to the execution context of the exception. */

        ERC_System_Error(NU_MEMORY_CORRUPT);

    }
        
    /* Return a pointer to the stack pointer of the execution context. This
       value is passed to the Unsolicited Switch function which will 
       return execution to the OS. */
    
    return (p_p_os_exec_ctxt_stack);
}

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
*       dbg_eng.c
*
*   COMPONENT
*
*       Debug Agent - Debug Engine
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
*       dbg_eng_command_log_begin
*       dbg_eng_command_log_end
*       dbg_eng_thread_get_current_id
*       dbg_eng_handle_breakpoint_hit
*       dbg_eng_handle_os_data_abort
*       dbg_eng_components_initialize
*       dbg_eng_components_configure
*       dbg_eng_components_terminate
*       dbg_eng_session_open
*       dbg_eng_session_close
*       dbg_eng_session_reset
*       dbg_eng_session_info
*       dbg_eng_thread_get_current
*       dbg_eng_thread_get_first
*       dbg_eng_thread_get_next
*       dbg_eng_thread_go
*       dbg_eng_thread_stop
*       dbg_eng_thread_step
*       dbg_eng_memory_read
*       dbg_eng_memory_write
*       dbg_eng_register_read
*       dbg_eng_register_write
*       dbg_eng_breakpoint_set
*       dbg_eng_breakpoint_clear
*       dbg_eng_breakpoint_clear_all
*       dbg_eng_event_handler_register
*       dbg_eng_event_handler_unregister
*       dbg_eng_thread_info
*       dbg_eng_thread_id
*       dbg_eng_thread_group
*
*       DBG_ENG_Command
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

/***** External variables */

/* Debug service control block pointer. */

extern DBG_CB *             DBG_p_cb;

/* Nucleus PLUS current thread. */

extern VOID * volatile      TCD_Current_Thread;
extern CS_NODE              *TCD_Created_Tasks_List;

/***** Global variables */

/* Debug Engine - Pointer to the debug engine control block if the engine
   is currently active, otherwise the value will be NULL. */

DBG_ENG_CB *                DBG_ENG_p_cb = NU_NULL;

/***** Local functions */

/* Local function prototypes */

static DBG_STATUS dbg_eng_thread_get_current_id(DBG_ENG_CB *           p_dbg_eng,
                                                DBG_THREAD_ID *        p_thread_id);

static INT dbg_eng_handle_breakpoint_hit(DBG_THREAD_ID                 exec_ctxt_id,
                                         DBG_BREAKPOINT_ID             bkpt_id,
                                         DBG_ENG_BKPT_HIT_FUNC_PARAM   param);

static VOID ** dbg_eng_handle_os_data_abort(VOID *   p_stack);

static DBG_STATUS dbg_eng_components_initialize(DBG_ENG_CB *    p_dbg_eng);

static DBG_STATUS dbg_eng_components_configure(DBG_ENG_CB *     p_dbg_eng);

static DBG_STATUS dbg_eng_components_terminate(DBG_ENG_CB *     p_dbg_eng);

static DBG_STATUS dbg_eng_session_open(DBG_SESSION_ID           session_id,
                                       DBG_CMD *                p_cmd);

static DBG_STATUS dbg_eng_session_close(DBG_SESSION_ID          session_id,
                                        DBG_CMD *               p_cmd);

static DBG_STATUS dbg_eng_session_reset(DBG_SESSION_ID         session_id,
                                        DBG_CMD *              p_cmd);

static DBG_STATUS dbg_eng_session_info(DBG_SESSION_ID           session_id,
                                       DBG_CMD *                p_cmd);

static DBG_STATUS dbg_eng_thread_get_current(DBG_SESSION_ID         session_id,
                                             DBG_CMD *              p_cmd);

static DBG_STATUS dbg_eng_thread_get_first(DBG_SESSION_ID           session_id,
                                           DBG_CMD *                p_cmd);

static DBG_STATUS dbg_eng_thread_get_next(DBG_SESSION_ID            session_id,
                                          DBG_CMD *                 p_cmd);

static DBG_STATUS dbg_eng_thread_go(DBG_SESSION_ID          session_id,
                                    DBG_CMD *               p_cmd);

static DBG_STATUS dbg_eng_thread_stop(DBG_SESSION_ID        session_id,
                                      DBG_CMD *             p_cmd);

static DBG_STATUS dbg_eng_thread_step(DBG_SESSION_ID        session_id,
                                      DBG_CMD *             p_cmd);

static DBG_STATUS dbg_eng_thread_info(DBG_SESSION_ID        session_id,
                                      DBG_CMD *             p_cmd);

static DBG_STATUS dbg_eng_thread_id(DBG_SESSION_ID          session_id,
                                    DBG_CMD *               p_cmd);

static DBG_STATUS dbg_eng_thread_group(DBG_SESSION_ID          session_id,
                                       DBG_CMD *               p_cmd);

static DBG_STATUS dbg_eng_memory_read(DBG_SESSION_ID            session_id,
                                      DBG_CMD *                 p_cmd);

static DBG_STATUS dbg_eng_memory_write(DBG_SESSION_ID           session_id,
                                       DBG_CMD *                p_cmd);

static DBG_STATUS dbg_eng_register_read(DBG_SESSION_ID          session_id,
                                        DBG_CMD *               p_cmd);

static DBG_STATUS dbg_eng_register_write(DBG_SESSION_ID         session_id,
                                         DBG_CMD *              p_cmd);

static DBG_STATUS dbg_eng_breakpoint_set(DBG_SESSION_ID         session_id,
                                         DBG_CMD *              p_cmd);

static DBG_STATUS dbg_eng_breakpoint_clear(DBG_SESSION_ID       session_id,
                                           DBG_CMD *            p_cmd);

static DBG_STATUS dbg_eng_breakpoint_clear_all(DBG_SESSION_ID       session_id,
                                               DBG_CMD *            p_cmd);

static DBG_STATUS dbg_eng_event_handler_register(DBG_SESSION_ID     session_id,
                                                 DBG_CMD *          p_cmd);

static DBG_STATUS dbg_eng_event_handler_unregister(DBG_SESSION_ID   session_id,
                                                   DBG_CMD *        p_cmd);

/* Default Command Function Array - This array contains all the command
   functions for the debug engine API.  Note that the order of these
   functions MUST match the corresponding DBG_CMD_OP command values. */

static DBG_CMD_FUNC     DBG_ENG_Cmd_Func_Array[] = {dbg_eng_session_open,
                                                    dbg_eng_session_close,
                                                    dbg_eng_session_reset,
                                                    dbg_eng_session_info,
                                                    dbg_eng_thread_get_current,
                                                    dbg_eng_thread_get_first,
                                                    dbg_eng_thread_get_next,
                                                    dbg_eng_thread_go,
                                                    dbg_eng_thread_stop,
                                                    dbg_eng_thread_step,
                                                    dbg_eng_thread_info,
                                                    dbg_eng_thread_id,
                                                    dbg_eng_thread_group,                                                   
                                                    dbg_eng_memory_read,
                                                    dbg_eng_memory_write,
                                                    dbg_eng_register_read,
                                                    dbg_eng_register_write,
                                                    dbg_eng_breakpoint_set,
                                                    dbg_eng_breakpoint_clear,
                                                    dbg_eng_breakpoint_clear_all,
                                                    dbg_eng_event_handler_register,
                                                    dbg_eng_event_handler_unregister};

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_get_current_id
*
*   DESCRIPTION
*
*       This function returns the current (last run application) thread's
*       ID value.
*
*   INPUTS
*
*       p_dbg_eng - Pointer to the control block.
*
*       p_thread_id - Return parameter that will be updated to indicate
*                     the thread ID value of the last run application
*                     thread if the operation is successful.  If the
*                     operation fails the value is undefined.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that no current thread
*                                         ID is available (no unprotected
*                                         threads have run, usually 
*                                         indicating an initialized
*                                         system state).
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_get_current_id(DBG_ENG_CB *           p_dbg_eng,
                                                DBG_THREAD_ID *        p_thread_id)
{
    DBG_STATUS                                          dbg_status;
    NU_TASK *                                           p_os_thread;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Get the last un-protected thread to have executed. */

    p_os_thread = TCC_Current_Application_Task_Pointer();

    /* Determine if a last unprotected thread is set. */
    
    if (p_os_thread == NU_NULL)
    {
        /* No last unprotected thread set, so no current ID is 
           available. */
           
        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
        
    }
        
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update the return parameters. */

        *p_thread_id = (DBG_THREAD_ID)p_os_thread;

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_handle_breakpoint_hit
*
*   DESCRIPTION
*
*       This function handles when a breakpoint hits.
*
*   INPUTS
*
*       exec_ctxt_id - The execution context ID that the breakpoint occurred
*                    on.
*
*       bkpt_id - The breakpoint ID.
*
*       param - Parameter pointer.  The parameter is expected to be the
*               pointer to the control block.
*
*   OUTPUTS
*
*       0 - Operation successful.
*
*       -1 - Operation failed.
*
*************************************************************************/
static INT dbg_eng_handle_breakpoint_hit(DBG_THREAD_ID                 exec_ctxt_id,
                                         DBG_BREAKPOINT_ID             bkpt_id,
                                         DBG_ENG_BKPT_HIT_FUNC_PARAM   param)
{
    DBG_STATUS                      dbg_status;
    INT                             result;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_ENG_EVT_HDLR_CALL_PARAM     evt_hdlr_call_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the control block. */

    p_dbg_eng = (DBG_ENG_CB *)param;

    /* Determine how to proceed based on the breakpoint ID value. */

    if (bkpt_id == DBG_BREAKPOINT_ID_STEP)
    {
        /* Setup for a "step complete" event.  The call will NOT be
           direct so that execution of the handler will occur in a thread
           context. */

        evt_hdlr_call_param.is_direct = NU_FALSE;
        evt_hdlr_call_param.hdlr_param.id = DBG_EVENT_ID_STEP_CPLT;
        evt_hdlr_call_param.hdlr_param.id_param.step_cplt.exec_ctxt_id = exec_ctxt_id;
        evt_hdlr_call_param.hdlr_param.id_param.step_cplt.bkpt_id = bkpt_id;

    }
    else
    {
        /* Setup for a "breakpoint hit" event.  The call will NOT be
           direct so that execution of the handler will occur in a thread
           context. */

        evt_hdlr_call_param.is_direct = NU_FALSE;
        evt_hdlr_call_param.hdlr_param.id = DBG_EVENT_ID_BKPT_HIT;
        evt_hdlr_call_param.hdlr_param.id_param.step_cplt.exec_ctxt_id = exec_ctxt_id;
        evt_hdlr_call_param.hdlr_param.id_param.step_cplt.bkpt_id = bkpt_id;

    }

    /* Attempt to call debug engine event handler. */

    dbg_status = DBG_ENG_EVT_Handler_Call(&p_dbg_eng -> evt,
                                          &evt_hdlr_call_param);

    /* Intercept the "Resource Unavailable" return status since it
       indicates that there is no registered event handler and that is
       acceptable. */

    if (dbg_status == DBG_STATUS_RESOURCE_UNAVAILABLE)
    {
        dbg_status = DBG_STATUS_OK;

    }

    /* Update function status value for return. */

    result = (dbg_status == DBG_STATUS_OK) ? 0 : -1;

    return (result);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_handle_os_data_abort
*
*   DESCRIPTION
*
*       This function handles an OS data abort exception.
*
*   INPUTS
*
*       p_stack - Pointer to the exception stack frame.
*
*   OUTPUTS
*
*       Pointer to pointer to the current task's stack.
*
*************************************************************************/
static VOID ** dbg_eng_handle_os_data_abort(VOID *   p_stack)
{
    DBG_STATUS                          dbg_status;
    ESAL_AR_STK_MIN *                   p_excp_stack_frame;
    DBG_ENG_CB *                        p_dbg_eng;
    DBG_ENG_MEM_OP_CNCL_PARAM           mem_op_cncl_param;
    NU_TASK *                           p_current_thread;
    VOID **                             p_p_current_thread_stack;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the exception stack frame. */

    p_excp_stack_frame = (ESAL_AR_STK_MIN *)p_stack;

    /* Get pointer to the current thread. */

    p_current_thread = (NU_TASK *)TCD_Current_Thread;

    /* Get pointer to the current thread's stack pointer member of the
       control block.  Note that this is a pointer to the task control
       block structure, not the stack. */

    p_p_current_thread_stack = &p_current_thread -> tc_stack_pointer;

    /* Save stack pointer in threads control block. */

    p_current_thread -> tc_stack_pointer = p_excp_stack_frame;

    /* Ensure debug engine is active (indicated by global control block
       value). */

    if (DBG_ENG_p_cb != NU_NULL)
    {
        /* Get pointer to the debug engine control block. */

        p_dbg_eng = DBG_ENG_p_cb;

        /* Determine if a debug engine operation is occurring that might have
           caused this exception. */

        if ((p_dbg_eng -> cur_op == DBG_CMD_OP_MEMORY_READ) ||
            (p_dbg_eng -> cur_op == DBG_CMD_OP_MEMORY_WRITE))
        {
            /* Handle exception by stopping the debug engine operation that
               caused the exception. */

            dbg_status = DBG_ENG_MEM_Operation_Cancel(&p_dbg_eng -> mem,
                                                      &mem_op_cncl_param);

            if (dbg_status == DBG_STATUS_OK)
            {
                /* Update the exception context return address to move
                   past the offending instruction. */

                p_excp_stack_frame -> rtn_address = (p_excp_stack_frame -> rtn_address + sizeof(DBG_OS_OPCODE));

            }

        }

    }

    /* Return a pointer to the stack pointer of the current thread.  This
       value is passed to the Unsolicited Switch function which will
       return execution to the OS. */

    return (p_p_current_thread_stack);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_components_initialize
*
*   DESCRIPTION
*
*       This function initializes the (sub-) components of the debug
*       engine.
*
*   INPUTS
*
*       p_dbg_eng - Pointer to debug engine control block.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static DBG_STATUS dbg_eng_components_initialize(DBG_ENG_CB *      p_dbg_eng)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_BKPT_INIT_PARAM         bkpt_init_param;
    DBG_ENG_REG_INIT_PARAM          reg_init_param;
    DBG_ENG_MEM_INIT_PARAM          mem_init_param;
    DBG_ENG_EXEC_INIT_PARAM         exec_init_param;
    DBG_ENG_EVT_INIT_PARAM          evt_init_param;

    /* Initialize the Breakpoint component. */

    bkpt_init_param.p_dbg_eng = p_dbg_eng;

    dbg_status = DBG_ENG_BKPT_Initialize(&p_dbg_eng -> bkpt,
                                         &bkpt_init_param);

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Initialize the Register component. */

        reg_init_param.p_dbg_eng = p_dbg_eng;

        dbg_status = DBG_ENG_REG_Initialize(&p_dbg_eng -> reg,
                                            &reg_init_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Initialize the Memory component. */

        mem_init_param.p_dbg_eng = p_dbg_eng;
        
        dbg_status = DBG_ENG_MEM_Initialize(&p_dbg_eng -> mem,
                                            &mem_init_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Initialize the Execution component. */

        exec_init_param.p_dbg_eng = p_dbg_eng;

        dbg_status = DBG_ENG_EXEC_Initialize(&p_dbg_eng -> exec,
                                             &exec_init_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Initialize the Event component. */

        evt_init_param.p_dbg_eng = p_dbg_eng;

        dbg_status = DBG_ENG_EVT_Initialize(&p_dbg_eng -> evt,
                                            &evt_init_param);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_components_configure
*
*   DESCRIPTION
*
*       This function configures the (sub-) components of the debug
*       engine.
*
*   INPUTS
*
*       p_dbg_eng - Pointer to debug engine control block.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static DBG_STATUS dbg_eng_components_configure(DBG_ENG_CB *      p_dbg_eng)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_BKPT_CONTROL_PARAM      bkpt_ctrl_param;

    /* Update the illegal instruction handling behavior in the
       breakpoint system. */

    bkpt_ctrl_param.id = DBG_ENG_BKPT_CONTROL_ID_ILLEGAL_INST;
    bkpt_ctrl_param.id_param.ill_inst.stop_thread = DBG_ENG_ILLEGAL_INST_STOPS_THREAD;
    bkpt_ctrl_param.id_param.ill_inst.replaced_with_bkpt = DBG_ENG_ILLEGAL_INST_REPLACE;

    if (DBG_ENG_ILLEGAL_INST_DEBUG_EVENT == NU_TRUE)
    {
        bkpt_ctrl_param.id_param.ill_inst.hit_func = dbg_eng_handle_breakpoint_hit;
        bkpt_ctrl_param.id_param.ill_inst.hit_func_param = (DBG_ENG_BKPT_HIT_FUNC_PARAM)p_dbg_eng;

    }
    else
    {
        bkpt_ctrl_param.id_param.ill_inst.hit_func = NU_NULL;
        bkpt_ctrl_param.id_param.ill_inst.hit_func_param = NU_NULL;

    }

    dbg_status = DBG_ENG_BKPT_Control(&p_dbg_eng -> bkpt,
                                      &bkpt_ctrl_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_components_terminate
*
*   DESCRIPTION
*
*       This function terminates the (sub-) components of the debug
*       engine.
*
*   INPUTS
*
*       p_dbg_eng - Pointer to debug engine control block.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static DBG_STATUS dbg_eng_components_terminate(DBG_ENG_CB *      p_dbg_eng)
{
    DBG_STATUS                  dbg_status;
    DBG_ENG_BKPT_TERM_PARAM     bkpt_term_param;
    DBG_ENG_EVT_TERM_PARAM      evt_term_param;
    DBG_ENG_EXEC_TERM_PARAM     exec_term_param;
    DBG_ENG_MEM_TERM_PARAM      mem_term_param;
    DBG_ENG_REG_TERM_PARAM      reg_term_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Terminate the breakpoint component. */

    dbg_status = DBG_ENG_BKPT_Terminate(&p_dbg_eng -> bkpt,
                                        &bkpt_term_param);
                                        
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Terminate the event component. */

        dbg_status = DBG_ENG_EVT_Terminate(&p_dbg_eng -> evt,
                                           &evt_term_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Terminate the execution component. */

        dbg_status = DBG_ENG_EXEC_Terminate(&p_dbg_eng -> exec,
                                            &exec_term_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Terminate the memory component. */

        dbg_status = DBG_ENG_MEM_Terminate(&p_dbg_eng -> mem,
                                           &mem_term_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Terminate the register component. */

        dbg_status = DBG_ENG_REG_Terminate(&p_dbg_eng -> reg,
                                           &reg_term_param);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_session_open
*
*   DESCRIPTION
*
*       Opens a debug engine session.  This causes the debug engine to
*       perform initialization activities.
*
*   INPUTS
*
*       session_id - The session ID.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_NOT_ACTIVE - Indicates debug service not active.
*
*       DBG_STATUS_NOT_SUPPORTED - Indicates that the requested Debug
*                                  Service API version to be used for the
*                                  session is not supported by this
*                                  implementation.
*
*       DBG_STATUS_FAILED - Indicates that the API semaphore could not be
*                           created.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_session_open(DBG_SESSION_ID         session_id,
                                       DBG_CMD *              p_cmd)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;
    DBG_CMD_SESSION_OPEN_PARAM *    p_op_param;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_OS_DEBUG_BEGIN_PARAM        os_debug_begin_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.ses_opn;

    /* Ensure debug service is active (check global flag). */

    if (DBG_p_cb == NU_NULL)
    {
        /* ERROR: Debug service not active. */
     
        dbg_status = DBG_STATUS_NOT_ACTIVE;
    } 

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Verify that the requested Debug Service API is supported by this
           implementation. */
    
        if (p_op_param -> version_id != DBG_VERSION_ID_1_0)
        {
            /* ERROR: Requested Debug Service API version not supported. */
    
            dbg_status = DBG_STATUS_NOT_SUPPORTED;
    
        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Allocate memory for the component control block. */

        p_dbg_eng = (DBG_ENG_CB *)DBG_System_Memory_Allocate(sizeof(DBG_ENG_CB),
                                                             DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                             DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                             DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                             NU_NO_SUSPEND);

        dbg_status = DBG_STATUS_FROM_NULL_PTR_CHECK(p_dbg_eng);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set debug engine pointer to debug service control block. */
        
        p_dbg_eng -> p_dbg = DBG_p_cb;

        /* Update the checking enabled value in the debug engine control
           block to be the new value. */

        p_dbg_eng -> checking_enabled = DBG_ENG_CHECKING_ENABLED;

        /* Setup component command API function table. */

        p_dbg_eng -> p_cmd_func_array = (VOID *)DBG_ENG_Cmd_Func_Array;
        p_dbg_eng -> cmd_func_array_size = (UINT)(sizeof(DBG_ENG_Cmd_Func_Array)
                                                  / sizeof(DBG_CMD_FUNC));

        /* Initialize thread list members. */
        
        p_dbg_eng -> p_thd_list = NU_NULL;
        p_dbg_eng -> thd_list_size = 0;
        p_dbg_eng -> thd_list_idx = 0;

        /* Create API semaphore. */
        
        nu_status = NU_Create_Semaphore(&p_dbg_eng -> api_sem,
                                        "DBG_ENG",
                                        1,
                                        NU_FIFO);    
    
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to create API semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        } 

    }

    if (dbg_status == DBG_STATUS_OK)
    {   

        /* Initialize debug engine (sub-) components. */

        dbg_status = dbg_eng_components_initialize(p_dbg_eng);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Configure debug engine (sub-) components. */

        dbg_status = dbg_eng_components_configure(p_dbg_eng);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup (start) OS debugging support. */

        os_debug_begin_param.soft_bkpt_hdlr = DBG_ENG_BKPT_Software_Breakpoint_Handler;
        os_debug_begin_param.hw_step_hdlr = DBG_ENG_BKPT_Hardware_Singlestep_Handler;
        os_debug_begin_param.data_abrt_hdlr = dbg_eng_handle_os_data_abort;

        dbg_status = DBG_OS_Debug_Begin(&os_debug_begin_param);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Update the global pointer to the debug engine control block to
           indicate that the debug engine is active. */
    
        DBG_ENG_p_cb = p_dbg_eng;        
        
        /* Update the return parameters. */

        *(p_op_param -> p_session_id) = (DBG_SESSION_ID)p_dbg_eng;

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_session_close
*
*   DESCRIPTION
*
*       Closes a debug engine session.  This causes the debug engine to
*       shut down.
*
*   INPUTS
*
*       session_id - The session ID.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates that the session ID value was
*                           invalid.  OR indicates that the API semaphore
*                           could not be deleted.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_session_close(DBG_SESSION_ID         session_id,
                                        DBG_CMD *              p_cmd)
{
    DBG_STATUS                      dbg_status;
    STATUS                          nu_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_OS_DEBUG_END_PARAM          os_debug_end_param;

    /* Set initial functions status. */

    dbg_status = DBG_STATUS_OK;

    /* Get component control block from session ID. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Update the global pointer to the debug engine control block to
       indicate that the debug engine is no longer active. */

    DBG_ENG_p_cb = NU_NULL;

    /* Stop debugging support in the OS (mainly exception handling). */

    dbg_status = DBG_OS_Debug_End(&os_debug_end_param);

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Terminate engine components. */
        
        dbg_status = dbg_eng_components_terminate(p_dbg_eng);

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Delete API semaphore. */
        
        nu_status = NU_Delete_Semaphore(&p_dbg_eng -> api_sem);    
    
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete API semaphore. */
    
            dbg_status = DBG_STATUS_FAILED;
            
        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Deallocate memory for the component control block. */

        dbg_status = DBG_System_Memory_Deallocate((VOID *)p_dbg_eng);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_session_reset
*
*   DESCRIPTION
*
*       Resets a debug engine session.  This causes the debug engine to
*       reset its state (but not de-allocate resources and terminate
*       components).
*
*   INPUTS
*
*       session_id - The session ID.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_session_reset(DBG_SESSION_ID         session_id,
                                        DBG_CMD *              p_cmd)
{
    DBG_STATUS      dbg_status;
    DBG_ENG_CB *    p_dbg_eng;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;
    
    /* Conditionally release thread list resources. */
    
    if (p_dbg_eng -> p_thd_list != NU_NULL)
    {
        dbg_status = DBG_System_Memory_Deallocate(p_dbg_eng -> p_thd_list);

    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* (Re-)initialize thread list members. */
        
        p_dbg_eng -> p_thd_list = NU_NULL;
        p_dbg_eng -> thd_list_size = 0;
        p_dbg_eng -> thd_list_idx = 0;

    }    

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_session_info
*
*   DESCRIPTION
*
*       This function retrieves information about the debug session.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*************************************************************************/
static DBG_STATUS dbg_eng_session_info(DBG_SESSION_ID           session_id,
                                       DBG_CMD *                p_cmd)
{
    /* RESERVED for future development */

    return(DBG_STATUS_OK);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_get_current
*
*   DESCRIPTION
*
*       This function returns the current (last run application) thread.
*
*   INPUTS
*
*       session_id - The session ID.
*
       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_get_current(DBG_SESSION_ID         session_id,
                                             DBG_CMD *              p_cmd)
{
    DBG_STATUS                              dbg_status;
    DBG_ENG_CB *                            p_dbg_eng;
    DBG_CMD_THREAD_GET_CURRENT_PARAM *      p_op_param;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_get_cur;

    /* Get the ID of the last un-protected thread to have executed. */

    dbg_status = dbg_eng_thread_get_current_id(p_dbg_eng,
                                               p_op_param -> p_thread_id);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_get_first
*
*   DESCRIPTION
*
*       This function returns the first thread ID value.
*
*   INPUTS
*
*       session_id - The session ID value (debug engine control block).
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that there are no
*                                         threads in the system.
*
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates insufficient memory
*                                           to complete the operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_get_first(DBG_SESSION_ID           session_id,
                                           DBG_CMD *                p_cmd)
{
    DBG_STATUS                                  dbg_status;
    DBG_ENG_CB *                                p_dbg_eng;
    DBG_CMD_THREAD_GET_FIRST_NEXT_PARAM *       p_op_param;    

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_get_fst_nxt;

    /* Conditionally release any thread list resources. */
    
    if (p_dbg_eng -> p_thd_list != NU_NULL)
    {
        dbg_status = DBG_System_Memory_Deallocate(p_dbg_eng -> p_thd_list);

    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Retrieve and store the list of threads from the OS (if any) and
           reset the thread get first/next system. */
        
        p_dbg_eng -> thd_list_size = TCF_Established_Application_Tasks();
        
        if (p_dbg_eng -> thd_list_size > 0)
        {
            p_dbg_eng -> p_thd_list = (NU_TASK **)DBG_System_Memory_Allocate((p_dbg_eng -> thd_list_size * sizeof(NU_TASK *)),
                                                                             DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                                             DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                                             DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                                             NU_NO_SUSPEND);            
            
            if (p_dbg_eng -> p_thd_list == NU_NULL)
            {
                /* ERROR: Unable to allocate memory for thread first/next
                   process. */
                   
                dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;
                
            }
            
            if (dbg_status == DBG_STATUS_OK)
            {
                p_dbg_eng -> thd_list_idx = 0;
                p_dbg_eng -> thd_list_size = TCF_Application_Task_Pointers(&p_dbg_eng -> p_thd_list[0],
                                                                           p_dbg_eng -> thd_list_size);
           
                if (p_dbg_eng -> thd_list_size == 0)
                {
                    /* ERROR: No threads in system. */
                    
                    dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
                    
                }
           
            }
            
        }
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Return the first thread in the stored thread list. */
        
        *p_op_param -> p_thread_id = (DBG_THREAD_ID)p_dbg_eng -> p_thd_list[p_dbg_eng -> thd_list_idx];

        /* Update to return next thread. */
        
        p_dbg_eng -> thd_list_idx++;

    } 

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_get_next
*
*   DESCRIPTION
*
*       This function returns the next thread ID value.
*
*   INPUTS
*
*       session_id - The session ID value (debug engine control block).
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that there are no more
*                                         threads in the system.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_get_next(DBG_SESSION_ID            session_id,
                                          DBG_CMD *                 p_cmd)
{
    DBG_STATUS                                  dbg_status;
    DBG_ENG_CB *                                p_dbg_eng;
    DBG_CMD_THREAD_GET_FIRST_NEXT_PARAM *       p_op_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_get_fst_nxt;

    /* Return the next thread in the system, using the stored thread
       list, if one is available. */
    
    if (p_dbg_eng -> thd_list_idx == p_dbg_eng -> thd_list_size)
    {
        /* Release any thread list resources. */
       
        if (p_dbg_eng -> p_thd_list != NU_NULL)
        {
            dbg_status = DBG_System_Memory_Deallocate(p_dbg_eng -> p_thd_list);
            p_dbg_eng -> p_thd_list = NU_NULL;
        }          
        
        if (dbg_status == DBG_STATUS_OK)
        {
            /* ERROR: No more threads in system. */
                        
            dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;
       
        }     
       
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Return the next thread in the stored thread list. */
        
        *p_op_param -> p_thread_id = (DBG_THREAD_ID)p_dbg_eng -> p_thd_list[p_dbg_eng -> thd_list_idx];

        /* Update to return next thread. */
        
        p_dbg_eng -> thd_list_idx++;        
    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_go
*
*   DESCRIPTION
*
*       This function starts or resumes execution of the specified thread
*       or group of threads.
*
*   INPUTS
*
*       p_dbg_eng - Pointer to the control block.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_go(DBG_SESSION_ID          session_id,
                                    DBG_CMD *               p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_THREAD_GO_PARAM *       p_op_param;
    DBG_ENG_EXEC_GO_PARAM           exec_go_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_go;

    /* Run the specified thread(s). */

    exec_go_param.thread_id = p_op_param -> thread_id;

    dbg_status = DBG_ENG_EXEC_Go(&p_dbg_eng -> exec,
                                 &exec_go_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_stop
*
*   DESCRIPTION
*
*       This function stops or halts execution of the specified thread
*       or group of threads.
*
*   INPUTS
*
*       p_dbg_eng - Pointer to the control block.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_stop(DBG_SESSION_ID        session_id,
                                      DBG_CMD *             p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_THREAD_STOP_PARAM *     p_op_param;    
    DBG_ENG_EXEC_STOP_PARAM         exec_stop_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_stop;

    /* Stop the specified thread(s). */

    exec_stop_param.thread_id = p_op_param -> thread_id;
    exec_stop_param.use_direct_events = NU_TRUE;

    dbg_status = DBG_ENG_EXEC_Stop(&p_dbg_eng -> exec,
                                   &exec_stop_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_step
*
*   DESCRIPTION
*
*       This function steps execution of the specified thread or group of
*       threads.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_step(DBG_SESSION_ID        session_id,
                                      DBG_CMD *             p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_THREAD_STEP_PARAM *     p_op_param;    
    DBG_ENG_EXEC_STEP_PARAM         exec_step_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_step;

    /* Step the specified thread(s). */

    exec_step_param.step_thread_id = p_op_param -> step_thread_id;
    exec_step_param.go_thread_id = p_op_param -> go_thread_id;
    exec_step_param.stop_thread_id = p_op_param -> stop_thread_id;
    exec_step_param.hit_func = dbg_eng_handle_breakpoint_hit;
    exec_step_param.hit_func_param = (DBG_ENG_BKPT_HIT_FUNC_PARAM)p_dbg_eng;

    dbg_status = DBG_ENG_EXEC_Step(&p_dbg_eng -> exec,
                                   &exec_step_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_info
*
*   DESCRIPTION
*
*       This function returns information on the specified thread.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates the thread is invalid.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_info(DBG_SESSION_ID            session_id,
                                      DBG_CMD *                 p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_THREAD_INFO_PARAM *     p_op_param;
    DBG_ENG_EXEC_STAT_PARAM         exec_stat_param;
    NU_TASK *                       p_os_thread;
    size_t                          str_len;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_info;

    /* Ensure a valid thread ID for operation. */
    
    if ((p_op_param -> thread_id == DBG_THREAD_ID_NONE) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ALL) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ANY))
    {
        /* ERROR: Invalid thread for operation. */
        
        dbg_status = DBG_STATUS_INVALID_THREAD;
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Retrieve the execution status of the thread into the 
           return parameter. */
        
        exec_stat_param.thread_id = p_op_param -> thread_id;
        exec_stat_param.p_thread_status = p_op_param -> p_thread_status;

        dbg_status = DBG_ENG_EXEC_Status(&p_dbg_eng -> exec,
                                         &exec_stat_param);
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Translate the thread ID into OS thread. */
        
        p_os_thread = (NU_TASK *)p_op_param -> thread_id;
        
        /* Update the thread information string with the thread name. */
        
        strncpy(&p_op_param -> thread_info_str[0],
                p_os_thread -> tc_name,
                DBG_THREAD_INFO_STR_SIZE);
                
        /* Ensure thread information string is NULL-terminated. */
        
        str_len = strlen(p_os_thread -> tc_name);        
        
        if (str_len < DBG_THREAD_INFO_STR_SIZE)
        {
            p_op_param -> thread_info_str[str_len] = NU_NULL;
            
        }
        else
        {
            p_op_param -> thread_info_str[DBG_THREAD_INFO_STR_SIZE - 1] = NU_NULL;
            
        }
        
    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_id
*
*   DESCRIPTION
*
*       This function returns the thread ID if provided the OS thread
*       control block pointer.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_id(DBG_SESSION_ID          session_id,
                                    DBG_CMD *               p_cmd)
{
    DBG_CMD_THREAD_ID_PARAM *                           p_op_param;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_id;

    /* Get context model thread ID for OS thread. */

    *p_op_param -> p_thread_id = (DBG_THREAD_ID)p_op_param -> p_os_thread;

    return(DBG_STATUS_OK);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_thread_group
*
*   DESCRIPTION
*
*       This function sets the thread group that a specified thread is a
*       member of.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates an invalid target thread.
*
*       DBG_STATUS_INVALID_STATE - Indicates an invalid thread group.
*
*************************************************************************/
static DBG_STATUS dbg_eng_thread_group(DBG_SESSION_ID          session_id,
                                       DBG_CMD *               p_cmd)
{
    DBG_STATUS                          dbg_status;
    DBG_CMD_THREAD_GRP_PARAM *          p_op_param;
    NU_TASK *                           p_os_thread;
    UNSIGNED                            os_thread_group_id;

    /* Set initial function status. */
    
    dbg_status = DBG_STATUS_OK;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.thd_grp;

    /* Ensure a valid thread ID for operation. */
    
    if ((p_op_param -> thread_id == DBG_THREAD_ID_NONE) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ALL) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ANY))
    {
        /* ERROR: Invalid thread for operation. */
        
        dbg_status = DBG_STATUS_INVALID_THREAD;
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Get pointer to os thread from thread ID. */
        
        p_os_thread = (NU_TASK *)p_op_param -> thread_id;
        
        /* Update thread group id. */
        
        switch (p_op_param -> thread_group)
        {
            case DBG_THREAD_GROUP_SYSTEM :
            {
                os_thread_group_id = TC_GRP_ID_SYS;
                
                break;
                
            }
            
            case DBG_THREAD_GROUP_APPLICATION :
            {
                os_thread_group_id = TC_GRP_ID_APP;
    
                break;
                
            }
    
            case DBG_THREAD_GROUP_NONE :
            default :
            {
                /* ERROR: Invalid thread group. */
                
                dbg_status = DBG_STATUS_INVALID_STATE;
                
                break;
                
            }
    
        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Set thread group flags. */
        
        (VOID)TCS_Change_Group_ID(p_os_thread,
                                  os_thread_group_id);   
    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_memory_read
*
*   DESCRIPTION
*
*       This function reads memory.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_memory_read(DBG_SESSION_ID           session_id,
                                      DBG_CMD *                p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_MEMORY_READ_PARAM *     p_op_param;
    DBG_ENG_MEM_COPY_PARAM          mem_copy_param;
    UINT                            actual_copy_size;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.mem_rd;

    /* Read memory using a copy operation. */

    mem_copy_param.p_src = p_op_param -> p_read;
    mem_copy_param.copy_size = p_op_param -> read_size;
    mem_copy_param.access_mode = p_op_param -> access_mode;
    mem_copy_param.p_dst = p_op_param -> p_read_buffer;
    mem_copy_param.p_actual_copy_size = &actual_copy_size;

    dbg_status = DBG_ENG_MEM_Memory_Copy(&p_dbg_eng->mem,
                                         &mem_copy_param);

    /* Update the return parameter value. */

    *(p_op_param -> p_actual_read_size) = actual_copy_size;

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_memory_write
*
*   DESCRIPTION
*
*       This function writes memory.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_memory_write(DBG_SESSION_ID          session_id,
                                       DBG_CMD *               p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_MEMORY_WRITE_PARAM *    p_op_param;
    DBG_ENG_MEM_COPY_PARAM          mem_copy_param;
    UINT                            actual_copy_size;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.mem_wrt;

    /* Read memory using a copy operation. */

    mem_copy_param.p_dst = p_op_param -> p_write;
    mem_copy_param.copy_size = p_op_param -> write_size;
    mem_copy_param.access_mode = p_op_param -> access_mode;
    mem_copy_param.p_src = p_op_param -> p_write_buffer;
    mem_copy_param.p_actual_copy_size = &actual_copy_size;

    dbg_status = DBG_ENG_MEM_Memory_Copy(&p_dbg_eng->mem,
                                         &mem_copy_param);

    /* Update the return parameter value. */

    *(p_op_param -> p_actual_write_size) = actual_copy_size;

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_register_read
*
*   DESCRIPTION
*
*       This function reads registers.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates an invalid thread ID value
*                                   for this operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_register_read(DBG_SESSION_ID          session_id,
                                        DBG_CMD *               p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_REGISTER_READ_PARAM *   p_op_param;
    DBG_ENG_REG_READ_PARAM          reg_read_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.reg_rd;

    /* Ensure valid target thread. */

    if ((p_op_param -> thread_id == DBG_THREAD_ID_NONE) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ALL) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ANY))
    {
        /* ERROR: Invalid thread ID for operation. */
        
        dbg_status = DBG_STATUS_INVALID_THREAD;
        
    }
    
    if (dbg_status == DBG_STATUS_OK)
    {
        /* Read register data from thread. */
    
        reg_read_param.p_os_thread = (NU_TASK *)p_op_param -> thread_id;
        reg_read_param.reg_id = p_op_param -> register_id;
        reg_read_param.p_reg_data = p_op_param -> p_register_data;
        reg_read_param.p_actual_reg_data_size = p_op_param -> p_actual_reg_data_size;
    
        dbg_status = DBG_ENG_REG_Register_Read(&p_dbg_eng -> reg,
                                               &reg_read_param);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_register_write
*
*   DESCRIPTION
*
*       This function writes registers.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates an invalid thread ID value
*                                   for this operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_register_write(DBG_SESSION_ID         session_id,
                                         DBG_CMD *              p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_REGISTER_WRITE_PARAM *  p_op_param;
    DBG_ENG_REG_WRITE_PARAM         reg_write_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.reg_wrt;

    /* Ensure valid target thread. */

    if ((p_op_param -> thread_id == DBG_THREAD_ID_NONE) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ALL) ||
        (p_op_param -> thread_id == DBG_THREAD_ID_ANY))
    {
        /* ERROR: Invalid thread ID for operation. */
        
        dbg_status = DBG_STATUS_INVALID_THREAD;
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Write register data to thread. */

        reg_write_param.p_os_thread = (NU_TASK *)p_op_param -> thread_id;
        reg_write_param.reg_id = p_op_param -> register_id;
        reg_write_param.p_reg_data = p_op_param -> p_register_data;

        dbg_status = DBG_ENG_REG_Register_Write(&p_dbg_eng -> reg,
                                                &reg_write_param);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_breakpoint_set
*
*   DESCRIPTION
*
*       This function sets a breakpoint.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_breakpoint_set(DBG_SESSION_ID         session_id,
                                         DBG_CMD *              p_cmd)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    DBG_CMD_BREAKPOINT_SET_PARAM *  p_op_param;
    DBG_ENG_BKPT_SET_PARAM          bkpt_set_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.bkpt_set;

    /* Perform breakpoint set operation. */

    bkpt_set_param.hit_exec_ctxt_id = p_op_param -> hit_thread_id;
    bkpt_set_param.stop_exec_ctxt_id = p_op_param -> stop_thread_id;
    bkpt_set_param.p_addr = (DBG_OS_OPCODE *)p_op_param -> p_address;
    bkpt_set_param.eval_func = DBG_ENG_BKPT_EVAL_FUNC_NONE;
    bkpt_set_param.eval_func_param = DBG_ENG_BKPT_EVAL_FUNC_PARAM_NONE;
    bkpt_set_param.hit_func = dbg_eng_handle_breakpoint_hit;
    bkpt_set_param.hit_func_param = (DBG_ENG_BKPT_HIT_FUNC_PARAM)p_dbg_eng;
    bkpt_set_param.pass_count = p_op_param -> pass_count;
    bkpt_set_param.pass_cycle = 0;
    bkpt_set_param.hit_count = p_op_param -> hit_count;

    dbg_status = DBG_ENG_BKPT_Breakpoint_Set(&p_dbg_eng -> bkpt,
                                             &bkpt_set_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_breakpoint_clear
*
*   DESCRIPTION
*
*       This function clears a breakpoint.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_breakpoint_clear(DBG_SESSION_ID       session_id,
                                           DBG_CMD *            p_cmd)
{
    DBG_STATUS                          dbg_status;
    DBG_ENG_CB *                        p_dbg_eng;
    DBG_CMD_BREAKPOINT_CLEAR_PARAM *    p_op_param;
    DBG_ENG_BKPT_CLEAR_PARAM            bkpt_clear_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.bkpt_clr;

    /* Complete parameter setup and perform operation. */

    bkpt_clear_param.hit_exec_ctxt_id = p_op_param -> thread_id;
    bkpt_clear_param.p_addr = p_op_param -> p_address;

    dbg_status = DBG_ENG_BKPT_Breakpoint_Clear(&p_dbg_eng -> bkpt,
                                               &bkpt_clear_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_breakpoint_clear_all
*
*   DESCRIPTION
*
*       This function clears all breakpoints.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_breakpoint_clear_all(DBG_SESSION_ID       session_id,
                                               DBG_CMD *            p_cmd)
{
    DBG_STATUS                              dbg_status;
    DBG_ENG_CB *                            p_dbg_eng;
    DBG_ENG_BKPT_RMV_ALL_PARAM              bkpt_rmv_all_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Clear all breakpoints. */

    dbg_status = DBG_ENG_BKPT_Breakpoints_Remove_All(&p_dbg_eng -> bkpt,
                                                     &bkpt_rmv_all_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_event_handler_register
*
*   DESCRIPTION
*
*       This function registers an event handler callback for debug
*       engine events.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_event_handler_register(DBG_SESSION_ID     session_id,
                                                 DBG_CMD *          p_cmd)
{
    DBG_STATUS                                  dbg_status;
    DBG_ENG_CB *                                p_dbg_eng;
    DBG_CMD_EVENT_HANDLER_REGISTER_PARAM *      p_op_param;
    DBG_ENG_EVT_HDLR_REG_PARAM                  evt_hdlr_reg_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Get pointer to operation-specific parameters. */

    p_op_param = &p_cmd -> op_param.evt_hdlr_reg;

    /* Attempt to register an event handler function. */

    evt_hdlr_reg_param.hdlr_func = p_op_param -> handler_func;
    evt_hdlr_reg_param.hdlr_ctxt = p_op_param -> handler_context;

    dbg_status = DBG_ENG_EVT_Handler_Register(&p_dbg_eng -> evt,
                                              &evt_hdlr_reg_param);

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_event_handler_unregister
*
*   DESCRIPTION
*
*       This function unregisters an event handler callback for debug
*       engine events.
*
*   INPUTS
*
*       session_id - The session ID value.
*
*       p_cmd - Pointer to the command structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_event_handler_unregister(DBG_SESSION_ID       session_id,
                                                   DBG_CMD *            p_cmd)
{
    DBG_STATUS                                  dbg_status;
    DBG_ENG_CB *                                p_dbg_eng;
    DBG_ENG_EVT_HDLR_UNRG_PARAM                 evt_hdlr_unrg_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the component control block. */

    p_dbg_eng = (DBG_ENG_CB *)session_id;

    /* Attempt to register an event handler function. */

    dbg_status = DBG_ENG_EVT_Handler_Unregister(&p_dbg_eng -> evt,
                                                &evt_hdlr_unrg_param);

    return(dbg_status);
}

/***** Global functions */

/* Debug Service API */

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_Command
*
*   DESCRIPTION
*
*       This function is the primary API for the debug engine.  It uses
*       a complex parameter structure to provide debug engine
*       functionality in a flexible and maintainable manner.
*
*       NOTE: This API function is compliant with the RSP Shared Component
*       definition of a debug engine interface.
*
*   INPUTS
*
*       session_id - The session ID that the command is for.
*
*       p_cmd - Pointer to the command structure to be processed.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_OPERATION - Indicates an invalid command
*                                      operation was encountered.
*
*       DBG_STATUS_NOT_IN_SESSION - Indicates that a command was attempted
*                                   outside of a debug session.  Outside
*                                   of a debug session the only valid
*                                   command is to open a session.
*
*       DBG_STATUS_NOT_ACTIVE - Indicates that the debug engine or debug
*                               service is not active (may be 
*                               initializing).
*
*       DBG_STATUS_FAILED - Indicates unable to obtain or release API
*                           semaphore, possibly because session was
*                           closed.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
DBG_STATUS DBG_ENG_Command(DBG_SESSION_ID         session_id,
                           DBG_CMD *              p_cmd)
{
    DBG_STATUS              dbg_status;
    STATUS                  nu_status;
    DBG_ENG_CB *            p_dbg_eng;
    DBG_CMD_FUNC *          p_cmd_func_array;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to debug service control block. */
    
    if (DBG_p_cb == NU_NULL)
    {
        /* ERROR: Debug service not active. */
        
        dbg_status = DBG_STATUS_NOT_ACTIVE;
        
    }

    if (dbg_status == DBG_STATUS_OK)
    {
        
        /* Determine how to proceed based on the command to be performed. */
    
        if (p_cmd -> op == DBG_CMD_OP_SESSION_OPEN)
        {
            /* Get pointer to command function array directly from the default
               command function table. */
    
            p_cmd_func_array = &DBG_ENG_Cmd_Func_Array[0];
            
            /* Call command operation function. */
    
            dbg_status = p_cmd_func_array[p_cmd -> op](session_id,
                                                       p_cmd);        
            
        }
        else
        {
            /* Get pointer to the debug engine control block from the debug
               session parameter. */
        
            p_dbg_eng = (DBG_ENG_CB *)session_id;        
            
            /* Ensure control block parameter is valid. */

            if (p_dbg_eng == NU_NULL)
            {
                /* ERROR: Invalid debug engine control block. */
                
                dbg_status = DBG_STATUS_NOT_IN_SESSION; 
                
            }
            
            if (dbg_status == DBG_STATUS_OK)
            {
                /* Determine how to proceed based on whether checking is to be
                   performed. */
        
                if (p_dbg_eng -> checking_enabled == NU_TRUE)
                {
                    /* Ensure parameter structure pointer is valid. */
    
                    dbg_status = DBG_STATUS_FROM_NULL_PTR_CHECK(p_cmd);
        
                    if (dbg_status == DBG_STATUS_OK)
                    {
                        /* Ensure that operation ID is valid. */
        
                        if ((p_cmd -> op < 0) ||
                            (p_cmd -> op >= p_dbg_eng->cmd_func_array_size))
                        {
                            dbg_status = DBG_STATUS_INVALID_OPERATION;
        
                        }
        
                    }
        
                }             
            
            }
            
            if (dbg_status == DBG_STATUS_OK)
            {            
                /* Obtain API semaphore, blocking if needed. */
            
                nu_status = NU_Obtain_Semaphore(&p_dbg_eng -> api_sem,
                                                NU_SUSPEND);    
                
                if (nu_status == NU_SUCCESS)
                {  
                    /* Get a pointer to the command function array from the
                       control block. */
        
                    p_cmd_func_array = (DBG_CMD_FUNC *)p_dbg_eng -> p_cmd_func_array;
        
                    /* Update OS to indicate a debug operation is occurring. */
        
                    dbg_status = DBG_OS_Debug_Operation_Begin();
        
                    /* Set the current operation. */
        
                    p_dbg_eng -> cur_op = p_cmd -> op;
            
                    if (dbg_status == DBG_STATUS_OK)
                    {
                        /* Call command operation function. */
                
                        dbg_status = p_cmd_func_array[p_cmd -> op](session_id,
                                                                   p_cmd);
                
                    }
            
                    /* Clear the current operation. */
            
                    p_dbg_eng -> cur_op = DBG_CMD_OP_NONE;
            
                    /* Update OS to indicate a debug operation has completed. */
            
                    (void)DBG_OS_Debug_Operation_End();
                    
                    /* Release API semaphore if the operation was not a
                       'session close'. */
                    
                    if (p_cmd -> op != DBG_CMD_OP_SESSION_CLOSE)
                    {
                        nu_status = NU_Release_Semaphore(&p_dbg_eng -> api_sem);    
                        
                        if (nu_status != NU_SUCCESS)
                        {
                            /* ERROR: Unable to release API semaphore. */
                        
                            dbg_status = DBG_STATUS_FAILED;
                                
                        }
        
                    }
        
                }
                else
                {
                    /* ERROR: Unable to obtain API semaphore. */
                    
                    dbg_status = DBG_STATUS_FAILED;
                            
                }
    
            }
    
        }
    
    }

    return (dbg_status);
}

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
*       dbg_eng_exec.c
*
*   COMPONENT
*
*       Debug Agent - Engine Engine - Execution
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
*       dbg_eng_exec_thread_go
*       dbg_eng_exec_thread_stop
*       dbg_eng_exec_thread_step_setup

*       DBG_ENG_EXEC_Initialize
*       DBG_ENG_EXEC_Terminate
*       DBG_ENG_EXEC_Go
*       DBG_ENG_EXEC_Stop
*       DBG_ENG_EXEC_Step
*       DBG_ENG_EXEC_Status
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

/***** Local functions */

/* Local function prototypes. */

static DBG_STATUS dbg_eng_exec_thread_go(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                                         DBG_THREAD_ID                thread_id);

static DBG_STATUS dbg_eng_exec_thread_stop(DBG_ENG_EXEC_CB *           p_dbg_eng_exec,
                                           DBG_THREAD_ID               thread_id,
                                           BOOLEAN                     use_direct_events);

static DBG_STATUS dbg_eng_exec_thread_step_setup(DBG_ENG_EXEC_CB *               p_dbg_eng_exec,
                                                  DBG_THREAD_ID                   step_thread_id,
                                                  DBG_THREAD_ID                   stop_thread_id,
                                                  DBG_ENG_BKPT_HIT_FUNC           hit_func,
                                                  DBG_ENG_BKPT_HIT_FUNC_PARAM     hit_func_param);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_exec_thread_go
*
*   DESCRIPTION
*
*       Starts or resumes execution of the specified thread provided that
*       the thread is not protected.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
*
*       thread_id - ID of the thread to be started or resumed.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_exec_thread_go(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                                         DBG_THREAD_ID                thread_id)
{
    DBG_STATUS                          dbg_status;
    DBG_ENG_CB *                        p_dbg_eng;
    DBG_OS_EXEC_CMD_PARAM               os_exec_cmd_param;
    NU_TASK *                           p_os_thread;
    DBG_ENG_BKPT_SET_SKIP_STEP_PARAM    bkpt_set_skip_step_param;
    DBG_ENG_BKPT_RMV_TEMP_PARAM         bkpt_rmv_temp_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the service control block. */

    p_dbg_eng = (DBG_ENG_CB *)p_dbg_eng_exec -> p_dbg_eng;

    /* Get information about the target thread. */

    p_os_thread = (NU_TASK *)thread_id;

    /* Setup skip-over breakpoint. */

    bkpt_set_skip_step_param.p_os_thread = p_os_thread;
    bkpt_set_skip_step_param.p_stack_frame = p_os_thread -> tc_stack_pointer;
    bkpt_set_skip_step_param.stack_frame_type = DBG_OS_STACK_FRAME_TYPE_THREAD;
    bkpt_set_skip_step_param.step_exec_ctxt_id = thread_id;
    bkpt_set_skip_step_param.stop_exec_ctxt_id = DBG_THREAD_ID_NONE;
    bkpt_set_skip_step_param.hit_func = DBG_ENG_BKPT_HIT_FUNC_NONE;
    bkpt_set_skip_step_param.hit_func_param = DBG_ENG_BKPT_HIT_FUNC_PARAM_NONE;

    dbg_status = DBG_ENG_BKPT_Breakpoint_Set_Skip_Step(&p_dbg_eng -> bkpt,
                                                       &bkpt_set_skip_step_param);

    /* Intercept and handle the 'invalid operation' status, which
       indicates that there was no primary breakpoint... which is
       possible and acceptable. */

    if (dbg_status == DBG_STATUS_INVALID_OPERATION)
    {
        dbg_status = DBG_STATUS_OK;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Resume execution of the thread. */

        os_exec_cmd_param.op = DBG_OS_EXEC_OP_DEBUG_RESUME;
        os_exec_cmd_param.op_param.dbg_res.p_task = (VOID *)p_os_thread;

        dbg_status = DBG_OS_Exec_Command(&os_exec_cmd_param);

    }

    if (dbg_status != DBG_STATUS_OK)
    {
        /* ERROR: Unable to resume task execution. */

        /* ERROR RECOVERY: Remove all temporary breakpoints. */

        bkpt_rmv_temp_param.p_os_thread = p_os_thread;

        DBG_ENG_BKPT_Breakpoints_Remove_Temporary(&p_dbg_eng -> bkpt,
                                                  &bkpt_rmv_temp_param);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_exec_thread_stop
*
*   DESCRIPTION
*
*       Stops or suspends execution of the specified thread provided that
*       the thread is not protected.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
*
*       thread_id - ID of the thread to be started or resumed.
*
*       use_direct_events - Indicates if direct event calls are to be used
*                           for reporting thread stop events.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_exec_thread_stop(DBG_ENG_EXEC_CB *           p_dbg_eng_exec,
                                           DBG_THREAD_ID               thread_id,
                                           BOOLEAN                     use_direct_events)
{
    DBG_STATUS                      dbg_status;
    DBG_ENG_CB *                    p_dbg_eng;
    NU_TASK *                       p_os_thread;
    DBG_OS_EXEC_CMD_PARAM           os_exec_cmd_param;
    DBG_ENG_EXEC_STAT_PARAM         exec_stat_param;
    DBG_THREAD_STATUS               thread_status;
    DBG_ENG_EVT_HDLR_CALL_PARAM     evt_hdlr_call_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the service control block. */

    p_dbg_eng = (DBG_ENG_CB *)p_dbg_eng_exec -> p_dbg_eng;

    /* Get information about the target thread. */

    p_os_thread = (NU_TASK *)thread_id;

    /* Retrieve pre-operation execution status of thread. */

    exec_stat_param.thread_id = thread_id;
    exec_stat_param.p_thread_status = &thread_status;

    dbg_status = DBG_ENG_EXEC_Status(p_dbg_eng_exec,
                                     &exec_stat_param);

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Attempt to suspend the task. */

        os_exec_cmd_param.op = DBG_OS_EXEC_OP_DEBUG_SUSPEND;
        os_exec_cmd_param.op_param.dbg_susp.p_task = (VOID *)p_os_thread;

        dbg_status = DBG_OS_Exec_Command(&os_exec_cmd_param);

    }

    if ((dbg_status == DBG_STATUS_OK) &&
        (thread_status == DBG_THREAD_STATUS_RUNNING))
    {
        /* Stopped a running thread so send debug engine event. */

        evt_hdlr_call_param.is_direct = use_direct_events;
        evt_hdlr_call_param.hdlr_param.id = DBG_EVENT_ID_THD_STOP;
        evt_hdlr_call_param.hdlr_param.id_param.thd_stop.exec_ctxt_id = thread_id;

        dbg_status = DBG_ENG_EVT_Handler_Call(&p_dbg_eng -> evt,
                                              &evt_hdlr_call_param);

        /* Intercept the 'resource unavailable' status as it is
           expected if there is no registered event handler. */

        if (dbg_status == DBG_STATUS_RESOURCE_UNAVAILABLE)
        {
            dbg_status = DBG_STATUS_OK;
        }

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_exec_thread_step_setup
*
*   DESCRIPTION
*
*       This function sets up a specified thread for a step operation.
*       Note that this function does NOT resume execution of the thread.
*       Also, the operation will only be performed for threads that are
*       not protected.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
*
*       step_thread_id - The execution context that the step will be
*                      performed on.  This is the "scope" of the step
*                      operation.
*
*       stop_thread_id - The execution context that will be stopped when
*                      the step is completed (hits).
*
*       hit_func - Function to be called when the step is complete.
*
*       hit_func_paramm - Hit function parameters.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
static DBG_STATUS dbg_eng_exec_thread_step_setup(DBG_ENG_EXEC_CB *               p_dbg_eng_exec,
                                                 DBG_THREAD_ID                   step_thread_id,
                                                 DBG_THREAD_ID                   stop_thread_id,
                                                 DBG_ENG_BKPT_HIT_FUNC           hit_func,
                                                 DBG_ENG_BKPT_HIT_FUNC_PARAM     hit_func_param)
{
    DBG_STATUS                          dbg_status;
    DBG_ENG_CB *                        p_dbg_eng;
    NU_TASK *                           p_os_thread;
    DBG_ENG_BKPT_SET_SKIP_STEP_PARAM    bkpt_set_skip_step_param;
    DBG_ENG_BKPT_RMV_TEMP_PARAM         bkpt_rmv_temp_param;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Get pointer to the service control block. */

    p_dbg_eng = (DBG_ENG_CB *)p_dbg_eng_exec -> p_dbg_eng;

    /* Get information about the target thread. */

    p_os_thread = (NU_TASK *)step_thread_id;

    /* Insert single-step breakpoint. */

    bkpt_set_skip_step_param.p_os_thread = p_os_thread;
    bkpt_set_skip_step_param.p_stack_frame = p_os_thread -> tc_stack_pointer;
    bkpt_set_skip_step_param.stack_frame_type = DBG_OS_STACK_FRAME_TYPE_THREAD;
    bkpt_set_skip_step_param.step_exec_ctxt_id = step_thread_id;
    bkpt_set_skip_step_param.stop_exec_ctxt_id = stop_thread_id;
    bkpt_set_skip_step_param.hit_func = hit_func;
    bkpt_set_skip_step_param.hit_func_param = hit_func_param;

    bkpt_rmv_temp_param.p_os_thread = NU_NULL;

    DBG_ENG_BKPT_Breakpoints_Remove_Temporary(&p_dbg_eng -> bkpt,
                                              &bkpt_rmv_temp_param);

    dbg_status = DBG_ENG_BKPT_Breakpoint_Set_Skip_Step(&p_dbg_eng -> bkpt,
                                                       &bkpt_set_skip_step_param);

    if (dbg_status != DBG_STATUS_OK)
    {
        /* ERROR: Something bad happened... */

        /* ERROR RECOVERY: Remove all temporary breakpoints. */

        bkpt_rmv_temp_param.p_os_thread = p_os_thread;

        DBG_ENG_BKPT_Breakpoints_Remove_Temporary(&p_dbg_eng -> bkpt,
                                                  &bkpt_rmv_temp_param);

    }

    return(dbg_status);
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EXEC_Initialize
*
*   DESCRIPTION
*
*       Initialize debug engine execution system.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
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
DBG_STATUS DBG_ENG_EXEC_Initialize(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                                   DBG_ENG_EXEC_INIT_PARAM *    p_param)
{
    DBG_STATUS                  dbg_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Set pointer to service control block. */

    p_dbg_eng_exec -> p_dbg_eng = p_param -> p_dbg_eng;

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EXEC_Terminate
*
*   DESCRIPTION
*
*       Terminate debug context system.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
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
DBG_STATUS DBG_ENG_EXEC_Terminate(DBG_ENG_EXEC_CB *             p_dbg_eng_exec,
                                  DBG_ENG_EXEC_TERM_PARAM *     p_param)
{
    DBG_STATUS                  dbg_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Nothing to do here! */

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EXEC_Go
*
*   DESCRIPTION
*
*       Starts or resumes execution of the specified execution context.
*       This function is non-reentrant.  It contains a statically
*       allocated array that it uses for the thread list.  Allocating
*       memory in this function can cause problems when stepping.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates a thread ID value of NONE
*                                   was received, which is an invalid
*                                   target thread ID value for this
*                                   operation.
*
*       DBG_STATUS_INSUFFICIENT_RESOURCES - Indicates not enough memory
*                                           available to complete the
*                                           operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
DBG_STATUS DBG_ENG_EXEC_Go(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                           DBG_ENG_EXEC_GO_PARAM *      p_param)
{
    DBG_STATUS                  dbg_status;
    DBG_THREAD_ID               thread_id;
    UINT                        os_thread_list_size;
    UINT                        i;
    static NU_TASK *            go_os_thread_list_buf[CFG_NU_OS_SVCS_DBG_CONTEXT_MAX];
    NU_TASK *                   p_os_thread;
    DBG_THREAD_ID               cur_app_thread_id = NU_NULL;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Determine how to proceed based on the thread ID value. */

    switch (p_param -> thread_id)
    {
        case DBG_THREAD_ID_NONE :
        case DBG_THREAD_ID_ANY :
        {
            /* ERROR: Thread ID value of NONE or ANY is invalid. */

            dbg_status = DBG_STATUS_INVALID_THREAD;

            break;

        }

        case DBG_THREAD_ID_ALL :
        {
            /* Get the thread list size */

            os_thread_list_size = TCF_Established_Application_Tasks();

            /* Verify the static array is large enough. */

            if (os_thread_list_size <= CFG_NU_OS_SVCS_DBG_CONTEXT_MAX)
            {
                os_thread_list_size = TCF_Application_Task_Pointers(&go_os_thread_list_buf[0],
                                                                    os_thread_list_size);
            }
            else
            {
                /* ERROR: Insufficient memory for operation. */

                dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;

            }

            if (dbg_status == DBG_STATUS_OK)
            {
                /* Get the last run application thread and resume it 
                   first.  If the last run application thread is not
                   available then use the first thread in the application
                   thread list. */

                cur_app_thread_id = (DBG_THREAD_ID)TCC_Current_Application_Task_Pointer();

                if (cur_app_thread_id == NU_NULL)
                {
                    cur_app_thread_id = (DBG_THREAD_ID)go_os_thread_list_buf[0];
                    
                }

                dbg_status = dbg_eng_exec_thread_go(p_dbg_eng_exec,
                                        cur_app_thread_id);

            }

            i = 0;
            while ((dbg_status == DBG_STATUS_OK) &&
                   (i < os_thread_list_size))
            {
                /* Perform operation on the current thread. */

                thread_id = (DBG_THREAD_ID)go_os_thread_list_buf[i];

                /* Make sure the last run application is not resumed again. */

                if (thread_id != cur_app_thread_id)
                {
                    dbg_status = dbg_eng_exec_thread_go(p_dbg_eng_exec,
                                                        thread_id);
                }

                i++;

            }

            break;

        }

        default :
        {
            /* Perform operation on the thread. */

            cur_app_thread_id = p_param -> thread_id;
            dbg_status = dbg_eng_exec_thread_go(p_dbg_eng_exec,
                                                cur_app_thread_id);

            break;

        }

    }

    /* Set the current application task to run first, if preemption is
	   disabled and the task is ready to run (not suspended). */

    if((dbg_status == DBG_STATUS_OK) && (cur_app_thread_id != NU_NULL))
    {
        p_os_thread = (NU_TASK *)cur_app_thread_id;

        /* Check the threads preemption state. */
        if ((p_os_thread -> tc_preemption == NU_FALSE) &&
		    (p_os_thread -> tc_status == NU_READY))
        {
            /* Execute the task first. */
            TCCT_Set_Execute_Task(p_os_thread);
        }

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EXEC_Stop
*
*   DESCRIPTION
*
*       Halts or stops execution of the specified execution context.
*       This function is non-reentrant.  It contains a statically
*       allocated array that it uses for the thread list.  Allocating
*       memory in this function can cause problems when stepping.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_INVALID_THREAD - Indicates a thread ID value of NONE
*                                   was received, which is an invalid
*                                   target thread ID value for this
*                                   operation.
*
*       <other> - Indicates internal error.
*
*************************************************************************/
DBG_STATUS DBG_ENG_EXEC_Stop(DBG_ENG_EXEC_CB *              p_dbg_eng_exec,
                             DBG_ENG_EXEC_STOP_PARAM *      p_param)
{
    DBG_STATUS                  dbg_status;
    DBG_THREAD_ID               thread_id;
    UINT                        os_thread_list_size;
    UINT                        i;
    static NU_TASK *            stop_os_thread_list_buf[CFG_NU_OS_SVCS_DBG_CONTEXT_MAX];

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Determine how to proceed based on the thread ID value. */

    switch (p_param -> thread_id)
    {
        case DBG_THREAD_ID_NONE :
        case DBG_THREAD_ID_ANY :
        {
            /* ERROR: Thread ID value of NONE or ANY is invalid. */

            dbg_status = DBG_STATUS_INVALID_THREAD;

            break;

        }

        case DBG_THREAD_ID_ALL :
        {
            /* Get the thread list size */

            os_thread_list_size = TCF_Established_Application_Tasks();

            /* Verify the static array is large enough. */

            if (os_thread_list_size <= CFG_NU_OS_SVCS_DBG_CONTEXT_MAX)
            {
                os_thread_list_size = TCF_Application_Task_Pointers(&stop_os_thread_list_buf[0],
                                                                    os_thread_list_size);
            }
            else
            {
                /* ERROR: Insufficient memory for operation. */

                dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;

            }

            i = 0;
            while ((dbg_status == DBG_STATUS_OK) &&
                   (i < os_thread_list_size))
            {
                /* Perform operation on the current thread. */

                thread_id = (DBG_THREAD_ID)stop_os_thread_list_buf[i];

                dbg_status = dbg_eng_exec_thread_stop(p_dbg_eng_exec,
                                                      thread_id,
                                                      p_param -> use_direct_events);

                i++;

            }

            break;

        }

        default :
        {
            /* Perform operation on the thread. */

            dbg_status = dbg_eng_exec_thread_stop(p_dbg_eng_exec,
                                                  p_param -> thread_id,
                                                  p_param -> use_direct_events);

            break;

        }

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EXEC_Step
*
*   DESCRIPTION
*
*       Steps execution of the specified execution context.
*       This function is non-reentrant.  It contains a statically
*       allocated array that it uses for the thread list.  Allocating
*       memory in this function can cause problems when stepping.
*
*   INPUTS
*
*       p_dbg_eng_exec - Pointer to the control block.
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
DBG_STATUS DBG_ENG_EXEC_Step(DBG_ENG_EXEC_CB *              p_dbg_eng_exec,
                             DBG_ENG_EXEC_STEP_PARAM *      p_param)
{
    DBG_STATUS                  dbg_status;
    DBG_THREAD_ID               thread_id;
    UINT                        os_thread_list_size;
    UINT                        i;
    DBG_ENG_EXEC_GO_PARAM       exec_go_param;
    static NU_TASK *            step_os_thread_list_buf[CFG_NU_OS_SVCS_DBG_CONTEXT_MAX];

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Determine how to proceed based on the thread ID value. */

    switch (p_param -> step_thread_id)
    {
        case DBG_THREAD_ID_NONE :
        case DBG_THREAD_ID_ANY :
        {
            /* ERROR: Thread ID value of NONE or ANY is invalid. */

            dbg_status = DBG_STATUS_INVALID_THREAD;

            break;

        }

        case DBG_THREAD_ID_ALL :
        {
            /* Get the thread list size */

            os_thread_list_size = TCF_Established_Application_Tasks();

            /* Verify the static array is large enough. */

            if (os_thread_list_size <= CFG_NU_OS_SVCS_DBG_CONTEXT_MAX)
            {
                os_thread_list_size = TCF_Application_Task_Pointers(&step_os_thread_list_buf[0],
                                                                    os_thread_list_size);
            }
            else
            {
                /* ERROR: Insufficient memory for operation. */

                dbg_status = DBG_STATUS_INSUFFICIENT_RESOURCES;

            }

            i = 0;
            while ((dbg_status == DBG_STATUS_OK) &&
                   (i < os_thread_list_size))
            {
                /* Setup step for the current thread. */

                thread_id = (DBG_THREAD_ID)step_os_thread_list_buf[i];

                dbg_status = dbg_eng_exec_thread_step_setup(p_dbg_eng_exec,
                                                            thread_id,
                                                            p_param -> stop_thread_id,
                                                            p_param -> hit_func,
                                                            p_param -> hit_func_param);

                i++;

            }

            break;

        }

        default :
        {
            /* Setup step for the target thread. */

            dbg_status = dbg_eng_exec_thread_step_setup(p_dbg_eng_exec,
                                                        p_param -> step_thread_id,
                                                        p_param -> stop_thread_id,
                                                        p_param -> hit_func,
                                                        p_param -> hit_func_param);

            break;

        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Resume the target thread ID. */

        exec_go_param.thread_id = p_param -> go_thread_id;

        dbg_status = DBG_ENG_EXEC_Go(p_dbg_eng_exec,
                                     &exec_go_param);

    }

    return(dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EXEC_Status
*
*   DESCRIPTION
*
*       Returns the execution status of the specified thread.
*
*   INPUTS
*
*       p_cb - Pointer to the control block.
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
DBG_STATUS DBG_ENG_EXEC_Status(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                               DBG_ENG_EXEC_STAT_PARAM *    p_param)
{
    DBG_STATUS                      dbg_status;
    NU_TASK *                       p_os_thread;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    p_os_thread = (NU_TASK *)p_param -> thread_id;

    /* Update the thread status in the information structure based on
       the current OS thread state. */

    switch (p_os_thread -> tc_status)
    {
        case NU_READY :
        case NU_FINISHED :
        {
            /* Thread is running. */

            *p_param -> p_thread_status = DBG_THREAD_STATUS_RUNNING;

            break;

        }

        case NU_EVENT_SUSPEND :
        case NU_MAILBOX_SUSPEND :
        case NU_MEMORY_SUSPEND :
        case NU_PARTITION_SUSPEND :
        case NU_PIPE_SUSPEND :
        case NU_PURE_SUSPEND :
        case NU_QUEUE_SUSPEND :
        case NU_SEMAPHORE_SUSPEND :
        case NU_SLEEP_SUSPEND :
        {
            /* Thread may be running.  Check if actually debug
               suspended. */

            if (p_os_thread -> tc_debug_suspend == NU_DEBUG_SUSPEND)
            {
                *p_param -> p_thread_status = DBG_THREAD_STATUS_STOPPED;

            }
            else
            {
                *p_param -> p_thread_status = DBG_THREAD_STATUS_RUNNING;

            }

            break;

        }

        case NU_DEBUG_SUSPEND :
        {
            /* Thread is stopped. */

            *p_param -> p_thread_status = DBG_THREAD_STATUS_STOPPED;

            break;

        }

        case NU_TERMINATED :
        {
            /* ERROR: Thread is no longer valid. */

           dbg_status = DBG_STATUS_INVALID_THREAD;

           break;

        }

        default :
        {
            /* ERROR: Unknown OS thread status. */

            dbg_status = DBG_STATUS_FAILED;

            break;

        }

    }

    return(dbg_status);
}

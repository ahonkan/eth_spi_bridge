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
*       dbg_eng_evt.c
*
*   COMPONENT
*
*       Debug Agent - Debug Engine - Event
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
*       dbg_eng_evt_event_log_begin
*       dbg_eng_evt_event_log_end
*       dbg_eng_evt_event_hisr_entry
*       dbg_eng_evt_event_thread_entry
*
*       DBG_ENG_EVT_Initialize
*       DBG_ENG_EVT_Terminate
*       DBG_ENG_EVT_Handler_Register
*       DBG_ENG_EVT_Handler_Unregister
*       DBG_ENG_EVT_Handler_Call
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       thread_control.h
*
*************************************************************************/

/***** Include files */

#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "os/kernel/plus/core/inc/thread_control.h"

/***** Local variables */

static DBG_ENG_EVT_CB *     dbg_eng_evt_p_cb = NU_NULL;

/***** Local functions */

/* Local function prototypes */

static VOID dbg_eng_evt_event_hisr_entry(VOID);

static VOID dbg_eng_evt_event_thread_entry(UNSIGNED    argc,
                                           VOID *      argv);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_evt_event_hisr_entry
*
*   DESCRIPTION
*
*       Main entry function for the event HISR.  The sole purpose of this
*       HISR is to activate the event thread.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID dbg_eng_evt_event_hisr_entry(VOID)
{
    NU_SUPERV_USER_VARIABLES

    if (dbg_eng_evt_p_cb != NU_NULL)
    {
        /* Release the event thread execution semaphore. */

        (VOID)NU_Release_Semaphore(&dbg_eng_evt_p_cb -> thd_exe_sem);

    }

    return;
}

/*************************************************************************
*
*   FUNCTION
*
*       dbg_eng_evt_event_thread_entry
*
*   DESCRIPTION
*
*       Main entry function for the event thread.
*
*   INPUTS
*
*       argc - Argument count.
*
*       argv - Argument vector.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID dbg_eng_evt_event_thread_entry(UNSIGNED    argc,
                                           VOID *      argv)
{
    DBG_STATUS                  dbg_status;
    STATUS                      nu_status;
    DBG_ENG_EVT_CB *            p_dbg_eng_evt;

    /* Set initial status. */

    dbg_status = DBG_STATUS_OK;

    /* Setup pointer to control block. */

    p_dbg_eng_evt = (DBG_ENG_EVT_CB *)argv;

    /* Thread main loop. */

    while (dbg_status == DBG_STATUS_OK)
    {
        /* Get the Execute semaphore and suspend waiting on it
           to become available. */

        nu_status = NU_Obtain_Semaphore(&p_dbg_eng_evt -> thd_exe_sem,
                                        NU_SUSPEND);

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to obtain execute semaphore. */

            dbg_status = DBG_STATUS_FAILED;

        }

        if (dbg_status == DBG_STATUS_OK)
        {

            /* Ensure that there is an event to handle, otherwise ignore
               this thread activation. */

            if (p_dbg_eng_evt -> hdlr_param_count > 0)
            {
                /* Call the handler function specified by the parameters,
                   passing the pointer to the handler function parameters. */

                p_dbg_eng_evt -> hdlr_func(p_dbg_eng_evt -> hdlr_ctxt,
                                           &p_dbg_eng_evt -> hdlr_param[p_dbg_eng_evt -> hdlr_param_read]);

                /* Update handler parameter count. */

                p_dbg_eng_evt -> hdlr_param_count--;

                /* Update handler parameter read index (circular). */

                p_dbg_eng_evt -> hdlr_param_read = ((p_dbg_eng_evt -> hdlr_param_read + 1)
                                                    % DBG_ENG_EVT_CALL_QUEUE_SIZE);

            }

        }

    }

    return;
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EVT_Initialize
*
*   DESCRIPTION
*
*       Initialize debug event component.
*
*   INPUTS
*
*       p_dbg_eng_evt - Pointer to the control block.
*
*       p_param - Pointer to the parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates operation failed.
*
*       <other> - Other internal error.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_EVT_Initialize(DBG_ENG_EVT_CB *             p_dbg_eng_evt,
                                   DBG_ENG_EVT_INIT_PARAM *     p_param)
{
    DBG_STATUS      dbg_status;
    STATUS          nu_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Set pointer to the debug engine control block. */

    p_dbg_eng_evt -> p_dbg_eng = p_param -> p_dbg_eng;

    /* Setup Event Execute Semaphore (EES). */

    nu_status = NU_Create_Semaphore(&p_dbg_eng_evt -> thd_exe_sem,
                                    "DBG_EES",
                                    0,
                                    NU_FIFO);

    if (nu_status != NU_SUCCESS)
    {
        /* ERROR: Unable to create execution semaphore. */

        dbg_status = DBG_STATUS_FAILED;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Create the Event HISR. */

        nu_status = NU_Create_HISR(&p_dbg_eng_evt -> hsr_cb,
                                   "DBG_EVT",
                                   dbg_eng_evt_event_hisr_entry,
                                   DBG_ENG_EVT_HISR_PRIORITY,
                                   &p_dbg_eng_evt -> hsr_stack[0],
                                   DBG_ENG_EVT_HISR_STACK_SIZE);

        if (nu_status == NU_SUCCESS)
        {
            /* Bind HISR to kernel module. */

            nu_status = NU_BIND_HISR_TO_KERNEL(&p_dbg_eng_evt -> hsr_cb);
        }

        if (nu_status == NU_SUCCESS)
        {
            /* Setup pointer to control block (for HISR). */

            dbg_eng_evt_p_cb = p_dbg_eng_evt;
        }
        else
        {
            /* ERROR: Unable to create event HISR. */

            dbg_status = DBG_STATUS_FAILED;

        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Create the Event Thread. */

        nu_status = NU_Create_Task(&p_dbg_eng_evt -> thd_cb,
                                   "DBG_EVT",
                                   dbg_eng_evt_event_thread_entry,
                                   1,
                                   (VOID *)p_dbg_eng_evt,
                                   &p_dbg_eng_evt -> thd_stack[0],
                                   DBG_ENG_EVT_TASK_STACK_SIZE,
                                   DBG_ENG_EVT_TASK_PRIORITY,
                                   DBG_ENG_EVT_TASK_TIME_SLICING,
                                   NU_PREEMPT,
                                   NU_NO_START);

        if (nu_status == NU_SUCCESS)
        {
            /* Bind task to kernel module. */

            nu_status = NU_BIND_TASK_TO_KERNEL(&p_dbg_eng_evt -> thd_cb);
        }

        if (nu_status == NU_SUCCESS)
        {
            /* Start task. */

            /* NOTE: Once the thread is started, if the OS/scheduler is
               running, execution will transfer to the thread. */

            nu_status = NU_Resume_Task(&p_dbg_eng_evt -> thd_cb);
        }

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to create event thread. */

            dbg_status = DBG_STATUS_FAILED;

        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Setup handler function. */

        p_dbg_eng_evt -> hdlr_func = NU_NULL;
        p_dbg_eng_evt -> hdlr_ctxt = NU_NULL;

        /* Setup handler parameter tracking. */

        p_dbg_eng_evt -> hdlr_param_write = 0;
        p_dbg_eng_evt -> hdlr_param_read = 0;
        p_dbg_eng_evt -> hdlr_param_count = 0;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EVT_Terminate
*
*   DESCRIPTION
*
*       Terminate debug protection component.
*
*   INPUTS
*
*       p_dbg_eng_evt - Pointer to the control block.
*
*       p_param - Pointer to the parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_FAILED - Indicates that the operation failed.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_EVT_Terminate(DBG_ENG_EVT_CB *              p_dbg_eng_evt,
                                  DBG_ENG_EVT_TERM_PARAM *      p_param)
{
    DBG_STATUS      dbg_status;
    STATUS          nu_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Delete the Event Execute Semaphore (EES). */

    nu_status = NU_Delete_Semaphore(&p_dbg_eng_evt -> thd_exe_sem);

    if (nu_status != NU_SUCCESS)
    {
        /* ERROR: Unable to delete execute semaphore. */

        dbg_status = DBG_STATUS_FAILED;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Clear pointer to control block (for HISR). */

        dbg_eng_evt_p_cb = NU_NULL;

        /* Delete the Event HISR. */

        nu_status = NU_Delete_HISR(&p_dbg_eng_evt -> hsr_cb);

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete event HISR. */

            dbg_status = DBG_STATUS_FAILED;

        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Terminate the Event Thread. */

        nu_status = NU_Terminate_Task(&p_dbg_eng_evt -> thd_cb);

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to terminate event thread. */

            dbg_status = DBG_STATUS_FAILED;

        }

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Delete the Event Thread (EVT). */

        nu_status = NU_Delete_Task(&p_dbg_eng_evt -> thd_cb);

        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete event thread. */

            dbg_status = DBG_STATUS_FAILED;

        }

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EVT_Handler_Register
*
*   DESCRIPTION
*
*       This function registers an event handler function.
*
*   INPUTS
*
*       p_dbg_eng_evt - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_ALREADY_EXISTS - Indicates that a handler is already
*                                   registered.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_EVT_Handler_Register(DBG_ENG_EVT_CB *               p_dbg_eng_evt,
                                         DBG_ENG_EVT_HDLR_REG_PARAM *   p_param)
{
    DBG_STATUS          dbg_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Ensure that a handler is not already registered. */

    if (p_dbg_eng_evt -> hdlr_func != NU_NULL)
    {
        /* ERROR: Registered handler already exists. */

        dbg_status = DBG_STATUS_ALREADY_EXISTS;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Register the handler. */

        p_dbg_eng_evt -> hdlr_func = p_param -> hdlr_func;
        p_dbg_eng_evt -> hdlr_ctxt = p_param -> hdlr_ctxt;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EVT_Handler_Unregister
*
*   DESCRIPTION
*
*       This function unregisters an event handler function.
*
*   INPUTS
*
*       p_dbg_eng_evt - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that there is no
*                                         registered handler to
*                                         unregister.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_EVT_Handler_Unregister(DBG_ENG_EVT_CB *                 p_dbg_eng_evt,
                                           DBG_ENG_EVT_HDLR_UNRG_PARAM *    p_param)
{
    DBG_STATUS              dbg_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Ensure that a handler is registered. */

    if (p_dbg_eng_evt -> hdlr_func == NU_NULL)
    {
        /* ERROR: No registered handler. */

        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Un-register the handler. */

        p_dbg_eng_evt -> hdlr_func = NU_NULL;
        p_dbg_eng_evt -> hdlr_ctxt = NU_NULL;

    }

    return (dbg_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DBG_ENG_EVT_Handler_Call
*
*   DESCRIPTION
*
*       This function calls a registered handler function.
*
*   INPUTS
*
*       p_dbg_eng_evt - Pointer to the control block.
*
*       p_param - Pointer to parameter structure.
*
*   OUTPUTS
*
*       DBG_STATUS_OK - Indicates successful operation.
*
*       DBG_STATUS_RESOURCE_UNAVAILABLE - Indicates that there is no
*                                         registered handler to
*                                         unregister or that the call
*                                         queue is out of room (for an
*                                         indirect handler call).
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
DBG_STATUS  DBG_ENG_EVT_Handler_Call(DBG_ENG_EVT_CB *                   p_dbg_eng_evt,
                                     DBG_ENG_EVT_HDLR_CALL_PARAM *      p_param)
{
    DBG_STATUS              dbg_status;
    STATUS                  nu_status;

    /* Set initial function status. */

    dbg_status = DBG_STATUS_OK;

    /* Ensure that a handler is registered. */

    if (p_dbg_eng_evt -> hdlr_func == NU_NULL)
    {
        /* ERROR: No registered handler. */

        dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;

    }

    if (dbg_status == DBG_STATUS_OK)
    {
        /* Determine if a direct call to the handler (using the current
           execution context call stack) is to be performed. */

        if (p_param -> is_direct == NU_TRUE)
        {

            /* Perform a direct call of the handler function. */

            p_dbg_eng_evt -> hdlr_func(p_dbg_eng_evt -> hdlr_ctxt,
                                       &p_param -> hdlr_param);

        }
        else
        {
            /* Ensure there is room in the call queue. */

            if (p_dbg_eng_evt -> hdlr_param_count > DBG_ENG_EVT_CALL_QUEUE_SIZE)
            {
                /* ERROR: Not enough room in call queue for this call. */

                dbg_status = DBG_STATUS_RESOURCE_UNAVAILABLE;

            }

            if (dbg_status == DBG_STATUS_OK)
            {
                /* NOTE: Must make a copy of the handler parameters since this
                   is an asynchronous call. */

                (VOID)memcpy((VOID *)&p_dbg_eng_evt -> hdlr_param[p_dbg_eng_evt -> hdlr_param_write],
                             (VOID *)&p_param -> hdlr_param,
                             sizeof(DBG_EVENT_HANDLER_PARAM));

                /* Update handler parameter count. */

                p_dbg_eng_evt -> hdlr_param_count++;

                /* Update handler parameter write index (circular). */

                p_dbg_eng_evt -> hdlr_param_write = ((p_dbg_eng_evt -> hdlr_param_write + 1)
                                                      % DBG_ENG_EVT_CALL_QUEUE_SIZE);

                /* Activate the event HISR (use internal API call to
                   prevent potential MMU supervisor/user mode switch). */
                nu_status = NU_Activate_HISR(&p_dbg_eng_evt -> hsr_cb);

                if (nu_status != NU_SUCCESS)
                {
                    /* ERROR: Unable to activate event HISR. */

                    dbg_status = DBG_STATUS_FAILED;

                }

            }

        }

    }

    return (dbg_status);
}

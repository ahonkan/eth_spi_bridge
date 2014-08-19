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
*       dbg_eng_evt.h
*
*   COMPONENT
*
*       Debug Agent - Debug Engine - Event
*
*   DESCRIPTION
*
*       This file contains the C external interface for the component.
*
*   DATA STRUCTURES
*
*       DBG_ENG_EVT_INIT_PARAM
*       DBG_ENG_EVT_TERM_PARAM
*       DBG_ENG_EVT_HDLR_REG_PARAM
*       DBG_ENG_EVT_HDLR_UNRG_PARAM
*       DBG_ENG_EVT_HDLR_CALL_PARAM
*       DBG_ENG_EVT_CB
*
*   FUNCTIONS
*
*       DBG_ENG_EVT_Initialize
*       DBG_ENG_EVT_Terminate
*       DBG_ENG_EVT_Handler_Register
*       DBG_ENG_EVT_Handler_Unregister
*       DBG_ENG_EVT_Handler_Call
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_ENG_EVT_H
#define DBG_ENG_EVT_H

/***** Global defines */

/* Debug Service Event Task parameters - The following parameters are used
   to control how the event task is configured.  The default values
   are:

    DBG_ENG_EVT_TASK_STACK_SIZE            (16 * NU_MIN_STACK_SIZE)
    DBG_ENG_EVT_TASK_PRIORITY              1
    DBG_ENG_EVT_TASK_TIME_SLICING          10

   Please reset to default values if issues are encountered. */

#define DBG_ENG_EVT_TASK_STACK_SIZE             (16 * NU_MIN_STACK_SIZE)
#define DBG_ENG_EVT_TASK_PRIORITY               0 
#define DBG_ENG_EVT_TASK_TIME_SLICING           10

/* Debug Service Event HISR parameters - The following parameters are used
   to control how the event HISR is configured.  The default values
   are:

    DBG_ENG_EVT_HISR_STACK_SIZE            (4 * NU_MIN_STACK_SIZE)
    DBG_ENG_EVT_HISR_PRIORITY              0

   Please reset to default values if issues are encountered. */

#define DBG_ENG_EVT_HISR_STACK_SIZE             (4 * NU_MIN_STACK_SIZE)
#define DBG_ENG_EVT_HISR_PRIORITY               0

/* Event Call Queue Size - The size (in elements) of the event queue.  The
   default value is 10. */

#define DBG_ENG_EVT_CALL_QUEUE_SIZE             10

/* Command - Initialize Parameters */

typedef struct _dbg_eng_evt_init_param_struct
{
    VOID *                      p_dbg_eng;      /* Pointer to the debug engine */

} DBG_ENG_EVT_INIT_PARAM;

/* Command - Terminate Parameters */

typedef struct _dbg_eng_evt_term_param_struct
{
    VOID *                      reserved;       /* Reserved for future development */

} DBG_ENG_EVT_TERM_PARAM;

/* Command - Handler Register parameters */

typedef struct _dbg_eng_evt_hdlr_reg_param_struct
{
    DBG_EVENT_HANDLER_FUNC      hdlr_func;      /* Handler function for events */
    VOID *                      hdlr_ctxt;      /* Context passed to handler function */

} DBG_ENG_EVT_HDLR_REG_PARAM;

/* Command - Handler Unregister parameters */

typedef struct _dbg_eng_evt_hdlr_unrg_param_struct
{
    VOID *                      reserved;       /* Reserved for future development */

} DBG_ENG_EVT_HDLR_UNRG_PARAM;

/* Command - Handler Call parameters */

typedef struct _dbg_eng_evt_hdlr_call_param_struct
{
    BOOLEAN                     is_direct;      /* Indicates how handler function is called */
    DBG_EVENT_HANDLER_PARAM     hdlr_param;     /* Parameters passed to handler function */

} DBG_ENG_EVT_HDLR_CALL_PARAM;

/* Component control block */

typedef struct _dbg_eng_evt_cb_struct
{
    VOID *                      p_dbg_eng;      /* Pointer to the debug engine control block. */
    NU_TASK                     thd_cb;         /* Thread CB */
    UINT8                       thd_stack[DBG_ENG_EVT_TASK_STACK_SIZE];     /* Thread Stack */
    NU_SEMAPHORE                thd_exe_sem;    /* Execute Semaphore. */
    NU_HISR                     hsr_cb;         /* Hisr CB */
    UINT8                       hsr_stack[DBG_ENG_EVT_HISR_STACK_SIZE];     /* HISR Stack */
    DBG_EVENT_HANDLER_FUNC      hdlr_func;      /* Handler function to be called */
    VOID *                      hdlr_ctxt;      /* Context passed to handler function */
    DBG_EVENT_HANDLER_PARAM     hdlr_param[DBG_ENG_EVT_CALL_QUEUE_SIZE];    /* Parameters passed to handler function */
    UNSIGNED                    hdlr_param_write;   /* Index of handler parameter write */
    UNSIGNED                    hdlr_param_read;    /* Index of handler parameter read */
    UNSIGNED                    hdlr_param_count;   /* handler parameter count */

} DBG_ENG_EVT_CB;

/***** Global functions */

DBG_STATUS  DBG_ENG_EVT_Initialize(DBG_ENG_EVT_CB *             p_dbg_eng_evt,
                                   DBG_ENG_EVT_INIT_PARAM *     p_param);

DBG_STATUS  DBG_ENG_EVT_Terminate(DBG_ENG_EVT_CB *              p_dbg_eng_evt,
                                  DBG_ENG_EVT_TERM_PARAM *      p_param);

DBG_STATUS  DBG_ENG_EVT_Handler_Register(DBG_ENG_EVT_CB *               p_dbg_eng_evt,
                                         DBG_ENG_EVT_HDLR_REG_PARAM *   p_param);

DBG_STATUS  DBG_ENG_EVT_Handler_Unregister(DBG_ENG_EVT_CB *                 p_dbg_eng_evt,
                                           DBG_ENG_EVT_HDLR_UNRG_PARAM *    p_param);

DBG_STATUS  DBG_ENG_EVT_Handler_Call(DBG_ENG_EVT_CB *                   p_dbg_eng_evt,
                                     DBG_ENG_EVT_HDLR_CALL_PARAM *      p_param);

#endif /* DBG_ENG_EVT_H */

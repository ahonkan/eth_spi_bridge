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
*       dbg_eng_exec.h                                    
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Debug Engine - Execution
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the declarations for the component.                        
*                                                                      
*   DATA STRUCTURES                                                      
*                      
*       DBG_ENG_EXEC_INIT_PARAM
*       DBG_ENG_EXEC_TERM_PARAM
*       DBG_ENG_EXEC_GO_PARAM
*       DBG_ENG_EXEC_STOP_PARAM
*       DBG_ENG_EXEC_STEP_PARAM
*       DBG_ENG_EXEC_STAT_PARAM
*       DBG_ENG_EXEC_CB
*            
*   FUNCTIONS
*
*       DBG_ENG_EXEC_Initialize
*       DBG_ENG_EXEC_Terminate
*       DBG_ENG_EXEC_Go
*       DBG_ENG_EXEC_Stop
*       DBG_ENG_EXEC_Step
*       DBG_ENG_EXEC_Status
*        
*   DEPENDENCIES
*                                                         
*       None
*                                                                      
*************************************************************************/

#ifndef DBG_ENG_EXEC_H
#define DBG_ENG_EXEC_H

/***** Global defines */

/* Initialize Parameters */

typedef struct _dbg_eng_exec_init_param_struct
{
    VOID *                      p_dbg_eng;      /* Pointer to the debug engine */
    
} DBG_ENG_EXEC_INIT_PARAM;

/* Terminate Parameters */

typedef struct _dbg_eng_exec_term_param_struct
{
    VOID *                      reserved;       /* Reserved for future development */
    
} DBG_ENG_EXEC_TERM_PARAM;

/* Go Parameters */

typedef struct _dbg_eng_exec_go_param_struct
{
    DBG_THREAD_ID               thread_id;      /* Thread ID to resume */
    
} DBG_ENG_EXEC_GO_PARAM;

/* Stop Parameters */

typedef struct _dbg_eng_exec_stop_param_struct
{
    DBG_THREAD_ID               thread_id;          /* Thread ID to stop */
    BOOLEAN                     use_direct_events;  /* Use directly called events */
    
} DBG_ENG_EXEC_STOP_PARAM;

/* Step Parameters */

typedef struct _dbg_eng_exec_step_param_struct
{
    DBG_THREAD_ID               step_thread_id;     /* Thread ID to step */
    DBG_THREAD_ID               go_thread_id;       /* Thread ID that will be resumed */
    DBG_THREAD_ID               stop_thread_id;     /* Thread ID that will be stopped */
    DBG_ENG_BKPT_HIT_FUNC       hit_func;           /* Function called on step complete */
    DBG_ENG_BKPT_HIT_FUNC_PARAM hit_func_param;     /* Parameters passed to function */   
    
} DBG_ENG_EXEC_STEP_PARAM;

/* Status Parameters */

typedef struct _dbg_eng_exec_stat_param_struct
{
    DBG_THREAD_ID               thread_id;          /* Thread ID for status */
    DBG_THREAD_STATUS *         p_thread_status;    /* Execution status of thread. */
   
} DBG_ENG_EXEC_STAT_PARAM;

/* Component control block */

typedef struct _dbg_eng_exec_cb_struct
{
    VOID *                      p_dbg_eng;          /* Pointer to the debug engine */
    
} DBG_ENG_EXEC_CB;

/***** Global functions */

DBG_STATUS DBG_ENG_EXEC_Initialize(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                                   DBG_ENG_EXEC_INIT_PARAM *    p_param);

DBG_STATUS DBG_ENG_EXEC_Terminate(DBG_ENG_EXEC_CB *             p_dbg_eng_exec,
                                  DBG_ENG_EXEC_TERM_PARAM *     p_param);

DBG_STATUS DBG_ENG_EXEC_Go(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                           DBG_ENG_EXEC_GO_PARAM *      p_param);

DBG_STATUS DBG_ENG_EXEC_Stop(DBG_ENG_EXEC_CB *              p_dbg_eng_exec,
                             DBG_ENG_EXEC_STOP_PARAM *      p_param);

DBG_STATUS DBG_ENG_EXEC_Step(DBG_ENG_EXEC_CB *              p_dbg_eng_exec,
                             DBG_ENG_EXEC_STEP_PARAM *      p_param);

DBG_STATUS DBG_ENG_EXEC_Status(DBG_ENG_EXEC_CB *            p_dbg_eng_exec,
                               DBG_ENG_EXEC_STAT_PARAM *    p_param);

#endif /* DBG_ENG_EXEC_H */

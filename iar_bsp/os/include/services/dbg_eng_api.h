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
*       dbg_eng_api.h
*
*   COMPONENT
*
*       Debug Agent - Debug Engine - API
*
*   DESCRIPTION
*
*       This file contains the C external interface for the component.
*
*   DATA STRUCTURES
*
*       DBG_EVENT_ID_HIT_PARAM
*       DBG_EVENT_ID_STEP_CPLT_PARAM
*       DBG_EVENT_HANDLER_PARAM
*       DBG_EVENT_HANDLER_FUNC
*       DBG_CMD_SESSION_OPEN_PARAM
*       DBG_CMD_SESSION_CLOSE_PARAM
*       DBG_CMD_SESSION_RESET_PARAM
*       DBG_CMD_SESSION_INFO_PARAM
*       DBG_CMD_THREAD_GET_CURRENT_PARAM
*       DBG_CMD_THREAD_SET_WORKING_PARAM
*       DBG_CMD_THREAD_GET_WORKING_PARAM
*       DBG_CMD_THREAD_GO_PARAM
*       DBG_CMD_THREAD_STOP_PARAM
*       DBG_CMD_THREAD_STEP_PARAM
*       DBG_CMD_THREAD_GET_FIRST_NEXT_PARAM
*       DBG_CMD_THREAD_INFO_PARAM
*       DBG_CMD_THREAD_ID_PARAM
*       DBG_CMD_MEMORY_READ_PARAM
*       DBG_CMD_MEMORY_WRITE_PARAM
*       DBG_CMD_REGISTER_READ_PARAM
*       DBG_CMD_REGISTER_WRITE_PARAM
*       DBG_CMD_BREAKPOINT_SET_PARAM
*       DBG_CMD_BREAKPOINT_CLEAR_PARAM
*       DBG_CMD_BREAKPOINT_CLEAR_ALL_PARAM
*       DBG_CMD_EVENT_HANDLER_REGISTER_PARAM
*       DBG_CMD_EVENT_HANDLER_UNREGISTER_PARAM
*       DBG_CMD
*       DBG_CMD_FUNC
*
*   FUNCTIONS
*
*       DBG_ENG_Command
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_ENG_API_H
#define DBG_ENG_API_H

/***** Global defines */

/* Debug Service Command Operation */

typedef enum _dbg_cmd_op_enum
{
    DBG_CMD_OP_NONE                             = -1,
    DBG_CMD_OP_SESSION_OPEN                     = 0,
    DBG_CMD_OP_SESSION_CLOSE                    = 1,
    DBG_CMD_OP_SESSION_RESET                    = 2,
    DBG_CMD_OP_SESSION_INFO                     = 3,
    DBG_CMD_OP_THREAD_GET_CURRENT               = 4,
    DBG_CMD_OP_THREAD_GET_FIRST                 = 5,
    DBG_CMD_OP_THREAD_GET_NEXT                  = 6,
    DBG_CMD_OP_THREAD_GO                        = 7,
    DBG_CMD_OP_THREAD_STOP                      = 8,
    DBG_CMD_OP_THREAD_STEP                      = 9,
    DBG_CMD_OP_THREAD_INFO                      = 10,
    DBG_CMD_OP_THREAD_ID                        = 11, 
    DBG_CMD_OP_THREAD_GRP                       = 12,   
    DBG_CMD_OP_MEMORY_READ                      = 13,
    DBG_CMD_OP_MEMORY_WRITE                     = 14,
    DBG_CMD_OP_REGISTER_READ                    = 15,
    DBG_CMD_OP_REGISTER_WRITE                   = 16,
    DBG_CMD_OP_BREAKPOINT_SET                   = 17,
    DBG_CMD_OP_BREAKPOINT_CLEAR                 = 18,
    DBG_CMD_OP_BREAKPOINT_CLEAR_ALL             = 19,
    DBG_CMD_OP_EVENT_HANDLER_REGISTER           = 20,
    DBG_CMD_OP_EVENT_HANDLER_UNREGISTER         = 21

} DBG_CMD_OP;

/* Debug Service Version ID */

typedef enum _dbg_version_id_enum
{
    DBG_VERSION_ID_NONE                         = -1,
    DBG_VERSION_ID_1_0                          = 0

} DBG_VERSION_ID;

/* Debug Service Session ID */

typedef INT         DBG_SESSION_ID;

/* Debug Service Session ID values */

#define DBG_SESSION_ID_NONE                     -1

/* Debug Service Thread ID */

typedef UNSIGNED    DBG_THREAD_ID;

/* Debug Service Thread ID values */

#define DBG_THREAD_ID_NONE                      ((DBG_THREAD_ID)-2)
#define DBG_THREAD_ID_ALL                       ((DBG_THREAD_ID)-1)
#define DBG_THREAD_ID_ANY                       ((DBG_THREAD_ID)0)

/* Debug Service Thread Status values. */

typedef enum _dbg_thread_status_enum
{
    DBG_THREAD_STATUS_NONE                      = -1,
    DBG_THREAD_STATUS_RUNNING                   = 0,
    DBG_THREAD_STATUS_STOPPED                   = 1

} DBG_THREAD_STATUS;

/* Debug Service Thread Information String Size. */

#define DBG_THREAD_INFO_STR_SIZE                32

/* Thread Group values */

typedef enum _dbg_thread_group_enum
{
    DBG_THREAD_GROUP_NONE,
    DBG_THREAD_GROUP_SYSTEM,
    DBG_THREAD_GROUP_APPLICATION  
    
} DBG_THREAD_GROUP;

/* Debug Service Target Command Type */

typedef enum _dbg_target_cmd_type_enum
{
    DBG_TARGET_CMD_TYPE_NONE                    = 0,
    DBG_TARGET_CMD_TYPE_EXEC                    = 1,
    DBG_TARGET_CMD_TYPE_OTHER                   = 2

} DBG_TARGET_CMD_TYPE;

/* Debug Service Memory Access Mode */

typedef enum _dbg_mem_access_mode_enum
{
    DBG_MEM_ACCESS_MODE_NONE                    = -1,
    DBG_MEM_ACCESS_MODE_ANY                     = 0,
    DBG_MEM_ACCESS_MODE_8_BIT                   = 1,
    DBG_MEM_ACCESS_MODE_16_BIT                  = 2,
    DBG_MEM_ACCESS_MODE_32_BIT                  = 3,
    DBG_MEM_ACCESS_MODE_64_BIT                  = 4

} DBG_MEM_ACCESS_MODE;

/* Debug Service Register ID */

typedef INT     DBG_REGISTER_ID;

/* Debug Service Register ID values */

#define DBG_REGISTER_ID_NONE                    -3
#define DBG_REGISTER_ID_EXPEDITE                -2
#define DBG_REGISTER_ID_ALL                     -1

/* Breakpoint ID */

typedef INT     DBG_BREAKPOINT_ID;

/* Predefined Breakpoint ID values */

#define DBG_BREAKPOINT_ID_NONE                  -3
#define DBG_BREAKPOINT_ID_STEP                  -2
#define DBG_BREAKPOINT_ID_HIT                   -1

/* Event ID */

typedef enum _dbg_event_id_enum
{
    DBG_EVENT_ID_NONE                           = -1,
    DBG_EVENT_ID_BKPT_HIT                       = 0,
    DBG_EVENT_ID_STEP_CPLT                      = 1,
    DBG_EVENT_ID_THD_STOP                       = 2

} DBG_EVENT_ID;

/* Event ID - Breakpoint Hit parameters */

typedef struct _dbg_event_id_bkpt_hit_param_struct
{
    /* exec_ctxt_id - ID of the execution context on which the breakpoint
       hit.  Breakpoints which are hit and do not belong to the debug
       engine will have an execution context of "system". */

    DBG_THREAD_ID                                   exec_ctxt_id;

    /* bkptID - ID of the breakpoint that hit.  A value of
       DBG_BREAKPOINT_ID_NONE indicates that the breakpoint hit does
       not belong to the debug engine (i.e. it may have been created by
       an external debugger via a memory write). */

    DBG_BREAKPOINT_ID                               bkpt_id;

} DBG_EVENT_ID_BKPT_HIT_PARAM;

/* Event ID - Step Complete parameters */

typedef struct _dbg_event_id_step_cplt_param_struct
{
    /* exec_ctxt_id - ID of the execution context on which the breakpoint
       hit.  Breakpoints which are hit and do not belong to the debug
       engine will have an execution context of "system". */

    DBG_THREAD_ID                                   exec_ctxt_id;

    /* bkpt_id - ID of the breakpoint that hit.  A value of
       DBG_BREAKPOINT_ID_NONE indicates that the breakpoint hit does
       not belong to the debug engine (i.e. it may have been created by
       an external debugger via a memory write). */

    DBG_BREAKPOINT_ID                               bkpt_id;

} DBG_EVENT_ID_STEP_CPLT_PARAM;

/* Event ID - Thread Stop parameters */

typedef struct _dbg_event_id_thd_stop_param_struct
{
    /* Execution Context ID - ID of the execution context that stopped. */

    DBG_THREAD_ID                                   exec_ctxt_id;

} DBG_EVENT_ID_THD_STOP_PARAM;

/* Handler parameters */

typedef struct _dbg_event_handler_param_struct
{
    /* ID - ID value of the event that occured. */

    DBG_EVENT_ID                                    id;

    /* Operation Parameters - Parameters for the operation. */

    union _dbg_event_id_param_union
    {
        DBG_EVENT_ID_BKPT_HIT_PARAM                 bkpt_hit;
        DBG_EVENT_ID_STEP_CPLT_PARAM                step_cplt;
        DBG_EVENT_ID_THD_STOP_PARAM                 thd_stop;

    } id_param;

} DBG_EVENT_HANDLER_PARAM;

/* Event Handler Function */

typedef void (*DBG_EVENT_HANDLER_FUNC)(VOID *                       p_context,
                                       DBG_EVENT_HANDLER_PARAM *    p_param);

/* Debug Service Command API parameters */

/* Debug Service Command - Session Open parameters */

typedef struct _dbg_cmd_session_open_param_struct
{
    /* Version ID - Indicates the version of the Debug Service API to
       be used for the session. */

    DBG_VERSION_ID                                  version_id;

    /* Session ID - Return paramter that will be updated to contain a
       debug session ID if the operation is successful.  If the operation
       fails the value is undefined. */

    DBG_SESSION_ID *                                p_session_id;

} DBG_CMD_SESSION_OPEN_PARAM;

/* Debug Service Command - Session Close parameters */

typedef struct _dbg_cmd_session_close_param_struct
{
    /* Reserved for future development. */

    INT                                             reserved;

} DBG_CMD_SESSION_CLOSE_PARAM;

/* Debug Service Command - Session Reset parameters */

typedef struct _dbg_cmd_session_reset_param_struct
{
    /* Reserved for future development. */

    INT                                             reserved;

} DBG_CMD_SESSION_RESET_PARAM;

/* Debug Service Command - Session Info parameters */

typedef struct _dbg_cmd_session_info_param_struct
{
    /* Reserved for future development. */

    INT                                             reserved;

} DBG_CMD_SESSION_INFO_PARAM;

/* Debug Service Command - Thread Get Current parameters */

typedef struct _dbg_cmd_thread_get_current_param_struct
{
    /* Thread ID - Return parameter that will be updated to contain the
       ID value of the current thread if the operation is successful.  If
       the operation fails the value is undefined. */

    DBG_THREAD_ID *                                 p_thread_id;

} DBG_CMD_THREAD_GET_CURRENT_PARAM;

/* Debug Service Command - Thread Go parameters */

typedef struct _dbg_cmd_thread_go_param_struct
{
    /* Thread ID - The ID of the thread to be resumed. */
    
    DBG_THREAD_ID                                   thread_id;

} DBG_CMD_THREAD_GO_PARAM;

/* Debug Service Command - Thread Stop parameters */

typedef struct _dbg_cmd_thread_stop_param_struct
{
    /* Thread ID - The ID of the thread to be stopped. */
    
    DBG_THREAD_ID                                   thread_id;

} DBG_CMD_THREAD_STOP_PARAM;

/* Debug Service Command - Thread Step parameters */

typedef struct _dbg_cmd_thread_step_param_struct
{
    /* Step Thread ID - The ID of the thread that will be stepped. */
    
    DBG_THREAD_ID                                   step_thread_id;

    /* Go Thread ID - The ID of the thread to run when a step
       operation is started. */

    DBG_THREAD_ID                                   go_thread_id;

    /* Stop Thread ID - The ID of the thread to be stopped when the step
       operation is completed. */
    
    DBG_THREAD_ID                                   stop_thread_id;

} DBG_CMD_THREAD_STEP_PARAM;

/* Debug Service Command - Thread Get First / Next parameters */

typedef struct _dbg_cmd_thread_get_first_next_param_struct
{
    /* Thread ID - Return parameter that will be updated to contain the
       ID value of the first or next thread if the operation is
       successful.  If the operation fails the value is undefined. */

    DBG_THREAD_ID *                                 p_thread_id;

} DBG_CMD_THREAD_GET_FIRST_NEXT_PARAM;

/* Debug Service Command - Thread Info parameters */

typedef struct _dbg_cmd_thread_info_param_struct
{
    /* Thread ID - Indicates the ID of the thread to retrive info on. */

    DBG_THREAD_ID                                   thread_id;

    /* Thread Status - Updated to indicate the threads status if the
                       operation is successful.  If the operation fails
                       the value is undefined. */

    DBG_THREAD_STATUS *                             p_thread_status;

    /* Thread Information String - Updated to contain a string containing
       thread-specific information. */
       
    CHAR                                            thread_info_str[DBG_THREAD_INFO_STR_SIZE];

} DBG_CMD_THREAD_INFO_PARAM;

/* Debug Service Command - Retrieves the ID of a thread. */

typedef struct _dbg_cmd_thread_id_struct
{
    /* Task Pointer - Pointer to the OS task to retrive the the TID for. */

    NU_TASK *                                       p_os_thread;

    /* Thread ID - Indicates the pointer to the thread ID to retrive. */

    DBG_THREAD_ID *                                 p_thread_id;

} DBG_CMD_THREAD_ID_PARAM;

/* Debug Service Command - Sets the group of a thread. */

typedef struct _dbg_cmd_thread_grp_struct
{
    /* Thread ID - Indicates the pointer to the thread ID to retrive. */

    DBG_THREAD_ID                                   thread_id;

    /* Thread Group - The new thread group value. */
    
    DBG_THREAD_GROUP                                thread_group;

} DBG_CMD_THREAD_GRP_PARAM;

/* Debug Service Command - Memory Read parameters */

typedef struct _dbg_cmd_memory_read_param_struct
{
    /* Read Source - Pointer to memory location to be read from. */

    VOID *                                          p_read;

    /* Read Size - The requested read size (in bytes). */

    UINT                                            read_size;

    /* Access Mode - The mode or manner in which memory will be accessed
                     during the operation. */

    DBG_MEM_ACCESS_MODE                             access_mode;

    /* Read Buffer - Pointer to buffer (memory) where read data will be
                     placed. */

    VOID *                                          p_read_buffer;

    /* Actual Read Size - Return parameter that will contain the actual
                          size (in bytes) of the memory read in to the
                          read buffer if the operation succeeds.  If the
                          operation fails the value is undefined. */

    UINT *                                          p_actual_read_size;

} DBG_CMD_MEMORY_READ_PARAM;

/* Debug Service Command - Memory Write parameters */

typedef struct _dbg_cmd_memory_write_param_struct
{
    /* Write Address - Pointer to memory location to be written to. */

    VOID *                                          p_write;

    /* Write Size - The requested write size (in bytes). */

    UINT                                            write_size;

    /* Access Mode - The mode or manner in which memory will be accessed
                     during the operation. */

    DBG_MEM_ACCESS_MODE                             access_mode;

    /* Write Buffer - Pointer to memory containing the data to be
                      written. */

    VOID *                                          p_write_buffer;

    /* Actual Write Size - Return parameter that will contain the actual
                           size (in bytes) of the memory written if the
                           operation succeeds.  If the operation fails the
                           value is undefined. */

    UINT *                                          p_actual_write_size;

} DBG_CMD_MEMORY_WRITE_PARAM;

/* Debug Service Command - Register Read parameters */

typedef struct _dbg_cmd_register_read_param_struct
{
    /* Thread ID - ID of the thread to read registers from. */
    
    DBG_THREAD_ID                                   thread_id;
    
    /* Register ID - ID of register(s) to be read. */

    DBG_REGISTER_ID                                 register_id;

    /* Register Data - Return value that will contain the value(s) of the
                       register(s) if the operation is successful.  The
                       memory pointed to must be large enough to contain
                       a appropriate value(s) for the specified
                       register(s). */

    VOID *                                          p_register_data;

    /* Actual Register Data Size - Return parameter that will be updated
                                   to contain the actual size of the
                                   register data read if the operation is
                                   successful.  If the operation fails the
                                   value is undefined. */

    UINT *                                          p_actual_reg_data_size;

} DBG_CMD_REGISTER_READ_PARAM;

/* Debug Service Command - Register Write parameters */

typedef struct _dbg_cmd_register_write_param_struct
{
    /* Thread ID - ID of the thread to read registers from. */
    
    DBG_THREAD_ID                                   thread_id;
        
    /* registerID - ID of register(s) to be written. */

    DBG_REGISTER_ID                                 register_id;

    /* Register Data - Pointer to memory containing appropriate value(s)
                       that will be written to the specified
                       register(s). */

    VOID *                                          p_register_data;

} DBG_CMD_REGISTER_WRITE_PARAM;

/* Debug Service Command - Breakpoint Set parameters */

typedef struct _dbg_cmd_breakpoint_set_param_struct
{
    /* Hit Thread ID - ID of the thread that the breakpoint will 'hit'
       on. */
    
    DBG_THREAD_ID                                   hit_thread_id;    
    
    /* Stop Thread ID - ID of the thread that will be stopped when the
       breakpoint 'hits'. */
    
    DBG_THREAD_ID                                   stop_thread_id;    
    
    /* Address - Address where breakpoint should be set. */

    VOID *                                          p_address;

    /* Pass Count - The number of times that a breakpoint will be "passed"
                    before execution is actually stopped.  A value of 0
                    indicates that the breakpoint will hit on the first
                    encounter. */

    UINT                                            pass_count;

    /* Hit Count - The number of times that a breakpoint will be "hit"
                   before it is automatically removed.  A value of 0
                   indicates that the breakpoint is permanent. */
                   
    UINT                                            hit_count;
    
} DBG_CMD_BREAKPOINT_SET_PARAM;

/* Debug Service Command - Breakpoint Clear parameters */

typedef struct _dbg_cmd_breakpoint_clear_param_struct
{
    /* Thread ID - ID of the thread associated with the breakpoint to be
       removed.  A value of DBG_THREAD_ID_NONE indicates that all
       breakpoints at the specified address should be removed. */
    
    DBG_THREAD_ID                                   thread_id;    
    
    /* Address - Address where breakpoint should be cleared. */

    VOID *                                          p_address;

} DBG_CMD_BREAKPOINT_CLEAR_PARAM;

/* Debug Service Command - Breakpoint Clear All parameters */

typedef struct _dbg_cmd_breakpoint_clear_all_param_struct
{
    /* Reserved for future development. */

    INT                                             reserved;

} DBG_CMD_BREAKPOINT_CLEAR_ALL_PARAM;

/* Debug Service Command - Event Handler Register parameters */

typedef struct _dbg_cmd_event_handler_register_param_struct
{
    /* Handler - Handler function that will be called for events. */

    DBG_EVENT_HANDLER_FUNC                          handler_func;

    /* Handler Context - Context value to be passed to the handler
                         function. */

    VOID *                                          handler_context;

} DBG_CMD_EVENT_HANDLER_REGISTER_PARAM;

/* Debug Service Command - Event Handler Unregister parameters */

typedef struct _dbg_cmd_event_handler_unregister_param_struct
{
    /* Reserved for future development. */

    INT                                             reserved;

} DBG_CMD_EVENT_HANDLER_UNREGISTER_PARAM;

typedef struct _dbg_cmd_struct
{
    /* Operation - The operation to be performed. */

    DBG_CMD_OP                                      op;

    /* Operation Parameters - Parameters for the operation. */

    union _dbg_cmd_op_param_union
    {
        DBG_CMD_SESSION_OPEN_PARAM                  ses_opn;
        DBG_CMD_SESSION_CLOSE_PARAM                 ses_cls;
        DBG_CMD_SESSION_RESET_PARAM                 ses_rst;
        DBG_CMD_SESSION_INFO_PARAM                  ses_info;
        DBG_CMD_THREAD_GET_CURRENT_PARAM            thd_get_cur;
        DBG_CMD_THREAD_GET_FIRST_NEXT_PARAM         thd_get_fst_nxt;
        DBG_CMD_THREAD_GO_PARAM                     thd_go;
        DBG_CMD_THREAD_STOP_PARAM                   thd_stop;
        DBG_CMD_THREAD_STEP_PARAM                   thd_step;
        DBG_CMD_THREAD_INFO_PARAM                   thd_info;
        DBG_CMD_THREAD_ID_PARAM                     thd_id;
        DBG_CMD_THREAD_GRP_PARAM                    thd_grp;
        DBG_CMD_MEMORY_READ_PARAM                   mem_rd;
        DBG_CMD_MEMORY_WRITE_PARAM                  mem_wrt;
        DBG_CMD_REGISTER_READ_PARAM                 reg_rd;
        DBG_CMD_REGISTER_WRITE_PARAM                reg_wrt;
        DBG_CMD_BREAKPOINT_SET_PARAM                bkpt_set;
        DBG_CMD_BREAKPOINT_CLEAR_PARAM              bkpt_clr;
        DBG_CMD_BREAKPOINT_CLEAR_ALL_PARAM          bkpt_clr_all;
        DBG_CMD_EVENT_HANDLER_REGISTER_PARAM        evt_hdlr_reg;
        DBG_CMD_EVENT_HANDLER_UNREGISTER_PARAM      evt_hdlr_unrg;

    } op_param;

} DBG_CMD;

/* Debug Service Command Function */

typedef DBG_STATUS (*DBG_CMD_FUNC)(DBG_SESSION_ID         session_id,
                                   DBG_CMD *              p_cmd);

/***** Global functions */

/* Debug Service API */

DBG_STATUS DBG_ENG_Command(DBG_SESSION_ID           session_id,
                          DBG_CMD *                 p_cmd);

#endif /* DBG_ENG_API_H */

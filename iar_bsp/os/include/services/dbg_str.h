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
*       dbg_str.h                        
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Debug Agent - Strings
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C definitions for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                                                                      
*       DBG_STR_VALUE_STRING
*               
*   FUNCTIONS
*
*       DBG_STR_Value_String_Get
*       DBG_STR_String_Combine
*       DBG_STR_String_From_UINT
*       DBG_STR_String_To_UINT
*       DBG_STR_String_From_BYTE
*
*   DEPENDENCIES
*                                                         
*       None                                  
*                                                                      
*************************************************************************/

#ifndef DBG_STR_H
#define DBG_STR_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* API defines */

#define DBG_STR_RADIX_BINARY                    2
#define DBG_STR_RADIX_DECIMAL                   10
#define DBG_STR_RADIX_HEXADECIMAL               16

#define DBG_STR_INC_PREFIX_ENABLE               NU_TRUE
#define DBG_STR_INC_PREFIX_DISABLE              NU_FALSE

#define DBG_STR_INC_LEADING_ZEROS_ENABLE        NU_TRUE
#define DBG_STR_INC_LEADING_ZEROS_DISABLE       NU_FALSE

#define DBG_STR_INC_NULL_TERM_ENABLE            NU_TRUE
#define DBG_STR_INC_NULL_TERM_DISABLE           NU_FALSE

/* Value String - This is the format for an element of the
   string array. */

typedef struct _dbg_str_value_string_struct
{
    INT                 value;      /* The value of the status. */
    CHAR *              string;     /* The string associated with the status. */

} DBG_STR_VALUE_STRING;

/* Value String Type - This is the general type of string that is to be 
   looked up. */
   
typedef enum _dbg_str_value_string_type_enum
{
    DBG_STR_VALUE_STRING_TYPE_NONE,
    DBG_STR_VALUE_STRING_TYPE_DBG_STATUS,
    DBG_STR_VALUE_STRING_TYPE_DBG_CMD_OP,
    DBG_STR_VALUE_STRING_TYPE_DBG_EVT_ID,
    DBG_STR_VALUE_STRING_TYPE_RSP_PKT_TYPE    
    
} DBG_STR_VALUE_STRING_TYPE;

/* RMD Status Strings Init - This is the initalization array for the RMD
   status strings array. */

#define DBG_STR_DBG_STATUS_STRING_INIT          {{DBG_STATUS_NONE,                  "None"}, \
                                                 {DBG_STATUS_OK,                    "OK"}, \
                                                 {DBG_STATUS_TOO_MANY_CMDS,         "Too many commands"}, \
                                                 {DBG_STATUS_NONEXISTENT_CMD,       "Non-existent command"}, \
                                                 {DBG_STATUS_BUFFER_TOO_SMALL,      "Buffer too small"}, \
                                                 {DBG_STATUS_BAD_FLAVOR,            "Bad flavor"}, \
                                                 {DBG_STATUS_OUT_OF_RANGE,          "Out of range"}, \
                                                 {DBG_STATUS_ALREADY_EXISTS,        "Already exists"}, \
                                                 {DBG_STATUS_OUT_OF_MEMORY,         "Out of memory"}, \
                                                 {DBG_STATUS_INVALID_THREAD,        "Invalid thread"}, \
                                                 {DBG_STATUS_BAD_PRIORITY,          "Bad priority"}, \
                                                 {DBG_STATUS_NOT_SUPPORTED_BY_OS,   "Not supported by OS"}, \
                                                 {DBG_STATUS_ALREADY_ATTACHED,      "Already attached"}, \
                                                 {DBG_STATUS_NOT_ATTACHED,          "Not attached"}, \
                                                 {DBG_STATUS_OUT_OF_BOUNDS,         "Out of bounds"}, \
                                                 {DBG_STATUS_INVALID_ADDRESS,       "Invalid address"}, \
                                                 {DBG_STATUS_BUSS_ERR,              "Bus Error"}, \
                                                 {DBG_STATUS_PROTECTED,             "Protected"}, \
                                                 {DBG_STATUS_VERIFY_FAIL,           "Verify fail"}, \
                                                 {DBG_STATUS_AUTHENTICATION_FAILED, "Authentication failed"}, \
                                                 {DBG_STATUS_BAD_WIDTH,             "Bad width"}, \
                                                 {DBG_STATUS_BAD_SEQUENCE,          "Bad sequence"}, \
                                                 {DBG_STATUS_BAD_STACK_SIZE,        "Bad stack size"}, \
                                                 {DBG_STATUS_INVALID_BREAKPOINT,    "Invalid breakpoint"}, \
                                                 {DBG_STATUS_BAD_COMMAND_CODE,      "Bad command code"}, \
                                                 {DBG_STATUS_ALREADY_ACTIVE,        "Already active"}, \
                                                 {DBG_STATUS_NOT_ACTIVE,            "Not active"}, \
                                                 {DBG_STATUS_BAD_OFFSET,            "Bad offset"}, \
                                                 {DBG_STATUS_NO_REPLY,              "No reply"}, \
                                                 {DBG_STATUS_NO_SYS_CALL,           "No system call"}, \
                                                 {DBG_STATUS_NOT_WRITABLE,          "Not writable"}, \
                                                 {DBG_STATUS_CANNOT_SUSPEND_THREAD, "Cannot suspend thread"}, \
                                                 {DBG_STATUS_CANNOT_RESUME_THREAD,  "Cannot resume thread"}, \
                                                 {DBG_STATUS_INVALID_CONTEXT,       "Invalid context"}, \
                                                 {DBG_STATUS_STILL_ATTACHED,        "Still attached"}, \
                                                 {DBG_STATUS_FAILED,                "Failed"}, \
                                                 {DBG_STATUS_NOT_IN_DEBUG_SCOPE,    "Not in debug scope"}, \
                                                 {DBG_STATUS_NOT_SUPPORTED,         "Not supported"}, \
                                                 {DBG_STATUS_INVALID_ID,            "Invalid ID"}, \
                                                 {DBG_STATUS_RESOURCE_UNAVAILABLE,  "Resource unavailable"}, \
                                                 {DBG_STATUS_INVALID_PATH,          "Invalid path"}, \
                                                 {DBG_STATUS_INVALID_SIZE,          "Invalid size"}, \
                                                 {DBG_STATUS_NOT_IN_SESSION,        "Not in session"}, \
                                                 {DBG_STATUS_INVALID_OPERATION,     "Invalid operation"}, \
                                                 {DBG_STATUS_PROTECTED_ACCESS,      "Protected access"}, \
                                                 {DBG_STATUS_STILL_ACTIVE,          "Still active"}, \
                                                 {DBG_STATUS_INVALID_STATE,         "Invalid state"}, \
                                                 {DBG_STATUS_INVALID_FORMAT,        "Invalid format"}, \
                                                 {DBG_STATUS_INVALID_TYPE,          "Invalid type"}, \
                                                 {DBG_STATUS_INVALID_DATA,          "Invalid data"}, \
                                                 {DBG_STATUS_INVALID_MODE,          "Invalid mode"}, \
                                                 {DBG_STATUS_INVALID_PARAMETERS,    "Invalid parameters"}}

/* Debug Command Type Strings Init - This is the initalization array for 
   the Debug Engine Command Type strings array. */    
    
#define DBG_STR_DBUG_CMD_OP_STRING_INIT        {{DBG_CMD_OP_NONE,                           "None"}, \
                                                {DBG_CMD_OP_SESSION_OPEN,                   "Ses Opn"}, \
                                                {DBG_CMD_OP_SESSION_RESET,                  "Ses Rst"}, \
                                                {DBG_CMD_OP_SESSION_CLOSE,                  "Ses Cls"}, \
                                                {DBG_CMD_OP_SESSION_INFO,                   "Ses Info"}, \
                                                {DBG_CMD_OP_THREAD_GET_CURRENT,             "Thd Get Cur"}, \
                                                {DBG_CMD_OP_THREAD_GET_FIRST,               "Thd Get Fst"}, \
                                                {DBG_CMD_OP_THREAD_GET_NEXT,                "Thd Get Nxt"}, \
                                                {DBG_CMD_OP_THREAD_GO,                      "Thd Go"}, \
                                                {DBG_CMD_OP_THREAD_STOP,                    "Thd Stop"}, \
                                                {DBG_CMD_OP_THREAD_STEP,                    "Thd Step"}, \
                                                {DBG_CMD_OP_THREAD_INFO,                    "Thd Info"}, \
                                                {DBG_CMD_OP_THREAD_ID,                      "Thd Id"}, \
                                                {DBG_CMD_OP_MEMORY_READ,                    "Mem Rd"}, \
                                                {DBG_CMD_OP_MEMORY_WRITE,                   "Mem Wrt"}, \
                                                {DBG_CMD_OP_REGISTER_READ,                  "Reg Rd"}, \
                                                {DBG_CMD_OP_REGISTER_WRITE,                 "Reg Wrt"}, \
                                                {DBG_CMD_OP_BREAKPOINT_SET,                 "Bkpt Set"}, \
                                                {DBG_CMD_OP_BREAKPOINT_CLEAR,               "Bkpt Clr"}, \
                                                {DBG_CMD_OP_BREAKPOINT_CLEAR_ALL,           "Bkpt Clr All"}, \
                                                {DBG_CMD_OP_EVENT_HANDLER_REGISTER,         "Hdlr Reg"}, \
                                                {DBG_CMD_OP_EVENT_HANDLER_UNREGISTER,       "Hdlr Unrg"}}
    
/* Debug Event Type Strings Init - This is the initalization array for 
   the Debug Engine Event Type strings array. */    
    
#define DBG_STR_DBUG_EVT_ID_STRING_INIT        {{DBG_EVENT_ID_NONE,                         "None"}, \
                                                {DBG_EVENT_ID_BKPT_HIT,                     "Bkpt Hit"}, \
                                                {DBG_EVENT_ID_STEP_CPLT,                    "Step Cplt"}, \
                                                {DBG_EVENT_ID_THD_STOP,                     "Thd Stop"}}    
    
///* RSP Packet Type Strings Init - This is the initalization array for 
//   the RSP Packet Type strings array. */    
//    
#define DBG_STR_RSP_PKT_TYPE_STRING_INIT        {{109,                    "m Pkt"}, \
                                                 {77,                     "M Pkt"}, \
                                                 {103,                    "g Pkt"}, \
                                                 {71,                     "G Pkt"}, \
                                                 {112,                    "p Pkt"}, \
                                                 {80,                     "P Pkt"}, \
                                                 {99,                     "c Pkt"}, \
                                                 {67,                     "s Pkt"}, \
                                                 {118,                    "BREAK Pkt"}, \
                                                 {90,                     "Z Pkt"}, \
                                                 {122,                    "z Pkt"}, \
                                                 {68,                     "q Pkt"}, \
                                                 {3,                      "QUERY Pkt"}, \
                                                 {0,                      "ACK Pkt"}, \
                                                 {1,                      "NO_ACK Pkt"}}
    
/***** Global functions */

DBG_STATUS DBG_STR_Value_String_Get(DBG_STR_VALUE_STRING_TYPE       value_string_type,
                                    UINT                            value,
                                    CHAR **                         p_string);

UINT     DBG_STR_String_Combine(CHAR *          string_0,
                                CHAR *          string_1,
                                CHAR *          string_2,
                                CHAR *          string_3,
                                CHAR *          string_4,
                                UINT            string_max_length,
                                CHAR *          string);

DBG_STATUS DBG_STR_String_From_UINT(CHAR *      str,
                                    UINT        value,
                                    UINT        radix,
                                    BOOLEAN     inc_prefix,
                                    BOOLEAN     inc_lead_zeros);

DBG_STATUS DBG_STR_String_To_UINT(CHAR *    str,
                                  UINT *    p_value);

DBG_STATUS DBG_STR_String_From_BYTE(CHAR *      str,
                                    UINT        value,
                                    UINT        radix,
                                    BOOLEAN     inc_prefix,
                                    BOOLEAN     inc_lead_zeros,
                                    BOOLEAN     inc_null_term);

#ifdef __cplusplus
}
#endif

#endif /* DBG_STR_H */

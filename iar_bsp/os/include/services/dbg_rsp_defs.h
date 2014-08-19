/*************************************************************************
*
*               Copyright 2009 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       dbg_rsp_defs.h
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP)
*
*   DESCRIPTION
*
*       This file contains constant definitions and function macros
*       for the RSP Support Component.
*
*   DATA STRUCTURES
*
*       RSP_STATUS
*       RSP_ERRNO
*       RSP_CALLBACK_IDX_TYPE
*       RSP_VRUN
*       RSP_VKILL
*       RSP_CALLBACK
*       RSP_MODE
*       RSP_NOTIFY_SIGNAL
*       RSP_ERROR_TYPE
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef RSP_DEFS_H
#define RSP_DEFS_H

/* Define communication buffers */
#define         RSP_MAX_PACKET_SIZE_SUPPORTED       0x200
#define         RSP_COMMS_BUFFER_SIZE               RSP_MAX_PACKET_SIZE_SUPPORTED * 2
#define         RSP_INPUT_BUFF_SIZE                 RSP_MAX_PACKET_SIZE_SUPPORTED
#define         RSP_OUTPUT_BUFF_SIZE                RSP_MAX_PACKET_SIZE_SUPPORTED
#define         RSP_BUFFERED_COMMS                  1
#define         RSP_NON_BUFFERED_COMMS              2

/* RSP status booleans */
#define         RSP_STATUS_BOOL_TRUE                1
#define         RSP_STATUS_BOOL_FALSE               0

/* RSP Notification signals */
#define         RSP_BREAKPOINT_NOTIFICATION                 1
#define         RSP_SINGLE_STEP_COMPLETE_NOTIFICATION       2
#define         RSP_ACK_NOTIFICATION                        3
#define         RSP_NO_ACK_NOTIFICATION                     4


/* Macros */
#define         RSP_REGISTER_PACKET_HANDLER(idx,pfn)    \
                    RSP_Packet_Handler[idx] = (RSP_STATUS (*) (CHAR *    p_rsp_cmd_buff, CHAR * p_rsp_resp_buff, \
                        UINT * rsp_resp_size)) (pfn)

#define         RSP_PACKET_HANDLER_EXECUTE(idx,p_rsp_cmd_buff, p_rsp_resp_buff, rsp_resp_size)    \
                    RSP_Packet_Handler[idx](p_rsp_cmd_buff, p_rsp_resp_buff, rsp_resp_size)

/* RSP callback context macros. */
#define         RSP_REGISTER_CALLBACK_CONTEXT(idx,p_context)  RspCallbackContextArray[idx] = p_context

#define         RSP_GET_CALLBACK_CONTEXT(idx)                RspCallbackContextArray[idx]

/* RSP callback handler registration macros. */
#define         RSP_REGISTER_CALLBACK_HANDLER(idx,pfn)    \
                    RSP_Registration_Handler[idx] = (RSP_STATUS (*) (RSP_CALLBACK* pRspCallback)) (pfn)

#define         RSP_CALLBACK_HANDLER_EXECUTE(idx, pRspCallback)  \
                    RSP_Registration_Handler[idx](pRspCallback)

#define         RSP_GET_CALLBACK_HANDLER_POINTER(idx)        RSP_Registration_Handler[idx]

/* RSP Specific Definitions */
#define         MAX_RSP_CMDS                        128

/* Supported RSP packets */
#define         RSP_READ_MEMORY                     "m"             /* "m" Packet *//* Required */
#define         RSP_WRITE_MEMORY                    "M"             /* "M" Packet *//* Required */
#define         RSP_READ_REGISTER_GROUP             "g"             /* "g" Packet *//* Required */
#define         RSP_WRITE_REGISTER_GROUP            "G"             /* "G" Packet *//* Required */
#define         RSP_READ_REGISTER                   "p"             /* "p" Packet */
#define         RSP_WRITE_REGISTER                  "P"             /* "P" Packet */
#define         RSP_RESUME_TARGET                   "c"             /* "c" Packet *//* Required */
#define         RSP_SINGLE_STEP                     "s"             /* "s" Packet *//* Required */
#define         RSP_HALT_TARGET                     3               /* "." Packet */
#define         RSP_INSERT_BREAKPOINT               "Z0,"           /* "Z" Packet */
#define         RSP_CLEAR_BREAKPOINT                "z0,"           /* "z" Packet */
#define         RSP_DISCONNECT                      "D"             /* "D" Packet */
#define         RSP_KILL                            "k"             /* "k" Packet */
#define         RSP_GENERAL_QUERY_GET               "qSupported"    /* "q" Packet */
#define         RSP_REASON_TARGET_STOPPED           "?"             /* "?" Packet */
#define         RSP_SET_THREAD                      "Hc"            /* "H" Packet */
#define         RSP_V_PACKET                        "vCont"         /* "v" Packet */
#define         RSP_THREAD_ALIVE                    "T"             /* "T" Packet */
#define         RSP_Q_PACKET                        "QNonStop:"     /* "Q" Packet */
#define         RSP_EXTENDED_MODE                   "!"             /* "!" Packet */


#define         RSP_COMMA_DELIMITER                 ','
#define         RSP_COLON_DELIMITER                 ':'
#define         RSP_SEMI_COLON_DELIMITER            ';'
#define         RSP_EQUAL_SIGN_DELIMITER            '='
#define         RSP_NUL_DELIMITER                   0

#define         RSP_ACK_CHAR                        '+'
#define         RSP_NO_ACK_CHAR                     '-'
#define         RSP_RUN_LEN_ENCODE_CHAR             '*'
#define         RSP_PKT_START_CHAR                  '$'
#define         RSP_PKT_END_CHAR                    '#'
#define         RSP_ESCAPE_CHAR                     '}'
#define         RSP_ESCAPE_VALUE                    0x20
#define         RSP_F_PKT_OVHD                      0xA
#define         RSP_HALT_PKT_CHAR                   3

#define         RSP_NO_ACTION                       1
#define         RSP_PROCESS_PACKET                  2
#define         RSP_ACK_AND_PROCESS_PACKET          3
#define         RSP_REQ_SERVER_RETRANSMISSION       4
#define         RSP_REQ_CLIENT_RETRANSMISSION       5

#define         RSP_PARSING_INITIAL                 0
#define         RSP_PARSE_PKT_BODY                  1
#define         RSP_PARSE_PKT_CHKSUM                2
#define         RSP_VALIDATE_CHKSUM                 3


/* Define RSP Status and Error Codes */
typedef enum _rsp_status_enum
{
    RSP_STATUS_NONE                   = -7801,
    RSP_STATUS_OK                     = -7802,
    RSP_STATUS_FAILED                 = -7803,
    RSP_STATUS_ERROR_RESP             = -7804,
    RSP_STATUS_NO_RESP                = -7805,
    RSP_STATUS_RESOURCE_NOT_AVAIL     = -7806,
    RSP_STATUS_DEBUG_CALL_FAILED      = -7807,
    RSP_STATUS_PACKET_ERROR           = -7808,
    RSP_STATUS_INVALID_MODE           = -7809,
    RSP_STATUS_NOT_SUPPORTED          = -7810,
    RSP_STATUS_TRUE                   = 1,
    RSP_STATUS_FALSE                  = 0

} RSP_STATUS;


/* Errno values for vFile RSP command. The values were taken from the GDB Manual 7.0.xxx.
   See www.opengroup.org for more information on errno values.*/
typedef enum _rsp_errno_enum
{
    RSP_EPERM                         = 1,               /* Operation not permitted. */
    RSP_ENOENT                        = 2,               /* No such file or directory. */
    RSP_EINTR                         = 4,               /* Interrupted function. */
    RSP_EBADF                         = 9,               /* Bad file descriptor. */
    RSP_EACCES                        = 13,              /* Permission denied. */
    RSP_EFAULT                        = 14,              /* Bad address. */
    RSP_EBUSY                         = 16,              /* Device or resource busy. */
    RSP_EEXIST                        = 17,              /* File exists. */
    RSP_ENODEV                        = 19,              /* No such device. */
    RSP_ENOTDIR                       = 20,              /* Not a directory. */
    RSP_EISDIR                        = 21,              /* Is a directory. */
    RSP_EINVAL                        = 22,              /* Invalid argument. */
    RSP_ENFILE                        = 23,              /* Too many files open in system. */
    RSP_EMFILE                        = 24,              /* Too many open files. */
    RSP_EFBIG                         = 27,              /* File too large. */
    RSP_ENOSPC                        = 28,              /* No space left on device. */
    RSP_ESPIPE                        = 29,              /* Invalid seek. */
    RSP_EROFS                         = 30,              /* Read-only file system. */
    RSP_ENAMETOOLONG                  = 91,              /* Filename too long. */
    RSP_EUNKNOWN                      = 9999

} RSP_ERRNO;


/* Supported RSP callback index type definitions. */
typedef enum _rsp_callback_idx_type_enum
{
    RSP_CALLBACK_VRUN,
    RSP_CALLBACK_VKILL,
    RSP_CALLBACK_END

} RSP_CALLBACK_IDX_TYPE;


/* RSP vRun structure definition */
typedef struct _rsp_vrun_struct
{
    NU_TASK*           pTaskCb;             /* Pointer to TCB of DLE application. */
    CHAR **            argv;                /* Array of pointers to null terminated strings. */
    INT                argc;                /* Number of arguments in argv. */
    VOID *             app_offset;          /* Offset of application in memory. */

} RSP_VRUN;


/* RSP vKill structure definition */
typedef struct _rsp_vkill_struct
{
    INT                pid;                 /* Process ID. */

} RSP_VKILL;


/* RSP callback structure definition */
typedef struct _rsp_callback_struct
{
    RSP_ERRNO          rsp_errno;           /* Error value returned to the GDB client. */
    VOID *             p_context;

    union _op_parm_union
    {
        RSP_VRUN           rsp_vrun;
        RSP_VKILL          rsp_vkill;

    } op_parm;

} RSP_CALLBACK;


/* RSP operating mode (non-stop, all-stop, etc.).  Related to QNonStop
   packet values. */
typedef enum _rsp_mode_enum
{
    RSP_MODE_ALL_STOP           = 0,
    RSP_MODE_NON_STOP           = 1

} RSP_MODE;

/* Type of signal to send with a % notification packet */
typedef enum _rsp_notify_signal
{
    RSP_NOTIFY_SIGNAL_T00       = 0,
    RSP_NOTIFY_SIGNAL_T05       = 1

} RSP_NOTIFY_SIGNAL;

/* Error message types. */

typedef enum _rsp_error_type_enum
{
    RSP_ERROR_E01_MSG           = 0,    /* Bad message from GDB client */
    RSP_ERROR_E02_DBG_ENG       = 1,    /* Error message from debug engine */
    RSP_ERROR_E03_MEM           = 2,    /* Memory Allocation/deallocation failure */
    RSP_ERROR_E04_DLE           = 3,    /* DLE error message */
    RSP_ERROR_E05_INTERNAL      = 4,    /* Unknown internal error */
    RSP_ERROR_E06_FILE          = 5,    /* File I/O error */
    RSP_ERROR_E07_APP           = 6     /* vRun is not allowed. A static application is running. */

} RSP_ERROR_TYPE;

#endif /* RSP_DEFS_H */



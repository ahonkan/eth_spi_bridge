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
*       dbg_rsp.h
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP)
*
*   DESCRIPTION
*
*       This file contains the rsp control block definitions and function
*       macros for the RSP Support Component.
*
*   DATA STRUCTURES
*
*       RSP_CB
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_RSP_H
#define DBG_RSP_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* Thread Queue Size - The size (in thread IDs) of the thread queue.  The
   default value is 512. */

#define RSP_THREAD_QUEUE_SIZE               512

/* Thread Timer Stack Size - The size (in bytes) of the thread timer stack
   used for notification resends.  The default value is
   (8 * NU_MIN_STACK_SIZE). */

#define RSP_THREAD_TIMER_STACK_SIZE         (8 * NU_MIN_STACK_SIZE)

/* Notification Packet Initial Timeout - The initial timeout for stop
   events.  The default value is:

   (NU_PLUS_TICKS_PER_SEC/100)

   This is a 10ms timeout. */

#define RSP_NOTIFY_PACKET_INIT_TIMEOUT      (NU_PLUS_TICKS_PER_SEC/100)

/* Notification Packet Resend Timeout - The resend timeout for stop
   events.  The default value is:

   (NU_PLUS_TICKS_PER_SEC * 2)

   This is a 2s timeout. */

#define RSP_NOTIFY_PACKET_RESEND_TIMEOUT    (NU_PLUS_TICKS_PER_SEC * 2)

/* vRun Thread Adjusted Priority - This is the priority that the thread
   executing the vRun callback will be set to during the callback.  The
   default value is 0. */

#define RSP_VRUN_THREAD_ADJUSTED_PRIORITY   0

/* RSP packet management control block structure definition */

typedef struct  _rsp_cb_struct
{
    UINT                dbg_session_id;         /* Debug Session ID */
    CHAR *              p_out_rsp_pkt_buff;     /* Pointer to output packet buffer */
    CHAR *              p_work_rsp_pkt_buff;    /* Pointer to working packet buffer */
    UINT                pkt_size;               /* Packet size tracking variable used primarily for
                                                   re-transmission. */
    CHAR *              last_pkt_buff;          /* Pointer to last transmitted buffer */
    UINT                last_pkt_size;          /* Size of last transmitted buffer */
    DBG_THREAD_ID       first_next_tid;         /* RSP previously returned first next TID */
    RSP_THREAD_QUEUE    thread_queue;           /* Queue for stopped threads */
    RSP_TIMER           thread_timer;           /* Timer for stopped threads */
    UINT                thread_seq_cnt;         /* Thread count after a '?' packet */
    UINT                dle_app_cnt;            /* DLE applications running count */
    DBG_THREAD_ID       notify_tid;             /* notification thread id */
    BOOLEAN             is_first_qsubseq;       /* Flag to track first pass in QSubsequent thread handler */
    RSP_MODE            mode;                   /* Current debugging mode */
    DBG_THREAD_ID       wrk_tid_exec;           /* Working thread for execution commands */
    DBG_THREAD_ID       wrk_tid_step;           /* Working thread for step execution commands */
    DBG_THREAD_ID       wrk_tid_other;          /* Working thread for other commands */
    DBG_THREAD_ID       current_tid;            /* Working thread for other commands */
    RSP_NOTIFY_SIGNAL   notify_sig_type;        /* Notification timeout signal type */
    VOID *              app_offset;             /* Offset of application image. */
    BOOLEAN             q_start_no_ack_mode;    /* Tracks no ack mode. */
    CHAR                rd_data[RSP_MAX_PACKET_SIZE_SUPPORTED];     /* vFile read data buffer. */
    BOOLEAN             static_app_mode;        /* Tracks if a statically linked application is running. */


} RSP_CB;

#ifdef __cplusplus
}
#endif

#endif /* DBG_RSP_H */

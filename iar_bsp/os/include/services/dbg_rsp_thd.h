/*************************************************************************
*
*            Copyright 2009 Mentor Graphics Corporation
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
*       dbg_rsp_thd.h
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP) - Thread Queue
*
*   DESCRIPTION
*
*       This file contains function prototypes for the RSP thread queue
*       component functions.
*
*   DATA STRUCTURES
*
*       RSP_THREAD_QUEUE
*       RSP_THREAD_QUEUE_INFO
*
*   FUNCTIONS
*
*       RSP_Thread_Queue_Initialize
*       RSP_Thread_Queue_Terminate
*       RSP_Thread_Queue_Send
*       RSP_Thread_Queue_Send_All
*       RSP_Thread_Queue_Send_All_Stopped
*       RSP_Thread_Queue_Receive
*       RSP_Thread_Queue_Reset
*       RSP_Thread_Queue_Info
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_RSP_THD_H
#define DBG_RSP_THD_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* Thread Queue */

typedef struct _rsp_thread_queue_struct
{
    UINT                msg_size;           /* The size of a queue message. */
    VOID *              p_memory;           /* Queue memory. */
    NU_QUEUE            queue;              /* Nucleus queue. */
    
} RSP_THREAD_QUEUE;

/* Thread Queue Information */

typedef struct _rsp_thread_queue_info_struct
{
    UINT                count;              /* Number of threads in the queue. */         
    
} RSP_THREAD_QUEUE_INFO;

/* Thread Queue Message Max - The maximum size (in UNSIGNED) that a thread
   queue message may be. */
   
#define RSP_THREAD_QUEUE_MSG_MAX            ((sizeof(DBG_THREAD_ID) / sizeof(UNSIGNED)) + 1)

/***** Global functions */

RSP_STATUS  RSP_Thread_Queue_Initialize(RSP_THREAD_QUEUE *  p_thread_queue,
                                        UINT                max_threads);

RSP_STATUS  RSP_Thread_Queue_Terminate(RSP_THREAD_QUEUE *   p_thread_queue);

RSP_STATUS  RSP_Thread_Queue_Send(RSP_THREAD_QUEUE *    p_thread_queue,
                                  DBG_THREAD_ID         thread_id);

RSP_STATUS  RSP_Thread_Queue_Send_All(RSP_THREAD_QUEUE *    p_thread_queue,
                                      DBG_SESSION_ID        session_id);

RSP_STATUS  RSP_Thread_Queue_Send_All_Stopped(RSP_THREAD_QUEUE *    p_thread_queue,
                                              DBG_SESSION_ID        session_id);

RSP_STATUS  RSP_Thread_Queue_Receive(RSP_THREAD_QUEUE *    p_thread_queue,
                                     DBG_THREAD_ID *       p_thread_id);

RSP_STATUS  RSP_Thread_Queue_Reset(RSP_THREAD_QUEUE *   p_thread_queue);

RSP_STATUS  RSP_Thread_Queue_Info(RSP_THREAD_QUEUE *        p_thread_queue,
                                  RSP_THREAD_QUEUE_INFO *   p_thread_queue_info);

#ifdef __cplusplus
}
#endif

#endif /* DBG_RSP_THD_H */

/*************************************************************************
*
*               Copyright 2009 Mentor Graphics Corporation
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
*       dbg_rsp_thd.c
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP) - Thread Queue
*
*   DESCRIPTION
*
*       This file contains the utility functions for the RSP Support
*       Component.
*
*   DATA STRUCTURES
*
*       None
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
*       dbg.h
*
*************************************************************************/

#include    "services/dbg.h"

/***** Global functions */

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Initialize
*
*   DESCRIPTION
*
*      This function initializes an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*       max_threads - Indicates the maximum number of threads that the 
*                     queue may contain.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to allocate memory for thread queue. OR
*                           unable to initialize thread queue.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Initialize(RSP_THREAD_QUEUE *  p_thread_queue,
                                        UINT                max_threads)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;
        
    /* Calculate the size (in UNSIGNED queue elements) needed to store a
       thread ID value (as a queue message). */
       
    p_thread_queue -> msg_size = (sizeof(DBG_THREAD_ID) / sizeof(UNSIGNED));
    if ((sizeof(DBG_THREAD_ID) % sizeof(UNSIGNED) > 0))
    {
        p_thread_queue -> msg_size++;
        
    }
        
    /* Allocate memory for thread queue. */
    
    p_thread_queue -> p_memory = DBG_System_Memory_Allocate(max_threads * p_thread_queue -> msg_size * sizeof(UNSIGNED),
                                                            sizeof(UNSIGNED),
                                                            DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                            DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                            NU_NO_SUSPEND);    
        
    if (p_thread_queue -> p_memory == NU_NULL)
    {   
        /* ERROR: Unable to allocate memory for thread queue. */
        
        rsp_status = RSP_STATUS_FAILED;
        
    }
        
    if (rsp_status == RSP_STATUS_OK)
    {           
        /* Initialize the thread queue. */
        
        nu_status = NU_Create_Queue(&p_thread_queue -> queue,
                                    "DBG_RTQ",
                                    p_thread_queue -> p_memory,
                                    max_threads,
                                    NU_FIXED_SIZE,
                                    p_thread_queue -> msg_size,
                                    NU_FIFO);

        if (nu_status != NU_SUCCESS)
        {   
            /* ERROR: Unable to initialize thread queue. */
            
            rsp_status = RSP_STATUS_FAILED;
          
            /* ERROR RECOVERY: Attempt to clean up any allocated 
               memory. */
               
            if (p_thread_queue -> p_memory == NU_NULL)
            {
                (VOID)DBG_System_Memory_Deallocate(p_thread_queue -> p_memory);
               
            }          
          
        }

    }
                                    
    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Terminate
*
*   DESCRIPTION
*
*      This function terminates an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to delete thread queue. OR unable to 
*                           deallocate thread queue memory.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Terminate(RSP_THREAD_QUEUE *   p_thread_queue)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
    DBG_STATUS      dbg_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;
    dbg_status = DBG_STATUS_OK;
        
    nu_status = NU_Delete_Queue (&p_thread_queue -> queue);
        
    if (nu_status != NU_SUCCESS)
    {   
        /* ERROR: Unable to delete thread queue. */
        
        rsp_status = RSP_STATUS_FAILED;
        
    }
        
    if (rsp_status == RSP_STATUS_OK)
    {           
        /* Deallocate thread queue memory (if needed). */

        if (p_thread_queue -> p_memory != NU_NULL)
        {
            dbg_status = DBG_System_Memory_Deallocate(p_thread_queue -> p_memory);
           
            if (dbg_status != DBG_STATUS_OK)
            {
                /* ERROR: Unable to deallocate thread queue memory. */
                
                rsp_status = RSP_STATUS_FAILED;
                
            }
           
        }          

    }
                                    
    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Send
*
*   DESCRIPTION
*
*      This function sends a thread ID to an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*       thread_id - The thread ID.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to add thread ID to queue.
*
*       RSP_STATUS_RESOURCE_NOT_AVAIL - Queue is full.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Send(RSP_THREAD_QUEUE *    p_thread_queue,
                                  DBG_THREAD_ID         thread_id)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;

    /* Add thread ID to queue. */

    nu_status = NU_Send_To_Queue(&p_thread_queue -> queue,
                                 (VOID *)&thread_id,
                                 p_thread_queue -> msg_size,
                                 NU_NO_SUSPEND);
        
    switch (nu_status)
    {
        case NU_SUCCESS :
        {
            /* Hooray! */
            
            break;
            
        }
        
        case NU_QUEUE_FULL :
        {
            /* Indicate queue is full. */
            
            rsp_status = RSP_STATUS_RESOURCE_NOT_AVAIL;
            
            break;
            
        }
        
        default :
        {
            /* ERROR: Unable to add thread ID to queue. */
            
            rsp_status = RSP_STATUS_FAILED;
      
            break;
            
        }
      
    }
                                    
    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Send_All
*
*   DESCRIPTION
*
*      This function sends all application threads within a debug session
*      to an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*       DBG_SESSION_ID - Debug session ID.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to retrieve session threads.  OR unable
*                           to add session threads to queue.
*
*       RSP_STATUS_RESOURCE_NOT_AVAIL - Queue is full.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Send_All(RSP_THREAD_QUEUE *    p_thread_queue,
                                      DBG_SESSION_ID        session_id)
{
    RSP_STATUS      rsp_status;
    DBG_STATUS      dbg_status;
    DBG_CMD         dbg_cmd;
    DBG_THREAD_ID   thread_id;
    
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    dbg_status = DBG_STATUS_OK;
    
    /* Attempt to get the first thread ID from session. */
    
    dbg_cmd.op = DBG_CMD_OP_THREAD_GET_FIRST;
    dbg_cmd.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

    dbg_status = DBG_ENG_Command(session_id,
                                 &dbg_cmd);    

    dbg_cmd.op = DBG_CMD_OP_THREAD_GET_NEXT;

    while ((rsp_status == RSP_STATUS_OK) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Add thread ID to queue. */
        
        rsp_status = RSP_Thread_Queue_Send(p_thread_queue,
                                           thread_id);
        
        if (rsp_status == RSP_STATUS_OK)
        {
            /* Attempt to get another thread ID from session. */
            
            dbg_status = DBG_ENG_Command(session_id,
                                         &dbg_cmd);
                                         
        }
        
    }
      
    if (rsp_status == RSP_STATUS_OK)
    {
        /* Intercept and handle any 'un-expected' status values. */
        
        if ((dbg_status != DBG_STATUS_OK) &&
            (dbg_status != DBG_STATUS_RESOURCE_UNAVAILABLE))
        {
            /* ERROR: Failed to retrieve session threads. */
            
            rsp_status = RSP_STATUS_FAILED;
            
        }

    }

    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Send_All_Stopped
*
*   DESCRIPTION
*
*      This function sends all application threads that are in a stopped
*      state within a debug session to an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*       DBG_SESSION_ID - Debug session ID.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to retrieve session threads.  OR unable
*                           to add session threads to queue.
*
*       RSP_STATUS_RESOURCE_NOT_AVAIL - Queue is full.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Send_All_Stopped(RSP_THREAD_QUEUE *    p_thread_queue,
                                              DBG_SESSION_ID        session_id)
{
    RSP_STATUS          rsp_status;
    DBG_STATUS          dbg_status;
    DBG_CMD             dbg_cmd_fst_nxt;
    DBG_CMD             dbg_cmd_info;
    DBG_THREAD_ID       thread_id;
    DBG_THREAD_STATUS   thread_status;
    
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    dbg_status = DBG_STATUS_OK;
    
    /* Attempt to get the first thread ID from session. */
    
    dbg_cmd_fst_nxt.op = DBG_CMD_OP_THREAD_GET_FIRST;
    dbg_cmd_fst_nxt.op_param.thd_get_fst_nxt.p_thread_id = &thread_id;

    dbg_status = DBG_ENG_Command(session_id,
                                 &dbg_cmd_fst_nxt);    

    dbg_cmd_fst_nxt.op = DBG_CMD_OP_THREAD_GET_NEXT;

    while ((rsp_status == RSP_STATUS_OK) &&
           (dbg_status == DBG_STATUS_OK))
    {
        /* Get information about the thread. */

        dbg_cmd_info.op = DBG_CMD_OP_THREAD_INFO;
        dbg_cmd_info.op_param.thd_info.thread_id = thread_id;
        dbg_cmd_info.op_param.thd_info.p_thread_status = &thread_status;
    
        dbg_status = DBG_ENG_Command(session_id,
                                     &dbg_cmd_info);        
        
        if (dbg_status == DBG_STATUS_OK)
        {
            if (thread_status == DBG_THREAD_STATUS_STOPPED)
            {
                /* Add thread ID to queue. */
                
                rsp_status = RSP_Thread_Queue_Send(p_thread_queue,
                                                   thread_id);            
             
            }
            
        }
        
        if ((rsp_status == RSP_STATUS_OK) &&
            (dbg_status == DBG_STATUS_OK))
        {
            /* Attempt to get another thread ID from session. */
            
            dbg_status = DBG_ENG_Command(session_id,
                                         &dbg_cmd_fst_nxt);
                                         
        }
        
    }
      
    if (rsp_status == RSP_STATUS_OK)
    {
        /* Intercept and handle any 'un-expected' status values. */
        
        if ((dbg_status != DBG_STATUS_OK) &&
            (dbg_status != DBG_STATUS_RESOURCE_UNAVAILABLE))
        {
            /* ERROR: Failed to retrieve session threads. */
            
            rsp_status = RSP_STATUS_FAILED;
            
        }

    }

    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Receive
*
*   DESCRIPTION
*
*      This function receives a thread ID from an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*       p_thread_id - Return parameter that will be updated to contain the
*                     dequeued thread ID value if the operation succeeds.
*                     If the operation fails the value is undefined.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to remove thread ID from queue (error).
*
*       RSP_STATUS_RESOURCE_NOT_AVAIL - Queue is empty.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Receive(RSP_THREAD_QUEUE *    p_thread_queue,
                                     DBG_THREAD_ID *       p_thread_id)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
    UNSIGNED        queue_data[RSP_THREAD_QUEUE_MSG_MAX];
    UNSIGNED        actual_size;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;

    /* Remove thread ID from queue. */

    nu_status = NU_Receive_From_Queue(&p_thread_queue -> queue,
                                      (VOID *)&queue_data[0],
                                      p_thread_queue -> msg_size,
                                      &actual_size,
                                      NU_NO_SUSPEND);
        
    switch (nu_status)
    {
        case NU_SUCCESS :
        {
            if (actual_size != p_thread_queue -> msg_size)
            {
                /* ERROR: Unable to remove thread ID from queue. */
        
                rsp_status = RSP_STATUS_FAILED;
        
            }
            
            break;
            
        }
        
        case NU_QUEUE_EMPTY :
        {
            /* Indicate no thread IDs in queue. */
            
            rsp_status = RSP_STATUS_RESOURCE_NOT_AVAIL;
            
            break;
            
        }
        
        default :
        {
            /* ERROR: Internal error. */
            
            rsp_status = RSP_STATUS_FAILED;
            
            break;
            
        }
        
    }
                 
    if (rsp_status == RSP_STATUS_OK)
    {
        /* Update thread ID value using queue data. */
        
        *p_thread_id = *((DBG_THREAD_ID *)&queue_data[0]);
        
    }            
                 
    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Reset
*
*   DESCRIPTION
*
*      This function resets an RSP thread queue, clearing all elements.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to reset the queue.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Reset(RSP_THREAD_QUEUE *   p_thread_queue)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;

    /* Remove thread ID from queue. */

    nu_status = NU_Reset_Queue(&p_thread_queue -> queue);
        
    if (nu_status != NU_SUCCESS)
    {
        /* ERROR: Unable reset the queue. */
        
        rsp_status = RSP_STATUS_FAILED;
        
    }
                                    
    return (rsp_status);
}

/*************************************************************************
*   FUNCTION
*
*      RSP_Thread_Queue_Info
*
*   DESCRIPTION
*
*      This function retrieves information about an RSP thread queue.
*
*   INPUTS
*
*       p_thread_queue - Pointer to the queue.
*
*       p_thread_queue_info - Pointer to a thread queue information 
*                             structure that will be updated to contain
*                             information about the queue if the operation
*                             succeeds.  If the operation fails the value
*                             is undefined.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to reset the queue.
*
*************************************************************************/
RSP_STATUS  RSP_Thread_Queue_Info(RSP_THREAD_QUEUE *        p_thread_queue,
                                  RSP_THREAD_QUEUE_INFO *   p_thread_queue_info)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
    CHAR            queue_name[8];
    VOID *          queue_start_address;
    UNSIGNED        queue_size;
    UNSIGNED        queue_available;
    UNSIGNED        queue_messages;
    OPTION          queue_message_type;
    UNSIGNED        queue_message_size;
    OPTION          queue_suspend_type;
    UNSIGNED        queue_tasks_waiting;
    NU_TASK *       queue_first_task;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;

    /* Retrieve information about the queue. */

    nu_status = NU_Queue_Information(&p_thread_queue -> queue,
                                     &queue_name[0],
                                     &queue_start_address,
                                     &queue_size,
                                     &queue_available,
                                     &queue_messages,
                                     &queue_message_type,
                                     &queue_message_size,
                                     &queue_suspend_type,
                                     &queue_tasks_waiting,
                                     &queue_first_task);
        
    if (nu_status != NU_SUCCESS)
    {
        /* ERROR: Unable retrieve information about the queue. */
        
        rsp_status = RSP_STATUS_FAILED;
        
    }
             
    if (rsp_status == RSP_STATUS_OK)
    {
        /* Update information structure. */
        
        p_thread_queue_info -> count = (UINT)queue_messages;
    }
             
    return (rsp_status);
}

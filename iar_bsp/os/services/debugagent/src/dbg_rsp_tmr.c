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
*       dbg_rsp_tmr.c
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP) - Timer
*
*   DESCRIPTION
*
*       This file contains the utility functions for the RSP Support
*       Component.  RSP timers differ from Nucleus timers in that they
*       involve a Nucleus thread, with the timer expiration occurring in
*       a thread context.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       dbg_rsp_tmr_thread_entry
*
*       RSP_Timer_Initialize
*       RSP_Timer_Terminate
*       RSP_Timer_Start
*       RSP_Timer_Stop
*
*   DEPENDENCIES
*
*       dbg.h
*
*************************************************************************/

#include    "services/dbg.h"

/***** Local functions */

/* Local function declarations */

static VOID dbg_rsp_tmr_thread_entry(UNSIGNED     argc,
                                     VOID *       argv);

/* Local function definitions */

/*************************************************************************
*
*   FUNCTION
*
*       dbg_rsp_tmr_thread_entry
*
*   DESCRIPTION
*
*       This is the entry function for an RSP timer thread.
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
static VOID dbg_rsp_tmr_thread_entry(UNSIGNED     argc,
                                     VOID *       argv)
{
    STATUS          nu_status;
    RSP_TIMER *     p_timer;
    
    /* Get timer control block from parameters. */
    
    p_timer = (RSP_TIMER *)argv;

    /* Release timer access semaphore to activate API. */
       
    nu_status = NU_Release_Semaphore(&p_timer -> acc_sem);
            
    if (nu_status == NU_SUCCESS)
    {
        /* Main timer loop. */
    
        while (p_timer -> is_valid == NU_TRUE)
        {
            /* Obtain the control semaphore, waiting expiration time first. */
            
            nu_status = NU_Obtain_Semaphore(&p_timer -> ctrl_sem,
                                            p_timer -> exp_time);
                                       
            switch (nu_status)
            {
                case NU_SUCCESS :
                {
                    /* Semaphore released by API (breaks out of semaphore
                       timer expiration wait and affects new timer values). */
                    
                    break;   
                }
                
                case NU_TIMEOUT :
                {
                    /* Timer expiration occurred. */
                    
                    p_timer -> exp_func(p_timer -> exp_func_ctxt);
             
                    break;
                }
             
                case NU_SEMAPHORE_DELETED :
                default :
                {
                    /* Error occurred obtaining control semaphore or control 
                       semaphore deleted (timer terminating). */
                    
                    p_timer -> is_valid = NU_FALSE;                
                    
                    break;
                }
             
            }
            
        }
    }
    else
    {
        /* ERROR: Unable to release timer access semaphore. */
     
        p_timer -> is_valid = NU_FALSE;
        
    }

    return;
}

/***** Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Timer_Initialize
*
*   DESCRIPTION
*
*       This function initializes an RSP timer.
*
*   INPUTS
*
*       p_timer - Pointer to the timer.
*
*       exp_func - Function to be called for each expiration of the timer.
*
*       exp_func_ctxt - The context value passed to the expiration
*                       function.
*
*       exp_func_stk_size - The size of the stack (in bytes) that the
*                           expiration function should be called from.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to create access semaphore.  OR Unable
*                           to create control semaphore.  OR unable to 
*                           allocate memory for thread stack.
*
*************************************************************************/
RSP_STATUS  RSP_Timer_Initialize(RSP_TIMER *        p_timer,
                                 RSP_TIMER_FUNC     exp_func,
                                 VOID *             exp_func_ctxt,
                                 UNSIGNED           exp_func_stk_size)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;

    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;
       
    /* Initialize timer control block.  Note that initial timeout is
       'suspend'. */
    
    p_timer -> is_valid = NU_TRUE;
    p_timer -> exp_func = exp_func;
    p_timer -> exp_func_ctxt = exp_func_ctxt;
    p_timer -> exp_time = NU_SUSPEND;
   
    /* Create timer access semaphore (as initially not available). */
    
    nu_status = NU_Create_Semaphore(&p_timer -> acc_sem,
                                    "DBG_RTA",
                                    0,
                                    NU_PRIORITY);
                                    
    if (nu_status != NU_SUCCESS)
    {   
        /* ERROR: Unable to create access semaphore. */
        
        rsp_status = RSP_STATUS_FAILED;
        
    }   
   
    /* Create timer control semaphore (as initially not available). */
    
    nu_status = NU_Create_Semaphore(&p_timer -> ctrl_sem,
                                    "DBG_RTC",
                                    0,
                                    NU_PRIORITY);
                                    
    if (nu_status != NU_SUCCESS)
    {   
        /* ERROR: Unable to create control semaphore. */
        
        rsp_status = RSP_STATUS_FAILED;
        
    }
      
    if (rsp_status == RSP_STATUS_OK)
    {
        /* Allocate memory for thread stack. */
        
        p_timer -> p_thd_stk = DBG_System_Memory_Allocate(exp_func_stk_size,
                                                          DBG_SYSTEM_MEMORY_ALLOC_NO_ALIGN,
                                                          DBG_SYSTEM_MEMORY_ALLOC_CACHED,
                                                          DBG_SYSTEM_MEMORY_ALLOC_ZERO_INIT,
                                                          NU_NO_SUSPEND);    
        
        if (p_timer -> p_thd_stk == NU_NULL)
        {   
            /* ERROR: Unable to allocate memory for thread stack. */
            
            rsp_status = RSP_STATUS_FAILED;
            
        }
      
    }
      
    if (rsp_status == RSP_STATUS_OK)
    {           
        /* Create timer thread. */
        
        nu_status = NU_Create_Task(&p_timer -> thd,
                                   "DBG_RTM",
                                   dbg_rsp_tmr_thread_entry, 
                                   1, 
                                   (VOID *)p_timer, 
                                   p_timer -> p_thd_stk,
                                   exp_func_stk_size,
                                   RSP_TIMER_THREAD_PRIORITY,
                                   RSP_TIMER_THREAD_TIMESLICE,
                                   NU_NO_PREEMPT,
                                   NU_NO_START);

        if (nu_status == NU_SUCCESS)
        {
            /* Bind task to kernel module. */

            nu_status = NU_BIND_TASK_TO_KERNEL(&p_timer -> thd);
        }

        if (nu_status == NU_SUCCESS)
        {
            /* Start task. */

            nu_status = NU_Resume_Task(&p_timer -> thd);
        }

        if (nu_status != NU_SUCCESS)
        {   
            /* ERROR: Unable to create thread. */
            
            rsp_status = RSP_STATUS_FAILED;
          
            /* ERROR RECOVERY: Attempt to clean up any allocated 
               memory. */
               
            if (p_timer -> p_thd_stk == NU_NULL)
            {
                (VOID)DBG_System_Memory_Deallocate(p_timer -> p_thd_stk);
               
            }          
          
        }

    }

    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Timer_Terminate
*
*   DESCRIPTION
*
*       This function terminates an RSP timer.
*
*       NOTE: This function will 'block' waiting on access to the timer.
*
*   INPUTS
*
*       p_timer - Pointer to the timer.
*
*       
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to obtain timer access semaphore.  OR
*                           Unable to delete timer control semaphore.  OR
*                           Unable to delete timer access semaphore.
*
*************************************************************************/
RSP_STATUS  RSP_Timer_Terminate(RSP_TIMER *         p_timer)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;
       
    /* Obtain access semaphore. */
    
    nu_status = NU_Obtain_Semaphore(&p_timer -> acc_sem,
                                    NU_SUSPEND);
    
    if (nu_status != NU_SUCCESS)
    {
        /* ERROR: Unable to obtain timer access semaphore. */
     
        rsp_status = RSP_STATUS_FAILED;
        
    }    
     
    if (rsp_status == RSP_STATUS_OK)
    {     
        /* Delete control semaphore to indicate timer terminating. */
           
        nu_status = NU_Delete_Semaphore(&p_timer -> ctrl_sem);
            
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete timer control semaphore. */
         
            rsp_status = RSP_STATUS_FAILED;
            
        }
    
    }
    
    if (rsp_status == RSP_STATUS_OK)
    {
        /* Wait for timer thread to complete. */

        nu_status = NU_Delete_Task(&p_timer -> thd);

        while (nu_status != NU_SUCCESS)
        {
            NU_Sleep(RSP_TIMER_TERM_POLL_INTERVAL);
            nu_status = NU_Delete_Task(&p_timer -> thd);
        
        }       
        
        /* Deallocate thread stack memory. */
        
        if (p_timer -> p_thd_stk == NU_NULL)
        {
            (VOID)DBG_System_Memory_Deallocate(p_timer -> p_thd_stk);
           
        }        
        
    }
     
    if (rsp_status == RSP_STATUS_OK)
    {     
        /* Delete access semaphore. */
           
        nu_status = NU_Delete_Semaphore(&p_timer -> acc_sem);
            
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to delete timer access semaphore. */
         
            rsp_status = RSP_STATUS_FAILED;
            
        }
    
    }     
     
    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Timer_Start
*
*   DESCRIPTION
*
*       This function starts an RSP timer running with the specified
*       expiration time.
*
*       NOTE: This function will 'block' waiting on access to the timer.
*
*   INPUTS
*
*       p_timer - Pointer to the timer.
*
*       exp_time - Amount of time (in Nucleus system ticks) for each timer
*                  expiration.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Unable to obtain timer access semaphore.  OR
*                           Timer is invalid.  OR unable to release timer
*                           control semaphore.  OR Unable to release timer
*                           access semaphore.
*
*************************************************************************/
RSP_STATUS  RSP_Timer_Start(RSP_TIMER *             p_timer,
                            UNSIGNED                exp_time)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;
       
    /* Obtain access semaphore. */
    
    nu_status = NU_Obtain_Semaphore(&p_timer -> acc_sem,
                                    NU_SUSPEND);
    
    if (nu_status == NU_SUCCESS)
    {
        /* Ensure timer is valid. */
    
        if (p_timer -> is_valid == NU_FALSE)
        {
            /* ERROR: Timer is invalid. */
            
            rsp_status = RSP_STATUS_FAILED;
            
        }
        
        if (rsp_status == RSP_STATUS_OK)
        {       
            /* Setup expiration time to be new value.  Not that a value
               of zero is invalid and will be translated to a minimum
               time of one. */
            
            if (exp_time > 0)
            {
                p_timer -> exp_time = exp_time;
    
            }
            else
            {
                p_timer -> exp_time = 1;
                
            }
    
            /* Release semaphore to affect changes (and start timer). */
               
            nu_status = NU_Release_Semaphore(&p_timer -> ctrl_sem);
                
            if (nu_status != NU_SUCCESS)
            {
                /* ERROR: Unable to release timer control semaphore. */
             
                rsp_status = RSP_STATUS_FAILED;
                
            }
          
        }
          
        /* Release timer access semaphore. */
           
        nu_status = NU_Release_Semaphore(&p_timer -> acc_sem);
            
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to release timer access semaphore. */
         
            rsp_status = RSP_STATUS_FAILED;
            
        }
          
    }
    else
    {
        /* ERROR: Unable to obtain timer access semaphore. */
     
        rsp_status = RSP_STATUS_FAILED;
        
    }             
     
    return (rsp_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       RSP_Timer_Stop
*
*   DESCRIPTION
*
*       This function stops an RSP timer.
*
*       NOTE: This function will 'block' waiting on access to the timer.
*
*   INPUTS
*
*       p_timer - Pointer to the timer.
*
*   OUTPUTS
*
*       RSP_STATUS_OK - Indicates successful operation.
*
*       RSP_STATUS_FAILED - Timer is invalid.  OR unable to release timer
*                           control semaphore.  OR unable to obtain timer
*                           access semaphore.
*
*************************************************************************/
RSP_STATUS  RSP_Timer_Stop(RSP_TIMER *              p_timer)
{
    RSP_STATUS      rsp_status;
    STATUS          nu_status;
        
    /* Set initial function status. */
    
    rsp_status = RSP_STATUS_OK;
    nu_status = NU_SUCCESS;
       
    /* Obtain access semaphore. */
    
    nu_status = NU_Obtain_Semaphore(&p_timer -> acc_sem,
                                    NU_SUSPEND);
    
    if (nu_status == NU_SUCCESS)
    {       
        /* Ensure timer is valid. */
        
        if (p_timer -> is_valid == NU_FALSE)
        {
            /* ERROR: Timer is invalid. */
            
            rsp_status = RSP_STATUS_FAILED;
            
        }
        
        if (rsp_status == RSP_STATUS_OK)
        {
            /* Setup expiration time to be 'suspend'. */
            
            p_timer -> exp_time = NU_SUSPEND;
               
            /* Release control semaphore to affect changes (and stop timer). */
               
            nu_status = NU_Release_Semaphore(&p_timer -> ctrl_sem);
                
            if (nu_status != NU_SUCCESS)
            {
                /* ERROR: Unable to release timer semaphore. */
             
                rsp_status = RSP_STATUS_FAILED;
                
            }
        
        }
    
        /* Release timer access semaphore. */
           
        nu_status = NU_Release_Semaphore(&p_timer -> acc_sem);
            
        if (nu_status != NU_SUCCESS)
        {
            /* ERROR: Unable to release timer access semaphore. */
         
            rsp_status = RSP_STATUS_FAILED;
            
        }
          
    }
    else
    {
        /* ERROR: Unable to obtain timer access semaphore. */
     
        rsp_status = RSP_STATUS_FAILED;
        
    }     
    
    return (rsp_status);
}

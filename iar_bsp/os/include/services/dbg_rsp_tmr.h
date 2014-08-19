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
*       dbg_rsp_tmr.h
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP) - Timer
*
*   DESCRIPTION
*
*       This file contains function prototypes for the RSP timer
*       component functions.  An RSP timer involves a thread, with the
*       timer expiration function occuring in a thread context.
*
*   DATA STRUCTURES
*
*       RSP_TIMER
*       RSP_TIMER_INFO
*
*   FUNCTIONS
*
*       RSP_Timer_Initialize
*       RSP_Timer_Terminate
*       RSP_Timer_Start
*       RSP_Timer_Stop
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_RSP_TMR_H
#define DBG_RSP_TMR_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* Timer Thread Behavior  - The following defines determine the behavior
   of the timer thread.  The default values are:
   
    #define RSP_TIMER_THREAD_PRIORITY                   6
    #define RSP_TIMER_THREAD_TIMESLICE                  0  
   
   */
       
#define RSP_TIMER_THREAD_PRIORITY                   6
#define RSP_TIMER_THREAD_TIMESLICE                  0

/* Timer Terminate Polling Interval - The interval (in Nucleus system 
   ticks) that the terminate function polls waiting to perform cleanup.  
   The default value is 100. */
   
#define RSP_TIMER_TERM_POLL_INTERVAL                100

/*************************************************************************
*
*   FUNCTION
*
*      RSP_TIMER_FUNC
*
*   DESCRIPTION
*
*      This function prototype defines the signature for an RSP timer
*      expiration function.
*
*   INPUTS
*
*       context - Context value set when the timer is initialized.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
typedef VOID (*RSP_TIMER_FUNC)(VOID *       context);

/* Timer */

typedef struct _rsp_timer_struct
{
    NU_SEMAPHORE        acc_sem;            /* Access semaphore. */    
    NU_SEMAPHORE        ctrl_sem;           /* Control semaphore. */
    UNSIGNED            exp_time;           /* Expiration timeout. */
    VOID *              p_thd_stk;          /* Thread stack. */
    NU_TASK             thd;                /* Thread control block. */
    RSP_TIMER_FUNC      exp_func;           /* Expiration function. */
    VOID *              exp_func_ctxt;      /* Function context. */
    BOOLEAN             is_valid;           /* Indicates if timer is valid. */ 
 
} RSP_TIMER;

/* Timer Information */

typedef struct _rsp_timer_info_struct
{
    UINT                count;              /* Number of threads in the queue. */         
    
} RSP_TIMER_INFO;

/***** Global functions */

RSP_STATUS  RSP_Timer_Initialize(RSP_TIMER *        p_timer,
                                 RSP_TIMER_FUNC     exp_func,
                                 VOID *             exp_func_ctxt,
                                 UNSIGNED           exp_func_stk_size);

RSP_STATUS  RSP_Timer_Terminate(RSP_TIMER *         p_timer);

RSP_STATUS  RSP_Timer_Start(RSP_TIMER *             p_timer,
                            UNSIGNED                exp_time);

RSP_STATUS  RSP_Timer_Stop(RSP_TIMER *              p_timer);

#ifdef __cplusplus
}
#endif

#endif /* DBG_RSP_TMR_H */

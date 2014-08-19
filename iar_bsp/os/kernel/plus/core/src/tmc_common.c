/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       tmc_common.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains the core routines for the timer management
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TMC_Init_Task_Timer                 Initialize task timer
*       TMC_Start_Timer                     Actually start a timer
*       TMC_Stop_Timer                      Actually stop a timer
*       TMC_Timer_HISR                      Timer High-Level Interrupt
*                                           Service Routine (HISR)
*       TMC_Timer_Expiration                Timer expiration function
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       timer.h                             Timer functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/timer.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "services/nu_trace_os_mark.h"

/* Define external inner-component global data references.  */

extern TM_TCB               *TMD_Active_Timers_List;
extern INT                  TMD_Active_List_Busy;
extern UNSIGNED             TMD_Timer_Start;

/* Define internal function prototypes.  */

VOID            TMC_Timer_Expiration(VOID);

/***********************************************************************
*
*   FUNCTION
*
*       TMC_Init_Task_Timer
*
*   DESCRIPTION
*
*       This function is responsible for initializing the supplied task
*       timer.  This routine must be called from Supervisor mode in a
*       Supervisor/User mode switching kernel.
*
*   CALLED BY
*
*       NU_Create_Task                      Task create function
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       timer                               Timer control block pointer
*       information                         Information pointer - always
*                                           the task pointer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMC_Init_Task_Timer(TM_TCB *timer, VOID *information)
{

    /* Initialize the task timer.  */
    timer -> tm_timer_type =      TM_TASK_TIMER;
    timer -> tm_information =     information;
    timer -> tm_next_timer =      NU_NULL;
    timer -> tm_previous_timer =  NU_NULL;
}


/***********************************************************************
*
*   FUNCTION
*
*       TMC_Start_Timer
*
*   DESCRIPTION
*
*       This function is responsible for starting both application and
*       task timers.  This routine must be called from Supervisor mode
*       in a Supervisor/User mode switching kernel.
*
*   CALLED BY
*
*       NU_Control_Timer                    Control timer operation
*       TMC_Timer_Expiration                Process timer expirations
*
*   CALLS
*
*       TCC_Suspend_Task                    Suspends a task
*       TMC_Timer_Expiration                Responsible for processing
*                                           all task timer expirations
*       TMCT_Adjust_Timer                   Adjust the count-down timer
*       TMCT_Enable_Timer                   Enable count-down timer
*       TMCT_Read_Timer                     Read current timer counter
*
*   INPUTS
*
*       timer                               Timer control block pointer
*       time                                Time associated with timer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMC_Start_Timer(TM_TCB *timer, UNSIGNED time)
{
    R1 TM_TCB       *list_ptr;              /* Working pointer timer ptr */
    UNSIGNED        elapsed;                /* Elapsed time variable     */
    INT             done;                   /* Search finished flag      */


    /* Note that protection over the active timer list is in force when this
       function is called.  */

    /* Determine if the active list is empty.  */
    if (TMD_Active_Timers_List == NU_NULL)
    {

        /* Place the timer on an empty list.  */
        timer -> tm_next_timer =      timer;
        timer -> tm_previous_timer =  timer;

        /* Link the timer to the list head.  */
        TMD_Active_Timers_List =  timer;

        /* Setup the actual count-down timer structures.  */
        TMD_Timer_Start =  time;
        timer -> tm_remaining_time =  time;

        /* Determine if there is any time remaining on the timer.
           If so, enable the timer. Otherwise, the Timer HISR is
           already pending, so skip starting the timer again.  */
        if (time != 0)
        {
           /* Start the actual count-down timer.  */
           TMCT_Enable_Timer(TMD_Timer_Start);
        }
        else
        {
            /* Indicate that the timer is expired */
            TMD_Timer_State =  TM_EXPIRED;
        }

    }
    else
    {

        /* Place the new timer into the list.  */

        /* Pickup the head of the list.  */
        list_ptr =  TMD_Active_Timers_List;

        /* Determine if the timer is being added while the timer
           expiration task is running.  If so, don't attempt to adjust
           the expiration list.  If not, adjust the list.  */
        if (!TMD_Active_List_Busy)
        {

            /* Calculate the elapsed amount of time from the last timer
               request.  */
            elapsed =  TMD_Timer_Start -  TMCT_Read_Timer();

            /* Adjust the first entry in the timer list and the timer
               start value accordingly.  */
            TMD_Timer_Start =  TMD_Timer_Start - elapsed;

            /* Make sure the remaining time is never below zero.  */
            if (list_ptr -> tm_remaining_time > elapsed)
            {
                list_ptr -> tm_remaining_time = list_ptr -> tm_remaining_time
                    - elapsed;
            }
            else
            {
                list_ptr -> tm_remaining_time = 0;
            }


        }

        /* At this point the timer list is accurate again.  Find the
           appropriate place on the timer list for the new timer.  */

        /* Determine where to place the timer in the list.  */
        done =  NU_FALSE;
        do
        {

            /* Determine if the timer belongs before the current timer
               pointed to by list_ptr.  */
            if (time < list_ptr -> tm_remaining_time)
            {

                /* Update the time of the next timer.  */
                list_ptr -> tm_remaining_time =
                                list_ptr -> tm_remaining_time - time;

                /* Determine if an insertion at the head of the list is
                   present.  */
                if (list_ptr == TMD_Active_Timers_List)
                {
                    /* Move the list head to the new timer.  */
                    TMD_Active_Timers_List =  timer;
                }

                /* Set the done flag to end the search.  */
                done =  NU_TRUE;
            }
            else
            {

                /* Decrement the time by the remaining value of each timer in
                   the list.  In this way, the list never has to be searched
                   again.  */
                time =  time - list_ptr -> tm_remaining_time;

                /* Move the list pointer to the next timer in the list.  */
                list_ptr =  list_ptr -> tm_next_timer;

                /* Check to see if the list has wrapped around.  */
                if (list_ptr == TMD_Active_Timers_List)
                {
                    /* Searching is done.  */
                    done =  NU_TRUE;
                }
            }
        } while (!done);

        /* Link the new timer into the list.  */
        timer -> tm_next_timer =      list_ptr;
        timer -> tm_previous_timer =  list_ptr -> tm_previous_timer;
        (list_ptr -> tm_previous_timer) -> tm_next_timer =  timer;
        list_ptr -> tm_previous_timer =  timer;

        /* Update the remaining time parameter.  */
        timer -> tm_remaining_time =  time;

        /* Determine if a new timer should be started.  */
        if (!TMD_Active_List_Busy)
        {

            /* Calculate the new timer expiration.  */
            time =  TMD_Active_Timers_List -> tm_remaining_time;

            /* Determine if the new expiration is less than the current
               time, if any.  If not, let already started time expire.  */
            if (time <= TMD_Timer_Start)
            {

                /* Setup for a smaller timer expiration.  */
                TMD_Timer_Start =  time;

                /* Determine if there is any time remaining on the timer in
                   the front of the list.  If so, adjust the timer.
                   Otherwise, the Timer HISR is already pending, so skip
                   starting the timer again.  */
                if (TMD_Timer_Start)
                {
                    /* Still some remaining time, adjust the timer.  */
                    TMCT_Adjust_Timer(TMD_Timer_Start);
                }
                else
                {
                    /* Indicate that the task and application timer has
                       expired. */
                    TMD_Timer_State =  TM_EXPIRED;
                }
            }
        }
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       TMC_Stop_Timer
*
*   DESCRIPTION
*
*       This function is responsible for stopping both application and
*       task timers.  This routine must be called from Supervisor mode
*       in a Supervisor/User mode switching kernel.
*
*   CALLED BY
*
*       TCC_Resume_Task                     Resumes a task
*       NU_Terminate_Task                   Terminates a task
*       TMC_Timer_Expiration                Process timer expirations
*       NU_Control_Timer                    Control application timer
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       timer                               Timer control block pointer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMC_Stop_Timer(TM_TCB *timer)
{

    /* Note that the active timer list is already under protection.  */

    /* If the next neighbor of the timer that needs to be stopped is not the
       head of the timer list, add the remaining time field to the remaining
       time of the next neighbor.  */
    if ((timer -> tm_next_timer) != TMD_Active_Timers_List)
    {
        /* Adjust the next neighbor's remaining time field.  */
        (timer -> tm_next_timer) -> tm_remaining_time =
                   (timer -> tm_next_timer) -> tm_remaining_time +
                                                timer -> tm_remaining_time;
    }

    /* Unlink the timer from the active list.  */
    if (timer -> tm_next_timer == timer)
    {
        /* Only timer on the list.  */
        TMD_Active_Timers_List =  NU_NULL;

        /* Disable the count-down timer */
        TMD_Timer_State = TM_NOT_ACTIVE;
    }
    else
    {

        /* More than one timer on the list.  */
        (timer -> tm_previous_timer) -> tm_next_timer = timer -> tm_next_timer;
        (timer -> tm_next_timer) -> tm_previous_timer =
                                                timer -> tm_previous_timer;

        /* Determine if the timer is at the head of the list.  */
        if (TMD_Active_Timers_List == timer)
        {
            /* Yes, move the head pointer to the next timer.  */
            TMD_Active_Timers_List =  timer -> tm_next_timer;
        }
    }

    /* Clear the timer's next and previous pointers.  */
    timer -> tm_next_timer =      NU_NULL;
    timer -> tm_previous_timer =  NU_NULL;
}


/***********************************************************************
*
*   FUNCTION
*
*       TMC_Timer_HISR
*
*   DESCRIPTION
*
*       This function is responsible for High-Level interrupt processing
*       of a timer expiration.  If an application timer has expired,
*       the timer expiration function is called.  Otherwise, if the
*       time-slice timer has expired, time-slice processing is invoked.
*
*   CALLED BY
*
*       [TCCE_Create_HISR]                  Performs error checking
*
*   CALLS
*
*       TCC_Time_Slice                      Task time-slice processing
*       TMC_Timer_Expiration                Timer expiration processing
*       TMCT_Retrieve_TS_Task               Retrieve time-sliced task
*                                           ptr
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMC_Timer_HISR(VOID)
{
    NU_TASK     *task;                      /* Time slice task.  */


    /* Determine if the task timer has expired.  */
    if (TMD_Timer_State == TM_EXPIRED)
    {
        /* Resume the timer task.  */
        TMC_Timer_Expiration();
    }

    /* Get current time-slice task pointer */
    task =  (NU_TASK *)TMCT_Retrieve_TS_Task();

    /* Determine if the time-slice timer has expired.  */
    if (task)
    {
        NU_SUPERV_USER_VARIABLES

        /* Switch to supervisor mode

           Note that this HISR function can make the switch to supervisor mode
           this is only possible because the code lives within the kernel */
        NU_SUPERVISOR_MODE();

        /* Process the time-slice.  */
        TCC_Time_Slice(task);

        /* Clear the time slice task pointer.  */
        TMD_Time_Slice_Task =  NU_NULL;

        /* Return to user mode */
        NU_USER_MODE();
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       TMC_Timer_Expiration
*
*   DESCRIPTION
*
*       This function is responsible for processing all task timer
*       expirations.  This includes application timers and basic task
*       timers that are used for task sleeping and timeouts.
*
*   CALLED BY
*
*       TMC_Timer_HISR                      Responsible for High-Level interrupt
*                                           processing of a timer expiration
*
*
*   CALLS
*
*       expiration_function                 Application specified timer
*                                           expiration function
*       TCC_Task_Timeout                    Task timeout function
*       TCCT_Schedule_Lock                  Protect active timer list
*       TCCT_Schedule_Unlock                Release protection of list
*       TMC_Start_Timer                     Start timer
*       TMC_Stop_Timer                      Stop timer
*       TMCT_Enable_Timer                   Enable timer
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID  TMC_Timer_Expiration(VOID)
{
    R1 TM_TCB       *timer;                 /* Pointer to timer         */
    R2 TM_APP_TCB   *app_timer;             /* Pointer to app timer     */
    INT             done;                   /* Expiration completion    */
    INT             type = 0;               /* Type of expiration       */
    VOID            *pointer = NU_NULL;     /* Pointer type             */
    UNSIGNED        id = 0;                 /* Application timer ID     */
                                            /* Expiration routine ptr   */
    VOID            (*expiration_routine)(UNSIGNED) = NU_NULL;
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Use system protect to protect the active timer list.  */
    TCCT_Schedule_Lock();

    /* Reset the timer state flag.  */
    TMD_Timer_State = TM_NOT_ACTIVE;

    /* Set the busy flag to indicate that the list is being processed.  */
    TMD_Active_List_Busy =  NU_TRUE;

    /* Update the head of the list with the timer expiration
       value.  */
    timer =  TMD_Active_Timers_List;
    if (timer)
    {

        /* Adjust the active timer's remaining time value.  Note that
           TMD_Timer_Start is never greater than the value in the first
           timer location.  */
        if (timer -> tm_remaining_time > TMD_Timer_Start)
        {
            /* Timer has not expired.  Simply subtract the last timer
               value. */
            timer -> tm_remaining_time = timer -> tm_remaining_time -
                                                    TMD_Timer_Start;
        }
        else
        {
            /* Clear the remaining time field of the timer.  */
            timer -> tm_remaining_time =  0;
        }
    }

    /* Release protection, but keep the busy flag set to prevent
       activating new timers.  */
    TCCT_Schedule_Unlock();

    /* Find expired timers.  Note that the expired timers have values of
       0 in the remaining time field.  */
    done =  NU_FALSE;
    do
    {
        /* Protect against list access.  */
        TCCT_Schedule_Lock();

        /* Pickup the head of the active list.  */
        timer =  TMD_Active_Timers_List;

        /* Determine if the timer now at the head of the list has
           expired.  Processing continues until the list is empty or
           until a non-expired timer is at the front of the list. */
        if ((timer) && (timer -> tm_remaining_time == 0))
        {

            /* Timer has expired.  Determine which type of timer has
               expired.  */
            if (timer -> tm_timer_type == TM_APPL_TIMER)
            {

                /* Application timer has expired.  */
                type =  TM_APPL_TIMER;

                /* Pickup the pointer to the application timer control
                   block.  */
                app_timer =  (TM_APP_TCB *) timer -> tm_information;

                /* Increment the number of expirations.  */
                app_timer -> tm_expirations++;

                /* Move the expiration information into local variables
                   in case they get corrupted before this expiration can
                   be processed.  Expirations are processed without the
                   list protection in force.  */
                id =                  app_timer -> tm_expiration_id;
                expiration_routine =  app_timer -> tm_expiration_routine;

                /* Clear the enabled flag and remove the timer from the
                   list.  */
                app_timer -> tm_enabled =  NU_FALSE;
                TMC_Stop_Timer(timer);

                /* Determine if this timer should be started again.  */
                if (app_timer -> tm_reschedule_time)
                {

                    /* Timer needs to be rescheduled.  */

                    /* Setup the enable flag to show that the timer is
                       enabled.  */
                    app_timer -> tm_enabled =  NU_TRUE;

                    /* Call the start timer function to actually enable
                       the timer.  This also puts it in the proper place
                       on the list.  */
                    TMC_Start_Timer(timer,app_timer -> tm_reschedule_time);
                }
            }
            else
            {

                /* Task timer has expired (sleeps and timeouts).  */
                type =  TM_TASK_TIMER;

                /* Remove the timer from the list.  */
                TMC_Stop_Timer(timer);

                /* Save-off the task control block pointer.  */
                pointer =  timer -> tm_information;
            }
        }
        else
        {
            /* Processing is now complete- no more expired timers on the
               list.  */
            done =  NU_TRUE;
        }

        /* Release protection of active list.  */
        TCCT_Schedule_Unlock();

        /* Determine if a timer expiration needs to be finished.  Note
           that the actual expiration processing is done with protection
           disabled.  This prevents deadlock situations from arising.  */
        if (!done)
        {

            /* Determine which type of timer has expired.  */
            if (type == TM_APPL_TIMER)
            {
                /* Trace log */
                T_TIMER_EXP_ROUTIN_RUNNING((VOID*)timer, id);

                /* Call application timer's expiration function.  */
                (*(expiration_routine)) (id);

                /* Trace log */
                T_TIMER_EXP_ROUTIN_STOPPED((VOID*)timer, id);
            }
            else
            {
                /* Call the task timeout function in the thread control
                   function.  */
                TCC_Task_Timeout((NU_TASK *) pointer);

            }
        }
    } while (!done);

    /* Protect the active list again.  */
    TCCT_Schedule_Lock();

    /* Clear the busy flag to indicate that list processing is complete. */
    TMD_Active_List_Busy =  NU_FALSE;

    /* Determine if a new timer should be enabled.  */
    if (TMD_Active_Timers_List)
    {

        /* Yes, a new timer should be activated.  */

        /* Pickup the new timer expiration value.  */
        TMD_Timer_Start =  TMD_Active_Timers_List -> tm_remaining_time;

        /* Start the new timer.  */
        TMCT_Enable_Timer(TMD_Timer_Start);
    }

    /* Release protection of the active timer list.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();
}

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
*       tmd.c
*
*   COMPONENT
*
*       TM - Timer Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       timer management component.
*
*   DATA STRUCTURES
*
*       TMD_Created_Timers_List             Pointer to the linked-list
*                                           of created application
*                                           timers
*       TMD_Total_Timers                    Total number of created
*                                           application timers
*       TMD_Active_Timers_List              Pointer to the linked-list
*                                           of active timers.
*       TMD_Active_List_Busy                Flag indicating that the
*                                           active timer list is in use
*       TMD_System_Clock                    System clock
*       TMD_System_Clock_Upper              System clock overflow count
*       TMD_Timer_Start                     Starting value of timer
*       TMD_Timer                           Timer count-down value
*       TMD_Timer_State                     State of timer
*       TMD_Time_Slice_Task                 Pointer to task to
*                                           time-slice
*       TMD_HISR                            Timer HISR control block
*       TMD_HISR_Stack                      Timer HISR stack area
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       timer.h                             Timer functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/timer.h"

/* TMD_Created_Timers_List is the head pointer of the linked list of
   created application timers.  If the list is NU_NULL, there are no timers
   currently created.  */

CS_NODE             *TMD_Created_Timers_List;


/* TMD_Total_Timers contains the total number of created application timers
   in the system.  */

UNSIGNED            TMD_Total_Timers;


/* TMD_Active_Timers_List is the head pointer of the linked list of active
   timers.  This includes both the application timers and the system timers
   used for task sleeping and timeouts.  If the list is NU_NULL, there are
   no timers currently active.  */

TM_TCB              *TMD_Active_Timers_List;


/* TMD_Active_List_Busy is a flag that indicates that the active timer list
   is being processed.  This is used to prevent multiple updates to the
   active timer list.  */

INT                 TMD_Active_List_Busy;

/* TMD_System_Clock is a continually incrementing clock.  One is added to
   the clock each timer interrupt.  */

volatile UNSIGNED   TMD_System_Clock;

/* TMD_System_Clock_Upper contains the number of overflows of the main 32
   bit value TMD_System_Clock.  One is added to the upper value on each
   overflow.  */

volatile UNSIGNED   TMD_System_Clock_Upper;

/* TMD_System_Clock_Offset contains the 64-bit offset clock value. */

volatile UINT64     TMD_System_Clock_Offset;

/* TMD_Timer_Start represents the starting value of the last set timer
   request.  */

UNSIGNED            TMD_Timer_Start;


/* TMD_Timer is a count-down timer that is used to represent the smallest
   active timer value in the system.  Once this counter goes to zero, a
   timer has expired.  */

volatile UNSIGNED   TMD_Timer;


/* TMD_Timer_State indicates the state of the timer variable.  If the state
   is active, the timer counter is decremented.  If the state is expired,
   the timer HISR and timer task are initiated to process the expiration.  If
   the state indicates that the timer is not-active, the timer counter is
   ignored.  */

volatile INT        TMD_Timer_State = TM_NOT_ACTIVE;


/* TMD_Time_Slice_Task is a pointer to the task to time-slice.  This pointer
   is built in the portion of the timer interrupt that determines if a time-
   slice timer has expired.  */

TC_TCB             * volatile TMD_Time_Slice_Task;


/* TMD_HISR is the timer HISR's control block.  */

TC_HCB              TMD_HISR;


/* TMD_HISR_Stack is the stack space used by the Nucleus Timer HISR.  */

UINT8               TMD_HISR_Stack[NU_TIMER_HISR_STACK_SIZE];

/* TMD_Last_Time_Stamp tracks the last time stamp value returned to the application */

UINT64              TMD_Last_Time_Stamp;

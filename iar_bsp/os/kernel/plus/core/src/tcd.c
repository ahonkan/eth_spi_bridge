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
*       tcd.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains global data structures for use within this
*       component.
*
*   DATA STRUCTURES
*
*       TCD_Created_Tasks_List              Pointer to the linked-list
*                                           of created tasks
*       TCD_Total_Tasks                     Total number of created
*                                           tasks
*       TCD_Priority_List                   Array of pointers to ready
*                                           tasks, indexed by priority
*       TCD_Execute_Task                    Highest priority task to
*                                           execute
*       TCD_Priority_Groups                 Bit map of 32 groups of task
*                                           priority
*       TCD_Sub_Priority_Groups             An array of 32 sub-priority
*                                           groups
*       TCD_Lowest_Set_Bit                  Lookup table to find the
*                                           lowest bit set in a byte
*       TCD_Highest_Priority                Highest priority ready
*       TCD_Created_HISRs_List              Pointer to the linked-list
*                                           of created HISRs
*       TCD_Total_HISRs                     Total number of created
*                                           HISRs
*       TCD_Active_HISR_Heads               Active HISR list head ptrs
*       TCD_Active_HISR_Tails               Active HISR list tail ptrs
*       TCD_Execute_HISR                    Highest priority HISR to
*                                           execute
*       TCD_Current_Thread                  Pointer to the currently
*                                           executing thread
*       TCD_Current_App_Task                Pointer to the control block 
*                                           of the current or last 
*                                           scheduled application task
*       TCD_Schedule_Lock                   Data structure that is used
*                                           to provide critical section
*                                           protection between threads
*       TCD_Interrupt_Level                 Enable interrupt level
*       TCD_Unhandled_Interrupt             Contains the most recent
*                                           unhandled interrupt in
*                                           system error conditions
*       TCD_Protect_Save                    Saved state of NU_Protect
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/* TCD_Created_Tasks_List is the head pointer of the linked list of
   created tasks.  If the list is NU_NULL, there are no tasks created.  */

CS_NODE             *TCD_Created_Tasks_List;

/* TCD_Total_Tasks contains the number of currently created tasks.  */

UNSIGNED            TCD_Total_Tasks;

/* TCD_App_Task_List is the head pointer of the linked list of created
   application tasks.  If the list is NU_NULL, there are no
   application tasks. */

TC_TCB              *TCD_App_Task_List;

/* TCD_Total_App_Tasks contains the number of currently created
   application tasks. */

UNSIGNED            TCD_Total_App_Tasks;

/* TCD_Priority_List is an array of TCB pointers.  Each element of the array
   is effectively the head pointer of the list of tasks ready for execution
   at that priority.  If the pointer is NULL, there are no tasks ready
   for execution at that priority.  The array is indexed by priority.
   NOTE:  The array is one greater than the number of priorities to ensure
          the last element of the array is NULL.  This simplifies the algorithm
          used to retrieve the highest priority ready task */

TC_TCB              *TCD_Priority_List[TC_PRIORITIES+1];

/* TCD_Priority_Groups is a 32-bit unsigned integer that is used as a bit
   map.  Each bit corresponds to an 8-priority group.  For example, if bit 0
   is set, at least one task of priority 0 through 8 is ready for execution. */

UNSIGNED            TCD_Priority_Groups;

/* TCD_Sub_Priority_Groups is an array of sub-priority groups.  These are
   also used as bit maps.  Index 0 of this array corresponds to priorities
   0 through 8.  Bit 0 of this element represents priority 0, while bit 7
   represents priority 7.  */

DATA_ELEMENT        TCD_Sub_Priority_Groups[TC_MAX_GROUPS];

/* TCD_Lowest_Set_Bit is nothing more than a standard lookup table.  The
   table is indexed by values ranging from 1 to 255.  The value at that
   position in the table indicates the number of the lowest set bit.  This is
   used to determine the highest priority task represented in the previously
   defined bit maps.  */

#if (NU_MIN_RAM_ENABLED == NU_FALSE)

UNSIGNED_CHAR       TCD_Lowest_Set_Bit[] =

#else

const UNSIGNED_CHAR TCD_Lowest_Set_Bit[] =

#endif
                            {0,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,
                             2,  0,  1,  0,  4,  0,  1,  0,  2,  0,  1,  0,
                             3,  0,  1,  0,  2,  0,  1,  0,  5,  0,  1,  0,
                             2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
                             4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,
                             2,  0,  1,  0,  6,  0,  1,  0,  2,  0,  1,  0,
                             3,  0,  1,  0,  2,  0,  1,  0,  4,  0,  1,  0,
                             2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
                             5,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,
                             2,  0,  1,  0,  4,  0,  1,  0,  2,  0,  1,  0,
                             3,  0,  1,  0,  2,  0,  1,  0,  7,  0,  1,  0,
                             2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
                             4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,
                             2,  0,  1,  0,  5,  0,  1,  0,  2,  0,  1,  0,
                             3,  0,  1,  0,  2,  0,  1,  0,  4,  0,  1,  0,
                             2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
                             6,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,
                             2,  0,  1,  0,  4,  0,  1,  0,  2,  0,  1,  0,
                             3,  0,  1,  0,  2,  0,  1,  0,  5,  0,  1,  0,
                             2,  0,  1,  0,  3,  0,  1,  0,  2,  0,  1,  0,
                             4,  0,  1,  0,  2,  0,  1,  0,  3,  0,  1,  0,
                             2,  0,  1,  0, 78, 85, 67, 32, 80, 76, 85, 83};

/* TCD_Highest_Priority contains the highest priority task ready for execution.
   Note that this does not necessarily represent the priority of the currently
   executing task.  This is true if the currently executing task has preemption
   disabled.  If no tasks are executing, this variable is set to the maximum
   priority.  */

INT                 TCD_Highest_Priority = TC_PRIORITIES;

/* TCD_Highest_Priority_HISR contains the highest priority hisr ready for execution. */

volatile INT        TCD_Highest_Priority_HISR = TC_HISR_PRIORITIES;

/* TCD_Execute_Task is a pointer to the task to execute.  Note that this
   pointer does not necessarily point to the currently executing task.  There
   are several points in the system where this is true.  One situation is
   when preemption is about to take place.  Another situation can result from
   a internal protection conflict.  */

TC_TCB * volatile   TCD_Execute_Task;


/* TCD_Created_HISRs_List is the head pointer of the list of created High-
   Level Interrupt Service Routines (HISR).  If this pointer is NU_NULL, there
   are no HISRs currently created.  */

CS_NODE             *TCD_Created_HISRs_List;


/* TCD_Total_HISRs contains the number of currently created HISRs.  */

UNSIGNED            TCD_Total_HISRs;


/* TCD_Active_HISR_Heads is an array of active HISR list head pointers.
   There are three HISR priorities available.  The HISR priority is an index
   into this table.  Priority/index 0 represents the highest priority.
   NOTE:  The array is one greater than the number of priorities to ensure
          the last element of the array is NULL.  This simplifies the algorithm
          used to retrieve the highest priority ready HISR. */

TC_HCB              *TCD_Active_HISR_Heads[TC_HISR_PRIORITIES+1];


/* TCD_Active_HISR_Tails is an array of active HISR list tail pointers.
   There are three HISR priorities available.  The HISR priority is an index
   into this table.  Priority/index 0 represents the highest priority.  */

TC_HCB              *TCD_Active_HISR_Tails[TC_HISR_PRIORITIES];


/* TCD_Execute_HISR contains a pointer to the highest priority HISR to execute.
   If this pointer is NU_NULL, no HISRs are currently activated.  Note that
   the current thread pointer is not always equal to this pointer.  */

TC_HCB * volatile   TCD_Execute_HISR;


/* TCD_Current_Thread points to the control block of the currently executing
   thread of execution.  Therefore, this variable points at either a TC_TCB
   or a TC_HCB structure.  Except for initialization, this variable is set
   and cleared in the target dependent portion of this component.  */

VOID * volatile     TCD_Current_Thread;


/* TCD_Current_App_Task points to the control block of the current or last
   scheduled application task. */

TC_TCB * volatile   TCD_Current_App_Task;

/* TCD_Schedule_Lock is a variable that is used to provide critical section
   protection between threads. */

BOOLEAN             TCD_Schedule_Lock;

/* TCD_Interrupt_Level is a variable that contains the enabled interrupt
   level.  If the target processor does not have multiple enable interrupt
   levels, this variable is a boolean.  */

INT                 TCD_Interrupt_Level = (INT)NU_DISABLE_INTERRUPTS;


/* TCD_Unhandled_Interrupt is a variable that contains the last unhandled
   interrupt in system error conditions.  */

INT                 TCD_Unhandled_Interrupt;

/* TCD_Unhandled_Exception is a variable that contains the last unhandled
   exception in system error conditions.  */

INT                 TCD_Unhandled_Exception;

/* TCD_Unhandled_Exception_SP is a variable that contains the stack pointer
   for the last exception in system error conditions.  */

VOID                *TCD_Unhandled_Exception_SP;

/* Used to save the state of interrupts when NU_Protect is called, and restore
   the saved state in NU_Unprotect */
INT                 TCD_Protect_Save;

/***********************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       tcs_group.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains functions that manage task os states.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCS_Task_Group_ID                Retrieve task group id
*       TCS_Change_Group_ID              Change task group id
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                           Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/*************************************************************************
*
*   FUNCTION
*
*      TCS_Task_Group_ID
*
*   DESCRIPTION
*
*      A function that retrieves the group id of the selected task.
*
*   INPUTS
*
*      task_ptr - pointer to the task from which to retrieve the id
*
*   OUTPUTS
*
*      UNSIGNED - id of the selected task
*
*************************************************************************/
UNSIGNED TCS_Task_Group_ID(NU_TASK *task_ptr)
{
    return task_ptr->tc_grp_id;
}

/*************************************************************************
*
*   FUNCTION
*
*      TCS_Change_Group_ID
*
*   DESCRIPTION
*
*      A function that changes the group id of a task
*
*   INPUTS
*
*      task_ptr - task that is to be changed
*
*   OUTPUTS
*
*      UNSIGNED - previous id of the selected task
*
*************************************************************************/
UNSIGNED TCS_Change_Group_ID(NU_TASK *task_ptr, UNSIGNED group_id)
{
    TC_TCB  *task;
    UNSIGNED old_group;

    task = (TC_TCB *)task_ptr;
    old_group = task->tc_grp_id;

    if(old_group != group_id)
    {
        /* Protect the list of application tasks.  */
        TCCT_Schedule_Lock();

        task->tc_grp_id = group_id;

        if(group_id == TC_GRP_ID_APP)
        {
            /* Add this task to the application list */
            TCC_Application_Task_Add(task);
        }
        else
        {
            /* Remove this task from the application list */
            TCC_Application_Task_Remove(task);
        }

        /* Release the list protection */
        TCCT_Schedule_Unlock();
    }

    return old_group;
}

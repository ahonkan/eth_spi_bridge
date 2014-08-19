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
*       tcf_pointers.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains information (fact) Pointer routines for the
*       Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Task_Pointers                    Build list of task pointers
*       NU_HISR_Pointers                    Build list of HISR pointers
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

/* Define external inner-component global data references.  */

extern CS_NODE              *TCD_Created_Tasks_List;
extern CS_NODE              *TCD_Created_HISRs_List;
extern TC_TCB               *TCD_App_Task_List;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Task_Pointers
*
*   DESCRIPTION
*
*       This function builds a list of task pointers, starting at the
*       specified location.  The number of task pointers placed in the
*       list is equivalent to the total number of tasks or the maximum
*       number of pointers specified in the call.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect task created list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       pointer_list                        Pointer to the list area
*       maximum_pointers                    Maximum number of pointers
*
*   OUTPUTS
*
*       pointers                            Number of tasks placed in
*                                           list
*
***********************************************************************/
UNSIGNED NU_Task_Pointers(NU_TASK **pointer_list, UNSIGNED maximum_pointers)
{
    CS_NODE          *node_ptr;             /* Pointer to each TCB       */
    UNSIGNED         pointers;              /* Number of pointers in list*/
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Initialize the number of pointers returned.  */
    pointers =  0;

    /* Protect the task created list.  */
    TCCT_Schedule_Lock();

    /* Loop until all task pointers are in the list or until the maximum
       list size is reached.  */
    node_ptr =  TCD_Created_Tasks_List;
    while ((node_ptr) && (pointers < maximum_pointers))
    {

        /* Place the node into the destination list.  */
        *pointer_list++ =  (NU_TASK *) node_ptr;

        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        node_ptr =  node_ptr -> cs_next;

        /* Determine if the pointer is at the head of the list.  */
        if (node_ptr == TCD_Created_Tasks_List)
        {
            /* The list search is complete.  */
            node_ptr =  NU_NULL;
        }
    }

    /* Release protection.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the number of pointers in the list.  */
    return(pointers);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_HISR_Pointers
*
*   DESCRIPTION
*
*       This function builds a list of HISR pointers, starting at the
*       specified location.  The number of HISR pointers placed in the
*       list is equivalent to the total number of HISRs or the maximum
*       number of pointers specified in the call.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect HISR created list
*       TCCT_Schedule_Unlock                Release protection of list
*
*   INPUTS
*
*       pointer_list                        Pointer to the list area
*       maximum_pointers                    Maximum number of pointers
*
*   OUTPUTS
*
*       pointers                            Number of HISRs placed in
*                                           list
*
***********************************************************************/
UNSIGNED NU_HISR_Pointers(NU_HISR **pointer_list, UNSIGNED maximum_pointers)
{
    CS_NODE          *node_ptr;             /* Pointer to each TCB       */
    UNSIGNED         pointers;              /* Number of pointers in list*/
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Initialize the number of pointers returned.  */
    pointers =  0;

    /* Protect the HISR created list.  */
    TCCT_Schedule_Lock();

    /* Loop until all HISR pointers are in the list or until the maximum
       list size is reached.  */
    node_ptr =  TCD_Created_HISRs_List;
    while ((node_ptr) && (pointers < maximum_pointers))
    {

        /* Place the node into the destination list.  */
        *pointer_list++ =  (NU_HISR *) node_ptr;

        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        node_ptr =  node_ptr -> cs_next;

        /* Determine if the pointer is at the head of the list.  */
        if (node_ptr == TCD_Created_HISRs_List)
        {
            /* The list search is complete.  */
            node_ptr =  NU_NULL;
        }
    }

    /* Release protection.  */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the number of pointers in the list.  */
    return(pointers);
}

/***********************************************************************
*
*   FUNCTION
*
*       TCF_Application_Task_Pointers
*
*   DESCRIPTION
*
*       This function builds a list of application task pointers,
*       starting at the specified location.  The number of task pointers
*       placed in the list is equivalent to the total number of tasks or
*       the maximum number of pointers specified in the call.
*
*   INPUTS
*
*       pointer_list                        Pointer to the list area
*       maximum_pointers                    Maximum number of pointers
*
*   OUTPUTS
*
*       pointers                            Number of tasks placed in
*                                           list
*
***********************************************************************/
UNSIGNED  TCF_Application_Task_Pointers(NU_TASK **pointer_list, UNSIGNED maximum_pointers)
{
    TC_TCB          *app_ptr;               /* Pointer to each TCB       */
    UNSIGNED         pointers;              /* Number of pointers in list*/

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Initialize the number of pointers returned.  */
    pointers =  0;

    /* Protect the task created list.  */
    TCCT_Schedule_Lock();

    /* Loop until all task pointers are in the list or until the maximum
       list size is reached.  */
    app_ptr =  TCD_App_Task_List;
    while ((app_ptr) && (pointers < maximum_pointers))
    {

        /* Place the node into the destination list.  */
        *pointer_list++ =  (NU_TASK *) app_ptr;

        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        app_ptr = app_ptr -> tc_grp_next;

        /* Determine if the pointer is at the head of the list.  */
        if (app_ptr == TCD_App_Task_List)
        {
            /* The list search is complete.  */
            app_ptr = NU_NULL;
        }
    }

    /* Release protection.  */
    TCCT_Schedule_Unlock();

    /* Return the number of pointers in the list.  */
    return(pointers);
}


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
*       evf_pointers.c
*
*   COMPONENT
*
*       EV - Event Group Management
*
*   DESCRIPTION
*
*       This file contains routine to obtain Pointers to the Event
*       Groups.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Event_Group_Pointers             Build event group pointer
*                                           list
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

/* Define external inner-component global data references.  */

extern CS_NODE         *EVD_Created_Event_Groups_List;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Event_Group_Pointers
*
*   DESCRIPTION
*
*       This function builds a list of event group pointers, starting at
*       the specified location.  The number of event group pointers
*       placed in the list is equivalent to the total number of
*       event groups or the maximum number of pointers specified in the
*       call.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*       TCCT_Schedule_Lock                  Protect created list
*       TCCT_Schedule_Unlock                Release protection
*
*   INPUTS
*
*       pointer_list                        Pointer to the list area
*       maximum_pointers                    Maximum number of pointers
*
*   OUTPUTS
*
*       pointers                            Number of event groups
*                                           placed in the list
***********************************************************************/
UNSIGNED NU_Event_Group_Pointers(NU_EVENT_GROUP **pointer_list,
                                 UNSIGNED maximum_pointers)
{
    CS_NODE         *node_ptr;              /* Pointer to each GCB       */
    UNSIGNED        pointers;               /* Number of pointers in list*/
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Initialize the number of pointers returned.  */
    pointers =  0;

    /* Protect against access to the list of created event groups.  */
    TCCT_Schedule_Lock();

    /* Loop until all event group pointers are in the list or until
       the maximum list size is reached.  */
    node_ptr =  EVD_Created_Event_Groups_List;
    while ((node_ptr) && (pointers < maximum_pointers))
    {

        /* Place the node into the destination list.  */
        *pointer_list++ =  (NU_EVENT_GROUP *) node_ptr;

        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        node_ptr =  node_ptr -> cs_next;

        /* Determine if the pointer is at the head of the list.  */
        if (node_ptr == EVD_Created_Event_Groups_List)
        {
            /* The list search is complete.  */
            node_ptr =  NU_NULL;
        }

    }

    /* Release protection against access to the list of created
       event groups. */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the number of pointers in the list.  */
    return(pointers);
}


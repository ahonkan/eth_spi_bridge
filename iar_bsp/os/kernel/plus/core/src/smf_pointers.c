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
*       smf_pointers.c
*
*   COMPONENT
*
*       SM - Semaphore Management
*
*   DESCRIPTION
*
*       This file contains the routine to obtain Pointer facts about
*       the Semaphore Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Semaphore_Pointers               Build semaphore pointer list
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*       semaphore.h                         Semaphore functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/semaphore.h"

/* Define external inner-component global data references.  */

extern CS_NODE         *SMD_Created_Semaphores_List;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Semaphore_Pointers
*
*   DESCRIPTION
*
*       This function builds a list of semaphore pointers, starting at
*       the specified location.  The number of semaphore pointers
*       placed in the list is equivalent to the total number of
*       semaphores or the maximum number of pointers specified in the
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
*       pointers                            Number of semaphores placed
*                                           in the list
*
***********************************************************************/
UNSIGNED NU_Semaphore_Pointers(NU_SEMAPHORE **pointer_list,
                               UNSIGNED maximum_pointers)
{
    CS_NODE          *node_ptr;             /* Pointer to each SMB       */
    UNSIGNED         pointers;              /* Number of pointers in list*/
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Initialize the number of pointers returned.  */
    pointers =  0;

    /* Protect against access to the list of created semaphores.  */
    TCCT_Schedule_Lock();

    /* Loop until all semaphore pointers are in the list or until the maximum
       list size is reached.  */
    node_ptr =  SMD_Created_Semaphores_List;
    while ((node_ptr) && (pointers < maximum_pointers))
    {

        /* Place the node into the destination list.  */
        *pointer_list++ =  (NU_SEMAPHORE *) node_ptr;

        /* Increment the pointers variable.  */
        pointers++;

        /* Position the node pointer to the next node.  */
        node_ptr =  node_ptr -> cs_next;

        /* Determine if the pointer is at the head of the list.  */
        if (node_ptr == SMD_Created_Semaphores_List)
        {
            /* The list search is complete.  */
            node_ptr =  NU_NULL;
        }

    }

    /* Release protection against access to the list of created semaphores. */
    TCCT_Schedule_Unlock();

    /* Return to user mode */
    NU_USER_MODE();

    /* Return the number of pointers in the list.  */
    return(pointers);
}

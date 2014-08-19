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
*       csc_common.c
*
*   COMPONENT
*
*       CS -    Common Services
*
*   DESCRIPTION
*
*       This file contains linked list manipulation facilities used
*       throughout the Nucleus PLUS system.  These facilities operate
*       on doubly-linked circular lists.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Place_On_List                    Place node at the end of a
*                                           list
*       NU_Priority_Place_On_List           Place node in priority order
*                                           on a list
*       NU_Remove_From_List                 Remove a node from a list
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"

#if (NU_PLUS_INLINING == NU_FALSE)
/***********************************************************************
*
*   FUNCTION
*
*       NU_Place_On_List
*
*   DESCRIPTION
*
*       This function places the specified node at the end of specified
*       linked list.
*
*   CALLED BY
*
*       NU_Allocate_Memory                  Allocate a memory block from
*                                           a dynamic memory pool
*       NU_Create_Memory_Pool               Create a dynamic memory pool
*       NU_Allocate_Aligned_Memory          Allocate an aligned memory
*                                           block from  a dynamic
*                                           memory pool
*       NU_Create_Event_Group               Create an event group
*       NU_Retrieve_Events                  Retrieve events from a group
*       NU_Create_Mailbox                   Create a mailbox
*       NU_Receive_From_Mailbox             Receive a mailbox message
*       NU_Send_To_Mailbox                  Send a mailbox message
*       NU_Broadcast_To_Mailbox             Broadcast to a mailbox
*       NU_Create_Pipe                      Create a message pipe
*       NU_Receive_From_Pipe                Receive a message from pipe
*       NU_Send_To_Pipe                     Send message to a pipe
*       NU_Broadcast_To_Pipe                Broadcast a message to pipe
*       NU_Send_To_Front_Of_Pipe            Send message to pipe's front
*       NU_Allocate_Partition               Allocate a partition from a
*                                           pool
*       NU_Create_Partition_Pool            Create a Partition Pool
*       NU_Receive_From_Queue               Receive a message from queue
*       NU_Send_To_Queue                    Send message to a queue
*       NU_Broadcast_To_Queue               Broadcast a message to queue
*       NU_Send_To_Front_Of_Queue           Send message to queue's
*                                           front
*       NU_Create_Semaphore                 Create a semaphore
*       NU_Obtain_Semaphore                 Obtain instance of semaphore
*       NU_Create_HISR                      Create HISR
*       NU_Create_Task                      Create Task
*       NU_Create_Timer                     Create an application timer
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       head                                Pointer to head pointer
*       new_node                            Pointer to node to add
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
VOID NU_Place_On_List(CS_NODE **head, CS_NODE *new_node)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Determine if the list in non-empty.  */
    if (*head)
    {

        /* The list is not empty.  Add the new node to the end of
           the list.  */
        new_node -> cs_previous =               (*head) -> cs_previous;
        (new_node -> cs_previous) -> cs_next =  new_node;
        new_node -> cs_next =                   (*head);
        (new_node -> cs_next) -> cs_previous =  new_node;
    }
    else
    {

        /* The list is empty, setup the head and the new node.  */
        (*head) =  new_node;
        new_node -> cs_previous =  new_node;
        new_node -> cs_next =      new_node;
    }

    /* Return to user mode */
    NU_USER_MODE();
}

#endif  /* NU_PLUS_INLINING == NU_FALSE */

/***********************************************************************
*
*   FUNCTION
*
*       NU_Priority_Place_On_List
*
*   DESCRIPTION
*
*       This function places the specified node after all other nodes on
*       the list of equal or greater priority.  Note that lower
*       numerical values indicate greater priority.
*
*   CALLED BY
*
*       NU_Allocate_Memory                  Allocate a memory block from
*                                           a dynamic memory pool
*       NU_Allocate_Aligned_Memory          Allocate an aligned memory
*                                           block from  a dynamic
*                                           memory pool
*       NU_Receive_From_Mailbox             Receive a mailbox message
*       NU_Send_To_Mailbox                  Send a mailbox message
*       NU_Broadcast_To_Mailbox             Broadcast to a mailbox
*       NU_Receive_From_Pipe                Receive a message from pipe
*       NU_Send_To_Pipe                     Send message to a pipe
*       NU_Broadcast_To_Pipe                Broadcast a message to pipe
*       NU_Allocate_Partition               Allocate a partition from a
*                                           pool
*       NU_Receive_From_Queue               Receive a message from queue
*       NU_Send_To_Queue                    Send message to a queue
*       NU_Broadcast_To_Queue               Broadcast a message to queue
*       NU_Obtain_Semaphore                 Obtain instance of semaphore
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       head                                Pointer to head pointer
*       new_node                            Pointer to node to add
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
VOID NU_Priority_Place_On_List(CS_NODE **head, CS_NODE *new_node)
{
    CS_NODE         *search_ptr;            /* List search pointer       */
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Determine if the list in non-empty.  */
    if (*head)
    {

        /* Search the list to find the proper place for the new node.  */
        search_ptr =  (*head);

        /* Check for insertion before the first node on the list.  */
        if (search_ptr -> cs_priority > new_node -> cs_priority)
        {

            /* Update the head pointer to point at the new node.  */
            (*head) =  new_node;
        }
        else
        {

            /* We know that the new node is not the highest priority and
               must be placed somewhere after the head pointer.  */

            /* Move search pointer up to the next node since we are trying
               to find the proper node to insert in front of. */
            search_ptr =  search_ptr -> cs_next;
            while ((search_ptr -> cs_priority <= new_node -> cs_priority) &&
                   (search_ptr != (*head)))
            {

                /* Move along to the next node.  */
                search_ptr =  search_ptr -> cs_next;
            }
        }

        /* Insert before search pointer.  */
        new_node -> cs_previous =               search_ptr -> cs_previous;
        (new_node -> cs_previous) -> cs_next =  new_node;
        new_node -> cs_next =                   search_ptr;
        (new_node -> cs_next) -> cs_previous =  new_node;
    }
    else
    {

        /* The list is empty, setup the head and the new node.  */
        (*head) =  new_node;
        new_node -> cs_previous =  new_node;
        new_node -> cs_next =      new_node;
    }

    /* Return to user mode */
    NU_USER_MODE();
}

#if (NU_PLUS_INLINING == NU_FALSE)
/***********************************************************************
*
*   FUNCTION
*
*       NU_Remove_From_List
*
*   DESCRIPTION
*
*       This function removes the specified node from the specified
*       linked list.
*
*   CALLED BY
*
*       DMC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Deallocate_Memory                Deallocate a memory block
*                                           from a dynamic memory pool
*       NU_Delete_Memory_Pool               Delete a dynamic memory pool
*       EVC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Delete_Event_Group               Delete an event group
*       NU_Set_Events                       Set events in a group
*       MBC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Delete_Mailbox                   Delete a mailbox
*       NU_Receive_From_Mailbox             Receive a mailbox message
*       NU_Send_To_Mailbox                  Send a mailbox message
*       PIC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Delete_Pipe                      Delete a message pipe
*       NU_Receive_From_Pipe                Receive a message from pipe
*       NU_Send_To_Pipe                     Send message to a pipe
*       NU_Broadcast_To_Pipe                Broadcast a message to pipe
*       NU_Send_To_Front_Of_Pipe            Send message to pipe's front
*       PMC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Deallocate_Partition             Deallocate a partition from
*                                           a pool
*       NU_Delete_Partition_Pool            Delete a Partition Pool
*       QUC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Delete_Queue                     Delete a message queue
*       NU_Receive_From_Queue               Receive a message from queue
*       NU_Send_To_Queue                    Send message to a queue
*       NU_Broadcast_To_Queue               Broadcast a message to queue
*       NU_Send_To_Front_Of_Queue           Send message to queue's
*                                           front
*       SMC_Cleanup                         Cleanup on timeout or a
*                                           terminate condition
*       NU_Delete_Semaphore                 Delete a semaphore
*       NU_Release_Semaphore                Release instance of
*                                           semaphore
*       NU_Delete_HISR                      Delete HISR
*       NU_Delete_Task                      Delete a task
*       NU_Delete_Timer                     Delete an application timer
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       head                                Pointer to head pointer
*       node                                Pointer to node to add
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
VOID NU_Remove_From_List(CS_NODE **head, CS_NODE *node)
{
    NU_SUPERV_USER_VARIABLES


    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Determine if this is the only node in the system.  */
    if (node -> cs_previous == node)
    {

        /* Yes, this is the only node in the system.  Clear the node's
           pointers and the head pointer.  */
        (*head) = NU_NULL;
    }
    else
    {

        /* Unlink the node from a multiple node list.  */
        (node -> cs_previous) -> cs_next =  node -> cs_next;
        (node -> cs_next) -> cs_previous =  node -> cs_previous;

        /* Check to see if the node to delete is at the head of the
           list. */
        if (node == *head)
        {

            /* Move the head pointer to the node after.  */
            *head =  node -> cs_next;

        }
    }

    /* Return to user mode */
    NU_USER_MODE();
}

#endif  /* NU_PLUS_INLINING == NU_FALSE */

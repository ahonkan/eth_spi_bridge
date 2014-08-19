/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
*
* FILE NAME
*
*      pms_watchdog_init.c
*
* COMPONENT
*
*      PMS - Watchdog Services
*
* DESCRIPTION
*
*      This file initializes a new watchdog element.
*
* DATA STRUCTURES
*
*      None
*
* DEPENDENCIES
*
*      power_core.h
*      watchdog.h
*      <string.h>
*
***********************************************************************/
#include    <string.h>
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/watchdog.h"
#include    "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)

/* EQM Instance. */
extern EQM_EVENT_QUEUE System_Eqm;

static NU_TASK        WD_Task;
static NU_TIMER       WD_Timer;

static VOID    PMS_Watchdog_Timer (UINT32 id);
static VOID    PMS_Watchdog_Task_Entry(UNSIGNED argc, VOID *argv);

/* Head Pointer to the Active watchdog List */
static PMS_NODE *PM_Active_List_Head;

/* Count of active watchdogs */
static UINT8 PM_Active_WD_Count  = 0;

/* Array of expired watchdogs */
static PM_WD *PM_Watchdog_Expired[PM_WATCHDOG_ARRAY_SIZE];

/* Count of expired watchdogs */
static UINT8 PM_Expired_WD_Count = 0;

/* Main Task Process Event Group control block */
NU_EVENT_GROUP   PM_WD_Process_Event;

extern PM_WD *PM_Watchdog_Array[PM_WATCHDOG_ARRAY_SIZE];

/* Write index */
extern UINT32 PM_Watchdog_Array_WIndex;

/* Read Index */
extern UINT32 PM_Watchdog_Array_RIndex;

static VOID PMS_Watchdog_List_Cleanup(PM_WD *element);
static VOID PMS_Priority_Place_On_List(PMS_NODE **head, PMS_NODE *new_node);
static VOID PMS_Remove_From_List(PMS_NODE **head, PMS_NODE *node);

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Watchdog_Initialize
*
*   DESCRIPTION
*
*       This function initializes the Watchdog Component by creating
*       a main watchdog task.
*
*   INPUTS
*
*      mem_pool
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID PMS_Watchdog_Initialize(NU_MEMORY_POOL* mem_pool)
{
    STATUS       status;
    VOID        *pointer;    
    
    NU_SUPERV_USER_VARIABLES
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Allocate memory for Watchdog task */
    status = NU_Allocate_Memory(mem_pool, &pointer,
                                WD_TASK_STACK_SIZE, NU_NO_SUSPEND);

    /* Check to see if previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Create task 0.  */
        status = NU_Create_Task(&WD_Task, "WDTask", PMS_Watchdog_Task_Entry, 0, NU_NULL, pointer,
                                WD_TASK_STACK_SIZE, WD_TASK_PRIORITY, WD_TASK_TIMESLICE,
                                NU_PREEMPT, NU_START);
                                
        if (status == NU_SUCCESS)
        {    
            /* Create a timer that will wake up the main task */
            status = NU_Create_Timer(&WD_Timer, "WDTimer", PMS_Watchdog_Timer,
                                     1, 1, 0, NU_DISABLE_TIMER);
            
            if (status == NU_SUCCESS)
            {
                /* Create an event group for task */
                (VOID)NU_Create_Event_Group(&PM_WD_Process_Event, "PMWDEVT");
            }
            
            /* Initialize pointers */
            PM_Active_List_Head   = NU_NULL;                 
        }                                                 
    }
    
    /* Return to user mode */
    NU_USER_MODE();
    
    return;
}                                       


/***********************************************************************
*
*   FUNCTION
*
*       PMS_Watchdog_Task_Entry
*
*   DESCRIPTION
*
*       Entry function for WD_Task.  This task will suspend until the 
*       expiration of the lowest next_inactivity_check value
*
*   INPUTS
*
*       argc                                unsigned value set at
*                                           task creation
*       argv                                void pointer set at task
*                                           creation
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
VOID PMS_Watchdog_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS   status = NU_SUCCESS;
    PM_WD    *active_wd;
    PM_WD    *expired_wd;  
    PM_WD    *head_wd;
    PMS_NODE *next_wd_ptr;
    PMS_NODE *tail_wd_ptr;    
    UINT8    i;
    UNSIGNED event_group;
    BOOLEAN  reprocess = NU_FALSE;
    UINT32   current_timestamp;
    UINT32   timer_reset_val;
    INT      watchdog_list_index;
    UINT32   ticks_to_rollover;
    PM_WD_NOTIFICATION wd_event;

    BOOLEAN  found = NU_FALSE;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
    /* Reference all parameters to ensure no toolset warnings */
    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);
    
    for (;;)
    {

        if (reprocess != NU_TRUE)
        {
            /* Wait for an event that wakes up this main thread */
            status = NU_Retrieve_Events(&PM_WD_Process_Event, (UNSIGNED)PM_WD_WAKEUP_MAIN_TASK, (OPTION)NU_OR_CONSUME,
                                        &event_group, (UNSIGNED)NU_SUSPEND);
        }
        if ((status == NU_SUCCESS) || (reprocess == NU_TRUE))
        {
            reprocess = NU_FALSE;
            
            /************************************************** 
            Watchdog Task AND Circular Array of New WD Elements 
            ***************************************************/  
                        
            /* Ensure the array is not empty */
            while (PM_Watchdog_Array_RIndex != PM_Watchdog_Array_WIndex)
            {    
                                          
                /* First check if the watchdogs has no timeout */
                if (PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->timeout == 0)
                {
                    /* If we have reached the end of the array set Read Index to 0 */
                    if (PM_Watchdog_Array_RIndex == (PM_WATCHDOG_ARRAY_SIZE - 1))
                    {
                        PM_Watchdog_Array_RIndex = 0;
                    }
                    else
                    {
                         /* Increment Read Index */
                        PM_Watchdog_Array_RIndex++;
                    } 
                    
                    PM_Watchdog_Array[PM_Watchdog_Array_RIndex - 1] = 0;
                }                
                else 
                {
                   
                    /* Check if activity has occurred in this new element */          
                    /* If the latest_activity_check != latest_activity_timestamp activity has occurred */
                    if ((PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->latest_activity_check) !=
                         (PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->latest_activity_timestamp))
                    {
                        /* Change the value of next_inactivity_check */
                        /* Note, latest_activity timestamp is in ms, so convert timeout to ticks */
                        /* 1 timeout value = 100 ms = 10 ticks */
                        PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->next_inactivity_check = 
                        ((PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->timeout * PM_TICKS_PER_100MS) +
                         PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->latest_activity_timestamp);
                        
                        /* Since activity has occurred, move this element to the active watchdog list */
                        active_wd = PM_Watchdog_Array[PM_Watchdog_Array_RIndex];                        
                        
                        /* Get current system time in ticks */
                        current_timestamp = NU_Retrieve_Clock();
                        
                        /* Calculate rollover ticks */
                        ticks_to_rollover = PM_CLOCK_MAX - current_timestamp;
                        
                        /* If timeout in ticks is larger than remaining time before rollover occurs,
                           it means that this watchdog element's next inactivity time check value 
                           will occur after the system clock rolls over to resets to 0  */
                        if ((active_wd->timeout * PM_TICKS_PER_100MS) > ticks_to_rollover)
                        {
                            /* Set is_rollover flag for this element to TRUE */
                            active_wd->is_rollover = NU_TRUE;
                        }
                        
                        /* Update the value of latest_activity_check for this processed element */
                        active_wd->latest_activity_check = active_wd->latest_activity_timestamp;
                        
                        /* Priority of this WD List is based on the value of next_inactivity_check */
                        /* The element with the lowest next_inactivity_check will have least priority */
                        /* Save the next_inactivity_check value as the element's priority */
                        active_wd->active_list.priority = 
                                       PM_Watchdog_Array[PM_Watchdog_Array_RIndex]->next_inactivity_check;
                        
                        /* Place the active element in the list */                  
                        PMS_Priority_Place_On_List(&PM_Active_List_Head, &(active_wd->active_list));  
                        
                        /* Increment active count */
                        PM_Active_WD_Count++;   
                        
                        /* Check if notification are on and if active type is set  */
                        if (active_wd->is_notifications & PM_ACTIVE_NOTIFICATIONS)
                        {
                            /* Assign the watchdog active notification to the event type. */
                            wd_event.pm_event_type = PM_ACTIVE_NOTIFICATIONS;

                            /* Assign the sender device ID into the watchdog notification structure. */
                            wd_event.pm_dev_id = active_wd->notification_dev;
                            
                            /* Send a notification to listeners now that system state change is successful */
                            (VOID)NU_EQM_Post_Event(&System_Eqm, (EQM_EVENT*)(&wd_event),
                                                    sizeof(PM_WD_NOTIFICATION), NU_NULL);
                        }
                    }
                    
                    /* If we have reached the end of the array set Read Index to 0 */
                    if (PM_Watchdog_Array_RIndex == (PM_WATCHDOG_ARRAY_SIZE - 1))
                    {
                        PM_Watchdog_Array_RIndex = 0;
                    }
                    else
                    {
                         /* Increment Read Index */
                        PM_Watchdog_Array_RIndex++;
                    }
                }
            }
            
                    
            /*******************************
            Watchdog Task AND Active WD List 
            ********************************/ 
            if (PM_Active_WD_Count != 0)
            {                           
                /* Get a pointer to the head of the active list */ 
                next_wd_ptr = PM_Active_List_Head;
                
                /* Check every element in the active list for activity */
                do
                {
                    /* Save the tail pointer */
                    tail_wd_ptr = PM_Active_List_Head->previous;
                    
                    /* Save a pointer to the current element */
                    active_wd = (PM_WD *)next_wd_ptr;
                    
                    /* Make sure we have a pointer to the next element in the list */
                    next_wd_ptr = active_wd->active_list.next; 
                    
                    /* First check if there are any watchdogs with no timeout */
                    if (active_wd->timeout == 0)
                    {
                        /* Call the list clean-up function */
                        PMS_Watchdog_List_Cleanup(active_wd);      
                    }                    
                    else
                    {   
                        /* Get the current system timestamp in ticks */
                        current_timestamp = NU_Retrieve_Clock();

                        /* If this element has rollover flag set, check if system clock has rolled 
                           over and reset so that this flag can be cleared and the element checked 
                           expiration */
                        if (active_wd->is_rollover == NU_TRUE)
                        {
                            /* Check if next_inactivity_check is upcoming */
                            if (active_wd->next_inactivity_check > current_timestamp) 
                            {
                                /* Clock has rolled over, so clear this flag */
                                active_wd->is_rollover = NU_FALSE;                              
                            }
                        }
                                                    
                        /* Check if Watchdog has expired and if there has been no recent activity */
                        if ((active_wd->is_rollover == NU_FALSE) && 
                            (current_timestamp >= active_wd->next_inactivity_check) && 
                            (active_wd->latest_activity_check == active_wd->latest_activity_timestamp))
                        {                                                                    
                            /* Update the structure */
                            expired_wd = active_wd;                           
                                                       
                            /* Set the expired flag to TRUE */
                            expired_wd->expired_flag = NU_TRUE;
                            
                            /* Place the expired element in the expired array */                  
                            PM_Watchdog_Expired[PM_Expired_WD_Count] = expired_wd;
                            
                            if (PM_Expired_WD_Count < (PM_WATCHDOG_ARRAY_SIZE - 1))
                            {
                                /* Increment expired count */
                                PM_Expired_WD_Count++;
                            }
                            
                            /* Call the list clean-up function */
                            PMS_Watchdog_List_Cleanup(active_wd);
                            
                            /* Trace log */
                            T_WD_EXPIRE((VOID*)expired_wd);
                            
                            /* Check if notification are on and if active type is set  */
                            if (expired_wd->is_notifications & PM_INACTIVE_NOTIFICATIONS)
                            {
                                /* Assign the watchdog inactive notification to the event type. */
                                wd_event.pm_event_type = PM_INACTIVE_NOTIFICATIONS;

                                /* Assign the sender device ID into the watchdog notification structure. */
                                wd_event.pm_dev_id = expired_wd->notification_dev;
                                
                                /* Send a notification to listeners now that system state change is successful */
                                (VOID)NU_EQM_Post_Event(&System_Eqm, (EQM_EVENT*)(&wd_event),
                                                        sizeof(PM_WD_NOTIFICATION), NU_NULL);
                            }

                            /* WAKE UP ANY SUSPENDED THREADS SUSPENDED ON WATCHDOG EXPIRY */
                            /* Release the semaphore which will cause the waiting tasks to resume */
                            if (expired_wd->semaphore_ptr != NU_NULL)
                            {
                                (VOID)NU_Release_Semaphore(expired_wd->semaphore_ptr);
                            }
                         
                        }       
                        else /* WD is not expired and it is active OR it is expired but is active */
                        {
                            /* If latest_activity_check != latest_activity_timestamp activity has occurred*/
                            if ((active_wd->latest_activity_check) != (active_wd->latest_activity_timestamp))
                            {
                                /* Change the value of next_inactivity_check */
                                active_wd->next_inactivity_check = ((active_wd->timeout * PM_TICKS_PER_100MS) + 
                                                                    active_wd->latest_activity_timestamp);
                                                                                                      
                                /* Update the value of latest_activity_check for this processed element */
                                active_wd->latest_activity_check = active_wd->latest_activity_timestamp;                                                            
                                
                            }
                        }
                    }
                    
                } while (active_wd != (PM_WD *)tail_wd_ptr);
            }
            
            /**********************************
            Watchdog Task AND Expired WD Array 
            ***********************************/ 
            for (i = 0; i < PM_Expired_WD_Count; i++)
            {                
                if ((PM_Expired_WD_Count < (PM_WATCHDOG_ARRAY_SIZE - 1)) && (PM_Watchdog_Expired[i] != 0))
                {
                    /* First check if there are any watchdogs with no timeout */
                    if (PM_Watchdog_Expired[i]->timeout == 0)
                    {                        
                        for (watchdog_list_index = i;
                                ((watchdog_list_index < (PM_Expired_WD_Count - 1)) && (watchdog_list_index < (PM_WATCHDOG_ARRAY_SIZE - 1)));
                                    watchdog_list_index++)
                        {
                            /* Shift down the list down */
                            PM_Watchdog_Expired[watchdog_list_index] = PM_Watchdog_Expired[watchdog_list_index + 1];
                        }
                        
                        /* Clear the last element */
                        PM_Watchdog_Expired[PM_Expired_WD_Count - 1] = 0;
                        
                        /* Decrement expired watchdog count */
                        PM_Expired_WD_Count--;
                    }
                    else
                    {
                         /* If the latest_activity_check != latest_activity_timestamp activity has occurred*/
                         if ((PM_Watchdog_Expired[i]->latest_activity_check) !=
                             (PM_Watchdog_Expired[i]->latest_activity_timestamp))
                         {
                             
                            /* Change the value of next_inactivity_check */
                            PM_Watchdog_Expired[i]->next_inactivity_check = 
                                                     ((PM_Watchdog_Expired[i]->timeout * PM_TICKS_PER_100MS) + 
                                                      PM_Watchdog_Expired[i]->latest_activity_timestamp);
                                              
                            /* Update the structure */
                            active_wd = PM_Watchdog_Expired[i]; 
                            
                            /* Update the expired flag */
                            active_wd->expired_flag = NU_FALSE;
                              
                            /* Priority of this WD List is based on the value of next_inactivity_check */
                            /* The element with the lowest next_inactivity_check will have least priority */
                            /* Save the next_inactivity_check value as the element's priority */
                            active_wd->active_list.priority = PM_Watchdog_Expired[i]->next_inactivity_check;
                            
                            /* Place the active element in the list */                  
                            PMS_Priority_Place_On_List(&PM_Active_List_Head, &(active_wd->active_list));
                            
                            /* Increment active count */
                            PM_Active_WD_Count++;
                            
                            for (watchdog_list_index = i;
                                ((watchdog_list_index < (PM_Expired_WD_Count - 1)) && (watchdog_list_index < (PM_WATCHDOG_ARRAY_SIZE - 1)));
                                    watchdog_list_index++)
                            {
                                /* Shift down the list down */
                                PM_Watchdog_Expired[watchdog_list_index] = PM_Watchdog_Expired[watchdog_list_index + 1];
                            }
                            
                            /* Clear the last element */
                            PM_Watchdog_Expired[PM_Expired_WD_Count - 1] = 0;
                            
                            /* Decrement expired watchdog count */
                            PM_Expired_WD_Count--;
                            
                            /* Update the value of latest_activity_check for this processed element */
                            active_wd->latest_activity_check = active_wd->latest_activity_timestamp;
                            
                            /* Trace log */
                            T_WD_ACTIVE((VOID*)active_wd);
                            
                            /* Check if notification are on and if active type is set  */
                            if (active_wd->is_notifications & PM_ACTIVE_NOTIFICATIONS)
                            {
                                /* Assign the watchdog active notification to the event type. */
                                wd_event.pm_event_type = PM_ACTIVE_NOTIFICATIONS;

                                /* Assign the sender device ID into the watchdog notification structure. */
                                wd_event.pm_dev_id = active_wd->notification_dev;
                                
                                /* Send a notification to listeners now that system state change is successful */
                                (VOID)NU_EQM_Post_Event(&System_Eqm, (EQM_EVENT*)(&wd_event),
                                                        sizeof(PM_WD_NOTIFICATION), NU_NULL);
                            }
                            
                            /* WAKE UP ANY SUSPENDED THREADS SUSPENDED ON WATCHDOG ACTIVITY */
                            /* Release the semaphore which will cause the waiting tasks to resume */
                            if (active_wd->semaphore_ptr != NU_NULL)
                            {
                                (VOID)NU_Release_Semaphore(active_wd->semaphore_ptr);
                            }
                            
                         }
                    }       
                }
            } /* for */            
        }
       
        /* For every active watchdog element */
        if (PM_Active_WD_Count != 0)
        {           
            /* Get a pointer to the head of the active list */ 
            next_wd_ptr = PM_Active_List_Head;
            
            /* Check every element in the active list for a WD WITHOUT rollover flag */
            do
            {   
                /* Save the tail pointer */
                tail_wd_ptr = PM_Active_List_Head->previous;
                                 
                /* Save a pointer to the current element */
                active_wd = (PM_WD *)next_wd_ptr;
                
                /* Make sure we have a pointer to the next element in the list */
                next_wd_ptr = active_wd->active_list.next; 
                
                /* Check if this element has is_rollover set to false */
                if (active_wd->is_rollover == NU_FALSE)
                {
                    found = NU_TRUE;
                    break;
                }
                else
                {
                    found = NU_FALSE;
                }
                
            } while (active_wd != (PM_WD *)tail_wd_ptr);
            
            /* Get current system timestamp in ticks */
            current_timestamp = NU_Retrieve_Clock(); 
            
            /* Calculate rollover ticks */
            ticks_to_rollover = PM_CLOCK_MAX - current_timestamp;
            
            /* If a WD element with rollover flag cleared is found */
            /* System clock may or may not have rolled over at this point. We want to
               set the timer to the earliest value of next inactivity timecheck */
            if (found == NU_TRUE)
            {                                           
                /* If the next_inactivity_check is coming up soon then set a timer for us to wake up */
                if ((active_wd->next_inactivity_check - current_timestamp) > 0)
                {               
                    /* Reset the timer for the next wake-up */                
                    timer_reset_val = (active_wd->next_inactivity_check - current_timestamp);                                                                  
                                        
                    /* Stop timer for Watchdog Task */
                    (VOID)NU_Control_Timer(&WD_Timer, NU_DISABLE_TIMER);
                    
                    /* Reset the timer */
                    (VOID)NU_Reset_Timer(&WD_Timer, PMS_Watchdog_Timer, 
                                           timer_reset_val, 0, NU_ENABLE_TIMER);                                                                           
                }
                else
                {
                    /* next_inactivity_check time was in the past, so just process the element now */
                    reprocess = NU_TRUE;
                }
            }
            
            /* If there was no element with rollover flag cleared */
            /* This means that clock has not rolled over and reset and all active elements 
               have their next inactivity timecheck after the clock is reset. */
            if (found == NU_FALSE)
            {               
                /* Point to the head of the active WD element list */
                head_wd = (PM_WD *)PM_Active_List_Head;
                
                /* Update timer reset value */
                timer_reset_val = (head_wd->next_inactivity_check + ticks_to_rollover);
                
                /* Stop timer for Watchdog Task */
                (VOID)NU_Control_Timer(&WD_Timer, NU_DISABLE_TIMER);
                
                /* Reset the timer */
                (VOID)NU_Reset_Timer(&WD_Timer, PMS_Watchdog_Timer, 
                                       timer_reset_val, 0, NU_ENABLE_TIMER); 
                
            }
        }      
        else    /* There are no active watchdogs */
        {
             /* Stop timer for Watchdog Task */
            (VOID)NU_Control_Timer(&WD_Timer, NU_DISABLE_TIMER);
        }    
        
    } /* while1 */  
           
    /* No need to return to user mode because of the infinite loop.    */
    /* Returning to user mode will cause warnings with some compilers. */    
}
           
/***********************************************************************
*
*   FUNCTION
*
*       PMS_Watchdog_Timer
*
*   DESCRIPTION
*
*       Timer Routine Task. This routine resumes the main task on 
*       expiration and resets itself to expire at the lowest 
*       'next_inactivity_time_check' (head element on the active 
*       list).
*
*   INPUTS
*
*       argc                                unsigned value set at
*                                           task creation
*       argv                                void pointer set at task
*                                           creation
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID PMS_Watchdog_Timer(UINT32 id)
{
    /* Set an event flag to lift suspension on Main Task */
    (VOID)NU_Set_Events(&PM_WD_Process_Event, (UNSIGNED)PM_WD_WAKEUP_MAIN_TASK, (OPTION)NU_OR);
        
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Watchdog_Active_Cleanup
*
*   DESCRIPTION
*
*       If timeout == 0, element is deleted from the array
*
*   INPUTS
*
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID PMS_Watchdog_List_Cleanup(PM_WD *element)
{
    PM_WD    *remove_wd;
                            
    /* Delete this watchdog element */
    remove_wd = element;

    /* Now remove this element from active list */                                  
    PMS_Remove_From_List(&PM_Active_List_Head, &(remove_wd->active_list)); 
    
    /* Decrement the Active list count */
    PM_Active_WD_Count--;
    
    return;
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_Watchdog_Place_On_Active_List
*
*   DESCRIPTION
*
*       This function places the specified node after all other nodes on
*       the list of equal or greater priority.  Note that lower
*       numerical values indicate greater priority.
*
*   INPUTS
*
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static VOID PMS_Priority_Place_On_List(PMS_NODE **head, PMS_NODE *new_node)
{
    PMS_NODE         *search_ptr;            /* List search pointer       */

    /* Determine if the list in non-empty.  */
    if (*head)
    {

        /* Search the list to find the proper place for the new node.  */
        search_ptr =  (*head);

        /* Check for insertion before the first node on the list.  */
        if (search_ptr -> priority > new_node -> priority)
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
            search_ptr =  search_ptr -> next;
            while ((search_ptr -> priority <= new_node -> priority) &&
                   (search_ptr != (*head)))
            {

                /* Move along to the next node.  */
                search_ptr =  search_ptr -> next;
            }
        }

        /* Insert before search pointer.  */
        new_node -> previous           =  search_ptr -> previous;
        (new_node -> previous) -> next =  new_node;
        new_node -> next               =  search_ptr;
        (new_node -> next) -> previous =  new_node;
    }
    else
    {

        /* The list is empty, setup the head and the new node.  */
        (*head) =  new_node;
        new_node -> previous =  new_node;
        new_node -> next =      new_node;
    }
}


/***********************************************************************
*
*   FUNCTION
*
*       PMS_Remove_From_List
*
*   DESCRIPTION
*
*       This function removes the specified node from the specified
*       linked list.
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
static VOID PMS_Remove_From_List(PMS_NODE **head, PMS_NODE *node)
{

    /* Determine if this is the only node in the system.  */
    if (node -> previous == node)
    {

        /* Yes, this is the only node in the system.  Clear the node's
           pointers and the head pointer.  */
        (*head) = NU_NULL;
    }
    else
    {

        /* Unlink the node from a multiple node list.  */
        (node -> previous) -> next =  node -> next;
        (node -> next) -> previous =  node -> previous;

        /* Check to see if the node to delete is at the head of the
           list. */
        if (node == *head)
        {

            /* Move the head pointer to the node after.  */
            *head =  node -> next;

        }
    }
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */



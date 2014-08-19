/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
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
*       pms_dvfs_set_current_op.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for setting the current operating point
*
*   DATA STRUCTURES
*
*       PMS_Parked_Devices
*       PMS_Set_OP_Task
*       PMS_Set_OP_Semaphore
*       PMS_Set_OP_Queue
*       PMS_Set_OP_Queue_Data
*
*   FUNCTIONS
*
*       NU_PM_Set_Current_OP
*       PMS_DVFS_Set_OP
*       PMS_DVFS_Park_Drivers
*       PMS_DVFS_Resume_Drivers
*
*   DEPENDENCIES
*
*       power_core.h
*       dvfs.h
*       idle_scheduler.h
*       cpu_dvfs.h
*       hibernate.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/dvfs.h"
#include "os/services/power/core/inc/idle_scheduler.h"
#include "os/services/power/core/inc/hibernate.h"
#include "services/cpu_dvfs.h"
#include "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

/* EQM Instance. */
extern EQM_EVENT_QUEUE System_Eqm;
extern UINT8           PM_DVFS_OP_Count;
extern CS_NODE        *PM_DVFS_Device_List;
extern DV_DEV_HANDLE   PM_DVFS_CPU_Handle;
extern UINT32          PM_DVFS_CPU_Base;
extern UINT8           PM_DVFS_Current_OP;
extern UINT8           PM_DVFS_Minimum_OP;
extern PM_OP         **PM_DVFS_OP_Array;
extern UINT32          PM_DVFS_Device_Count;
extern UINT32          PM_DVFS_Smallest_Duration;
extern DV_DEV_ID       PM_DVFS_Device_ID;
extern PM_DVFS_REG    *PM_DVFS_Resume_Array[PM_MAX_DEV_CNT];
extern BOOLEAN         PM_DVFS_Transitions;

static INT             PMS_Parked_Devices;
NU_SEMAPHORE           PMS_Set_OP_Semaphore;
NU_QUEUE               PMS_Set_OP_Queue;
NU_TASK                PMS_Set_OP_Task;

/* Queue data is setup statically since dynamically allocating
   such a small space could return an error */
static UNSIGNED        PMS_Set_OP_Queue_Data[PM_SET_OP_QUEUE_SIZE];

static STATUS PMS_DVFS_Set_OP(UINT8 op_id);
static STATUS PMS_DVFS_Park_Drivers(VOID);
static STATUS PMS_DVFS_Resume_Drivers(VOID);

UINT8 Set_OP_Stack[PM_SET_OP_STACK_SIZE];

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Set_Current_OP
*
*   DESCRIPTION
*
*       This function attempts to transition the CPU into a specified
*       operating point
*
*   INPUT
*
*       op_id               this is the desired new operating point id
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_TRANSITION_FAILED DVFS service was unable to transition due
*                            to current peripheral activity
*       PM_REQUEST_BELOW_MIN A request to use an OP below a currently 
*                            requested minimum state
*       PM_INVALID_OP_ID    Provided OP is invalid
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*
*************************************************************************/
STATUS NU_PM_Set_Current_OP(UINT8 op_id)
{
    STATUS    pm_status = NU_SUCCESS;
    UNSIGNED  queue_value = 0;
    STATUS    status;
    UNSIGNED  size = 0;
    
    NU_SUPERV_USER_VARIABLES
       
    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();
    if (pm_status == NU_SUCCESS)
    {
        /* Determine if the OP is valid */
        if (op_id >= PM_DVFS_OP_Count)
        {
            pm_status = PM_INVALID_OP_ID;
        }
        /* Is this the same OP? */
        else if (PM_DVFS_Current_OP == op_id)
        {
            /* Already set just return success */
            pm_status = NU_SUCCESS;
        }
        /* Is this below the minimum? */
        else if (PM_DVFS_Minimum_OP > op_id)
        {
            /* Can't drop below the minimum */
            pm_status = PM_REQUEST_BELOW_MIN;
        }
        /* Check for transition lockout */
        else if (PM_DVFS_Transitions == NU_FALSE)
        {
            /* No transitions are allowed to occur */
            pm_status = PM_TRANSITION_FAILED;
        }
        else
        {
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Trace log */
            T_OP_TL_START();
                
            /* Obtain the Set OP semaphore, this guarantees only one
               caller may utilize the set OP thread */
            status = NU_Obtain_Semaphore(&PMS_Set_OP_Semaphore, NU_SUSPEND);
            if (status == NU_SUCCESS)
            {          
            
                /* Terminate the task regardless of its state */
                (VOID)NU_Terminate_Task(&PMS_Set_OP_Task);
        
                /* Reset the task with new parameters */
                status = NU_Reset_Task(&PMS_Set_OP_Task, (UNSIGNED)op_id, NU_NULL);
                if (status == NU_SUCCESS)
                {
                    /* Resume the task which will attempt the op change */
                    (VOID)NU_Resume_Task(&PMS_Set_OP_Task);
                }
                
                /* Wait on the queue to get the result */
                status = NU_Receive_From_Queue(&PMS_Set_OP_Queue, &queue_value,
                                               PM_SET_OP_QUEUE_SIZE, &size,
                                               NU_SUSPEND);
                if (status == NU_SUCCESS)
                {
                    /* Cast the value back to pm_status */
                    pm_status = (STATUS)queue_value;
                }
                else
                {
                    /* Update with an unexpected error */
                    pm_status = PM_UNEXPECTED_ERROR;
                }
                
                /* Release the Set OP semaphore */
                status = NU_Release_Semaphore(&PMS_Set_OP_Semaphore);
            }
             
            /* Trace log */
            T_OP_TL_STOP();
            
            /* Return to user mode */
            NU_USER_MODE();
            
            /* if any errors from semaphore or queue usage
               leads to an error return an unexpected error */
            if (status != NU_SUCCESS)
            {
                pm_status = PM_UNEXPECTED_ERROR;
            }
            
        }
    }

    return (pm_status);
}

/***********************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Set_OP_Task_Entry
*
*   DESCRIPTION
*
*       This task only runs when a call to set OP is made.  This 
*       task is non-preemptive and may only be called one at a time.
*       It will do the set OP operation and return the result through
*       a queue and run to completion.  The task will need to be reset
*       with new arguments from the set current OP API.
*
*   INPUTS
*
*       argc - Argument count.  Values in the range [0..255] will be used
*              to perform an OP transition.  Values in the range
*              [256..512] will be used to perform a hibernate level
*              transition.  All other values are considered invalid.
*
*       argv - Argument vector.  Not used.
*
*   OUTPUTS
*
*       Returns the value through a queue to the set OP API
*
***********************************************************************/
VOID PMS_DVFS_Set_OP_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS           pm_status = NU_SUCCESS;
    UNSIGNED         queue_return = 0;
    STATUS           status;
    UINT8            op_id;
    STATUS           pm_restore_status;
    CPU_DVFS_FROM_TO from_to;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
    
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)
    /* Determine if the new OP is a hibernate level */
    if (argc >= PM_DVFS_SET_OP_THD_LEVEL_OFFSET)
    {
        /* Transition to new hibernate level */
        (VOID)PMS_Hibernate_Enter((UINT8)(argc - PM_DVFS_SET_OP_THD_LEVEL_OFFSET));
    }
    else
#endif 
    {
        /* Get the OP from the parameters */
        op_id = (UINT8)argc;

        /* Determine if any drivers need to be notified */
        if (PM_DVFS_Device_List != NU_NULL)
        {
            /* Set up the ioctl structure */
            from_to.pm_op_from = PM_DVFS_Current_OP;
            from_to.pm_op_to = op_id;
    
            /* Test if a transition is possible, read the
               transition time from the CPU driver and compare
               with the smallest remaining duration time */
            status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_FROM_TO), 
                                   &from_to, sizeof(from_to));
            if ((status == NU_SUCCESS) && 
                ((PM_DVFS_Smallest_Duration > from_to.pm_time) || (PM_DVFS_Smallest_Duration == 0)))
            {
                /* If the time is available make the switch */
                /* Call DVFS notification system */
                pm_status = PMS_DVFS_Park_Drivers();
    
                /* If notification was successful change OP */
                if (pm_status == NU_SUCCESS)
                {
#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))
                    PMS_Update_Cpu_Utilization_Counters(NU_FALSE);
#endif                
                    pm_status = PMS_DVFS_Set_OP(op_id);
                }
    
                /* Call dvfs restoration system rather we had a failure
                   or not; the drivers need to be returned to normal states */
                pm_restore_status = PMS_DVFS_Resume_Drivers();
    
                /* If a failure only occurred in restore return that error */
                if ((pm_restore_status != NU_SUCCESS) && (pm_status == NU_SUCCESS))
                {
                    pm_status = pm_restore_status;
                }
            }
            else
            {
                /* Cannot change frequency, update error status */
                pm_status = PM_TRANSITION_FAILED;
            }
        }
        else
        {
            /* Simply set the OP */
            pm_status = PMS_DVFS_Set_OP(op_id);
        }
        
        /* Trace log */ 
        T_OP_TRANS(PM_DVFS_Current_OP, pm_status);
        
        /* Determine if this is called from initialization,
           if called before initialization no need to set the
           queue.  The only time this may be called is to set
           a custom intial OP during DVFS initialization */
        if (PMS_DVFS_Status_Check() == NU_SUCCESS)
        {
            /* Put return value is element of appropriate size */
            queue_return = (UNSIGNED)pm_status;
            
            /* Return the value in the queue */
            (VOID)NU_Send_To_Queue(&PMS_Set_OP_Queue, &queue_return,
                                   PM_SET_OP_QUEUE_SIZE, NU_SUSPEND);
        }
    }
    
    /* Return to user mode */
    NU_USER_MODE();
    
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Set_OP
*
*   DESCRIPTION
*
*       This function calls the CPU driver to set the new operating
*       point and then sends out a notification that an operating
*       point has changed.
*
*   INPUT
*
*       op_id               New operating point
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_TRANSITION_FAILED DVFS service was unable to transition due
*                            to current peripheral activity
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*
*************************************************************************/
static STATUS PMS_DVFS_Set_OP(UINT8 op_id)
{
    STATUS        pm_status = NU_SUCCESS;
    STATUS        status;
    STATUS        eqm_status;
    UINT8         driver_op_id;
    UINT8         old_op;
    PM_OP_NOTIFICATION op_message;

    /* Get the id the driver uses, as it may differ */
    driver_op_id = PM_DVFS_OP_Array[op_id] -> pm_op_id;

    /* Notify the CPU driver that an OP change needs to occur */
    status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_SET_OP), 
                           &driver_op_id, sizeof(driver_op_id));

    /* Update the new op in the global structure */
    if (status == NU_SUCCESS)
    {
        /* Save the old OP for device notification service */
        old_op = PM_DVFS_Current_OP;

        /* Update the new OP */
        PM_DVFS_Current_OP = op_id;

        /* Store OP information in notification message */
        op_message.pm_event_type = PM_OP_CHANGE;
        op_message.pm_old_op = old_op;
        op_message.pm_new_op = op_id;

        /* Send a notification to listeners to inform about OP change */
        eqm_status = NU_EQM_Post_Event(&System_Eqm, (EQM_EVENT*)(&op_message), sizeof(op_message), NU_NULL);

        /* Check for unexpected failure in notification */
        if (eqm_status != NU_SUCCESS)
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }
    else
    {
        /* If the ioctl call failed transition failed for some reason */
        pm_status = PM_TRANSITION_FAILED;
    }

    return(pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Park_Drivers
*
*   DESCRIPTION
*
*       This function traverses the park list and notifies each device
*       that is currently active that it needs to park.  If any device
*       returns failure the transition is cancelled and the parked devices
*       will be marked as needing to be resumed.
*
*   INPUT
*
*       NONE
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_TRANSITION_FAILED DVFS service was unable to transition due
*                            to current peripheral activity
*
*************************************************************************/
static STATUS PMS_DVFS_Park_Drivers(VOID)
{
    STATUS       pm_status = NU_SUCCESS;
    INT          list_index;
    CS_NODE     *node_ptr;
    PM_DVFS_REG *reg_entry;

    /* Initialize the node list */
    node_ptr = PM_DVFS_Device_List;
    PMS_Parked_Devices = 0;

    /* Loop through the registered devices */
    for (list_index = 0; ((list_index < PM_DVFS_Device_Count) && (pm_status == NU_SUCCESS) && (PM_DVFS_Transitions == NU_TRUE)); list_index++)
    {
        /* Read the registry entry */
        reg_entry = (PM_DVFS_REG *)node_ptr;

        /* Notify the drivers to stop while OP is changed if notification is on */
        if (reg_entry -> pm_notify == PM_NOTIFY_ON)
        {
            pm_status = reg_entry -> pm_notify_func(reg_entry -> pm_instance, PM_PARK);

            if (pm_status == NU_SUCCESS)
            {
                /* Save the registry entry on the resume array */
                PM_DVFS_Resume_Array[PMS_Parked_Devices] = reg_entry;
                PMS_Parked_Devices++;
            }
        }

        /* Position the node pointer to the previous node
           as these are stored from smallest duration
           to largest.  */
        node_ptr = node_ptr -> cs_previous;
    }
    if (pm_status != NU_SUCCESS)
    {
        /* Transition failed */
        pm_status = PM_TRANSITION_FAILED;
    }

    return(pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Resume_Drivers
*
*   DESCRIPTION
*
*       This function traverses the resume list and notifies each device
*       that is currently active and parked that it is safe to resume.
*       If any device returns failure the transition is cancelled and the 
*       parked devices will be marked as needing to be resumed.
*
*   INPUT
*
*       NONE
*
*   OUTPUT
*
*       NU_SUCCESS           Successful transition
*       PM_TRANSITION_FAILED DVFS service was unable to transition due
*                            to current peripheral activity
*
*************************************************************************/
static STATUS PMS_DVFS_Resume_Drivers(VOID)
{
    STATUS       pm_status = NU_SUCCESS;
    INT          list_index;
    PM_DVFS_REG *reg_entry;
    BOOLEAN      error = NU_FALSE;

    /* Loop through the registered devices */
    for (list_index = PMS_Parked_Devices; list_index > 0; list_index--)
    {
        /* Read the registry entry */
        reg_entry =  PM_DVFS_Resume_Array[(list_index - 1)];

        /* Notify the drivers to resume */
        if (reg_entry != NU_NULL)
        {
            /* Call the resume function */
            pm_status = reg_entry -> pm_notify_func(reg_entry -> pm_instance, PM_RESUME);
            
            /* Remove the device from the list */
            PM_DVFS_Resume_Array[(list_index - 1)] = NU_NULL;
            if (pm_status != NU_SUCCESS)
            {
                /* Indicate that an error has occurred
                   but keep processing the resumes */
                error = NU_TRUE;
            }
        }
    }
    
    /* No more devices to resume */
    PMS_Parked_Devices = 0;

    if (error == NU_TRUE)
    {
        /* Transition failed in at least one device */
        pm_status = PM_TRANSITION_FAILED;
    }

    return(pm_status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Set_OP_Initialize
*
*   DESCRIPTION
*
*       This function creates the objects needed to run a task
*       specified for updating the OP of the system.
*
*   INPUT
*
*       mem_pool_ptr        Pointer to use for allocating stack and
*                           queue space
*
*   OUTPUT
*
*       NU_SUCCESS           Successful transition
*       PM_UNEXPECTED_ERROR  One of the set OP objects was not created
*
*************************************************************************/
STATUS PMS_DVFS_Set_OP_Initialize(NU_MEMORY_POOL *mem_pool_ptr)
{
    STATUS  pm_status = NU_SUCCESS;
    STATUS     status;

    /* Create the semaphore */
    status = NU_Create_Semaphore(&PMS_Set_OP_Semaphore, "DVFS OP", 1, NU_FIFO);
        
    /* Check to see if previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Create cleanup queue */
        status = NU_Create_Queue(&PMS_Set_OP_Queue, "DVFS OP", &PMS_Set_OP_Queue_Data[0],
                                 PM_SET_OP_QUEUE_SIZE, NU_FIXED_SIZE, 
                                 PM_SET_OP_QUEUE_SIZE, NU_FIFO);
    }
    
    /* Create Set OP task with no preemption and not started */
    if (status == NU_SUCCESS)
    {
        status = NU_Create_Task(&PMS_Set_OP_Task, "DVFS OP", PMS_DVFS_Set_OP_Task_Entry,
                                0, NU_NULL, Set_OP_Stack, PM_SET_OP_STACK_SIZE, PM_SET_OP_TASK_PRIORITY,
                                PM_SET_OP_TASK_TIMESLICE, NU_NO_PREEMPT, NU_NO_START);                                
        if (status == NU_SUCCESS)
        {
            /* Bind this task to the kernel module */
            (VOID)NU_BIND_TASK_TO_KERNEL(&PMS_Set_OP_Task);
        }
    }
    
    /* If there were any error return error to 
       indicate initialization failed */
    if (status != NU_SUCCESS)
    {
        pm_status = PM_UNEXPECTED_ERROR;
    }
    
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */



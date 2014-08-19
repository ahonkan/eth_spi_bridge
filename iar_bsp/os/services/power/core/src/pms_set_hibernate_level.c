/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
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
*       pms_set_hibernate_level.c
*
*   COMPONENT
*
*       Hibernate
*
*   DESCRIPTION
*
*       Contains all functionality for setting the hibernate level.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Set_Hibernate_Level
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       power_core.h
*       dvfs.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/dvfs.h"
#include "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

/* External references */
extern NU_SEMAPHORE         PMS_Set_OP_Semaphore;
extern NU_TASK              PMS_Set_OP_Task;
extern NU_QUEUE             PMS_Set_OP_Queue;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Set_Hibernate_Level
*
*   DESCRIPTION
*
*       This function attempts to transition the CPU into a specified
*       hibernate level.
*
*       NOTE: This function uses the DVFS set OP thread to perform the
*             transition to a hibernate level.
*
*   INPUT
*
*       level - This is the new hibernate level to transition to.
*
*   OUTPUT
*
*       NU_SUCCESS - This indicates a successful transition to the new
*                    hibernate level.
*
*       PM_UNEXPECTED_ERROR - This indicates an unexpected error has
*                             occurred.
*
*       PM_NOT_INITIALIZED - This indicates the power services are not
*                            fully initialized.
*
*************************************************************************/
STATUS NU_PM_Set_Hibernate_Level (UINT8 level)
{
    STATUS    status = NU_SUCCESS;
    STATUS    pm_status = NU_SUCCESS;
    UNSIGNED  queue_value = 0;
    UNSIGNED  size = 0;

    NU_SUPERV_USER_VARIABLES

    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();
    if (pm_status == NU_SUCCESS)
    {
        /* Determine if the level is valid */
        if ((level != NU_STANDBY) &&
            (level != NU_DORMANT))
        {
            /* ERROR: Invalid level parameter. */
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }

    if (pm_status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Trace log */
        T_HIB_TL_START();

        /* Obtain the Set OP semaphore, this guarantees only one caller
           may utilize the set OP thread. */
        status = NU_Obtain_Semaphore(&PMS_Set_OP_Semaphore,
                                     NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Terminate the task regardless of its state. */
            (VOID)NU_Terminate_Task(&PMS_Set_OP_Task);

            /* Reset the task with new parameters. */
            status = NU_Reset_Task(&PMS_Set_OP_Task,
                                   (UNSIGNED)(level + PM_DVFS_SET_OP_THD_LEVEL_OFFSET),
                                   NU_NULL);
            if (status == NU_SUCCESS)
            {
                /* Resume the task which will attempt the hibernate. */
                (VOID)NU_Resume_Task(&PMS_Set_OP_Task);
            }

            /* Wait on the queue to get the result */
            status = NU_Receive_From_Queue(&PMS_Set_OP_Queue,
                                           &queue_value,
                                           PM_SET_OP_QUEUE_SIZE,
                                           &size,
                                           NU_SUSPEND);
            if (status == NU_SUCCESS)
            {
                /* Cast the value back to pm_status */
                pm_status = (STATUS)queue_value;
            }
            else
            {
                /* ERROR: Error returned from the set OP thread. */
                pm_status = PM_UNEXPECTED_ERROR;
            }

            /* Release the Set OP semaphore */
            status = NU_Release_Semaphore(&PMS_Set_OP_Semaphore);
        }
        
        /* Trace log */
        T_HIB_TL_STOP();

        /* Return to user mode */
        NU_USER_MODE();

        /* If any errors from semaphore or queue usage
           leads to an error return an unexpected error */
        if (status != NU_SUCCESS)
        {
            /* ERROR: Internal synchronization errors occurred. */
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }
    
    /* Trace log */
    T_HIB_ENTER(level, pm_status);

    return (pm_status);
}

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */

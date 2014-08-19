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
*       pms_error_init.c
*
*   COMPONENT
*
*       Error Handling
*
*   DESCRIPTION
*
*       Initialization for the error component in power management
*       services
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMS_Error_Initialize
*
*   DEPENDENCIES
*
*       power_core.h
*       error_management.h
*       string.h
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/error_management.h"
#include    <string.h>

/* Global structures used in this file */
extern NU_QUEUE PMS_Error_Queue;

VOID PMS_Default_Error_Handler(STATUS pm_status, VOID *thread, VOID *info_ptr, UINT32 length);

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Error_Initialize
*
*   DESCRIPTION
*
*       This function initializes the error component of the PMS.
*
*   INPUT
*
*       mem_pool_ptr
*
*   OUTPUT
*
*       NU_SUCCESS          Successful completion
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*       Return value from NU_PM_Register_Error_Handler
*
*************************************************************************/
STATUS PMS_Error_Initialize(NU_MEMORY_POOL* mem_pool_ptr)
{
    STATUS  pm_status;
    STATUS  status;
    VOID    *queue_area;

    NU_SUPERV_USER_VARIABLES
    
    /* Set the default error handler */
    pm_status = NU_PM_Register_Error_Handler(PMS_Default_Error_Handler, NU_NULL);

    /* Create the mailbox for passing messages to the
       error task */
    if (pm_status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Allocate space for the queue */
        status = NU_Allocate_Memory(mem_pool_ptr, &queue_area, PM_ERROR_QUEUE_AREA_SIZE, NU_NO_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Clear the error message blocks */
            memset(queue_area, 0, PM_ERROR_QUEUE_AREA_SIZE);

            /* Create the queue for sending messages to the error task */
            status = NU_Create_Queue(&PMS_Error_Queue, "PMERR", queue_area, PM_ERROR_QUEUE_AREA_SIZE,
                                    NU_FIXED_SIZE, sizeof(PM_ERROR), NU_FIFO);
        }
        
        /* Return to user mode */
        NU_USER_MODE();

        /* if there was a failure anywhere in queue creation report
           unexpected error */
        if (status != NU_SUCCESS)
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }
    }

    return (pm_status);
}

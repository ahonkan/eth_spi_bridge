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
*        pms_sched_init.c
*
*   COMPONENT
*
*        SCHED - PMS idle scheduling and CPU utilization components
*
*   DESCRIPTION
*
*        This file performs initializations
*
*   DATA STRUCTURES
*
*        None
*
*   FUNCTIONS
*
*        PMS_CPU_Presched_Initialize
*        PMS_CPU_Post_Sched_Initialize
*        CPU_Device_Register_Callback
*
*   DEPENDENCIES
*
*        power_core.h
*        idle_scheduler.h
*        tick_suppression.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "kernel/dev_mgr.h"
#include    "services/cpu_idle.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/idle_scheduler.h"
#include    "os/services/power/core/inc/tick_suppression.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE)

/* Call back function for CPU_IDLE_CLASS_LABEL device registration event. */
static STATUS CPU_Device_Register_Callback(DV_DEV_ID device, VOID *context);

/* Handle to the currently open CPU driver */
static DV_DEV_HANDLE                PM_SCHED_CPU_Handle;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

extern PM_CPU_UTILIZATION_COUNTERS  PM_CPU_Util_Counters;

#endif  /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)) */

/*************************************************************************
*
*   FUNCTION
*
*       PMS_CPU_Presched_Initialize
*
*   DESCRIPTION
*
*       This function initializes the idle and wakeup scheduling component
*       for Nucleus PLUS and setting the CPU counters to default values.
*       This function will provide default idle and wakeup
*       function pointers that will be executed before the scheduler starts.
*
*   INPUT
*
*       mem_pool            - Memory pool
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*
*************************************************************************/
STATUS PMS_CPU_Presched_Initialize(NU_MEMORY_POOL* mem_pool)
{
    STATUS status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set ISR default wake up handle */
    ESAL_GE_ISR_EXECUTE_HOOK_SET(PMS_TS_Wakeup);

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE))

    /* Initialize CPU utilization counters and flags */
    PM_CPU_Util_Counters.idle_time              = 0;
    PM_CPU_Util_Counters.total_time             = 0;
#if(ESAL_PR_TMR_OS_COUNT_DIR == ESAL_COUNT_DOWN)
    PM_CPU_Util_Counters.old_hw_tmr_value       = SINGLE_TICK;
#else
    PM_CPU_Util_Counters.old_hw_tmr_value       = 0;
#endif /* ESAL_PR_TMR_OS_COUNT_DIR == ESAL_COUNT_DOWN */
    PM_CPU_Util_Counters.hw_idle_tmr_flag       = NU_FALSE;

#endif  /* (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_CPU_USAGE == NU_TRUE)) */

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_CPU_Post_Sched_Initialize
*
*   DESCRIPTION
*
*       This function adds the notification for any CPU_IDLE_CLASS_LABEL
*       device registration event.
*
*   INPUT
*
*       mem_pool           - Memory pool
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_CPU_Post_Sched_Initialize(NU_MEMORY_POOL* mem_pool)
{
    DV_DEV_LABEL       cpu_device_label = {CPU_IDLE_CLASS_LABEL};
    DV_LISTENER_HANDLE listener_id;

    /* Call DM API to add callback for device register event. We are not
     * interested in un-registration at the moment. */
    (VOID)DVC_Reg_Change_Notify(&cpu_device_label,
                                DV_GET_LABEL_COUNT(cpu_device_label),
                                &CPU_Device_Register_Callback,
                                NU_NULL,
                                NU_NULL,
                                &listener_id);
}

/*************************************************************************
*
*   FUNCTION
*
*       CPU_Device_Register_Callback
*
*   DESCRIPTION
*
*       This function performs all the post initialization from device
*       discovery task context, which invokes this callback when a CPU
*       driver is registered. It also opens the device, get the idle
*       and wakeup and sets up the CPU idle and wakeup based on previously
*       registered drivers.
*
*   INPUT
*
*       device                              Newly registered Device ID.
*       context                             Any context from caller.
*
*   OUTPUT
*
*       NU_SUCCESS                          Function returns success.
*       (Device Manager error values)
*
*************************************************************************/
static STATUS CPU_Device_Register_Callback(DV_DEV_ID device, VOID *context)
{
    STATUS            status;
    UINT32            pm_sched_cpu_base;
    UINT32            cpu_wakeup_addr;
    UINT32            cpu_idle_addr;
    DV_DEV_LABEL      cpu_class_id = {CPU_IDLE_CLASS_LABEL};
    DV_IOCTL0_STRUCT  ioctl0;

    /* Open the CPU driver for CPU Idle and wakeup usage, save the handle for later usage */
    status = DVC_Dev_ID_Open(device, &cpu_class_id, DV_GET_LABEL_COUNT(cpu_class_id), &PM_SCHED_CPU_Handle);
    if (status == NU_SUCCESS)
    {
        /* Get ioctl base */
        ioctl0.label = cpu_class_id;
        status = DVC_Dev_Ioctl(PM_SCHED_CPU_Handle, DV_IOCTL0, &ioctl0, sizeof(ioctl0));
        if (status == NU_SUCCESS)
        {
             /* Update the saved ioctl base */
            pm_sched_cpu_base = ioctl0.base;

            /* Get the CPU idle from the driver */
            status = DVC_Dev_Ioctl(PM_SCHED_CPU_Handle, (pm_sched_cpu_base + CPU_IDLE_IOCTL_GET_IDLE),&cpu_idle_addr, sizeof(cpu_idle_addr));

            if (status == NU_SUCCESS)
            {
                /* Get the CPU wakeup from the driver */
                status = DVC_Dev_Ioctl(PM_SCHED_CPU_Handle, (pm_sched_cpu_base + CPU_IDLE_IOCTL_GET_WAKEUP),&cpu_wakeup_addr, sizeof(cpu_wakeup_addr));
                if (status == NU_SUCCESS)
                {
                    /* Set the CPU idle and wakeup. If error occurs, default CPU idle and wake up will be used instead of the
                    one from the CPU driver.  */
                    (VOID)NU_PM_CPU_Set_CPU_Idle_Fn( (VOID(*)(VOID))(cpu_wakeup_addr),(VOID(*)(UINT32,UINT32))(cpu_idle_addr));
                }
            }
        }
    }

    return status;
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_IDLE == NU_TRUE) */


